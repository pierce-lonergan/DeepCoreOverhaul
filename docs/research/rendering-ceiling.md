<!-- Research document. Authored by source reading and reproducible read-only shell commands
     only. No build was run for this document; no game was run for this document (there is no
     installation on the authoring machine). Every number below is re-derivable from the
     commands given. Anything that would require observing the game running is marked
     UNDETERMINED and is never asserted. -->

# The Direct3D Retained Mode ceiling — measured, and what to do about it

Repo: `C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`
Branch `main`, HEAD `ea63ee1` at time of writing.

**Headline, stated up front so nobody has to read to the end.**

1. The Retained Mode coupling is **far smaller and far more localised than the project's
   reputation suggests**: 39 of 229 source files mention D3DRM at all, 23 touch an
   `IDirect3DRM*` interface, and **`game/` contains exactly 3 such files**
   (`game/audio/SFX.{h,cpp}`, `game/world/Roof.cpp`) totalling 10 references.
2. **A partial immediate-mode path is not "reportedly" there — it is the main path.** Every
   animated model, creature, vehicle, building, weapon beam and particle in the game is drawn
   by `Gods98::Mesh_RenderTriangleList` calling `IDirect3DDevice3::DrawIndexedPrimitive`
   (`engine/gfx/Mesh.cpp:2443`). Retained Mode is not the rasteriser. It is the **scene graph,
   the culler, the light rig and the projection matrix** — and it invokes our own code through
   an `IDirect3DRMUserVisual` callback (`engine/gfx/Mesh.cpp:156`).
3. **Resolution is not fixed.** Two independent, already-implemented seams exist:
   `-res <W>x<H>` (`engine/Main.cpp:1166-1182`) and, more interestingly, `Main_RenderScale()`
   (`engine/Main.cpp:112-120`) — a complete resolution-independence layer already threaded
   through DirectDraw, the D3DRM viewport, image blits, the Draw primitives, FLIC, AVI and the
   radar map. What blocks arbitrary resolution is **not** the renderer. It is four
   `sprintf("...%ix%i", appWidth(), appHeight())` config-key lookups, three of which are in
   **our own C++**.
4. Therefore the recommended workstream is **not** a backend rewrite. It is a
   `DeepCore` **Display layer** that decouples the HUD's *layout* coordinate space from the
   *render* resolution, unlocking arbitrary resolution and widescreen. It is a change to files
   we already own, it is compile-verifiable, and it is gated off by default.

Everything below shows the work.

---

## 0. Ground rules honoured by this document

- **OURS** = an implemented C++ body in this DLL that is installed over the exe by
  `hook_write_jmpret` in `src/openlrr/interop.cpp`. Editing it changes behaviour.
- **EXE** = a live raw-address macro, e.g.
  `#define Lego_LoadPanels ((void (__cdecl*)(const Gods98::Config*, uint32, uint32))0x00434520)`
  (`game/interface/../Game.h:1902`). Editing the `.cpp` changes nothing.
- **There is no trampoline.** `hook_write_jmpret` overwrites 6 prologue bytes with `E9 rel32` +
  `C3` and every restore path in `hook.cpp:30-32,47-49` is commented out; 1515 hook
  installations, zero backup buffers. Any re-implementation must be **complete**.
- **The cardinal rule.** `Container` (`assert_sizeof(Container, 0x2c)`, `gfx/Containers.h:326`),
  `Mesh` (`0x34`, `gfx/Mesh.h:306`) and `Viewport` (`0x20`, `gfx/Viewports.h:83`) are pinned,
  and their owning globs are references overlaid on the exe's data segment
  (`Containers.cpp:44` @`0x0076bd80`, `Mesh.cpp:29` @`0x005353c0`, `Viewports.cpp:25`
  @`0x0076bce0`). **No renderer handle may be added to any of them.** Side tables only —
  the pattern already proven by the water relocation.
- **We cannot run the game.** Compile-verified is the ceiling. No claim below is behavioural.

---

## 1. Quantifying the coupling

### 1.1 Reproducible commands

```bash
# Files mentioning D3DRM in any form (types, enums, interfaces, header names)
grep -rlE 'IDirect3DRM|D3DRM|d3drm' --include=*.h --include=*.hpp --include=*.cpp src/ | wc -l   # 39

# Files touching an actual COM interface pointer
grep -rlE 'IDirect3DRM' --include=*.h --include=*.hpp --include=*.cpp src/ | wc -l               # 23

# Total source files in the DLL
find src/openlrr -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) | wc -l             # 229

# Per-directory
for d in $(find src -type d | sort); do
  files=$(find "$d" -maxdepth 1 -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \))
  [ -z "$files" ] && continue
  n=$(grep -hcE 'IDirect3DRM|D3DRM|d3drm' $files 2>/dev/null | awk '{s+=$1} END {print s+0}')
  nf=$(grep -lE 'IDirect3DRM|D3DRM|d3drm' $files 2>/dev/null | wc -l)
  tf=$(echo "$files" | wc -l)
  printf "%-42s files=%2d/%2d refs=%d\n" "$d" $nf $tf $n
done
```

### 1.2 Per-directory table (measured)

| Directory | files touching D3DRM / total | D3DRM references | Verdict |
| --- | ---: | ---: | --- |
| `src/openlrr/engine/gfx/` | **13 / 16** | **489** | **The blast radius.** |
| `src/openlrr/engine/` (root) | 5 / 9 | 67 | `Graphics.{h,cpp}`, `Main.{h,cpp}`, `geometry.h` |
| `src/openlrr/engine/core/` | 2 / 17 | 62 | `Maths.{h,cpp}` only — `D3DVECTOR` typedef aliasing, **not** RM |
| `src/openlrr/engine/audio/` | 3 / 4 | 39 | 3D sound is positioned off `IDirect3DRMFrame3` |
| `src/openlrr/engine/util/` | 2 / 6 | 32 | `Dxbug.{h,cpp}` — HRESULT decoding tables |
| `src/openlrr/platform/` | 4 / 10 | 26 | The shim + 9 IID bindings |
| `src/openlrr/engine/drawing/` | 6 / 14 | 18 | Mostly forward decls; `Draw.cpp` has one IM routine |
| `src/openlrr/game/audio/` | 2 / 2 | 6 | `SFX` passes a raw `IDirect3DRMFrame3*` |
| `src/openlrr/game/world/` | 1 / 24 | 4 | `Roof.cpp` — only for the `D3DRMGROUPINDEX` typedef |
| `src/openlrr/` (root) | 1 / 11 | 1 | one include |
| `game/object/`, `game/interface/`, `game/front/`, `game/mission/`, `game/effects/`, `game/interface/hud/`, `cmdline/`, `engine/input/`, `engine/video/` | **0** | **0** | **Completely RM-free** |

**The single most important number in this document: `game/` — 285 files of gameplay code —
contains 3 files and 10 references to D3DRM, and two of those files only need a typedef.**
The renderer boundary is already inside `engine/gfx/`.

### 1.3 Interface-level breakdown (what RM object types we actually name)

```bash
grep -rhoE 'IDirect3DRM[A-Za-z0-9]*' --include=*.h --include=*.cpp src/openlrr | sort | uniq -c | sort -rn
```

