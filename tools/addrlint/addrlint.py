#!/usr/bin/env python3
"""
addrlint -- address-space linter for DeepCoreOverhaul.

WHY THIS EXISTS
---------------
OpenLRR does not own its own global state. Dozens of structs are *references
overlaid onto the original 1999 executable's data segment* at hardcoded
addresses, e.g.

    LegoRR::Stats_Globs & LegoRR::statsGlobs = *(LegoRR::Stats_Globs*)0x00503bd8;
    assert_sizeof(Stats_Globs, 0x5b0);

The 1999 machine code still reads and writes those same bytes at those same
offsets. So growing any such struct by even one byte silently corrupts whatever
lives next door -- and because we cannot run the game, that corruption would be
invisible until a player hit it.

This tool reconstructs the exe's data-segment layout from source alone:
every overlaid binding, joined to its compile-time size assertion, sorted by
address, checked for overlap. It is the only automated safety net this project
has, and it is what turns "we cannot raise the cap" into "here is proof that
this specific relocation is safe."

USAGE
    python addrlint.py [--repo ROOT] [--markdown OUT.md] [--json OUT.json]
                       [--check] [--quiet]

    --check   exit non-zero on any overlap (for CI / pre-commit)

EXIT CODES
    0  no overlaps
    1  overlap(s) detected
    2  tool error (bad path, unparseable input)
"""

import argparse
import json
import os
import re
import sys
from collections import defaultdict

# --------------------------------------------------------------------------
# Sizes of primitive/simple types used in overlaid bindings.
#
# Only types whose size we KNOW are used for overlap maths. Anything unknown is
# still reported, but marked so a human can see the map has a hole in it --
# silently assuming a size would defeat the entire purpose of the tool.
# --------------------------------------------------------------------------
PRIMITIVE_SIZES = {
    'sint8': 1, 'uint8': 1, 'char': 1, 'bool': 1,
    'sint16': 2, 'uint16': 2, 'short': 2,
    'sint32': 4, 'uint32': 4, 'int': 4, 'long': 4,
    'real32': 4, 'float': 4, 'bool32': 4,
    'undefined4': 4, 'BoolTri': 4,
    'sint64': 8, 'uint64': 8, 'real64': 8, 'double': 8,
    'Point2I': 8, 'Point2F': 8,
    'Vector3F': 12,
    'IID': 16, 'GUID': 16,
}

# Binding form:  <Type> & <ns::name> = *( <Type> * ) 0xADDR ;
BINDING_RE = re.compile(
    r'^(?P<indent>\s*)'
    r'(?P<decltype>[A-Za-z_][\w:]*)\s*&\s*'
    r'(?P<name>[A-Za-z_][\w:]*)\s*=\s*'
    r'\*\s*\(\s*(?P<casttype>[A-Za-z_][\w:]*)\s*\*\s*\)\s*'
    r'(?P<addr>0[xX][0-9a-fA-F]+)'
)

# assert_sizeof(Type, 0xSIZE);
SIZEOF_RE = re.compile(
    r'assert_sizeof\s*\(\s*(?P<type>[A-Za-z_][\w:]*)\s*,\s*'
    r'(?P<size>0[xX][0-9a-fA-F]+|\d+)\s*\)'
)


def strip_ns(name):
    """Gods98::Stats_Globs -> Stats_Globs"""
    return name.rsplit('::', 1)[-1]


def is_commented(line):
    """True if the statement is commented out.

    Only handles leading // and /* on the same line, which is the only form
    these declarations take in this codebase. Deliberately conservative: a
    missed comment produces a spurious entry (visible, fixable), whereas
    wrongly treating a live binding as a comment would hide a real overlap.
    """
    s = line.lstrip()
    return s.startswith('//') or s.startswith('/*')


def scan(repo):
    """Collect size assertions and overlaid bindings from the source tree."""
    sizes = {}          # bare type name -> size in bytes
    size_origin = {}    # bare type name -> "file:line"
    bindings = []       # list of dicts

    src = os.path.join(repo, 'src')
    if not os.path.isdir(src):
        raise SystemExit(f'addrlint: no src/ under {repo!r}')

    for root, _dirs, files in os.walk(src):
        for fn in files:
            if not fn.endswith(('.cpp', '.h', '.hpp')):
                continue
            path = os.path.join(root, fn)
            rel = os.path.relpath(path, repo).replace('\\', '/')
            try:
                with open(path, 'r', encoding='utf-8', errors='replace') as fh:
                    lines = fh.readlines()
            except OSError as exc:
                raise SystemExit(f'addrlint: cannot read {rel}: {exc}')

            for n, line in enumerate(lines, 1):
                if is_commented(line):
                    continue

                m = SIZEOF_RE.search(line)
                if m:
                    t = strip_ns(m.group('type'))
                    sizes[t] = int(m.group('size'), 0)
                    size_origin[t] = f'{rel}:{n}'

                m = BINDING_RE.search(line)
                if m:
                    bindings.append({
                        'name': m.group('name'),
                        'type': strip_ns(m.group('casttype')),
                        'addr': int(m.group('addr'), 0),
                        'where': f'{rel}:{n}',
                    })

    return sizes, size_origin, bindings


