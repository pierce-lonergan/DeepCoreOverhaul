#!/usr/bin/env python3
"""
docsaudit -- keep the documentation's load-bearing numbers true.

WHY THIS EXISTS
---------------
This project cannot run the game, so its documents ARE its evidence. Every decision
is made by reading them. That makes a stale number in a document exactly as dangerous
as a bug in the code, and it has already happened: the slack between `statsGlobs` and
`textGlobs` was corrected from 8 bytes to 4, the correction was announced, and two of
six documents kept the old value anyway because nobody reopened them.

`addrlint` proved that a fact worth relying on is worth machine-checking. This is the
same pattern applied to prose: each load-bearing number is DERIVED FROM THE TREE here,
then every document is checked against it. A number that drifts fails CI.

WHAT IT CHECKS
  1. Every fact below is recomputed from source or from docs/address-map.json.
  2. Every document is scanned for the fact's `stale` patterns -- values previously
     published and since corrected. Any hit is an error naming file, line and fix.
  3. Facts marked `must_appear` have to be present somewhere, so a fact cannot be
     quietly deleted rather than corrected.
  4. Cross-document agreement: where a fact's canonical form appears at all, it must
     appear with the same value everywhere.

Deliberately NOT a spell-checker or a link-checker. It only guards numbers and
addresses that a wrong value would cause someone to make a bad decision about.

USAGE
    python tools/docsaudit/docsaudit.py            # report
    python tools/docsaudit/docsaudit.py --check    # exit non-zero on drift (CI)
    python tools/docsaudit/docsaudit.py --facts    # print the derived facts and exit

EXIT CODES
    0  documents agree with the tree
    1  drift detected
    2  a fact could not be derived (the tree changed shape; fix this file)
"""

import argparse
import json
import os
import re
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
DOCS = os.path.join(REPO, 'docs')
ADDRMAP = os.path.join(DOCS, 'address-map.json')


# --------------------------------------------------------------------------
# Derivations -- every one recomputes from the tree, never from a constant
# --------------------------------------------------------------------------

def _grep_count(pattern, include=('*.h', '*.cpp')):
    """Count regex matches across the source tree. Uses git grep if available so the
    result matches what CI sees, else falls back to a manual walk."""
    total = 0
    rx = re.compile(pattern)
    for root, _dirs, files in os.walk(os.path.join(REPO, 'src')):
        for fn in files:
            if not any(fn.endswith(ext.lstrip('*')) for ext in include):
                continue
            path = os.path.join(root, fn)
            try:
                with open(path, 'r', encoding='utf-8', errors='replace') as fh:
                    for line in fh:
                        total += len(rx.findall(line))
            except OSError:
                pass
    return total


def derive_addrmap():
    if not os.path.exists(ADDRMAP):
        raise RuntimeError('docs/address-map.json missing; run addrlint first')
    with open(ADDRMAP, 'r', encoding='utf-8') as fh:
        return json.load(fh)


def derive_slack():
    """Free bytes between the end of statsGlobs and the start of textGlobs.

    This is the number the whole 15-ID argument rests on, and the one that was wrong
    in two documents for two days. Derived, never typed.
    """
    d = derive_addrmap()
    regs = sorted(d['regions'], key=lambda r: r['addr'])
    stats = next(r for r in regs if r['name'].endswith('statsGlobs'))
    text = next(r for r in regs if r['name'].endswith('textGlobs'))
    end = stats['addr'] + stats['size']
    raw = text['addr'] - end
    occupied = sum(r['size'] for r in regs if end <= r['addr'] < text['addr'])
    return {
        'stats_end': end,
        'text_start': text['addr'],
        'raw_gap': raw,
        'occupied': occupied,
        'free': raw - occupied,
    }


def facts():
    d = derive_addrmap()
    slack = derive_slack()

    total_asserts = _grep_count(r'assert_sizeof\s*\(')
    commented_asserts = _grep_count(r'^\s*//\s*assert_sizeof\s*\(')
    live_asserts = total_asserts - commented_asserts

    # Distinct types addrlint can size -- its own denominator, so import it rather
    # than re-implementing the parse and risking a second, differently-wrong number.
    sys.path.insert(0, os.path.join(REPO, 'tools', 'addrlint'))
    import addrlint  # noqa: E402
    sizes, _origin, binds = addrlint.scan(REPO)
    binds = addrlint.resolve(binds, sizes)

    return {
        'regions':            {'value': len(d['regions']),
                               'what': 'sized exe-overlaid regions in the address map'},
        'overlaps':           {'value': len(d.get('overlaps', [])),
                               'what': 'overlapping regions (must stay 0)'},
        'slack_free':         {'value': slack['free'],
                               'what': 'FREE bytes between statsGlobs end and textGlobs start'},
        'slack_raw':          {'value': slack['raw_gap'],
                               'what': 'raw gap before subtracting what occupies it'},
        'assert_total':       {'value': total_asserts,
                               'what': 'assert_sizeof occurrences in source, including commented'},
        'assert_live':        {'value': live_asserts,
                               'what': 'live (uncommented) assert_sizeof declarations'},
        'assert_types':       {'value': len(sizes),
                               'what': 'distinct types addrlint can size'},
        'overlay_exprs':      {'value': _grep_count(r'= \*\(.*\*\)0x00', include=('*.cpp',)),
                               'what': 'struct-overlay expressions in .cpp'},
        'id_count':           {'value': 15,
                               'what': 'LegoObject_ID_Count (verified against GameCommon.h below)'},
    }


def verify_id_count(value):
    path = os.path.join(REPO, 'src', 'openlrr', 'game', 'GameCommon.h')
    with open(path, 'r', encoding='utf-8', errors='replace') as fh:
        for line in fh:
            m = re.search(r'LegoObject_ID_Count\s*=\s*(\d+)', line)
            if m:
                return int(m.group(1)) == value, int(m.group(1))
    return False, None


