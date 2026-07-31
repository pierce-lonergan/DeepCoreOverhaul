<!-- Research document. Authored by source reading and reproducible read-only shell commands
     only. NO BUILD WAS RUN for this document. NO GAME WAS RUN for this document (there is no
     installation on the authoring machine). Every file:line below is re-derivable with the
     commands quoted. Anything requiring observation of the running game is marked
     UNDETERMINED and is never asserted as fact. -->

# Arbitrary resolution and widescreen — file-level implementation plan

Repo: `C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`
Branch `main`, HEAD `03077c2` ("Correct my own guards: they were killing the game, not surviving it").
Working tree clean at time of writing.

Companion document: `docs/research/rendering-ceiling.md`. **This document verifies its
load-bearing claim and corrects it in three places.** Read §1.6, §3.4 and §5 first if you only
read one thing — those are the corrections, and one of them (§3.4) would have produced a
shipped hit-test bug.

---

## 0. Verdict up front

| Prior claim (`rendering-ceiling.md`) | Verdict |
| --- | --- |
| `-res <W>x<H>` exists and is parsed (`Main.cpp:1166`) | **CONFIRMED**, verbatim |
| `Main_RenderScale()` exists (`Main.cpp:112-120`) and is threaded through DirectDraw, viewports, images, Draw, FLIC, AVI, radar and mouse un-projection | **CONFIRMED — all nine, individually checked.** Table in §1.4 |
| "a complete resolution-independence layer" | **REFUTED as stated.** `Main_RenderScale()` is an **integer supersample multiplier**, not a resolution abstraction. It is orthogonal to arbitrary resolution and contributes nothing to widescreen. What actually delivers arbitrary resolution is `-res` → `Init_Initialise` → `DirectDraw_Setup` → `Main_SetupDisplay` → `appWidth()/appHeight()`, plus the fact that `Viewport_Create(0,0,1,1,…)` is fractional. See §1.5 |
| "four `sprintf("%ix%i")` config-key lookups, three of which are ours" | **INCOMPLETE.** There are **eight** resolution-keyed config lookups across **three** files, **all eight OURS**; the prior doc missed `Objective.cpp:401,437,498` entirely. Plus two **EXE** loaders reached by argument. See §1.1 |
| "~6 hardcoded literals" | **CONFIRMED**, 9 genuine sites, all ours. See §1.3 |
| "Buttons carry absolute screen rects (`Panels.h:87`), so move them with the panel" | **REFUTED.** `PanelButtonData::rect` is **panel-relative**. Shipping the prior doc's `RelayoutHUD` sketch would double-offset every panel button hit-box. Evidence in §3.4 |
| "a 16:9 viewport should widen the view rather than stretch (Hor+)" | **STILL UNDETERMINED**, and not resolvable from source or from public documentation (§5). But it is resolvable **at runtime by the program itself**, with code that compiles here. §5.3 gives that code |

**Bottom line: the workstream is real and is smaller than a rewrite, but it is not "an
afternoon".** The honest scope is 11 numbered steps across 9 files, all OURS, no new hooks, no
struct growth, `addrlint` untouched.

---

## 0.1 Reproducible commands

```bash
cd "C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul"

# every resolution-keyed config-key construction
grep -rn "%ix%i\|%dx%d\|%ux%u" --include=*.cpp --include=*.h src/openlrr

# genuine hardcoded 640/480 (struct offset comments stripped)
grep -rnE '\b640\b|\b480\b' --include=*.cpp --include=*.h src/openlrr \
  | grep -vE '/\*[0-9a-f]+,' | grep -vE '^\S+:[0-9]+:\s*//'

# every consumer of the render-scale layer
grep -rn "Main_RenderScale\|Main_RadarMapScale\|Main_IsRenderScaling" --include=*.cpp src/openlrr

# every consumer of the app resolution
grep -rn "appWidth()\|appHeight()" --include=*.cpp --include=*.h src/openlrr

# hook census
grep -c "^\s*result &= hook_write_jmpret"   src/openlrr/interop.cpp   # 1206 active
grep -c "^\s*//result &= hook_write_jmpret" src/openlrr/interop.cpp   #  309 disabled
                                                                     # 1515 total

python tools/addrlint/addrlint.py
# addrlint: 113 sized regions, 1 unsized, 376 assert_sizeof types
# (no overlap lines; only zero-slack notices)
```

**Ownership vocabulary used throughout.**
**OURS** = a compiled C++ body in this DLL installed over the exe by `hook_write_jmpret`
(`src/openlrr/interop.cpp`) — editing it changes behaviour.
**EXE** = a live raw-address macro, e.g.
`#define Lego_LoadPanels ((void (__cdecl*)(const Gods98::Config*, uint32, uint32))0x00434520)`
(`game/Game.h:1902`) — the `.cpp` body is commented out and editing it changes nothing.
**There is no trampoline** (1515 installs, zero backup buffers), so any EXE function we want to
change must be *reached by changing the arguments we pass it*, never wrapped.

---

# 1. VERIFY AND LOCATE

## 1.1 Every resolution-keyed config-key construction

`grep -rn "%ix%i" --include=*.cpp --include=*.h src/openlrr` returns 13 lines. Ten are
key constructions or their echo comments, three are in `Objective.cpp`, and **one is not a
config key at all**.

| # | Site | Key built | Ownership | Args come from | Effect when the key is absent |
| --- | --- | --- | --- | --- | --- |
| 1 | `game/GameState.cpp:461` | `NextButton%ix%i` | **OURS** | `appWidth()/appHeight()` inline | `nextButton == nullptr`, whole `if` block skipped (`:463`), `NextButtonImage` stays null, `NERPs_SetHasNextButton(false)` at `:511`. **Tutorial "next" button gone.** |
| 2 | `game/GameState.cpp:468` | `NextButtonPos%ix%i` | **OURS** | same | position stays whatever `memset` left; only reached if #1 resolved |
| 3 | `game/GameState.cpp:481` | `BackButton%ix%i` | **OURS** | same | `RepeatButtonImage` null. **Tutorial "back" button gone.** |
| 4 | `game/GameState.cpp:489` | `BackButtonPos%ix%i` | **OURS** | same | as #2 |
| 5 | `game/interface/Advisor.cpp:188` | `AdvisorPositions%ix%i` | **OURS** | `screenWidth/screenHeight` **parameters** | `Config_FindArray` returns nullptr, loop body never runs, **no advisor position is ever registered**; `advisorGlobs.positions[*].flags` never gets `ADVISOR_FLAG_USED` (`Advisor.cpp:227`) |
| 6 | `game/mission/Objective.cpp:401` | `ObjectiveImage%ix%i` | **OURS** | `screenWidth/screenHeight` **parameters** | `level->objective.panelImage` stays null. **Mission briefing image gone.** *Missed entirely by `rendering-ceiling.md`.* |
| 7 | `game/mission/Objective.cpp:437` | `ObjectiveAcheivedImage%ix%i` (sic) | **OURS** | same | mission-complete image gone |
| 8 | `game/mission/Objective.cpp:498` | `ObjectiveFailedImage%ix%i` | **OURS** | same | mission-failed image gone |
| — | `engine/drawing/DirectDraw.cpp:256` | `"%ix%i (%i bit)"` → `mode->desc` | **OURS** | enumerated mode | **NOT a config key.** It is the display string in the mode-selection listbox, and it is also the *lookup key* `Init_GetMode` (`Init.cpp:370-383`) matches on with `strcmp`. Do not touch it. |

Sites 5–8 are the good news: they take the resolution **as parameters**, and both entry points
are OURS and hooked —

* `Advisor_LoadPositions` (`Advisor.cpp:183`), hook `interop.cpp` @ `0x004014a0`; the only call
  site is **ours**, `GameState.cpp:838`.
* `Objective_LoadLevel` (`Objective.cpp:371`), hook `interop.cpp:3782` @ `0x00458000`; there is
  **no C++ caller** (`grep -rn "Objective_LoadLevel" src/openlrr` finds only the declaration,
  the definition and the hook), so the caller is EXE (`Lego_LoadLevel`) and the arguments it
  passes are `appWidth()/appHeight()`. Because the *body* is ours we simply ignore the incoming
  parameters and substitute the layout resolution.

### The two EXE-owned loaders

| Site | Callee | Ownership of callee | Ownership of call site |
| --- | --- | --- | --- |
| `game/GameState.cpp:802` | `Lego_LoadPanels(cfg, w, h)` | **EXE** `@0x00434520` (`Game.h:1902`) | **OURS** |
| `game/GameState.cpp:811` | `Lego_LoadPanelButtons(cfg, w, h)` | **EXE** `@0x00434640` (`Game.h:1906`) | **OURS** |

These are the entire reason the plan works: we never reimplement them, we change what we hand
them. The exact same trick is available for `Advisor_LoadPositions` at `GameState.cpp:838`,
which already takes them as explicit arguments.

## 1.2 The panel/interface globs are decompiled and writable

* `Panel_Globs` — `assert_sizeof(Panel_Globs, 0x7b8)` (`game/interface/Panels.h:163`), bound
  `LegoRR::panelGlobs = *(LegoRR::Panel_Globs*)0x005010e0` (`Panels.cpp:16`). Fully typed:
  `panelTable[Panel_Type_Count]` of `PanelData` (`Panels.h:99-112`, `assert_sizeof 0x30`),
  `Panel_Type_Count == 12` (`GameCommon.h:741-757`).
* `Interface_Globs` — `assert_sizeof(Interface_Globs, 0x14ac)` (`game/interface/Interface.h:154`),
  bound at `0x004ddd58` (`Interface.h:174-175`).
* **`Panels.cpp` is 0% implemented** — 52 commented-out declarations, two `/// CUSTOM:` helpers
  (`Panel_RadarMap_ZoomIn/Out`, `Panels.cpp:27-38`). Every `Panel_*` function is EXE.

