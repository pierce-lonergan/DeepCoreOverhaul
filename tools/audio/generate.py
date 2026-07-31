#!/usr/bin/env python3
"""
generate.py -- build DeepCoreOverhaul's audio assets from a manifest.

WHY THE OUTPUT FORMAT IS NOT NEGOTIABLE
---------------------------------------
Read from the loader, not assumed:

  * Every secondary sound buffer is created with DSBCAPS_CTRL3D, unconditionally
    (src/openlrr/engine/audio/3DSound.cpp:1123-1124). DirectSound3D buffers are
    MONO ONLY -- CreateSoundBuffer fails with DSERR_INVALIDPARAM on a stereo
    buffer that asks for CTRL3D, and the engine reports only a generic
    "Cannot create sound buffer." (:1128). A stereo file therefore fails silently
    as far as a modder is concerned. This is the single most important constraint
    in this file.
  * The format handed to CreateSoundBuffer comes straight from the file
    (dsbuf.lpwfxFormat = sound->pwf, :1126), so the WAV's own header decides.
  * Loading is standard MMIO RIFF reading -- WaveOpenFile / WaveStartDataRead /
    WaveReadFile (:1057-1072) -- so plain uncompressed PCM RIFF is what is wanted.
  * The primary buffer is 22050 Hz, 16-bit (:154-156). Matching it avoids a
    resample at mix time.
  * Samples are addressed as "<name>.wav" (:280).

So: RIFF WAV, PCM, MONO, 22050 Hz, 16-bit signed little-endian.

ElevenLabs returns exactly that as `output_format=pcm_22050` -- raw headerless
16-bit mono PCM at 22050 Hz, verified by measuring a known-duration request -- so this
script only has to prepend a 44-byte RIFF header. No ffmpeg, no converter, no
dependency beyond the standard library.

USAGE
    set ELEVENLABS_API_KEY=...            (never stored in the repo)
    python tools/audio/generate.py                  # generate everything missing
    python tools/audio/generate.py --check          # validate manifest only, no network
    python tools/audio/generate.py --force          # regenerate even if up to date
    python tools/audio/generate.py --only briefing  # one category

WITHOUT A KEY it still validates the manifest end to end and prints exactly what
it would produce, so the moment a key exists one command fills assets/audio/.

IDEMPOTENCE
Each output records the hash of the inputs that produced it in
assets/audio/.manifest-lock.json. An entry is regenerated only when its text,
voice, settings or target format change. Re-running costs nothing and produces
no network traffic.

LICENSING
Generated audio is original work and is committable. Nothing extracted from the
game ever is. See NOTICE.md.
"""

import argparse
import hashlib
import json
import os
import struct
import sys
import time

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
MANIFEST = os.path.join(REPO, 'tools', 'audio', 'manifest.json')
OUTROOT = os.path.join(REPO, 'assets', 'audio')
LOCKFILE = os.path.join(OUTROOT, '.manifest-lock.json')

API_ROOT = 'https://api.elevenlabs.io/v1'

# The engine's requirements, derived above. Changing any of these without
# re-reading 3DSound.cpp is how you ship silence.
TARGET_RATE = 22050
TARGET_BITS = 16
TARGET_CHANNELS = 1

# Request the engine's rate directly. VERIFIED, after first getting this wrong:
#
# A 2.0-second sound-generation request returns 88,200 samples at output_format=pcm_22050
# and 176,400 samples at pcm_44100. Both are 4.0 seconds at their own declared rate, so the
# API honours output_format exactly and no resampling is needed.
#
# What actually surprised me was the LENGTH: the response is consistently about twice the
# requested duration_seconds. That is a content quirk, not a format bug, and it is why the
# manifest's durations are chosen with the doubling in mind. My first reading mistook the
# extra length for a wrong sample rate and briefly added a 44100->22050 decimation step,
# which was wasted bandwidth and an unnecessary filter. Measured, then removed.
ELEVEN_REQUEST_FORMAT = 'pcm_22050'
ELEVEN_REQUEST_RATE = TARGET_RATE

# Observed ratio of returned audio length to requested duration_seconds. Used only to warn
# when a result is wildly off, so a silent API change does not quietly produce 30-second
# stingers.
OBSERVED_DURATION_FACTOR = 2.0


# --------------------------------------------------------------------------
# WAV writing
# --------------------------------------------------------------------------

def wrap_pcm_as_wav(pcm_bytes):
    """Prepend a canonical 44-byte RIFF header to raw 16-bit mono PCM."""
    byte_rate = TARGET_RATE * TARGET_CHANNELS * TARGET_BITS // 8
    block_align = TARGET_CHANNELS * TARGET_BITS // 8
    return b''.join([
        b'RIFF', struct.pack('<I', 36 + len(pcm_bytes)), b'WAVE',
        b'fmt ', struct.pack('<IHHIIHH', 16, 1, TARGET_CHANNELS, TARGET_RATE,
                             byte_rate, block_align, TARGET_BITS),
        b'data', struct.pack('<I', len(pcm_bytes)),
        pcm_bytes,
    ])