def resolve(bindings, sizes):
    """Attach a byte size to each binding where one is knowable."""
    for b in bindings:
        t = b['type']
        if t in sizes:
            b['size'] = sizes[t]
            b['size_src'] = 'assert_sizeof'
        elif t in PRIMITIVE_SIZES:
            b['size'] = PRIMITIVE_SIZES[t]
            b['size_src'] = 'primitive'
        else:
            b['size'] = None
            b['size_src'] = 'UNKNOWN'
    return bindings


def find_overlaps(bindings):
    """Pairwise overlap detection over bindings with known sizes."""
    known = sorted([b for b in bindings if b['size']], key=lambda b: b['addr'])
    overlaps = []
    for i in range(len(known) - 1):
        a = known[i]
        a_end = a['addr'] + a['size']
        # Compare against every later binding that could still start before a ends.
        for b in known[i + 1:]:
            if b['addr'] >= a_end:
                break
            overlaps.append({
                'a': a, 'b': b,
                'bytes': min(a_end, b['addr'] + b['size']) - b['addr'],
            })
    return overlaps, known


def compute_gaps(known):
    """Slack between consecutive known regions -- the growth headroom."""
    gaps = []
    for i in range(len(known) - 1):
        a, b = known[i], known[i + 1]
        a_end = a['addr'] + a['size']
        if b['addr'] >= a_end:
            gaps.append({'after': a, 'before': b, 'slack': b['addr'] - a_end})
    return gaps


def render_markdown(known, unknown, overlaps, gaps, sizes, size_origin):
    L = []
    w = L.append
    w('# Address map of the original executable\'s data segment')
    w('')
    w('**Generated by `tools/addrlint/addrlint.py`. Do not edit by hand.**')
    w('Regenerate with `python tools/addrlint/addrlint.py --markdown docs/ADDRESS-MAP.md`.')
    w('')
    w('Every entry below is a struct or variable that OpenLRR does **not** own. Each is a')
    w('C++ reference overlaid onto a fixed address inside the original 1999 executable, whose')
    w('own machine code still reads and writes those bytes at those offsets. Sizes come from')
    w('`assert_sizeof` (a `static_assert`) or from a known primitive type.')
    w('')
    w('> **The rule this file exists to enforce:** you may not grow any of these types.')
    w('> Growth does not move the neighbour -- it overwrites it. Add DLL-side storage instead.')
    w('')
    w(f'- Regions with a known size: **{len(known)}**')
    w(f'- Bindings whose size is not knowable from source: **{len(unknown)}**')
    w(f'- Overlaps detected: **{len(overlaps)}**')
    w('')

    if overlaps:
        w('## ⚠ OVERLAPS')
        w('')
        w('These regions collide. Either a size assertion is wrong, or a change has grown a')
        w('struct past its neighbour. **This is a build-breaking condition.**')
        w('')
        w('| Region A | A range | Region B | B range | Overlap |')
        w('| --- | --- | --- | --- | ---: |')
        for o in overlaps:
            a, b = o['a'], o['b']
            w(f"| `{a['name']}` | `0x{a['addr']:08x}`–`0x{a['addr']+a['size']:08x}` "
              f"| `{b['name']}` | `0x{b['addr']:08x}`–`0x{b['addr']+b['size']:08x}` "
              f"| {o['bytes']} B |")
        w('')
    else:
        w('## No overlaps')
        w('')
        w('Every known region is disjoint from every other. The layout is self-consistent.')
        w('')

    w('## Tightest neighbours')
    w('')
    w('Slack is the free space between the end of one region and the start of the next. A')
    w('region with little slack cannot grow *at all*; this table is the first thing to check')
    w('before contemplating any struct change.')
    w('')
    w('| Slack | Region ending | Ends at | Next region | Starts at |')
    w('| ---: | --- | --- | --- | --- |')
    for g in sorted(gaps, key=lambda g: g['slack'])[:25]:
        a, b = g['after'], g['before']
        w(f"| **{g['slack']} B** | `{a['name']}` | `0x{a['addr']+a['size']:08x}` "
          f"| `{b['name']}` | `0x{b['addr']:08x}` |")
    w('')

    w('## Full map, by address')
    w('')
    w('| Address | End | Size | Region | Type | Size from | Declared at |')
    w('| --- | --- | ---: | --- | --- | --- | --- |')
    for b in known:
        origin = size_origin.get(b['type'], '')
        w(f"| `0x{b['addr']:08x}` | `0x{b['addr']+b['size']:08x}` | {b['size']} "
          f"| `{b['name']}` | `{b['type']}` | {b['size_src']}{(' @ ' + origin) if origin else ''} "
          f"| `{b['where']}` |")
    w('')

    if unknown:
        w('## Bindings with no knowable size')
        w('')
        w('These are overlaid at a fixed address but their type carries no `assert_sizeof` and')
        w('is not a known primitive, so they are **excluded from overlap checking**. The map is')
        w('incomplete by exactly this much.')
        w('')
        w('| Address | Region | Type | Declared at |')
        w('| --- | --- | --- | --- |')
        for b in sorted(unknown, key=lambda b: b['addr']):
            w(f"| `0x{b['addr']:08x}` | `{b['name']}` | `{b['type']}` | `{b['where']}` |")
        w('')

    w('## Method')
    w('')
    w('1. Walk `src/**` for `*.cpp`, `*.h`, `*.hpp`, skipping commented-out lines.')
    w('2. Collect `assert_sizeof(Type, 0xN)` into a type→size table '
      f'({len(sizes)} types found).')
    w('3. Collect bindings of the form `T & name = *(T*)0xADDR;`.')
    w('4. Join, sort by address, check every pair that could overlap, and measure slack.')
    w('')
    w('Sizes are taken from the source\'s own assertions rather than from a compiler, so this')
    w('map is only as correct as those assertions. They are `static_assert`s, so a wrong one')
    w('fails the build -- but a *missing* one leaves a hole here, which is why unsized')
    w('bindings are listed rather than hidden.')
    return '\n'.join(L) + '\n'