| Interface | refs | What it is to us |
| --- | ---: | --- |
| `IDirect3DRMFrame3` | **254** | The scene graph node. **This is the coupling.** |
| `IDirect3DRMMesh` | 45 | RM's *own* mesh — terrain, roof, water, select cursor |
| `IDirect3DRMVisual` | 25 | base of what hangs off a frame |
| `IDirect3DRMViewport2` | 24 | camera + projection + `Render()` |
| `IDirect3DRM3` | 20 | the factory |
| `IDirect3DRMUserVisual` | 15 | **the callback that hands rendering back to us** |
| `IDirect3DRMMaterial`/`Material2` | 20 | per-group material |
| `IDirect3DRMDevice3` | 12 | quality/dither/texture-quality knobs |
| `IDirect3DRMTexture3`/`Texture` | 16 | textures + MIP generation |
| `IDirect3DRMAnimationSet`/`2` | 15 | LightWave scene animation playback |
| `IDirect3DRMLight` | 7 | lights |
| `IDirect3DRMMeshBuilder2`/`3` | 7 | `.lwo` load path |

### 1.4 Is there an existing seam a different backend could slot into?

**Yes — three, and they are of very different quality.**

| Seam | Where | Quality |
| --- | --- | --- |
| **`Gods98::Container`** (OURS) | `engine/gfx/Containers.{h,cpp}`, 3902 lines, 151 RM refs in the `.cpp` | The **intended** abstraction. All of `game/` goes through it. But it is a *thin* wrapper: `Container` (`Containers.h:310-326`) stores `IDirect3DRMFrame3* masterFrame` and `* activityFrame` **as public fields** and 254 references reach through them. The seam exists; the abstraction leaks. |
| **`Gods98::Mesh` + `Mesh_RenderCallback`** (OURS) | `engine/gfx/Mesh.cpp:1274-1426`, `:2389-2454` | **The best seam in the codebase.** RM does not draw our models; it *calls us* and we draw them with immediate mode. See §3. |
| **`platform/d3drm.{h,cpp}` + `lib/d3drm/d3drm.lib`** | `platform/d3drm.h:60-63`, `openlrr.vcxproj:110,140` | A link-time seam. `::Direct3DRMCreate` is resolved through our own vendored import library, and `legacy::Direct3DRMCreate` is a thunk into the exe's copy at `0x0049b970`. A drop-in `d3drm.dll` replacement is *architecturally* possible, but it means implementing the COM interfaces in §2, not "slotting in a backend". |

There is **no** `IRenderBackend`-style interface in the tree. Nothing dispatches on a renderer
type. Searching for one returns nothing.

---

## 2. What Retained Mode actually owns

Method surface actually called, enumerated mechanically:

```bash
grep -rhoE 'lpD3DRM\(\)->[A-Za-z_][A-Za-z0-9_]*' --include=*.cpp src/openlrr | sed 's/.*->//' | sort -u
grep -rhoE 'lpVP->[A-Za-z_][A-Za-z0-9_]*'        --include=*.cpp src/openlrr | sed 's/.*->//' | sort -u
grep -rhoE '(masterFrame|activityFrame|frameCreatedOn|scene|camera|frame)->[A-Za-z_][A-Za-z0-9_]*\(' \
    --include=*.cpp src/openlrr | sed 's/.*->//' | sort -u
```

| RM responsibility | Owner | Wrapped by OpenLRR C++? | Evidence |
| --- | --- | --- | --- |
| **Scene graph (frames)** | RM, 100% | **`Gods98::Container` — OURS**, but leaky (fields are public `IDirect3DRMFrame3*`) | `Containers.h:310-326`; 39 distinct frame methods used: `AddChild AddDestroyCallback AddLight AddMoveCallback AddRotation AddScale AddTransform AddTranslation AddVisual DeleteVisual GetAppData GetChildren GetName GetOrientation GetParent GetPosition GetScene GetSceneBackground GetTransform GetVisuals InverseTransform Load LookAt QueryInterface Release SetAppData SetColor SetMaterialMode SetName SetOrientation SetPosition SetSceneBackground SetSceneFog{Color,Enable,Method,Mode,Params} SetSortMode Transform` |
| **Model meshes (characters, buildings, FX)** | **NOT RM — ours, via immediate mode** | **`Gods98::Mesh` — OURS** | `Mesh.cpp:156` creates an `IDirect3DRMUserVisual`; `Mesh.cpp:2443` issues `DrawIndexedPrimitive`. See §3. |
| **Static/world meshes (terrain, roof, water, cursor)** | RM `IDirect3DRMMesh` | `Container_MakeMesh2` / `Container_Mesh_*` — **OURS** | `Containers.cpp:883-946`, `:1873-1930`. Callers: `Map3D_LoadSurfaceMap` (EXE, via our hook — `interop.cpp:448`), `Roof.cpp:30`, `SelectPlace.cpp:115,277`, `Water.cpp:691` |
| **Textures** | RM (`CreateTextureFromSurface`, `LoadTexture`, `GenerateMIPMap`) | `Container_LoadTexture` etc. — **OURS** | `Containers.cpp:1657,1674-1675,3813-3814` |
| **Materials** | RM `IDirect3DRMMaterial2` | `engine/gfx/Materials.{h,cpp}` — **OURS**, 71 lines | `Materials.cpp` |
| **Lights** | RM `IDirect3DRMLight` | `Container_MakeLight` — **OURS and hooked** | `Containers.cpp:852-880`, hook at `interop.cpp:444`. **Only colour-at-creation is exposed**; there is no `Container_Light_SetColour`. |
| **Viewport / projection / FOV / clip planes** | RM `IDirect3DRMViewport2` | `Gods98::Viewport` — **OURS** | `Viewports.cpp:104` `CreateViewport`, `:284` `Render`, `:321-341` `SetField`/`GetField`, `:175-203` back/front clip |
| **Traversal, culling, draw order** | **RM, exclusively** | **not wrapped at all** | `Viewports.cpp:284`: `vp->lpVP->Render(root->masterFrame)` is one call that does the whole frame. Our only hooks into it are the `D3DRMUSERVISUAL_CANSEE` / `_RENDER` reasons (`Mesh.cpp:1280,1315`). |
| **Animation (`.lws` playback)** | RM `IDirect3DRMAnimationSet2` | `Lws` + `AnimClone` — **OURS** | `Containers.cpp:3644-3667`, `AnimClone.cpp:321-322` |
| **Picking** | **NOT RM** | ours, hand-rolled | `Viewport_Pick`/`Viewport_PickInfo` are declared (`Viewports.h:86-103`) and **never used anywhere**. Real picking is a ray built from `Viewport_InverseTransform` plus our own maths — `Game.cpp:1203-1216`, `Game.cpp:1350-1354`. |
| **Fog** | RM scene fog on the root frame | `Container_EnableFog / SetFogColour / SetFogMode / SetFogParams` — **OURS** | `Containers.cpp:1326-1352`; game-side driver `Game.cpp:1093-1115` and `Game.cpp:2843-2864` (the latter is marked `/// CUSTOM:` — already ours) |
| **Device state (quality, dither, texture filter)** | RM `IDirect3DRMDevice3` | `Graphics_Setup3D` — **OURS and hooked** | `Graphics.cpp:237-296`, hook `interop.cpp:1163`. Its only caller is EXE `Lego_LoadGraphicsSettings` @`0x00433420` (`Game.h:1861`), so **we see and can override every value it reads out of Lego.cfg.** |
| **Rasterisation, render states, textures-per-draw, blend modes** | **NOT RM — immediate mode** | `Graphics_ChangeRenderState`, `Mesh_ChangeTextureStageState` — **OURS** | `Graphics.cpp:314-361`, `Mesh.cpp:2117-2150` |

