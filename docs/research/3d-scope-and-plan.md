<!-- Research document. Authored by reading this repository's source and by web research with
     cited URLs. NO BUILD WAS RUN for this document; the lead engineer was compiling
     concurrently and this document touches no file but itself. No game was run. Every claim
     about our own code cites a file and line at HEAD c33e4a5 and is re-derivable by reading
     it. Every line-of-code figure is an ESTIMATE and is labelled as one. Every judgement
     about "perceived gap" is a judgement, is labelled as one, and is never dressed up as a
     measurement. -->

# 3D: the real gap, the real ceiling, and the order to do the work in

Repo: `C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`
Branch `main`, HEAD `c33e4a5` ("DeepCore 3D: real 3D, zero dependencies, driven by the
project's own systems") at time of writing.
Subject: `src/game3d/DeepCore3D.cpp` (725 lines), `src/game3d/deepcore3d.vcxproj`.

This is the adversarial planning document of a set of four. The other three
(`docs/research/3d-*.md`) argue *how* to do rendering, procedural content and audio/juice.
This one argues *whether*, *in what order*, and *what to refuse* — and it disagrees with
parts of the framing it was given.

---

## Headline, stated up front so nobody has to read to the end

1. **The zero-dependency constraint is not the binding constraint on visual quality.** The
   real ceiling is not "OpenGL 1.x". Windows' `opengl32.dll` exports GL 1.1 statically, but
   **every GL feature the driver supports is reachable at runtime through
   `wglGetProcAddress`** — VBOs, GLSL, FBOs, MSAA, the lot — with roughly 60 lines of
   hand-declared function pointers and *no library at all*
   ([Khronos wiki](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions)). We have been
   describing a self-imposed limit as if it were an external one. §2.1.

2. **The binding constraint is that we have no content pipeline.** `.kkrieger` fit an entire
   FPS in 96 KB with every asset procedural — and it did that with **werkkzeug3, a node-based
   authoring tool with a live viewport**, plus a demo group's artists driving it
   ([Wikipedia](https://en.wikipedia.org/wiki/.kkrieger),
   [werkkzeug3 source](https://github.com/jaromil/kkrieger-werkkzeug3)). "Procedural" did not
   mean "nobody authored it". It meant "the authoring was recorded as a program". We have no
   viewport, no tool, and per the brief we cannot run the build to look at what we made. That
   is the thing that actually caps us. §2.2.

3. **The gap decomposes unevenly, and that is the good news.** Manic Miners' visual quality
   comes from three sources: Unreal Engine 4's renderer, the LDraw parts library (a
   community CAD library of LEGO® part geometry, CCAL 2.0 —
   [LDraw legal info](https://www.ldraw.org/legal-info)), and four years of solo authoring.
   We can buy back a large fraction of source #1 in code. We can buy back **none** of source
   #2, both because it is licensed content we will not ship and because the shapes it encodes
   are the exact shapes we must not evoke. Source #3 is time we do not have. §1.4.

4. **My honest estimate of what is closable.** Of the gap a viewer perceives from a
   *screenshot*, ~60–70% is closable in the plan below. Of the gap perceived across a
   *ten-minute play session*, ~25–35%. Of the *total product* gap — campaign, editor,
   scripting, mods, animation library — under 15%, and no plan changes that. These are
   judgements, defended in §2.4, not measurements.

5. **Three changes carry most of the closable gap, and they cost about two and a half days
   between them**: (a) a face-culled, batched mesh with Minecraft-style per-vertex ambient
   occlusion; (b) an honest frame — MSAA, vsync, fixed timestep, and a HUD that is not
   currently guaranteed to draw at all; (c) one authored palette plus a warm/cool light
   split. §4.

6. **There is a real bug in the shipped 3D code, and it is a visual one.** The pixel format
   at `DeepCore3D.cpp:632` requests `PFD_DOUBLEBUFFER` without `PFD_SUPPORT_GDI`, and then
   `DrawText2D` (`:603`) calls `TextOutA` on that same DC before `SwapBuffers` (`:716`).
   Those two flags are mutually exclusive; GDI text over a double-buffered GL back buffer is
   not supported and behaves differently per driver and per Windows version
   ([Khronos forums](https://community.khronos.org/t/pfd-doublebuffer/51171),
   [Intel community, Win10 regression](https://community.intel.com/t5/Intel-Fortran-Compiler/Windows10-Fortran-OpenGL-GDI/m-p/1091690)).
   The entire HUD — crystal count, wave timer, win/lose banner — is on undefined behaviour.
   Fixing it is fifteen lines and it is Stage 0, item 1. §3, Stage 0.

7. **The dependency question has a defensible answer and it is "no runtime library, yes to
   one header."** raylib and SDL2 are both zlib-licensed, both ship x86 MSVC builds
   compatible with v142, and both solve problems we do not have while costing ~1 MB and the
   project's central claim. The only dependency worth taking is a **declaration-only** header
   with zero binary footprint, and even that is optional. §6.

Everything below shows the work.

---

## 0. Ground rules honoured by this document

- **Measured** means I read it in this repo and cite file:line. **Estimated** means I made it
  up from experience and am telling you so. **Judgement** means it is an opinion about human
  perception and cannot be measured from a chair.
- I did not run a build, run the game, or touch any file but this one.
- Line-of-code figures are for *new, net* code in `src/game3d/`, excluding comments, and
  should be read as ±50%.
- The "L" in "LRR" is not expanded anywhere in this document. Where the 1999 title is meant,
  I write "the 1999 game".
- Nothing in the plan below proposes shipping, copying, or deriving from any asset of any
  commercial game. Every technique named is generative code. Where a Windows *system* font is
  used at runtime (§3 Stage 0) that is a call into the OS, not a redistributed asset, and I
  say so where it comes up because it is exactly the sort of thing that deserves flagging
  rather than assuming.

---

## 1. Estimate the real gap

### 1.1 What we actually have — measured from the source

Read from `src/game3d/DeepCore3D.cpp` at HEAD `c33e4a5`:

| Property | Value | Where |
| --- | --- | --- |
| Total source | 725 lines, one translation unit | file |
| Renderer | OpenGL 1.1 fixed function, `glBegin`/`glEnd` per cube | `:363-368` |
| Lighting | none. Six hardcoded per-face brightness constants `{1.00, 0.35, 0.72, 0.60, 0.82, 0.50}` | `:355-362` |
| Normals | computed and stored in `struct F::n[3]` — **and never used**. Dead data. | `:354-362` |
| Shadows | none | — |
| Ambient occlusion | none | — |
| Textures | none. Zero texture objects created anywhere. | — |
| Anti-aliasing | none. No multisample request in the PFD. | `:630-636` |
| Vsync | none. `::Sleep(8)` as the frame limiter. | `:717` |
| Timestep | variable, clamped to 100 ms | `:667-668` |
| Fog | `GL_LINEAR`, 26 → 62 units, one colour | `:644-649` |
| Terrain | 40×40 = 1600 tiles, one or two `Cube()` calls each | `:165, :441-476` |
| Faces drawn per cube | **6, always** — including the five buried inside neighbours | `:363` |
| Miner model | 8 cubes, hardcoded literals | `:389-397` |
| Monster model | 7 cubes, hardcoded literals | `:424-433` |
| Animation | `sin(phase)` on bob, leg swing, arm swing, body roll | `:373, :395, :422, :431` |
| Particles | none | — |
| Audio | `::Beep(1200, 60)` / `::Beep(70, 90)` / `::Beep(200, 130)` | `:251, :310, :334` |
| HUD | GDI `TextOutA` over a double-buffered GL surface — see Headline #6 | `:603-608, :707-714` |
| Picking | `glReadPixels` depth + `gluUnProject` — a GPU→CPU stall every click | `:510-527` |
| Camera | orbit, yaw/pitch/dist, pitch clamped 0.25–1.45 rad | `:105, :586-588` |

Two numbers matter more than the rest.

**Number one: 83% of the triangles drawn are invisible.** Every `Cube()` emits all six faces
unconditionally (`:363-368`). In a 40×40 cavern that is mostly solid rock, an interior tile's
six faces are all buried; only tiles adjacent to open floor show anything, and those show
one to three faces. A face-culled mesh emits roughly one sixth of the geometry. This is not
primarily a performance argument — see the next number — it is a *correctness* argument,
because per-vertex lighting and ambient occlusion computed on buried faces produce garbage
that bleeds through at the seams.

**Number two: immediate mode is not currently the bottleneck, and it is dishonest to say it
is.** Estimated per-frame cost today: ~1,930 cubes × 24 vertices ≈ **46,000 `glVertex3fv`
calls and ~1,930 `glBegin`/`glEnd` pairs**. A modern driver eats that and hits vsync. Where
it dies is scale: Manic Miners supports 256×256 maps
([features page](https://manicminers.baraklava.com/features/)). At 256×256 that is 65,536
tiles → ~1.57 M vertices and 65,536 batches per frame; at 60 Hz, **94 M `glVertex3fv` calls
per second**, which is one to two orders of magnitude past what immediate mode delivers. So:
batching is not a Stage 1 performance need. It is a Stage 1 *enabler*, because you cannot put
per-vertex AO into an immediate-mode path without recomputing it every frame, and it is the
thing that later unlocks map size for free.

### 1.2 What Manic Miners has — sourced, not guessed

From the project's own features page
([manicminers.baraklava.com/features](https://manicminers.baraklava.com/features/)) and the
modding wiki ([manicminers.fandom.com/wiki/Modding](https://manicminers.fandom.com/wiki/Modding)):

- Unreal Engine 4 rendering with "advanced lighting"; upscaled wall textures and
  high-resolution brick textures
- "Pristine LDraw models with maximum detail"
- Fully customisable miners "down to the individual print and colour"
- First-person Eye/Shoulder view
- A level editor with real-time changes; save/load of buildings, vehicles, miners
- Maps to 256×256; height map from −100 to +100 × tile size
- Advanced level scripting, plus visual scripting with Script Blocks
- Landslides, lava erosion, spiders, hazard scaling
- Smart task AI, vehicles that take drill orders and shoot autonomously
- "Many new animations, details and secrets"
- A classic UI with modern visuals; fully customisable hotkeys
- Modding (custom buildings and vehicles) is **planned, not shipped** — "some time after
  V0.3" per the wiki

That last point is worth holding onto. The brief listed "mod support" as something Manic
Miners has and we do not. Its own wiki says it is a planned feature. One item of the gap is
not a gap.

### 1.3 The gap, grouped and unsentimental

**(a) Cheap to approximate procedurally — days, not weeks, and it shows immediately**

| Gap item | Our procedural substitute | Est. LOC | Perceived gain |
| --- | --- | --- | --- |
| Baked/dynamic global illumination | Per-vertex AO, 4-corner rule, computed at mesh build | 120 | **very high** |
| "Advanced lighting" | One directional key + one warm point light per miner lamp, Lambert + wrapped diffuse, per-vertex | 200 | **very high** |
| Anti-aliased edges | `WGL_ARB_multisample`, 4× or 8× | 90 | **very high** |
| Wall/brick textures | Triplanar procedural noise in a fragment shader — no UVs, no atlas, no seams | 250 | high |
| Rock surface variation | Per-tile hash → hue/value jitter within a fixed palette | 60 | high |
| Smooth frame pacing | Fixed 60 Hz timestep + interpolated render + `WGL_EXT_swap_control` | 120 | high |
| HUD/UI polish | `wglUseFontBitmaps` display lists + GL quads for panels | 300 | high |
| Landslides | Particle burst + tile-state transition + screenshake | 180 | medium-high |
| Erosion / lava creep | Cellular automaton on the existing `Block::flags` | 150 | medium |
| Camera feel (Eye/Shoulder) | Second camera mode reusing the same `gluLookAt` | 80 | medium |
| Dust, sparks, drill debris | One pooled point-sprite system | 220 | medium-high |
| Water | Animated vertex offset + fresnel-ish colour ramp on the existing water tiles | 130 | medium |
| Hazard scaling | Already ours — `WaveTuning` is exposed at `:188-191` | 20 | low visual, high feel |
| Silhouette clarity | Depth-based outline pass, or back-face-expanded shell | 140 | medium-high |

**(b) Expensive but possible — a week-plus each, and we might do one or two**

| Gap item | What it would take | Est. LOC | Honest risk |
| --- | --- | --- | --- |
| Dynamic shadows | Shadow map, one 1024² depth FBO, PCF 3×3, cascade-free (small world) | 450 | Peter-panning and acne tuned blind, with no GPU debugger |
| Height-mapped terrain (MM has −100..+100) | Break the single-height voxel assumption in `Block`, remesh | 600 | Touches `SyntheticLevel`, which the sandbox and the logic tests depend on |
| Bloom / tonemap | FBO ping-pong, separable Gaussian, ACES-ish curve | 350 | On a flat-shaded scene, bloom without a real HDR range just makes it foggy |
| Screen-space AO | 16-sample hemisphere in view space | 300 | Strictly worse than per-vertex AO *for voxels*, at 3× the cost. See §5. |
| Skeletal animation rig | Joint hierarchy, blend, procedural clips | 700 | Authoring clips blind — see §5, this is the trap |
| 256×256 maps | Chunked meshing, frustum cull per chunk | 400 | Free-ish *after* Stage 1; pointless before it |
| Building construction | Model set + placement UI + resource sink | 900 | This is game design, not rendering |
| Vehicles | Model set + pathing + the autonomous-drill behaviour | 1100 | Same |
| Level editor | Immediate-mode GUI from scratch | 3000+ | See §5 |
| Scripting system | Tokeniser, AST, interpreter, event bindings | 1500 | We already have `docs/NERPS-LANGUAGE.md`; not a *visual* gain |

**(c) Needs an artist, or an engine we do not have — do not plan for these**

- **Character models with the density of LDraw parts.** A minifigure in LDraw is hundreds of
  triangles of chamfers, studs, clutch geometry and printed faces. Ours is 8 boxes. Closing
  that means either a parts library (licensed content, and the wrong shapes for us to be
  evoking anyway) or a modeller sitting in a viewport. Neither exists here.
- **Hand-authored animation.** Weight, anticipation, follow-through, secondary motion. This
  is not an algorithm; it is a craft with a scrub bar. Procedural motion gets the read but not
  the charm.
- **Painted textures and decals** — prints, wear, logos, signage. Procedural noise gives
  *material*; it does not give *authorship*.
- **UI art.** Icons, frames, cursors, the "classic UI with modern visuals". We can draw clean
  geometric UI; we cannot draw *illustrated* UI.
- **Music.** Generative music is real and we could do it, but generative music that a person
  wants to hear for an hour is a research project, not a task.
- **A campaign.** Level design is authorship. A generator gives you caverns; it does not give
  you a designed difficulty curve or a mission with a story beat.
- **Unreal's renderer as a whole.** Not any single feature — the *integration*: temporal AA,
  a real material system, GPU particles, Lumen/lightmaps, a post stack that has been tuned
  against thousands of scenes. Individual features we can approximate. The compounding we
  cannot.

### 1.4 The uncomfortable decomposition — and why it is good news

Attribute Manic Miners' *visual* quality to three sources:

| Source | Roughly | Can we buy it back in code? |
| --- | --- | --- |
| UE4's renderer and post stack | ~40% | **Partly — this is the plan.** Maybe 60% of it, in shaders we write. |
| LDraw geometry (CCAL 2.0 parts library) | ~35% | **No.** Licensed third-party content, and the shapes are precisely the ones we must not evoke. Structural, permanent. |
| Four years of solo authoring | ~25% | **No.** That is time. |

Those percentages are judgement. The structure is not. And the structure says something
useful: **the single largest slice is the one made of code**, and it is the only slice a
zero-dependency C++ project can attack at all. That is why the plan below is almost entirely
renderer work and almost not at all content work — not because content does not matter, but
because content is the slice where our marginal hour buys the least.

It also says the honest framing of this project is not "a Manic Miners competitor". It is
"a readable, hackable systems toy that looks *deliberate*". `DeepCore3D.cpp:11-20` already
says exactly this, and it is the right message. The work below is about making the visuals
match the confidence of that comment.

---

## 2. The realistic ceiling

### 2.1 The constraint we thought we had, and the one we actually have

The brief says "OpenGL 1.x immediate mode" and "zero dependencies beyond the Windows SDK",
and treats those as one constraint. They are not.

Windows' `opengl32.dll` statically exports GL 1.0 and 1.1 and nothing more. But every
function above 1.1 is retrieved at runtime through `wglGetProcAddress`, which is *also* in
`opengl32.lib`. An extension loader library (GLEW, glad) is a **convenience**, not a
requirement ([Khronos: Load OpenGL Functions](https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions),
[dwmkerr on wglGetProcAddress](https://dwmkerr.com/importing-opengl-extensions-functions-with-wglgetprocaddress/)).

So the actual reachable feature set, with zero libraries added, is:

```cpp
// ~60 lines buys the whole modern pipeline. This is the entire "dependency" story.
typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void   (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const char* const*, const GLint*);
typedef void   (APIENTRY *PFN_glGenBuffers)(GLsizei, GLuint*);
typedef void   (APIENTRY *PFN_glBufferData)(GLenum, ptrdiff_t, const void*, GLenum);
// ... ~40 more

static PFN_glCreateShader  glCreateShader;
static PFN_glGenBuffers    glGenBuffers;
// ...

static void* GLGet(const char* name)
{
    void* p = (void*)::wglGetProcAddress(name);
    // Some drivers return 1, 2, 3 or -1 for "not present". Check all of them; this is the
    // classic bug that makes a loader work on NVIDIA and crash on Intel.
    if (p == nullptr || p == (void*)0x1 || p == (void*)0x2 ||
        p == (void*)0x3 || p == (void*)-1) {
        static HMODULE gl = ::LoadLibraryA("opengl32.dll");   // GL 1.1 fallback path
        p = gl ? (void*)::GetProcAddress(gl, name) : nullptr;
    }
    return p;
}

#define LOAD(t, n) n = (t)GLGet(#n); if (!n) return false;
```

That is the whole thing. It adds nothing to the binary but the pointers, it is x86-clean, it
warns at nothing under `/W4` (the `void*`↔function-pointer cast is the one to watch — do it
through a helper that MSVC does not diagnose, or accept a documented `#pragma warning` scope),
and it works on v142.

**But there is a one-way door here, and the plan must walk through it deliberately.**

If we ever request a *core profile* context via `wglCreateContextAttribsARB`, we lose, all at
once:

- `glBegin`/`glEnd` — the entire current renderer
- `gluUnProject` — our mouse picking (`:523`)
- `gluPerspective`/`gluLookAt` — our camera (`:686`, `:692`)
- Display lists — including `wglUseFontBitmaps`, the free HUD text fix
- The fixed-function matrix stack that `PickTile` reads back at `:514-515`

**Recommendation, and it is load-bearing for every stage below: never request a core
profile.** The default context Windows gives you on any real driver is a *compatibility*
context that exposes GL 4.x alongside every deprecated 1.1 entry point. We get shaders and
VBOs *and* keep `glBegin` and GLU. This is the single most important architectural decision
in the document and it costs nothing.

**The residual risk, named:** if there is no vendor ICD — a bare RDP session, some VMs, a
freshly imaged machine before driver install — Windows falls back to a software GL **1.1**
implementation and `wglGetProcAddress` returns nothing for anything. The plan must therefore
keep a **Tier 0** path: the current immediate-mode renderer, retained, selected at startup by
capability query. That is ~40 lines of branching and it is the difference between "runs
everywhere" and "black window on the reviewer's laptop".

```cpp
enum class GLTier { Legacy11, Shader21, Modern33 };
GLTier DetectTier()
{
    const char* ver = (const char*)glGetString(GL_VERSION);   // always valid, GL 1.1+
    int major = 1, minor = 1;
    if (ver) std::sscanf(ver, "%d.%d", &major, &minor);
    if (major < 2)                      return GLTier::Legacy11;   // software fallback
    if (!LoadShaderEntryPoints())       return GLTier::Legacy11;
    if (major > 3 || (major == 3 && minor >= 3)) return GLTier::Modern33;
    return GLTier::Shader21;
}
```

### 2.2 The constraint that actually caps us: no viewport, no tool, no run

`.kkrieger` is the standard citation for "procedural art can look good", and it is the wrong
citation if you stop at the headline. It shipped in 96 KB with meshes built by deforming
boxes and cylinders, and textures stored as *creation histories* replayed at load
([Wikipedia](https://en.wikipedia.org/wiki/.kkrieger)). But those histories were authored in
**werkkzeug3**, a node-based tool with a live preview, by people who did this for sport
([source](https://github.com/jaromil/kkrieger-werkkzeug3)). The procedural-ness bought file
size. It did not buy the art. A human still sat there and turned knobs until it looked right.

We have no knobs and no preview. Under this brief we do not even run the build. That means:

- **Anything requiring visual iteration is dramatically more expensive for us than its line
  count suggests.** Shadow-map bias, bloom threshold, particle curves, animation timing —
  each is one number found by looking. Blind, each is a guess plus a round trip through
  someone else's compile.
- **Therefore prefer techniques that are correct by construction over techniques that are
  correct by tuning.** Per-vertex AO from a discrete 4-corner rule has *no tunable* except a
  4-entry brightness table. A shadow map has bias, slope-scale bias, normal offset, PCF
  radius, texel density and a near/far plane, and every one of them is found by staring.
  That single criterion reorders the entire plan, and it is why AO is Stage 1 and shadows are
  Stage 4.
- **Corollary: build a debug view before building the thing it debugs.** A key toggled
  render mode that draws AO term as greyscale, or normals as RGB, costs 20 lines and converts
  "I can't see what's wrong" into "oh, it's inverted".

### 2.3 Comparables — games that hit a high bar with primitives, and what they actually did

| Game | Art budget | What it actually did | What we can steal |
| --- | --- | --- | --- |
| **Minecraft** (smooth lighting) | textured, but blocks | Per-vertex AO from the 4-corner neighbour rule; flat-shaded quads otherwise | **The whole technique.** See §3 Stage 1 and [0 FPS](https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/) |
| **Townscaper** (Oskar Stålberg) | 1 person, no textures in the usual sense | Procedural mesh, tight palette, "every odd pixel is a line" texture trick for pixel-perfect antialiased edges, boolean geometry for windows ([breakdown](https://reindernijhoff.net/2021/11/townscapers-rendering-style-in-webgl/)) | Palette discipline; edge lines as a *first-class* visual element, not an afterthought |
| **SUPERHOT** | small team | Untextured white geometry + one saturated accent colour + clean AA + restrained bloom. The art direction *is* the constraint, stated confidently | Commit to a limited palette and let lighting do the work. Do not apologise for flat shading — frame it |
| **Teardown** | 1 primary dev | Voxels + a genuinely good lighting model (path-traced-ish GI, real shadows, volumetrics) | Proof that **voxel + strong light = premium**, not "retro". Lighting is the whole difference |
| **Islanders** | tiny team | Very low-poly, flat colour, strong palette, fog, water, and a camera that always frames well | Fog and camera framing as art direction |
| **`.kkrieger`** | demoscene group + custom tool | Everything generated; box-modelling + texture histories | The *architecture* (assets as programs). Not the workflow — we lack the tool |
| **Elevated / 4K intros** | 1–2 people | Raymarched procedural terrain, entirely in a shader | Proof that procedural detail scales without authoring — but it is a *shader* skill, and it is high-risk blind |

The pattern across all seven: **none of them won on geometry density.** Every one of them won
on **lighting, palette, silhouette and framing**. That is a very cheap list to attack in code,
and it is exactly the list our current renderer scores zero on.

### 2.4 The ceiling, named with a number

**Screenshot gap: 60–70% closable.** A still frame is judged on aliasing, contact shading,
colour coherence, silhouette and framing. Every one of those is code, none needs an artist,
and we currently have none of them. This is the optimistic number and it is honestly earned.

**Ten-minute-session gap: 25–35% closable.** A play session adds animation quality, feedback,
audio, UI responsiveness and content variety. We can move animation from "sine wave" to
"layered procedural" and feedback from "`::Beep`" to a real generated mixer — both real gains
— but we cannot move a rig with 8 boxes into the same category as an animated minifigure.

**Total product gap: under 15%.** Campaign, editor, scripting, mods, vehicles, buildings,
first-person view. Even at full velocity this is quarters of work, most of it not visual.

**What the project realistically becomes:** the visual bar of a well-directed 2015-era
stylised indie — think the Islanders/Townscaper register — running at 1280×820 with clean
edges, real contact shading, a coherent palette and a camera that frames the cavern. That is
a genuinely respectable result and a very large step from where `c33e4a5` stands. It is not
Manic Miners and pretending otherwise in the README would undo the credibility that
`DeepCore3D.cpp:11-20` currently earns.

---

## 3. The ordered plan

Sequencing rules used, in priority order:

1. **Correctness before beauty.** A HUD on undefined behaviour outranks a bloom pass.
2. **Prefer correct-by-construction over correct-by-tuning** (§2.2). We cannot iterate.
3. **Enablers before the things they enable.** Mesh batching before per-vertex lighting.
4. **Every stage ships.** After each one the build runs, `/W4` is clean, and the thing looks
   better than it did. No stage leaves the tree in a worse state than it found it.
5. **Cheapest big win first**, so that if the work stops at any point, what shipped is the
   best available prefix.

---

### Stage 0 — Make the frame honest

**~1 day. ~350 LOC. Risk: low. This is the highest ratio in the document.**

| # | What lands | Est. LOC | Why |
| --- | --- | --- | --- |
| 0.1 | **Fix the HUD.** Replace GDI `TextOutA` with `wglUseFontBitmaps` display lists. | 60 | It is currently UB. See Headline #6. |
| 0.2 | **MSAA 4×/8×** via `WGL_ARB_multisample` + `wglChoosePixelFormatARB` | 120 | Single biggest per-pixel quality delta available |
| 0.3 | **Vsync** via `WGL_EXT_swap_control`, replacing `::Sleep(8)` | 20 | Kills tearing and the jittery frame pacing |
| 0.4 | **Fixed timestep** with render interpolation | 90 | Makes motion read as smooth instead of variable |
| 0.5 | **GL tier detection** + retain the current path as Tier 0 | 40 | Insurance (§2.1) |
| 0.6 | **Debug render modes** on F1–F4 | 20 | Force multiplier for every later stage |

**The MSAA gotcha, spelled out because it bites everyone once.** `SetPixelFormat` may be
called exactly once per window, and you need a GL context to call `wglChoosePixelFormatARB`
to *find* a multisample format. So: create a throwaway window, give it any pixel format, make
a context, load `wglChoosePixelFormatARB`, destroy all of it, then create the real window.

```cpp
static int ChooseMultisampleFormat(HINSTANCE inst, int samples)
{
    // 1. Dummy window + dummy context, purely to reach the ARB entry point.
    WNDCLASSA wc{}; wc.lpfnWndProc = ::DefWindowProcA; wc.hInstance = inst;
    wc.lpszClassName = "DC3DProbe"; ::RegisterClassA(&wc);
    HWND probe = ::CreateWindowA("DC3DProbe", "", 0, 0, 0, 1, 1, nullptr, nullptr, inst, nullptr);
    HDC  pdc   = ::GetDC(probe);

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24;
    ::SetPixelFormat(pdc, ::ChoosePixelFormat(pdc, &pfd), &pfd);
    HGLRC prc = ::wglCreateContext(pdc);
    ::wglMakeCurrent(pdc, prc);

    typedef BOOL (APIENTRY *PFN_wglChoosePixelFormatARB)(
        HDC, const int*, const FLOAT*, UINT, int*, UINT*);
    PFN_wglChoosePixelFormatARB choose =
        (PFN_wglChoosePixelFormatARB)::wglGetProcAddress("wglChoosePixelFormatARB");

    int  format = 0;
    UINT found  = 0;
    if (choose) {
        const int attribs[] = {
            0x2001 /*WGL_DRAW_TO_WINDOW_ARB*/, GL_TRUE,
            0x2010 /*WGL_SUPPORT_OPENGL_ARB*/, GL_TRUE,
            0x2011 /*WGL_DOUBLE_BUFFER_ARB */, GL_TRUE,
            0x2013 /*WGL_PIXEL_TYPE_ARB    */, 0x202B /*WGL_TYPE_RGBA_ARB*/,
            0x2014 /*WGL_COLOR_BITS_ARB    */, 32,
            0x2022 /*WGL_DEPTH_BITS_ARB    */, 24,
            0x2041 /*WGL_SAMPLE_BUFFERS_ARB*/, GL_TRUE,
            0x2042 /*WGL_SAMPLES_ARB       */, samples,
            0
        };
        if (!choose(pdc, attribs, nullptr, 1, &format, &found) || found == 0) format = 0;
    }

    ::wglMakeCurrent(nullptr, nullptr);
    ::wglDeleteContext(prc);
    ::ReleaseDC(probe, pdc);
    ::DestroyWindow(probe);
    ::UnregisterClassA("DC3DProbe", inst);
    return format;   // 0 => caller falls back to ChoosePixelFormat, no MSAA
}
```

Then `glEnable(GL_MULTISAMPLE)` after context creation, and note that MSAA *also* fixes the
`GL_LINE_LOOP` selection ring at `:381-386` and the marker beam at `:484-487`, both of which
are currently crawling staircases.

**The HUD fix, which is smaller than the bug it replaces:**

```cpp
// Once, after wglMakeCurrent. Uses a Windows system font at runtime through the OS text
// API -- no font file is read, embedded, or redistributed by this program.
HFONT font = ::CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, ANSI_CHARSET,
                           OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                           FF_DONTCARE, "Consolas");
::SelectObject(dc, font);
GLuint fontBase = glGenLists(96);
::wglUseFontBitmaps(dc, 32, 96, fontBase);   // ASCII 32..127

// Per frame, in an ortho pass after the world, before SwapBuffers.
void GLText(GLuint base, float x, float y, const char* s, float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(base - 32);
    glCallLists((GLsizei)strlen(s), GL_UNSIGNED_BYTE, s);
    glPopAttrib();
}
```

This is compatibility-profile-only, which §2.1 already committed us to. It is also why an
`stb_truetype` dependency is unnecessary — see §6.

**What the user sees:** edges stop crawling. The image stops shimmering when the camera
orbits. Motion stops hitching. The HUD reliably appears — possibly for the first time on
their machine. Nothing is "prettier" and everything feels a category more solid. **This is
the stage that most changes whether a viewer classifies the program as "prototype".**

---

### Stage 1 — The mesh, and ambient occlusion

**~1.5 days. ~500 LOC. Risk: low-medium. This is the biggest single visual jump.**

| # | What lands | Est. LOC |
| --- | --- | --- |
| 1.1 | Face-culled mesh build: emit a face only if the neighbour tile is not solid | 120 |
| 1.2 | Per-vertex AO via the 4-corner rule, baked at mesh build | 120 |
| 1.3 | Quad diagonal flip to kill the AO anisotropy artifact | 25 |
| 1.4 | Batch into a VBO (Tier ≥ 2.1) or a display list (Tier 0) | 150 |
| 1.5 | Dirty-region remesh on drill (`Update` already calls `RecomputeWalls` at `:246`) | 80 |

**The AO rule, complete.** For each vertex of an emitted face, look at the three voxels
touching that vertex on the outside of the face — two edge-adjacent ("side1", "side2") and
one diagonal ("corner"):

```cpp
// Returns 0 (darkest) .. 3 (unoccluded). From the standard voxel-AO formulation;
// see 0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
inline int VertexAO(bool side1, bool side2, bool corner)
{
    if (side1 && side2) return 0;                       // fully closed inside corner
    return 3 - ((side1 ? 1 : 0) + (side2 ? 1 : 0) + (corner ? 1 : 0));
}

// Brightness lookup. These four numbers are the ONLY tunable in the entire technique,
// which is precisely why this belongs in Stage 1 and shadow maps do not (see 2.2).
static const float AO_LUT[4] = { 0.42f, 0.62f, 0.82f, 1.00f };
```

Emitting the top face of a solid tile at (x, z), with `S(x,z)` = "this tile is solid":

```cpp
// Vertex order: (x0,z0) (x1,z0) (x1,z1) (x0,z1) -- CCW seen from above.
// For a top face the occluders are the eight tiles ringing (x,z) in the same layer.
const int ao[4] = {
    VertexAO(S(x-1,z  ), S(x  ,z-1), S(x-1,z-1)),
    VertexAO(S(x+1,z  ), S(x  ,z-1), S(x+1,z-1)),
    VertexAO(S(x+1,z  ), S(x  ,z+1), S(x+1,z+1)),
    VertexAO(S(x-1,z  ), S(x  ,z+1), S(x-1,z+1)),
};

// THE FLIP. A quad is two triangles; if the diagonal runs between the two DARK corners the
// interpolation produces a visible bias that reads as a diagonal crease across every tile.
// Flipping the split when ao[0]+ao[2] > ao[1]+ao[3] removes it. This one line is the
// difference between "looks lit" and "looks like a bug".
const bool flip = (ao[0] + ao[2]) > (ao[1] + ao[3]);
EmitQuad(v, ao, flip);
```

Store AO in the vertex colour (Tier 0/2.1) or as a per-vertex attribute (Tier 3.3). At Tier
0 this still works — bake `colour * AO_LUT[ao]` into `glColor3f` inside a display list, which
costs one build and zero per-frame work.

**Why AO before lights.** AO is what makes a voxel scene look like a *place*. It darkens
every inside corner, every wall-floor junction, every crevice behind a crystal seam. It needs
no light direction, no shadow map, no bias, no FBO, and it is computed once per remesh. The
current renderer's six constant face multipliers (`:355-362`) mean **every one of the ~1,600
tiles renders in exactly the same six colours** — the scene is flat by construction, and no
amount of lighting on top of that fixes the missing contact shading.

**What the user sees:** the cavern gains depth. Corners recede. Walls meet floors with a
visible seat instead of a hard colour join. The 40×40 grid stops reading as a grid of
identical boxes and starts reading as carved rock. In my judgement this single stage closes
more perceived gap than any other in the document.

**Risk to name:** step 1.5 must invalidate the right neighbourhood. Drilling a tile changes
the AO of every vertex within one tile of it, so the dirty rect is (x±2, z±2), not (x, z).
Get that wrong and you get a permanent bright square where the wall used to be — a very
visible bug and an easy one to make.

---

### Stage 2 — Light and colour

**~1 day. ~400 LOC. Risk: low.**

| # | What lands | Est. LOC |
| --- | --- | --- |
| 2.1 | **One authored palette.** Every colour literal in the file replaced by a named entry | 90 |
| 2.2 | Per-tile hash → hue/value jitter within the palette | 50 |
| 2.3 | Directional key light, Lambert with a wrap term, per-vertex | 90 |
| 2.4 | Warm point light at each miner's lamp (`:392` already models a lamp cube) | 120 |
| 2.5 | Cool ambient / warm key split; fog colour tied to ambient | 50 |

**On the palette, because this is the cheapest thing in the document and the most
underrated.** Read `:389-397` and `:447-474`: there are ~30 independently chosen RGB triples
in the file. `0.30f, 0.42f, 0.85f` for a torso. `0.34f, 0.29f, 0.25f` for wall. `0.26f, 0.24f,
0.22f` for floor. Each is defensible alone; together they have no relationship. That
*specific* quality — colours that are individually fine and collectively unrelated — is the
single most reliable visual tell of programmer art, and it is fixed by a table:

```cpp
// One place, one decision. Cool ambient rock, warm artificial light, one saturated accent
// reserved exclusively for the crystals so the player's eye always finds the objective.
struct Palette {
    float rockDeep[3]   = { 0.11f, 0.12f, 0.15f };
    float rockFace[3]   = { 0.30f, 0.30f, 0.34f };
    float floorCool[3]  = { 0.22f, 0.23f, 0.27f };
    float oreWarm[3]    = { 0.52f, 0.36f, 0.20f };
    float crystalKey[3] = { 0.62f, 0.38f, 0.95f };   // the ONLY high-chroma value in the set
    float lampWarm[3]   = { 1.00f, 0.86f, 0.62f };
    float ambientCool[3]= { 0.16f, 0.19f, 0.26f };
    float hazard[3]     = { 0.92f, 0.26f, 0.20f };
};
```

**Per-tile variation, 8 lines, disproportionate return:**

```cpp
inline float TileJitter(int x, int z, int salt)
{
    std::uint32_t h = (std::uint32_t)(x * 73856093) ^ (std::uint32_t)(z * 19349663)
                    ^ (std::uint32_t)(salt * 83492791);
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (float)(h & 0xffff) / 65535.0f;          // deterministic, seed-stable
}
// value jitter only -- +/-6% on brightness, none on hue. Hue jitter reads as noise;
// value jitter reads as rock.
const float v = 0.94f + 0.12f * TileJitter(x, z, 0);
```

**On the lamp light.** The miners already carry a lamp cube (`:392`). Making it an actual
light source is the cheapest narrative-lighting win available: a cave lit by the crew's own
lamps is a completely different image from a cave lit by nothing. At Tier 0 this can be a
per-vertex attenuation term added at draw time for the tiles near a miner; at Tier ≥2.1 it is
three lines of fragment shader. It also does something no static light can: it makes the
scene change as the player moves, which is what makes lighting *read* as lighting.

**What the user sees:** the cavern acquires a mood. Crystals become the brightest thing in
frame and the eye goes to them without being told. Miners cast a warm pool of light that
moves with them. The image looks *chosen* rather than defaulted.

---

### Stage 3 — Silhouette, surface, and the camera

**~1.5 days. ~550 LOC. Risk: medium (first shader work).**

| # | What lands | Est. LOC |
| --- | --- | --- |
| 3.1 | Triplanar procedural rock texture (fragment shader, Tier ≥2.1) | 220 |
| 3.2 | Depth-discontinuity outline pass, or back-face shell for units | 140 |
| 3.3 | Camera: smoothed follow, framing constraints, subtle roll on shake | 110 |
| 3.4 | Better unit silhouettes — chamfer the boxes; add a drill, a pack | 80 |

**Triplanar is the right texturing answer for us and it is worth saying why.** Every UV-based
approach needs a UV layout, and a UV layout needs an art tool. Triplanar needs neither: you
sample a procedural function three times in world space and blend by the squared normal.
Axis-aligned voxels make it nearly free, because the normal is always one of six unit vectors
and the blend collapses to a single branch.

```glsl
// Fragment. World position and normal from the vertex stage. No UVs, no atlas, no seams.
float Hash3(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float Noise3(vec3 x) {
    vec3 i = floor(x), f = fract(x);
    f = f * f * (3.0 - 2.0 * f);                       // smoothstep, cheaper than quintic
    return mix(mix(mix(Hash3(i + vec3(0,0,0)), Hash3(i + vec3(1,0,0)), f.x),
                   mix(Hash3(i + vec3(0,1,0)), Hash3(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(Hash3(i + vec3(0,0,1)), Hash3(i + vec3(1,0,1)), f.x),
                   mix(Hash3(i + vec3(0,1,1)), Hash3(i + vec3(1,1,1)), f.x), f.y), f.z);
}
float Fbm(vec3 p) {                                    // 4 octaves is plenty at this scale
    float a = 0.5, s = 0.0;
    for (int i = 0; i < 4; ++i) { s += a * Noise3(p); p *= 2.03; a *= 0.5; }
    return s;
}

void main() {
    // Axis-aligned faces: no blending needed, just pick the two tangent axes.
    vec3 an = abs(vNormal);
    vec2 uv = (an.y > 0.5) ? vWorld.xz : ((an.x > 0.5) ? vWorld.zy : vWorld.xy);

    float grain = Fbm(vec3(uv * 3.0, vWorld.y * 0.5)) * 0.35 + 0.65;
    float strat = 0.90 + 0.10 * sin(vWorld.y * 7.0 + Fbm(vWorld * 0.6) * 4.0); // strata bands

    vec3 base = vColor.rgb * grain * strat;
    gl_FragColor = vec4(base * vAO, 1.0);              // vAO from Stage 1, still doing the work
}
```

Note what this is *not*: it is not a normal map, not PBR, not a material system. It is one
noise function that stops flat colour from looking like flat colour. That is the whole
ambition and it is the right ambition.

**On outlines**, per the Townscaper observation (§2.3): a stylised renderer that commits to
edge lines reads as *designed*. The cheap version for voxels is a depth-discontinuity pass —
sample depth at four neighbours, threshold the difference, darken. ~40 lines of shader. The
even cheaper version for the ~8-cube units is a back-face-expanded shell: draw the model
again, scaled 1.04×, front-face-culled, in near-black. That is 12 lines and it makes miners
and monsters pop off the rock at any camera distance, which matters because at `camDist = 70`
(`:597`) a miner is currently a handful of pixels of similar value to the floor.

**What the user sees:** rock stops being a colour and becomes a material. Units separate from
the background. The camera stops feeling like a debug orbit and starts feeling framed.

---

### Stage 4 — Motion, feedback, and shadows

**~2 days. ~700 LOC. Risk: medium-high (shadow bias is the tuning trap).**

| # | What lands | Est. LOC | Note |
| --- | --- | --- | --- |
| 4.1 | Pooled particle system: drill sparks, rock debris, dust motes, spawn burst | 240 | Highest gain in the stage |
| 4.2 | Layered procedural animation: squash on land, lean into turns, recoil on drill, IK-free foot pinning | 200 | Replaces the single `sin` at `:373` |
| 4.3 | Screenshake with a decaying noise curve (not a sine) | 40 | |
| 4.4 | Damage/impact flash, hit-stop on the drill-through moment | 60 | |
| 4.5 | Single shadow map, 1024², PCF 3×3 | 250 | **Do last. May be cut.** |

**Deliberate ordering note.** Particles and animation are in the same stage as shadows and
placed *before* them, because if the stage runs out of time, 4.5 is the item to drop and the
stage still ships as a large improvement. A shadow map on top of Stage 1's AO is a genuine
but *marginal* gain — the contact shading that shadows are usually hired for is already being
delivered by AO, at a fraction of the cost and with no bias to tune. If someone insists on
shadows, insist back on 4.1–4.4 first.

**On animation.** The current models animate with one shared `sin(phase)` driving bob, legs
and arms (`:373`, `:395`, `:431`). The upgrade is not skeletal animation (see §5) — it is
*layering*: three or four independent sine/impulse channels at different frequencies with
phase offsets, plus event-driven impulses (a recoil spring on each drill hit, a squash on
arrival). This is ~200 lines and it moves the units from "bobbing" to "acting" without any
authored data.

```cpp
// A critically-damped spring: the workhorse of procedural juice. One per animated channel.
struct Spring {
    float x = 0, v = 0, k = 90.0f, d = 2.0f * 9.487f;   // d = 2*sqrt(k) => critical
    void  Kick(float impulse) { v += impulse; }
    float Step(float dt) {                              // sub-step for stability at low fps
        const int n = 4; const float h = dt / n;
        for (int i = 0; i < n; ++i) { v += (-k * x - d * v) * h; x += v * h; }
        return x;
    }
};
// miner.drillRecoil.Kick(-0.9f) on each drill tick; add drillRecoil.Step(dt) to the torso Z.
```

**What the user sees:** drilling throws sparks and rock chips, and the miner rocks back with
each hit. Monsters erupt with a debris burst instead of sliding up out of the floor
(`:418-421`). Impacts feel like impacts. This is the stage that changes how the game *feels*
rather than how it looks, and it is where the audio doc's work should land alongside.

---

### Stage 5 — Post, and the frame around the game

**~1.5 days. ~500 LOC. Risk: medium.**

| # | What lands | Est. LOC |
| --- | --- | --- |
| 5.1 | FBO render target + fullscreen pass plumbing | 150 |
| 5.2 | Bloom: bright-pass, two-tap separable Gaussian, additive | 130 |
| 5.3 | Vignette + subtle chromatic falloff + filmic curve | 70 |
| 5.4 | Real UI: panels, resource readout, selection cards, drawn as GL quads | 250 |

Bloom is deliberately *fifth*. On a scene with no HDR range, bloom is a haze filter. It only
pays once the crystals (Stage 2's single high-chroma value) and the lamps (2.4) are genuinely
brighter than everything around them. Applied at Stage 1 it would have made the image worse
while feeling like progress, which is the most dangerous kind of change on a project that
cannot look at its own output.

**What the user sees:** crystals glow. Lamps bleed light. The image gains a photographic
quality. The HUD stops being debug text in the corner and becomes an interface.

---

### Stage 6 — Content and systems depth

**Open-ended. This is where the project stops being a rendering exercise.**

Ranked by (perceived gain)/(cost), the survivors:

1. **Landslides and erosion** — cellular automaton over `Block::flags`, ~150 LOC, and it
   makes the cavern feel alive and hostile. `SyntheticLevel` already has the substrate.
2. **256×256 maps + chunked meshing** — ~400 LOC, essentially free after Stage 1, and it
   changes the sense of scale more than any shader.
3. **A second camera mode (shoulder view)** — ~80 LOC on the existing `gluLookAt`, and it
   makes the same geometry feel like a different game.
4. **Height variation** — the biggest *shape* change available, but it breaks the
   single-height assumption baked into `Block` and the sandbox depends on that. Cost is in the
   blast radius, not the algorithm.
5. Buildings, vehicles, scripting — real work, low visual return per hour, and they are
   game-design decisions rather than rendering ones.

### Stage summary, with the honest cumulative judgement

| Stage | Days | Est. LOC | Risk | Cumulative screenshot gap closed *(judgement)* |
| --- | --- | --- | --- | --- |
| 0 — honest frame | 1.0 | 350 | low | ~15% |
| 1 — mesh + AO | 1.5 | 500 | low-med | ~40% |
| 2 — light + palette | 1.0 | 400 | low | ~52% |
| 3 — silhouette + surface | 1.5 | 550 | med | ~60% |
| 4 — motion + feedback | 2.0 | 700 | med-high | ~65% |
| 5 — post + UI | 1.5 | 500 | med | ~68% |
| 6 — content | open | — | — | plateaus |

Note the shape of that column. **Stages 0–2 deliver ~52% of the closable gap in 3.5 days;
stages 3–5 deliver ~16% more in 5 days.** If time is the constraint, stopping cleanly after
Stage 2 is a far better outcome than a half-finished Stage 4. Plan for that explicitly rather
than discovering it.

---

## 4. The three highest-leverage changes

### #1 — Per-vertex ambient occlusion on a face-culled mesh (Stage 1)

**One day. ~265 LOC for the core of it.**

Today the renderer draws ~1,600 tiles in the *same six colours* (`:355-362`). There is no
signal anywhere in the image that says "this corner is enclosed" or "this floor meets that
wall". AO adds exactly that signal, at every junction in the scene simultaneously, from a
rule with **one four-entry lookup table as its only tunable** — which under §2.2's
can't-iterate constraint makes it uniquely suited to us.

*Why not shadow maps instead?* Shadows are ~250 LOC plus six numbers found by staring at
output we cannot see. They deliver directional occlusion, which is beautiful, but the
*contact* shading that makes geometry read as solid is AO's job, and AO does it for a third
of the code and none of the risk. Do AO; do shadows later or never.

*Why not SSAO?* See §5.4. For axis-aligned voxels, per-vertex AO is exactly correct and free
per-frame; SSAO is an approximation that costs a depth prepass, a 16-tap kernel and a blur,
and looks *worse* on hard edges.

### #2 — The honest frame: MSAA + vsync + fixed timestep + a HUD that draws (Stage 0)

**One day. ~350 LOC, and one of them fixes a real bug.**

This is the one people underrate because nothing in it is a "feature". But consider what the
scene is made of: axis-aligned cubes viewed from an orbiting perspective camera. Every silhouette
edge in the frame is a near-diagonal line against a contrasting background, which is the
*worst case* for aliasing — and there are thousands of them. Without MSAA the entire image
crawls whenever the camera moves. A viewer will not say "it needs anti-aliasing". They will
say "it looks like a prototype", and they will be responding to this.

Add that the HUD is currently on undefined behaviour (Headline #6) and that the frame limiter
is `::Sleep(8)` (`:717`), and this stage is buying correctness, smoothness and per-pixel
quality in a single day with essentially no technical risk.

*Why ahead of AO?* Because AO makes a shimmering image a shimmering image with better
shading. Fix the frame, then fill it.

### #3 — One palette, one warm/cool light split (Stage 2, items 2.1–2.5)

**Half a day. ~180 LOC, most of it a table and a hash function.**

This is the cheapest of the three by a wide margin and it is the one that makes the other two
legible. AO produces darkening; a coherent palette decides *what colour* that darkening is.
Right now ~30 independently chosen RGB literals across the file mean there is no answer to
that question. Replace them with a table where a cool ambient sits under a warm key, reserve
the single high-chroma value for crystals so the eye always finds the objective, and tie the
fog colour (`:645`) to the ambient so distance reads as atmosphere rather than as a grey wash.

*Why not particles instead?* Particles (Stage 4.1) are excellent and cheap, but they are
*juice* — they change how an action feels at the moment it happens. In a still frame, in a
trailer, in the first three seconds of someone's attention, palette does more. Juice
compounds on top of a good image; it does not substitute for one.

*Why not triplanar texturing instead?* Texturing is ~220 lines of shader written blind, and
it improves surfaces that are already reading correctly. Palette is ~180 lines of arithmetic
that cannot fail to compile and improves everything at once.

**The three together: ~2.5 days, ~800 LOC, and in my judgement roughly 52% of the closable
screenshot gap.** Nothing else in this document approaches that ratio.

---

## 5. What we should explicitly NOT attempt

Each entry gives the arithmetic or the structural reason, not a preference.

### 5.1 Skeletal animation with authored clips

**The arithmetic.** A credible set is idle, walk, drill, carry, hurt, die — six clips. A
minimal humanoid rig is ~12 joints. At 8 keys per clip that is 6 × 12 × 8 = **576 keyframes**,
each a rotation someone must choose. Keyframes are not chosen by reasoning; they are chosen by
scrubbing. With no viewport and no ability to run the build, each key costs a compile-run-look
cycle through another person. At an optimistic 3 minutes per look and 5 iterations per key,
that is **144 hours**. Layered procedural animation (Stage 4.2) gets, in my judgement, ~70% of
the read for ~200 lines and zero iteration.

**Verdict: never, under these constraints.** If the constraints change — if we can run and
look — revisit.

### 5.2 SSAO

**The arithmetic.** SSAO in view space: a depth (and ideally normal) prepass, 16 hemisphere
samples per pixel with a random rotation kernel, plus a 4×4 bilateral blur. At 1280×820 that
is ~1.05 M pixels × 16 taps = **16.8 M depth samples per frame**, plus the blur, plus the FBO
plumbing (~300 LOC).

**The structural point.** SSAO exists to approximate occlusion when you cannot compute it
exactly. On axis-aligned voxels we *can* compute it exactly — the 4-corner rule (§4.1) is
not an approximation of the occlusion, it *is* the occlusion for that geometry, and it costs
zero per frame because it is baked at mesh build. Paying 16.8 M samples per frame for a worse
answer is the definition of a bad trade. SSAO also has a characteristic weakness — halos and
undersampling at hard depth discontinuities — and our scene is *made entirely of* hard depth
discontinuities.

**Verdict: no, and this one is not close.** This is the recommendation I would most expect the
rendering document to disagree with, and I would defend it.

### 5.3 Deferred shading and PBR

**The symbol that kills it.** The reason PBR looks good is that the specular BRDF
`f(l,v) = D(h)·F(v,h)·G(l,v,h) / (4·(n·l)(n·v))` is driven by *spatially varying* roughness
and metalness sampled from texture maps. Strip the maps and roughness becomes a per-object
constant; `D(h)` collapses to a single fixed lobe; the entire model degenerates to one
specular highlight per material — which a two-line Blinn-Phong term already gives us.

We have no texture maps and, per §1.3(c), no way to author them. PBR costs a G-buffer (3–4
render targets), a light accumulation pass, and a rewrite of every draw path, and returns
approximately the visual difference between `pow(dot(n,h), 32.0)` and itself.

**Verdict: no. Use Lambert + a wrap term + one Blinn-Phong lobe, and spend the difference on
Stage 2 and Stage 3.**

### 5.4 A level editor

**The arithmetic.** No GUI toolkit is available and none is coming (§6 declines runtime
dependencies). An immediate-mode GUI written from scratch — text input, sliders, scroll
regions, focus, keyboard nav, layout — is **3,000+ lines before the first useful tool exists**,
and every one of those lines is UI code that must be *looked at* to be correct (§2.2).

**And we already have the editor.** `SyntheticLevel.hpp:96-98` declares
`Level::ToText()` / `Level::FromText()`, described in its own comment as "human-authorable,
diffable, and the thing a campaign author would eventually edit". A text format editable in
Notepad, diffable in git, with `R` to reload, is 95% of the *value* of a level editor for
this project's actual audience, at ~30 lines of file I/O.

**Verdict: no editor. Ship a documented text format and a hot-reload key.**

### 5.5 Physics and Teardown-style destruction

**The arithmetic.** Destruction means fragments, fragments mean rigid bodies, and a
from-scratch sequential-impulse solver with broadphase, contact manifolds and friction is
~2,000 lines *before* it stops jittering. A single wall shattering into 5×5×5 fragments is 125
bodies; a landslide across ten tiles is 1,250.

**Buy the look instead.** A particle burst of 40 tumbling cubes on a ballistic arc with a
floor bounce (`v.y *= -0.35` on ground contact) plus screenshake is **~120 lines** and, in a
0.6-second event that the player is not inspecting closely, reads as approximately the same
thing.

**Verdict: no solver. Stage 4.1 buys the perception at 6% of the cost.**

### 5.6 Matching Manic Miners' feature list

Vehicles, buildings, first-person view, scripting, mod support, campaign. Roughly 6,000+ LOC
of *game* work with, per the table in §1.3(b), low visual return per hour — and a strategic
error besides. A partial clone of a finished, free, better-resourced game invites exactly the
comparison we lose. `DeepCore3D.cpp:11-20` already picked a different and defensible position:
the systems are readable and hackable and they live in this repository. Feature-matching
abandons that position to compete on the axis where we are weakest.

**Verdict: no. Compete on legibility and on look, not on feature count.**

### 5.7 Any core-profile OpenGL context

Covered in §2.1. It costs `glBegin`, `gluUnProject` (our picking, `:523`), `gluPerspective`,
`gluLookAt`, display lists and `wglUseFontBitmaps` — all at once, for a benefit
(validation strictness) worth nothing to us. A compatibility context gives us every modern
feature the driver has *and* keeps all of the above.

**Verdict: never request one. Write it in a comment at the context-creation site so nobody
"modernises" it later.**

### 5.8 Generative music

Ambient drone and stingers: yes, cheap, do it (the audio document owns this). *Music* someone
wants to hear for an hour: that is an open research problem, not a task, and the failure mode
— music that is technically correct and actively unpleasant — is worse than silence.

---

## 6. The dependency question, answered on evidence

The brief invites this to be reopened, so let me open it properly rather than defending the
default reflexively.

### 6.1 What a dependency would actually have to save us

Going down the plan, the code we are about to write that a library could supply:

| Work item | Stage | Est. LOC | A library could supply it? |
| --- | --- | --- | --- |
| GL extension loading | 0 | 60 | glad / GLEW — yes |
| MSAA pixel format selection | 0 | 120 | SDL2 / raylib / GLFW — yes |
| Text rendering | 0 | 60 | stb_truetype — yes, but see below |
| Fixed timestep + input | 0 | 130 | SDL2 / raylib — yes |
| Mesh build + AO | 1 | 265 | **No. This is the actual work.** |
| Palette + lighting | 2 | 400 | **No.** |
| Shaders | 3 | 220 | Loader helpers only; the shaders are ours |
| mat4 / quaternion maths | 3+ | ~180 | HandmadeMath — yes |
| Particles, springs, camera | 4 | 400 | **No.** |
| FBO / post plumbing | 5 | 150 | Partially |

**Total library-substitutable: ~700 of ~3,000 lines, and it is all in Stages 0, 3 and 5 —
none of it in the three highest-leverage changes.** That is the whole answer in one row, and
everything below is just checking it.

### 6.2 raylib — evaluated properly

- **Licence:** unmodified zlib/libpng. OSI-certified, BSD-like, explicitly permits static
  linking with closed source ([raylib.com/license.html](https://www.raylib.com/license.html)).
  No licence objection whatsoever.
- **x86 / v142 availability:** yes. Official releases ship `raylib-5.5_win32_msvc16.zip`;
  `msvc16` is Visual Studio 2019, which is toolset **v142**
  ([releases](https://github.com/raysan5/raylib/releases),
  [mirror](https://sourceforge.net/projects/raylib.mirror/files/5.5/)). Genuinely compatible.
- **Binary size:** roughly 1–1.5 MB added to a statically linked x86 executable in practice.
  Real but not disqualifying.
- **What it would save:** window/context creation, MSAA, vsync, input, a font renderer,
  `rlgl`'s automatic batching, shader helpers, and audio via bundled miniaudio.
- **What it costs, and this is the real argument:**
  1. **It takes the frame loop.** raylib owns `BeginDrawing`/`EndDrawing` and the window. Our
     `WinMain` (`:613-725`), our `WndProc` (`:537-601`) and our message pump are ~150 lines
     that already work. We would delete working code to adopt a foreign structure.
  2. **CRT link-mode coupling.** raylib's prebuilt MSVC static libs must match our `/MT` vs
     `/MD` choice or we get `LNK2038`/`LNK2005`. Building it ourselves fixes that and adds a
     CMake step to a repo whose build is currently `msbuild` on `.vcxproj` files.
  3. **`/W4` and `SDLCheck` are on** (`deepcore3d.vcxproj:59-60, 76-77`). Third-party headers
     included at `/W4` routinely emit warnings, which means `#pragma warning(push/disable)`
     wrappers or a relaxed warning level — and "0 warnings at /W4" is a stated hard constraint.
  4. **It costs the project's thesis.** The README's claim is that these systems are readable
     and live here. `DeepCore3D.cpp:22-23` says "DEPENDENCIES: none beyond the Windows SDK".
     That is not vanity; it is the differentiator against a better-funded, better-looking
     competitor.
- **What it does *not* save:** anything in §4. Not the AO, not the palette, not the meshing,
  not the animation. raylib solves *portability and setup*. Our problem is *look*.

**Verdict: decline.** It is a good library, correctly licensed, genuinely x86/v142-compatible,
and it solves the ~250 lines of Stage 0 we can write in a day while costing a build system, a
warnings fight, and the project's central claim.

### 6.3 SDL2 — evaluated properly

Same licence class (zlib), same maturity, same x86 availability, and a stronger audio and
input story than raylib. And the same verdict, for a sharper reason: SDL's value is
**portability across platforms and input devices**. This is a Win32 program with a `WNDCLASS`,
a `WndProc` and `opengl32.lib`, targeting one platform, and the portability we do have
(`src/sandbox/CMakeLists.txt` builds the *logic* on Linux and Emscripten) is achieved by
keeping `DeepCoreLogic.hpp` stdlib-only, not by a portability layer in the renderer.

**Verdict: decline.** SDL2 solves a problem this program does not have.

### 6.4 The candidates that are actually tempting

| Candidate | Form | Licence | Binary cost | Verdict |
| --- | --- | --- | --- | --- |
| Khronos `glext.h` + `wglext.h` | declaration-only headers | MIT (Khronos) | **zero** | **Take, conditionally** |
| HandmadeMath | single header, all `static inline` | public domain / CC0 ([LICENSE](https://github.com/HandmadeMath/HandmadeMath/blob/master/LICENSE)) | ~0 | Lead's call |
| `stb_truetype.h` | single header | MIT / public domain | ~30 KB | **Decline — unnecessary** |
| glad | generated loader source | MIT | small | Decline — `glext.h` + 60 lines does it |

**Khronos headers — the one with a real case.** Hand-declaring GL entry points is tedious,
typo-prone, and returns exactly zero visual improvement per hour spent. `glext.h` and
`wglext.h` are *declarations only*: no code, no binary, no build change, no link step, no
runtime. They are the antithesis of a dependency — the only thing they add is text.

**The condition:** if the plan's shader work needs fewer than ~40 entry points (my estimate:
Stages 0–3 need about 28), hand-declaring them in one clearly-commented block is ~60 lines,
keeps the "zero external files" claim literally true, and avoids `/W4` noise from a header we
do not control. **Take the Khronos headers only if the count crosses ~80.** Note also that
`glext.h` at `/W4` is generally clean, but it is not *our* file and `SDLCheck` is on; verify
before committing.

**HandmadeMath.** Header-only, all `static inline`, C and C++, public domain — no attribution
burden, no NOTICE.md entry required, no binary cost, x86-irrelevant. It saves ~180 lines of
`mat4`/quaternion code we will otherwise write in Stage 3 when we leave the fixed-function
matrix stack. This is the closest call in the document, and I land marginally against it: the
maths is 180 lines we understand completely, it is the least likely code in the project to be
wrong, and writing it preserves an unqualified claim. But I would not argue with a lead who
took it, and I would not call that a mistake.

**`stb_truetype` — declined for a concrete reason.** It solves scalable text. We do not have
that problem, because `wglUseFontBitmaps` (§3, Stage 0.1) puts any installed Windows font into
GL display lists in about fifteen lines, using the OS's own text stack at runtime with no font
file read, embedded or redistributed. For a HUD at one or two fixed sizes that is complete.
The one thing it cannot do is scale smoothly with resolution — and `wglUseFontOutlines`, also
free, covers that if we ever need it.

### 6.5 Recommendation

**Keep the zero-runtime-dependency stance, on evidence rather than principle.** The evidence:

1. Only ~700 of ~3,000 planned lines are library-substitutable (§6.1), and **none of them are
   in the three highest-leverage changes** (§4).
2. The GL feature ceiling people assume the stance imposes **does not exist** — every modern
   GL feature is one `wglGetProcAddress` away, no library required (§2.1).
3. The binding constraint is the absence of a *content pipeline* (§2.2), and no library on the
   list supplies one.
4. `/W4` with 0 warnings and `SDLCheck` is a stated hard constraint, and third-party headers
   at `/W4` are a recurring tax on a benefit worth about one day.
5. The stance is the project's actual differentiator against a competitor we cannot beat on
   polish. Trading it for a day of setup code is a bad trade *even if the day were free*.

**Accept exactly one carve-out, and write it into the project's rules so it does not get
relitigated every sprint:** declaration-only, zero-binary, permissively-licensed headers are
permitted; anything that links is not. That is a rule with a bright line, it is defensible in
the README, and it takes the only dependency that was ever actually worth taking.

---

## 7. Decision

**If there is one day:** Stage 0. Fix the HUD bug, add MSAA, add vsync, add a fixed timestep.
The program stops reading as a prototype and starts reading as software.

**If there are three days:** Stages 0, 1 and 2. In my judgement this is ~52% of the closable
screenshot gap for ~1,250 lines, and it is the best stopping point in the entire plan — the
result is *coherent* rather than partially upgraded.

**If there is a week:** add Stage 3. Rock becomes material, units gain silhouette, the camera
starts framing rather than orbiting.

**If there are two weeks:** Stages 4 and 5, with shadows (4.5) as the designated cut.

**What to say about it publicly:** exactly what `DeepCore3D.cpp:11-20` already says. Not "a
Manic Miners competitor". Something like: *a subterranean mining game whose every system —
generation, waves, lighting, animation, audio — is a few hundred lines of readable C++ in this
repository, with no dependencies and no assets.* That is true, it is unusual, it is the thing
Manic Miners is not, and it is the only framing under which the comparison flatters us.

---

## Appendix A — Risk register

| # | Risk | Where | Mitigation |
| --- | --- | --- | --- |
| A1 | GDI HUD is UB over a double-buffered surface | `:632`, `:603-608` | Stage 0.1. **Existing bug, not a new risk.** |
| A2 | No ICD (RDP, VM, pre-driver) → GL 1.1 only, all loads fail | new loader | Tier 0 fallback, §2.1. Retain the immediate-mode path. |
| A3 | `wglGetProcAddress` returns 1/2/3/−1 for "absent" on some drivers | new loader | Check all four sentinels — code in §2.1 |
| A4 | `/W4` breakage from function-pointer casts | new loader | Cast through a helper; scope any `#pragma warning` narrowly |
| A5 | AO dirty-rect too small on drill → permanent bright square | Stage 1.5 | Invalidate (x±2, z±2), not (x, z) |
| A6 | AO diagonal crease across every tile | Stage 1.3 | The flip rule — one line, easy to omit, very visible |
| A7 | Shadow-map bias tuned blind → acne or peter-panning | Stage 4.5 | Designated cut item. Normal-offset bias is the least fiddly variant |
| A8 | Bloom on a non-HDR scene reads as haze | Stage 5.2 | Sequenced after Stage 2 for exactly this reason |
| A9 | `glReadPixels` picking stalls the pipeline every click | `:519` | Acceptable at click frequency. If it bites, switch to a CPU ray-vs-grid march — ~50 LOC and it removes the readback entirely |
| A10 | Height-map work breaks `SyntheticLevel` and the logic tests | Stage 6.4 | Do not start it without a plan for the sandbox's dependents |

## Appendix B — Sources

- Manic Miners features — https://manicminers.baraklava.com/features/
- Manic Miners modding (mod support is *planned*) — https://manicminers.fandom.com/wiki/Modding
- Manic Miners home — https://manicminers.baraklava.com/
- LDraw legal / CCAL 2.0 — https://www.ldraw.org/legal-info
- Loading OpenGL functions (GL 1.1 static, rest via `wglGetProcAddress`) — https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
- `wglGetProcAddress` practice and sentinel returns — https://dwmkerr.com/importing-opengl-extensions-functions-with-wglgetprocaddress/
- Manual OpenGL setup on Windows — https://riptutorial.com/opengl/example/5305/manual-opengl-setup-on-windows
- `PFD_SUPPORT_GDI` and `PFD_DOUBLEBUFFER` are mutually exclusive — https://community.khronos.org/t/pfd-doublebuffer/51171
- GDI-over-GL clobbering on Windows 10 — https://community.intel.com/t5/Intel-Fortran-Compiler/Windows10-Fortran-OpenGL-GDI/m-p/1091690
- `ChoosePixelFormat` reference — https://learn.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-choosepixelformat
- Ambient occlusion for Minecraft-like worlds (the 4-corner rule and the flip) — https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
- Meshing in a Minecraft game (greedy meshing) — https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/
- `.kkrieger` — https://en.wikipedia.org/wiki/.kkrieger
- werkkzeug3 source (the tool that made it possible) — https://github.com/jaromil/kkrieger-werkkzeug3
- Townscaper rendering style breakdown — https://reindernijhoff.net/2021/11/townscapers-rendering-style-in-webgl/
- Townscaper — https://en.wikipedia.org/wiki/Townscaper
- raylib licence (zlib/libpng) — https://www.raylib.com/license.html
- raylib releases, incl. `win32_msvc16` (x86, VS2019 = v142) — https://github.com/raysan5/raylib/releases
- raylib 5.5 win32 msvc16 mirror — https://sourceforge.net/projects/raylib.mirror/files/5.5/
- raylib — https://github.com/raysan5/raylib
- HandmadeMath licence (public domain / CC0) — https://github.com/HandmadeMath/HandmadeMath/blob/master/LICENSE
- HandmadeMath README — https://github.com/HandmadeMath/HandmadeMath/blob/master/README.md