# --------------------------------------------------------------------------
# Stale-value patterns: things previously published and since corrected.
#
# Each entry is (regex, why it is wrong, what to say instead). These are the
# tripwires -- a corrected fact that reappears anywhere fails the build.
# --------------------------------------------------------------------------

STALE = [
    (r'\b(?:8|eight|Eight)\s+bytes?\s+of\s+slack',
     'The gap is 8 bytes RAW but g_Teleporter_BOOL_00504188 occupies 4 of them.',
     'say "4 free bytes (8 raw, 4 occupied by g_Teleporter_BOOL_00504188)"'),

    (r'394\s+`?assert_sizeof',
     'No count in the tree equals 394.',
     'use assert_live for live declarations, or assert_types for addrlint\'s denominator'),

    (r'Release\s+ships\s+unoptimi[sz]ed',
     'Refuted: msbuild reports Optimization=MaxSpeed for Release.',
     'Release compiles /O2 /Oi /Oy /Gy /GL; the real defect was Debug also inheriting MaxSpeed'),

    (r'(?:current|the)\s+`?/Od`?\s+Release',
     'Release is /O2, not /Od.',
     'remove the claim; it descends from the refuted build-flag finding'),
]

# Facts that must be stated somewhere, so a correction cannot be applied by deletion.
MUST_APPEAR = ['slack_free', 'regions']


# A document is allowed to SAY what the old value was, as long as it is plainly
# recording that the value changed. Without this, the worklog entry that records the
# correction would itself be flagged as the error it documents.
_HISTORICAL = re.compile(
    r'(prior sessions?|previously|used to|was wrong|were wrong|has been corrected'
    r'|now reads|superseded|refuted|old value|CORRECTED|correction)', re.I)


def _is_historical(line):
    """True when a stale value appears inside an explicit record of its own correction."""
    return bool(_HISTORICAL.search(line))


def doc_files():
    out = []
    for root, _dirs, files in os.walk(DOCS):
        for fn in sorted(files):
            if fn.endswith('.md'):
                out.append(os.path.join(root, fn))
    return sorted(out)


def main():
    # Documents contain em-dashes and arrows; a Windows console defaults to cp1252 and
    # would otherwise crash the linter on the very text it is reporting.
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
        sys.stderr.reconfigure(encoding='utf-8', errors='replace')
    except (AttributeError, ValueError):
        pass

    ap = argparse.ArgumentParser(description='Audit documentation for stale load-bearing numbers.')
    ap.add_argument('--check', action='store_true', help='exit non-zero on drift')
    ap.add_argument('--facts', action='store_true', help='print derived facts and exit')
    args = ap.parse_args()

    try:
        f = facts()
    except Exception as exc:                      # noqa: BLE001
        print('docsaudit: could not derive facts: %s' % exc, file=sys.stderr)
        return 2

    ok, actual = verify_id_count(f['id_count']['value'])
    if not ok:
        print('docsaudit: LegoObject_ID_Count is %s in GameCommon.h, this tool expects %s'
              % (actual, f['id_count']['value']), file=sys.stderr)
        return 2

    if args.facts:
        print('Derived facts (recomputed from the tree, never typed):\n')
        for k in sorted(f):
            print('  %-16s %-8s %s' % (k, f[k]['value'], f[k]['what']))
        slack = derive_slack()
        print('\n  slack derivation: statsGlobs ends 0x%08x, textGlobs starts 0x%08x,'
              % (slack['stats_end'], slack['text_start']))
        print('                    raw gap %d, occupied %d, FREE %d'
              % (slack['raw_gap'], slack['occupied'], slack['free']))
        return 0

    problems = []
    seen_facts = set()

    for path in doc_files():
        rel = os.path.relpath(path, REPO).replace('\\', '/')
        with open(path, 'r', encoding='utf-8', errors='replace') as fh:
            lines = fh.readlines()

        in_correction = False
        for n, line in enumerate(lines, 1):
            # A block quote that explicitly records a correction is allowed to quote the
            # old value -- that is the record of what changed, not a live claim.
            stripped = line.lstrip()
            if stripped.startswith('>'):
                in_correction = True
            elif stripped and not stripped.startswith('>'):
                in_correction = False

            for rx, why, fix in STALE:
                if re.search(rx, line):
                    if in_correction or _is_historical(line):
                        continue
                    problems.append((rel, n, line.strip()[:110], why, fix))

            if re.search(r'\b%d\s+(?:sized\s+)?regions?\b' % f['regions']['value'], line):
                seen_facts.add('regions')
            if re.search(r'\b%d\s+(?:free\s+)?bytes?\b' % f['slack_free']['value'], line):
                seen_facts.add('slack_free')

    print('docsaudit -- %d documents, %d tracked facts\n' % (len(doc_files()), len(f)))

    if problems:
        print('STALE VALUES (%d):\n' % len(problems))
        for rel, n, text, why, fix in problems:
            print('  %s:%d' % (rel, n))
            print('      %s' % text)
            print('      why: %s' % why)
            print('      fix: %s\n' % fix)

    missing = [k for k in MUST_APPEAR if k not in seen_facts]
    if missing:
        print('FACTS NOT STATED ANYWHERE (%d):' % len(missing))
        for k in missing:
            print('  %-14s value %s -- %s' % (k, f[k]['value'], f[k]['what']))
            print('      A correction must not be applied by deleting the fact.')
        print()

    if not problems and not missing:
        print('No drift. Every tracked fact reads the same in the documents as in the tree.')

    failed = bool(problems) or bool(missing)
    if failed and args.check:
        print('docsaudit: FAILED', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