**Summary in one line:** RM owns *frames, traversal, lights, projection, animation playback and
four kinds of static mesh*. It does **not** own the pixels of anything that moves.

---

## 3. Immediate mode — verified, and what it means

### 3.1 The grep, and the answer

```bash
grep -rnE 'IDirect3DDevice|DrawIndexedPrimitive|ExecuteBuffer|IDirect3DViewport' \
     --include=*.h --include=*.cpp src/
```

`ExecuteBuffer`: **zero hits anywhere.** The 1997-era execute-buffer path is not used.

The rest, in full:

| Site | File:line | Status |
| --- | --- | --- |
| IM device acquired from the RM device | `engine/Graphics.cpp:189-192` | **live** — `mainGlobs.device->GetDirect3DDevice2(&imdev2); imdev2->QueryInterface(IID_IDirect3DDevice3, ...)` |
| IM device accessor | `engine/Graphics.h:166` | `__inline IDirect3DDevice3* lpIMDevice() { return mainGlobs.imDevice; }` |
| Stored on the exe-overlaid glob | `engine/Main.h:216` | `/*11c,4*/ IDirect3DDevice3* imDevice;` |
| **`DrawIndexedPrimitive`** | **`engine/gfx/Mesh.cpp:2443`** | **live, the main geometry path** |
| `DrawPrimitive(D3DPT_LINELIST)` | `engine/drawing/Draw.cpp:345` | live but in an `// <unused>` function |
| Commented-out `DrawIndexedPrimitive` | `engine/drawing/Draw.cpp:347-351` | dead |
| `SetCurrentViewport` (IM) | `Mesh.cpp:2051,2082`, `Draw.cpp:329` | live |
| `IDirect3DViewport3::Clear2` | `engine/gfx/Viewports.cpp:238-248` | live — the screen clear is immediate mode |
| 38 further `lpIMDevice()->` calls | `Mesh.cpp`, `Graphics.cpp`, `Draw.cpp` | `BeginScene EndScene Get/SetRenderState Get/SetTexture Get/SetTextureStageState Get/SetTransform Get/SetLightState GetDirect3D QueryInterface` |

### 3.2 What the partial IM path actually is

It is not a side path. It is **the** path, reached through an RM callback:

```cpp
// engine/gfx/Mesh.cpp:148-160  — OURS
Gods98::Mesh* __cdecl Gods98::Mesh_CreateOnFrame(IDirect3DRMFrame3* frame, ...)
{
    ...
    lpD3DRM()->CreateUserVisual(Mesh_RenderCallback, mesh, &mesh->uv);   // :156
    frame->AddVisual((IUnknown*)mesh->uv);
    ...
}
```

RM then calls back twice per mesh per frame (`Mesh.cpp:1274-1426`, OURS):

- `D3DRMUSERVISUAL_CANSEE` (`:1280`) — our visibility veto (hidden flag, container flag,
  billboard `LookAt`).
- `D3DRMUSERVISUAL_RENDER` (`:1315`) — we read RM's composed world matrix straight off the IM
  device (`lpIMDevice()->GetTransform(D3DTRANSFORMSTATE_WORLD, ...)`, `:1329`), set our own
  render states, and draw:

```cpp
// engine/gfx/Mesh.cpp:2389-2454  — OURS
bool32 __cdecl Gods98::Mesh_RenderTriangleList(D3DMATERIALHANDLE matHandle,
        IDirect3DTexture2* texture, Mesh_RenderFlags renderFlags,
        Mesh_Vertex* vertices, uint32 vertexCount, uint16* faceData, uint32 indexCount)
{
    ...
    if (lpIMDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, renderFlags,
                                           vertices, vertexCount, faceData, indexCount, 0) != D3D_OK)
    { Error_Warn(true, "Cannot 'DrawIndexedPrimitive'."); ok = false; }
    ...
}
```

Note the `renderFlags` parameter is doing double duty as the **FVF code**:
`MESH_RENDERFLAGS_DEFAULT = MESH_RENDER_FLAG_XYZ|_NORMAL|_TEX1` with the comment
`// = D3DFVF_VERTEX` (`Mesh.h:165`), and `Mesh_Vertex` is exactly `{pos, normal, tu, tv}`,
`assert_sizeof(..., 0x20)` (`Mesh.h:259-268`). This is a plain lit-textured vertex stream.

**Consequences, stated precisely:**

- Replacing the rasteriser for *models* does not require touching RM at all. It requires
  rewriting one 65-line function (`Mesh_RenderTriangleList`) and the state-setting helpers
  around it (`Mesh_SetRenderDesc`, `Mesh.cpp:1494-1617`; `Graphics_ChangeRenderState`,
  `Graphics.cpp:314-342`).
- What you would *not* get from that is a new scene graph, culling, lighting or projection —
  those still come from RM, and RM would still need a device to render *its* own meshes
  (terrain/roof/water) into.
- `Draw_WorldLineListEx` (`Draw.cpp:299-365`, `// <unused>`) is a **complete, self-contained
  `BeginScene`/`DrawPrimitive`/`EndScene` block** that already sets its own transform, texture,
  material and blend states. It is an existence proof that we can issue arbitrary immediate-mode
  work inside the RM frame, and it is dead code we could revive without risk to anything.

---

## 4. The honest menu, ranked by (visual impact) / (risk × effort)

Ranking is mine; the facts under each item are cited.

### (a) Available today with **zero code** — configuration and command line

All parsed in `Gods98::Main_ParseCommandLineOptions` (`engine/Main.cpp:989-1388`, OURS).

| Option | Line | What it does | Impact |
| --- | ---: | --- | --- |
| **`-scale <N>`** | `Main.cpp:1263-1266` | Sets `mainGlobs2.windowScale` **and** `renderScaling = true`, which makes `Main_RenderScale()` return N (`Main.cpp:112-120`). The DirectDraw back surface is created at `width*N × height*N` (`DirectDraw.cpp:322-323`), the D3DRM viewport is created at N× (`Viewports.cpp:85-88`), 2D blits (`Images.cpp:407`), Draw primitives (`Draw.cpp:165-177`), FLIC (`Flic.cpp:379-382`), AVI (`Animation.cpp:577-580`, `Movie.cpp:180-183`) and the radar (`RadarMap.cpp:190`) all follow, and mouse→world is un-scaled on the way back (`Viewports.cpp:353-354,375-376`). **This is a true higher-resolution render, not an upscale.** | **Very high.** Biggest free win in the tree. |
| `-nnscale <N>` | `Main.cpp:1256-1259` | Same window size, `renderScaling = false` → nearest-neighbour magnification of a 640×480 image. | Low (pixel-art look) |
| `-radarscale <N>` | `Main.cpp:1271-1273` | Radar pixel density independent of render scale. Defaults to `ceil(renderScale/2)` (`Main.cpp:131`). | Low |
| `-res <W>x<H>` | `Main.cpp:1166-1182` | Feeds `Init_Initialise` (`Init.cpp:70`) which registers it as the **first** valid mode (`Init.cpp:83-97`). The header comment is candid: `// -res <W>x<H> : Overrides the display resolution (not supported by engine)` (`Main.h:285`). | High **but breaks the HUD** — see §5 |
| `-bpp 8\|16\|24\|32` | `Main.cpp:1206-1214` | Colour depth. Default 16 (`Init.cpp:71`). 32-bit removes dithering banding. | Medium |
| `-window` / `-fullscreen` | `Main.cpp:1025-1026` | Windowed is fully supported (`DirectDraw.cpp:317-324`, `:441-466`). | — |
| `-ftm` / `-nm` | `Main.cpp:1032-1033` | Texture management. `Graphics_ManageTextures()` (`Graphics.h:188`) gates the modern `SetTexture` path vs the legacy `D3DRENDERSTATE_TEXTUREHANDLE` path (`Mesh.cpp:2395-2423`). | Medium |
| `-fvf` | `Main.cpp:1034` | Forces vertex fog instead of table fog (`Graphics.cpp:166-172`). | Low |
| `-fpscap` / `-fpslock` | `Main.cpp:1130-1143` | Frame pacing. | Perceptual, high |
| `-clipcursor` | `Main.cpp:1280-1286` | Cursor trapping. | Quality-of-life |