def describe_wav(path):
    """Read back a written file and report its real format, so the check is on
    the artifact rather than on our intent."""
    try:
        with open(path, 'rb') as fh:
            head = fh.read(44)
        if len(head) < 44 or head[:4] != b'RIFF' or head[8:12] != b'WAVE':
            return 'not a RIFF WAVE file'
        fmt, ch, rate, _br, _ba, bits = struct.unpack('<HHIIHH', head[20:36])
        size = os.path.getsize(path)
        seconds = max(0, (size - 44)) / float(rate * ch * bits / 8) if rate and ch and bits else 0
        ok = (fmt == 1 and ch == TARGET_CHANNELS and rate == TARGET_RATE and bits == TARGET_BITS)
        return ('%s %dch %dHz %dbit %.2fs%s'
                % ('PCM' if fmt == 1 else 'fmt%d' % fmt, ch, rate, bits, seconds,
                   '' if ok else '   <-- WRONG FORMAT, engine will reject'))
    except OSError as exc:
        return 'unreadable: %s' % exc


# --------------------------------------------------------------------------
# HTTP
# --------------------------------------------------------------------------

def http_post_json(url, api_key, payload, accept='audio/mpeg'):
    """POST JSON, return raw response bytes. urllib so there is no hard dependency."""
    import urllib.request
    import urllib.error
    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, method='POST')
    req.add_header('Content-Type', 'application/json')
    req.add_header('Accept', accept)
    req.add_header('xi-api-key', api_key)
    try:
        with urllib.request.urlopen(req, timeout=180) as resp:
            return resp.read()
    except urllib.error.HTTPError as exc:
        body = exc.read().decode('utf-8', 'replace')[:600]
        raise RuntimeError('HTTP %s from %s\n%s' % (exc.code, url, body))


def http_get_json(url, api_key):
    import urllib.request
    import urllib.error
    req = urllib.request.Request(url)
    req.add_header('xi-api-key', api_key)
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            return json.loads(resp.read().decode('utf-8'))
    except urllib.error.HTTPError as exc:
        body = exc.read().decode('utf-8', 'replace')[:600]
        raise RuntimeError('HTTP %s from %s\n%s' % (exc.code, url, body))


# --------------------------------------------------------------------------
# Manifest
# --------------------------------------------------------------------------

REQUIRED_KEYS = ('name', 'category', 'kind')
VALID_KINDS = ('speech', 'sfx')


def load_manifest(path):
    if not os.path.exists(path):
        raise SystemExit('generate.py: no manifest at %s' % path)
    with open(path, 'r', encoding='utf-8') as fh:
        try:
            doc = json.load(fh)
        except json.JSONDecodeError as exc:
            raise SystemExit('generate.py: manifest is not valid JSON: %s' % exc)
    if 'entries' not in doc or not isinstance(doc['entries'], list):
        raise SystemExit('generate.py: manifest has no "entries" list')
    return doc


def validate(doc):
    """Return a list of human-readable problems. Never raises on bad data --
    the whole point is to report every problem at once."""
    problems = []
    seen = {}
    voices = doc.get('voices', {})

    for i, e in enumerate(doc['entries']):
        where = 'entry %d (%s)' % (i, e.get('name', '<unnamed>'))

        for k in REQUIRED_KEYS:
            if not e.get(k):
                problems.append('%s: missing required key "%s"' % (where, k))

        kind = e.get('kind')
        if kind and kind not in VALID_KINDS:
            problems.append('%s: kind "%s" is not one of %s' % (where, kind, VALID_KINDS))

        name = e.get('name')
        if name:
            if name in seen:
                problems.append('%s: duplicate name, already used by entry %d' % (where, seen[name]))
            seen[name] = i
            # The engine addresses samples as "<name>.wav" and config identifiers are
            # case-insensitive; anything exotic here becomes an unfindable file.
            if not all(c.isalnum() or c in '_-' for c in name):
                problems.append('%s: name may only contain letters, digits, underscore and hyphen' % where)

        if kind == 'speech':
            if not e.get('text'):
                problems.append('%s: speech entries need "text"' % where)
            v = e.get('voice')
            if not v:
                problems.append('%s: speech entries need "voice"' % where)
            elif v not in voices:
                problems.append('%s: voice "%s" is not defined in the manifest "voices" table' % (where, v))
        elif kind == 'sfx':
            if not e.get('prompt'):
                problems.append('%s: sfx entries need "prompt"' % where)
            d = e.get('duration_seconds')
            if d is not None and not (0.5 <= float(d) <= 22.0):
                problems.append('%s: duration_seconds %s outside the 0.5..22 the API accepts' % (where, d))

        # The naming rule is a project hard line, and generated audio is content too.
        for field in ('text', 'prompt'):
            val = (e.get(field) or '')
            low = val.lower()
            if 'lego' in low:
                problems.append('%s: "%s" contains an expansion of the L. That is never allowed, '
                                'including in generated audio.' % (where, field))

    return problems


