#!/usr/bin/env python3
"""Generate texture source images with Together.ai.

WHY THIS EXISTS
---------------
Every realism ceiling this project has hit traces back to one constraint: no texture assets.
Procedural noise in a pixel shader tops out at "plausible-looking plastic" -- it can produce a
shading gradient but not the grain, mineral variety, or stain history that makes stone read as
stone. Photogrammetry is off the table (it would be somebody else's copyrighted scan).

Generated images resolve that cleanly. They are original content, produced by a generator this
repository drives, so they carry none of the provenance problems of scraped textures, and they
can be regenerated from the prompt at any time -- which means the PROMPT is the source of truth
and the PNG is a build artefact.

THE KEY IS NOT IN THIS REPOSITORY.
Set TOGETHER_API_KEY in the environment, or put it in a file outside the repo and point
TOGETHER_KEY_FILE at it. Never paste it into a tracked file.

Usage:
    python genimage.py --prompt "..." --out rock_albedo.png
    python genimage.py --preset granite_albedo --out granite.png
    python genimage.py --list
"""

import argparse
import base64
import json
import os
import sys
import time
import urllib.request
import urllib.error

API_URL = "https://api.together.xyz/v1/images/generations"

# Cheapest capable model first. FLUX.1-schnell is a few-step distilled model: fast and pennies,
# which matters when a texture set is dozens of images and the loop is iterative.
# FLUX.2-dev, not schnell. Measured difference on this exact task: schnell renders a rock SLAB
# on a black background no matter how the prompt negates it, while FLUX.2-dev produces a
# full-frame evenly-lit surface first time. For textures the instruction-following is worth far
# more than the speed, and at ~1.5 cents an image the cost is irrelevant next to the iteration.
DEFAULT_MODEL = "black-forest-labs/FLUX.2-dev"
FALLBACK_MODELS = [
    "black-forest-labs/FLUX.1-schnell",
]

# Shared suffix. Every one of these clauses is load-bearing for a TEXTURE as opposed to a
# picture: a texture must be evenly lit (baked shadows fight the renderer's own lighting),
# perfectly flat-on (perspective cannot be undone), and gap-free to the edges (so it tiles).
TEXTURE_SUFFIX = (
    "extreme close-up macro photograph of the surface itself, the surface completely fills the "
    "entire frame from edge to edge, camera pressed flat against the surface, "
    "no background, no black background, no edges or outline of the rock visible, "
    "not an object on a background, not a specimen, not a slab, "
    "seamless repeating tileable pattern, flat orthographic view, "
    "evenly lit with soft diffuse light, no cast shadows, no vignette, no depth of field, "
    "sharp uniform focus across the whole image, "
    "no text, no watermark, no border, no frame"
)

# The first attempt used "rock face" and asked for a "texture"; FLUX rendered a rock SLAB
# floating on black. The lesson generalises: these models will happily produce a photograph OF
# a thing when what a texture needs is a photograph OF A SURFACE, filling the frame with no
# silhouette anywhere in it. Saying "fills the entire frame" and explicitly negating
# "object / specimen / slab / background" is what moves it.


PRESETS = {
    # Host rock. Each names a real lithology, because "rock" alone produces a generic
    # grey lump and the whole point of the lithology model is that classes look different.
    "granodiorite": "coarse-grained granodiorite rock face, grey feldspar and quartz crystals with "
                    "black biotite flecks, freshly broken angular surface, dry",
    "shale": "dark grey-brown fissile shale rock face, thin parallel bedding laminae, "
             "flaky platy surface, slightly damp",
    "sandstone": "buff-brown sandstone rock face, visible sand grain, faint cross-bedding laminae, "
                 "iron oxide staining streaks, dry",
    "limestone": "pale grey limestone rock face, solution pockets and small vugs, "
                 "thin white calcite veining, slightly damp",
    "basalt": "very dark grey fine-grained basalt rock face, small vesicles and gas bubbles, "
              "fresh fracture surface",
    "schist": "grey-silver mica schist rock face, strong parallel foliation banding, "
              "reflective muscovite flakes, platy",
    # Excavation signatures -- the highest-value detail per the research, and impossible
    # to get from a generic rock texture.
    "blasted": "drill-and-blast tunnel wall in hard rock, visible half-cast drill hole grooves "
               "running parallel across the face, angular blast breakage between them, grey rock dust",
    "shotcrete": "sprayed shotcrete tunnel lining, lumpy grey concrete surface, "
                 "slight sag texture, patches of exposed rock",
    # Ore.
    "sulphide": "massive sulphide ore vein in dark host rock, brassy metallic pyrite and "
                "chalcopyrite bands, faint iridescent tarnish",
    "pegmatite": "coarse pegmatite quartz vein cutting dark host rock, milky white and "
                 "translucent crystal faces, sharp angular crystal boundaries",
    # Floor.
    "muck": "loose broken rock muck pile on a mine floor, angular grey gravel and fines, "
            "fresh blast debris, dusty",
}