Writing new *values* into these is legal under the cardinal rule; the rule pins *sizes*.
`addrlint` is unaffected by field writes.

## 1.3 Hardcoded resolution literals

After stripping `/*offset,size*/` comments, nine genuine sites remain — **all OURS**:

| Site | Literal | Nature |
| --- | --- | --- |
| `engine/Init.cpp:70` | `Size2U{640,480}` | default when `-res` absent. **Correct as-is** — it is the fallback, not an assumption |
| `engine/Init.cpp:85` | `Size2U{640,480}` | second entry in the always-registered mode list. **Correct as-is** |
| `engine/Init.cpp:156` | `DirectDraw_SetupWindowed(..., 640, 480)` | only reached when `!best && !setup` and `selMode == nullptr`. Should honour `cmdRes` |
| `game/GameState.cpp:219` | `Interface_Initialise(565, 18, font)` | `565 == 640 - 75`. Icon-menu origin. **Real breakage** |
| `game/GameState.cpp:392` | `Viewport_CreatePixel(16, 13, 151, 151, …)` | radar viewport, absolute pixels |
| `game/GameState.cpp:790` | `radarScreenRect = {16,13,151,151}` | **must agree with `:392`** |
| `game/front/FrontEnd.cpp:1773-1774` | `x < 640`, `y < 480` | off-screen cull for select-item images. Items past x=640 are **culled, not misplaced** |
| `game/front/FrontEnd.cpp:1976-1977` | `640.0f, 480.0f` src area | front-end background blit size |
| `game/front/FrontEnd.cpp:2433,2436,2438,2448,2449` | `480.0f`, `480 - imageHeight` | level-select scroll maths |

And the one site that already does it right, worth copying as the house pattern:

```cpp
// game/interface/RadarMap.cpp:271-274 — OURS
// Convert 640x480 units to [0,1] range.
for (uint32 i = 0; i < radarmapGlobs.arrowPointCount; i++) {
    radarmapGlobs.arrowPointsFrom[i].x *= static_cast<real32>(Gods98::appWidth())  / 640.0f;
    radarmapGlobs.arrowPointsFrom[i].y *= static_cast<real32>(Gods98::appHeight()) / 480.0f;
}
```

## 1.4 Is `Main_RenderScale()` genuinely threaded through the listed subsystems?

**Yes — every one, verified individually.** The prior doc's list was accurate.

```cpp
// engine/Main.cpp:111-120 — OURS
/// CUSTOM: Gets the rendering scale of the drawing surface, unlike Main_Scale,
///  this affects the resolution that things are drawn at.
sint32 Gods98::Main_RenderScale()
{
    if (Main_IsRenderScaling()) return static_cast<sint32>(mainGlobs2.windowScale);
    else                        return 1;
}
```

| Subsystem | Site(s) | What it does with it | Ownership | Verdict |
| --- | --- | --- | --- | --- |
| **DirectDraw back surface** | `drawing/DirectDraw.cpp:322-323` | `desc.dwWidth = width * Main_RenderScale()` — **windowed only**; fullscreen takes the attached flip surface at `:329` | OURS | **CONFIRMED**, with the fullscreen caveat below |
| **DirectDraw present** | `DirectDraw.cpp:451-452` (`Main_Scale`), `:460-461` (`Main_RenderScale`) | src rect at render scale, dest rect at window scale | OURS | **CONFIRMED** |
| **DirectDraw clear** | `DirectDraw.cpp:558-561, 574-577, 590-593` | `DirectDraw_Clear*` window rects scaled | OURS | **CONFIRMED** |
| **DirectDraw clipper** | `DirectDraw.cpp:644` | `CreateRectRgn(0,0,w*scale,h*scale)` | OURS | **CONFIRMED** |
| **D3DRM viewport** | `gfx/Viewports.cpp:64-65` (divide device size down), `:85-88` (multiply requested rect up) | viewport is authored in logical pixels, created in device pixels | OURS | **CONFIRMED** |
| **Image blits** | `drawing/Images.cpp:397, 407, 456-459` | `drawScale == 0 → Main_RenderScale()`; dest rect multiplied | OURS | **CONFIRMED** |
| **Draw primitives** | `drawing/Draw.cpp:52-53, 158-182, 782-785` | `_drawScale` defaults to render scale; every pixel write expands to a `scale × scale` block | OURS | **CONFIRMED** |
| **Fonts** | `drawing/Fonts.cpp:274, 310` | `Image_DisplayScaled2(..., 0, true)` → `0` means render scale | OURS | **CONFIRMED** (the prior doc did not list fonts; they *are* covered) |
| **FLIC** | `drawing/Flic.cpp:379-382` | dest rect multiplied | OURS | **CONFIRMED** |
| **AVI (in-game)** | `video/Animation.cpp:577-580` | dest rect multiplied | OURS | **CONFIRMED** |
| **AVI (movie player)** | `video/Movie.cpp:180-183` | dest rect multiplied | OURS | **CONFIRMED** |
| **Radar** | `interface/RadarMap.cpp:124` (`RadarMap_GetDrawScale`), `:190` (`_RadarMap_GetTransformScale`) | radar pixel density decoupled from render scale | OURS | **CONFIRMED** |
| **Mouse un-projection** | `gfx/Viewports.cpp:352-357` (screen→world scales **up**), `:375-376` (world→screen scales **down**) | logical-pixel mouse coords survive supersampling | OURS | **CONFIRMED** |
| **Mouse input** | `input/Input.cpp:226-232` | `cursorPos /= Main_Scale()`, clamped to `appWidth()/appHeight()` | OURS | **CONFIRMED** — note this is `Main_Scale` (window scale), which is correct: the OS cursor lives in window pixels |

**Fullscreen caveat, re-verified.** `Main_IsScaleSupported` returns `false` under
`Main_FullScreen()` (`Main.cpp:146-147`), but the `-scale` parser writes
`mainGlobs2.windowScale` **directly without consulting it** (`Main.cpp:1263-1266`). In
fullscreen the back buffer is the attached flip surface (`DirectDraw.cpp:326-329`), not one we
size, so `Main_RenderScale()` would scale 2D drawing past the surface edge. **Windowed is the
supported combination.** UNDETERMINED behaviourally; the code path is unambiguous.

## 1.5 CORRECTION — what `Main_RenderScale()` actually is

`Main_RenderScale()` returns `mainGlobs2.windowScale`, an **integer** (`Main.h:269`,
`Main.cpp:115`), set only by `-scale N` / `-nnscale N` (`Main.cpp:1256-1266`) and floored at 1
(`Main.cpp:1275-1276`). It is a **uniform integer supersample factor**. It cannot express
1280×720, it cannot express a non-4:3 aspect, and every one of the fourteen consumers above
multiplies *both* axes by the same integer.

Calling it "a complete resolution-independence layer" oversells it and, worse, points the
implementer at the wrong seam. The chain that actually produces arbitrary resolution is:

```
-res W×H                       Main.cpp:1166-1182   → mainOptions.res (Main.h:285)
  │
Init_Initialise                Init.cpp:56-165
  ├─ cmdRes = mainOptions.res.value_or({640,480})              :70
  ├─ Init_AddValidMode(cmdRes, each of 4 bpp)                  :94-97
  ├─ (+800×600, +1024×768 only when -debug)                    :87-88, :99-104
  ├─ DirectDraw_EnumModes  →  the DRIVER's real mode list      :119
  ├─ Init_IsValidMode  =  enumerated ∩ registered              :347-366
  └─ DirectDraw_SetupFullScreen / SetupWindowed                :154-156
       │
DirectDraw_Setup               DirectDraw.cpp:268-365
  ├─ directDrawGlobs.{width,height} = mode->{width,height}     :285-286
  ├─ Main_SetupDisplay(...)  →  mainGlobs.{appWidth,appHeight} Main.cpp:1666-1667
  ├─ back surface = width*RenderScale × height*RenderScale     :322-323
  └─ Graphics_SetupDirect3D → CreateDeviceFromSurface          :334
       │
Viewport_Create(0,0,1,1,camMain)   GameState.cpp:385 → Viewports.cpp:58-74
  └─ FRACTIONAL — the 3D view already fills any surface, any aspect
```

`Main_RenderScale()` sits *beside* this chain, not inside it. **The two are independent and
both work; do not conflate them.**

## 1.6 CORRECTION — the mode-enumeration gate the prior doc missed

`-res` does **not** unconditionally give you that resolution. `Init_IsValidMode`
(`Init.cpp:347-366`) intersects the registered list with the list the **driver enumerated**
(`DirectDraw_EnumModes` → `EnumDisplayModes`, `DirectDraw.cpp:230, 245-265`). The `best` path
(`Init.cpp:120-134`) walks *enumerated* modes and picks the first that is valid. So:

* `-res 1920x1080` works only if the driver enumerates 1920×1080 at the chosen bpp.
* **This applies in windowed mode too.** `Init.cpp:155` passes `initGlobs.selMode->width/height`
  to `DirectDraw_SetupWindowed`, and `selMode` came out of the enumerated list. A windowed game
  does not need a display mode — but the current code makes it need one anyway.
* `DirectDraw_EnumModeCallback` further filters windowed modes to the desktop bit depth
  (`DirectDraw.cpp:259-262`).

Consequence: `-res 1600x900` may silently fall back to 640×480 on hardware that does not
enumerate it, and `-res 2560x1080` on a 16:9 panel almost certainly will. **Step 3 of the plan
fixes the windowed case, where the constraint is spurious.** The fullscreen case is a genuine
hardware constraint and stays.

