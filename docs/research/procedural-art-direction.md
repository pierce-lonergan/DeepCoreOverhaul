<!-- Research document. Authored by reading `src/game3d/DeepCore3D.cpp` at the revision present
     in the worktree, plus public web sources cited inline. NO BUILD WAS RUN for this
     document and no game was run for this document. Every code sketch below is a sketch:
     it is written to be transcribable, not to have been compiled. Claims about how the
     current build *looks* are derived from reading the draw calls, not from observing
     pixels, and are marked where that distinction matters. -->

# Procedural art direction — what actually separates a prototype look from a finished one

Repo: `C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`
Subject: `src/game3d/DeepCore3D.cpp` (725 lines), OpenGL 1.x immediate mode, v142 / x86 / `/W4`.
Benchmark named by the user: **Manic Miners** — a free, closed-source Unreal Engine remake of
a 1999 mining game, four-plus years of solo development with authored art.

**Naming rule honoured throughout: the "L" in "LRR" is never expanded, anywhere in this
document.**

---

## 0. The headline, stated before the evidence

**The gap you are looking at is not a geometry gap. It is a lighting gap, a contact gap and a
frame gap, in that order.** Every asset in Manic Miners is authored; you cannot have those.
But almost none of the *perceived* distance between the two images comes from the assets. It
comes from six properties that are all pure code:

1. Every surface in a finished frame carries a **gradient**. Ours carries one constant per
   face — six numbers for the entire world (`DeepCore3D.cpp:355-362`).
2. Every object in a finished frame **touches the ground**. Nothing in ours does: there is not
   one shadow, one contact darkening, or one dust puff in the file.
3. A finished frame has a **frame** — vignette, grade, edge falloff. Ours ends abruptly at the
   window border, which reads as a viewport, not an image.
4. A finished frame's **colours all come from one small family**. Ours are eleven independent
   guesses, several at effectively full saturation.
5. A finished game's **camera has an opinion**. Ours is an orbit control with no smoothing at
   all — `camYaw += (p.x - g_last.x) * 0.008f` applied straight out of `WM_MOUSEMOVE`
   (`:585-588`), so the rate depends on mouse message frequency rather than on time.
6. A finished game's **HUD was designed**. Ours is `TextOutA` at `(8, 8)` in Consolas 16
   (`:701-714`), which is the visual signature of a debug overlay — and, per §6.1, on a
   double-buffered accelerated pixel format it is probably not even reliably visible.

Everything in this document is an implementation of one of those six. The ranked table is §7;
the six-item shortlist is §7.2. Sources are cited inline and collected in §9.

### 0.1 How the reference was observed, stated honestly

I fetched the official site's feature list (`manicminers.baraklava.com/features/`), the itch.io
page, and community writeups. **I did not view the screenshots or video frames themselves** —
the fetch path returns page text, not images. So §1 is split into two clearly-labelled halves:
**(A) sourced**, quoted from those pages, and **(B) inferred**, which is my reading of what
techniques produce that impression, based on the sourced feature list plus general knowledge of
what makes 3D scenes read as finished. Do not treat (B) as observation.

---

## 1. Reading the reference

### 1.1 (A) Sourced — what the reference states about its own presentation

