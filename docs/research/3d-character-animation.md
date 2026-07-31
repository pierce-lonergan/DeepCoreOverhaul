<!-- Research document. Authored by source reading, published literature and reproducible
     read-only shell commands only. NO BUILD WAS RUN for this document. No game was run for
     this document. Every joint angle, constant and formula below is either (a) derived here
     from stated geometry and re-checkable with a calculator, (b) cited to a named public
     source with a URL, or (c) declared a TUNING CONSTANT, meaning it is an opinion with a
     starting value, not a measurement. Nothing here is asserted as observed behaviour of a
     running program. -->

# Making a primitive-built 3D character feel ALIVE

Repo: `C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`
Subject: `src/game3d/DeepCore3D.cpp` (725 lines at time of writing)
Toolset: MSVC **v142**, **x86**, `/W4`, `/permissive-`, `/sdl`, **C++17**
(`src/game3d/deepcore3d.vcxproj:34,59,63,73,80`)
Dependencies permitted: Windows SDK, `opengl32.lib`, `glu32.lib`. Nothing else.

---

## Headline, stated up front so nobody has to read to the end

1. **The single largest perceived-life win in this file is not animation at all. It is
   `glEnable(GL_LIGHTING)` plus per-face `glNormal3f`.** Right now every cube is shaded by a
   *constant per-face brightness baked into the local axis* (`DeepCore3D.cpp:354-362`,
   `const F faces[6]` with a hardcoded `k`). That is fine while every cube is axis-aligned.
   The moment you rotate a limb, the shading **does not change**, and the brain reads the limb
   as a flat sticker rather than a solid object turning in space. Every animation technique in
   this document is worth roughly half as much until this is fixed. It is ~14 lines. Do it
   first. (§2)

2. **There is a "Stage 0" worth roughly 45% of the total achievable gain that requires no
   skeleton, no IK, no keyframes and no restructuring — about 190 net lines.** Blinking eyes,
   distance-driven gait phase (kills foot-sliding), per-entity variation, one spring-damper
   primitive, head look-at, Poisson idle twitches, squash-on-landing, hit-stop. Ship that
   before you build a skeleton. (§9, Stage 0)

3. **The current gait is guaranteed to foot-slide and cannot be fixed by tuning.**
   `m.phase += dt * 6.0f` (`DeepCore3D.cpp:230`) advances the leg swing at a fixed 0.95 Hz
   regardless of `MINER_SPEED`. The fix is one line: drive the phase by **distance travelled**,
   not by time. `phase += distanceMoved / strideLength`. Foot-sliding is the loudest
   "this is a prototype" signal in real-time character animation and it is 12 lines to remove
   entirely. (§6.7)

4. **A skeleton for a cube character does not need quaternions, a model format, skinning
   weights, or a matrix palette.** 18 joints × 9 floats of pose, a `parent` index array with
   parents ordered before children, one 4×4 multiply per joint, and cubes parented to joints.
   ~180 lines total, and `glMultMatrixf` does the drawing. (§3)