def main():
    ap = argparse.ArgumentParser(description='Address-space linter for DeepCoreOverhaul.')
    ap.add_argument('--repo', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
    ap.add_argument('--markdown')
    ap.add_argument('--json')
    ap.add_argument('--check', action='store_true', help='exit non-zero if overlaps found')
    ap.add_argument('--quiet', action='store_true')
    args = ap.parse_args()

    repo = os.path.abspath(args.repo)
    sizes, size_origin, bindings = scan(repo)
    bindings = resolve(bindings, sizes)
    overlaps, known = find_overlaps(bindings)
    unknown = [b for b in bindings if not b['size']]
    gaps = compute_gaps(known)

    if args.markdown:
        os.makedirs(os.path.dirname(os.path.abspath(args.markdown)), exist_ok=True)
        with open(args.markdown, 'w', encoding='utf-8', newline='\n') as fh:
            fh.write(render_markdown(known, unknown, overlaps, gaps, sizes, size_origin))

    if args.json:
        os.makedirs(os.path.dirname(os.path.abspath(args.json)), exist_ok=True)
        with open(args.json, 'w', encoding='utf-8', newline='\n') as fh:
            json.dump({
                'regions': [{k: v for k, v in b.items()} for b in known],
                'unsized': unknown,
                'overlaps': [{
                    'a': o['a']['name'], 'b': o['b']['name'], 'bytes': o['bytes'],
                } for o in overlaps],
                'gaps': [{
                    'after': g['after']['name'], 'before': g['before']['name'], 'slack': g['slack'],
                } for g in gaps],
            }, fh, indent=1)

    if not args.quiet:
        print(f'addrlint: {len(known)} sized regions, {len(unknown)} unsized, '
              f'{len(sizes)} assert_sizeof types')
        tight = sorted(gaps, key=lambda g: g['slack'])[:5]
        for g in tight:
            print(f"  slack {g['slack']:>6} B  after {g['after']['name']:<28} "
                  f"-> {g['before']['name']}")
        for o in overlaps:
            print(f"  OVERLAP {o['bytes']} B: {o['a']['name']} x {o['b']['name']}", file=sys.stderr)

    if overlaps and args.check:
        print(f'addrlint: FAILED -- {len(overlaps)} overlap(s)', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