**Compile-verifiable: N/A (no code change).** Behaviour: **UNDETERMINED** — none of this has
been observed running by this project.

> **Caveat worth recording.** `-scale` in **fullscreen** is suspect. `Main_IsScaleSupported`
> returns `false` when `Main_FullScreen()` (`Main.cpp:146-147`), but the command-line parser
> writes `mainGlobs2.windowScale` **directly** without consulting it (`Main.cpp:1264`). In
> fullscreen the back buffer is the attached flip surface, not one we size (`DirectDraw.cpp:
> 325-330`), so `Main_RenderScale()` would scale 2D drawing beyond the surface. Windowed is the
> supported combination. UNDETERMINED, flagged for anyone who can test.

### (b) Achievable by editing **our own C++ inside the RM path** — the sweet spot

Every item here is a body we already own and that is already hooked.

| # | Change | File(s) | Ownership | Compile-verifiable |
| --- | --- | --- | --- | --- |
| b1 | **Texture filtering / MIP quality override.** `Graphics_Setup3D` receives `linearFilter/mipMap/mipMapLinear` from EXE `Lego_LoadGraphicsSettings` and turns them into `D3DRMTEXTURE_*`. Override the parameters from `DeepCore` before use. Note `Mesh.cpp:1597-1608` currently **hard-disables** `D3DFILTER_LINEARMIPLINEAR` (trilinear) behind a commented-out branch, despite `Lego.cfg` shipping `LinearMipMap TRUE`. Re-enabling trilinear behind a gate is a 3-line change. | `engine/Graphics.cpp:237-296`, `engine/gfx/Mesh.cpp:1589-1616` | **OURS** (hooks `interop.cpp:1163`) | **Yes** |
| b2 | **Render quality / shade mode.** `Graphics_Quality::{Wireframe,UnlitFlat,Flat,Gouraud,Phong}` is fully plumbed (`Graphics.h:86-101`, `Mesh.cpp:1441-1468`) but `Phong` is annotated `// CUSTOM: … will never be passed as a parameter` (`Graphics.cpp:257`). Exposing it is a config read. | `engine/Graphics.cpp:250-259` | **OURS** | **Yes** |
| b3 | **Fog.** `Lego_SetSceneFogParams` is already `/// CUSTOM:` — a function this project owns outright — and hard-codes `density = 0.0032f`, a `3/4` start ratio, and a `0.8` topdown density multiplier. Multipliers from config are trivial. Fog colour cycling lives at `Game.cpp:1100-1112`. | `game/Game.cpp:2842-2864`, `:1093-1115`; `engine/gfx/Containers.cpp:1326-1352` | **OURS** | **Yes** |
| b4 | **Draw distance.** `legoGlobs.TVClipDist` (cfg `Main::tvclipdist`) and `FPClipBlocks` are read at `GameState.cpp:589` and applied at `:591-592` and `Game.cpp:2886,2901`. A `DrawDistanceScale` multiplier applied at those four sites raises the far plane. **Must** be applied to fog too or the world fades into grey short of the clip plane — which is exactly why b3 and b4 are one work item. | `game/GameState.cpp:589-592`, `game/Game.cpp:2886,2901,2850-2857` | **OURS** | **Yes** |
| b5 | **Field of view.** `Viewport_SetField(viewMain, 0.5f)` for topdown (`Game.cpp:2900`) and `0.9f/0.6f` for first-person (`Game.cpp:2870`) are literals in our code. `Viewport_SmoothSetField` gives free interpolation (`Viewports.cpp:269-281`). | `game/Game.cpp:2867-2907` | **OURS** | **Yes** |
| b6 | **Light rig.** `Container_MakeLight(parent, type, r,g,b)` is OURS and hooked (`interop.cpp:444`); EXE `Lego_LoadLighting` @`0x004336a0` calls straight into it. Intercepting there lets us scale/tint/retype **every light the game creates** without touching the loader. There is no setter after creation, so this is the *only* interception point. | `engine/gfx/Containers.cpp:852-880` | **OURS** | **Yes** |
| b7 | **Per-object colour / alpha / emissive.** `Container_SetColourAlpha`, `Container_Mesh_SetColourAlpha`, `Container_Mesh_SetEmissive` (`Containers.h:677-691`). Already used in production by the shipped creature-variant tint. Reuse for power-state glow, damage tint, ownership colour. | `engine/gfx/Containers.cpp` | **OURS** | **Yes** |
| b8 | **Alpha-modulation switch.** `Graphics_SetAlphaModulation` (`Graphics.h:238-241`) toggles the already-applied fix that made lava smoke fade out instead of popping (`Mesh.cpp:1527-1563`). Currently reachable only from code. | `engine/gfx/Mesh.cpp:1546-1563` | **OURS** | **Yes** |
| b9 | **Revive `Draw_WorldLineListEx`.** A complete IM line renderer sitting unused (`Draw.cpp:299-365`). Gives world-space debug/holo overlays with no RM involvement. | `engine/drawing/Draw.cpp` | **OURS** | **Yes** |
| b10 | **LOD.** `MeshLOD` is a *set-ID mesh swap*, not distance LOD (`object/MeshLOD.cpp:98-115`); callers pass `setID = 0` or the FP camera number (`Creature.cpp:174-186`, `Vehicle.cpp:506-509`). There is **no distance term anywhere**. Adding one means inventing the policy. | `game/object/MeshLOD.cpp` | **OURS** | Yes, but **new behaviour**, not a tweak |

**Risk profile for the whole of (b): low.** Every item is a value change or an added branch
inside a function we already replaced wholesale. None resizes a pinned struct. None adds a new
hook. All are gateable to vanilla-by-default in `DeepCore.cfg`.

### (c) An RM → immediate-mode bridge

**What it would actually require.** Not "port `Mesh.cpp`" — that is already done. It means
replacing what RM does *around* our draw calls:

1. **A frame graph.** 254 `IDirect3DRMFrame3` references, 39 distinct methods. Transform
   composition, parent/child, `AddMoveCallback`/`AddDestroyCallback` semantics,
   `SetSortMode`, `LookAt` with `D3DRMCONSTRAIN_Z`, `AddRotation` accumulation.
   `Containers.cpp` alone is 3902 lines.