def entry_hash(entry, voices):
    """Everything that would change the produced bytes."""
    payload = {
        'kind': entry.get('kind'),
        'text': entry.get('text'),
        'prompt': entry.get('prompt'),
        'voice': voices.get(entry.get('voice', ''), {}),
        'model': entry.get('model'),
        'settings': entry.get('settings'),
        'duration_seconds': entry.get('duration_seconds'),
        'format': [TARGET_RATE, TARGET_BITS, TARGET_CHANNELS, ELEVEN_REQUEST_FORMAT],
    }
    blob = json.dumps(payload, sort_keys=True, ensure_ascii=False).encode('utf-8')
    return hashlib.sha256(blob).hexdigest()


def output_path(entry):
    return os.path.join(OUTROOT, entry['category'], entry['name'] + '.wav')


# --------------------------------------------------------------------------
# Generation
# --------------------------------------------------------------------------

def resolve_voice_ids(doc, api_key):
    """Map manifest voice names to live voice IDs.

    Resolved against the account rather than hardcoded, so the manifest stays
    readable and does not rot when a voice ID changes. A manifest may also pin an
    explicit id, which wins.
    """
    wanted = doc.get('voices', {})
    need_lookup = [n for n, v in wanted.items() if not v.get('voice_id')]
    if not need_lookup:
        return

    catalog = http_get_json(API_ROOT + '/voices', api_key)

    # Voices on a real account are named descriptively -- "Brian - Deep, Resonant and
    # Comforting" -- so an exact match on "Brian" finds nothing. Match on the leading
    # name before the first dash, then fall back to a substring search, so the manifest
    # can stay readable and does not have to carry opaque IDs.
    exact, leading = {}, {}
    for v in catalog.get('voices', []):
        full = (v.get('name') or '').strip()
        vid = v.get('voice_id')
        if not full or not vid:
            continue
        exact[full.lower()] = vid
        leading.setdefault(full.split('-')[0].strip().lower(), vid)

    for n in need_lookup:
        spec = wanted[n]
        key = (spec.get('elevenlabs_name') or n).strip().lower()
        vid = exact.get(key) or leading.get(key)
        if not vid:
            for full, candidate in exact.items():
                if full.startswith(key):
                    vid = candidate
                    break
        spec['voice_id'] = vid
        if not vid:
            print('  ! voice "%s" (looking for "%s") not found among %d account voices'
                  % (n, key, len(exact)))


def generate_one(entry, doc, api_key):
    voices = doc.get('voices', {})
    if entry['kind'] == 'speech':
        spec = voices.get(entry['voice'], {})
        vid = spec.get('voice_id')
        if not vid:
            raise RuntimeError('voice "%s" has no voice_id' % entry['voice'])
        url = '%s/text-to-speech/%s?output_format=%s' % (API_ROOT, vid, ELEVEN_REQUEST_FORMAT)
        payload = {
            'text': entry['text'],
            'model_id': entry.get('model') or doc.get('default_model') or 'eleven_multilingual_v2',
        }
        settings = dict(spec.get('settings') or {})
        settings.update(entry.get('settings') or {})
        if settings:
            payload['voice_settings'] = settings
        return http_post_json(url, api_key, payload, accept='audio/pcm')

    # sfx
    url = '%s/sound-generation?output_format=%s' % (API_ROOT, ELEVEN_REQUEST_FORMAT)
    payload = {'text': entry['prompt']}
    if entry.get('duration_seconds') is not None:
        payload['duration_seconds'] = float(entry['duration_seconds'])
    if entry.get('prompt_influence') is not None:
        payload['prompt_influence'] = float(entry['prompt_influence'])
    return http_post_json(url, api_key, payload, accept='audio/pcm')