def load_key():
    key = os.environ.get("TOGETHER_API_KEY")
    if key:
        return key.strip()
    path = os.environ.get("TOGETHER_KEY_FILE")
    if path and os.path.exists(path):
        return open(path, "r", encoding="utf-8").read().strip()
    sys.exit("No API key. Set TOGETHER_API_KEY or TOGETHER_KEY_FILE (never commit the key).")


def generate(prompt, out, model=None, width=1024, height=1024, steps=4, seed=None, retries=3):
    key = load_key()
    models = [model] if model else [DEFAULT_MODEL] + FALLBACK_MODELS

    last_err = None
    for m in models:
        body = {
            "model": m,
            "prompt": prompt,
            "width": width,
            "height": height,
            "n": 1,
            "response_format": "b64_json",
        }
        # dev/pro models want more steps than the distilled schnell.
        body["steps"] = steps if "schnell" in m else max(steps, 20)
        if seed is not None:
            body["seed"] = seed

        for attempt in range(retries):
            req = urllib.request.Request(
                API_URL,
                data=json.dumps(body).encode(),
                headers={"Authorization": "Bearer " + key,
                         "Content-Type": "application/json",
                         # Without an explicit User-Agent the edge returns 403/1010 -- it
                         # blocks urllib's default agent string before the request ever
                         # reaches the API, which looks exactly like an auth failure.
                         "User-Agent": "DeepCoreOverhaul/1.0 (+texture-generator)",
                         "Accept": "application/json"},
            )
            try:
                with urllib.request.urlopen(req, timeout=180) as r:
                    payload = json.load(r)
                item = payload["data"][0]
                if "b64_json" in item:
                    raw = base64.b64decode(item["b64_json"])
                else:
                    img_req = urllib.request.Request(
                        item["url"], headers={"User-Agent": "DeepCoreOverhaul/1.0"})
                    with urllib.request.urlopen(img_req, timeout=180) as ir:
                        raw = ir.read()
                os.makedirs(os.path.dirname(os.path.abspath(out)) or ".", exist_ok=True)
                with open(out, "wb") as f:
                    f.write(raw)
                print("%s  <- %s  (%d bytes)" % (out, m, len(raw)))
                return True
            except urllib.error.HTTPError as e:
                detail = e.read().decode("utf-8", "replace")[:300]
                last_err = "%s HTTP %s: %s" % (m, e.code, detail)
                # 429/5xx are worth retrying; a 4xx about the model is not.
                if e.code in (429, 500, 502, 503, 504):
                    time.sleep(2 * (attempt + 1))
                    continue
                break
            except Exception as e:
                last_err = "%s %r" % (m, e)
                time.sleep(1 + attempt)
    print("FAILED: %s" % last_err, file=sys.stderr)
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--prompt")
    ap.add_argument("--preset", choices=sorted(PRESETS))
    ap.add_argument("--out", default="out.png")
    ap.add_argument("--model")
    ap.add_argument("--width", type=int, default=1024)
    ap.add_argument("--height", type=int, default=1024)
    ap.add_argument("--steps", type=int, default=4)
    ap.add_argument("--seed", type=int)
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()

    if a.list:
        for k, v in sorted(PRESETS.items()):
            print("%-14s %s" % (k, v))
        return

    if a.preset:
        prompt = PRESETS[a.preset] + ", " + TEXTURE_SUFFIX
    elif a.prompt:
        prompt = a.prompt + ", " + TEXTURE_SUFFIX
    else:
        sys.exit("need --prompt or --preset (or --list)")

    ok = generate(prompt, a.out, a.model, a.width, a.height, a.steps, a.seed)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