2. **Traversal, culling and draw ordering** to replace the single `lpVP->Render(rootFrame)`
   (`Viewports.cpp:284`) — including the `CANSEE`/`RENDER` two-phase contract our own
   `Mesh_RenderCallback` depends on, and the post-render alpha list (`Mesh.cpp:1394-1395`,
   `Mesh_PostRenderAll` at `Viewports.cpp:286`).
3. **A second mesh renderer** for RM-native `IDirect3DRMMesh` (45 refs) — terrain
   (`Map3D_LoadSurfaceMap`, **EXE**), roof, water, select cursor — including
   `SetGroupMapping`, `SetGroupQuality`, `SetGroupTexture`, `Scale`, `GetBox`.
4. **Lighting**, because RM currently converts `IDirect3DRMLight` into IM light state for us.
5. **`.lws` animation playback** (`IDirect3DRMAnimationSet2`, `Containers.cpp:3644-3667`) and
   `.lwo` `IDirect3DRMMeshBuilder2::CreateMesh`.
6. **Projection / `InverseTransform`**, which the *gameplay* depends on for mouse picking
   (`Game.cpp:1203`) and radar arrows (`RadarMap.cpp:497-530`).

**Files: 13 in `engine/gfx/` (~9,900 lines), plus `engine/Graphics.{h,cpp}`,
`engine/Main.h`, `engine/audio/3DSound.{h,cpp}` (frames as sound emitters, 35 refs),
`engine/drawing/DirectDraw.{h,cpp}`, `platform/d3drm.{h,cpp}`, and `game/audio/SFX.{h,cpp}`
which passes raw frames. ~19 files.**

**And a hard constraint most plans miss:** every one of the 876 unimplemented exe functions
that manipulates 3D state does so *through our hooked `Container_*` / `Mesh_*` / `Viewport_*`
functions* — that is why this is even conceivable. But it also means the bridge must preserve
those functions' exact signatures and observable behaviour, with **no trampoline to fall back
to**, for code we cannot read.

**Is any of it verifiable without running the game?** Partially and unsatisfyingly:

- A frame-graph implementation is ordinary C++ and **could** carry unit tests
  (transform composition, hierarchy, `LookAt`) that run on the build machine. That is real
  verification of a real component. Precedent exists: `tools/addrlint/test_addrlint.py`.
- Everything downstream of `IDirect3DRMViewport2::Render` — culling correctness, draw order,
  z-fighting, alpha sorting, light falloff — is **UNVERIFIABLE here**. It is exactly the class
  of thing that looks fine and is wrong.

**Verdict: not compile-verifiable in any meaningful sense.** Do not start it.

### (d) A full modern backend (D3D9/11, or an OpenGL/Vulkan translation layer)

The real cost, stated plainly:

- Everything in (c), **plus** a shader pipeline replacing the fixed-function state machine
  encoded across `Graphics_ChangeRenderState` (200 render-state slots, `Main.h:62,229`) and
  `Mesh_ChangeTextureStageState` (50 stage slots, `Mesh.h:99,333`).
- **Plus DirectDraw.** `engine/drawing/DirectDraw.cpp` is 1175 lines and the entire 2D layer —
  Draw, Images, Fonts, Bmp, Flic, TextWindow, the movie player — draws by **locking a surface
  and writing pixels** (`Draw.cpp:73-76`, `Draw_Pixel16Address` and friends). A modern backend
  must either emulate a lockable surface or rewrite all of `engine/drawing/` (14 files).
- **Plus the cardinal rule.** `Container` is 0x2c bytes and pinned; `Mesh` is 0x34 and pinned;
  `Viewport` is 0x20 and pinned. **No GPU handle may be stored in any of them.** Every
  container/mesh/viewport needs a DLL-side side table keyed by pointer, on the hot path, in a
  32-bit process.
- **Plus 32-bit-forever.** `openlrr.sln` has x64 configurations; `DIRECTORY.md:80-83` records
  they "cannot work — the whole design is pinned to 32-bit absolute addresses."
- **Plus the warning budget.** 0 errors / exactly 44 warnings at `/W3` (`openlrr.vcxproj:99,
  126`). A new backend that adds one warning fails the contract.

**Verdict: this is a different project.** It is the kind of thing that is finished by someone
who can run the game, in a fork whose stated goal is the renderer. Recording the cost is the
useful output; attempting it is not.

---

## 5. The display / resolution path — assessed seriously

### 5.1 Is resolution fixed? **No.** Where does it come from?

```
Main_ParseCommandLineOptions  (Main.cpp:1166-1182)   -res W×H  →  mainOptions.res
        │
Init_Initialise               (Init.cpp:56-165)
        ├─ cmdRes = mainOptions.res.value_or({640,480})            :70
        ├─ Init_AddValidMode(cmdRes, each bpp)                     :94-97
        ├─ (+ 800×600, 1024×768 only when -debug)                  :87-88,99-104
        ├─ DirectDraw_EnumModes → driver's real mode list          :119
        ├─ Init_IsValidMode filters enumerated ∩ registered        :347-366
        └─ DirectDraw_SetupFullScreen / SetupWindowed              :154-156
                │
DirectDraw_Setup              (DirectDraw.cpp:268-365)
        ├─ directDrawGlobs.{width,height} = mode->{width,height}   :285-286
        ├─ Main_SetupDisplay(...)  →  mainGlobs.{appWidth,appHeight}  Main.cpp:1666-1667
        ├─ back surface = width*RenderScale × height*RenderScale   :322-323
        └─ Graphics_SetupDirect3D → CreateDeviceFromSurface        :334, Graphics.cpp:187
                │
Viewport_Create(0,0,1,1,camMain)  (GameState.cpp:385 → Viewports.cpp:58-74)
        └─ fractional, so the 3D view **already fills any resolution**
```

`Init_AddValidMode` has **no bounds check** against `GRAPHICS_MAXMODES = 200`
(`Init.cpp:326-343`; cap at `DirectDraw.h:64`; `validModes[]` at `Init.h:38`). Today it is
called at most `2 sizes × 4 bpp` plus 2 debug sizes, so it is unreachable. **Any change that
enumerates a mode list into it makes it reachable** — worth a guard in the same commit, and it
belongs on the `docs/research/silent-failures.md` list.

### 5.2 What breaks at a non-640×480 resolution

I searched for hard-coded dimensions across the whole DLL:

```bash
grep -rhoE '\b640\b' --include=*.cpp --include=*.h src/openlrr | wc -l   # 10
grep -rhoE '\b480\b' --include=*.cpp --include=*.h src/openlrr | wc -l   # 14
```

Most of those are `/*offset,size*/` struct comments. The genuine ones are few — the engine is
**already** written against `Gods98::appWidth()` / `appHeight()` (`Main.h:564-565`). So the
breakage is not arithmetic. It is **four resolution-keyed config lookups**:

| Site | Key built | Ownership | Consequence when the key is absent |
| --- | --- | --- | --- |
| `game/GameState.cpp:802` | `Lego_LoadPanels(cfg, appWidth(), appHeight())` → reads `Panels<W>x<H>` | **EXE** `@0x00434520` (`Game.h:1902`) | **No HUD panels load at all** — not misplaced, *absent*. `PANEL_FLAG_HASIMAGE` never set (`Panels.h:54`). |
| `game/GameState.cpp:811` | `Lego_LoadPanelButtons(cfg, appWidth(), appHeight())` | **EXE** `@0x00434640` (`Game.h:1906`) | No panel buttons. |
| `game/GameState.cpp:461,468,481,489` | `NextButton%ix%i`, `NextButtonPos%ix%i`, `BackButton%ix%i`, `BackButtonPos%ix%i` | **OURS** | Tutorial next/back buttons vanish. |
| `game/interface/Advisor.cpp:188` | `AdvisorPositions%ix%i` | **OURS** | Advisor appears at (0,0) or not at all. |

Secondary, all in **our** code and all one-liners:

| Site | Literal | Fix |
| --- | --- | --- |
| `game/GameState.cpp:219` | `Interface_Initialise(565, 18, font)` — 565 = 640 − 75 | `appWidth() - 75` |
| `game/interface/Interface.h:125` | `Point2F currMenuPosition; // (init: 565,18)` | writable field, exe-typed |
| `game/GameState.cpp:790` / `:392` | radar rect `{16,13,151,151}` (twice, must agree) | anchor to a corner |
| `game/front/FrontEnd.cpp:1773-1774,1976-1977,2433-2449` | literal 640/480 in menu scroll maths | `appWidth()/appHeight()` |
| `game/interface/RadarMap.cpp:273-274` | already divides by 640/480 — **correct precedent** | none |
| `game/interface/Panels.h:140-146` | air-meter / crystal-bar offsets hard-set at `GameState.cpp:806,808` | recompute from `appWidth()` |

### 5.3 What makes this tractable — the structures are typed and writable

This is the part that turns "hard" into "an afternoon of careful work":

- `Panel_Globs` is **fully decompiled and typed**, `assert_sizeof(Panel_Globs, 0x7b8)`
  (`Panels.h:125-163`), bound at `0x005010e0`. `panelTable[i].openPos`, `.closedPos`,
  `.position` are plain `Point2F`; `buttonList[j].rect` is a `Rect2F`.
- `Interface_Globs` likewise, `0x14ac` at `0x004ddd58` (`Interface.h:85-125`), with
  `currMenuPosition` and per-icon `Point2F` offset arrays.
- **Writing new values into these is fully legal** under the cardinal rule — the rule forbids
  changing *sizes*, not contents. The exe's own drawing code reads these fields every frame.

So a **DLL-side relayout pass** is possible without reimplementing a single exe function.

### 5.4 The plan that actually works: decouple *layout space* from *render space*

The key realisation: `Lego_LoadPanels` does not need the *screen* size. It needs the size that
`Lego.cfg` has a block for. Those are two different things, and today they are the same
variable purely by accident of 1999.

```cpp
// game/GameState.cpp — currently :802 and :811, both OURS and already inside our body
//   Lego_LoadPanels      (cfg, Gods98::appWidth(), Gods98::appHeight());
//   Lego_LoadPanelButtons(cfg, Gods98::appWidth(), Gods98::appHeight());

// PROPOSED: load against the *layout* resolution (the one Lego.cfg has blocks for),
// then relayout DLL-side for the real one.
const Size2U layout = DeepCore::LayoutResolution();   // {640,480} unless overridden
Lego_LoadPanels      (legoConfig, layout.width, layout.height);
Panel_Crystals_Initialise(...);   // unchanged, :804
Panel_AirMeter_Initialise(...);   // unchanged, :806
Panel_CryOreSideBar_Initialise(...); // unchanged, :808
Lego_LoadPanelButtons(legoConfig, layout.width, layout.height);
...
DeepCore::RelayoutHUD(layout, Size2U{ (uint32)Gods98::appWidth(),
                                      (uint32)Gods98::appHeight() });  // NEW, no-op when equal
```

and the relayout itself, DLL-side, anchor-based rather than stretch-based:

```cpp
// game/DeepCore.cpp  (NEW section) — PROJECT code, no exe function reimplemented
namespace {
    // Anchors chosen per panel type so a wider screen pushes panels outward
    // instead of stretching 640-wide artwork across 1920.
    enum class Anchor { TopLeft, TopRight, BottomLeft, BottomRight, TopCentre, BottomCentre };

    Point2F Reanchor(Point2F p, Anchor a, Size2U from, Size2U to)
    {
        const real32 dx = (real32)to.width  - (real32)from.width;
        const real32 dy = (real32)to.height - (real32)from.height;
        switch (a) {
        case Anchor::TopLeft:      return p;
        case Anchor::TopRight:     return { p.x + dx,       p.y };
        case Anchor::BottomLeft:   return { p.x,            p.y + dy };
        case Anchor::BottomRight:  return { p.x + dx,       p.y + dy };
        case Anchor::TopCentre:    return { p.x + dx*0.5f,  p.y };
        case Anchor::BottomCentre: return { p.x + dx*0.5f,  p.y + dy };
        }
        return p;
    }
}

void DeepCore::RelayoutHUD(Size2U from, Size2U to)
{
    if (!settings.widescreenHUD) return;              // vanilla by default
    if (from.width == to.width && from.height == to.height) return;

    using namespace LegoRR;                            // required: exe macros name LegoRR types
    for (uint32 t = 0; t < Panel_Type_Count; t++) {
        const Anchor a = DeepCore::AnchorForPanel((Panel_Type)t);
        PanelData* panel = &panelGlobs.panelTable[t];  // Panels.h:128, exe-overlaid, WRITABLE
        const Point2F delta = {
            Reanchor(panel->openPos, a, from, to).x - panel->openPos.x,
            Reanchor(panel->openPos, a, from, to).y - panel->openPos.y,
        };
        panel->openPos.x   += delta.x;  panel->openPos.y   += delta.y;
        panel->closedPos.x += delta.x;  panel->closedPos.y += delta.y;
        panel->position.x  += delta.x;  panel->position.y  += delta.y;

        // Buttons carry absolute screen rects (Panels.h:87), so move them with the panel.
        for (uint32 b = 0; b < panel->buttonCount; b++) {
            Rect2F* r = &panel->buttonList[b].rect;
            r->left += delta.x; r->right  += delta.x;
            r->top  += delta.y; r->bottom += delta.y;
        }
    }
    // Icon menu anchor (Interface.h:125) — the exe reads this every frame.
    interfaceGlobs.currMenuPosition =
        Reanchor(interfaceGlobs.currMenuPosition, Anchor::TopRight, from, to);
}
```

`Interface_Initialise(565, 18, ...)` at `GameState.cpp:219` becomes
`Interface_Initialise(Gods98::appWidth() - 75, 18, ...)` — 565 is `640 - 75`, so this is exact
at 640 and correct elsewhere.

**Aspect ratio.** `Viewport_Create(0,0,1,1,…)` (`GameState.cpp:385`) already fills the surface,
and D3DRM derives horizontal extent from the viewport's own width/height, so a 16:9 viewport
should widen the view rather than stretch it (Hor+). **UNDETERMINED** — this is a claim about
`IDirect3DRMViewport2` behaviour that cannot be checked here. If it turns out to be Vert−
instead, `Viewport_SetField` (`Game.cpp:2900`, OURS) is the single compensation point, and
`Viewport_SmoothSetField` gives it for free.