def main():
    ap = argparse.ArgumentParser(description="Generate DeepCoreOverhaul audio assets.")
    ap.add_argument('--manifest', default=MANIFEST)
    ap.add_argument('--check', action='store_true', help='validate only; never touch the network')
    ap.add_argument('--force', action='store_true', help='regenerate even when up to date')
    ap.add_argument('--only', help='only entries in this category')
    ap.add_argument('--limit', type=int, default=0, help='stop after N generations (for a smoke test)')
    args = ap.parse_args()

    doc = load_manifest(args.manifest)
    entries = doc['entries']
    if args.only:
        entries = [e for e in entries if e.get('category') == args.only]

    print('DeepCoreOverhaul audio generator')
    print('  manifest : %s' % os.path.relpath(args.manifest, REPO))
    print('  entries  : %d%s' % (len(entries), (' (filtered from %d)' % len(doc['entries'])) if args.only else ''))
    print('  target   : RIFF PCM, %d ch, %d Hz, %d bit  (mandatory: DSBCAPS_CTRL3D buffers are mono-only)'
          % (TARGET_CHANNELS, TARGET_RATE, TARGET_BITS))
    print('  source   : requesting %s directly (verified: output_format is honoured)'
          % ELEVEN_REQUEST_FORMAT)
    print()

    problems = validate(doc)
    if problems:
        print('MANIFEST PROBLEMS (%d):' % len(problems))
        for p in problems:
            print('  - %s' % p)
        print('\nNothing generated.')
        return 2
    print('Manifest validates: %d entries, %d voices, no problems.\n'
          % (len(doc['entries']), len(doc.get('voices', {}))))

    lock = {}
    if os.path.exists(LOCKFILE):
        try:
            with open(LOCKFILE, 'r', encoding='utf-8') as fh:
                lock = json.load(fh)
        except (OSError, json.JSONDecodeError):
            lock = {}

    todo, current = [], []
    for e in entries:
        want = entry_hash(e, doc.get('voices', {}))
        out = output_path(e)
        if not args.force and os.path.exists(out) and lock.get(e['name']) == want:
            current.append(e)
        else:
            todo.append((e, want, out))

    api_key = os.environ.get('ELEVENLABS_API_KEY', '').strip()

    if args.check or not api_key:
        if not api_key and not args.check:
            print('ELEVENLABS_API_KEY is not set -- running in plan mode.\n')
        print('WOULD GENERATE %d file(s); %d already up to date:\n' % (len(todo), len(current)))
        by_cat = {}
        for e, _h, out in todo:
            by_cat.setdefault(e['category'], []).append((e, out))
        for cat in sorted(by_cat):
            print('  %s/' % cat)
            for e, out in by_cat[cat]:
                if e['kind'] == 'speech':
                    detail = '%s: "%s"' % (e['voice'], (e['text'][:64] + ('...' if len(e['text']) > 64 else '')))
                else:
                    detail = 'sfx: "%s"%s' % ((e['prompt'][:56] + ('...' if len(e['prompt']) > 56 else '')),
                                              ('  %.1fs' % float(e['duration_seconds'])) if e.get('duration_seconds') else '')
                print('    %-28s %s' % (e['name'] + '.wav', detail))
            print()
        print('Set ELEVENLABS_API_KEY and re-run without --check to produce them.')
        return 0

    print('Resolving voices...')
    resolve_voice_ids(doc, api_key)
    for n, spec in sorted(doc.get('voices', {}).items()):
        print('  %-14s -> %s' % (n, spec.get('voice_id') or 'UNRESOLVED'))
    print()

    made = failed = 0
    for e, want, out in todo:
        if args.limit and made >= args.limit:
            print('  (--limit %d reached, stopping)' % args.limit)
            break
        os.makedirs(os.path.dirname(out), exist_ok=True)
        label = '%s/%s.wav' % (e['category'], e['name'])
        try:
            pcm = generate_one(e, doc, api_key)
            if not pcm:
                raise RuntimeError('empty response')
            with open(out, 'wb') as fh:
                fh.write(wrap_pcm_as_wav(pcm))

            # Sanity-check length against expectation. Not a hard failure -- generative
            # output legitimately varies -- but a silent API change that started returning
            # 30-second stingers should not pass unnoticed.
            want_s = e.get('duration_seconds')
            if want_s:
                got_s = (len(pcm) / 2.0) / float(TARGET_RATE)
                expect_s = float(want_s) * OBSERVED_DURATION_FACTOR
                if got_s > expect_s * 2.0 or got_s < expect_s * 0.4:
                    print('       ! %.1fs returned, expected around %.1fs' % (got_s, expect_s))

            lock[e['name']] = want
            made += 1
            print('  ok   %-40s %s' % (label, describe_wav(out)))
        except Exception as exc:               # noqa: BLE001 - report and continue
            failed += 1
            print('  FAIL %-40s %s' % (label, exc))
        time.sleep(float(doc.get('request_delay_seconds', 0.25)))

    os.makedirs(OUTROOT, exist_ok=True)
    with open(LOCKFILE, 'w', encoding='utf-8', newline='\n') as fh:
        json.dump(lock, fh, indent=1, sort_keys=True)

    print('\n%d generated, %d already current, %d failed.' % (made, len(current), failed))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