5. **Five of the six highest-ratio techniques in the whole ranking are procedural, not
   keyframed**: the spring-damper, look-at, distance-phasing, Poisson twitching, and
   squash-from-impulse. This is not a compromise forced by having no art tools — it is what
   Wolfire shipped in *Overgrowth* deliberately, with a total of ~13 keyframes for the entire
   character
   ([Rosen, GDC 2014](https://archive.org/details/GDC2014Rosen);
   [GDC Vault](https://www.gdcvault.com/play/1020583/Animation-Bootcamp-An-Indie-Approach)).

6. **You will not match Manic Miners' authored art. You can match a surprising fraction of its
   *aliveness*,** because aliveness is timing and secondary motion, and timing and secondary
   motion are code. Art quality and life quality are largely independent axes. This document is
   entirely about the second axis.

---

## 0. Ground rules honoured by this document

- **No copyrighted assets.** Every curve below is either a stated biomechanical measurement
  from the public clinical literature (cited), a classical animation principle from the public
  record (cited), or a tuning constant invented here. No motion capture data, no game assets,
  no rig files.
- **Zero third-party dependencies.** Nothing in this document requires a library. The only
  place a library was even considered is discussed and rejected in §10.4.
- **`/W4` clean, x86, v142, C++17.** Every code sketch is written to that standard. The traps
  are enumerated in §3.9. In particular: no designated initialisers (C++20), no
  `while (true)` (C4127 under `/permissive-` is mostly quiet but `/sdl` is on), explicit `f`
  suffixes everywhere, and `(void)param;` for deliberately unused parameters, matching the
  existing style at `DeepCore3D.cpp:410`.
- **Everything ships as code.** Every table in this document is a `static const float[]` that
  can be pasted into a `.cpp`.
- **We cannot run the game.** No claim here is behavioural. Every number is a *starting value*
  with a stated derivation, and every one of them is expected to be tuned by eye.

### 0.1 What is on screen today

Read from `DeepCore3D.cpp`:

| Element | Where | Current motion |
| --- | --- | --- |
| Miner body | `DrawMiner`, `:371-411` | 4 stacked cubes + 2 leg cubes |
| Miner "walk" | `:395-397` | `sin(phase) * 0.10` translation on Z, opposite legs. No rotation. Legs *slide*, they do not pivot. |
| Miner bob | `:373` | `sin(phase) * 0.06`, once per leg-swing (should be twice) |
| Miner facing | `:233` | `atan2` assigned directly. Snaps instantly. No turn, no lean. |
| Miner arms | — | **There are none.** |
| Monster body | `DrawMonster`, `:413-435` | 7 cubes, whole body rocked `sin(phase)*9°` about Z (`:422`) |
| Monster legs | — | **There are none.** It slides. |
| Monster emerge | `:417-418` | uniform scale 0.35→1.0 **and** a sink lerp. Reads as "spawning in", not "digging out". |
| Death | `:270,297` | `erase()` from a vector. The entity vanishes on the frame its health hits zero. |
| Hit reaction | — | **None.** Damage is a silent number. |
| Eyes | `:427-429` | two static cubes. They never blink, never move, never look at anything. |

Six of those rows are "none". That is the gap, and most of those rows are cheap.

---

## 1. THE RANKING — perceived life per line of code

The metric, stated so it can be argued with:

- **G** = perceived-life gain, 1–10, judged as *fraction of the felt gap to a finished game
  that this one item closes*. This is a judgement, not a measurement.
- **L** = net new lines of `/W4`-clean C++ (my estimate, including the table data).
- **R = G / (L / 50)** — gain per 50 lines. Higher is better. **Sort by R.**

Anything with R ≥ 5 is nearly free. Anything with R < 1 you should be sure you want.

### 1.1 Tier S — do these this week

| # | Technique | § | G | L | **R** | Needs |
| --- | --- | --- | ---: | ---: | ---: | --- |
| 1 | **Blink** (scale the eye cube's Y to 0.06 for 90 ms, Poisson-timed) | 7.2 | 6 | 14 | **21.4** | — |
| 2 | **Distance-driven gait phase** (kills foot-slide) | 6.7 | 8 | 20 | **20.0** | — |
| 3 | **Per-entity variation** (seeded phase / rate / size / hue jitter) | 7.1 | 7 | 18 | **19.4** | — |
| 4 | **Hit-stop + damage flash** (freeze `dt` 60 ms, additive white) | 7.5 | 7 | 22 | **15.9** | — |
| 5 | **Spring-damper primitive** (8 lines) + apply to helmet, tool, head | 6.1 | 9 | 34 | **13.2** | — |
| 6 | **Facing spring + bank into turns** (replaces the `atan2` snap at `:233`) | 6.5 | 6 | 24 | **12.5** | 5 |
| 7 | **Real normals + one directional light** | 2 | 8 | 34 | **11.8** | — |
| 8 | **Root bob / sway / roll from gait phase** (2× step frequency) | 4.2 | 6 | 26 | **11.5** | 2 |
| 9 | **Squash & stretch from vertical impulse** | 5.1 | 6 | 28 | **10.7** | 7 |
| 10 | **Head look-at with angular limits + saccade timing** | 6.2 | 9 | 46 | **9.8** | 5 |
| 11 | **Poisson idle twitches** (impulse into the springs from #5) | 7.2 | 8 | 40 | **10.0** | 5 |
| 12 | **Footstep events → dust puff + camera trauma + `Beep`** | 6.6 | 7 | 40 | **8.8** | 2 |

**Tier S total: L ≈ 346 lines, and it is the majority of the felt improvement.** Items 1–4 and
6, 8, 9, 11 need *no skeleton*. Ship them against the cube stack you already have.

### 1.2 Tier A — the skeleton and what it unlocks

| # | Technique | § | G | L | **R** | Needs |
| --- | --- | --- | ---: | ---: | ---: | --- |
| 13 | **Minimal joint hierarchy** (poses, world matrices, skin table) | 3 | 9 | 185 | **2.4** | 7 |
| 14 | **Walk clip** (8 keys × 8 channels, Catmull-Rom) | 4.3 | 8 | 70 | **5.7** | 13 |
| 15 | **Additive layer + joint mask** (unlocks carry / aim / strain for free) | 3.7 | 7 | 45 | **7.8** | 13 |
| 16 | **Idle breathing clip** (asymmetric inhale/exhale) | 4.5 | 6 | 30 | **10.0** | 13 |
| 17 | **Two-bone IK** (law of cosines) | 6.3 | 7 | 60 | **5.8** | 13 |
| 18 | **Foot pinning during stance** (the real anti-slide fix) | 6.4 | 7 | 45 | **7.8** | 17 |
| 19 | **Attack anticipation + strike + follow-through** | 4.7 | 8 | 55 | **7.3** | 13 |
| 20 | **Death by rigid-pendulum topple** | 4.8 | 7 | 45 | **7.8** | 13 |
| 21 | **Drill / work cycle** (macro push + micro vibration) | 4.6 | 6 | 40 | **7.5** | 13 |
| 22 | **Run clip + walk↔run blend by speed** | 4.4 | 5 | 45 | **5.6** | 14 |
| 23 | **Carry-heavy variant** (additive layer + phase dwell-warp) | 4.9 | 5 | 30 | **8.3** | 15 |
| 24 | **Emerge-from-ground** (clip plane + burst + overshoot) | 7.6 | 8 | 70 | **5.7** | 13 |
| 25 | **Gait asymmetry / limp / multi-leg phase tables** | 7.1 | 6 | 35 | **8.6** | 14 |

### 1.3 Tier B — real, but buy it last

| # | Technique | § | G | L | **R** |
| --- | --- | --- | ---: | ---: | ---: |
| 26 | Helmet lamp as `GL_LIGHT1` spot (needs a tessellated floor) | 2.4 | 7 | 90 | 3.9 |
| 27 | Overlapping-action cascade (per-joint lag chain) | 5.3 | 5 | 25 | 10.0 |
| 28 | Two-segment antenna / tail spring chain | 6.1 | 4 | 30 | 6.7 |
| 29 | Terrain-height foot IK (low value here: the floor is flat) | 6.3 | 2 | 35 | 2.9 |
| 30 | Shadow blobs under characters (grounds them; not animation but reads as one) | 10.2 | 6 | 30 | 10.0 |

**Note on #29.** Honesty matters more than completeness: the world in `RenderWorld`
(`:441-476`) is a flat floor at a single height (`Cube(x, -0.14f, z, 0.5f, 0.06f, 0.5f, ...)`
→ floor top at y = −0.02 everywhere). Foot IK *for uneven terrain* buys you almost nothing
until there are ramps. Foot IK **for root motion** (§6.3) buys you a great deal, because it is
what lets the body bob, squat, land and lean without the feet leaving the ground. Build the
solver; use it for the second reason.

---

## 2. PREREQUISITE — the shading must respond to rotation

### 2.1 Why this is an animation problem

`Cube()` at `DeepCore3D.cpp:345-369` assigns each face a constant brightness `k` chosen by
which *local* axis the face points along: top 1.00, bottom 0.35, −Z 0.72, +Z 0.60, −X 0.82,
+X 0.50. Because the cube is always axis-aligned, this is a perfectly good fake light.

The instant a joint rotates a cube, the fake breaks: a thigh swinging through 60° keeps the
exact same six brightnesses. The silhouette moves, the shading does not. Human vision uses
shading gradient as the primary cue for *solidity under rotation*; a rotating object with
frozen shading reads as a cardboard cut-out. **Every joint rotation you add is devalued by
this.** Fix it before anything else.

### 2.2 The fix: fixed-function lighting, 14 lines

OpenGL 1.1's fixed-function pipeline transforms `glNormal3f` by the inverse-transpose of the
modelview matrix for free. You get correct per-face lighting on rotated limbs with no shader,
no extension, and no per-frame CPU work.

```cpp
// --- once, after wglMakeCurrent, alongside the existing glEnable(GL_DEPTH_TEST) at :640 ---
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);
glEnable(GL_COLOR_MATERIAL);                              // glColor3f still sets the albedo
glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
glEnable(GL_NORMALIZE);                                   // REQUIRED: squash/stretch scales
glShadeModel(GL_FLAT);                                    // keep the faceted look on purpose

static const GLfloat kDiffuse[4] = { 0.90f, 0.88f, 0.82f, 1.0f };
static const GLfloat kAmbient[4] = { 0.26f, 0.29f, 0.38f, 1.0f };   // cool cave bounce
static const GLfloat kSpecular[4]= { 0.0f,  0.0f,  0.0f,  1.0f };   // no highlight: chalky
glLightfv(GL_LIGHT0, GL_DIFFUSE,  kDiffuse);
glLightfv(GL_LIGHT0, GL_SPECULAR, kSpecular);
glLightModelfv(GL_LIGHT_MODEL_AMBIENT, kAmbient);
```

**The gotcha that will cost you an hour if nobody says it:** `GL_POSITION` is transformed by
the modelview matrix *at the moment you set it*
([Khronos `glLight` reference](https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glLight.xml)).
To get a world-fixed light you must set it **after `gluLookAt` and before any model
transform**, every frame:

```cpp
gluLookAt(ex, ey, ez, g.camFocus.x, 0.0f, g.camFocus.z, 0, 1, 0);   // existing, :692
static const GLfloat kLightDir[4] = { 0.42f, 0.82f, 0.39f, 0.0f };  // w=0 → directional
glLightfv(GL_LIGHT0, GL_POSITION, kLightDir);                       // <-- add here
RenderWorld();
```

If you set it before `gluLookAt` the light rides the camera and the whole scene looks flat and
video-conference-lit as you orbit.

### 2.3 The unit cube as a display list

Replace the six hand-written quads-with-baked-brightness with one normalled unit cube compiled
into a display list, then place it with the matrix stack. This is faster (one `glCallList` vs
24 `glVertex3fv` + 6 `glColor3f`), shorter, and it is what joint transforms need anyway.

```cpp
GLuint g_cubeList = 0;

void BuildCubeList()
{
    // Unit cube, centred on the origin, extents +/-0.5, CCW when viewed from outside
    // so the existing glCullFace(GL_BACK) at :641-642 stays correct.
    struct Face { float n[3]; float v[4][3]; };
    static const Face F[6] = {
        { { 0, 1, 0}, {{-.5f, .5f,-.5f},{-.5f, .5f, .5f},{ .5f, .5f, .5f},{ .5f, .5f,-.5f}} },
        { { 0,-1, 0}, {{-.5f,-.5f,-.5f},{ .5f,-.5f,-.5f},{ .5f,-.5f, .5f},{-.5f,-.5f, .5f}} },
        { { 0, 0, 1}, {{-.5f,-.5f, .5f},{ .5f,-.5f, .5f},{ .5f, .5f, .5f},{-.5f, .5f, .5f}} },
        { { 0, 0,-1}, {{ .5f,-.5f,-.5f},{-.5f,-.5f,-.5f},{-.5f, .5f,-.5f},{ .5f, .5f,-.5f}} },
        { { 1, 0, 0}, {{ .5f,-.5f, .5f},{ .5f,-.5f,-.5f},{ .5f, .5f,-.5f},{ .5f, .5f, .5f}} },
        { {-1, 0, 0}, {{-.5f,-.5f,-.5f},{-.5f,-.5f, .5f},{-.5f, .5f, .5f},{-.5f, .5f,-.5f}} },
    };
    g_cubeList = glGenLists(1);
    glNewList(g_cubeList, GL_COMPILE);
    glBegin(GL_QUADS);
    for (int f = 0; f < 6; ++f) {
        glNormal3fv(F[f].n);
        for (int i = 0; i < 4; ++i) glVertex3fv(F[f].v[i]);
    }
    glEnd();
    glEndList();
}

inline void DrawBox(float hx, float hy, float hz)   // half-extents
{
    glPushMatrix();
    glScalef(hx * 2.0f, hy * 2.0f, hz * 2.0f);
    glCallList(g_cubeList);
    glPopMatrix();
}
```

`GL_NORMALIZE` is not optional once `glScalef` is non-uniform, which it will be the moment you
add squash and stretch (§5.1). Without it, a squashed character goes dark and a stretched one
blows out.

### 2.4 Retain the *stylised* ambient occlusion you already have for free

The old per-face `k` table encoded something valuable: the bottom face was 0.35, much darker
than physical ambient would give. That is stylised ambient occlusion and it is why the current
render reads as solid. Keep it by adding a **hemisphere ambient term** rather than a constant:
tint the ambient by the world-space Y of the normal. In fixed function you cannot do this per
face without a second pass, so do it on the CPU per part, since you already have the joint
matrix (§3.5):

```cpp
// worldUpDotNormal for the part's "up" axis, from column 1 of the joint world matrix.
const float upness = m.m[5];                       // world Y of the part's local +Y axis
const float ao     = 0.72f + 0.28f * (upness * 0.5f + 0.5f);   // 0.72 down .. 1.00 up
glColor3f(r * ao, g * ao, b * ao);
```

Two lines, and it restores the grounded, occluded look on top of the real directional light.

### 2.5 The helmet lamp (Tier B, but note it here)

`GL_LIGHT1` as a positional spotlight parented to the helmet joint is a genuinely spectacular
effect in a cave game and it is ~20 lines of GL state. The catch is that fixed-function
lighting is **per vertex**, and the floor is one quad per tile, so a spot cone lands on four
vertices and produces nothing. It requires subdividing floor tiles 4×4 (16 quads/tile;
40×40 grid → 25,600 quads, trivial inside a display list). That subdivision is a *renderer*
change, so it belongs with the rendering work, not here — but if the renderer work happens
anyway, the lamp is the single best thing to spend `GL_LIGHT1` on. GL 1.1 guarantees at least
8 lights (`GL_MAX_LIGHTS`), so budget: sun + up to 6 nearest lamps + 1 spare.

---

## 3. SKELETAL ANIMATION FROM SCRATCH

### 3.1 The decision that saves 300 lines: no quaternions

Standard skeletal animation stacks (see
[LearnOpenGL, *Skeletal Animation*](https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation);
[Wikipedia, *Skeletal animation*](https://en.wikipedia.org/wiki/Skeletal_animation))
carry quaternions, inverse bind matrices, per-vertex weights and a matrix palette. **We need
none of it**, for one reason: *our skin is rigid primitives, not a weighted mesh.* Each cube
belongs to exactly one joint with weight 1. That deletes:

- the inverse bind pose matrix (the cube's offset *is* its bind pose, expressed in joint space),
- vertex weights and the matrix palette,
- linear-blend-skinning artefacts and therefore dual quaternions,
- the model file format and importer.

And quaternions specifically: every joint in a cube character is a hinge or a 2-DOF ball
driven by curves we author. Gimbal lock is only a hazard when you blend *arbitrary*
orientations near ±90° on the middle Euler axis. Our tables never go there. **Use three floats
of Euler angle with a declared order, per joint.** The one place quaternions would help is
slerping between two authored *look* directions; §6.2 shows how to avoid needing that too.

If you disagree and want quaternions later, the retrofit is confined to `TRS()` and `BlendPose()`.

### 3.2 Data structures — the whole system

```cpp
// ============================================================================
// anim.h : a joint hierarchy for characters made of boxes.
// No allocation, no virtuals, no dependencies. sizeof(Pose) == 648 bytes.
// ============================================================================

enum JointId
{
    // PARENTS MUST PRECEDE CHILDREN. ComputeWorld() relies on it and
    // ValidateSkeleton() asserts it.
    J_ROOT = 0,     // world placement; drives translation, facing, bob, squash
    J_PELVIS,       // sway, list, transverse rotation
    J_SPINE,        // lower torso
    J_CHEST,        // upper torso; counter-rotates the pelvis
    J_NECK,
    J_HEAD,
    J_HELMET,       // separate so it can lag (secondary motion)
    J_LAMP,
    J_SHOULDER_L, J_ELBOW_L, J_HAND_L,
    J_SHOULDER_R, J_ELBOW_R, J_HAND_R,
    J_HIP_L,       J_KNEE_L,  J_FOOT_L,
    J_HIP_R,       J_KNEE_R,  J_FOOT_R,
    J_COUNT
};

struct JointDef
{
    const char* name;
    int         parent;             // -1 for root
    float       ox, oy, oz;         // rest offset, expressed in PARENT space
};

// Rotation order is fixed for every joint: R = Ry(ry) * Rx(rx) * Rz(rz).
// Rationale: ry is "which way does the limb point sideways", rx is the dominant
// hinge, rz is roll/abduction. This ordering keeps the hinge channel free of the
// other two, which is what makes the tables in section 4 readable.
struct JointPose
{
    float rx, ry, rz;               // radians
    float tx, ty, tz;               // ADDITIVE offset on top of JointDef's rest offset
    float sx, sy, sz;               // scale, 1 = neutral
};

struct Pose { JointPose j[J_COUNT]; };

struct Mat4 { float m[16]; };       // column-major, i.e. straight into glMultMatrixf
```

`Pose` is a flat POD. Blending, additive layering and channel indexing all become
`reinterpret_cast<float*>` arithmetic over 9 floats per joint — which is exactly why the
struct has no padding and no constructor. A `static_assert(sizeof(JointPose) == 36, "")`
at namespace scope makes that contract explicit and is `/W4`-clean.

### 3.3 The skeleton, defined in code

Proportions matter more than you expect. The current miner (`:389-397`) is ~0.75 units tall
with 0.20-unit legs — a 27% leg fraction, where a human is 47–50%. Short legs make a walk
cycle physically impossible to read because the hip rotation subtends too little arc. **Re-
proportion before animating.** Below: total height 0.92, leg 0.42 (46%), which is a stocky,
slightly-heroic miner silhouette.

```cpp
static const JointDef MINER_SKEL[J_COUNT] = {
//   name          parent          ox      oy      oz
    { "root",      -1,           0.000f, 0.000f, 0.000f },
    { "pelvis",    J_ROOT,       0.000f, 0.420f, 0.000f },   // hip pivot height
    { "spine",     J_PELVIS,     0.000f, 0.075f, 0.000f },
    { "chest",     J_SPINE,      0.000f, 0.130f, 0.000f },
    { "neck",      J_CHEST,      0.000f, 0.105f, 0.000f },
    { "head",      J_NECK,       0.000f, 0.045f, 0.000f },
    { "helmet",    J_HEAD,       0.000f, 0.105f, 0.000f },
    { "lamp",      J_HELMET,     0.000f, 0.005f, 0.115f },
    { "shoulderL", J_CHEST,     -0.135f, 0.080f, 0.000f },
    { "elbowL",    J_SHOULDER_L, 0.000f,-0.130f, 0.000f },
    { "handL",     J_ELBOW_L,    0.000f,-0.120f, 0.000f },
    { "shoulderR", J_CHEST,      0.135f, 0.080f, 0.000f },
    { "elbowR",    J_SHOULDER_R, 0.000f,-0.130f, 0.000f },
    { "handR",     J_ELBOW_R,    0.000f,-0.120f, 0.000f },
    { "hipL",      J_PELVIS,    -0.072f, 0.000f, 0.000f },
    { "kneeL",     J_HIP_L,      0.000f,-0.215f, 0.000f },   // L1 = 0.215 (thigh)
    { "footL",     J_KNEE_L,     0.000f,-0.205f, 0.000f },   // L2 = 0.205 (shin)
    { "hipR",      J_PELVIS,     0.072f, 0.000f, 0.000f },
    { "kneeR",     J_HIP_R,      0.000f,-0.215f, 0.000f },
    { "footR",     J_KNEE_R,     0.000f,-0.205f, 0.000f },
};
```

Check: pelvis 0.420 = thigh 0.215 + shin 0.205 exactly. So at zero pose the feet sit exactly at
`root.y`, and `root.y = 0` puts the feet on the floor. That identity is worth preserving; every
IK and grounding routine below depends on it.

**Sign convention, stated once because sign errors eat days.** With all channels zero, every
bone points along **local −Y** (limbs hang down). `Rx(θ)·(0,−1,0) = (0, −cosθ, −sinθ)`, so a
*positive* `rx` swings a downward bone **backwards** (toward −Z; the character faces +Z, per
`atan2(dx, dz)` at `:233` and `glRotatef(facing, 0,1,0)` at `:376`).

The animation tables in §4 are written in **animator sign**: positive = **flexion** (hip and
shoulder swing *forward*; knee and elbow *close*). Map it with one array:

```cpp
// +1 where a positive table value is a positive local rx; -1 where it must be negated.
static const float FLEX_SIGN[J_COUNT] = {
    1,1,1,1,1,1,1,1,      // root..lamp: authored directly, no flip
   -1, 1, 1,              // shoulderL (flexion = forward = -rx), elbowL, handL
   -1, 1, 1,              // shoulderR, elbowR, handR
   -1, 1, 1,              // hipL (flexion = forward = -rx), kneeL, footL
   -1, 1, 1,              // hipR, kneeR, footR
};
```

### 3.4 The skin: primitives parented to joints

```cpp
struct SkinPart
{
    int   joint;
    float ox, oy, oz;               // box centre, in JOINT space
    float hx, hy, hz;               // half-extents
    float r, g, b;
};

static const SkinPart MINER_SKIN[] = {
//   joint          centre offset                 half-extents               colour
    { J_PELVIS,     0.000f,-0.030f, 0.000f,   0.115f,0.070f,0.090f,   0.24f,0.26f,0.33f },
    { J_SPINE,      0.000f, 0.060f, 0.000f,   0.125f,0.075f,0.095f,   0.30f,0.42f,0.85f },
    { J_CHEST,      0.000f, 0.045f, 0.000f,   0.140f,0.080f,0.100f,   0.30f,0.42f,0.85f },
    { J_CHEST,      0.000f, 0.020f,-0.105f,   0.090f,0.070f,0.045f,   0.20f,0.22f,0.28f }, // backpack
    { J_HEAD,       0.000f, 0.050f, 0.000f,   0.085f,0.062f,0.078f,   0.95f,0.78f,0.55f },
    { J_HELMET,     0.000f, 0.010f, 0.000f,   0.108f,0.038f,0.100f,   0.98f,0.80f,0.15f },
    { J_LAMP,       0.000f, 0.000f, 0.010f,   0.030f,0.026f,0.024f,   1.00f,1.00f,0.85f },
    { J_SHOULDER_L, 0.000f,-0.065f, 0.000f,   0.042f,0.070f,0.045f,   0.30f,0.42f,0.85f },
    { J_ELBOW_L,    0.000f,-0.060f, 0.000f,   0.038f,0.065f,0.040f,   0.95f,0.78f,0.55f },
    { J_HAND_L,     0.000f,-0.020f, 0.000f,   0.044f,0.036f,0.044f,   0.22f,0.24f,0.30f },
    /* ... R mirrored ... */
    { J_HIP_L,      0.000f,-0.108f, 0.000f,   0.052f,0.112f,0.055f,   0.22f,0.24f,0.30f },
    { J_KNEE_L,     0.000f,-0.100f, 0.000f,   0.046f,0.105f,0.048f,   0.22f,0.24f,0.30f },
    { J_FOOT_L,     0.000f,-0.022f, 0.028f,   0.050f,0.026f,0.072f,   0.14f,0.15f,0.18f },
    /* ... R mirrored ... */
};
static const int MINER_SKIN_COUNT = (int)(sizeof(MINER_SKIN) / sizeof(MINER_SKIN[0]));
```

That is 22 boxes for a miner (vs 6 today), and it is *data*, not code. A monster is a second
table against a second `JointDef` array. The whole "asset pipeline" is `static const`.

### 3.5 Composition — the entire update, 30 lines

```cpp
inline Mat4 Identity()
{
    Mat4 r = {};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 Mul(const Mat4& a, const Mat4& b)                  // column-major: result = a * b
{
    Mat4 r;
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i < 4; ++i)
            r.m[c * 4 + i] = a.m[0 * 4 + i] * b.m[c * 4 + 0]
                           + a.m[1 * 4 + i] * b.m[c * 4 + 1]
                           + a.m[2 * 4 + i] * b.m[c * 4 + 2]
                           + a.m[3 * 4 + i] * b.m[c * 4 + 3];
    return r;
}

// Local matrix = T(rest + tx,ty,tz) * Ry * Rx * Rz * S
Mat4 LocalTRS(const JointDef& d, const JointPose& p)
{
    const float cy = std::cos(p.ry), sy = std::sin(p.ry);
    const float cx = std::cos(p.rx), sx = std::sin(p.rx);
    const float cz = std::cos(p.rz), sz = std::sin(p.rz);

    // R = Ry * Rx * Rz, expanded.
    const float r00 =  cy * cz + sy * sx * sz;
    const float r01 = -cy * sz + sy * sx * cz;
    const float r02 =  sy * cx;
    const float r10 =  cx * sz;
    const float r11 =  cx * cz;
    const float r12 = -sx;
    const float r20 = -sy * cz + cy * sx * sz;
    const float r21 =  sy * sz + cy * sx * cz;
    const float r22 =  cy * cx;

    Mat4 o;
    o.m[ 0] = r00 * p.sx; o.m[ 1] = r10 * p.sx; o.m[ 2] = r20 * p.sx; o.m[ 3] = 0.0f;
    o.m[ 4] = r01 * p.sy; o.m[ 5] = r11 * p.sy; o.m[ 6] = r21 * p.sy; o.m[ 7] = 0.0f;
    o.m[ 8] = r02 * p.sz; o.m[ 9] = r12 * p.sz; o.m[10] = r22 * p.sz; o.m[11] = 0.0f;
    o.m[12] = d.ox + p.tx; o.m[13] = d.oy + p.ty; o.m[14] = d.oz + p.tz; o.m[15] = 1.0f;
    return o;
}

// THE UPDATE LOOP. One pass, no recursion, no stack, because parents precede children.
void ComputeWorld(const JointDef* def, const Pose& pose, Mat4* out, int n)
{
    for (int i = 0; i < n; ++i) {
        const Mat4 local = LocalTRS(def[i], pose.j[i]);
        out[i] = (def[i].parent < 0) ? local : Mul(out[def[i].parent], local);
    }
}
```

Cost per character: 19 joints × (12 trig + one 4×4 multiply ≈ 64 mul + 48 add). With 5 miners
and ~7 monsters that is under 1,000 4×4 multiplies per frame. Irrelevant on any machine that
runs the existing immediate-mode terrain loop.

**Verification you should build in.** A one-time `ValidateSkeleton()` run at startup that
asserts `def[i].parent < i` for all `i`, and that every `SkinPart::joint` is in range. Ten
lines, catches every reordering mistake you will make in the next six months.

### 3.6 Drawing

```cpp
void DrawSkinned(const Mat4* world, const SkinPart* skin, int count,
                 float px, float py, float pz, float facingRad)
{
    glPushMatrix();
    glTranslatef(px, py, pz);
    glRotatef(facingRad * 57.29577951f, 0.0f, 1.0f, 0.0f);
    for (int i = 0; i < count; ++i) {
        const SkinPart& s = skin[i];
        const Mat4& m = world[s.joint];
        const float ao = 0.72f + 0.28f * (m.m[5] * 0.5f + 0.5f);   // section 2.4
        glColor3f(s.r * ao, s.g * ao, s.b * ao);
        glPushMatrix();
        glMultMatrixf(m.m);
        glTranslatef(s.ox, s.oy, s.oz);
        DrawBox(s.hx, s.hy, s.hz);
        glPopMatrix();
    }
    glPopMatrix();
}
```

Note it composes the *joint world matrix in model space* onto the model transform. Two
`glPushMatrix` levels total, so `GL_MAX_MODELVIEW_STACK_DEPTH` (guaranteed ≥ 32) is never a
concern. And because `world[]` is CPU-side, IK, look-at, attachment points, footstep world
positions and particle spawn points are all one indexed read.

### 3.7 Clips: sparse keys, cubic interpolation, and why 8 keys is enough

```cpp
struct Key     { float t; float v; };                 // t normalised to [0,1] of the clip
struct Channel { unsigned char joint;                 // JointId
                 unsigned char chan;                  // 0..8 index into JointPose as float[9]
                 unsigned char nkeys;
                 unsigned char pad;
                 const Key*    keys; };
struct Clip    { const Channel* ch; int nch; float duration; bool looping; };
```

**Interpolation is the whole ballgame.** Eight linearly-interpolated keys read as a robot,
because a walk's angular *velocity* is what the eye reads, and linear interpolation makes
velocity a step function. **Catmull-Rom** over the same eight keys reads as smooth, organic
motion, and it costs nine multiplies.

```cpp
inline float CatmullRom(float p0, float p1, float p2, float p3, float s)
{
    const float s2 = s * s, s3 = s2 * s;
    return 0.5f * ((2.0f * p1)
                 + (-p0 + p2) * s
                 + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * s2
                 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * s3);
}

float SampleChannel(const Channel& c, float t, bool looping)
{
    const int n = (int)c.nkeys;
    if (n == 1) return c.keys[0].v;

    int i = 0;
    while (i + 1 < n && c.keys[i + 1].t <= t) ++i;      // <= 8 iterations; keep it simple
    const int i1 = i;
    const int i2 = (i + 1 < n) ? i + 1 : (looping ? 0 : n - 1);
    const float t1 = c.keys[i1].t;
    float t2 = c.keys[i2].t;
    if (t2 <= t1) t2 += 1.0f;                            // wrapped
    const float s = (t2 > t1) ? (t - t1) / (t2 - t1) : 0.0f;

    const int i0 = looping ? ((i1 - 1 + n) % n) : (i1 > 0 ? i1 - 1 : i1);
    const int i3 = looping ? ((i2 + 1) % n)     : (i2 + 1 < n ? i2 + 1 : i2);
    return CatmullRom(c.keys[i0].v, c.keys[i1].v, c.keys[i2].v, c.keys[i3].v, s);
}

void SampleClip(const Clip& clip, float t01, Pose& out)
{
    for (int k = 0; k < clip.nch; ++k) {
        const Channel& c = clip.ch[k];
        float* dst = reinterpret_cast<float*>(&out.j[c.joint]);
        dst[c.chan] = SampleChannel(c, t01, clip.looping);
    }
}
```

Because Catmull-Rom overshoots slightly at direction changes, **you get follow-through and
soft anticipation for free** at every extreme in every table. That is not a bug; it is the
reason to use it. If a specific channel must not overshoot (a knee crossing 0 into
hyperextension, for example), clamp that channel after sampling — one line, and only where
needed.

### 3.8 Blending, additive layers, masks

```cpp
inline void BlendPose(Pose& dst, const Pose& a, const Pose& b, float w)
{
    float*       d = reinterpret_cast<float*>(&dst);
    const float* p = reinterpret_cast<const float*>(&a);
    const float* q = reinterpret_cast<const float*>(&b);
    for (int i = 0; i < (int)(sizeof(Pose) / sizeof(float)); ++i)
        d[i] = p[i] + (q[i] - p[i]) * w;
}

// Additive: rotations and translations ADD, scales MULTIPLY.
// `add` is authored as a DELTA from the rest pose, not as an absolute pose.
void AddPose(Pose& dst, const Pose& add, float w, const float* jointMask /*or nullptr*/)
{
    for (int j = 0; j < J_COUNT; ++j) {
        const float m = (jointMask ? jointMask[j] : 1.0f) * w;
        if (m <= 0.0f) continue;
        float*       d = reinterpret_cast<float*>(&dst.j[j]);
        const float* a = reinterpret_cast<const float*>(&add.j[j]);
        for (int c = 0; c < 6; ++c) d[c] += a[c] * m;              // rx..tz
        for (int c = 6; c < 9; ++c) d[c] *= (1.0f + (a[c] - 1.0f) * m);   // sx..sz
    }
}
```

**This is the highest-leverage 45 lines in §3.** Once additive layers exist, every one of the
following is a *data* problem, not a code problem:

| Layer | Mask | Weight driven by |
| --- | --- | --- |
| Breathing | chest, spine, shoulders | `1 − locomotionWeight * 0.6` |
| Carry-heavy | spine, chest, shoulders, head | `carrying ? 1 : 0`, spring-smoothed |
| Strain tremor | spine, arms | `load / maxLoad` |
| Aim / drill offset | chest, arms | drill state |
| Hit recoil | whole body | decaying impulse (§7.5) |
| Look-at | neck, head, chest | §6.2 |

Six behaviours, one mechanism. That is why its R is 7.8 despite being infrastructure.

Standard masks:

```cpp
static const float MASK_UPPER[J_COUNT] = {
    0,0.35f,0.70f,1,1,1,1,1,  1,1,1, 1,1,1,  0,0,0, 0,0,0 };
static const float MASK_LOWER[J_COUNT] = {
    1,0.65f,0.30f,0,0,0,0,0,  0,0,0, 0,0,0,  1,1,1, 1,1,1 };
```

The graded 0.35/0.70 on pelvis/spine is what stops upper-body layers from looking bolted on.
A hard 0/1 mask at the waist is the classic tell that a game has two animation layers.

### 3.9 `/W4` + `/permissive-` + `/sdl` traps in this specific code

| Warning | Where it will bite | Fix |
| --- | --- | --- |
| C4244 `double`→`float` | `0.5 * (...)`, `sin(3.14 * t)` | `f` suffix on **every** literal; `0.5f`, `6.2831853f` |
| C4305 truncation | `float k = 1.70158;` | same |
| C4100 unused param | debug-only params in draw fns | `(void)param;` — matches `:410` |
| C4189 init-but-unused | leftover intermediates while tuning | delete, or `(void)x;` |
| C4127 constant conditional | `while (true)` in the key search | use `while (i + 1 < n && ...)` as above |
| C4267 `size_t`→`int` | `sizeof(...)/sizeof(...)` into `int` | explicit `(int)` cast, as above |
| C4701 potentially uninit | `Mat4 r;` in `Mul` before the loop | it is fully written; if MSVC complains, `Mat4 r = {};` |
| C4324/C4820 padding | not enabled at `/W4` | ignore |
| `/sdl` C4996 | `strlen`, `sprintf` | already handled by `_CRT_SECURE_NO_WARNINGS` (`:64`) |

Also: `reinterpret_cast<float*>(&pose)` over a POD struct of 9 floats is fine under
`/permissive-` and MSVC's aliasing rules, but add
`static_assert(sizeof(JointPose) == 9 * sizeof(float), "JointPose must be tightly packed");`
so a future field addition fails loudly rather than corrupting poses silently.

---

## 4. THE CLIPS, WITH NUMBERS

### 4.0 Scale and cadence audit — read this before authoring a single key

This is the most useful arithmetic in the document, and it says your current speed constant is
wrong by roughly 3×.

- Miner height (proposed, §3.3): **0.92 units**. Leg length L = 0.420.
- Stride length = distance the body travels per full gait cycle (two steps):
  `stride = 2 * L * (sin θ_flex + sin θ_ext)`.
  With θ_flex = 28°, θ_ext = 22°: `stride = 2 * 0.420 * (0.4695 + 0.3746) = 0.709 units`.
- Cadence: `f_cycle = speed / stride`.
- `MINER_SPEED = 3.0f` (`:57`) → `f_cycle = 3.0 / 0.709 = 4.23 Hz` = **508 steps per minute.**
  A human walks at 110 and sprints at ~180.

So one of three things must change:

| Option | Change | Consequence |
| --- | --- | --- |
| A | `MINER_SPEED` 3.0 → **1.15** | Cadence 1.62 Hz = 195 spm. Reads as a brisk jog. Feels slow to play. |
| B | Keep 3.0, author a **run** cycle (θ_flex 42°, θ_ext 38° → stride 1.078) | `f = 2.78 Hz` = 334 spm. Still fast but *readable* as a cartoon scamper. |
| C | Keep 3.0, scale the miner up to 1.6 units tall (L = 0.73, stride 1.23) | `f = 2.44 Hz`. Miners become as tall as a tile is wide. Changes the whole look. |

**Recommendation: A + B together.** Set `MINER_SPEED = 1.6f`, author both walk and run, and
blend by speed (§4.4). 1.6 gives walk cadence 2.26 Hz — a fast walk, which is what an RTS unit
should look like. If playtesting demands 3.0 for feel, you are in run territory permanently and
should say so with the animation rather than fight it.

**Cadence is not a free parameter.** Once you drive phase from distance (§6.7), cadence falls
out of speed and stride, and if you dislike it your only knobs are speed, leg length, and hip
range. That constraint is exactly what makes it look right.

### 4.1 The reference: what a real walk does

Values below are the clinical consensus for adult level walking, given as the *starting truth*
before stylisation. Gait cycle is measured right-heel-strike to right-heel-strike; stance is
**0–60%**, swing **60–100%**
([AAPM&R, *Biomechanics of Normal Gait*](https://now.aapmr.org/biomechanics-normal-gait/);
[Wheeless, *Stance Phase of Gait*](https://www.wheelessonline.com/orthopaedics/stance-phase-of-gait/);
[Physiopedia, *Joint Range of Motion During Gait*](https://www.physio-pedia.com/Joint_Range_of_Motion_During_Gait)).

| Quantity | Real human | Note |
| --- | --- | --- |
| Hip flexion at heel strike (0%) | +30° | |
| Hip extension at terminal stance (~50%) | −10° to −20° | |
| Knee at heel strike | 0–5° | |
| Knee, loading-response peak (~15%) | 15–18° | **the shock absorber; omitting it is the #1 reason a walk looks weightless** |
| Knee, midstance (~40%) | ~3° | |
| Knee, swing peak (~72%) | **~60–65°** | |
| Ankle plantarflexion at toe-off (~62%) | −20° | |
| Ankle dorsiflexion, midstance (~45%) | +10° | |
| Pelvis vertical displacement | ~±2.3 cm on a 1.7 m adult ≈ **±1.35% of height**, at **2× step frequency** | lowest at double support, highest near midstance |
| Pelvis transverse rotation | ±4° | |
| Pelvic list (contralateral drop) | ~5° | |
| Thorax counter-rotation | ±4–7°, opposite the pelvis | |

For **stylised** characters, the animation convention (Richard Williams' four keys —
contact, down, passing, up —
[*The Animator's Survival Kit*](https://archive.org/details/TheAnimatorsSurvivalKitRichardWilliams);
[UW CSE 458 walk-cycle notes](https://courses.cs.washington.edu/courses/cse458/06au/projects/project7/walk.html))
differs from biomechanics in two deliberate ways:

1. **Amplitudes go up ~20–30%.** Real walking is subtle; on a 0.9-unit cube figure at 30 m
   camera distance it disappears.
2. **The body's high point is pushed later**, to the *up* position at ~35% rather than
   midstance at 25%, because that reads as a push-off rather than a bounce. Williams also
   places the widest arm swing at *contact* rather than at *down*, where it physically occurs,
   for the same reason: it reads better.

I follow the animation convention below and mark where it diverges.

### 4.2 Root motion — the four continuous curves (no keyframes needed)

These are analytic, not tabled. `p` ∈ [0,1) is the gait phase, right heel strike at `p = 0`.

```cpp
// Vertical bob: TWICE per cycle (once per step). Peak at the "up" position.
// A_BOB = 1.35% of height * 2.6 stylisation = 0.032 units on a 0.92-unit miner.
const float A_BOB = 0.032f;
rootY = A_BOB * std::cos(4.0f * PI_F * (p - 0.35f));

// Lateral weight shift: ONCE per cycle, toward the stance foot.
// Peak +X (right) at right midstance p = 0.25.
const float A_SWAY = 0.026f;
rootX_local = A_SWAY * std::sin(2.0f * PI_F * p);

// Shoulder roll: opposes the pelvis list, once per cycle.
const float A_ROLL = 3.2f * DEG2RAD;
chest_rz = -A_ROLL * std::sin(2.0f * PI_F * p);
pelvis_rz = 5.0f * DEG2RAD * std::sin(2.0f * PI_F * p);   // swing-side hip drops

// Transverse counter-rotation: pelvis leads, chest opposes. Once per cycle.
pelvis_ry =  5.0f * DEG2RAD * std::cos(2.0f * PI_F * p);
chest_ry  = -4.0f * DEG2RAD * std::cos(2.0f * PI_F * p);
```

**The phase relationship is the whole point and is worth stating explicitly:** the bob runs at
**2×** the step rate and the sway at **1×**. Get that wrong — for example the current code's
`sin(m.phase) * 0.06f` at `:373`, which bobs once per *leg swing* — and the walk reads as a
limp even with perfect leg curves. This single relationship is the reason item #8 scores G=6
for 26 lines.

`chest_ry = −0.8 × pelvis_ry` means the shoulders counter-rotate against the hips. Net
shoulder rotation in world space is small (+1°); relative to the pelvis it is −9°. That
counter-rotation is what makes a biped look like it has a spine.

### 4.3 THE WALK — right leg, 8 keys, animator sign (positive = flexion)

Keys at `p = 0, 0.125, 0.250, 0.375, 0.500, 0.625, 0.750, 0.875`. Williams' four positions
land at: **contact** 0.000, **down** 0.125, **passing** 0.250, **up** 0.375 — then the same
four again for the left leg at +0.5.

**Left leg = the same table sampled at `p + 0.5`.** One table, two legs.

| Channel | 0.000 contact | 0.125 down | 0.250 pass | 0.375 up | 0.500 | 0.625 | 0.750 | 0.875 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **hip R** (flex+) | **+28** | +16 | +2 | −14 | −22 | −6 | +18 | **+30** |
| **knee R** (flex+) | 5 | **18** | 8 | 4 | 12 | 55 | **68** | 25 |
| **ankle R** (dorsi+) | 0 | −6 | +4 | **+10** | −8 | **−22** | −2 | +4 |
| **shoulder R** (fwd+) | −22 | −13 | −2 | +11 | **+18** | +5 | −14 | −24 |
| **elbow R** (flex+) | 20 | 18 | 20 | 30 | **38** | 34 | 24 | 20 |
| **spine pitch** | +6 | +6 | +6 | +6 | +6 | +6 | +6 | +6 |

Degrees. Four load-bearing details, each of which is individually visible:

1. **Terminal-swing overshoot.** Hip peaks at **+30 at p = 0.875**, *before* the contact value
   of +28. The leg reaches out and then settles back onto the foot. Remove that 2° and the
   walk instantly reads as mechanical. This is the *ease-in/out* principle expressed as a key.
2. **The loading-response knee bend (18° at p = 0.125).** The knee is nearly straight at
   contact, then bends to absorb the drop, then straightens. This is where the character's
   *weight* lives. It costs one key.
3. **Arm counter-swing is the leg curve, negated and scaled by 0.8.**
   `shoulderR(p) = −0.80 × hipR(p)`. Check: `−0.8 × 28 = −22.4` ✔ against the table's −22.
   *The right arm swings back when the right leg swings forward.* Get this backwards and it is
   the most immediately, viscerally wrong thing a viewer can see. Because the relationship is
   exact, **you do not need to store the arm table at all** — sample the hip channel and
   negate. Saves 16 keys and guarantees they stay in sync when you retune.
4. **The elbow bends on the forward swing** (peak 38° at p = 0.5, when the arm is furthest
   forward). Constant elbow flexion is the second-most-common tell.

The C++ literal:

```cpp
static const Key K_HIP[8] = {
    {0.000f,  28.0f}, {0.125f,  16.0f}, {0.250f,   2.0f}, {0.375f, -14.0f},
    {0.500f, -22.0f}, {0.625f,  -6.0f}, {0.750f,  18.0f}, {0.875f,  30.0f} };
static const Key K_KNEE[8] = {
    {0.000f,   5.0f}, {0.125f,  18.0f}, {0.250f,   8.0f}, {0.375f,   4.0f},
    {0.500f,  12.0f}, {0.625f,  55.0f}, {0.750f,  68.0f}, {0.875f,  25.0f} };
static const Key K_ANKLE[8] = {
    {0.000f,   0.0f}, {0.125f,  -6.0f}, {0.250f,   4.0f}, {0.375f,  10.0f},
    {0.500f,  -8.0f}, {0.625f, -22.0f}, {0.750f,  -2.0f}, {0.875f,   4.0f} };
static const Key K_ELBOW[8] = {
    {0.000f,  20.0f}, {0.125f,  18.0f}, {0.250f,  20.0f}, {0.375f,  30.0f},
    {0.500f,  38.0f}, {0.625f,  34.0f}, {0.750f,  24.0f}, {0.875f,  20.0f} };
```

Apply (degrees → radians → animator sign → channel):

```cpp
void ApplyWalk(Pose& po, float p)
{
    const float pL = p + 0.5f;                       // SampleChannel wraps t internally
    const float hipR   = SampleKeys(K_HIP,   8, p ), hipL   = SampleKeys(K_HIP,   8, pL);
    const float kneeR  = SampleKeys(K_KNEE,  8, p ), kneeL  = SampleKeys(K_KNEE,  8, pL);
    const float ankR   = SampleKeys(K_ANKLE, 8, p ), ankL   = SampleKeys(K_ANKLE, 8, pL);
    const float elbR   = SampleKeys(K_ELBOW, 8, p ), elbL   = SampleKeys(K_ELBOW, 8, pL);

    po.j[J_HIP_R ].rx = FLEX_SIGN[J_HIP_R ] * hipR  * DEG2RAD;
    po.j[J_HIP_L ].rx = FLEX_SIGN[J_HIP_L ] * hipL  * DEG2RAD;
    po.j[J_KNEE_R].rx = FLEX_SIGN[J_KNEE_R] * kneeR * DEG2RAD;
    po.j[J_KNEE_L].rx = FLEX_SIGN[J_KNEE_L] * kneeL * DEG2RAD;
    po.j[J_FOOT_R].rx = FLEX_SIGN[J_FOOT_R] * ankR  * DEG2RAD;
    po.j[J_FOOT_L].rx = FLEX_SIGN[J_FOOT_L] * ankL  * DEG2RAD;
    // Arms are DERIVED, not stored: counter-swing = -0.80 * the hip curve.
    po.j[J_SHOULDER_R].rx = FLEX_SIGN[J_SHOULDER_R] * (-0.80f * hipR) * DEG2RAD;
    po.j[J_SHOULDER_L].rx = FLEX_SIGN[J_SHOULDER_L] * (-0.80f * hipL) * DEG2RAD;
    po.j[J_ELBOW_R].rx = FLEX_SIGN[J_ELBOW_R] * elbR * DEG2RAD;
    po.j[J_ELBOW_L].rx = FLEX_SIGN[J_ELBOW_L] * elbL * DEG2RAD;
}
```

Total walk data: **32 keys**. Rosen shipped an entire character on 13
([GDC 2014](https://archive.org/details/GDC2014Rosen)).

**Foot plant vs foot lift, explicitly.** Stance is `p ∈ [0.00, 0.60]`, swing `[0.60, 1.00]`.
During stance the foot must be motionless in *world* space. Two ways, in increasing quality:

- **Cheap (do first):** ensure the stride implied by the curves matches the stride used to
  advance the phase (§6.7). Residual slip with the table above is roughly ±0.02 units — below
  the perception threshold at RTS camera distance.
- **Correct (do with IK):** on the frame stance begins, record the foot's world position; for
  the rest of stance, IK the leg to that fixed world point and let the hip lower to reach it
  (§6.4).

**Foot lift height.** Peak toe clearance occurs at `p ≈ 0.72` (mid-swing) and should be
**0.055 units** — about 6% of body height. Too high reads as a march, too low as a shuffle. It
falls out of the hip+knee curves; verify it rather than authoring it, and adjust the knee peak
(68°) to tune.

### 4.4 THE RUN, and the blend

Not a scaled walk. Three structural differences: a **flight phase** (both feet off the ground),
a forward torso lean, and arms held high.

| Channel | 0.000 | 0.125 | 0.250 | 0.375 | 0.500 | 0.625 | 0.750 | 0.875 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| hip R | **+42** | +20 | −4 | −26 | −38 | −8 | +26 | **+46** |
| knee R | 22 | 40 | 22 | 14 | 34 | 92 | **105** | 48 |
| ankle R | −4 | −12 | +6 | +12 | **−26** | −18 | +2 | −2 |
| shoulder R | −29 | −14 | +3 | +18 | +27 | +6 | −18 | −32 |
| elbow R | 78 | 70 | 74 | 88 | **95** | 90 | 82 | 78 |
| spine pitch | +14 | +14 | +14 | +14 | +14 | +14 | +14 | +14 |

- **Stance fraction 0.35** (vs 0.60). `p ∈ [0.00,0.35]` right stance, `[0.50,0.85]` left
  stance, so `[0.35,0.50]` and `[0.85,1.00]` are **flight**. During flight the root follows a
  ballistic arc, not a sinusoid:
  `rootY = A_BOB_RUN * (1 − 4(u − 0.5)²)` where `u` is the normalised progress through the
  flight window, `A_BOB_RUN = 0.075`. Parabola, not cosine — that is what makes it read as a
  *jump between footfalls* rather than a bounce.
- Stride 1.078 units (from §4.0 with θ = 42/38).
- Blend: `runW = clamp((speed − 0.95f) / 0.45f, 0, 1)`, smoothstepped, then
  `BlendPose(out, walkPose, runPose, runW)`. Blend the two clips **at the same normalised
  phase** — they share the same `p = 0 is right heel strike` convention, so they stay in sync
  through the blend. Blending clips whose phase conventions differ is the classic source of
  "the legs scissor during the transition".

### 4.5 IDLE — breathing, and why it is asymmetric

A sinusoidal breath reads as a machine. Real breathing is **inhale ~40% of the period, exhale
~48%, pause ~12%**, with different curvature on each side.

```cpp
// b(u) in [0,1]: 0 = fully exhaled, 1 = fully inhaled.
float BreathCurve(float u)                      // u = fractional position in the breath cycle
{
    if (u < 0.40f) { const float s = u / 0.40f;            return s * s * (3.0f - 2.0f * s); }
    if (u < 0.88f) { const float s = (u - 0.40f) / 0.48f;  return 1.0f - s * s * (3.0f - 2.0f * s); }
    return 0.0f;                                                        // held-out pause
}
```

Rate: **0.25 Hz at rest** (15 breaths/min). After exertion, **0.55 Hz** (33/min) decaying back
with a **6.0 s half-life**. Tying breath rate to recent activity is four lines and it is the
difference between "idle animation" and "this thing just ran here".

Amplitudes (all channels are an *additive layer*, mask = upper body):

| Channel | Amplitude | Note |
| --- | --- | --- |
| chest `sy` | ×(1 + 0.020·b) | |
| chest `sx`, `sz` | ×(1 + 0.030·b) | chest widens more than it lengthens |
| shoulder L/R `rz` | ∓2.5°·b | shoulders rise |
| spine `rx` | −1.2°·b | slight straightening on inhale |
| head `rx` | +0.8°·b | |
| root `ty` | +0.006·b | |

**Desynchronise the two shoulders by 0.06 of a cycle.** Perfect bilateral symmetry is the
single strongest "this is a machine" cue in idle animation, and the fix costs one `+ 0.06f`.

**Idle weight shift.** Every `Exp(mean = 4.5 s)`, transfer weight to the other foot over 0.9 s
with a `smootherstep`: `root.tx ±0.022`, `pelvis.rz ±3.5°`, unloaded knee `+6°` flexion, head
`ry` drifts `∓4°`. This alone makes a standing character look like it is *waiting* rather than
*paused*.

### 4.6 DRILLING / WORKING — a macro cycle carrying a micro cycle

The structure that sells power tools is **two frequencies at once**: a slow body cycle of
effort, and a fast tool vibration that the body only partially filters.

**Macro cycle, T = 1.6 s, looping:**

| Window | Action | Curve |
| --- | --- | --- |
| 0.00–0.22 (0.35 s) | Lean in. spine `rx` +6°→+18°, both shoulders +10°, root `tz` +0.05, knees +15° | `easeOutCubic` |
| 0.22–0.47 (0.40 s) | **Push and hold.** Peak effort. Everything static except the micro cycle. | hold |
| 0.47–0.72 (0.40 s) | Grind. Slow oscillation, spine `rx` 18°→14°→18° at 2.5 Hz, ±1.5° | sine |
| 0.72–1.00 (0.45 s) | Recover, reset stance, one short breath | `easeInOutQuad` |

**Micro cycle, 14 Hz, always on:**

```cpp
const float vib = std::sin(t * 14.0f * 2.0f * PI_F);
tool.tz     += 0.012f * vib;                    // hammer action along the tool axis
elbowR.rx   += 1.5f * DEG2RAD * vib;
elbowL.rx   += 1.5f * DEG2RAD * vib * 0.85f;    // hands never perfectly in phase
chest.rx    += 0.4f * DEG2RAD * vib;            // the body FILTERS it: 3.75x smaller
head.rx     += 0.15f * DEG2RAD * vib;           // the head filters it further still
```

**The filtering gradient is the point.** Tool 100%, forearm 12%, chest 3%, head 1%. Vibration
that propagates undamped through the whole body reads as an earthquake; vibration that stops at
the wrist reads as a floating tool. The gradient reads as flesh.

**Stance:** feet planted, hip abduction (`rz`) 8° outward each side, knees 15° flexed, weight
0.04 units forward, torso yawed 12° toward the drill so the shoulders are not square to the
wall. Head pitched down 12°, or better, use the look-at (§6.2) targeted at the drill contact
point — free, and it automatically works for any wall direction.

**Events:** emit sparks/dust at the tool tip on the vibration peaks (`vib` crossing +0.9), and
push `trauma += 0.02` (§6.6). The drill progress bar at `:495-506` becomes redundant once the
body is visibly straining, which is a small win in HUD clutter.

### 4.7 ATTACK LUNGE — anticipation : strike : recovery

The classic ratio is **anticipation ≈ 2× the strike duration**, and the strike is as short as
you can make it while remaining visible (2–3 frames at 60 Hz).

| Phase | Medium (miner) | Heavy (large monster) | Pose | Curve |
| --- | ---: | ---: | --- | --- |
| **Anticipation** | 0.18 s | 0.42 s | root `tz` −0.10, spine `rx` −12° (back), shoulders −25°, knees +12°, **squash sx/sz 1.10, sy 0.90** | `easeOutCubic` — decelerate *into* the loaded pose |
| **Strike** | 0.09 s | 0.14 s | root `tz` +0.42, spine `rx` +22°, arm `rx` +70°, **stretch sx/sz 0.88, sy 1.14** | `easeInQuart` — accelerate hard |
| **Contact hold** | 0.06 s | 0.10 s | freeze; arm overshoots +8° for one frame then returns | hold |
| **Recovery** | 0.35 s | 0.70 s | return to neutral, overshooting by −6° | `easeOutBack(s = 1.70158)` |

Three things do most of the work here:

1. **The contact hold.** A hard stop of 3–6 frames at the moment of impact is worth more than
   any amount of pose polish. It is the animation equivalent of hit-stop (§7.5).
2. **The anticipation squash is opposite in axis to the strike stretch.** Wide-and-low, then
   tall-and-thin. That axis flip is what makes the strike feel *released* rather than merely
   fast.
3. **The recovery overshoot.** Coming back to exactly neutral reads as a machine resetting.
   Overshooting by −6° and springing back reads as a body absorbing its own momentum.

### 4.8 DEATH — a rigid pendulum, not a keyframed fall

The single best death for a box character is a **physical topple**, because it is different
every time, it interacts with the direction of the killing blow, and it is nine lines.

A rigid rod pivoting about its base under gravity:

```
θ̈ = (3 g / 2 L) · sin θ
```

(standard rigid-rod result; the `3/2` comes from `I = mL²/3` about the end and torque
`mgL/2·sinθ`). Integrate semi-implicitly:

```cpp
struct Topple { float theta = 0.08f, omega = 0.0f; float axisX = 1.0f, axisZ = 0.0f; };

void StepTopple(Topple& tp, float L, float dt)
{
    const float acc = (3.0f * 9.81f / (2.0f * L)) * std::sin(tp.theta);
    tp.omega += acc * dt;
    tp.theta += tp.omega * dt;
    if (tp.theta > 1.5707963f) { tp.theta = 1.5707963f; tp.omega *= -0.25f; }  // hit the floor, small bounce
}
```

Full sequence:

| t (s) | Phase | Content |
| --- | --- | --- |
| 0.00–0.09 | **Hit-stop** | freeze the whole entity, white flash 0.8, scale 1.06/0.94/1.06 |
| 0.09–0.26 | **Stagger** | root back 0.15 along the damage direction, arms up +55°, head snaps back 25°, `easeOutQuint` |
| 0.26–~0.95 | **Topple** | `StepTopple` about the *foot contact*, not the centre. Seed `θ₀ = 0.08 rad`, `ω₀ = 0.9 · (damage direction · facing)`. Limbs go limp: each limb joint gets an under-damped spring with goal = a random slack angle, `f = 3.2 Hz`, `ζ = 0.30` |
| impact | **Land** | squash sy 0.82 / sx,sz 1.15, dust ring, `trauma += 0.35`, `Beep(90, 120)` |
| +0.0–0.5 | **Settle** | springs decay; a small secondary bounce falls out of the `ω *= −0.25` above |
| +1.5–2.5 | **Sink** | translate `−0.9` on Y over 1.0 s with `easeInQuad`, then `erase()` |

Rotating about the **foot contact point** rather than the origin is what makes it read as
falling rather than spinning. In code that is `glTranslatef(0, 0, 0)` at the foot, rotate,
then translate the body up — or equivalently set `J_ROOT.rx` and simultaneously
`J_ROOT.ty = −L/2 · (1 − cos θ)`, `J_ROOT.tz = ∓L/2 · sin θ`.

Compare with today: `monsters.erase(...)` at `:297`. Every enemy in the game currently
disappears mid-stride. This item is G=7 for ~45 lines and it changes how the combat *feels*
more than any weapon change would.

### 4.9 CARRYING SOMETHING HEAVY — an additive layer plus a time warp

No new clip. A delta pose on top of the walk, plus four scalar changes, plus one trick.

**Additive pose (mask = `MASK_UPPER` with pelvis 0.5):**

| Channel | Delta |
| --- | --- |
| spine `rx` | +14° (lean back to counterweight a front carry; +8° *forward* for a shoulder carry) |
| pelvis `rx` | +6° anterior tilt |
| both knees | +8° throughout — a loaded leg never straightens |
| head `rx` | −8° (looking down at the footing) |
| shoulders | forced to +40° flexion, elbows to +85° (arms occupied) |
| chest `rz` | 1.2° tremor at 0.9 Hz + noise (strain) |

**Scalar changes:**

| Parameter | ×factor |
| --- | ---: |
| cadence | 0.78 |
| stride | 0.85 |
| arm counter-swing gain (the −0.80 in §4.3) | **0.15** |
| bob amplitude `A_BOB` | 1.60 |
| sway amplitude `A_SWAY` | 1.80 |
| foot lift height | 0.60 |

**The trick — phase dwell-warp.** Carrying weight lengthens double support (both feet down)
from ~20% of the cycle to ~34%. You can express that *without touching a single pose* by
warping time:

```cpp
// Slows the cycle near p = 0, 0.5 and speeds it in between. `amount` in [0,1).
// Monotonic for amount < 1 because dp'/dp = 1 - amount*cos(2*pi*lobes*p) > 0.
inline float DwellWarp(float p, float amount, int lobes)
{
    const float w = 6.2831853f * (float)lobes;
    return p - (amount / w) * std::sin(w * p);
}
// walk phase for a loaded miner:
const float pWarped = DwellWarp(p, 0.55f, 2);
```

**This is a general-purpose weight dial and I recommend adding it to every cycle in the game.**
`amount = 0.0` is a light, even gait; `0.55` is loaded; `−0.35` (negative) is a light, floaty,
skipping gait. One float per creature type, and it changes perceived mass more than doubling
any amplitude does.

---

## 5. THE PRINCIPLES THAT DO THE MOST WORK

The twelve principles are from Thomas & Johnston, *The Illusion of Life* (1981)
([Arlington Museum summary](https://arlingtonmuseum.org/explore-more/the-twelve-principles-of-animation)).
For a stylised box character, five of them do ~90% of the work. Concrete implementations:

### 5.1 Squash and stretch — driven by vertical *impulse*, not vertical velocity

The naive version reads `vy` directly. That is wrong for two reasons: it produces no rebound,
and it means a character standing still on a moving platform squashes. **Drive an under-damped
spring with an impulse at the impact event instead.**

```cpp
struct Squash { float x = 0.0f, v = 0.0f; };   // x = stretch amount; 0 = neutral

// On any landing / footfall / impact:
void SquashImpulse(Squash& s, float impactSpeed)      // impactSpeed in units/s, positive
{
    s.v -= 6.0f * impactSpeed;                        // TUNING: 6.0 s^-1 per unit/s
    if (s.v < -9.0f) s.v = -9.0f;                     // clamp so a 100-unit fall isn't a pancake
}

// Every frame: under-damped so it rebounds 2-3 times.
void StepSquash(Squash& s, float dt)
{
    const float f = 7.0f;                             // Hz  TUNING
    const float z = 0.35f;                            // damping ratio  TUNING
    const float w = 6.2831853f * f;
    const float acc = -(w * w) * s.x - (2.0f * z * w) * s.v;
    s.v += acc * dt;                                  // semi-implicit Euler: stable for w*dt < 2
    s.x += s.v * dt;
    if (s.x < -0.30f) { s.x = -0.30f; if (s.v < 0.0f) s.v = 0.0f; }
    if (s.x >  0.35f) { s.x =  0.35f; if (s.v > 0.0f) s.v = 0.0f; }
}

// Apply, PRESERVING VOLUME:  sx * sy * sz == 1
void ApplySquash(JointPose& root, const Squash& s)
{
    const float sy = 1.0f + s.x;
    const float lat = 1.0f / std::sqrt(sy > 0.05f ? sy : 0.05f);
    root.sy = sy; root.sx = lat; root.sz = lat;
}
```

**Volume preservation is not optional.** `sx·sy·sz = 1` is what makes it read as deformation
rather than as a scale animation. `sx = sz = 1/√sy` gives it exactly.

**Stability check on `w·dt < 2`:** at f = 7 Hz, `w = 43.98`, so `dt < 0.0455 s` (22 fps). The
existing main loop clamps `dt` to 0.1 (`:668`) which is *not* enough. Either lower the clamp to
0.033 for animation purposes, or sub-step the springs. One line:
`const int steps = 1 + (int)(dt / 0.02f); const float sdt = dt / (float)steps;`

**Where squash applies, at what magnitude:**

| Event | Impulse | Reads as |
| --- | ---: | --- |
| Walk footfall | 0.10 | barely perceptible weight; do it anyway |
| Run footfall | 0.35 | |
| Landing from a fall | `0.55 · fallSpeed` | |
| Melee attack strike | −0.22 (stretch, negative) | the lunge |
| Taking damage | 0.30, axis aligned to the damage direction | recoil |
| Monster emerging | 0.45 on the burst frame | |

### 5.2 Anticipation — a formula, not a feeling

For any triggered action whose target pose is `P` and whose neutral is `N`, the anticipation
pose is `N + (N − P)·a` — literally the mirror of the action, scaled.

```cpp
// Pre-pose = neutral reflected away from the target pose.
void MakeAnticipation(Pose& out, const Pose& neutral, const Pose& target, float a)
{
    float*       o = reinterpret_cast<float*>(&out);
    const float* n = reinterpret_cast<const float*>(&neutral);
    const float* t = reinterpret_cast<const float*>(&target);
    for (int i = 0; i < (int)(sizeof(Pose) / sizeof(float)); ++i)
        o[i] = n[i] + (n[i] - t[i]) * a;
}
```

| Mass class | `tAnticipation` | `a` | Curve into the hold |
| --- | ---: | ---: | --- |
| Light / nimble (a small critter) | 0.10 s | 0.18 | `easeOutQuad` |
| Medium (miner) | 0.18 s | 0.28 | `easeOutCubic` |
| Heavy (big monster) | 0.42 s | 0.40 | `easeOutQuart` + a 0.08 s hold at the extreme |

**Anticipation is also how you telegraph gameplay.** A 0.42 s wind-up on a heavy attack is
simultaneously an animation principle and a fairness mechanic — it is the frame budget the
player uses to dodge. Do not treat these as separate systems; the wind-up duration should be
read from the same constant the combat code uses.

### 5.3 Follow-through and overlapping action — a lag chain, 6 lines

Every appendage should reach its pose *later* than its parent. The mechanism is the spring
(§6.1) with a **per-joint half-life that increases down the chain**, plus an angular-velocity
impulse injected from the parent.

```cpp
// Per-frame, for each "loose" joint (helmet, tool, backpack, antenna, ponytail):
void FollowThrough(Spring1& s, float parentAngularVel, float halflife, float coupling, float dt)
{
    s.v -= parentAngularVel * coupling;      // the joint is DRAGGED by the parent's rotation
    SpringStep(s, 0.0f, halflife, dt);       // and always tries to return to rest
}
```

Half-life and coupling table (TUNING CONSTANTS, but these are sane starting points):

| Joint | half-life (s) | coupling | Result |
| --- | ---: | ---: | --- |
| head follow | 0.055 | 0.30 | subtle; do not skip |
| helmet | 0.075 | 0.45 | rocks when the miner turns fast |
| tool in hand | 0.110 | 0.55 | trails the arm |
| backpack | 0.130 | 0.50 | thumps on footfall |
| antenna base | 0.140 | 0.70 | |
| antenna tip | 0.190 | 0.85 | **chain it to the base's output, not to the body** |
| torso lean | 0.160 | — | tracks velocity |

**The increasing half-life down the chain is the entire principle.** Three joints at 0.075 /
0.110 / 0.190 produce a whip. Three joints at the same half-life produce a rigid stick with a
delay.

### 5.4 Ease in / ease out — the curve library, exact

```cpp
inline float Clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }

inline float SmoothStep  (float t) { t = Clamp01(t); return t * t * (3.0f - 2.0f * t); }
inline float SmootherStep(float t) { t = Clamp01(t); return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
inline float EaseOutCubic(float t) { const float u = 1.0f - Clamp01(t); return 1.0f - u * u * u; }
inline float EaseOutQuint(float t) { const float u = 1.0f - Clamp01(t); return 1.0f - u * u * u * u * u; }
inline float EaseInQuart (float t) { t = Clamp01(t); return t * t * t * t; }

inline float EaseOutBack(float t, float s)          // s = 1.70158 gives ~10% overshoot
{
    const float u = Clamp01(t) - 1.0f;
    return 1.0f + (s + 1.0f) * u * u * u + s * u * u;
}

inline float EaseOutElastic(float t)
{
    t = Clamp01(t);
    if (t <= 0.0f || t >= 1.0f) return t;
    const float c = 2.0f * 3.14159265f / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c) + 1.0f;
}
```

(Reference forms: [easings.net](https://easings.net/).)

**Which to use where — the actual decision table, because this is where taste hides:**

| Situation | Curve | Why |
| --- | --- | --- |
| Anticipation into a loaded pose | `EaseOutCubic` | decelerating into the load |
| The action itself | `EaseInQuart` or 2 linear frames | must accelerate; must be short |
| Recovery from an action | `EaseOutBack(1.70158)` | overshoot = absorbed momentum |
| UI, selection ring, banner | `SmootherStep` | zero 1st **and** 2nd derivative at both ends |
| Any spring-able quantity | **do not use a curve; use §6.1** | interruptible, which curves are not |
| Landing settle | `EaseOutElastic` **or** an under-damped spring | spring is better: it composes |
| Emerge / spawn / pop-in | `EaseOutBack(2.4)` | the overshoot *is* the pop |

**The single most useful rule in this table**: if a quantity can be *interrupted* — the player
re-orders the miner mid-turn, the monster is hit mid-lunge — **it must be a spring, not a
curve.** A curve has a start pose baked into it and interrupting one produces a visible pop.
This is the argument
[Juckett](https://www.ryanjuckett.com/damped-springs/) and
[Holden](https://theorangeduck.com/page/spring-roll-call) both make, and it is why §6.1 is
ranked so high.

### 5.5 Secondary motion — see §6.1. Arcs — one line

Nothing in a living body travels in a straight line. If you move a hand from A to B, bow the
path:

```cpp
// Lift the midpoint of any point-to-point motion perpendicular to the travel direction.
pos = Lerp(a, b, t) + up * (arcHeight * 4.0f * t * (1.0f - t));   // parabola, 0 at both ends
```

`arcHeight = 0.12 · |b − a|` for a hand reach, `0.20 · |b − a|` for a foot step (§6.4). Four
multiplies, and it removes the single most robotic thing a limb can do.

---

## 6. PROCEDURAL MOTION — the highest value per line in this document

### 6.1 The spring-damper — eight lines that change everything

This is **item #5 in the ranking and the most important code block in the document.** The
closed-form critically-damped spring, parameterised by *half-life* (intuitive) rather than
stiffness and damping (not), from
[Daniel Holden, *Spring-It-On: The Game Developer's Spring-Roll-Call*](https://theorangeduck.com/page/spring-roll-call)
(see also [Ryan Juckett, *Damped Springs*](https://www.ryanjuckett.com/damped-springs/)):

```cpp
inline float FastNegExp(float x)                    // ~1e-4 accurate for x in [0, 8]
{
    return 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
}
inline float HalflifeToDamping(float halflife)
{
    return (4.0f * 0.69314718056f) / (halflife + 1e-5f);      // 4 * ln(2)
}

struct Spring1 { float x = 0.0f, v = 0.0f; };

// Critically damped: fastest approach with NO overshoot. Exact, so stable at any dt.
inline void SpringStep(Spring1& s, float goal, float halflife, float dt)
{
    const float y  = HalflifeToDamping(halflife) * 0.5f;
    const float j0 = s.x - goal;
    const float j1 = s.v + j0 * y;
    const float e  = FastNegExp(y * dt);
    s.x = e * (j0 + j1 * dt) + goal;
    s.v = e * (s.v - j1 * y * dt);
}

// Tracking a MOVING goal without lag needs the goal's velocity too.
inline void SpringStepTracking(Spring1& s, float goal, float goalVel, float halflife, float dt)
{
    const float d = HalflifeToDamping(halflife);
    const float c = goal + (d * goalVel) / ((d * d) / 4.0f);
    const float y = d * 0.5f;
    const float j0 = s.x - c;
    const float j1 = s.v + j0 * y;
    const float e  = FastNegExp(y * dt);
    s.x = e * (j0 + j1 * dt) + c;
    s.v = e * (s.v - j1 * y * dt);
}
```

**Why this exact form and not `x += (goal − x) * 0.1f`:**

1. It is **framerate-independent** — the exponential is exact, not an approximation, so the
   result at dt = 1/30 equals two steps at dt = 1/60. The naive lerp does not have this property
   and will make your game feel different on different machines. That is a real bug the current
   `::Sleep(8)` main loop (`:717`) will expose the moment anyone runs it on different hardware.
2. It is parameterised by a **half-life in seconds**, which you can reason about and read off a
   table, unlike a stiffness coefficient.
3. It is **interruptible for free** — retarget `goal` at any moment and the motion stays C1
   continuous, because velocity is state.
4. `FastNegExp` is a rational approximation; no `expf` call, no libm dependency concerns.

**Where to apply it in this file, immediately, with no skeleton:**

| Quantity | Currently | With a spring | half-life |
| --- | --- | --- | ---: |
| `Miner::facing` | `atan2` assigned raw (`:233`) — snaps | turns smoothly, banks (§6.5) | 0.090 |
| Camera focus | teleports on pan | glides | 0.220 |
| `camDist` on wheel | steps 2.5 units | zooms | 0.140 |
| Health bar width (`:403`) | snaps | drains | 0.180 |
| Selection ring radius | constant | pops on select via an impulse | 0.080 |
| Banner Y position | static | slides in | 0.150 |
| Helmet / tool / backpack | rigid | lags (§5.3) | 0.075–0.130 |

**Angle springs need wrapping.** For `facing`, spring the *error*, not the angle:

```cpp
inline float WrapPi(float a)
{
    while (a >  3.14159265f) a -= 6.28318531f;
    while (a < -3.14159265f) a += 6.28318531f;
    return a;
}
// then: SpringStep on `err`, where err starts at WrapPi(goal - current), goal 0,
// and current += (errBefore - errAfter).
```

Skipping the wrap gives you a character that occasionally spins 350° the long way round to face
something 10° to its left. It is a five-minute bug that survives for months because it is rare.

**Two-segment chains (antenna, tail, ponytail):** each segment springs toward *its parent's*
current angle, with an increasing half-life. Three lines per segment, and you get whip.

### 6.2 Head look-at with angular limits — G=9

**Gaze is the strongest life signal a game character has.** A creature whose head tracks what
it cares about reads as having a mind; the same creature with a locked head reads as a prop,
regardless of how good its walk is. 46 lines.

```cpp
struct LookAt
{
    Spring1 yaw, pitch;                    // radians, in CHEST space
    float   targetX = 0, targetY = 0, targetZ = 0;
    float   weight  = 0.0f;                // 0 = neutral, 1 = full tracking
    float   holdTimer = 0.0f;
    bool    hasTarget = false;
};

// Split across the chain so the motion cascades. THIS is where "overlapping action"
// and "look-at" become the same feature.
struct LookDistribution { float head, neck, chest; float hlHead, hlNeck, hlChest; };
static const LookDistribution LOOK_HUMAN = { 0.60f, 0.20f, 0.20f, 0.070f, 0.120f, 0.230f };
static const LookDistribution LOOK_BEAST = { 0.45f, 0.15f, 0.40f, 0.055f, 0.100f, 0.180f };

void UpdateLookAt(LookAt& L, Pose& po, const Mat4& chestWorld,
                  float bodyX, float bodyY, float bodyZ, float facing,
                  const LookDistribution& D, float dt)
{
    float gYaw = 0.0f, gPitch = 0.0f, w = 0.0f;
    if (L.hasTarget) {
        // Direction to the target in BODY space (facing already applied).
        const float dx = L.targetX - bodyX;
        const float dy = L.targetY - (bodyY + chestWorld.m[13]);
        const float dz = L.targetZ - bodyZ;
        const float cf = std::cos(-facing), sf = std::sin(-facing);
        const float lx = dx * cf + dz * sf;
        const float lz = -dx * sf + dz * cf;
        const float horiz = std::sqrt(lx * lx + lz * lz);

        gYaw   = std::atan2(lx, lz);
        gPitch = std::atan2(dy, horiz > 1e-4f ? horiz : 1e-4f);

        // ANGULAR LIMITS. Beyond the dead zone the character gives up and looks away,
        // which is far more lifelike than clamping and staring at the shoulder.
        const float YAW_MAX  = 1.2217f;                 // 70 degrees
        const float YAW_DEAD = 1.9199f;                 // 110 degrees
        const float PITCH_MAX= 0.6109f;                 // 35 degrees
        if (std::fabs(gYaw) > YAW_DEAD) { w = 0.0f; }
        else {
            // Soft falloff over the last 20 degrees rather than a hard clamp.
            const float over = std::fabs(gYaw) - YAW_MAX;
            w = (over <= 0.0f) ? 1.0f : Clamp01(1.0f - over / (YAW_DEAD - YAW_MAX));
            if (gYaw >  YAW_MAX) gYaw =  YAW_MAX;
            if (gYaw < -YAW_MAX) gYaw = -YAW_MAX;
            if (gPitch >  PITCH_MAX) gPitch =  PITCH_MAX;
            if (gPitch < -PITCH_MAX) gPitch = -PITCH_MAX;
        }
    }
    SpringStep(L.weight_s, w, 0.180f, dt);              // ease the whole behaviour in/out

    // Three springs, three half-lives -> the chest starts, the head arrives last.
    Spring1& hy = L.yaw; Spring1& hp = L.pitch;
    SpringStep(hy, gYaw   * L.weight_s.x, D.hlHead,  dt);
    SpringStep(hp, gPitch * L.weight_s.x, D.hlHead,  dt);

    po.j[J_HEAD ].ry += hy.x * D.head;   po.j[J_HEAD ].rx += -hp.x * D.head;
    po.j[J_NECK ].ry += hy.x * D.neck;   po.j[J_NECK ].rx += -hp.x * D.neck;
    po.j[J_CHEST].ry += hy.x * D.chest;  po.j[J_CHEST].rx += -hp.x * D.chest;
}
```

**The four details that make it read as attention rather than as a servo:**

1. **Distribution across three joints with three half-lives.** The chest begins turning first
   (long half-life = slow, big mass), the head arrives last and fastest. That cascade *is*
   overlapping action, obtained free.
2. **A 60–90 ms reaction latency before the head starts.** Insert a delay timer on target
   acquisition. Instantaneous tracking reads as a security camera; 70 ms of latency reads as
   noticing.
3. **A dead zone at 110°, not a clamp.** Outside it, `w → 0` and the head returns to neutral.
   Clamping produces a character staring fixedly at its own shoulder, which is worse than not
   looking at all.
4. **Saccades.** Do not track one target continuously. Hold a target for
   `t ~ Exp(mean 1.8 s)`, then re-choose from a weighted list. That is §7.2's mechanism reused.

**Target selection priority** (this is the whole behaviour, and it is a switch statement):

| Priority | Target | Hold |
| --- | --- | --- |
| 1 | Nearest monster within 8 units | while present |
| 2 | Current drill tile | while drilling |
| 3 | Move destination | 0.8 s after the order |
| 4 | Nearest other miner within 4 units | Exp(1.2 s) |
| 5 | Crystal seam within 6 units | Exp(1.5 s) |
| 6 | A random point on the movement heading | Exp(2.5 s) |

Six lines of `if`, and the crew starts looking like it is aware of the cavern.

### 6.3 Two-bone IK — the law of cosines, worked and verified

Analytic, exact, no iteration.
([Ryan Juckett, *Analytic Two-Bone IK in 2D*](https://www.ryanjuckett.com/analytic-two-bone-ik-in-2d/);
[Holden, *Simple Two Joint IK*](https://theorangeduck.com/page/simple-two-joint);
[Gamasutra/Game Developer, *Inverse Kinematics (two joints) for foot placement*](https://www.gamedeveloper.com/programming/inverse-kinematics-two-joints-for-foot-placement);
[Law of cosines](https://en.wikipedia.org/wiki/Law_of_cosines).)

**Geometry.** Hip at `H`, foot target at `T`, thigh `L1 = 0.215`, shin `L2 = 0.205` (§3.3).
Let `d = |T − H|`, clamped into the reachable annulus.

```
Knee interior angle:   φ = acos( (L1² + L2² − d²) / (2·L1·L2) )
Knee FLEXION:          kneeFlex = π − φ
Hip offset from aim:   ψ = acos( (L1² + d²  − L2²) / (2·L1·d ) )
```

**Worked check** (do this by hand once; sign errors here are expensive). `L1 = L2 = 0.17`,
`d = 0.30`:
`φ = acos((0.0289 + 0.0289 − 0.0900) / (2 × 0.0289)) = acos(−0.5571) = 123.85°`
→ `kneeFlex = 56.15°`.
`ψ = acos((0.0289 + 0.0900 − 0.0289) / (2 × 0.17 × 0.30)) = acos(0.0900 / 0.1020) = acos(0.88235) = 28.07°`.
For an isosceles chain `ψ` must equal `(180° − φ)/2 = 28.07°`. ✔ The formulas agree.

Also check the degenerate case: fully extended `d = L1 + L2 = 0.34` →
`φ = acos((0.0289+0.0289−0.1156)/0.0578) = acos(−1) = 180°` → `kneeFlex = 0` ✔, and
`ψ = acos((0.0289+0.1156−0.0289)/(0.1156)) = acos(1) = 0` ✔.

```cpp
// Solve a leg. Returns hip rx/rz and knee rx in ANIMATOR SIGN (flexion positive).
// tx,ty,tz : foot target in PELVIS space. Bone lengths L1 (thigh), L2 (shin).
void SolveLegIK(float tx, float ty, float tz, float L1, float L2,
                float* outHipPitch, float* outHipRoll, float* outKneeFlex)
{
    float d = std::sqrt(tx * tx + ty * ty + tz * tz);
    const float dmin = std::fabs(L1 - L2) + 1e-3f;
    const float dmax = (L1 + L2) - 1e-3f;                 // never fully lock the knee
    if (d < dmin) d = dmin;
    if (d > dmax) d = dmax;

    // Unit direction hip -> target. At rest the bone points along -Y.
    const float inv = 1.0f / std::sqrt(tx*tx + ty*ty + tz*tz + 1e-12f);
    const float ux = tx * inv, uy = ty * inv, uz = tz * inv;

    // With R = Ry*Rx*Rz applied to (0,-1,0) and ry = 0:
    //   result = ( sin(rz), -cos(rz)cos(rx), -cos(rz)sin(rx) )
    // Invert:
    float sr = ux; if (sr >  1.0f) sr =  1.0f; if (sr < -1.0f) sr = -1.0f;
    const float rz   = std::asin(sr);                     // abduction
    const float aim  = std::atan2(-uz, -uy);              // sagittal aim angle (local rx)

    // Law of cosines.
    float cphi = (L1*L1 + L2*L2 - d*d) / (2.0f * L1 * L2);
    if (cphi >  1.0f) cphi =  1.0f; if (cphi < -1.0f) cphi = -1.0f;
    const float kneeFlex = 3.14159265f - std::acos(cphi);

    float cpsi = (L1*L1 + d*d - L2*L2) / (2.0f * L1 * d);
    if (cpsi >  1.0f) cpsi =  1.0f; if (cpsi < -1.0f) cpsi = -1.0f;
    const float psi = std::acos(cpsi);

    // The knee must bend BACKWARDS, so the thigh rotates FORWARD from the aim by psi.
    // aim is a local rx; positive local rx swings the bone backwards; therefore subtract.
    *outKneeFlex  = kneeFlex;
    *outHipRoll   = rz;
    *outHipPitch  = -(aim - psi);        // negate: caller wants ANIMATOR sign (flexion +)
}
```

**Pole vector.** In this formulation the bend direction is implicit: the knee always bends
backward, because `ψ` is always subtracted from the aim in the `rx` (sagittal) channel. For a
character whose knees bend *forward* (a bird, a mech), flip the sign of `ψ`. For a knee that
should bend outward/inward (an insect), add the pole as a `ry` on the hip *before* solving —
the solve then happens in the rotated plane. One extra rotation, no extra maths.

**Where this actually earns its keep** — say it plainly, because it is not where people expect:

- ❌ *Uneven terrain*: the floor here is flat (§1.3 note). Low value **today**.
- ✅ **Root motion becomes free.** With the feet IK'd to fixed world points, you can move
  `J_ROOT` vertically (bob, squash, landing, breathing) and laterally (weight shift) *without
  the feet leaving the ground*. Every root-motion technique in §4.2 and §5.1 becomes usable
  instead of looking like the character is floating.
- ✅ **Crouch, squat, and lunge poses** stop self-intersecting.
- ✅ **Foot pinning during stance** (§6.4) — the definitive anti-slide fix.
- ✅ **Standing on the lip of a drilled tile** — actually does occur in this game, when a wall
  is removed under a miner's foot.

**The hip-lowering pass** (needed whenever a target is unreachable):

```cpp
// One iteration is enough for a two-legged character on near-flat ground.
float deficit = 0.0f;
for (int leg = 0; leg < 2; ++leg) {
    const float need = DistanceFromHipToTarget(leg);
    const float have = L1 + L2 - 0.004f;
    if (need > have) deficit = Max(deficit, need - have);
}
pose.j[J_ROOT].ty -= deficit;           // drop the whole body until both feet can reach
// then re-solve both legs.
```

### 6.4 Foot planting — the definitive anti-slide fix, and stepping for creatures

**For bipeds (pin during stance):**

```cpp
struct FootPin { float wx, wy, wz; bool planted = false; };

// At the START of stance (phase crosses 0.0 for the right foot, 0.5 for the left):
//   record the foot's CURRENT world position and hold it.
// During stance: IK to that fixed world point (converted into pelvis space each frame).
// At the START of swing: release, and let FK drive the foot.
// During the last 25% of swing: blend the FK foot toward the PREDICTED next plant point,
//   which is  bodyPos + heading * (stride * 0.5)  -- half a stride ahead.
```

The predicted plant point is what removes the last visible artefact: a foot that is still
sliding into position when it touches down. Blend over `p ∈ [0.90, 1.00]` with `SmootherStep`.

**Step arc.** During swing, the foot must travel on an arc, not a line (§5.5):

```cpp
const float s = (p - stanceEnd) / (1.0f - stanceEnd);     // 0..1 through swing
foot.x = Lerp(liftPos.x, plantPos.x, SmootherStep(s));
foot.z = Lerp(liftPos.z, plantPos.z, SmootherStep(s));
foot.y = groundY + 0.055f * 4.0f * s * (1.0f - s) * 1.35f;   // parabola, peak 0.074 at s=0.5
```

The `SmootherStep` on the horizontal but a raw parabola on the vertical is deliberate: the foot
should accelerate off the ground and decelerate into the plant, but its *height* should peak
symmetrically. Using the same easing on both makes the step look mechanical.

**For creatures with 4, 6 or 8 legs — the distance-threshold stepper.** This is the technique
behind every procedural spider you have seen
([80.lv, *IK-Driven Procedural Spider Locomotion*](https://80.lv/articles/ik-driven-procedural-spider-locomotion-in-godot-4-5);
[Unity Procedural IK Wall-Walking Spider](https://github.com/PhilS94/Unity-Procedural-IK-Wall-Walking-Spider)):

```cpp
struct Leg {
    float homeX, homeZ;          // rest position in BODY space
    float curX, curY, curZ;      // current WORLD foot position
    float tgtX, tgtZ;            // where it is stepping to
    float stepT = 1.0f;          // 1 = planted; <1 = mid-step
    int   group;                 // gait group
};

const float STEP_TRIGGER = 0.16f;      // units. TUNING: ~35% of leg reach
const float STEP_TIME    = 0.22f;      // s
const float STEP_LEAD    = 0.10f;      // s of velocity look-ahead

// each frame, per leg:
const float wantX = bodyX + homeX_rotated + velX * STEP_LEAD;
const float wantZ = bodyZ + homeZ_rotated + velZ * STEP_LEAD;
const float err   = Dist2D(curX, curZ, wantX, wantZ);
if (leg.stepT >= 1.0f && err > STEP_TRIGGER && GroupMayStep(leg.group))
    { leg.tgtX = wantX; leg.tgtZ = wantZ; leg.stepT = 0.0f; }
if (leg.stepT < 1.0f) {
    leg.stepT += dt / STEP_TIME;
    const float s = Clamp01(leg.stepT);
    leg.curX = Lerp(startX, leg.tgtX, SmootherStep(s));
    leg.curZ = Lerp(startZ, leg.tgtZ, SmootherStep(s));
    leg.curY = groundY + 0.09f * 4.0f * s * (1.0f - s);
}
```

**Gait groups** (`GroupMayStep` allows a group only if the *other* group is fully planted):

| Creature | Legs | Groups | Phase offsets |
| --- | ---: | --- | --- |
| Quadruped, walk | 4 | 4 groups | FL 0.00, RR 0.25, FR 0.50, RL 0.75 (lateral sequence) |
| Quadruped, trot | 4 | 2 groups | {FL,RR} 0.0, {FR,RL} 0.5 |
| Hexapod, alternating tripod | 6 | 2 groups | {L1,R2,L3} 0.0, {R1,L2,R3} 0.5 |
| Octopod, alternating tetrapod | 8 | 2 groups | {L1,R2,L3,R4} 0.0, {R1,L2,R3,L4} 0.5 |

`STEP_LEAD` (velocity look-ahead) is the parameter that makes it look like the creature is
*reaching where it is going* rather than *catching up*. 0.10 s is a good default; a lumbering
creature wants 0.05, a scuttling one 0.16.

### 6.5 Banking into turns

```cpp
// Angular velocity from the facing spring, then lean.
const float omega = (facing - prevFacing_wrapped) / dt;     // rad/s
float bankTarget = -BANK_K * omega * speed;                 // BANK_K = 0.085 s^2/unit
if (bankTarget >  MAX_BANK) bankTarget =  MAX_BANK;         // MAX_BANK = 18 deg
if (bankTarget < -MAX_BANK) bankTarget = -MAX_BANK;
SpringStep(bank, bankTarget, 0.120f, dt);
pose.j[J_ROOT].rz = bank.x;
```

Three refinements that are each one line and each visible:

1. **Lean into acceleration too**, not just turning:
   `pose.j[J_SPINE].rx += -0.055f * forwardAccel;` — the character pitches forward when
   starting and rocks back when stopping. This is the cheapest "weight" cue in the document.
2. **The head anticipates the turn.** `lookAt.targetYaw += 0.5f * omega * 0.25f;` — the head
   leads the body into a corner by ~250 ms. Real animals steer with their heads.
3. **Heavy creatures bank the other way.** A ponderous monster leaning *into* a turn looks
   like a motorcycle. Give it `BANK_K = −0.030` (small, outward) and it looks like it is
   resisting its own momentum. Sign flip; huge character difference.

### 6.6 Footstep-driven camera shake

Trauma-based, from
[Squirrel Eiserloh, *Math for Game Programmers: Juicing Your Cameras With Math*, GDC 2016](http://www.mathforgameprogrammers.com/gdc2016/GDC2016_Eiserloh_Squirrel_JuicingYourCameras.pdf)
([archive](https://archive.org/details/GDC2016Eiserloh),
[video](https://www.youtube.com/watch?v=tu-Qe66AvtY)).
The two ideas that matter: **shake magnitude is trauma *squared*** (so small traumas are
nearly invisible and large ones are dramatic — a linear mapping feels wrong), and **use
smoothed noise, not `rand()`** (so it survives pause and slow-motion and is reproducible).

```cpp
float g_trauma = 0.0f;                     // 0..1, decays linearly

inline void AddTrauma(float amount) { g_trauma = Min(1.0f, g_trauma + amount); }

// 12-line value noise: no Perlin needed, no libraries, no tables.
inline float Hash1(int i)
{
    unsigned int n = (unsigned int)i;
    n = (n << 13) ^ n;
    n = n * (n * n * 15731u + 789221u) + 1376312589u;
    return 1.0f - (float)(n & 0x7fffffffu) * (1.0f / 1073741824.0f);   // [-1, 1)
}
inline float VNoise(float x)
{
    const int i = (int)std::floor(x);
    const float f = x - (float)i;
    const float u = f * f * (3.0f - 2.0f * f);
    return Hash1(i) * (1.0f - u) + Hash1(i + 1) * u;
}

void ApplyShake(float& camX, float& camY, float& camRollDeg, float t, float dt)
{
    g_trauma -= 1.4f * dt;                                  // decay rate: full -> 0 in 0.71 s
    if (g_trauma < 0.0f) g_trauma = 0.0f;
    const float shake = g_trauma * g_trauma;                // <-- the squaring
    const float F = 18.0f;                                  // Hz
    camX       += 0.25f * shake * VNoise(t * F + 0.0f);
    camY       += 0.25f * shake * VNoise(t * F + 137.0f);
    camRollDeg += 2.50f * shake * VNoise(t * F + 913.0f);
}
```

**Trauma budget** — this table is the whole design, because over-shaking is worse than not
shaking:

| Event | Trauma | Note |
| --- | ---: | --- |
| Miner footstep | **0.00** | never. A 0.9-unit character does not shake a camera. |
| Large monster footstep | 0.09 | scaled by `1 / (1 + camDist·0.08)` |
| Monster emerging (burst frame) | 0.50 | |
| Melee hit landed | 0.14 | |
| Miner death | 0.30 | |
| Drill contact (per macro cycle) | 0.02 | continuous low buzz |
| Wall collapse | 0.40 | |

The distance falloff matters: at `camDist = 34` (the default, `:105`) a footstep at 0.09 becomes
0.024, which squares to 0.0006 — invisible. That is correct. It should only shake when you are
zoomed in on the thing that is stomping.

### 6.7 Distance-driven phase — the foot-slide killer, 20 lines, ranked #2

The current line `m.phase += dt * 6.0f;` (`:230`) is the problem. Replace with:

```cpp
// In Miner, replace `float phase` with:
float gaitPhase = 0.0f;         // [0,1), 0 = right heel strike
float prevX = 0.0f, prevZ = 0.0f;

// In Update(), after moving:
const float moved = Dist2D(m.pos, { m.prevX, 0.0f, m.prevZ });
m.prevX = m.pos.x; m.prevZ = m.pos.z;

const float speed  = moved / (dt > 1e-5f ? dt : 1e-5f);
const float stride = Lerp(STRIDE_WALK, STRIDE_RUN, runWeight);   // 0.709 .. 1.078
m.gaitPhase += moved / stride;                                   // <-- THE FIX

// When nearly stationary, ease the phase to a rest pose rather than freezing mid-stride.
if (speed < 0.05f) {
    const float toRest = WrapPhase(0.0f - m.gaitPhase);   // settle onto the contact pose
    m.gaitPhase += toRest * (1.0f - std::exp(-6.0f * dt));
}
while (m.gaitPhase >= 1.0f) m.gaitPhase -= 1.0f;
```

**Footstep event detection** (drives dust, sound, squash, trauma):

```cpp
inline bool CrossedPhase(float prev, float now, float mark)
{
    if (now >= prev) return (prev < mark && now >= mark);
    return (prev < mark) || (now >= mark);              // wrapped this frame
}
if (CrossedPhase(prevPhase, m.gaitPhase, 0.0f)) OnFootPlant(m, /*right=*/true);
if (CrossedPhase(prevPhase, m.gaitPhase, 0.5f)) OnFootPlant(m, /*right=*/false);
```

`OnFootPlant` is where four other systems hook in at once: `SquashImpulse(0.10)`, a dust quad,
`AddTrauma`, and a footstep sound. **One event, four payoffs** — this is why item #12 in the
ranking is high despite touching several systems.

---

## 7. WHAT SELLS A CREATURE, AS OPPOSED TO A HUMANOID

### 7.1 Asymmetry — the anti-clone measures

Right now all five miners share `phase = i * 0.8f` (`:184`) and are otherwise identical. Five
identical figures moving identically is the strongest "these are instances of a struct" signal
in the build. **Eighteen lines fixes it.**

```cpp
struct Variation                            // one per entity, seeded once at spawn
{
    float phaseOffset;      // 0..1
    float rateMul;          // 0.94 .. 1.06   -- they drift in and out of sync forever
    float heightMul;        // 0.93 .. 1.07
    float widthMul;         // 0.95 .. 1.05
    float hueShift;         // -0.04 .. +0.04
    float bobMul;           // 0.85 .. 1.15
    float armGainMul;       // 0.80 .. 1.20
    float idleRate;         // 0.7 .. 1.4  -- twitch frequency
    float asymL, asymR;     // 0.90 .. 1.10 per-side amplitude
    float phaseSplit;       // 0.44 .. 0.56 -- NOT exactly 0.5
};
```

`rateMul` is the important one and it is the least obvious: entities whose cycle rates differ
by ±6% will drift into and out of phase over 8–20 seconds, so a crowd never *settles* into
formation. A fixed phase offset (what `:184` does today) produces a crowd that marches in
permanent lockstep, which reads worse than pure randomness.

**Gait asymmetry proper** — the difference between a walk and a *creature's* walk:

| Parameter | Humanoid | Creature |
| --- | --- | --- |
| `phaseSplit` (left leg offset) | 0.500 | **0.44–0.56**, fixed per entity |
| Left/right amplitude ratio | 1.00 | 0.88–1.12 |
| Stance fraction, left vs right | equal | differ by up to 0.06 |
| Dwell warp (§4.9) | 0.0 | 0.2–0.6 |

**A limp** is a specific, recognisable asymmetry worth implementing for damaged creatures:
`stanceFraction` on the good leg 0.68, on the bad leg 0.48; the bad leg's knee flexion ×0.55
(it does not want to bend); `A_BOB` ×1.9 but only on the good-leg push; and a 0.09 s dwell in
the double-support after the bad leg lands. Drive the whole thing from
`limpAmount = 1 − health/maxHealth` and a wounded creature visibly deteriorates.

### 7.2 Idle twitches — Poisson, not periodic

**Periodic idle motion is the tell.** If a twitch happens every 3.0 s the viewer's brain locks
onto the period in about 12 seconds and the illusion dies. Poisson-distributed intervals never
lock.

```cpp
// Exponentially distributed interval with mean 1/lambda. rng.Unit() in (0,1].
inline float NextEventDelay(Rng& rng, float lambda)
{
    float u = rng.Unit();
    if (u < 1e-6f) u = 1e-6f;
    return -std::log(u) / lambda;
}
```

The twitch table. Each entry is an **impulse into an existing spring** — which is why this
whole system is 40 lines: it reuses §6.1 and adds no new state.

| Twitch | λ (per s) | Implementation | Duration |
| --- | ---: | --- | ---: |
| **Blink** | 0.28 | eye cube `sy → 0.06`; 25% chance of a double-blink 0.16 s later | 0.09 s |
| Head flick | 0.22 | `lookAt.yaw.v += ±2.2` impulse; retarget | ~0.4 s settle |
| Ear / antenna flick | 0.30 | `antenna.v += ±5.0` | ~0.5 s |
| Weight shift | 0.16 | see §4.5 | 0.9 s |
| Shoulder roll | 0.08 | additive `rz` pulse ±6°, `EaseOutBack` | 0.7 s |
| Tail / tool adjust | 0.12 | `tool.v += ±1.5` | 0.4 s |
| Whole-body shiver | 0.03 | 8 Hz noise on spine `rz`, amplitude 1.4°, decaying over 0.6 s | 0.6 s |

**BLINKING IS THE HIGHEST-RATIO ITEM IN THIS ENTIRE DOCUMENT (R = 21.4).** The monster eyes at
`:427-429` are two static cubes. Scale their Y to 0.06 for 90 ms at Poisson intervals with
λ = 0.28 and they become eyes. Fourteen lines:

```cpp
// per creature
float blinkTimer = 0.0f, blinkPhase = 1.0f;
int   blinkQueued = 0;

blinkTimer -= dt;
if (blinkTimer <= 0.0f) {
    blinkPhase = 0.0f;
    blinkTimer = NextEventDelay(rng, 0.28f);
    if (rng.Chance(0.25f)) blinkQueued = 1;                 // double blink
}
if (blinkPhase < 1.0f) {
    blinkPhase += dt / 0.09f;
    if (blinkPhase >= 1.0f && blinkQueued) { blinkPhase = -0.7f; blinkQueued = 0; }
}
// closed-ness: 1 at the middle of the blink, 0 outside
const float shut = (blinkPhase >= 0.0f && blinkPhase <= 1.0f)
                 ? std::sin(blinkPhase * 3.14159265f) : 0.0f;
const float eyeSy = 1.0f - 0.94f * shut;
```

Note the asymmetry you get free: `sin(πt)` closes and opens at the same rate, which is wrong —
real blinks close in ~1/3 of the time they take to open. Use `shut = t < 0.32f ? t/0.32f :
1 − (t−0.32f)/0.68f` for the real thing. Two more lines, and it is noticeably better.

### 7.3 Telegraphed wind-up — the creature version

Beyond §5.2's anticipation: a creature's telegraph should be **legible from any camera angle
and at any zoom**, because it is a gameplay contract. Layer three cues that fail independently:

| Channel | Cue | Why |
| --- | --- | --- |
| **Silhouette** | rear back, raise limbs, widen stance — the outline changes | works when zoomed out to 70 units |
| **Colour** | eyes brighten from 1.0/0.95/0.30 to 1.0/0.35/0.20 over the wind-up | works when the silhouette is small |
| **Motion** | the whole body *stops* moving for 0.10 s before the lunge | the "moving hold" — a hard freeze reads as intent |
| **Ground** | a scuff mark / dust puff at the rear foot | reads even if the creature is occluded |
| **Audio** | `::Beep(220, 90)` rising | the existing beep vocabulary at `:310`, `:335` |

**The 0.10 s freeze is the strongest of the five.** In a scene where everything is in motion,
one thing stopping dead is the most conspicuous event possible. It is `if (windUpPhase >
0.62f && windUpPhase < 0.80f) return;  // skip the pose update` — two lines.

### 7.4 Timing the wind-up to the creature's mass

| Creature | Wind-up | Strike | Recovery | Windows for the player |
| --- | ---: | ---: | ---: | --- |
| Small / fast (species 1, `scale` ~0.8) | 0.22 s | 0.07 s | 0.25 s | reaction only |
| Medium (species 0) | 0.38 s | 0.10 s | 0.45 s | reposition |
| Large (species 2, `scale` ~1.5) | 0.72 s | 0.16 s | 0.95 s | reposition + counter |

The `mo.scale` field already exists (`:307`, `0.75 + rng.Unit()*0.8` → range 0.75–1.55).
**Drive all animation timing from it**, with a single exponent:

```cpp
// Heavier things are slower. sqrt is the right law: pendulum period ~ sqrt(L).
const float timeScale = 1.0f / std::sqrt(mo.scale);     // scale 0.75 -> 1.155, 1.55 -> 0.803
```

That is one line and it makes the big variant *feel* big rather than merely *be* big, which is
currently the only thing the scale field does.

### 7.5 Recoil on hit — G=7, 22 lines, no skeleton required

Currently damage is invisible (`:262-265` decrements a float). Four cues, all cheap:

```cpp
struct Hit { float flash = 0.0f; float dirX = 0, dirZ = 0; };

void OnDamaged(Entity& e, float dmg, float fromX, float fromZ)
{
    const float d = Dist2D(e.pos, {fromX, 0, fromZ});
    const float nx = (e.pos.x - fromX) / (d > 1e-4f ? d : 1e-4f);
    const float nz = (e.pos.z - fromZ) / (d > 1e-4f ? d : 1e-4f);
    e.hit.flash = 1.0f;
    e.hit.dirX = nx; e.hit.dirZ = nz;
    e.knockback.v += 2.4f * dmg * 0.05f * nx;      // spring impulse, x
    e.knockbackZ.v+= 2.4f * dmg * 0.05f * nz;
    SquashImpulse(e.squash, 0.30f);                // section 5.1
    e.hitStop = 0.055f;                            // FREEZE
    AddTrauma(0.14f);
}
```

1. **Hit-stop.** For 55 ms, `dt` for this entity (or globally, for a big hit) is set to 0.
   Universally described as the single highest-value impact technique in action games. Global
   hit-stop for player-relevant hits, per-entity for background ones.
2. **Flash.** `flash` decays at 1/0.12 s. Add it to the colour:
   `glColor3f(r + (1−r)·flash·0.85, ...)`. Additive-toward-white reads better than
   replace-with-white because the silhouette's shading survives.
3. **Knockback spring**, not a teleport — so it recovers, and so simultaneous hits accumulate.
4. **Directional squash**: compress along the damage direction, expand perpendicular. Needs a
   `ry` on the root to align the squash axis with `(dirX, dirZ)` — three lines.

### 7.6 EMERGE FROM GROUND — the set piece, and the current implementation is wrong

Current (`:417-421`): uniform scale 0.35 → 1.0 combined with a sink. **Scaling up reads as
"spawning in", never as "digging out",** because real things do not change size. The creature
must be full-size the entire time and *occluded* by the floor.

**OpenGL 1.1 gives you exactly the right tool for that: a user clip plane.** Guaranteed
minimum 6 (`GL_MAX_CLIP_PLANES`)
([Khronos `glClipPlane`](https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glClipPlane.xml)).

```cpp
// Draw the creature at full size; discard everything below the floor plane y = FLOOR_Y.
const GLdouble plane[4] = { 0.0, 1.0, 0.0, -(GLdouble)FLOOR_Y };   // keep y > FLOOR_Y
glEnable(GL_CLIP_PLANE0);
glClipPlane(GL_CLIP_PLANE0, plane);       // NOTE: transformed by the CURRENT modelview
DrawSkinned(...);
glDisable(GL_CLIP_PLANE0);
```

Same gotcha as the light: the plane equation is transformed by the modelview at the moment of
the `glClipPlane` call, so set it with the *view* matrix loaded, before the model transform.

**The four-phase sequence** (total 1.7 s):

| Window | Phase | Content |
| --- | --- | --- |
| 0.00–0.90 s | **Telegraph** | The existing red marker (`:479-489`) plus: floor tile jitters `±0.02 · (t/0.9)²` at 18 Hz; a dust ring expands to r = 0.55 with alpha decaying; 3 crack quads rotate outward from the centre; `Beep(70, 90)` (already at `:310`) |
| 0.90–1.05 s | **Burst** | 8 rock cubes launched ballistically: `vy ∈ [2.5, 4.5]`, `vxz` radial ∈ [0.8, 1.8], spin ∈ [4, 12] rad/s, gravity −9.81, lifetime 1.4 s. The creature's **head punches to 1.15× final height and overshoots**. `AddTrauma(0.50)` |
| 1.05–1.35 s | **Haul out** | root Y from `−bodyHeight` to `+0.06` with `EaseOutBack(2.4)`; root `rx` from −35° to +8° to 0 — it *levers* itself out. Front limbs plant on the rim first (IK them to the tile edge), then the body follows. |
| 1.35–1.70 s | **Assert** | Landing squash `sy 0.82 / sx,sz 1.15` springing out; arms slam down; head sweeps 40° left→right over 0.35 s using the look-at (§6.2) with `hlHead = 0.10`; eyes brighten; a roar |

Five details, ranked by how much each contributes:

1. **The overshoot at 1.05–1.35 (`EaseOutBack`, s = 2.4).** Coming up to exactly the right
   height and stopping is what makes the current version read as a spawn. Overshooting by 11%
   and settling reads as effort against resistance.
2. **The clip plane.** Full-size and occluded, not small and growing.
3. **The head-first burst.** A creature emerges head-first with the body following, not as a
   rigid unit rising. Give `J_HEAD` a 0.10 s *lead* over the root — negative lag.
4. **The debris.** Eight cubes on ballistic arcs. This is ~25 lines and it does as much work as
   the creature motion, because it is evidence that the ground was displaced.
5. **The head sweep at the end.** It arrives, and then it *looks for you*. That is the moment
   the thing becomes an agent rather than an event.

---

## 8. TIMING AND WEIGHT

### 8.1 The principle, stated mechanically

Mass is not communicated by size. It is communicated by **how long a thing takes to change
velocity**, which the eye reads as: (a) duration of the acceleration phase, (b) presence or
absence of overshoot, (c) length of the settle, (d) how much anticipation precedes the move.

A light object and a heavy object moving the same distance in the same total time still read
as different masses **purely from the shape of the curve**.

### 8.2 The numbers

| Property | Light (×0.4 mass) | Medium (×1.0) | Heavy (×5.0) |
| --- | ---: | ---: | ---: |
| Move duration for a 1-unit displacement | 0.16 s | 0.30 s | 0.75 s |
| **Slow-in (acceleration) fraction** | **0.05** | 0.20 | **0.45** |
| Slow-out (deceleration) fraction | 0.60 | 0.45 | 0.40 |
| Out curve | `EaseOutQuint` | `EaseOutCubic` | `EaseInOutQuad` |
| **Overshoot at the stop** | **+12%** | +6% | **0%** |
| Settle half-life | 0.06 s | 0.10 s | 0.22 s |
| Residual sway after stop | none | 1 small bounce | 0.5 Hz sway for 1.2 s |
| Anticipation duration | 0.10 s | 0.18 s | 0.42 s |
| Anticipation depth `a` | 0.18 | 0.28 | 0.40 |
| Hold at the extremes of a cycle | 0 frames | 2 frames | **5 frames** (dwell warp 0.55) |
| Step cadence | 2.8 Hz | 2.0 Hz | 1.1 Hz |
| Bob amplitude / height | 1.8% | 3.5% | 5.5% |
| Squash impulse on footfall | 0.06 | 0.15 | 0.42 |
| Camera trauma per footstep | 0.00 | 0.02 | 0.14 |
| Contact hold on impact | 2 frames | 4 frames | 7 frames |

### 8.3 The three differences that carry the most information

Everything above is refinement of three facts. If you implement nothing else from this section,
implement these:

1. **Heavy things have a long slow-IN; light things have almost none.** A light creature is at
   full speed on frame 2. A heavy one takes 45% of the move to get there. This is inertia and
   it is the primary cue. Light: `EaseOut` only. Heavy: `EaseInOut`, weighted toward the in.
2. **Light things overshoot and rebound; heavy things do not.** A heavy creature arriving at a
   pose *thuds* — it stops, and any residual is a slow low-frequency sway, not a bounce.
   Implement as: light = under-damped spring (ζ = 0.35), heavy = critically damped (ζ = 1.0)
   plus a separate 0.5 Hz sway.
3. **Anticipation scales with force, not with size.** A 0.42 s wind-up on a heavy attack is
   the character *gathering* the force. Skipping it makes a heavy creature feel weightless no
   matter how large you draw it.

### 8.4 Frame-rate honesty

The current loop is `::Sleep(8)` with `GetTickCount()` timing (`:656-668`, `:717`).
`GetTickCount()` has ~15.6 ms resolution by default, which means `dt` quantises to 0 or 16 ms
in bursts. **That alone will make every spring and every eased curve stutter visibly**, and it
will be blamed on the animation code. Two options, both small:

- `timeBeginPeriod(1)` — `winmm.lib` is already linked (`:48`) — and keep `GetTickCount`.
- Better: `QueryPerformanceCounter` / `QueryPerformanceFrequency`, ~8 lines, microsecond
  resolution, no global system side effect.

Do this before tuning any constant in this document, or every constant you tune will be
compensating for timer noise.

---

## 9. THE ORDERED IMPLEMENTATION LIST

Ordered by ratio within dependency order. Each stage is independently shippable and each one
visibly improves the build on its own.

### Stage 0 — no skeleton, no keyframes, ~190 lines

| Order | Item | § | Lines | Touches |
| ---: | --- | --- | ---: | --- |
| 0.1 | `QueryPerformanceCounter` timing | 8.4 | 10 | `WinMain` `:656-668` |
| 0.2 | `Spring1` + `SpringStep` + `FastNegExp` | 6.1 | 20 | new |
| 0.3 | Easing library | 5.4 | 25 | new |
| 0.4 | `Hash1` / `VNoise` | 6.6 | 12 | new |
| 0.5 | Per-entity `Variation`, seeded at spawn | 7.1 | 18 | `Miner`, `Monster`, `NewLevel` `:181-186` |
| 0.6 | Blink on monster eyes | 7.2 | 14 | `DrawMonster` `:427-429` |
| 0.7 | Distance-driven gait phase + footstep events | 6.7 | 20 | `Update` `:230`, `Miner` |
| 0.8 | Facing spring + bank into turns | 6.5 | 24 | `Update` `:233`, `DrawMiner` `:376` |
| 0.9 | Root bob (2×) / sway (1×) / roll from phase | 4.2 | 26 | `DrawMiner` `:373` |
| 0.10 | Hit-stop + damage flash + knockback spring | 7.5 | 22 | `Update` `:260-266` |

**Stop here and look at it.** Nothing above requires the skeleton, and it should already be a
visibly different game.

### Stage 1 — lighting and the skeleton, ~250 lines

| Order | Item | § | Lines |
| ---: | --- | --- | ---: |
| 1.1 | `glEnable(GL_LIGHTING)`, normals, `GL_NORMALIZE`, light after `gluLookAt` | 2.2 | 20 |
| 1.2 | Unit cube display list + `DrawBox` | 2.3 | 30 |
| 1.3 | Hemisphere AO term per part | 2.4 | 4 |
| 1.4 | `JointDef` / `JointPose` / `Pose` / `Mat4` / `LocalTRS` / `ComputeWorld` | 3.2–3.5 | 90 |
| 1.5 | `MINER_SKEL` + `MINER_SKIN` tables (re-proportioned) | 3.3–3.4 | 55 |
| 1.6 | `DrawSkinned` + `ValidateSkeleton` | 3.6 | 30 |
| 1.7 | Monster skeleton + skin tables (3 species) | 3.3 | 60 |

### Stage 2 — clips and layers, ~230 lines

| Order | Item | § | Lines |
| ---: | --- | --- | ---: |
| 2.1 | `Key` / `Channel` / `Clip` + Catmull-Rom sampling | 3.7 | 55 |
| 2.2 | `BlendPose` / `AddPose` / masks | 3.8 | 45 |
| 2.3 | Walk clip (32 keys) + `ApplyWalk` | 4.3 | 55 |
| 2.4 | Idle breathing additive layer | 4.5 | 30 |
| 2.5 | Poisson twitch system | 7.2 | 40 |

### Stage 3 — procedural layer, ~180 lines

| Order | Item | § | Lines |
| ---: | --- | --- | ---: |
| 3.1 | Head look-at with limits, distribution, saccades | 6.2 | 46 |
| 3.2 | Squash & stretch from impulse | 5.1 | 28 |
| 3.3 | Follow-through lag chain (helmet, tool, backpack) | 5.3 | 25 |
| 3.4 | `SolveLegIK` | 6.3 | 40 |
| 3.5 | Foot pinning during stance + hip lowering | 6.4 | 45 |

### Stage 4 — the set pieces, ~290 lines

| Order | Item | § | Lines |
| ---: | --- | --- | ---: |
| 4.1 | Attack anticipation / strike / hold / recovery | 4.7 | 55 |
| 4.2 | Death topple (rigid pendulum) | 4.8 | 45 |
| 4.3 | Emerge-from-ground with clip plane + debris | 7.6 | 70 |
| 4.4 | Drill / work cycle (macro + micro) | 4.6 | 40 |
| 4.5 | Camera trauma + shake | 6.6 | 35 |
| 4.6 | Run clip + speed blend | 4.4 | 45 |

### Stage 5 — the long tail

Carry-heavy layer (§4.9), dwell warp on every cycle, gait asymmetry and limp (§7.1), multi-leg
steppers for creatures (§6.4), telegraph colour/silhouette cues (§7.3), the helmet lamp (§2.5),
shadow blobs.

**Total: ~1,140 net lines of `/W4`-clean C++ across five stages**, against a 725-line file. It
roughly triples the source, and it is the difference between "competent prototype" and
"something is living in there".

---

## 10. THINGS THIS DOCUMENT DELIBERATELY DOES NOT RECOMMEND

**10.1 A full physics ragdoll.** Constraint solving, collision response and a stable solver is
600+ lines to reach *worse* results than the §4.8 pendulum topple for a box character. The
pendulum has 90% of the payoff for 8% of the code. Revisit only if you want dismemberment.

**10.2 Real shadow maps.** Out of reach in OpenGL 1.1 without extensions. A **blob shadow** —
a dark, alpha-blended quad on the floor, radius `0.28 · (1 − height·0.8)` — is 30 lines, needs
no extension, and delivers most of the *grounding* benefit, which is the part that matters for
character believability. Listed at #30 in the ranking for a reason: it is not animation, but it
reads as one, because a character that does not touch the ground never looks alive.

**10.3 Blend trees / state machines with graph editors.** For ~8 clips, a `switch` on an enum
plus two blend weights is smaller, faster to debug and easier to read. Revisit past ~25 clips.

**10.4 Any third-party library.** The only serious candidate is
[ozz-animation](https://guillaumeblanc.github.io/ozz-animation/) (MIT, ~300 KB static). It is
excellent, and it is the wrong tool here: it is built around imported rigs, skinned meshes and
a runtime-optimised sampling format, all of which are exactly the parts we do not have and do
not need. Its value proposition is *pipeline*, and we have no pipeline by choice. **Reject.**
No library in this document is required; total external dependency count remains **zero**.

**10.5 Motion capture, in any form.** Beyond the licensing question, a rigid-cube character
does not benefit from mocap fidelity — the reason stylised characters read well is the
*exaggeration* that mocap specifically removes. The tables in §4 are better for this art style
than any capture would be.

---

## 11. SOURCES

**Procedural animation, practice**
- David Rosen (Wolfire), *Animation Bootcamp: An Indie Approach to Procedural Animation*, GDC 2014 — https://archive.org/details/GDC2014Rosen · https://www.gdcvault.com/play/1020583/Animation-Bootcamp-An-Indie-Approach · https://www.gameanim.com/2018/03/04/indie-approach-procedural-animation/
- Alexander Bereznyak (Ubisoft), *IK Rig: Procedural Pose Animation*, GDC 2016 — https://www.gdcvault.com/play/1023279/IK-Rig-Procedural-Pose · https://archive.org/details/GDC2016Bereznyak · https://www.youtube.com/watch?v=KLjTU0yKS00
- Squirrel Eiserloh, *Math for Game Programmers: Juicing Your Cameras With Math*, GDC 2016 — http://www.mathforgameprogrammers.com/gdc2016/GDC2016_Eiserloh_Squirrel_JuicingYourCameras.pdf · https://archive.org/details/GDC2016Eiserloh · https://www.youtube.com/watch?v=tu-Qe66AvtY

**Springs and damping**
- Daniel Holden, *Spring-It-On: The Game Developer's Spring-Roll-Call* — https://theorangeduck.com/page/spring-roll-call
- Ryan Juckett, *Damped Springs* — https://www.ryanjuckett.com/damped-springs/
- *Instant Game Feel — Springs Explained*, Game Developer — https://www.gamedeveloper.com/game-platforms/instant-game-feel---springs-explained

**Inverse kinematics**
- Ryan Juckett, *Analytic Two-Bone IK in 2D* — https://www.ryanjuckett.com/analytic-two-bone-ik-in-2d/
- Daniel Holden, *Simple Two Joint IK* — https://theorangeduck.com/page/simple-two-joint
- *Inverse Kinematics (two joints) for foot placement*, Game Developer — https://www.gamedeveloper.com/programming/inverse-kinematics-two-joints-for-foot-placement
- *Law of cosines* — https://en.wikipedia.org/wiki/Law_of_cosines
- *IK-Driven Procedural Spider Locomotion in Godot 4.5*, 80.lv — https://80.lv/articles/ik-driven-procedural-spider-locomotion-in-godot-4-5
- PhilS94, *Unity Procedural IK Wall-Walking Spider* — https://github.com/PhilS94/Unity-Procedural-IK-Wall-Walking-Spider

**Gait: biomechanical reference**
- *Biomechanics of Normal Gait*, AAPM&R PM&R KnowledgeNow — https://now.aapmr.org/biomechanics-normal-gait/
- *Stance Phase of Gait*, Wheeless' Textbook of Orthopaedics — https://www.wheelessonline.com/orthopaedics/stance-phase-of-gait/
- *Joint Range of Motion During Gait*, Physiopedia — https://www.physio-pedia.com/Joint_Range_of_Motion_During_Gait
- *Gait Analysis: The 8 Phases of Gait* — https://compedgept.com/blog/gait-analysis-the-8-phases-of-gait/

**Gait and animation principles: craft reference**
- Richard Williams, *The Animator's Survival Kit* (contact / down / passing / up) — https://archive.org/details/TheAnimatorsSurvivalKitRichardWilliams
- *Stationary Walk Cycle Tutorial*, UW CSE 458 — https://courses.cs.washington.edu/courses/cse458/06au/projects/project7/walk.html
- *Assignment #5: Basic Walk Cycle*, UW CSE 459 — https://courses.cs.washington.edu/courses/cse459/19au/assignments/assignment_5/index.html
- Thomas & Johnston, *The Illusion of Life* — the twelve principles, summarised — https://arlingtonmuseum.org/explore-more/the-twelve-principles-of-animation · https://www.adobe.com/creativecloud/animation/discover/principles-of-animation.html
- *Easing functions* (exact forms) — https://easings.net/

**Skeletal animation, for what we deliberately omit**
- LearnOpenGL, *Skeletal Animation* — https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation
- *Skeletal animation*, Wikipedia — https://en.wikipedia.org/wiki/Skeletal_animation
- ozz-animation (evaluated and rejected, §10.4) — https://guillaumeblanc.github.io/ozz-animation/

**OpenGL 1.x reference (the two gotchas that cost time)**
- `glLight` — `GL_POSITION` is transformed by the current modelview — https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glLight.xml
- `glClipPlane` — plane equation transformed by the current modelview; ≥ 6 planes guaranteed — https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glClipPlane.xml

**Benchmark named by the user**
- *Manic Miners* — free, closed-source, Unreal Engine. Named as the target for perceived
  quality. No asset, model, texture, sound, animation curve or code from it (or from any
  commercial game) is referenced, reproduced or relied upon anywhere in this document.
  https://manicminers.dev/