From the official features page ([manicminers.baraklava.com/features](https://manicminers.baraklava.com/features/)):

| Quoted claim | What it implies technically | Do we have any of it? |
| --- | --- | --- |
| "Unreal's engine 4 providing fantastic lighting" | Dynamic + baked GI, shadow maps, real light falloff, tone-mapped HDR, post stack | No. Six per-face constants. |
| "Upscaled wall textures" | Every surface is textured, at two frequencies | No. `glColor3f` only; no texture unit is ever enabled in the file. |
| "Accurate high-resolution brick textures, some made by hand" | Authored material variety per surface type | No, and cannot have. Must be procedural. |
| "Pristine LDraw models with maximum detail" | Real modelled geometry with bevels, studs, silhouette detail | ~8 axis-aligned cubes per figure (`DrawMiner`, `:371-411`). |
| "A classic LRR UI with modern visuals" | Framed panels, icons, hierarchy, a designed layout | No. Three `TextOutA` calls. |
| Optional **subdued lighting** mode: caverns darken and the scene is lit by the miner's hard-hat lamp ([TV Tropes writeup](https://tvtropes.org/pmwiki/pmwiki.php/VideoGame/ManicMiners)) | A movable warm key light against a dark cool ambient | **This is the single most transferable idea on the list.** See §2.4. |
| "Extensive first-person controls through Eye/Shoulder view"; WASD move, RMB rotate, MMB pan, **edge-hover pan** | Multiple framings, and the camera is treated as a designed system | Ours: MMB orbit, wheel zoom, WASD pan, no smoothing, no edge pan. |
| "Many new animations, details and secrets" | Density of small motion everywhere | Ours: two `sin()` calls (bob, leg swing). |

The one item I would steal outright is the **helmet-lamp lighting mode**. It is a headline
feature of a four-year project, it is 100% code, and it is the highest-value single change
available to us (§2.4, §7.2 item 1).

### 1.2 (B) Inferred — the twelve tells, and where ours falls

These are the properties that make an image read as finished regardless of asset budget. For
each I name the current state in `DeepCore3D.cpp` with a line reference.

**1. Gradient across every surface.** Flat colour reads as "untextured placeholder" to the eye
in well under a second, because no real material is uniform. Ours: `Cube()` writes one
`glColor3f(r*k, g*k, b*k)` per face and then four vertices (`:363-368`). The whole world is
built from six luminance multipliers `{1.00, 0.35, 0.72, 0.60, 0.82, 0.50}`. Cure: per-vertex
colour (§2.6, §3.2) — same immediate-mode path, one `glColor3f` per `glVertex3f` instead of one
per quad. This is close to free and it is the largest single change in the document.

**2. Contact.** Nothing in our scene is attached to the ground. No blob shadow, no AO gusset
where a wall meets the floor, no dust at footfalls. Objects that do not darken what they stand
on read as *pasted onto* the scene rather than *in* it. This is the loudest 3D prototype tell
after flat shading, and it is also the cheapest to fix (§4.6, §3.6).

**3. Colour family.** Every colour in the file was picked independently: torso
`(0.30, 0.42, 0.85)`, head `(0.95, 0.78, 0.55)`, helmet `(0.98, 0.80, 0.15)`, monster species
`{(0.72,0.28,0.20), (0.45,0.80,0.92), (1.0,0.45,0.12)}`, crystal `(0.75,0.35,1.00)` scaled by a
pulse. Several sit near maximum saturation. Nothing shares a ramp, so nothing looks like it
belongs to the same world. Cure: §2.

**4. Value hierarchy.** Value (lightness) is what a viewer reads first, hue second, saturation
third — this is the standard ordering in production colour work
([Pav Creations](https://pavcreations.com/color-theory-for-game-art-design-the-basics/)). In our
scene the wall (`0.34, 0.29, 0.25`), the floor (`0.26, 0.24, 0.22`) and the hidden rock
(`0.13, 0.115, 0.10`) are separated by value, which is correct — but the *actors* sit inside
the same band as the environment, so the things you can click do not separate from the things
you cannot.

**5. Silhouette hierarchy.** The miner is six boxes whose half-extents are
`0.17 / 0.13 / 0.16 / 0.05` — near-identical widths stacked vertically. In black-on-white that
is a featureless column. §3.

**6. Scale contrast.** Monster body half-extent is `0.34 * scale` with `scale ∈ [0.75, 1.55]`
(`:307`), against a miner torso half-extent of `0.17`. The monster is roughly two miners wide
and about the same height. A threat that does not differ in *mass* has no drama. §3.4.

**7. Motion arcs.** Everything in the file is linear or instantaneous. `m.pos` is advanced at
`MINER_SPEED * dt` toward the target with no acceleration; drilling ends with a boolean flip;
monsters snap between wander targets. Finished motion eases in and out of everything.

**8. Reaction to events.** Drilling a crystal wall does this (`:240-256`): set flags, increment
a counter, set a banner string, `::Beep(1200, 60)`. The block simply ceases to exist. No debris,
no flash, no shake, no light change, no residue. **The world does not react to itself**, and
that single property accounts for an enormous share of the "prototype" read. §4.

**9. Density of small motion.** A finished frame is never still: motes in the air, glimmer,
flicker, idle sway. Ours has two sines. §4.9.

**10. Depth cues beyond fog.** We have `GL_LINEAR` fog from 26 to 62 units into the clear colour
(`:644-649`). Linear fog desaturates and darkens uniformly; real aerial perspective also shifts
*hue* toward the ambient. §2.7.

**11. Typography.** Consolas 16 semibold, `RGB(240,240,245)`, hard-coded at `(8, 8)` and
`(8, H-24)`. One size, one weight, no frame, no grid, no icons, no alignment. §6.

**12. Camera behaviour.** No smoothing anywhere; pitch clamp allows 83° (`:587-588`), which is
effectively top-down — at that pitch every vertical face vanishes and the scene flattens into a
map. §5.

---

## 2. Colour and lighting — the dominant factor

### 2.1 The three rules, and why they are rules

**Rule 1 — value before hue.** Build a value ladder first, assign hues into it second. The
common failure is picking a palette before establishing a value structure; value is what the
player reads first, then hue, then saturation
([Pav Creations](https://pavcreations.com/color-theory-for-game-art-design-the-basics/)). We
will define a five-step ladder in perceptual lightness (L\*) and then express *all four biomes*
against the same ladder — biomes differ in hue and chroma only, never in value. That guarantees
gameplay readability is invariant across biomes for free, and it is the reason the
recolour costs nothing in tuning time.

**Rule 2 — saturation discipline.** The goal is not more colours, it is more variation within
fewer colours ([Nasty Rodent](https://nastyrodent.com/color-theory-for-game-art/)). Budget:

| Class | HSV saturation | Max share of screen pixels |
| --- | --- | --- |
| Environment (rock, floor, walls) | ≤ 0.22 | unbounded |
| Actors (miners, monsters, buildings) | 0.30 – 0.55 | ~8 % |
| Objectives (crystal, ore glint) | 0.60 – 0.85 | ~2 % |
| Hazard / alert | 0.85 – 1.00 | **< 0.5 %** |

The operative sentence: **the brightest, most saturated pixel in the frame should be the thing
you most want the player to click.** Right now the helmet (`0.98, 0.80, 0.15`) is the most
saturated thing on screen and it is not clickable.

**Rule 3 — warm key against cool ambient.** Ambient is cool blue-slate; the key light (the
helmet lamp) is warm amber; the objective (crystal) is a cool violet that is nearly
complementary to the key. This is the classic "warm for lit, cool for shadow" split
([Pav Creations](https://pavcreations.com/color-theory-for-game-art-design-the-basics/)) and §2.4
explains exactly why it manufactures depth.

### 2.2 The value ladder

Five environment steps, specified in CIE L\* with a ±3 tolerance. Every biome ramp must hit
these, which makes the palette **machine-checkable** — see §2.9.

| Step | Role | Target L\* |
| --- | --- | --- |
| `E0` | crevice / AO floor / occluded corner | 12 |
| `E1` | unlit face, ambient only, facing away | 20 |
| `E2` | ambient-lit, facing up | 33 |
| `E3` | lamp-lit | 48 |
| `E4` | lamp-hot, near highlight | 63 |

Actors sit at `E3`–`E4` in value even when unlit, so they separate from a wall that is at
`E1`–`E2`. That is a *gameplay* decision expressed as a palette constraint.

### 2.3 The palette

All values are 8-bit sRGB. Convert to linear before doing any lighting maths (§2.6).

#### Atmosphere

| Token | Hex | Role |
| --- | --- | --- |
| `SKY_VOID` | `#070B12` | clear colour, far fog terminus |
| `AMBIENT_SKY` | `#2A3E5C` | hemisphere up-colour (cool) |
| `AMBIENT_GROUND` | `#1A1512` | hemisphere down-colour (warm-dark bounce off warm rock) |
| `FOG_NEAR` | `#111B2A` | `GL_EXP2` fog colour |
| `HORIZON_LIFT` | `#1A2536` | top of the background gradient quad |

`AMBIENT_GROUND` being *warm* while `AMBIENT_SKY` is *cool* is not decoration: it means every
down-facing surface picks up a warm cast and every up-facing one a cool cast, which produces
form on flat geometry from a single dot product. §2.6.

#### Base rock — Basalt (default biome), hue ≈ 212°

| Step | Hex | Notes |
| --- | --- | --- |
| `E0` | `#1B222C` | |
| `E1` | `#2A323E` | |
| `E2` | `#454E5A` | |
| `E3` | `#69727E` | saturation drops as value rises |
| `E4` | `#939BA5` | hue drifts ~4° warmer at the top |

The ramp rule: as value rises, **shift hue toward 40° by 3–5° per step and reduce HSV
saturation by ~0.02 per step**. Constant-hue, constant-saturation ramps are the reason
programmer-art multiplies (`r*k, g*k, b*k`, exactly what `Cube()` does) look like plastic:
scaling RGB by a scalar holds hue and saturation fixed, which never happens in the real world.

#### Biome variants — same ladder, different hue

**Ochre (dry sandstone caverns), hue ≈ 28°**

| `E0` | `E1` | `E2` | `E3` | `E4` |
| --- | --- | --- | --- | --- |
| `#2C2118` | `#3F2E20` | `#5C452F` | `#856745` | `#B2946F` |

**Rime (cold, high-chroma-poor; contrast carried by hue not value), hue ≈ 195°**

| `E0` | `E1` | `E2` | `E3` | `E4` |
| --- | --- | --- | --- | --- |
| `#19262D` | `#26383F` | `#3D5A63` | `#628892` | `#93B7BF` |

**Magma (dark rock plus emissive fissures), hue ≈ 12°**

| `E0` | `E1` | `E2` | `E3` | `E4` |
| --- | --- | --- | --- | --- |
| `#2B1D1A` | `#432D28` | `#63413A` | `#94665A` | `#C0917F` |

Magma adds two emissive-only tokens that are **not** part of the ladder (emissives ignore the
ladder by definition): fissure core `#FFC24A`, fissure body `#FF6A2A`.

#### Objectives

| Token | Hex | Notes |
| --- | --- | --- |
| `CRYSTAL_CORE` | `#F2E4FF` | the near-white bit; without it nothing reads as *emitting* |
| `CRYSTAL_INNER` | `#C77DFF` | |
| `CRYSTAL_BODY` | `#8A3FE0` | |
| `CRYSTAL_DEEP` | `#3E1668` | |
| `CRYSTAL_HALO` | `#9B4DFF` | additive billboard, α 0.35 |
| `ORE_DULL` | `#6E5231` | ore in ambient — deliberately *low* value |
| `ORE_GLINT` | `#E0A94E` | ore under the lamp only |

**This is a gameplay-meaningful colour decision, not a decorative one: crystal *emits*, ore is
*revealed*.** Ore is dull until the lamp reaches it and then it glints. That single asymmetry
gives the lamp a reason to exist beyond atmosphere, and it makes sweeping the lamp across a wall
a genuinely informative action.

Violet was chosen for crystal over cyan for three reasons: (a) it is near-complementary to the
amber lamp (violet ≈ 275°, amber ≈ 32° — a 117° separation on the wheel plus a large value
contrast); (b) cyan would collide with the Rime biome and with water (`0.10, 0.34, 0.60`); (c)
violet appears nowhere else in nature-adjacent rock palettes, so it is unambiguous.

#### Actors

| Token | Hex | Notes |
| --- | --- | --- |
| `MINER_SUIT` | `#2E5FA8` | mid-blue; sits at `E3` value so it separates from wall |
| `MINER_SUIT_DK` | `#1E3D6E` | shadow side of the suit |
| `MINER_SKIN` | `#C99A6E` | desaturated from the current `(0.95,0.78,0.55)` |
| `MINER_HELMET` | `#E8B93C` | still the loudest actor colour but pulled off full sat |
| `MINER_LAMP` | `#FFF3D0` | lamp lens; the only near-white on the miner |
| `MON_ROCK` | `#B4462E` | rust — the "default" threat |
| `MON_ICE` | `#6FC7DE` | ties to Rime biome |
| `MON_LAVA` | `#FF7A1A` + emissive `#FFD24A` | the elite read |
| `MON_EYE` | `#FFE9A8` | all species share eye colour — that consistency *is* the family |

#### Hazard

| Token | Hex | Notes |
| --- | --- | --- |
| `ALERT` | `#FF3B2F` | spawn telegraph only; nothing else in the game may use it |
| `ALERT_DIM` | `#8E1B14` | the telegraph's off-phase |
| `DAMAGE_WASH` | `#FF6A5A` | full-screen, α ≤ 0.14, 0.12 s |
| `HEALTH_OK` | `#37D6A0` | |
| `HEALTH_LOW` | `#FFB13B` | |

#### UI

| Token | Hex | Role |
| --- | --- | --- |
| `UI_PLATE` | `#0D131C` @ α 0.72 | panel fill |
| `UI_BEVEL_HI` | `#39465A` @ α 0.50 | 1 px top/left inner edge |
| `UI_BEVEL_LO` | `#060A10` @ α 0.60 | 1 px bottom/right inner edge |
| `UI_RULE` | `#22303F` | dividers |
| `UI_TEXT` | `#E8EEF5` | primary — ≈ 15:1 against `UI_PLATE` |
| `UI_TEXT_2` | `#8A9AAC` | secondary — ≈ 6.4:1 |
| `UI_TEXT_OFF` | `#4A5665` | disabled |
| `UI_ACCENT` | `#37D6A0` | interactive / positive |
| `UI_WARN` | `#FFB13B` | |
| `UI_DANGER` | `#FF4A3D` | |

**HUD-to-world colour identity rule:** a resource's HUD colour must be the *same hex* as its
world colour. The crystal counter's icon is `#C77DFF`, which is `CRYSTAL_INNER`. The crew icon
is `#E8B93C`, which is `MINER_HELMET`. This costs nothing and it is one of the strongest
"someone designed this" signals available, because it makes the HUD feel like an instrument
reading the world rather than a label pasted over it.

### 2.4 The light rig

Four lights. All four are evaluated **on the CPU into vertex colours** — see §2.6 for why that
beats `GL_LIGHT0..7`.

#### L0 — Hemisphere ambient (not a GL light; a normal-dependent constant)

```
ambient(n) = lerp(AMBIENT_GROUND_lin, AMBIENT_SKY_lin, 0.5 + 0.5 * n.y)
```

Intensity: sky term 0.30, ground term 0.10. One `lerp` and one component read per vertex.
**This is the highest gain-per-line item in the whole document.** It replaces the six constant
face factors with a physically-motivated term that automatically makes tops cool-bright, sides
mid, and undersides warm-dark — which is exactly the shading a real cave has.

#### L1 — Key: the helmet lamp (spot)

| Property | Value | Reasoning |
| --- | --- | --- |
| Position | miner head + `0.70 * up` + `0.16 * facing` | the lamp cube at `:392` is already there |
| Direction | miner facing, pitched **8° down** | pointing dead level looks robotic and lights the wall tops instead of the floor |
| Colour | `#FFC77A` (linear) | reads as ~2600 K tungsten; the warm end of believable |
| Intensity | 2.2 | over-driven on purpose so the falloff has range to lose |
| Radius `R` | 7.5 world units | ≈ 7.5 tiles; touches ~177 tiles, cheap to cull |
| Cone | inner half-angle 22°, outer 38° | narrow enough to be a *beam*, wide enough to play |

Falloff — use the windowed inverse square, not raw `1/d²` (which never reaches zero and so
cannot be culled) and not `GL_LINEAR` attenuation (which looks like a fade, not a light):

```
float d2   = dot(dv, dv);
float win  = saturate(1.0f - (d2*d2) / (R*R*R*R));   // (1 - (d/R)^4)
float att  = (win * win) / (d2 + 1.0f);              // +1 avoids the singularity at d=0
float cone = smoothstep(cosOuter, cosInner, dot(-Ldir, spotDir));
float lam  = saturate(dot(n, Ldir));
rgb += lampColour * intensity * att * cone * lam;
```

This is the standard windowed-inverse-square used in modern real-time rendering; the `(1-(d/R)⁴)²`
window is the form popularised by Karis's Unreal course notes
([SIGGRAPH 2013 physically-based shading course](https://blog.selfshadow.com/publications/s2013-shading-course/)).

**Per-vertex lighting artifact and its fix.** A lamp of radius 7.5 over 1×1 tiles gives about
15 quads across the pool — Gouraud interpolation over 1-unit quads with a point light 0.7 above
them produces visible faceting and Mach bands near the lamp. Fix: **tessellate**. Floor tiles
into 3×3 sub-quads always (9 × 1600 = 14 400 quads, trivial); wall faces into 2×2. Do not
tessellate adaptively — the branch costs more than the vertices.

#### L2 — Crystal fill lights (point, up to 6)

Every *discovered* crystal within 20 units of the camera focus becomes a point light, sorted by
distance, capped at 6.

| Property | Value |
| --- | --- |
| Colour | `#A560FF` |
| Intensity | 0.9, modulated by the same pulse already at `:452` |
| Radius | 4.5 |
| Falloff | same windowed inverse square, no cone |

This is what turns a glowing box into a light *source*. The moment a crystal casts violet onto
the rock beside it, the crystal stops being a decal and becomes an object. Very high gain, and
the code is already written once for L1.

#### L3 — Rim / separation light (directional, camera-relative)

```
rimDir = normalize(-cameraForward + 0.60f * worldUp + 0.35f * cameraRight);
```

Recomputed each frame as the camera orbits. Colour `#5FA8FF`, intensity 0.35, and — critically —
**applied to actors only, not to terrain**. It puts a cool edge along the top-back of every
miner and monster, which separates them from whatever wall is behind. This is the cheap,
tasteful version of an outline (§3.5) and it should be tried before any outline shell.

Add a `pow(1 - saturate(dot(n, viewDir)), 2.0)` Fresnel weight to concentrate it at the edges:

```
float fres = 1.0f - saturate(dot(n, viewDir));
rgb += rimColour * 0.35f * fres * fres * saturate(dot(n, rimDir));
```

### 2.5 Why warm key against cool ambient creates depth

Four mechanisms, and it is worth knowing all four because they tell you when the trick will
fail:

1. **It doubles the channels carrying form.** With one white light, form is expressed only in
   value; if fog or a dark palette compresses the value range, form disappears. With a warm key
   and a cool ambient, form is *also* expressed in hue, and hue survives value compression. This
   is why the trick is specifically valuable in a dark subterranean setting.
2. **Chromatic advance.** The eye's lens has longitudinal chromatic aberration: long wavelengths
   focus behind short ones, so warm surfaces are read as nearer and cool surfaces as further.
   Warm-lit near geometry and cool-lit far geometry therefore reinforce the depth the perspective
   projection already gives you.
3. **Hue boundaries segment better than low-contrast value boundaries.** The lit/unlit terminator
   becomes a hue edge, which the visual system finds even when the value difference across it is
   small. In practice: your shadows can be much lighter (and therefore less muddy) without the
   form collapsing.
4. **Theatrical attention.** The lamp is a moving pool that says *here*. A game where the light
   follows the thing you control is a game where you always know what to look at, and that
   legibility is itself a large part of the "finished" impression.

The failure mode to avoid: if the ambient is too bright, the lamp cannot win and you get a flat,
evenly-lit scene with a slight orange stain. **Keep the ambient contribution to a lit surface at
30–40 % of what the lamp gives at 2 units.** With the numbers above, ambient on an up-face is
≈ 0.30 and the lamp at 2 units is ≈ `2.2 * (1/5) ≈ 0.44` before the cone — a ratio of ~0.68,
which is too high. **Drop the sky ambient to 0.20 and the ground to 0.07** and re-check; the
correct ratio is a tuning decision but the *method* is: log the two numbers at a known distance
and set the ratio explicitly rather than eyeballing it.

### 2.6 Do the maths in linear space, and do it on the CPU

**The single most common reason procedural colours look wrong is doing lighting arithmetic in
sRGB.** `Cube()` currently does `r * k` on sRGB values, which is why `k = 0.35` for the
underside does not look like "35 % as much light" — it looks like a smear. Multiplying sRGB by
0.5 is roughly a 0.73 change in actual light.

```cpp
// --- colour.hpp sketch ---------------------------------------------------
struct Rgb { float r, g, b; };

inline float SrgbToLinear(float c)
{
    return (c <= 0.04045f) ? (c / 12.92f)
                           : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
inline float LinearToSrgb(float c)
{
    return (c <= 0.0031308f) ? (c * 12.92f)
                             : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
}

// Compile-time-ish palette entry from a hex literal. Decoded once at startup.
inline Rgb FromHex(unsigned int hex)
{
    const float r = static_cast<float>((hex >> 16) & 0xFFu) / 255.0f;
    const float g = static_cast<float>((hex >>  8) & 0xFFu) / 255.0f;
    const float b = static_cast<float>((hex      ) & 0xFFu) / 255.0f;
    return { SrgbToLinear(r), SrgbToLinear(g), SrgbToLinear(b) };   // LINEAR
}

// Pack a linear colour for the vertex stream (GL_C4UB, so encode to sRGB here).
inline unsigned int PackLinear(const Rgb& c, float a = 1.0f)
{
    const float r = LinearToSrgb(c.r < 0 ? 0 : (c.r > 1 ? 1 : c.r));
    const float g = LinearToSrgb(c.g < 0 ? 0 : (c.g > 1 ? 1 : c.g));
    const float b = LinearToSrgb(c.b < 0 ? 0 : (c.b > 1 ? 1 : c.b));
    return  static_cast<unsigned int>(r * 255.0f + 0.5f)
         | (static_cast<unsigned int>(g * 255.0f + 0.5f) <<  8)
         | (static_cast<unsigned int>(b * 255.0f + 0.5f) << 16)
         | (static_cast<unsigned int>(a * 255.0f + 0.5f) << 24);
}
```

**Why CPU, not `glEnable(GL_LIGHTING)` with `GL_LIGHT0..7`?** Three reasons, all decisive:

1. Fixed-function GL lighting operates on whatever numbers you give it, in *no* colour space,
   and clamps per-light to `[0,1]` before summing. You cannot do linear accumulation with a
   final encode. The rig above would come out wrong and you would spend the whole budget
   fighting it.
2. Fixed-function attenuation is only `1/(k0 + k1·d + k2·d²)`. No window, no smooth zero, no
   cull radius, and the spot exponent (`GL_SPOT_EXPONENT`) is a `cos^n` curve which cannot
   express an inner/outer cone.
3. It costs a `glNormal3f` per vertex plus per-light state changes, versus a `unsigned int`
   already sitting in your interleaved array.

The CPU cost is negligible. See §2.8.

### 2.7 Fog and the background gradient

Replace the linear fog:

```cpp
glFogi(GL_FOG_MODE, GL_EXP2);
glFogf(GL_FOG_DENSITY, 0.030f);
const float fog[4] = { /* FOG_NEAR #111B2A as sRGB floats */ 0.067f, 0.106f, 0.165f, 1.0f };
glFogfv(GL_FOG_COLOR, fog);
```

`GL_EXP2` matches how the eye reads aerial perspective far better than a linear ramp with a hard
start plane: the current `GL_FOG_START 26.0` produces a *visible band* at 26 units where fog
switches on, which is itself a prototype tell.

Then draw a **background gradient quad** before the world, with depth test and depth write off,
in ortho: `HORIZON_LIFT #1A2536` at the top of the screen to `SKY_VOID #070B12` at the bottom.
Two triangles, four `glColor4ub` calls. It removes the "flat black void" read and gives the fog
something to fade *into* that is not a constant.

**Fog colour must be the ambient sky colour, not the clear colour.** Right now they are the same
value (`0.03, 0.035, 0.05`, `:643-646`), which means distant geometry fades into pure void; when
the fog colour is a lifted, slightly blue ambient, distant geometry fades into *atmosphere*.

### 2.8 Cost

Per frame, per lit vertex: one hemisphere lerp (3 mul, 3 add), one lamp evaluation (~14 flops)
if inside `R`, up to 6 crystal evaluations, one rim term for actors. Terrain is 40×40 tiles;
floors tessellated 3×3 and walls 2×2 gives roughly **60 000 vertices**. Only ~180 tiles fall
inside the lamp radius, so the per-frame *dynamic* set is ~6 500 vertices; the rest can be
lit once and cached (§3.2). At ~30 flops/vertex the dynamic cost is under 0.2 M flops per frame.
This is not a performance question.

The real cost is the **draw path**: 60 000 vertices through `glBegin`/`glVertex3f` is ~120 000
API calls per frame and will cost you several milliseconds. Use client-side vertex arrays —
core GL 1.1, zero dependencies:

```cpp
struct Vtx { float u, v; unsigned int rgba; float x, y, z; };   // 24 bytes
static_assert(sizeof(Vtx) == 24, "GL_T2F_C4UB_V3F layout");

std::vector<Vtx> g_terrainVerts;   // rebuilt on drill; recoloured per frame in the lamp radius

glInterleavedArrays(GL_T2F_C4UB_V3F, 0, g_terrainVerts.data());
glDrawArrays(GL_QUADS, 0, static_cast<GLsizei>(g_terrainVerts.size()));
```

One call for the whole world. `GL_T2F_C4UB_V3F` is a documented `glInterleavedArrays` format in
GL 1.1, so this needs no extension and no loader.

### 2.9 Make the palette machine-checked

Palette drift — someone nudges a hex and quietly breaks the value ladder — is how art direction
dies six weeks in. Prevent it with 20 lines:

```cpp
inline float RelLuminance(const Rgb& lin)   // lin = already-linear
{ return 0.2126f*lin.r + 0.7152f*lin.g + 0.0722f*lin.b; }

inline float Lstar(float Y)
{ return (Y > 0.008856f) ? (116.0f * std::pow(Y, 1.0f/3.0f) - 16.0f) : (903.3f * Y); }

#ifdef _DEBUG
void CheckPalette()
{
    const float want[5] = { 12.0f, 20.0f, 33.0f, 48.0f, 63.0f };
    for (int biome = 0; biome < 4; ++biome)
        for (int step = 0; step < 5; ++step) {
            const float got = Lstar(RelLuminance(kRamp[biome][step]));
            if (std::fabs(got - want[step]) > 3.0f)
                ::OutputDebugStringA("palette: ramp out of tolerance\n");
        }
}
#endif
```

Now the ladder is a build-time invariant, not a memory.

---

## 3. Silhouette and readability

### 3.1 Why blocky characters can look great

Blocky characters read well when the boxes are arranged so that **mass**, **proportion** and
**one asymmetry** do the work that surface detail would otherwise do. The industry framing:
roughly 70 % of a design's impact comes from silhouette and 30 % from detail, and detail that
distorts the silhouette read is noise
([80.lv, Character Design: Shape Language and Readability](https://80.lv/articles/character-design-shape-language-and-readability)).
Riot's character-art guidance uses a **70/30 rule** — about 70 % of the visual interest is placed
in the upper body: head, shoulders, torso, and the held tool
([Riot Games, Character Art](https://www.riotgames.com/en/artedu/character-art)). Blizzard's
stylised pipeline exaggerates head size by 15–20 % over realistic proportion.

Ours violates all of it. `DrawMiner` (`:389-397`):

| Part | Half-extents `(sx, sy, sz)` | Actual size |
| --- | --- | --- |
| torso | `0.17, 0.17, 0.12` | 0.34 × 0.34 × 0.24 (note: `Cube` uses `y0 = cy`, `y1 = cy + sy*2`) |
| head | `0.13, 0.10, 0.12` | 0.26 × 0.20 × 0.24 |
| helmet | `0.16, 0.05, 0.15` | 0.32 × 0.10 × 0.30 |
| lamp | `0.05, 0.03, 0.03` | tiny |
| legs | `0.05, 0.10, 0.05` | 0.10 × 0.20 × 0.10 |

Total standing height ≈ 0.80 units; head-plus-helmet ≈ 0.30 → about **2.7 heads tall**, but the
head is only 0.26 wide against a 0.34-wide torso, so the *widths* are near-identical top to
bottom. The result is a column with a slight bulge, and the helmet — the one distinguishing
feature — is only 0.10 tall, so it disappears at any distance. **We built a
realistic-proportioned figure out of boxes, which is the worst of both worlds:** it has neither
the readability of a stylised toy nor the information of a detailed model.

### 3.2 Proportion rules — concrete respec

Set the miner as the unit of scale: 1.0 = miner standing height, which we fix at **1.15 world
units** (up from ~0.80, so it reads against 1.0-unit tiles).

| Part | Fraction of height | Width relative to torso | Rationale |
| --- | --- | --- | --- |
| Helmet | 0.13 | **1.55×** | the widest thing on the figure — this is the whole silhouette read |
| Head | 0.15 | 0.70× | narrow head under a wide helmet = a strong negative-space notch |
| Shoulders / torso | 0.34 | 1.00× (define = 0.34 units wide) | |
| Hips | — | 0.72× | taper creates a shoulder read |
| Legs | 0.38 | 0.28× each, splayed 0.09 apart | thin legs under a heavy top = "carrying gear" |
| Backpack | protrudes 0.14 behind | 0.62× | **the asymmetry** — visible from every angle except dead-front |
| Drill arm | right side only, +0.10 length | | the second asymmetry; also tells you what he does |

Head + helmet = 0.28 of total height ≈ **3.6 heads tall**. That is the stylised-readable band
(3.5–4.5) and it is a two-line change to the constants in `DrawMiner`.

Add one non-axis-aligned element. Every single box in the file is axis-aligned. A helmet brim
rotated 8° forward, or a backpack canted 5°, breaks the grid read instantly and costs one
`glRotatef`.

### 3.3 The silhouette test, in-engine, for ~15 lines

Add a debug key that renders the frame as pure black on white:

```cpp
if (g_silhouetteTest) {
    glClearColor(0.92f, 0.92f, 0.94f, 1.0f);
    glDisable(GL_FOG);
    g_forceFlatColour = true;             // every draw uses glColor4ub(0,0,0,255)
}
```

Rule: **if a miner and a monster are not instantly distinguishable in that view, the design has
failed and no amount of colour will save it.**

Second test, the squint test, also cheap: render at 1/8 resolution and blit it back magnified.
With `glCopyTexSubImage2D` (core GL 1.1) into a 160×102 texture and a full-screen textured quad
with `GL_NEAREST`, that is about 12 lines. What survives at 1/8 res is what the player actually
perceives at a glance.

### 3.4 Scale contrast between miner and monster

| Class | Height (miner = 1.0) | Width | Posture | Read |
| --- | --- | --- | --- | --- |
| Miner | 1.00 | 0.34 units | upright, narrow, head high | vulnerable, bipedal |
| Small monster (`species 1`) | 0.80 | **0.62** | low, wide, head forward and *below* shoulder line | scuttling, many |
| Standard (`species 0`) | 1.65 | 0.95 | hunched, arms long | the fight |
| Elite (`species 2`) | 2.60 | 1.60 | massive, slow, head at miner-eye height despite the size | the problem |

The two rules that matter more than the numbers:

- **Aspect-ratio contrast, not just size.** A monster that is 1.6× a miner in *both* dimensions
  is just a big miner. A monster that is 1.65× tall and 2.8× wide is a different *kind* of thing.
- **Head height as posture.** Miners' heads are at the top of their silhouette; monsters' heads
  should be forward and low. That single difference reads as predator-versus-prey before any
  colour is applied, and in `DrawMonster` it is a change to two `Cube` calls.

The current `mo.scale = 0.75f + rng.Unit() * 0.8f` (`:307`) randomises size *continuously*, which
is exactly wrong for readability: it gives you a smear of ambiguous sizes instead of three
distinct classes. Replace with a discrete class index and ±6 % jitter within the class.

### 3.5 Outline and rim, ranked

1. **Rim light (§2.4 L3) — do this first.** No extra draw call, no extra geometry, and it reads
   as lighting rather than as cel-shading. On actors only.
2. **Inverted-hull outline shell — second, and only on selected units and monsters.** Draw the
   figure a second time with front faces culled and vertices pushed along their normal:

```cpp
// Outline pass. Actors only. Draw BEFORE the lit pass so depth works out.
void OutlineShell(const Actor& a, float pixelWidth, unsigned int colour, float distToCam)
{
    // Constant screen-space width: expand by an amount proportional to view distance.
    const float expand = pixelWidth * distToCam * kTanHalfFovOverScreenHeight;
    glCullFace(GL_FRONT);
    glColor4ubv(reinterpret_cast<const GLubyte*>(&colour));
    DrawActorGeometry(a, expand);          // each vertex offset by expand * faceNormal
    glCullFace(GL_BACK);
}
```

For axis-aligned cubes "offset along the normal" is just growing the half-extents by `expand`,
so this costs one extra float in the existing `Cube()` signature. `pixelWidth` of 2.0–2.5 at
1080p.

**Do not outline the terrain.** Outlined terrain turns a cave into a technical diagram. And do
not use black: use the biome's `E0` (`#1B222C` for basalt), which reads as an art choice rather
than as a filter.

3. **Depth-buffer edge detection** — needs a full-resolution readback or a render-to-texture path
   we do not have in 1.x. Skip; the ratio is terrible.

### 3.6 Voxel vertex ambient occlusion — the terrain's silhouette

This is separate from actor silhouette but it belongs here because it is what makes voxel
terrain read as *carved* rather than as *stacked*. The algorithm is Mikola Lysenko's, from the
original Minecraft AO mod
([0 FPS, Ambient occlusion for Minecraft-like worlds](https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/)):

```cpp
// side1, side2, corner are 1 if the corresponding neighbour voxel is solid.
// Returns 0 (fully occluded) .. 3 (unoccluded).
inline int VertexAO(int side1, int side2, int corner)
{
    if (side1 && side2) return 0;
    return 3 - (side1 + side2 + corner);
}
```

Per face you sample the 8 neighbours in the plane displaced by the face normal; each of the four
corners takes its two adjacent sides and its diagonal. Map `{0,1,2,3}` to occlusion multipliers
`{0.42f, 0.62f, 0.82f, 1.00f}` — non-linear on purpose, because the darkest step is the one that
does the work. Multiply into the vertex's *linear* colour before the encode.

The quad-diagonal flip rule, also from that article, prevents the interpolation seam artefact:

```cpp
// a00,a01,a11,a10 are the corner AO values clockwise.
if (a00 + a11 > a01 + a10) { EmitFlippedQuad(); } else { EmitNormalQuad(); }
```

Since we already tessellate faces (§2.4), apply AO at the *tile-corner* vertices and bilinearly
interpolate across the sub-quads.

**Cost:** 8 neighbour lookups per face, computed only when the terrain mesh is rebuilt — which is
already gated by the existing `level.RecomputeWalls()` call at `:246`. This is the highest
value-per-hour item after the light rig, and it is exactly the "modest mesh construction time
for an enormous quality improvement" trade the source article describes.

### 3.7 Procedural surface texture (the 7th item, not the 6th)

Two frequencies is enough. Generate at startup into a 256×256 RGB texture and a 128×128 detail
texture, upload with `gluBuild2DMipmaps` (in `glu32`, already linked at `:47`):

```cpp
// Tileable value noise: hash on a wrapped lattice so the texture wraps exactly.
inline float Hash2(int x, int y, int period, unsigned int seed)
{
    x = ((x % period) + period) % period;
    y = ((y % period) + period) % period;
    unsigned int h = static_cast<unsigned int>(x) * 374761393u
                   + static_cast<unsigned int>(y) * 668265263u + seed;
    h = (h ^ (h >> 13)) * 1274126177u;
    return static_cast<float>(h ^ (h >> 16)) * (1.0f / 4294967296.0f);
}

inline float SmoothStepF(float t) { return t * t * (3.0f - 2.0f * t); }

float ValueNoise(float x, float y, int period, unsigned int seed)
{
    const int   xi = static_cast<int>(std::floor(x)), yi = static_cast<int>(std::floor(y));
    const float xf = x - static_cast<float>(xi),      yf = y - static_cast<float>(yi);
    const float u  = SmoothStepF(xf),  v = SmoothStepF(yf);
    const float a = Hash2(xi,   yi,   period, seed), b = Hash2(xi+1, yi,   period, seed);
    const float c = Hash2(xi,   yi+1, period, seed), d = Hash2(xi+1, yi+1, period, seed);
    return (a + (b-a)*u) + ((c + (d-c)*u) - (a + (b-a)*u)) * v;
}

// FBM with domain warp — the warp is what turns "noise" into "rock".
float RockField(float x, float y, unsigned int seed)
{
    const float wx = ValueNoise(x * 2.0f, y * 2.0f, 8,  seed + 11u) - 0.5f;
    const float wy = ValueNoise(x * 2.0f, y * 2.0f, 8,  seed + 29u) - 0.5f;
    float f = 0.0f, amp = 0.5f, frq = 4.0f;
    int   per = 16;
    for (int o = 0; o < 5; ++o) {
        f   += amp * ValueNoise((x + wx * 0.35f) * frq, (y + wy * 0.35f) * frq, per, seed + static_cast<unsigned int>(o) * 101u);
        amp *= 0.5f; frq *= 2.0f; per *= 2;
    }
    return f;
}
```

Wrapping the lattice by `period` and doubling `period` alongside `frq` is what makes every octave
tile, so the whole FBM tiles — the standard construction
([Tileable Procedural Textures, alinloghin.com](http://alinloghin.com/articles/tileable_textures/)).
Domain warping (`f(p) = fbm(p + fbm(p))`) is what produces the fractured, non-blobby look that
reads as rock rather than as clouds.

Bind with `GL_MODULATE` so the texture multiplies the lit vertex colour — that keeps every
biome recolour working with one texture. Use a second, high-frequency **detail** texture at
8× the UV rate via `GL_ARB_multitexture` (universally available) or, if you want to avoid even
that, bake the detail into the base texture's alpha and skip it. Two frequencies kills the
"single flat tint stretched over a 40-metre wall" read.

---

## 4. Particles and juice

The canonical demonstration that this layer is where the perceived quality lives is Jonasson &
Purho's *Juice it or lose it* (Nordic Game Jam 2012), which takes a working Breakout and adds
nothing but juice
([gamejuice.co.uk write-up](https://gamejuice.co.uk/resources/juice-it-or-lose-it)); and Jan
Willem Nijman's *The art of screenshake* (INDIGO Classes 2013), which lists ~30 discrete
techniques including permanence, impact frames, camera lerp and kickback
([talk](https://www.youtube.com/watch?v=AJdEqssNZ-U)).

### 4.0 Prerequisite bug: the clock

Before any of this, fix the timebase. `DeepCore3D.cpp:656,666-668` uses `::GetTickCount()`,
whose resolution is the system timer tick — typically **15.6 ms**. At 60 fps you are quantising
a 16.6 ms frame to a 15.6 ms grid, so `dt` alternates between 0 and 15.6 and 31.2. Every eased
motion, every particle integration and every camera damp in this document will visibly judder
on top of that, and you will blame the smoothing.

```cpp
LARGE_INTEGER freq, now, prev;
::QueryPerformanceFrequency(&freq);
::QueryPerformanceCounter(&prev);
// per frame:
::QueryPerformanceCounter(&now);
float dt = static_cast<float>(static_cast<double>(now.QuadPart - prev.QuadPart)
                              / static_cast<double>(freq.QuadPart));
prev = now;
if (dt > 0.10f) dt = 0.10f;
```

And replace `::Sleep(8)` (`:717`) with vsync, obtained through `wglGetProcAddress` — still zero
dependencies:

```cpp
typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
    reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(::wglGetProcAddress("wglSwapIntervalEXT"));
if (wglSwapIntervalEXT) wglSwapIntervalEXT(1);
```

Judder-free presentation is itself a large fraction of "finished", and this is 10 lines.

### 4.1 The particle system — pooled, zero per-frame allocation

```cpp
// --- particles.hpp sketch -------------------------------------------------
enum PKind : unsigned char { PK_SOFT = 0, PK_SPARK, PK_STAR, PK_GRIT, PK_CUBE };

struct Particle
{
    V3    p, v;
    float life;          // counts DOWN, seconds
    float invLife;       // 1 / lifetime, precomputed
    float size0, size1;
    float drag;          // per-second velocity decay coefficient
    float gravity;       // world units / s^2, negative = down
    float angle, spin;   // billboard roll
    unsigned int c0, c1; // packed RGBA at birth and at death
    unsigned char kind;
    unsigned char bounce;// 1 = collide with y = 0
};

constexpr int kMaxParticles = 4096;
Particle g_pool[kMaxParticles];
int      g_liveCount = 0;

inline Particle* Spawn()
{
    if (g_liveCount >= kMaxParticles) return nullptr;   // hard cap, never allocates
    return &g_pool[g_liveCount++];
}

void UpdateParticles(float dt)
{
    for (int i = 0; i < g_liveCount; )
    {
        Particle& q = g_pool[i];
        q.life -= dt;
        if (q.life <= 0.0f) {
            g_pool[i] = g_pool[--g_liveCount];          // swap-remove: O(1), no free list
            continue;
        }
        q.v.y += q.gravity * dt;
        const float k = 1.0f - q.drag * dt;             // cheap exponential-ish drag
        q.v.x *= k; q.v.y *= k; q.v.z *= k;
        q.p.x += q.v.x * dt; q.p.y += q.v.y * dt; q.p.z += q.v.z * dt;
        q.angle += q.spin * dt;
        if (q.bounce && q.p.y < 0.02f && q.v.y < 0.0f) {
            q.p.y = 0.02f;
            q.v.y *= -0.25f;                            // restitution
            q.v.x *=  0.60f; q.v.z *= 0.60f;            // lateral damping
        }
        ++i;
    }
}
```

Swap-remove instead of a free list is deliberate: the live set stays contiguous, so the draw loop
is a linear scan with no indirection and no dead-slot branch.

### 4.2 Drawing — two draw calls, total

Billboard basis is extracted **once per frame**, not per particle, by reading the camera's right
and up axes out of the modelview matrix (they are the first two *rows* of the upper-left 3×3,
which in column-major storage are elements 0/4/8 and 1/5/9):

```cpp
float mv[16];
glGetFloatv(GL_MODELVIEW_MATRIX, mv);
const V3 camRight{ mv[0], mv[4], mv[8]  };
const V3 camUp   { mv[1], mv[5], mv[9]  };
```

Then:

```cpp
void DrawParticles()
{
    glDepthMask(GL_FALSE);            // particles never write depth
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_particleAtlas);
    glDisable(GL_CULL_FACE);

    // Pass 1: additive (sparks, stars, glow). No sorting needed - additive commutes.
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    EmitBatch(/*additiveKinds*/);

    // Pass 2: alpha (dust, smoke, grit, cubes). Not sorted - see note.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    EmitBatch(/*alphaKinds*/);

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}
```

**Deliberately not sorting the alpha pass.** With depth writes off and dust that is low-contrast
and low-alpha by design, sorting error is invisible, and a per-frame sort of a few hundred
elements is budget spent where it does not show. If you later add high-contrast smoke, insertion-sort
by view depth (nearly-sorted input, so insertion sort is ~O(n)).

`EmitBatch` builds into a reused `std::vector<Vtx>` (reserve once at startup) and issues one
`glInterleavedArrays` + `glDrawArrays`. Two draw calls for the entire effects layer.

**Streaks matter.** A spark drawn as a round dot reads as a firefly. A spark drawn as a quad
stretched along its own velocity reads as *speed*:

```cpp
// For PK_SPARK: replace camUp with the velocity direction, scaled by speed.
const V3 dir   = Normalise(q.v);
const V3 along = dir * (q.size0 + Length(q.v) * 0.045f);
const V3 side  = Normalise(Cross(dir, camForward)) * (q.size0 * 0.35f);
```

That is four lines and it is the difference between "particles" and "sparks".

### 4.3 The atlas — 128×128, generated at startup, 2×2 cells

| Cell | Kind | Generator |
| --- | --- | --- |
| (0,0) | `PK_SOFT` dust/smoke | `a = pow(saturate(1 - 2r), 2.0)`, rgb = 255 |
| (0,1) | `PK_SPARK` | `a = pow(saturate(1 - 2r), 6.0) + 0.9 * step(r, 0.06)` — tight core, fast falloff |
| (1,0) | `PK_STAR` 4-point flare | `a = pow(saturate(1-2r),3) * (0.25 + 0.75 * pow(max(|cos2θ|, |sin2θ|), 6))` |
| (1,1) | `PK_GRIT` | `a = step(0.5, ValueNoise(u*7, v*7, 7, seed)) * pow(saturate(1-2r), 1.5)` |

```cpp
void BuildParticleAtlas()
{
    static unsigned char px[128 * 128 * 4];
    for (int y = 0; y < 128; ++y)
    for (int x = 0; x < 128; ++x)
    {
        const int cell = (y >= 64 ? 2 : 0) + (x >= 64 ? 1 : 0);
        const float u = (static_cast<float>(x % 64) + 0.5f) / 64.0f * 2.0f - 1.0f;
        const float v = (static_cast<float>(y % 64) + 0.5f) / 64.0f * 2.0f - 1.0f;
        const float r = std::sqrt(u * u + v * v);
        float a = 0.0f;
        switch (cell) {
            case 0: a = std::pow(Saturate(1.0f - r), 2.0f); break;
            case 1: a = std::pow(Saturate(1.0f - r), 6.0f) + (r < 0.10f ? 0.9f : 0.0f); break;
            case 2: {
                const float th = std::atan2(v, u);
                const float sp = std::pow(Max(std::fabs(std::cos(2.0f*th)),
                                              std::fabs(std::sin(2.0f*th))), 6.0f);
                a = std::pow(Saturate(1.0f - r), 3.0f) * (0.25f + 0.75f * sp);
            } break;
            default: a = (ValueNoise(u * 3.5f, v * 3.5f, 7, 1234u) > 0.5f ? 1.0f : 0.0f)
                       * std::pow(Saturate(1.0f - r), 1.5f); break;
        }
        const int i = (y * 128 + x) * 4;
        px[i+0] = px[i+1] = px[i+2] = 255;
        px[i+3] = static_cast<unsigned char>(Saturate(a) * 255.0f + 0.5f);
    }
    glGenTextures(1, &g_particleAtlas);
    glBindTexture(GL_TEXTURE_2D, g_particleAtlas);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
```

Note the atlas is white-with-alpha; every particle is tinted by its own `glColor4ub`, so one
texture serves the entire palette. Add a 1-texel inset to each cell's UVs to stop mip bleeding
between cells.

### 4.4 Emitters, ranked by impact per line

**1. Rock debris on break — highest impact in the document's particle section.**
It converts an instantaneous state change (the block ceases to exist at `:244`) into an *event*.

| Parameter | Value |
| --- | --- |
| Count | 16 `PK_CUBE` + 8 `PK_SOFT` + 1 flash |
| Cube size | 0.05–0.11, `spin` 4–11 rad/s about a random axis |
| Velocity | hemisphere biased toward the drilling miner; speed 2.5–5.5 |
| Gravity | −13.0 |
| Drag | 0.30 |
| Life | 0.7–1.4 s |
| Bounce | yes, restitution 0.25, lateral 0.60 |
| Colour | **`E2` of the destroyed block's own biome** → `E0`; debris is biome-correct for free |
| Dust | `PK_SOFT`, size 0.35 → 1.50, life 0.9 s, colour `E1` @ α 0.50 → 0 |
| Flash | one screen-facing quad at the wall face, `#FFF3D0`, α 0.85, 0.05 s, additive |
| Trauma | +0.25 (§4.5) |
| Hit-stop | 45 ms (§4.6) |

**2. Drill dust and sparks (continuous while drilling).** This is what makes drilling feel like
*work* rather than like a progress bar. `Miner::drilling` and `drillProgress` already exist
(`:75-76`), so the hook is a two-line addition to the update.

- Dust: rate 25/s, `PK_SOFT`, `v = wallNormal * 0.6` plus a 55° random cone, gravity −1.0,
  drag 1.2, size 0.10 → 0.45, life 0.5 s, colour block-`E2` @ α 0.45 → 0.
- Sparks: rate 18/s, `PK_SPARK` drawn as streaks, speed 3–7 in a 70° cone about the reflection
  of the drill axis off the wall normal, gravity −18, drag 0.5, life 0.25 s,
  colour `#FFD07A` → `#FF5A18`, size 0.03, `bounce = 1` (sparks that skitter off the floor are
  disproportionately convincing).
- Trauma: +0.06 **per 0.25 s**, not per frame — a continuous trauma feed reads as a rumble, which
  is right for a drill.

**3. Emerge dust burst (monster spawn).** `Monster::emerge` already ramps 0→1 over 1.3 s (`:277`).

- 30 `PK_SOFT` + 10 `PK_GRIT` emitted in a **ring** of radius 0.35, outward speed 2.2, upward
  1.0, life 1.1 s — a ring reads as "something pushed the floor up"; a sphere reads as "an
  explosion", which is the wrong verb.
- Displace the terrain: raise the emerging tile's floor vertices by `0.12f * sinf(PI * emerge)`.
  Free — it uses a parameter you already compute.
- Trauma +0.35, and the camera framing bias in §5.5.

**4. Crystal sparkle.** One `PK_STAR` per *visible* crystal, additive, size pulsing 0.10 → 0.22
on a per-crystal random phase (reuse the `x*3 + z` phase already at `:452`). Then the important
part: **rarity**. With probability 0.6/s per crystal, spawn one 0.18 s star at size 0.45 and full
`CRYSTAL_CORE` white. Constant sparkle reads as a lightbulb; occasional sparkle reads as a gem.
Also give the persistent star a slow `spin` of 0.4 rad/s.

**5. Damage flash and squash.** On an actor taking damage:

```cpp
a.flash = 1.0f;                          // decays: flash -= dt / 0.11f
// at draw time:
const float f  = a.flash * a.flash;
const float sx = 1.0f + 0.12f * f, sy = 1.0f - 0.08f * f;   // squash
glScalef(sx, sy, sx);
// and bias the vertex colour toward white in LINEAR space:
lit.r += 2.5f * f; lit.g += 2.5f * f; lit.b += 2.5f * f;
```

Squash-and-stretch on a box character is startlingly effective for three lines. Pair with a
full-screen `DAMAGE_WASH #FF6A5A` quad at α `0.14 * f` when the damaged actor is the player's
selection.

**6. Footstep puffs.** The leg-swing phase is already computed (`m.phase`, `:230`). Emit one
`PK_SOFT` each time `sin(phase)` crosses zero while `hasTarget`: size 0.12 → 0.40, life 0.45 s,
`v = up 0.30 + backward 0.40`, α 0.35 → 0, colour floor-`E1`. Combined with blob shadows (§4.7)
this is the entire "contact" fix and it is under 20 lines.

**7. Screen shake.** §4.5.

**8. Hit-stop.** §4.6.

**9. Ambient cave motes.** 120 persistent `PK_SOFT` particles in a 30-unit box that re-wraps
around the camera focus, size 0.02, α 0.10, drifting at 0.15 u/s with a slow sinusoidal sway.
Tint them by the lamp (they are already going through the same lighting helper), which makes the
lamp's cone visible in the air **without any volumetric work at all** — this is the single
cheapest way to get light shafts in a fixed-function renderer.

**10. Floating numbers.** "+1" at the crystal site, rising 0.6 units and fading over 0.7 s, in
`CRYSTAL_INNER`, drawn with the §6 font atlas as a billboard. Replaces the banner text at
`:249-250` with something spatial.

### 4.5 Screen shake — trauma model

Eiserloh's formulation: keep a `trauma` scalar in `[0,1]`, decay it linearly, and drive the shake
by `trauma²` (or `trauma³`) so that small traumas are nearly invisible and large ones are violent
([GDC 2016, *Juicing Your Cameras With Math*](https://gdcvault.com/play/1023146/Math-for-Game-Programmers-Juicing);
[slides](http://www.mathforgameprogrammers.com/gdc2016/GDC2016_Eiserloh_Squirrel_JuicingYourCameras.pdf)).

**In 3D, use rotational shake only.** Translational shake pushes the camera through geometry and
reads as a broken camera rather than as impact — this is Eiserloh's explicit recommendation.

```cpp
float g_trauma = 0.0f;

inline void AddTrauma(float t) { g_trauma = Min(1.0f, g_trauma + t); }

void ApplyShake(float t, float& yaw, float& pitch, float& roll)
{
    g_trauma = Max(0.0f, g_trauma - 1.6f * kDt);
    const float s = g_trauma * g_trauma;
    const float f = 24.0f;                        // Hz
    yaw   += 0.0157f * s * Noise1D(t * f, 0u);    // 0.90 degrees
    pitch += 0.0157f * s * Noise1D(t * f, 1u);
    roll  += 0.0279f * s * Noise1D(t * f, 2u);    // 1.60 degrees - roll carries the punch
}
```

The noise must be **coherent**, not `rand()` per frame — white noise buzzes; smooth noise shakes.
A 1D value noise in twelve lines:

```cpp
inline float Hash1(int i, unsigned int seed)
{
    unsigned int h = static_cast<unsigned int>(i) * 2654435761u + seed * 40503u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return static_cast<float>(h) * (2.0f / 4294967296.0f) - 1.0f;   // [-1, 1]
}
inline float Noise1D(float x, unsigned int seed)
{
    const int   i = static_cast<int>(std::floor(x));
    const float f = x - static_cast<float>(i);
    const float u = f * f * (3.0f - 2.0f * f);
    return Hash1(i, seed) + (Hash1(i + 1, seed) - Hash1(i, seed)) * u;
}
```

Trauma budget — keep it small, or it stops meaning anything:

| Event | Trauma |
| --- | --- |
| Drill tick (per 0.25 s) | 0.06 |
| Wall breaks | 0.25 |
| Crystal recovered | 0.15 (a *good* shake — pair with the camera pitch-up in §5.5) |
| Monster emerges | 0.35 |
| Miner dies | 0.60 |
| Crew wiped | 1.00 |

### 4.6 Hit-stop — five lines, disproportionate return

```cpp
float g_stopTimer = 0.0f;   // seconds of remaining freeze

// In the frame loop, before g.Update():
if (g_stopTimer > 0.0f) { g_stopTimer -= realDt; dt = 0.0f; }
```

Freeze durations: wall break 45 ms, monster death 70 ms, miner death 110 ms, crew wipe 250 ms.
Then ease the timescale back to 1.0 over ~90 ms rather than snapping. Nijman's "impact frames"
point: pausing the world for a few frames on impact is what gives a hit weight
([The art of screenshake](https://www.youtube.com/watch?v=AJdEqssNZ-U)).

Note the freeze must **not** stop particles or the camera — freeze gameplay, keep presentation
running, otherwise the effect reads as a stutter bug.

### 4.7 Blob shadows — the contact fix

```cpp
// Generate once: 64x64 radial alpha.
for (y, x) { r = length(uv); a = pow(saturate(1 - r), 2.2f) * 0.72f; }

// Draw, per actor, after terrain and before actors:
glDepthMask(GL_FALSE);
glEnable(GL_BLEND);
glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);   // MULTIPLY-DARKEN, not alpha-over
glEnable(GL_POLYGON_OFFSET_FILL);
glPolygonOffset(-1.0f, -1.0f);
// one quad, footprintRadius * 1.30, at the actor's floor height + 0.0
glDisable(GL_POLYGON_OFFSET_FILL);
```

`GL_ZERO, GL_ONE_MINUS_SRC_ALPHA` is the detail most people get wrong: with alpha-over you paint
a grey disc that hides the floor texture and reads as a sticker. With multiply-darken you *darken*
the floor and its texture and AO survive underneath, which is what a shadow does. Scale the
shadow's alpha by the actor's height above the floor (`α *= saturate(1 - h / 1.5f)`) so jumps and
the monster emerge look attached.

Optionally, offset the blob along the lamp direction so the selected miner's own shadow points
away from his lamp. Cheap and it makes the lamp feel real.

### 4.8 Vignette and grade — the frame

Full-screen ortho pass after everything except the HUD:

```cpp
// Vignette: one 256x256 texture, a = pow(saturate(length(uv) * 0.72f), 2.6f) * 0.55f
glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);   // darken the corners
DrawFullScreenQuad(g_vignetteTex, 0xFFFFFFFFu);

// Grade: a single flat quad, multiply then add. Two quads, no shader.
glBlendFunc(GL_DST_COLOR, GL_ZERO);             // multiply: cool the shadows
DrawFullScreenQuad(0, PackSrgb(0xB8C4D8u));     // slight blue multiply
glBlendFunc(GL_ONE, GL_ONE);                    // add: lift the blacks by a warm sliver
DrawFullScreenQuad(0, PackSrgb(0x0A0705u));
```

Multiply-then-add is a two-point grade: the multiply controls the highlight tint, the add controls
the shadow tint. It is a poor man's lift/gain and it is four draw calls. The effect on the
"finished" read is out of all proportion to the cost, because it removes the single most
prototype-flavoured property of an untouched framebuffer — that the darkest pixel is pure `#000`
and the image has no edge.

**Damage wash** and **low-health pulse** ride the same pass: one extra additive quad tinted
`DAMAGE_WASH` at α `0.14 * flash`.

### 4.9 Optional: bloom without shaders

Fully achievable in GL 1.1 and worth it once §7.2 is done, because crystals and the lamp are the
things that most want it.

1. `glCopyTexSubImage2D` the framebuffer into a 256×160 texture.
2. Draw that texture full-screen into a second 128×80 texture with `glBlendFunc(GL_SRC_ALPHA, GL_ONE)`
   several times at slightly offset UVs — this is a box blur assembled from blend ops. Four taps
   horizontally, four vertically, in two passes.
3. Bright-pass approximation: because we have no shader, get it by drawing the copy **with
   `glColor4f(1,1,1,1)` and `GL_ONE, GL_ONE` twice and then subtracting a constant with
   `GL_ONE_MINUS_DST_COLOR`** — or, much simpler and better-looking, **skip the bright pass
   entirely and instead render the emissive objects a second time into the small target** and
   blur that. Emissive-only bloom is more controllable and there is no threshold to tune.
4. Additively blend the blurred target back over the scene.

Six quads and two small textures. Note this is genuinely optional; the ratio is below everything
in §7.2.

---

## 5. Camera as an art tool

### 5.1 Diagnosis of the current camera

| Issue | Line | Why it reads as prototype |
| --- | --- | --- |
| No smoothing at all; `camYaw += dx * 0.008f` in `WM_MOUSEMOVE` | `:585-588` | The rate is per *message*, not per second, so it varies with mouse polling rate and CPU load. Also there is no inertia, and inertia is what makes something read as a camera rather than as a slider. |
| Pitch clamp `[0.25, 1.45]` rad = 14°–83° | `:587-588` | 83° is effectively top-down: all vertical faces foreshorten to nothing and the world becomes a flat map. 14° is inside the terrain. Both extremes are worse than the middle. |
| Zoom instant, ±2.5 units per wheel notch | `:594-597` | Instant zoom has no weight. |
| `WASD` pan at a constant 12 u/s with instant start and stop | `:671-676` | The world slides like a spreadsheet. |
| FOV fixed at 52° | `:686` | Wide FOV at long range exaggerates edge distortion, which reads as "tech demo". |
| The camera never reacts to the game | — | Nothing that happens in the world moves the camera. This is the biggest one. |
| No occlusion handling | — | The eye can sit inside rock. |

### 5.2 Smoothing — the correct formula

Split into `desired` (what input writes) and `actual` (what renders), and damp with the
frame-rate-independent exponential form. The naive `a = lerp(a, b, 0.1f)` is frame-rate
dependent — a 144 Hz player gets a much faster camera than a 60 Hz one
([Rory Driscoll, *Frame Rate Independent Damping Using Lerp*](https://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/)).

```cpp
inline float Damp(float a, float b, float lambda, float dt)
{ return a + (b - a) * (1.0f - std::exp(-lambda * dt)); }
```

Recommended λ values (units: e-folds per second — larger is snappier):

| Channel | λ | Why |
| --- | --- | --- |
| Focus (x, z) | 9.0 | responsive but not rigid |
| Yaw | 12.0 | orbit should feel direct |
| Pitch | 12.0 | |
| Distance | **6.0** | deliberately the slowest: zoom that lags reads as mass |
| FOV | 5.0 | |

Mouse input writes into `desired` scaled by *pixels* (`desiredYaw += dx * 0.0075f`) and the damp
handles time — which makes the whole thing correct at any frame rate for free.

### 5.3 Pitch band and its coupling to distance

Clamp to **0.52–1.02 rad (30°–58°)**. Then make the default pitch a function of zoom, so the
camera automatically becomes more dramatic close in and more map-like far out:

```cpp
const float zoomT = Smoothstep(10.0f, 55.0f, desiredDist);
const float autoPitch = Lerp(0.62f, 0.95f, zoomT);     // 35.5 deg .. 54.4 deg
desiredPitch = Clamp(autoPitch + userPitchOffset, 0.52f, 1.02f);
```

`userPitchOffset` is what the drag writes, clamped to ±0.20. The player still has control; the
camera still has taste. Six lines.

### 5.4 FOV

```cpp
desiredFov = Lerp(55.0f, 40.0f, zoomT);
```

Narrow at distance kills the edge distortion that makes wide-FOV strategy views look cheap; wide
up close adds drama. Then **dolly-zoom on action**: on a monster emerge, subtract 4° over 0.12 s
and restore over 0.5 s while simultaneously *increasing* distance by 4 % — the classic
push-pull, which produces a lurch of unease without moving the subject in frame.

### 5.5 Lookahead and framing — "the camera has an opinion"

This is the item that most separates a directed camera from an orbit control.

```cpp
V3 FocusTarget()
{
    V3 base = SelectionCentroid();
    // 1. Lookahead along the selection's velocity.
    V3 lead = SelectionVelocity() * 0.45f;
    ClampLength(lead, 3.5f);
    base = base + lead;

    // 2. Frame the fight. If a monster is close to any miner, bias toward the midpoint.
    const Threat t = NearestThreat();
    if (t.valid && t.dist < 12.0f) {
        const float w = 0.35f * (1.0f - t.dist / 12.0f);
        base = Lerp(base, Midpoint(t.miner, t.monster), w);
        g_distBias = Lerp(g_distBias, -0.12f, 1.0f - std::exp(-3.0f * kDt));  // pull in 12 %
    } else {
        g_distBias = Lerp(g_distBias, 0.0f, 1.0f - std::exp(-0.9f * kDt));    // release slowly
    }
    return base;
}
```

Plus three small behaviours that cost almost nothing:

- **Order acknowledgement.** On a right-click order (`:576-580`), ease the focus 35 % of the way
  toward the ordered tile over 0.4 s. The camera nods.
- **Crystal recovered.** Pitch *up* by 0.05 rad over 0.2 s and back over 0.6 s. A tiny upward
  tilt reads as elation; it is the camera equivalent of a smile.
- **Idle drift.** After 6 s with no input, add `dyaw = 0.02 rad/s` and a 0.15-unit vertical sine
  at 0.08 Hz. Screenshots and idle moments stop looking dead. Four lines.

### 5.6 Occlusion — never let the camera enter rock

We already have `Game::Solid(x, z)` (`:119-124`). March the eye→focus segment in 0.5-unit steps
and pull the eye in to the first free position:

```cpp
float ResolveCameraDistance(const V3& focus, const V3& dir, float wanted)
{
    for (float d = wanted; d > 4.0f; d -= 0.5f) {
        const V3 e = focus + dir * d;
        if (!g.Solid(static_cast<int>(std::floor(e.x + 0.5f)),
                     static_cast<int>(std::floor(e.z + 0.5f))))
            return d;
    }
    return 4.0f;
}
```

Damp the *result* (λ = 14 going in — fast, so you never clip; λ = 3 coming out — slow, so it does
not pop). Additionally, fade wall cubes within 3 units of the eye to α 0.25 so the near wall never
fully blocks the view. Both together are perhaps 30 lines and they eliminate the most jarring
single failure a 3D camera can have.

### 5.7 Why a fixed awkward angle makes everything look worse

Because at a bad angle you lose the two things that create the impression of three dimensions:
**overlap** and **foreshortening variety**. Near top-down (our 83° limit) every wall's vertical
face collapses, so the scene has only one visible surface orientation and the light rig from §2
has nothing to differentiate — a hemisphere ambient with only up-facing normals is a constant.
Too low (our 14° limit) and near geometry occludes everything, so the player cannot read the
level and instinctively resents the camera. The 30°–58° band is where you get: distinct top and
side faces (so the lighting works), silhouettes that overlap and therefore sort in depth, and
enough ground plane visible to plan on. **The light rig and the camera band are the same
decision**; there is no point building §2 if the player can flatten it with one drag.

---

## 6. UI and HUD craft

### 6.1 Diagnosis of the current HUD

`DrawText2D` (`:603-608`) does `SetBkMode` / `SetTextColor` / `TextOutA` on `dc` — the same
device context that `wglCreateContext` was made current on, with a pixel format requesting
`PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER` (`:632`). Problems, in order of
severity:

1. **`PFD_SUPPORT_GDI` is not requested, and could not be.** On the primary plane, `PFD_SUPPORT_GDI`
   and `PFD_DOUBLEBUFFER` are mutually exclusive for accelerated formats. Without it, GDI drawing
   on that DC does not target the GL back buffer. Community reports of exactly this combination
   describe garbled or absent text
   ([GameDev.net thread](https://www.gamedev.net/forums/topic/558160-2d-rendering-with-windows-gdi-on-top-of-3d-opengl/)).
   The calls are made *before* `SwapBuffers` (`:707-716`), so whatever lands in the front buffer
   is then overwritten. **I have not run the build, so I cannot state which of "invisible",
   "flickering" or "works on this driver" you actually get — but all three are failure modes and
   the construction is not sound.**
2. **It forces a pipeline stall.** Interleaving GDI and GL on one DC requires an implicit flush;
   you pay it every frame.
3. **It cannot participate in the image.** GDI text cannot blend, cannot be tinted per glyph,
   cannot scale, cannot animate, and sits *outside* the vignette and grade of §4.8, so it will
   look pasted on even if it renders.
4. **`CLEARTYPE_QUALITY` (`:652`) is wrong here.** ClearType writes subpixel RGB fringes tuned for
   an opaque known background. Over a dark 3D scene those fringes read as coloured haloes.
5. **A small per-frame GDI churn.** `::SelectObject(hud, font)` at `:699` is called every frame
   and the returned previous object is never restored.
6. **`W / 2 - 170` for centring (`:713`)** is a hard-coded guess at the string's pixel width. Any
   font or resolution change breaks the layout silently.

### 6.2 The replacement: bake a font atlas with GDI once, draw text as GL quads

Zero dependencies — GDI is the rasteriser, GL is the renderer. This also settles the library
question: **`stb_truetype` (public domain / MIT, single header, ~40 KB of source, no binary
dependency) would be the only defensible third-party addition here, and it is not needed**,
because `CreateFontA` + `TextOutA` into a DIB already gives us hinted, kerned glyphs from any
installed face. Recommendation: **zero libraries.**

```cpp
struct Glyph { float u0, v0, u1, v1; short w, h, bearingX, bearingY; short advance; };
Glyph  g_glyphs[96];          // ASCII 32..127
GLuint g_fontTex = 0;
int    g_fontPx  = 0;         // the size we baked at

bool BakeFontAtlas(const char* face, int pixelHeight)
{
    const int A = 512;                                   // atlas edge
    HDC   memDc = ::CreateCompatibleDC(nullptr);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = A;
    bi.bmiHeader.biHeight      = -A;                     // top-down
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = ::CreateDIBSection(memDc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ oldBm = ::SelectObject(memDc, dib);

    // ANTIALIASED_QUALITY, *not* CLEARTYPE_QUALITY: we need a grey coverage mask,
    // not per-subpixel coverage, or tinting the glyph produces colour fringes.
    HFONT f = ::CreateFontA(pixelHeight, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, ANSI_CHARSET,
                            OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                            FF_DONTCARE, face);
    HGDIOBJ oldFn = ::SelectObject(memDc, f);
    ::SetBkMode(memDc, OPAQUE);
    ::SetBkColor(memDc, RGB(0, 0, 0));
    ::SetTextColor(memDc, RGB(255, 255, 255));
    ::PatBlt(memDc, 0, 0, A, A, BLACKNESS);

    TEXTMETRICA tm{};  ::GetTextMetricsA(memDc, &tm);
    int penX = 1, penY = 1, rowH = 0;
    for (int c = 32; c < 128; ++c) {
        const char s[2] = { static_cast<char>(c), 0 };
        SIZE sz{}; ::GetTextExtentPoint32A(memDc, s, 1, &sz);
        ABC abc{};  const bool haveAbc = (::GetCharABCWidthsA(memDc, static_cast<UINT>(c),
                                                              static_cast<UINT>(c), &abc) != 0);
        if (penX + sz.cx + 2 > A) { penX = 1; penY += rowH + 2; rowH = 0; }
        ::TextOutA(memDc, penX, penY, s, 1);
        Glyph& gl = g_glyphs[c - 32];
        gl.u0 = static_cast<float>(penX)          / static_cast<float>(A);
        gl.v0 = static_cast<float>(penY)          / static_cast<float>(A);
        gl.u1 = static_cast<float>(penX + sz.cx)  / static_cast<float>(A);
        gl.v1 = static_cast<float>(penY + sz.cy)  / static_cast<float>(A);
        gl.w  = static_cast<short>(sz.cx);
        gl.h  = static_cast<short>(sz.cy);
        gl.bearingX = haveAbc ? static_cast<short>(abc.abcA) : 0;
        gl.bearingY = static_cast<short>(tm.tmAscent);
        gl.advance  = haveAbc ? static_cast<short>(abc.abcA + static_cast<int>(abc.abcB) + abc.abcC)
                              : static_cast<short>(sz.cx);
        penX += sz.cx + 2;
        if (sz.cy > rowH) rowH = static_cast<int>(sz.cy);
    }

    // DIB is BGRA with white glyphs on black -> use the blue channel as alpha, rgb = white.
    std::vector<unsigned char> tex(static_cast<std::size_t>(A) * A * 4);
    const unsigned char* src = static_cast<const unsigned char*>(bits);
    for (std::size_t i = 0; i < static_cast<std::size_t>(A) * A; ++i) {
        tex[i*4+0] = 255; tex[i*4+1] = 255; tex[i*4+2] = 255;
        tex[i*4+3] = src[i*4+0];
    }
    glGenTextures(1, &g_fontTex);
    glBindTexture(GL_TEXTURE_2D, g_fontTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, A, A, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ::SelectObject(memDc, oldFn); ::DeleteObject(f);
    ::SelectObject(memDc, oldBm); ::DeleteObject(dib);
    ::DeleteDC(memDc);
    g_fontPx = pixelHeight;
    return true;
}
```

**Bake at 2–3× the largest display size and scale down when drawing.** Free antialiasing quality,
and the HUD then scales with window size without a rebake.

The draw call gets everything GDI could not:

```cpp
float DrawTextGL(float x, float y, float px, unsigned int rgba, float tracking, const char* s);
// tracking is extra advance in ems; returns the final pen x so you can right-align.

// Drop shadow: two calls.
DrawTextGL(x + 1.0f, y + 1.0f, px, 0x8C000000u, tr, s);   // 55 % black
DrawTextGL(x,        y,        px, UI_TEXT,     tr, s);
```

Measure first, then place — never `W / 2 - 170`:

```cpp
float MeasureTextGL(float px, float tracking, const char* s);
const float w = MeasureTextGL(30.0f, 0.0f, msg);
DrawTextGL((W - w) * 0.5f, y, 30.0f, UI_TEXT, 0.0f, msg);
```

### 6.3 What makes a HUD look designed

Ten items, roughly in order of signal per line.

**1. A grid and a margin.** Define one base unit and make every position and size a multiple of it:

```cpp
const float u = std::max(6.0f, std::floor(static_cast<float>(H) / 100.0f));  // 10.8 px at 1080p
const float safe = 3.0f * u;
```

Nothing communicates "hand-placed by a programmer" faster than `(8, 8)` and `(8, H - 24)`.

**2. Plates with a real edge.** Any HUD element floating directly on the scene reads as debug.
Give it a plate: chamfered corners (four extra vertices, no texture), fill `UI_PLATE` at α 0.72,
a 1 px `UI_BEVEL_HI` inner line on top and left, a 1 px `UI_BEVEL_LO` on bottom and right, and a
**2 px accent strip on the left edge** in the element's semantic colour. About 40 lines for a
reusable `Plate(x, y, w, h, accent)` and it is the largest single "designed" signal in the list.

**3. Typographic hierarchy — exactly three sizes, two weights, three colours.**

| Role | Size at 1080p | Colour | Notes |
| --- | --- | --- | --- |
| Counter value | 30 px | `UI_TEXT` | tabular numerals |
| Label | 12 px | `UI_TEXT_2` | **UPPERCASE, tracking +0.08 em** |
| Body / banner | 17 px | `UI_TEXT` | |

Letter-spacing on small uppercase labels is one of the strongest "a designer touched this"
signals available and it is one extra float in the draw loop. Use a monospaced-digit face for
counters so the number does not jitter as digits change — Consolas is fine for the numerals; pair
it with a proportional face (Segoe UI) for labels, which costs one extra atlas.

**4. Icons, drawn as polygons.** Never use a word where a 16×16 shape will do. All of these are
6–12 vertices in the same immediate-mode path:

- Crystal: a 6-vertex elongated hexagon with a lighter top facet (2 triangles), in `CRYSTAL_INNER`.
- Ore: a rounded lozenge with a darker lower half, in `ORE_GLINT`.
- Crew: helmet dome (a 7-vertex arc) plus a brim rectangle, in `MINER_HELMET`.
- Threat: a triangle with a notch cut from the base, in `UI_DANGER`.

**5. Animated numerals.** A counter that snaps is a debug print. On increment: scale the glyph
1.0 → 1.35 → 1.0 over 0.25 s with an ease-out-back; flash the icon to `CRYSTAL_CORE`; and if the
delta is > 1, **count the displayed value up** over 0.3 s rather than jumping. Plus a "+1" that
rises 22 px and fades over 0.7 s.

**6. Replace formatted floats with state.** `" next %.0fs"` (`:702`) is telemetry. Draw a
progress **arc** instead: a 24-vertex `GL_TRIANGLE_STRIP` between an inner and outer radius,
swept by the fraction, in `UI_WARN` shading to `UI_DANGER` in the last 20 %. An arc reads as
designed; a float reads as a printf.

**7. Vignette-integrated framing.** The HUD is drawn *after* §4.8's vignette and grade, so it is
crisp while the scene edges are darkened — that contrast is a large part of why a real HUD sits
"on top of" rather than "inside" the image.

**8. Banner treatment.** `Say()` currently sets a string and a timer (`:111`), drawn at `(8, 30)`.
Replace with a centre-top plate that slides down 18 px over 0.18 s (ease-out cubic), holds, then
fades and slides back up. Above the message, a small letter-spaced kicker: `OBJECTIVE`,
`ALERT`, `STATUS`, coloured by severity. Same string, entirely different read.

**9. Our own cursor.** `::SetCursor(nullptr)` on `WM_SETCURSOR` and draw a procedural reticle in
the ortho pass, changing shape by context: a thin arrow over UI, four inward-pointing drill
chevrons over a drillable wall, a ring over walkable floor, a bracketed crosshair over a monster.
An OS arrow over a game world is one of the loudest prototype signals there is, and this is
perhaps 60 lines.

**10. Selection and order feedback.** The current 24-segment `GL_LINE_LOOP` at radius 0.55 and
`y = 0.03` (`:380-386`) will z-fight with the floor at distance. Replace with:

- A ring built as a `GL_TRIANGLE_STRIP` between r = 0.50 and r = 0.62, alpha fading to 0 at both
  edges, drawn with `glPolygonOffset(-1, -1)` rather than a y-bias.
- Four corner ticks rotating at 0.35 rad/s — rotation is what makes a selection ring read as
  *active*.
- A one-shot ground decal at the ordered destination: a ring that expands from r = 0.2 to 0.8 and
  fades over 0.45 s.

**11. The HUD pass itself.**

```cpp
glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
glOrtho(0.0, static_cast<double>(W), static_cast<double>(H), 0.0, -1.0, 1.0);
glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
glDisable(GL_DEPTH_TEST); glDisable(GL_FOG); glDisable(GL_CULL_FACE);
glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// ... plates, icons, text, cursor ...
glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
```

Note `glOrtho` with `bottom > top` puts the origin at the top-left, matching the mouse coordinate
space — which removes an entire class of flip bugs.

---

## 7. The ranked list

### 7.1 Everything, ranked by perceived gain ÷ implementation cost

Gain is a 1–10 subjective estimate of how much of the perceived prototype/finished gap the item
closes. Cost is engineering hours for one competent person including tuning. **Ratio is what you
should sort by.** Dependencies are strict.

| # | Item | § | Gain | Cost (h) | Ratio | Depends on |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Blob shadows + footstep puffs (contact) | 4.6–4.7 | 7 | 2 | **3.5** | — |
| 2 | Hemisphere ambient + linear-space vertex lighting | 2.4–2.6 | 10 | 6 | **1.7** | — |
| 3 | Vignette + two-point grade + background gradient | 4.8, 2.7 | 6 | 3.5 | **1.7** | — |
| 4 | Helmet-lamp spot + crystal fill lights | 2.4 | 9 | 3 | **3.0** | 2 |
| 5 | Voxel vertex AO on terrain | 3.6 | 9 | 4 | **2.3** | 2 |
| 6 | QPC timebase + vsync (prerequisite) | 4.0 | 3 | 0.5 | **6.0** | — |
| 7 | Trauma screen shake + hit-stop | 4.5–4.6 | 7 | 2 | **3.5** | 6 |
| 8 | Particle pool + atlas + 2-call draw | 4.1–4.3 | — | 5 | — | 6 (enabler) |
| 9 | Break debris + drill sparks + emerge burst | 4.4 | 9 | 3 | **3.0** | 8 |
| 10 | Camera: damp, pitch band, FOV, lookahead, framing | 5.2–5.5 | 8 | 5 | **1.6** | 6 |
| 11 | Camera occlusion pull-in + near-wall fade | 5.6 | 5 | 1.5 | **3.3** | 10 |
| 12 | GL font atlas from GDI | 6.2 | 5 | 4 | **1.3** | — |
| 13 | HUD plates + icons + hierarchy + animated counters | 6.3 | 8 | 5 | **1.6** | 12 |
| 14 | Custom cursor + selection ring rebuild | 6.3 | 4 | 2 | **2.0** | 12 |
| 15 | Miner/monster reproportion + scale classes | 3.2, 3.4 | 6 | 2.5 | **2.4** | — |
| 16 | Rim light on actors | 2.4 L3 | 5 | 1 | **5.0** | 2 |
| 17 | Vertex-array terrain path (`GL_T2F_C4UB_V3F`) | 2.8 | 2 | 3 | 0.7 | 2 (enabler) |
| 18 | Procedural rock + detail textures | 3.7 | 7 | 5 | **1.4** | 17 |
| 19 | Ambient cave motes | 4.4 #9 | 4 | 0.5 | **8.0** | 8 |
| 20 | Crystal sparkle (rare-flash version) | 4.4 #4 | 4 | 1 | **4.0** | 8 |
| 21 | Damage flash + squash | 4.4 #5 | 5 | 1 | **5.0** | — |
| 22 | Outline shell on actors | 3.5 | 4 | 2 | **2.0** | 15 |
| 23 | Silhouette/squint test modes | 3.3 | 2 (tooling) | 1 | — | — |
| 24 | Emissive bloom (copy-tex, blur, add) | 4.9 | 5 | 6 | 0.8 | 3 |
| 25 | Palette L\* self-check | 2.9 | 1 (tooling) | 0.5 | — | 2 |
| 26 | Shader path via `wglGetProcAddress` (per-pixel lamp) | 8 | 6 | 12 | 0.5 | everything |

Items 19, 6, 16, 21, 20 have absurd ratios and take a combined **4 hours**. Do them in the first
afternoon regardless of what else you plan.

### 7.2 If you can only do six things

These are chosen for gain *and* for the fact that each one lifts the whole frame rather than one
object, and they are ordered so that each is buildable when you reach it.

**1. The timebase and the light rig.** `QueryPerformanceCounter` + vsync, then replace the six
per-face constants in `Cube()` with per-vertex colours computed in **linear space** from a
hemisphere ambient (`AMBIENT_SKY #2A3E5C` / `AMBIENT_GROUND #1A1512`), the **helmet-lamp spot**
(`#FFC77A`, intensity 2.2, R 7.5, cone 22°/38°, windowed inverse square), and a camera-relative
**rim light** on actors. Tessellate floors 3×3 and walls 2×2 so the falloff is smooth. *This is
the single largest change available and everything else is amplified by it.* (§2, §4.0)

**2. Voxel vertex AO on the terrain,** using `VertexAO(side1, side2, corner)` and the
diagonal-flip rule, computed at the existing `RecomputeWalls()` point. It is what makes voxel
terrain read as carved rather than stacked, and it costs mesh-build time only. (§3.6)

**3. Contact and frame.** Blob shadows with `GL_ZERO, GL_ONE_MINUS_SRC_ALPHA` (multiply-darken,
*not* alpha-over), footstep puffs, ambient cave motes, `GL_EXP2` fog into a lifted ambient colour,
a background gradient quad, a vignette, and a two-quad multiply/add grade. Objects stop floating
and the image gets an edge. Cheapest big win in the document. (§4.6–4.8, §2.7)

**4. The particle system plus break debris, drill sparks, emerge burst, trauma shake and
hit-stop.** The pooled system is the infrastructure; the four emitters plus the two camera
responses are what make the world react to itself. Sparks must be drawn as velocity-aligned
streaks and the shake must be rotation-only with coherent noise. (§4)

**5. The camera.** Exponential damping with per-channel λ, the 30°–58° pitch band coupled to
zoom, FOV coupled to zoom, velocity lookahead, combat framing, order acknowledgement, idle drift,
and occlusion pull-in. A camera with an opinion is the difference between a game and a viewer.
(§5)

**6. The HUD.** GDI-baked font atlas drawn as GL quads (which also fixes a construction that is
not sound today), an 8-unit grid, chamfered plates with bevels and accent strips, procedural
icons in world-matched colours, three sizes / two weights / three colours, letter-spaced small-caps
labels, animated counters, a progress arc instead of `%.0fs`, and our own cursor. (§6)

### 7.3 What is deliberately not in the six, and why

- **Procedural textures (§3.7).** Genuinely large, but items 1 and 2 already deliver most of the
  "surfaces have variation" win, and untextured-but-well-lit reads far better than
  textured-but-flatly-lit. Do it seventh.
- **Shadow maps.** Needs render-to-texture and a depth-comparison path we do not have in 1.x
  without extensions. Blob shadows get ~70 % of the perceived benefit for ~5 % of the cost.
- **Shaders.** A real option (§8) but not until the six are done — and if the six are done well,
  you may find you do not want them.
- **SSAO.** Requires a depth prepass and a full-resolution kernel. Vertex AO (§3.6) is exact for
  voxels, free at runtime, and better-looking here.
- **More geometry per figure.** Adding cubes to the miner does not help until §3.2's proportions
  are fixed. More detail on a bad silhouette is noise
  ([80.lv](https://80.lv/articles/character-design-shape-language-and-readability)).

---

## 8. The dependency question, answered explicitly

The brief asks for an explicit argument if a library is worth it. **My recommendation is zero
third-party libraries.** The three candidates and why each is declined:

| Candidate | Licence | Size | Verdict |
| --- | --- | --- | --- |
| `stb_truetype.h` | Public domain / MIT | ~40 KB source, header-only, no binary dep | **Declined.** GDI already rasterises hinted, kerned glyphs from any installed face (§6.2). We would be adding a rasteriser to replace one we already link. |
| GLEW / GLAD | MIT / public domain | 1.2 MB source, ~200 KB object | **Declined.** We need at most five entry points; `wglGetProcAddress` plus our own typedefs is ~30 lines (§4.0 shows the pattern). |
| `stb_image.h` | Public domain / MIT | ~7 KB | **Declined outright.** We have no image files by construction — every texture in this document is generated in code. |

**Loading modern GL without a loader,** should you ever want it, is genuinely small. Note the
documented split: `wglGetProcAddress` returns nothing for functions exported directly by
`opengl32.dll` (the GL 1.1 set); for those you use `GetProcAddress` on the module. And a valid
context must already be current
([Khronos wiki, *Load OpenGL Functions*](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions);
[apoorvaj.io, *Loading OpenGL without GLEW*](https://apoorvaj.io/loading-opengl-without-glew)).

```cpp
static void* GlGet(const char* name)
{
    void* p = reinterpret_cast<void*>(::wglGetProcAddress(name));
    const intptr_t v = reinterpret_cast<intptr_t>(p);
    if (v == 0 || v == 1 || v == 2 || v == 3 || v == -1) {
        static HMODULE m = ::LoadLibraryA("opengl32.dll");
        p = reinterpret_cast<void*>(::GetProcAddress(m, name));
    }
    return p;
}
```

If you do go there, the one shader worth writing is a per-pixel version of the §2.4 rig, since
per-vertex lighting is the only real quality ceiling in the plan. GLSL 1.20 (the version
guaranteed alongside GL 2.0), no extensions:

```glsl
// deepcore.frag -- GLSL 1.20. Per-pixel helmet lamp + hemisphere ambient, linear-space.
#version 120
varying vec3 vWorldPos;
varying vec3 vNormal;
varying vec4 vBakedAO;        // r = vertex AO, gba = baked static ambient (linear)

uniform vec3  uLampPos;
uniform vec3  uLampDir;
uniform vec3  uLampColour;    // linear
uniform float uLampIntensity;
uniform float uLampRadius;
uniform vec2  uCone;          // (cos(outer), cos(inner))
uniform vec3  uSky, uGround;  // linear hemisphere colours

void main()
{
    vec3  n  = normalize(vNormal);
    vec3  dv = uLampPos - vWorldPos;
    float d2 = dot(dv, dv);
    vec3  L  = dv * inversesqrt(max(d2, 1e-6));

    float r2  = uLampRadius * uLampRadius;
    float win = clamp(1.0 - (d2 * d2) / (r2 * r2), 0.0, 1.0);
    float att = (win * win) / (d2 + 1.0);
    float cone = smoothstep(uCone.x, uCone.y, dot(-L, uLampDir));
    float lam  = max(dot(n, L), 0.0);

    vec3 ambient = mix(uGround, uSky, 0.5 + 0.5 * n.y) * vBakedAO.r;
    vec3 lit     = ambient + uLampColour * uLampIntensity * att * cone * lam;

    // Encode to sRGB on the way out. All arithmetic above was linear.
    gl_FragColor = vec4(pow(max(lit, vec3(0.0)), vec3(1.0 / 2.2)), 1.0);
}
```

Cost estimate 12 h including the loader, the vertex shader, the uniform plumbing and a
fixed-function fallback path (which you must keep, because a shader failure on an old driver
must not be fatal). Ratio 0.5 — last on the list, and only worth it after §7.2.

---

## 9. Build-constraint notes (`/W4`, v142, x86)

The techniques above touch the exact places where `/W4` bites. Concretely, in this file's style:

| Warning | Where it will fire | Fix |
| --- | --- | --- |
| `C4244` conversion, possible loss of data | `gluUnProject` and `glGetDoublev` are `double`; `GetTextExtentPoint32A` gives `LONG`; `abc.abcB` is `UINT` | Explicit `static_cast<float>` / `static_cast<int>` at every boundary. The existing file already does this at `:524-525`; keep the habit. |
| `C4245` / `C4018` signed-unsigned | `abc.abcA + abc.abcB + abc.abcC` mixes `int` and `UINT`; loops over `.size()` | Cast `abcB` to `int` (as the §6.2 sketch does); index with `std::size_t` as `:269` already does. |
| `C4100` unreferenced formal parameter | `DrawMiner(const Miner&, float t)` already carries a `(void)t;` at `:410` | Keep using `(void)param;` or drop the name. |
| `C4189` local initialised but not referenced | Debug-only lighting intermediates | Guard with `#ifdef _DEBUG` or `(void)x;`. |
| `C4305` truncation from `double` to `float` | Literals like `0.5` in the new maths | Suffix every float literal with `f`. The existing file is disciplined about this; stay disciplined. |
| `C4127` conditional expression is constant | `if (kMaxParticles > 0)`-style guards | Use `if constexpr` (the project is `stdcpp17`, see `deepcore3d.vcxproj`) or restructure. |

x86 note: `std::pow` in the sRGB encode is called once per vertex colour in the naive version.
On x86 that is a real cost at 60 000 vertices. **Build a 1024-entry linear→sRGB lookup table at
startup** and index it; the encode becomes a table read and the error is invisible at 8 bits.

---

## 10. Sources

- Manic Miners — official feature list: <https://manicminers.baraklava.com/features/>
- Manic Miners — itch.io page: <https://baraklava.itch.io/manic-miners>
- Manic Miners — community writeup (helmet-lamp / subdued lighting mode): <https://tvtropes.org/pmwiki/pmwiki.php/VideoGame/ManicMiners>
- Mikola Lysenko, *Ambient occlusion for Minecraft-like worlds*, 0 FPS: <https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/>
- Squirrel Eiserloh, *Math for Game Programmers: Juicing Your Cameras With Math*, GDC 2016: <https://gdcvault.com/play/1023146/Math-for-Game-Programmers-Juicing> — slides: <http://www.mathforgameprogrammers.com/gdc2016/GDC2016_Eiserloh_Squirrel_JuicingYourCameras.pdf>
- Rory Driscoll, *Frame Rate Independent Damping Using Lerp*: <https://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/>
- Jan Willem Nijman (Vlambeer), *The art of screenshake*, INDIGO Classes 2013: <https://www.youtube.com/watch?v=AJdEqssNZ-U>
- Martin Jonasson & Petri Purho, *Juice it or lose it*, Nordic Game Jam 2012 — write-up: <https://gamejuice.co.uk/resources/juice-it-or-lose-it>
- 80.lv, *Character Design: Shape Language and Readability*: <https://80.lv/articles/character-design-shape-language-and-readability>
- Riot Games, *Character Art* (Art Education): <https://www.riotgames.com/en/artedu/character-art>
- Pav Creations, *Color Theory for Game Art Design — The Basics*: <https://pavcreations.com/color-theory-for-game-art-design-the-basics/>
- Nasty Rodent, *Color Theory for Game Art: The Production Application Guide*: <https://nastyrodent.com/color-theory-for-game-art/>
- Alin Loghin, *Tileable Procedural Textures*: <http://alinloghin.com/articles/tileable_textures/>
- Brian Karis et al., *Physically Based Shading in Theory and Practice*, SIGGRAPH 2013 course (windowed inverse-square falloff): <https://blog.selfshadow.com/publications/s2013-shading-course/>
- Khronos OpenGL Wiki, *Load OpenGL Functions*: <https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions>
- Apoorva Joshi, *Loading OpenGL without GLEW*: <https://apoorvaj.io/loading-opengl-without-glew>
- GameDev.net, *2D rendering with Windows GDI on top of 3D OpenGL* (GDI/back-buffer failure modes): <https://www.gamedev.net/forums/topic/558160-2d-rendering-with-windows-gdi-on-top-of-3d-opengl/>