**What still would not work, honestly:**
- Panel *artwork* is fixed-size bitmaps. A 1920-wide screen gets correctly-placed 640-era
  panels with gaps between them, not a wider panel. That is the standard, accepted outcome for
  this class of fix; anything better needs new art.
- FMV, splash screens and FLICs are fixed-size; they will letterbox or need `-scale`.
- The `viewTrack` radar viewport is fixed pixel `(16,13,151,151)` (`GameState.cpp:392`) and its
  duplicate `legoGlobs.radarScreenRect` (`:790`) — **these two must be changed together or the
  radar hit-test and the radar image disagree.**
- Anything the exe draws at a literal coordinate inside a function we have not decompiled is
  invisible to this analysis. `game/interface/` is 16.9% implemented; `Panels.h` is 0%.
  **UNDETERMINED and non-trivial.**

---

## 6. Upstream branches — what is there, what is harvestable

Our history is **unrelated** to upstream's (`git merge-base HEAD upstream/main` exits 1; 366
commits here, 350 there, no common ancestor). So nothing can be cherry-picked; anything wanted
must be ported by hand. The branch-to-branch diffs still read cleanly because the two upstream
branches *do* share history with `upstream/main`.

### 6.1 `upstream/trigger-lasertracker-minifigs` — 1 commit `bac3bb9`, 2 files, +72/−8

```
src/openlrr/game/GameState.cpp      | 12 +++++--
src/openlrr/game/object/Weapons.cpp | 68 ++++++++++++++++++++++++++++++++--
```

**What it does:** lets a MiniFigure enter laser-tracker mode (press `L`) and manually aim/fire
its equipped beam at the mouse world position — `Weapon_LegoObject_UpdateTracker` gains a
MiniFigure branch that orients the unit toward `Lego_GetMouseWorldPosition` and triggers
`Activity_FireLaser` with `LIVEOBJ2_FIRING{LASER,PUSHER,FREEZER}`
(`Weapons.cpp` @ branch, +1354..+1418), and `Lego_HandleKeys` admits MiniFigures to the mode
(`GameState.cpp` @ branch, +2311..+2326).

**Visual content: incidental.** It causes existing beam effects to be drawn in new situations.
It adds no rendering capability. It pairs *thematically* with our shipped `WeaponBeamStyles`
(more beams fired ⇒ more places the per-weapon styles show), which is the only reason to care.