Also re-confirmed: `Init_AddValidMode` (`Init.cpp:326-343`) writes
`initGlobs.validModes[initGlobs.validModeCount++]` with **no bound** against
`GRAPHICS_MAXMODES == 200` (`DirectDraw.h:64`; array at `Init.h:38`, `Init_Globs` pinned
`assert_sizeof 0x1d444`). Today the maximum reachable count is `(1 cmdRes + 1 default) × 4 bpp
+ 2 debug sizes × 4 bpp = 16`, so it is unreachable — **but any change that enumerates a mode
list into it makes it reachable, and `validModeCount` is the last field in the glob**
(`Init.h:39`), so overflow writes past the end of a pinned exe-overlaid structure. This belongs
in the same commit as anything that touches mode registration. It is the exact shape of the
`Stats_Initialise` / `SFX sampleGroupTable` class already fixed in this tree
(`docs/WORKLOG.md`).

---

# 2. WHAT ACTUALLY BREAKS AT A NON-4:3 RESOLUTION

Two distinct failure modes, and they need distinguishing because they need different fixes:

* **(A) ABSENT** — a resolution-keyed config lookup misses, the guarded block is skipped, and
  the asset never loads. Fixed by *changing the key we build*.
* **(B) MISPLACED** — the asset loads with coordinates authored for 640×480 and is drawn at
  those coordinates on a larger screen. Fixed by *re-anchoring after load*.

| Subsystem | What breaks | Mode | Where the assumption lives | Ownership |
| --- | --- | --- | --- | --- |
| **HUD panels** (radar, messages, top panel, crystal sidebar, priority list, camera control, info dock, encyclopedia — 12 total) | Panel images/flics never load; `PANEL_FLAG_HASIMAGE` (`Panels.h:54`) never set | **A** | `GameState.cpp:802` → EXE `Lego_LoadPanels @0x00434520`. The exe reads a `Panels<W>x<H>` block. **INFERRED, not read** — `Panels.cpp` is 0% decompiled. The inference is forced by the signature `(config, screenWidth, screenHeight)` (`Game.h:1902`) and by the identical `%ix%i` pattern in the eight sites we *can* read | call site OURS, callee EXE |
| **Panel buttons** | Buttons never created | **A** | `GameState.cpp:811` → EXE `Lego_LoadPanelButtons @0x00434640` | call site OURS, callee EXE |
| **Icon menu (build/unit radial menu)** | Anchored at x=565 instead of near the right edge | **B** | `GameState.cpp:219` `Interface_Initialise(565, 18, …)`; stored in `interfaceGlobs.currMenuPosition` (`Interface.h:125`) and copied into `slideEndPosition` (`Interface.h:129`, comment `init: pointf_f78`) | call site OURS, `Interface_Initialise` EXE `@0x0041a230` |
| **Advisor** | No advisor position ever registers → advisor never appears | **A** | `Advisor.cpp:188` (OURS), fed from `GameState.cpp:838` (OURS) | **OURS both ends** |
| **Advisor, once fixed** | Positions with a panel are **panel-relative** and follow the panel automatically (`Advisor.cpp:378-383`: `Panel_GetPosition(...)` then `x + panelPos.x`). Only `ADVISOR_FLAG_NOPANEL` entries are absolute-screen | **B**, partial | `Advisor.cpp:376-392` | OURS |
| **Mission briefing / achieved / failed images** | Never load | **A** | `Objective.cpp:401,437,498` (OURS) | **OURS** |
| **Radar** | Nothing breaks *arithmetically*: `RadarMap_Initialise` already scales the view-cone arrow by `appWidth()/640` (`RadarMap.cpp:271-274`) and the camera-footprint box is built from `appWidth()/appHeight()` (`RadarMap.cpp:482-490`). What breaks is **placement**: `viewTrack` is a fixed pixel viewport at `(16,13,151,151)` (`GameState.cpp:392`) and `legoGlobs.radarScreenRect` duplicates it (`GameState.cpp:790`). If either moves without the other, the radar **image and its hit-test disagree** (`RadarMap.cpp:690-691` compares raw `mouseX` against `screenRect`) | **B** | `GameState.cpp:392` and `:790` | **OURS both** |
| **Mouse un-projection** | **Does not break.** `Viewport_InverseTransform` (`Viewports.cpp:344-361`) delegates to `IDirect3DRMViewport2::InverseTransform` for the *actual* viewport, which is fractional `(0,0,1,1)` and therefore already the full surface. Mouse coords are clamped to `appWidth()/appHeight()` (`Input.cpp:229-232`) | — | — | OURS |
| **Object selection boxes / names** | Do not break — bounds-checked against `appWidth()/appHeight()` (`Game.cpp:1200-1201`, `:1350-1351`) | — | — | OURS |
| **FLIC** | Plays at its authored size at its authored position. A 640×480 full-screen FLIC on a 1920×1080 surface occupies the top-left ninth | **B** | `Flic.cpp:373-382` takes `destArea` from the caller; the caller supplies 640-era coordinates | OURS (engine), callers vary |
| **AVI / FMV** | Same. `Animation_BlitToBackBuffer` (`Animation.cpp:570-580`) and `G98CMovie` (`Movie.cpp:179-186`) blit into a caller-supplied rect | **B** | callers | mixed |
| **Front-end menus** (title, options, level select, save/load) | Background image blits at `(0,0)` with a 640×480 source area and **`destSize == nullptr`**, which `Image_DisplayScaled2` resolves to *source* size (`Images.cpp:437-439`) → a 640×480 island in the top-left corner. All menu items are drawn at `item->x1 + menu->position.x` and hit-tested at the same expression, so they are internally consistent but collectively mis-anchored | **B** | `FrontEnd.cpp:1963-1982` (background), `:850-851` (`menu->position`), `:794-795` (`currPosition`), `:863-864` (`anchoredPosition`) | **OURS — `FrontEnd.cpp` has 106 implemented functions and 0 live macros** |
| **Front-end select items** | Additionally **culled** past x=640 / y=480 (`FrontEnd.cpp:1771-1775`), so widening without fixing this hides items rather than moving them | **B** + hard cull | `FrontEnd.cpp:1773-1774` | OURS |
| **Level-select scroll** | Clamp maths uses literal 480 (`FrontEnd.cpp:2433-2449`) | **B** | as cited | OURS |
| **Font metrics** | Do **not** break. `Font_GetStringWidth` (`Fonts.cpp:69`) and `fontHeight` (`Fonts.cpp:166`) are measured off the glyph bitmap, and drawing goes through `Image_DisplayScaled2(..., drawScale = 0)` (`Fonts.cpp:274, 310`) → `Main_RenderScale()`. Everything is self-consistent. What changes is **apparent size**: at 1920×1080 with `-scale 1` the HUD text is one third the relative height it had at 640×480. That is a legibility problem, not a correctness problem, and the only in-tree lever is `-scale`, which is integer-only | — (legibility) | `Fonts.cpp:240-330` | OURS |
| **Tool tips** | `ToolTip_Initialise(..., appWidth(), appHeight(), …)` (`GameState.cpp:214`) already receives the real resolution | — | — | OURS call site, EXE callee |
| **Dialog box** | Already centred: `appWidth()/2 - imageWidth/2` (`GameState.cpp:707-708`) | — | — | OURS |
| **Loading bar** | Already centred (`front/Loader.cpp:232`) | — | — | OURS |
| **Credits** | Already resolution-derived (`front/Credits.cpp:44-50`) | — | — | OURS |
| **Priorities list** | Panel-relative (`Priorities.cpp:178-184`, `:362-367`, `:381-388`), follows `Panel_PriorityList` for free | — | — | OURS |
| **Anything drawn at a literal coordinate inside undecompiled exe code** | Invisible to this analysis | **B**, unknown extent | `Panels.h` 0/52 implemented; `Interface.h` mostly macros | **EXE — this is the honest reason the gate defaults off** |

---

# 3. THE PANEL / INTERFACE LAYOUT PROBLEM

## 3.1 Are `panelGlobs` and `interfaceGlobs` exe-overlaid, frozen in size, but writable?

**Yes to all three.**

```cpp
// game/interface/Panels.cpp:15-16
// <LegoRR.exe @005010e0>
LegoRR::Panel_Globs & LegoRR::panelGlobs = *(LegoRR::Panel_Globs*)0x005010e0;
```
`assert_sizeof(Panel_Globs, 0x7b8)` (`Panels.h:163`).

```cpp
// game/interface/Interface.h:174-175
// <LegoRR.exe @004ddd58>
extern Interface_Globs & interfaceGlobs;
```
`assert_sizeof(Interface_Globs, 0x14ac)` (`Interface.h:154`).

They are C++ **references bound to fixed addresses**, non-const, with fully typed public
fields. Nothing prevents assignment. The cardinal rule forbids changing `sizeof`, and no step
in this plan does. `addrlint` reports region *extents*, which field writes do not alter, so it
stays at **113 regions / 0 overlaps**.

## 3.2 Does the "layout resolution" trick work?

**Yes, with one structural caveat and one ordering caveat.**

The insight is sound: `Lego_LoadPanels` does not need the *screen* size, it needs *a size the
config has a block for*. Passing `{640,480}` makes the stock block resolve at any screen size.

**Structural caveat.** The trick is only *sufficient* for the (A)-class failures. It converts
"absent" into "present but 640-anchored", which is strictly better but still wrong on a wide
screen. The (B)-class re-anchor is mandatory, not optional polish.

**Ordering caveat, verified.** Inside `Lego_Initialise` (`GameState.cpp:95`) the sequence is:

| Line | Call | Note |
| --- | --- | --- |
| 146 | `DeepCore::Load()` | settings available from here on |
| 214 | `ToolTip_Initialise(..., appWidth(), appHeight(), …)` | already correct |
| 219 | `Interface_Initialise(565, 18, font)` | **fix here, not later** — see §3.5 |
| 385 | `Viewport_Create(0,0,1,1,…)` → `viewMain` | fractional, correct at any aspect |
| 392 | `Viewport_CreatePixel(16,13,151,151,…)` → `viewTrack` | **cannot be moved after creation** |
| 461-495 | tutorial next/back button keys | |
| 589-592 | `TVClipDist`, `Viewport_SetBackClip` on both viewports | |
| 790 | `radarScreenRect = {16,13,151,151}` | must match `:392` |
| 802 | `Lego_LoadPanels(cfg, w, h)` | **the load-bearing line** |
| 804/806/808 | `Panel_Crystals_Initialise`, `Panel_AirMeter_Initialise(…,85,6,236,…,21,0)`, `Panel_CryOreSideBar_Initialise(…,615,434,423)` | all EXE (`Panels.h:379,391,408`) |
| 811 | `Lego_LoadPanelButtons(cfg, w, h)` | |
| 812 | `Lego_LoadTutorialIcon(cfg)` | EXE `@0x00434930` |
| 815-822 | `Interface_LoadMenuItems`, `Interface_LoadItemPanels`, `Panel_LoadInterfaceButtons_ScrollInfo` | all EXE |
| 830 | `Front_Initialise(cfg)` | |
| 838 | `Advisor_LoadPositions(cfg, gameName, w, h)` | |

The relayout pass must run **after 822 and before 830**. `viewTrack` at `:392` is created
*before* the panels load, which is why the radar gets its own treatment (§3.6).

## 3.3 Every field that would need re-anchoring — enumerated, with writability

### `Panel_Globs` (`Panels.h:125-163`), instance `panelGlobs` @ `0x005010e0`