**Harvestable?** Yes, cleanly — it touches `Weapons.cpp` (**OURS**, 0 live macros) and
`GameState.cpp` (**OURS**). Our `Weapons.cpp` has already diverged (`git rev-parse
HEAD:…/Weapons.cpp` ≠ upstream's), so it is a manual port, not a merge. **Not a visual
workstream.** Park it as a gameplay item.

### 6.2 `upstream/trigger-multibyte-fonts` — 1 commit `2763e77`, 10 files, +1458/−28

```
src/openlrr/engine/drawing/Fonts.cpp      | 717 +++++++++++++++++++++
src/openlrr/engine/drawing/Fonts.h        | 173 ++++++-
src/openlrr/engine/drawing/TextWindow.cpp | 514 ++++++++++++++++-
src/openlrr/engine/drawing/TextWindow.h   |  21 +
src/openlrr/game/GameCommon.h             |   9 +
src/openlrr/interop.cpp                   |  17 +-
src/openlrr/engine/core/{Config,ListSet}  |  31
src/openlrr/game/GameState.cpp            |   3 +-
```

**What it does visually:** it is a **GDI TrueType text renderer layered under the bitmap font**.
`Font_TT : public Font` (`Fonts.h` @ branch, +112) adds face name, height, weight, italic,
underline, strikethrough, charset/codepage, fore/back colour with a shadow offset
(`backOffset`), and variable-width metrics. Glyphs the 202-entry bitmap font cannot represent
are drawn with `::TextOutA` onto a DC obtained from the DirectDraw surface
(`Font_GDI_OutputCodePoint`, and the commented alternative shows
`DirectDraw_bSurf()->GetDC(&hdc)`). `Font_GDI_IsLeadByteChar` gives DBCS lead-byte handling —
the actual point of the branch (Korean).

**Why it matters here more than it looks.** Text quality is the *most* resolution-sensitive
part of an old game's presentation. Under `-scale 2` the bitmap font is nearest-neighbour
doubled; a TrueType path renders at native size. **This is the one upstream branch with real
visual value.**

**Three things that make it not-yet-harvestable, all verifiable from the diff:**

1. **`GameCommon.h` `#define`s over the whole engine API**:
   ```
   #define Font_GetStringWidth Font_GDI_GetStringWidth_White
   #define Font_PrintF        Font_GDI_PrintF_White
   #define TextWindow_Update  TextWindow_GDI_Update_Green
   ```
   A project-wide macro rename in a header included by nearly all of `game/`. Aggressive, and
   at odds with this fork's gate-everything-off-by-default rule.
2. **It re-points 8 live hooks** (`interop.cpp` @ branch, `0x00401b90`, `0x00401bc0`,
   `0x0047a410`, `0x0047a440`, `0x0047a470`, `0x00485650`). With no trampoline, each of those
   must be a complete replacement — for the font path the entire game draws through.
3. **It would break the warning contract.** In `Fonts.h` @ branch:
   ```cpp
   inline uint32 __cdecl Font_GDI_PrintF(Font_TT* fontTT, sint32 x, sint32 y, bool32 render,
                                         OPTIONAL OUT uint32* lineCount, const char* msg, ...)
   {
       std::va_list args;
       va_start(args, msg);
       uint32 result = Font_GDI_VPrintF2(fontTT, x, y, render, lineCount, msg, args);
       va_end(args);
   }               // <-- no return statement
   ```
   Missing return on a non-void function. The build is `/W3` (`openlrr.vcxproj:99,126`) with a
   contract of **exactly 44** warnings. Whether MSVC emits C4715 for an uninstantiated inline
   is **UNDETERMINED until compiled** — but the missing return is a fact and is undefined
   behaviour if the function is ever called.

**Verdict on the branch: genuinely valuable, not ready.** The harvestable *core* is
`Font_GDI_*` in `Fonts.cpp` plus the `Font_TT` type — roughly 700 lines — taken **without**
the `GameCommon.h` macro block and **without** re-pointing the existing hooks, exposed instead
as an opt-in `DeepCore` font override. That is its own workstream, and it is second in the
ranked plan below, not first.

**Also noted for separate consideration** (not visual, but in the same diff):
`GameState.cpp` @ branch changes `Config_Load("Lego.cfg")` to a two-stage
`Config_Load2(..., FILE_FLAG_DATADIR|FILE_FLAG_NOWAD)` then a WAD fallback — effectively a
`-cfgfirst` that is always on. We already have `-cfgfirst` (`Main.cpp:1361`). Do not take it.

---

## DECISION

### The single highest-value visual workstream: **the `DeepCore` Display layer**

**Rationale, in one paragraph.** The largest visual improvement available to this project is
arbitrary resolution and widescreen, for three reasons that are all measured above rather than
assumed: (1) it is the improvement players of a 1999 game ask for first; (2) the renderer
already supports it — `Viewport_Create(0,0,1,1,…)` is fractional, `Main_RenderScale()` is a
complete resolution-independence layer already threaded through nine subsystems, and `-res`
already exists; and (3) the thing that actually blocks it is **not** Retained Mode. It is four
`sprintf("%ix%i")` config-key lookups and a handful of literals, three of the four sites being
in our own C++, and the two exe-owned sites being reachable by *changing the arguments we pass*
rather than by reimplementing anything. That last point is what makes this compile-verifiable
work instead of a rewrite. Nothing else on the menu combines that impact with that risk.

**File-level plan.** All new behaviour gated `FALSE` by default in `data/Settings/DeepCore.cfg`.

| Step | File | Change | Ownership |
| --- | --- | --- | --- |
| 1 | `src/openlrr/game/DeepCore.hpp` | Add a `Display` block to `Settings`: `widescreenHUD`, `layoutWidth`/`layoutHeight` (default 640/480), `hudAnchorOverrides`, and the §(b) knobs `drawDistanceScale`, `fogDensityScale`, `fogStartScale`, `fieldOfViewTop`/`FP`, `textureFilter`, `renderQuality`, `lightBrightnessScale`. Declare `Size2U LayoutResolution()`, `void RelayoutHUD(Size2U from, Size2U to)`, `Anchor AnchorForPanel(Panel_Type)`. | PROJECT |
| 2 | `src/openlrr/game/DeepCore.cpp` | Parse them; implement `RelayoutHUD` exactly as sketched in §5.4. **No struct grows.** Guard: no-op when `from == to` **and** when the gate is off, so a stock run is byte-identical. | PROJECT |
| 3 | `src/openlrr/game/GameState.cpp:802,811` | Pass `DeepCore::LayoutResolution()` instead of `appWidth()/appHeight()` to `Lego_LoadPanels` / `Lego_LoadPanelButtons`. **This is the load-bearing line** — it is what makes the stock `Panels640x480` block resolve at any screen size. | OURS (call site) → EXE (callee, untouched) |
| 4 | `src/openlrr/game/GameState.cpp` after `:824` | Call `DeepCore::RelayoutHUD(layout, real)` once, after every panel/button/crystal/air/sidebar initialiser has run and before `Front_Initialise`. | OURS |
| 5 | `src/openlrr/game/GameState.cpp:461,468,481,489` | Build the `NextButton%ix%i` / `BackButton%ix%i` keys from `LayoutResolution()`; offset the loaded `NextButtonPos`/`RepeatButtonPos` through the same `Reanchor`. | OURS |
| 6 | `src/openlrr/game/interface/Advisor.cpp:188` + `GameState.cpp:838` | Same treatment for `AdvisorPositions%ix%i`; reanchor in `Advisor_AddPosition` (`Advisor.cpp:227`). | OURS |
| 7 | `src/openlrr/game/GameState.cpp:219` | `Interface_Initialise(Gods98::appWidth() - 75, 18, …)`. Exact at 640. | OURS |
| 8 | `src/openlrr/game/GameState.cpp:392` **and** `:790` | Radar viewport and `legoGlobs.radarScreenRect` — **change together**, anchored bottom-left, or leave both alone. Disagreement here is a hit-test bug, not a cosmetic one. | OURS |
| 9 | `src/openlrr/engine/Init.cpp:326-343` | Bounds-check `Init_AddValidMode` against `GRAPHICS_MAXMODES` (`DirectDraw.h:64`) before the write, matching the `Stats_Initialise` / `Weapon_Initialise` precedent. Unconditional — it is a correctness fix, not a feature. | OURS |
| 10 | `src/openlrr/game/Game.cpp:2842-2907`, `GameState.cpp:589-592` | Apply `drawDistanceScale` / `fogDensityScale` / FOV from step 1. **Ship with step 1-9, not after**: a wider FOV without a matched far plane and fog just shows more fog. | OURS |
| 11 | `src/openlrr/engine/Graphics.cpp:237-296`, `engine/gfx/Mesh.cpp:1589-1616` | Honour `textureFilter` / `renderQuality`; un-comment the trilinear branch behind the gate. | OURS |
| 12 | `data/Settings/DeepCore.cfg` | Document all of it in the existing house style, including the plain statement that this is compile-verified and not play-tested, and that panel *artwork* does not get wider. | data |
| 13 | `docs/WORKLOG.md`, `docs/HANDOFF-*.md` | Record what is verified (compiles, 0 errors / 44 warnings) and what is not (all of it, behaviourally). | docs |

**Not required and deliberately excluded:** no new `hook_write_jmpret`; no reimplementation of
`Lego_LoadPanels`, `Lego_LoadPanelButtons`, `Panel_Draw` or anything in `Panels.h`; no change to
any `assert_sizeof` type; no change to `interop.cpp`; no new file in `engine/gfx/`.

**Second workstream, when the first has landed:** port the `Font_TT` / `Font_GDI_*` core from
`upstream/trigger-multibyte-fonts` (§6.2) as an **opt-in** font override — without the
`GameCommon.h` macro block, without re-pointing the five existing font hooks, and with the
missing `return` fixed. Sharp text is what makes a high-resolution mode look finished rather
than merely larger.

**Third:** the remaining §(b) items — light-rig interception at `Container_MakeLight`
(`Containers.cpp:852-880`) and reviving `Draw_WorldLineListEx` (`Draw.cpp:299-365`).

### What is NOT achievable, and why

| Not achievable | Why, specifically |
| --- | --- |
| **A modern rendering backend** | ~19 files, ~13,000 lines in `engine/gfx/` + `engine/drawing/`, including a from-scratch frame graph (254 `IDirect3DRMFrame3` refs, 39 methods), traversal/culling/sorting to replace one `lpVP->Render()` call (`Viewports.cpp:284`), a second mesh renderer for RM-native meshes, `.lws` animation playback, and a lockable-surface emulation for the entire 2D layer (`Draw.cpp:73-76`). Plus: no trampoline, 32-bit forever, and a 44-warning budget. |
| **An RM → immediate-mode bridge as a stepping stone** | It is the same work as above minus the shader layer. And its correctness — culling, ordering, alpha sort, light falloff — is precisely the class of thing that compiles clean and is visibly wrong. **We cannot see the screen.** |
| **Storing anything per-Container / per-Mesh / per-Viewport** | `assert_sizeof(Container, 0x2c)` `Containers.h:326`; `assert_sizeof(Mesh, 0x34)` `Mesh.h:306`; `assert_sizeof(Viewport, 0x20)` `Viewports.h:83`, all owned by exe-overlaid globs at `0x0076bd80` / `0x005353c0` / `0x0076bce0`. Side tables only. |
| **Real shader effects (bloom, SSAO, shadow maps, PBR)** | The pipeline is fixed-function: 200 render-state slots (`Main.h:62,229`) and 50 texture-stage slots (`Mesh.h:99,333`) driving `IDirect3DDevice3`. There is nowhere to put a shader. |
| **Panels that are genuinely *wider* at 16:9** | The artwork is fixed-size bitmaps in the game's own WADs. Repositioning is possible; resizing needs new art, which this project does not ship. |
| **Anything drawn at a literal coordinate inside undecompiled exe code** | `game/interface/` is 16.9% implemented; `Panels.h` is 0/52; `Map3D.h` is 0/56. If any of those hard-codes a screen coordinate we cannot see it from source. **UNDETERMINED, and the honest reason the widescreen gate must default to off.** |
| **Any behavioural claim in this document** | There is no installation of the game on this machine. Every improvement above is compile-verifiable and **none of it is play-tested.** |