| Field | Offset | Type | Semantics | Re-anchor? | Writable |
| --- | --- | --- | --- | --- | --- |
| `panelTable[t].openPos` | `0x030 + t*0x30 + 0x08` | `Point2F` | absolute screen, panel shown | **YES** | yes |
| `panelTable[t].closedPos` | `+0x10` | `Point2F` | absolute screen, panel hidden off-edge | **YES**, same delta | yes |
| `panelTable[t].position` | `+0x18` | `Point2F` | current interpolated position, read every frame by EXE `Panel_Draw @0x0045a9f0` | **YES**, same delta | yes |
| `panelTable[t].buttonList[b].rect` | `PanelButtonData+0x00`, `Rect2F` | **panel-relative** — see §3.4 | **NO. DO NOT TOUCH** | yes (but don't) |
| `airMeterJuiceOffset` | `0x718` | `Point2F` | commented *"Offset relative to top-left corner of MsgPanel"* (`Panels.h:140`) | **NO** — follows `Panel_Messages` | yes |
| `airMeterNoAirOffset` | `0x724` | `Point2F` | same comment (`Panels.h:142`) | **NO** | yes |
| `cryOreSideBarOffset` | `0x734` | `Point2F` | `Hard: (615, 434)` (`Panels.h:145`). **No "relative to" comment**, and 615/434 read as absolute bottom-right screen coordinates in a 640×480 frame | **UNDETERMINED** — see §6 | yes |
| `rotateCenter` | `0x744` | `Point2I` | camera-rotation control centre, loaded by EXE `Panel_RotationControl_Initialise @0x0045bc90` | **UNDETERMINED** — probably panel-relative (its consumer `Panel_RotationControl_HandleRotation` also takes raw mouse coords) | yes |
| `rotateUp/Down/Left/RightOffset` | `0x750..0x768` | `Point2F` | named *Offset* | **NO** (assume relative) | yes |
| `panelName[]`, images, counts, flags | — | — | not positional | — | — |

### `Interface_Globs` (`Interface.h:85-154`), instance `interfaceGlobs` @ `0x004ddd58`

| Field | Offset | Type | Semantics | Re-anchor? | Writable |
| --- | --- | --- | --- | --- | --- |
| `currMenuPosition` | `0x0f78` | `Point2F` | `(init: 565,18)` — the icon menu's live position, read every frame | **YES**, but preferably by fixing the `Interface_Initialise` argument (§3.5) | yes |
| `slideStartPosition` | `0x0f88` | `Point2F` | slide animation source | **YES if patching post-hoc** | yes |
| `slideEndPosition` | `0x0f90` | `Point2F` | `(init: pointf_f78)` — initialised *from* `currMenuPosition` | **YES if patching post-hoc** | yes |
| `iconPanelIconOffsets[11]` | `0x0030` | `Point2F[]` | cfg `InterfaceSurroundImages[1,2]`, named *Offsets* | **NO** | yes |
| `iconPanelNoBackIconOffsets[11]` | `0x00b4` | `Point2F[]` | cfg `[6,7]` | **NO** | yes |
| `iconPanelBackButtonOffsets[11]` | `0x0114` | `Point2F[]` | cfg `[3,4]` | **NO** | yes |
| `backButtonSize` | `0x016c` | `Size2I` | a size, not a position | **NO** | yes |
| `selBlockPos`, `highlightBlockPos` | `0x0f6c`, `0x0f9c` | `Point2I` | **map block** coordinates, not screen | **NO** | yes |
| `areaf_fb4` | `0x0fb4` | `Area2F` | purpose unknown | **UNDETERMINED** | yes |

**Every field above is a plain public member of a non-const reference. All are writable. No
`sizeof` changes anywhere.**

## 3.4 CORRECTION — panel button rects are PANEL-RELATIVE

`rendering-ceiling.md` §5.4 asserts *"Buttons carry absolute screen rects (`Panels.h:87`), so
move them with the panel"* and its sketch offsets every `buttonList[b].rect` by the panel delta.
**That is wrong, and shipping it would move every panel button's hit-box twice.**

Evidence, from code we own and can read:

```cpp
// game/interface/Priorities.cpp:362-367 — OURS
Panel_GetPosition(Panel_PriorityList, &x3, &y3);
Panel_Button_GetArea(Panel_PriorityList, buttonType, &buttonArea);
sint32 x = (sint32)(x3 + buttonArea.x + buttonArea.width  * 0.5f);
sint32 y = (sint32)(y3 + buttonArea.y + buttonArea.height * 0.5f);
Gods98::Input_SetCursorPos(x, y);
```

`Panel_Button_GetArea` (EXE `@0x0045c230`, `Panels.h:375`) returns the button's area and **our
own code adds the panel position to it** before treating it as a screen coordinate. If the
stored rect were absolute this would place the cursor at roughly twice the panel offset.

Corroboration:

* `Panel_TestPointInsideRect(PanelData* panel, const Rect2F* rect, sint32 mouseX, sint32 mouseY)`
  (EXE `@0x0045b850`, `Panels.h:331`) takes **both** the panel and the rect. There is no reason
  to pass the panel unless the panel supplies the origin.
* The same panel-relative convention holds for `prioritiesGlobs.buttonPoints[]`
  (`Priorities.cpp:178-184`, `:381-388`) and for advisor positions (`Advisor.cpp:378-383`).

**Therefore: re-anchoring `openPos`/`closedPos`/`position` is sufficient and complete for a
panel and everything attached to it — buttons, priority icons, air meter, advisor pointers.
Touching `buttonList[].rect` is a bug.**

This single correction is why §4 Step 5 is nine lines rather than the prior doc's twenty.

## 3.5 The icon menu: fix the argument, not the field

`interfaceGlobs.currMenuPosition` is initialised by EXE `Interface_Initialise @0x0041a230` from
its `x_565, y_18` parameters (`Interface.h:200`), and `slideEndPosition` is documented
`(init: pointf_f78)` (`Interface.h:129`) — i.e. copied *from* `currMenuPosition` at init. Patching
`currMenuPosition` after the fact leaves `slideEndPosition` stale, and the first menu slide
snaps back to 565.

Because the call site is **ours**, the correct fix is upstream of the problem:

```cpp
// game/GameState.cpp:219 — OURS (call site); Interface_Initialise is EXE @0x0041a230
// WAS: Interface_Initialise(565, 18, legoGlobs.fontToolTip);
const Point2I menuOrigin = DeepCore::Display::IconMenuOrigin();   // {565,18} at 640x480
Interface_Initialise((uint32)menuOrigin.x, (uint32)menuOrigin.y, legoGlobs.fontToolTip);
```

with `IconMenuOrigin()` returning `{ appWidth() - 75, 18 }` when the gate is on — exact at
640 (`640 - 75 == 565`) and correct elsewhere. **No exe function is reimplemented and no
post-hoc field patch is needed.**

## 3.6 The radar: the one case the layout trick cannot cover

`viewTrack` is a **pixel** viewport created at `GameState.cpp:392`, *before* panels load, and
there is no `Viewport_SetPosition` — `Viewports.h:86-118` exposes create, get-size, set-camera,
clip, clear, render, remove, field, transforms. Position is fixed at creation.

Three options, ranked:

1. **Anchor the radar top-left (recommended default).** `Panel_Radar` is top-left in stock
   layout, so `Anchor::TopLeft` gives `delta == (0,0)`, `viewTrack` never needs to move, and
   `radarScreenRect` stays `{16,13,151,151}`. **Zero risk, and it is what a player expects.**
2. Compute the radar anchor *before* `:392` from the layout delta (which is knowable — it is
   pure arithmetic on `appWidth()/appHeight()`, not on config data) and pass the moved rect to
   **both** `:392` and `:790`. Correct, slightly more code.
3. Destroy and recreate `viewTrack` after the relayout (`Viewport_Remove` + `Viewport_CreatePixel`,
   both OURS/hooked) and rewrite `legoGlobs.viewTrack`. **Not recommended**: the EXE may have
   cached the pointer between `:392` and the relayout point, and we cannot read the exe to check.

Whichever is chosen, **`GameState.cpp:392` and `GameState.cpp:790` must be derived from one
expression.** Today they are two independently-typed literals that happen to agree; that is a
latent bug regardless of widescreen.

---

# 4. THE PLAN

Design constraints honoured by every step: **no new `hook_write_jmpret`; no reimplementation of
any EXE function; no change to any `assert_sizeof` type; no change to `interop.cpp`;
`addrlint` stays 113/0; gated FALSE by default so a stock run is byte-identical.**

## Step 0 — decide where the target resolution comes from

`DeepCore::Load()` runs at `GameState.cpp:146`, which is inside `Lego_Initialise`, which runs
**after** `Init_Initialise` (`Main.cpp:827`) has already chosen and set the display mode. So
`DeepCore.cfg` **cannot** choose the resolution without an earlier load.

Two clean answers; **take (a) first**:

**(a) Resolution comes from the existing `-res` / `-window` / `-bpp` / `-scale` command line;
`DeepCore.cfg` only controls *layout*.** Zero new startup machinery, zero ordering risk.

**(b) Later, if wanted: an early display-only pre-pass.** It is feasible —
`File_Initialise` / `Config_Initialise` (`Main.cpp:802-810`) both run before `Init_Initialise`
(`:827`), and `mainGlobs.programName` — which is what `legoGlobs.gameName` is later assigned
from (`Game.cpp:69`) — is set at `Main.cpp:718`. So a `DeepCore::Display::PreLoad(const char*
gameName)` reading only the display keys would work. But `DeepCore_ID` currently hard-codes
`LegoRR::legoGlobs.gameName` (`DeepCore.cpp:29`), which is still null at that point, so this
needs a parameterised ID. **Defer it. Do not do it in the same commit as the layout work.**

## Step 1 — `src/openlrr/game/DeepCore.hpp` — declare the Display block  *(PROJECT)*

```cpp
// ---- Display / widescreen -------------------------------------------------
// Resolution itself comes from the command line (-res / -window / -scale).
// These settings govern LAYOUT only: which coordinate space the HUD's config
// blocks are looked up in, and how the loaded coordinates are re-anchored for
// the real screen afterwards.

/// Master gate. FALSE == every Display setting below is inert and the game
/// behaves exactly as it does today at any resolution.
bool widescreenHUD = false;

/// The resolution the HUD's config blocks are AUTHORED for -- i.e. the one the
/// user's Lego.cfg actually has "Panels<W>x<H>", "AdvisorPositions<W>x<H>",
/// "NextButton<W>x<H>" and "ObjectiveImage<W>x<H>" entries for. Almost always
/// 640x480. This is deliberately NOT the screen resolution.
uint32 layoutWidth  = 640;
uint32 layoutHeight = 480;

/// Per-panel anchor overrides, "PanelName:Anchor". Unlisted panels use the
/// built-in default from AnchorForPanel().
std::vector<std::string> panelAnchors;

/// Re-anchor the front-end menus. Independent of the HUD because the front end
/// is 100% OpenLRR-owned code and can be verified separately.
///   Off      -- leave at top-left (current behaviour)
///   Centre   -- centre the 640x480 menu block on the screen
bool frontEndCentre = false;

/// Aspect handling for the 3D view. See docs/research/widescreen-plan.md sec.5.
///   Auto    -- measure the projection at runtime and correct only if needed
///   Hor     -- assume wider = more horizontal FOV, do nothing
///   Vert    -- assume wider = stretched, scale the field by (4/3)/(w/h)
///   Off     -- never touch Viewport_SetField
std::string aspectMode = "Off";

/// Multiplier applied on top of whatever aspectMode decides. 1.0 == no change.
real32 fieldOfViewScale = 1.0f;
```

plus declarations:

```cpp
namespace Display
{
    enum class Anchor : uint32 {
        TopLeft, TopCentre, TopRight,
        BottomLeft, BottomCentre, BottomRight,
        Centre,
    };

    /// The resolution HUD config blocks are looked up in. {640,480} unless
    /// overridden; equals the real resolution when the gate is off, which makes
    /// every call site below a no-op in a stock run.
    Size2U LayoutResolution(void);

    /// The real resolution, as one call, so no site has to remember the cast.
    Size2U ScreenResolution(void);

    /// Icon-menu origin for GameState.cpp:219. {565,18} when the gate is off.
    Point2I IconMenuOrigin(void);

    /// Radar viewport rect, used by BOTH GameState.cpp:392 and :790.
    Area2F RadarRect(void);

    /// Front-end translation applied to menu positions. {0,0} when off.
    Point2I FrontEndOffset(void);

    /// Translate one point between layout space and screen space.
    Point2F Reanchor(Point2F p, Anchor a);

    /// Default anchor for a panel index (LegoRR::Panel_Type as uint32, passed
    /// untyped so this header need not include the game headers).
    Anchor AnchorForPanel(uint32 panelType);

    /// Re-anchor panelGlobs + interfaceGlobs after the loaders have run.
    /// No-op when the gate is off or layout == screen.
    void RelayoutHUD(void);
}
```

`Point2I`/`Point2F`/`Size2U`/`Area2F` come from `engine/geometry.h`, which `DeepCore.hpp`
does not currently include — add that include, not the game headers.

## Step 2 — `src/openlrr/game/DeepCore.cpp` — implement it  *(PROJECT)*

Parse with the existing `_ReadIntIfPresent` / `Config_GetBoolOrFalse` / `_SplitFields` helpers
(`DeepCore.cpp:400-433`, `:462-470`), in `Load()` alongside the other blocks.

```cpp
// game/DeepCore.cpp  (new section) -- PROJECT code, no exe function reimplemented.
// MACRO TRAP: exe address macros cannot be namespace-qualified and their
// expansions name unqualified types from the game namespace, so any function
// here that touches panelGlobs/interfaceGlobs needs `using namespace LegoRR;`
// in scope. See docs/HOOK-ARCHITECTURE.md.

Size2U DeepCore::Display::ScreenResolution(void)
{
    return Size2U { (uint32)Gods98::appWidth(), (uint32)Gods98::appHeight() };
}

Size2U DeepCore::Display::LayoutResolution(void)
{
    if (!settings.widescreenHUD) return ScreenResolution();   // stock: identical
    return Size2U { settings.layoutWidth, settings.layoutHeight };
}

Point2F DeepCore::Display::Reanchor(Point2F p, Anchor a)
{
    const Size2U from = LayoutResolution();
    const Size2U to   = ScreenResolution();
    const real32 dx = (real32)to.width  - (real32)from.width;
    const real32 dy = (real32)to.height - (real32)from.height;

    switch (a) {
    case Anchor::TopLeft:      return p;
    case Anchor::TopCentre:    return { p.x + dx * 0.5f, p.y };
    case Anchor::TopRight:     return { p.x + dx,        p.y };
    case Anchor::BottomLeft:   return { p.x,             p.y + dy };
    case Anchor::BottomCentre: return { p.x + dx * 0.5f, p.y + dy };
    case Anchor::BottomRight:  return { p.x + dx,        p.y + dy };
    case Anchor::Centre:       return { p.x + dx * 0.5f, p.y + dy * 0.5f };
    }
    return p;
}
```

Defaults chosen to match where each panel actually sits in the stock 640×480 layout
(`Panel_Type` order from `GameCommon.h:743-756`):

```cpp
DeepCore::Display::Anchor DeepCore::Display::AnchorForPanel(uint32 panelType)
{
    using namespace LegoRR;
    switch ((Panel_Type)panelType) {
    case Panel_Radar:          // top-left cluster: keep put so viewTrack need not move
    case Panel_RadarFill:
    case Panel_RadarOverlay:
    case Panel_TopPanel:          return Anchor::TopLeft;
    case Panel_Messages:
    case Panel_MessagesSide:      return Anchor::BottomLeft;
    case Panel_CrystalSideBar:    return Anchor::TopRight;
    case Panel_Information:
    case Panel_InfoDock:
    case Panel_Encyclopedia:      return Anchor::BottomRight;
    case Panel_PriorityList:
    case Panel_CameraControl:     return Anchor::BottomRight;
    default:                      return Anchor::TopLeft;
    }
}
```

> These assignments are **guesses about a layout nobody here has seen**. They are cheap to be
> wrong about and cheap to fix — that is the entire reason `panelAnchors` exists as a config
> list. Ship them as *defaults*, document them as guesses in `DeepCore.cfg`.

The relayout itself — note what it does **not** touch:

```cpp
void DeepCore::Display::RelayoutHUD(void)
{
    if (!settings.widescreenHUD) return;                     // vanilla by default

    const Size2U from = LayoutResolution();
    const Size2U to   = ScreenResolution();
    if (from.width == to.width && from.height == to.height) return;

    using namespace LegoRR;   // REQUIRED: exe macros name game types unqualified

    for (uint32 t = 0; t < (uint32)Panel_Type_Count; t++) {
        PanelData* panel = &panelGlobs.panelTable[t];        // Panels.h:128, WRITABLE

        const Anchor a = AnchorForPanel(t);
        const Point2F moved = Reanchor(panel->openPos, a);
        const Point2F delta = { moved.x - panel->openPos.x,
                                moved.y - panel->openPos.y };
        if (delta.x == 0.0f && delta.y == 0.0f) continue;

        panel->openPos.x   += delta.x;  panel->openPos.y   += delta.y;
        panel->closedPos.x += delta.x;  panel->closedPos.y += delta.y;
        panel->position.x  += delta.x;  panel->position.y  += delta.y;

        // DELIBERATELY NOT TOUCHED: panel->buttonList[b].rect.
        // Button rects are PANEL-RELATIVE -- the exe adds the panel position at
        // hit-test and draw time. Proof: Priorities.cpp:362-367 calls
        // Panel_GetPosition() and adds it to Panel_Button_GetArea()'s result.
        // Offsetting them here would move every button hit-box twice.
    }

    // interfaceGlobs.currMenuPosition is NOT patched here either -- it is fixed
    // upstream at its source, GameState.cpp:219, so that slideEndPosition
    // (Interface.h:129, "init: pointf_f78") is initialised consistently.

    if (settings.verboseStartup) {
        Error_InfoF("DeepCore: HUD relaid out from %ux%u to %ux%u",
                    from.width, from.height, to.width, to.height);
    }
}
```

## Step 3 — `src/openlrr/engine/Init.cpp` — two independent fixes  *(OURS)*

**3a. Bound `Init_AddValidMode` (unconditional correctness fix, no gate).**

```cpp
// engine/Init.cpp:326-343 -- OURS
void __cdecl Gods98::Init_AddValidMode(uint32 width, uint32 height, uint32 depth)
{
    log_firstcall();

    for (uint32 i = 0; i < initGlobs.validModeCount; i++) { /* dedupe, unchanged */ }

    /// SANITY: validModes[] is GRAPHICS_MAXMODES entries (DirectDraw.h:64) and is
    /// the LAST array in the exe-overlaid Init_Globs (Init.h:38-39,
    /// assert_sizeof 0x1d444). Overflowing it walks past the end of a pinned
    /// structure. Unreachable today (max 16), reachable the moment anything
    /// enumerates modes into it. Same class as Stats_Initialise / SFX
    /// sampleGroupTable -- see docs/WORKLOG.md.
    if (initGlobs.validModeCount >= GRAPHICS_MAXMODES) {
        Error_Warn(true, "Init_AddValidMode: validModes[] full, mode dropped");
        return;
    }

    initGlobs.validModes[initGlobs.validModeCount].width    = width;
    initGlobs.validModes[initGlobs.validModeCount].height   = height;
    initGlobs.validModes[initGlobs.validModeCount].bitDepth = depth;
    initGlobs.validModeCount++;
}
```

**3b. Stop windowed mode from requiring an enumerated display mode (§1.6).**

```cpp
// engine/Init.cpp:154-156 -- OURS
if (initGlobs.selFullScreen) {
    ok = DirectDraw_SetupFullScreen(initGlobs.selDriver, initGlobs.selDevice, initGlobs.selMode);
}
else {
    /// CHANGE: A window does not need to be a supported DISPLAY mode. When -res
    /// was given explicitly, honour it verbatim rather than falling back to
    /// whatever the driver happened to enumerate (Init_IsValidMode, :347-366).
    const Size2U winSize = (mainOptions.res.has_value()
        ? *mainOptions.res
        : (initGlobs.selMode ? Size2U { initGlobs.selMode->width, initGlobs.selMode->height }
                             : Size2U { 640, 480 }));
    ok = DirectDraw_SetupWindowed(initGlobs.selDevice, cmdPos.x, cmdPos.y,
                                  winSize.width, winSize.height);
}
```

This also removes the `640, 480` literal at `:156`. **Behaviour change even with the gate off**
— gate it on `mainOptions.res.has_value()` as written, which is exactly "the user asked for it".

## Step 4 — `src/openlrr/game/GameState.cpp` — the load-bearing lines  *(OURS)*

```cpp
// :219  icon menu origin -- see sec.3.5
const Point2I menuOrigin = DeepCore::Display::IconMenuOrigin();
Interface_Initialise((uint32)menuOrigin.x, (uint32)menuOrigin.y, legoGlobs.fontToolTip);

// :392 and :790  radar -- ONE expression, two consumers (sec.3.6)
const Area2F radarRect = DeepCore::Display::RadarRect();     // {16,13,151,151} when off
legoGlobs.viewTrack = Gods98::Viewport_CreatePixel(
    (sint32)radarRect.x, (sint32)radarRect.y,
    (uint32)radarRect.width, (uint32)radarRect.height,
    (legoGlobs.cameraTrack)->contCam);
...
legoGlobs.radarScreenRect = radarRect;      // :790, same value, no second literal

// :461,468,481,489  tutorial buttons -- build the key in LAYOUT space
const Size2U layout = DeepCore::Display::LayoutResolution();
std::sprintf(Button_buffer, "NextButton%ix%i", (sint32)layout.width, (sint32)layout.height);
...
std::sprintf(Button_buffer, "NextButtonPos%ix%i", (sint32)layout.width, (sint32)layout.height);
// then, after parsing NextButtonPos:
legoGlobs.NextButtonPos = DeepCore::Display::Reanchor(
    legoGlobs.NextButtonPos, DeepCore::Display::Anchor::BottomRight);
// ...identical treatment for BackButton / RepeatButtonPos at :481-495

// :802 and :811  THE load-bearing change -- callee untouched, argument changed
Lego_LoadPanels      (legoConfig, layout.width, layout.height);   // EXE @0x00434520
Panel_Crystals_Initialise(...);      // :804 unchanged
Panel_AirMeter_Initialise(...);      // :806 unchanged -- offsets are panel-relative
Panel_CryOreSideBar_Initialise(...); // :808 unchanged -- see sec.6 UNDETERMINED
Lego_LoadPanelButtons(legoConfig, layout.width, layout.height);   // EXE @0x00434640

// :838  advisor -- same trick, and this callee is OURS
Advisor_LoadPositions(legoConfig, legoGlobs.gameName,
                      (sint32)layout.width, (sint32)layout.height);

// NEW, after :822 (Panel_LoadInterfaceButtons_ScrollInfo) and before :830
// (Front_Initialise): everything positional has now been loaded.
DeepCore::Display::RelayoutHUD();
```

`GameState.cpp:589-592` (`TVClipDist`, `Viewport_SetBackClip` on both viewports) needs **no
change for resolution** — but see §5.4, because a wider FOV without a matched far plane and fog
just shows more fog.

## Step 5 — `src/openlrr/game/interface/Advisor.cpp`  *(OURS)*

`Advisor_LoadPositions` already takes the resolution as parameters, so Step 4 alone fixes the
(A)-class failure. The (B)-class re-anchor belongs in `Advisor_AddPosition` (`Advisor.cpp:227`),
and **only for the NOPANEL case** — positions attached to a panel already follow that panel
through `Panel_GetPosition` at `Advisor.cpp:378-383`:

```cpp
// game/interface/Advisor.cpp:227 -- OURS
void __cdecl LegoRR::Advisor_AddPosition(Advisor_Type advisorType, Advisor_Anim animType,
        Text_Type textType, SFX_ID sfxID, Panel_Type panelType, real32 x, real32 y)
{
    /// CHANGE: A position with no panel is an ABSOLUTE screen coordinate, so it
    /// must be re-anchored. A position WITH a panel is panel-relative and is
    /// already carried by RelayoutHUD moving the panel (see :378-383).
    if (panelType == Panel_Type_Count) {   // NULL panel
        const Point2F p = DeepCore::Display::Reanchor(
            Point2F { x, y }, DeepCore::Display::Anchor::Centre);
        x = p.x;  y = p.y;
    }

    advisorGlobs.positions[advisorType].flags    = ADVISOR_FLAG_USED;
    ... // rest unchanged, including origX/origY = x/y
}
```

Note `origX`/`origY` are assigned from the same `x`/`y` (`Advisor.cpp:232-233`), so they stay
consistent for free.

## Step 6 — `src/openlrr/game/mission/Objective.cpp`  *(OURS)*

`Objective_LoadLevel` (`:371`) is ours and hooked (`interop.cpp:3782`); its only caller is EXE,
so we ignore the incoming parameters:

```cpp
// game/mission/Objective.cpp:371 -- OURS, hooked @0x00458000
void __cdecl LegoRR::Objective_LoadLevel(const Gods98::Config* config, const char* gameName,
        const char* levelName, Lego_Level* level, uint32 screenWidth, uint32 screenHeight)
{
    /// CHANGE: The EXE caller passes appWidth()/appHeight(). Look the image keys
    /// up in LAYOUT space instead, so the stock "ObjectiveImage640x480" entries
    /// resolve at any screen size. Identical to the incoming values when the
    /// Display gate is off.
    const Size2U layout = DeepCore::Display::LayoutResolution();
    screenWidth  = layout.width;
    screenHeight = layout.height;
    ...
```

and re-anchor the three loaded positions — `panelImagePosition` (`:414`),
`achievedImagePosition` (`:450`), `failedImagePosition` (`:511`) — with `Anchor::Centre`.
`achievedVideoPosition` (`:534`) too, if `noAchievedVideoPosition` is false.

## Step 7 — `src/openlrr/game/front/FrontEnd.cpp`  *(OURS, 106 functions, 0 live macros)*

The front end is the single largest *visible* breakage and the single cleanest fix, because
**every menu item's draw position and its hit-test both derive from `menu->position`**
(draw: `:1024-1025`, `:2314-2315`; hit-test: `:1055-1056`, `:1131-1132`). Translate three
fields at load and the whole front end moves coherently:

```cpp
// game/front/FrontEnd.cpp:850-851, inside Front_Menu_CreateMenu -- OURS
const Point2I fe = DeepCore::Display::FrontEndOffset();   // {0,0} when off
menu->position.x = positionX + fe.x;
menu->position.y = positionY + fe.y;

// :794-795, inside Front_Menu_LoadMenuImage -- OURS
menu->currPosition.x = (real32)std::atof(stringParts[1]) + (real32)fe.x;
menu->currPosition.y = (real32)std::atof(stringParts[2]) + (real32)fe.y;

// :863-864 -- OURS
menu->anchoredPosition.x = std::atoi(stringParts[0]) + fe.x;
menu->anchoredPosition.y = std::atoi(stringParts[1]) + fe.y;
```

plus three literal fixes in the same file:

```cpp
// :1963-1982  Front_Menu_DrawMenuImage -- the non-HASPOSITION background path
const Point2F destPos = { (real32)fe.x, (real32)fe.y };   // was { 0.0f, 0.0f }
const Area2F srcArea = { -(real32)frontGlobs.scrollOffset.x,
                         -(real32)frontGlobs.scrollOffset.y,
                         (real32)DeepCore::Display::LayoutResolution().width,
                         (real32)DeepCore::Display::LayoutResolution().height };

// :1771-1775  Front_MenuItem_DrawSelectItem -- the cull, which currently HIDES
// items rather than misplacing them
if (image &&
    ((x + Gods98::Image_GetWidth(image))  > 0 && x < Gods98::appWidth()) &&
    ((y + Gods98::Image_GetHeight(image)) > 0 && y < Gods98::appHeight()))

// :2433-2449  Front_Menu_UpdateMousePosition -- replace 480.0f with appHeight()
```

`FrontEndOffset()` returns `{(appWidth()-layoutWidth)/2, (appHeight()-layoutHeight)/2}` when
`frontEndCentre`, else `{0,0}`. **Keep this gate separate from `widescreenHUD`** — the front
end and the in-game HUD fail independently and should be bisectable independently.

## Step 8 — `data/Settings/DeepCore.cfg`  *(data)*

Following the existing house style (`data/Settings/DeepCore.cfg:1-18`):

```ini
;;;;;;;;;;;;;;;;;;;;;;;;;; DISPLAY / WIDESCREEN ;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; The RESOLUTION itself is a command-line option, not a setting here:
;
;   OpenLRR.exe -res 1920x1080 -window
;   OpenLRR.exe -res 1280x720  -fullscreen
;   OpenLRR.exe -scale 2                     ; 2x supersample of 640x480
;
; Fullscreen -res only works for a mode your display driver actually reports.
; Windowed -res is honoured verbatim.
;
; The settings BELOW govern LAYOUT: where the HUD ends up once the screen is
; bigger than the one the artwork was drawn for.
;
; HONEST WARNING, read it: the panel ARTWORK is fixed-size bitmaps inside the
; game's own WAD files. Turning this on gives you correctly-placed 640-era
; panels with GAPS between them on a wide screen. It does not give you wider
; panels. That needs new art, which this project does not ship.
;
; Compile-verified, NOT play-tested. Nobody involved has run this.

; Master gate for HUD re-anchoring. FALSE == today's behaviour exactly.
WidescreenHUD               FALSE

; The resolution your Lego.cfg's HUD blocks are authored for. Change this only
; if you have a config with, say, "Panels800x600" blocks and want those used.
LayoutWidth                 640
LayoutHeight                480

; Per-panel anchor overrides, "PanelName:Anchor", space or comma separated.
; Anchors: TopLeft TopCentre TopRight BottomLeft BottomCentre BottomRight Centre
; The built-in defaults are GUESSES -- nobody here has seen the layout run.
; If a panel lands in the wrong corner, this is the knob.
;   Panel_CrystalSideBar:TopRight  Panel_Messages:BottomLeft
PanelAnchors

; Centre the front-end menus (title screen, options, level select) instead of
; leaving them in the top-left corner. Separate gate from WidescreenHUD on
; purpose: these two fail independently, so bisect them independently.
FrontEndCentre              FALSE

; 3D view aspect handling. See docs/research/widescreen-plan.md section 5.
;   Off   -- never touch the field of view (default, and what ships today)
;   Auto  -- measure the projection once at startup and correct only if the
;            renderer turns out to stretch rather than widen
;   Hor   -- assume widening (do nothing, but log the assumption)
;   Vert  -- assume stretching, compensate by (4/3) / (width/height)
AspectMode                  Off

; Extra multiplier on the top-down field of view. 1.0 == unchanged.
; Values BELOW 1.0 zoom IN (Viewport_SetField is a half-extent, not an angle).
FieldOfViewScale            1.0
```

## Step 9 — `src/openlrr/game/Game.cpp` — pair FOV with clip and fog  *(OURS)*

Only if `fieldOfViewScale != 1.0` or `aspectMode != Off`. `Lego_SetViewMode`
(`Game.cpp:2867-2907`) sets `Viewport_SetField(viewMain, 0.5f)` for top-down (`:2900`) and
`SmoothSetField(0.9f | 0.6f)` for first-person (`:2870-2871`), and `Lego_SetSceneFogParams`
(`Game.cpp:2843-2864`, already `/// CUSTOM:` and therefore wholly ours) hard-codes
`density = 0.0032f`, a `3/4` start ratio and a `0.8` top-down multiplier. **Widening the field
without moving the far plane and the fog start just shows more fog.** Ship them together or
ship neither.

## Step 10 — docs  *(docs)*

`docs/WORKLOG.md` and `docs/HANDOFF-*.md`: record what is verified (compiles, 0 errors /
exactly 44 warnings, `addrlint` 113/0 unchanged) and what is not (**all behaviour**).

## Step 11 — regression guard

`tools/addrlint/addrlint.py` must still print `113 sized regions … 376 assert_sizeof types`
with no overlap lines. No step above changes a `sizeof` or a binding address, so this should be
mechanical — but run it, because it is the one acceptance test this project can actually
execute.

---

# 5. ASPECT CORRECTNESS

## 5.1 Where the projection is set

There is exactly one main viewport and exactly one field-of-view control path:

```cpp
// game/GameState.cpp:385 -- OURS
legoGlobs.viewMain = Gods98::Viewport_Create(0.0f, 0.0f, 1.0f, 1.0f, (legoGlobs.cameraMain)->contCam);
```

```cpp
// engine/gfx/Viewports.cpp:58-74 -- OURS
Gods98::Viewport* __cdecl Gods98::Viewport_Create(real32 xPos, real32 yPos,
                                                  real32 width, real32 height, Container* camera)
{
    uint32 devWidth  = lpDevice()->GetWidth();
    uint32 devHeight = lpDevice()->GetHeight();
    devWidth  /= Main_RenderScale();
    devHeight /= Main_RenderScale();

    sint32 actXPos   = (sint32)(xPos * devWidth);
    sint32 actYPos   = (sint32)(yPos * devHeight);
    uint32 actWidth  = (uint32)(width  * devWidth);
    uint32 actHeight = (uint32)(height * devHeight);

    return Viewport_CreatePixel(actXPos, actYPos, actWidth, actHeight, camera);
}
```

which lands in `lpD3DRM()->CreateViewport(lpDevice(), camera->masterFrame, xPos, yPos, width,
height, &newViewport->lpVP)` (`Viewports.cpp:104`). **So the viewport already fills the whole
surface at any aspect — no code change is needed for it to cover a 16:9 screen.**

The only projection knobs we hold:

| Knob | Site | Ownership |
| --- | --- | --- |
| `Viewport_SetField(vp, fov)` → `lpVP->SetField(fov)` | `Viewports.cpp:321-330` | OURS |
| `Viewport_SmoothSetField(vp, fov)` (interpolated in `Viewport_Render`, `:269-281`) | `Viewports.cpp:312-317` | OURS |
| `Viewport_GetField(vp)` | `Viewports.cpp:333-341` | OURS |
| top-down field `0.5f` | `Game.cpp:2900` | OURS |
| first-person field `0.9f` / `0.6f` | `Game.cpp:2870` | OURS |
| back clip `TVClipDist` / `FPClipBlocks * BlockSize` | `Game.cpp:2887, 2901`; `GameState.cpp:589-592` | OURS |

## 5.2 The question, and why it is not answerable from here

For a widescreen view to show **more horizontally** (Hor+) rather than stretch, the projection
must derive its horizontal extent from the viewport's own width using the *same* per-unit scale
it uses vertically. Whether `IDirect3DRMViewport2` does that is a property of the closed
1998 `d3drm.dll` implementation.

* It is not in this repo. `grep -rn "aspect" --include=*.cpp --include=*.h src/openlrr` finds
  nothing relevant; `Viewports.h:69-80` stores only `xoffset/yoffset/width/height`, the RM
  pointer, `smoothFOV` and `rendering`.
* It is not derivable from usage. The only three modes the game ever registered are 640×480
  (`Init.cpp:85`) and, under `-debug`, 800×600 and 1024×768 (`Init.cpp:87-88`) — **all 4:3**.
  There is no non-4:3 data point anywhere in this project's history.
* I searched for the D3DRM specification of `SetField` and did not find a statement precise
  enough to rely on. The sources found describe it only as "the camera's visible range,
  corresponding to angles", with no statement about how viewport width and height enter the
  projection.

**Therefore: UNDETERMINED. Do not assert Hor+.** The prior document's "should widen (Hor+)"
is a plausible guess, correctly flagged there as UNDETERMINED, and it stays that way here.

## 5.3 What to do instead — let the program measure it

This is the useful contribution. The renderer will tell us the answer at runtime, and the code
that asks is ordinary C++ that compiles on this machine.

`Viewport_InverseTransform` maps a screen point (plus a depth in `z`) back to a world point
(`Viewports.cpp:344-361`), and `RadarMap.cpp:482-500` already uses exactly this idiom to build
the camera's ground footprint from the four screen corners. Un-project three screen points that
form a small right-angled cross at the screen centre and compare the world-space lengths of the
two arms:

* **Equal** ⇒ one world unit per pixel horizontally and vertically ⇒ **square pixels ⇒ Hor+**.
  A 16:9 viewport already shows more world horizontally. Do nothing.
* **Ratio ≈ (width/height)/(4/3)** ⇒ the projection normalised x by the aspect ⇒ **stretch**.
  Compensate through the one control we have.

```cpp
// game/DeepCore.cpp -- PROJECT. Called once, after Lego_SetViewMode has set a
// field, from Lego_Initialise. Read-only with respect to the renderer.
static real32 _MeasurePixelSkew(Gods98::Viewport* vp)
{
    const real32 cx = (real32)Gods98::appWidth()  * 0.5f;
    const real32 cy = (real32)Gods98::appHeight() * 0.5f;
    const real32 d  = 64.0f;   // pixels; large enough to swamp float noise

    // z = 1.0 == the far plane, matching RadarMap.cpp:496-499's usage.
    const Vector4F sC = { cx,     cy,     1.0f, 1.0f };
    const Vector4F sX = { cx + d, cy,     1.0f, 1.0f };
    const Vector4F sY = { cx,     cy + d, 1.0f, 1.0f };

    Vector3F pC, pX, pY;
    Gods98::Viewport_InverseTransform(vp, &pC, &sC);
    Gods98::Viewport_InverseTransform(vp, &pX, &sX);
    Gods98::Viewport_InverseTransform(vp, &pY, &sY);

    const real32 wx = Gods98::Maths_Vector3DDistance(&pX, &pC);   // Maths.h:198
    const real32 wy = Gods98::Maths_Vector3DDistance(&pY, &pC);
    return (wy > 0.0001f) ? (wx / wy) : 1.0f;   // 1.0 == square pixels == Hor+
}

void DeepCore::Display::ApplyAspect(Gods98::Viewport* viewMain)
{
    if (settings.aspectMode == "Off") return;

    const real32 aspect = (real32)Gods98::appWidth() / (real32)Gods98::appHeight();
    real32 correction = 1.0f;

    if (settings.aspectMode == "Vert") {
        correction = (4.0f / 3.0f) / aspect;
    }
    else if (settings.aspectMode == "Auto") {
        const real32 skew = _MeasurePixelSkew(viewMain);
        // Square pixels (skew ~ 1) means the renderer is already Hor+.
        correction = (std::fabs(skew - 1.0f) < 0.02f) ? 1.0f : (1.0f / skew);
        Error_InfoF("DeepCore: measured projection pixel skew %.4f at %ix%i "
                    "(1.0 == square pixels); field correction %.4f",
                    (double)skew, Gods98::appWidth(), Gods98::appHeight(),
                    (double)correction);
    }
    // "Hor" falls through with correction == 1.0, but still logs via verbose.

    correction *= settings.fieldOfViewScale;
    if (correction != 1.0f) {
        Gods98::Viewport_SetField(viewMain,
            Gods98::Viewport_GetField(viewMain) * correction);   // Viewports.cpp:321,333
    }
}
```

Two honest limitations, stated rather than hidden:

1. **Ordering.** `Viewport_SetField` must be re-applied after every `Lego_SetViewMode`
   (`Game.cpp:2870, 2900`), because that function sets the field unconditionally. The clean
   place is at the end of `Lego_SetViewMode` itself — which is **OURS**.
2. **`Auto` is a measurement, not a proof.** It tells you the *pixel* skew, which is the thing
   that matters visually, but it cannot tell you whether the far plane `z = 1.0` behaves as
   assumed under this particular device. If the numbers come back absurd (`skew > 4` or
   `< 0.25`), fall back to 1.0 and log. Add that clamp.

## 5.4 Whatever the answer, ship FOV with clip and fog

`Lego_SetSceneFogParams` (`Game.cpp:2843-2864`, `/// CUSTOM:`, ours outright) computes fog start
as `TVClipDist * 3/4` and end as `TVClipDist`. A wider field with an unchanged far plane and
unchanged fog means the extra horizontal world you gained is grey. `legoGlobs.TVClipDist` is
read at `GameState.cpp:589` and applied at `:591-592` and `Game.cpp:2887, 2901`. Four sites,
all ours, one multiplier.

---

# 6. RISKS AND WHAT IS UNDETERMINED

## Risks, ranked

| # | Risk | Why it matters | Mitigation |
| --- | --- | --- | --- |
| 1 | **`Lego_LoadPanels` may not key on `<W>x<H>` at all.** `Panels.h` is 0/52 implemented; the `Panels<W>x<H>` block name is an *inference* from the `(config, screenWidth, screenHeight)` signature (`Game.h:1902`) and from the eight readable `%ix%i` sites | If the inference is wrong, Step 4's central change does nothing and the panels are still absent | The change is a **no-op when layout == screen**, so a stock run is unaffected either way. Verify with `VerboseStartup` plus a `PANEL_FLAG_HASIMAGE` count after `:811` — one line, and it is a real test the first person with the game can run |
| 2 | **Panel anchor defaults are guesses.** Nobody here has seen the HUD | Panels could land in the wrong corners | `PanelAnchors` config list exists precisely for this. Document the defaults as guesses |
| 3 | **`cryOreSideBarOffset` (615, 434) may be absolute.** `Panels.h:145` carries no "relative to" comment where `airMeterJuiceOffset` (`:140`) and `airMeterNoAirOffset` (`:142`) both do | If absolute, the crystal/ore side bar stays at 615,434 while its panel moves to the right edge | **Step 4 deliberately leaves `Panel_CryOreSideBar_Initialise` untouched.** Add a config-gated re-anchor as a follow-up once someone can look at it |
| 4 | **The `viewTrack` viewport cannot move after creation** (`GameState.cpp:392`; no `Viewport_SetPosition` in `Viewports.h:86-118`) | A user who anchors the radar anywhere but top-left gets a radar image and a radar hit-test that disagree | Default `Panel_Radar` to `Anchor::TopLeft` so `delta == 0`; make any other radar anchor derive `RadarRect()` **before** `:392` |
| 5 | **`-scale` in fullscreen is already suspect** (`Main.cpp:1263-1266` bypasses `Main_IsScaleSupported`, `Main.cpp:146-147`) | Pre-existing, not caused by this work, but a user combining `-res` and `-scale` will hit it | Do not fix it in this workstream; record it. It is a two-line guard in the parser and belongs in its own commit |
| 6 | **Undecompiled exe drawing code may hard-code screen coordinates** we cannot see | Unknowable extent | This is the reason the gate defaults FALSE and the reason `frontEndCentre` is a separate gate from `widescreenHUD` |
| 7 | **The warning budget.** 0 errors / exactly 44 warnings at `/W3` | A single new warning fails the build contract | The float↔int conversions in `Reanchor`/`RadarRect` are the likely culprits — write every cast explicitly, never rely on implicit narrowing |
| 8 | **`Init.cpp` Step 3b changes startup behaviour when `-res` is present**, even with the Display gate off | A user who already passes `-res` gets a different (better) windowed path | Correct as designed, but call it out in the commit message; it is not covered by the DeepCore gate |

## UNDETERMINED — cannot be settled without running the game

1. Whether the config block `Lego_LoadPanels` reads is named `Panels<W>x<H>` (risk #1).
2. Whether a wide viewport is Hor+ or stretch (§5.2). `AspectMode Auto` (§5.3) answers it at
   runtime; it does not answer it here.
3. Whether `cryOreSideBarOffset` and `rotateCenter` are absolute or panel-relative (risk #3).
4. Which corner each of the twelve panels actually occupies, and therefore whether the default
   anchor table is right.
5. Whether `Interface_SetScrollParameters` (EXE `@0x0041e900`) is ever called by exe code with
   hard-coded 640-era coordinates, which would fight the `IconMenuOrigin()` fix.
6. Whether the driver enumerates any given `-res` mode in fullscreen (§1.6) — hardware-dependent
   by definition.
7. Whether any FLIC or AVI called with a 640-era `destArea` looks acceptable letterboxed in a
   corner, or needs centring. Centring is easy; knowing it is needed is not.
8. Every behavioural claim in this document. **There is no installation of the game on this
   machine and no build was run for this document.**

---

# DECISION — ranked plan

**SHIP, in this order, as four separate commits.** The ordering is chosen so each commit is
independently revertible and each has its own bisect handle.

### Commit 1 — correctness, no gate, no feature *(smallest, ship first)*
* `Init.cpp:326-343` — bound `Init_AddValidMode` against `GRAPHICS_MAXMODES` (Step 3a).
* `GameState.cpp:392` + `:790` — derive the radar rect from **one** expression instead of two
  independent literals that happen to agree.

Rationale: both are latent bugs today, both are unconditional, neither depends on anything else
in this plan, and the second one is a prerequisite the rest of the work would otherwise smuggle
in.

### Commit 2 — the Display layer, HUD only, gated FALSE
Steps 1, 2, 4, 5, 6. `DeepCore.hpp/.cpp` Display block; `GameState.cpp` layout-resolution
substitution at `:219, :461-495, :802, :811, :838` plus `RelayoutHUD()` after `:822`;
`Advisor.cpp:227`; `Objective.cpp:371` and its three positions.

Rationale: this is the actual feature, and it is the one that turns "the HUD does not exist at
1920×1080" into "the HUD exists at 1920×1080". Everything in it is a no-op when
`WidescreenHUD FALSE`, and a **strict** no-op when layout == screen, so a stock run is
byte-identical.

### Commit 3 — front end, separate gate
Step 7. `FrontEnd.cpp:794-795, 850-851, 863-864, 1771-1775, 1963-1982, 2433-2449`.

Rationale: 100% ours, entirely self-contained, and it fails independently of the HUD — so it
gets its own gate (`FrontEndCentre`) and its own commit.

### Commit 4 — windowed resolution + aspect + the FOV/clip/fog trio
Steps 3b, 9, and §5.3's `ApplyAspect` with `AspectMode Auto`.

Rationale: this is the part with the genuinely unknown answer in it (§5.2). Keeping it last
means the first three commits are useful even if this one has to be reworked once somebody can
see the screen.

### Explicitly NOT in scope
* No new `hook_write_jmpret`. The install count stays 1206 active / 1515 total.
* No reimplementation of `Lego_LoadPanels`, `Lego_LoadPanelButtons`, `Interface_Initialise`,
  `Panel_Draw`, or anything else in `Panels.h`.
* No change to any `assert_sizeof` type, no change to any binding address, no change to
  `interop.cpp`. `addrlint` stays at **113 regions / 0 overlaps**.
* No touching `panelGlobs.panelTable[t].buttonList[b].rect` — see §3.4.
* No attempt to make the panel **artwork** wider. It is fixed-size bitmaps in the game's own
  WAD files. Repositioning is possible; resizing needs art this project does not ship.
* No early `DeepCore::Load()`. Resolution stays a command-line concern until somebody wants it
  otherwise, and then it is its own piece of work (Step 0b).
