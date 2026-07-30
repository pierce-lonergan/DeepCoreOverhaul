# The Wave Director

**Timed monster waves independent of map emerge triggers.**

Status: **design + feasibility study. Nothing in this document has been written to the
tree yet.** Every claim below is cited to `file:line` in
`C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`. Where I could not
determine something from source, it says so explicitly — those are marked
**UNDETERMINED** and are load-bearing for the plan.

This is Tier 2 item 1.5 in `docs/OVERHAUL-PLAN.md:88`
("Spawn/wave director in `MainLoop_Wrapper`", value **high**, size M, no new object ID,
no new art), and the plan already names the two anchors this document expands:
`LegoObject_TryGenerateSlugAtBlock` at `Object.cpp:1596-1632` and the wrapper slot at
`OpenLRR.cpp:970` (`docs/OVERHAUL-PLAN.md:66`).

Reminder of the constraint that shapes everything here: we build a DLL injected over the
original 1999 executable, dozens of structs are references overlaid on that executable's
data segment and pinned by `assert_sizeof`, and **no struct carrying `assert_sizeof` may
change layout**. The address-space linter agrees the current layout is self-consistent —
113 sized overlaid regions, 0 overlaps (`docs/ADDRESS-MAP.md:15-17`). *The wave director
proposed here adds zero bytes to any overlaid struct.* All of its state is DLL-side, the
pattern already used for the PowerGrid vectors at `Game.cpp:168-170`.

We also cannot run the game. **Compile-verified is the ceiling.** Nothing below has been
play-tested and nothing below may be described as play-tested.

---

## 0. Executive summary

| Question | Answer |
| --- | --- |
| Is a from-scratch monster spawn possible? | **Yes.** The recipe is proven in-tree by `LegoObject_TryGenerateSlugAtBlock` (`Object.cpp:1602-1639`). Four of its five calls are already ours; only `AITask_DoAnimationWait` and `Info_Send` are still exe address macros. |
| Can we reuse that function directly? | **No.** It hard-refuses any block that is not a registered slug hole (`Object.cpp:1608-1616`). A sibling function is required. |
| Can we find a legal spawn block? | **Yes**, from `Lego_Block::flags1/flags2` directly (`Game.h:341-368`, `Game.h:184-245`). `BLOCK1_WALL` vs `BLOCK1_HIDDEN` is exactly the "exposed wall vs undiscovered rock" distinction (`Game.cpp:1912-1916`). |
| Can we count living monsters? | **Yes.** `objectListSet.EnumerateSkipUpgradeParts()` is a real C++ range-for and is already used from eight other translation units. `.Count()` exists (`GameState.cpp:1791`). |
| Where does it tick? | `OpenLRR_MainLoop_Wrapper` post-MainLoop, `OpenLRR.cpp:970-985`, gated on `Lego_IsInLevel()` (`Game.h:812`). Front-end menus run *inside* `Lego_MainLoop`, so the guard is mandatory, not decorative. |
| Time unit? | "Standard units", 25.0 == 1 second (`common.h:109`), hard-capped at 3.0 per tick (`Main.cpp:395`, `Main.cpp:440-446`). |
| Telegraphing? | Four implemented channels: `Info_Send` (exe macro, panel message with a block position), `SFX_Random_PlaySoundNormal` (**ours**, `SFX.cpp:311`), `Camera_Shake` (**ours**, `Camera.cpp:192`), `Smoke_CreateSmokeArea` (**ours**, `Smoke.cpp:68`). |
| New hooks needed? | **None.** No new `hook_write_jmpret`. The director is called only from our own code. |
| Build-contract risk | Two new files must be added to `openlrr.vcxproj` (`ClInclude`/`ClCompile`, mirroring `game\DeepCore.hpp` at line 258 and `game\DeepCore.cpp` at line 384). Warning-count risk is discussed in §9. |

Two real defects were found while reading the spawn path. They are written up in §1.4 and
§1.5 because the director must not inherit them.

---

## 1. The exact spawn recipe

### 1.1 The proven path, quoted in full

`src/openlrr/game/object/Object.cpp:1602-1639`, exactly as it stands in the tree:

```cpp
/// CUSTOM: Generation for slug with a specific block pos already specified.
LegoRR::LegoObject* LegoRR::LegoObject_TryGenerateSlugAtBlock(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 bx, uint32 by, real32 heading, bool assignHeading)
{
	if (objType != LegoObject_RockMonster)
		return nullptr;

	bool isSlugHole = false;
	for (uint32 i = 0; i < objectGlobs.slugHoleCount; i++) {
		if (objectGlobs.slugHoleBlocks[i].x == bx && objectGlobs.slugHoleBlocks[i].y == by) {
			isSlugHole = true;
			break;
		}
	}
	if (!isSlugHole)
		return nullptr;

	// If no heading supplied, generate a random one.
	if (!assignHeading) {
		heading = Gods98::Maths_RandRange(0.0f, M_PI*2.0f);
	}

	Point2F wPos = { 0.0f }; // dummy init
	Map3D_BlockToWorldPos(Lego_GetMap(), bx, by, &wPos.x, &wPos.y);
	wPos.x += Maths_Sin(heading) * 12.75f;
	wPos.y += Maths_Cos(heading) * 12.75f;

	LegoObject* slugObj = LegoObject_CreateInWorld(objModel, objType, objID, 0, wPos.x, wPos.y, heading);
	if (slugObj != nullptr) {
		slugObj->flags1 |= LIVEOBJ1_EXPANDING;
		slugObj->flags3 &= ~LIVEOBJ3_POWEROFF;

		LegoObject_SetActivity(slugObj, Activity_Emerge, 0);
		LegoObject_UpdateActivityChange(slugObj);
		AITask_DoAnimationWait(slugObj);
		Info_Send(Info_SlugEmerge, nullptr, slugObj, nullptr);
	}
	return slugObj;
}
```

Its only caller is `LegoObject_TryGenerateSlug` (`Object.cpp:1585-1600`), which is itself
hooked over the exe at `0x0043b010` (`interop.cpp:3609`). The `AtBlock` variant is *not*
hooked — it is a pure DLL-side helper, and that is precisely the precedent the director
follows.

### 1.2 Step by step

| # | Step | What it does / why | Source of truth |
| --- | --- | --- | --- |
| 1 | `objType != LegoObject_RockMonster` guard | All creature-shaped objects (monsters, slugs, bats, spiders) live under `LegoObject_RockMonster` and are distinguished by `objID`. The guard exists because the rest of the body assumes `objModel` is a `CreatureModel*`. | `Object.cpp:1605-1606`; `legoGlobs.rockMonsterData` is `CreatureModel*` at `Game.h:533` |
| 2 | slug-hole membership scan | **This is the part the director must NOT copy.** It rejects every block that is not in `objectGlobs.slugHoleBlocks`. | `Object.cpp:1608-1616` |
| 3 | random heading | `Maths_RandRange(0, 2π)`. `Maths_RandRange` is `Gods98::` (`Maths.h:47`); `Maths_Sin`/`Maths_Cos` are plain `#define`s over `std::sin/cos` and are namespace-agnostic (`Maths.h:99-100`). | `Object.cpp:1618-1621` |
| 4 | `Map3D_BlockToWorldPos` → world 2D | Converts block indices to a world position. Elsewhere the unmodified result of this call is used directly as a *building's* spawn position (`Construction.cpp:895-901`), so it is the block **centre**, not a corner. | `Object.cpp:1624` |
| 5 | `+= sin/cos(heading) * 12.75f` | Pushes the spawn off dead-centre along the facing direction. **UNDETERMINED:** I could not derive `12.75` from any constant in the tree. `Map3D_BlockSize` is still an exe macro (`Map3D.h:345`) and the config `BlockSize` is per-level (`Game.h:400`, `Map3D.h:117`), so I cannot confirm this is "a fixed fraction of a block". Treat 12.75 as an empirical magic number that is known to look right for slugs and *unverified* for anything else. | `Object.cpp:1625-1626` |
| 6 | `LegoObject_CreateInWorld` | `LegoObject_Create` + `LegoObject_SetPositionAndHeading(..., assignHeading = true)`. Returns `nullptr` if creation failed. | `Object.cpp:1628`; impl `Object.cpp:1294-1305` |
| 7 | `flags1 \|= LIVEOBJ1_EXPANDING` | Marks the object as mid-emerge. While set, `LegoObject_IsActive` returns false (`Object.cpp:1283-1284`), so the monster cannot be shot, damaged or tasked until the emerge animation has produced a positive `animTime`. It is cleared in `LegoObject_Callback_Update` at `Object.cpp:3214-3227`. | `Object.cpp:1630` |
| 8 | `flags3 &= ~LIVEOBJ3_POWEROFF` | `LegoObject_Create` **sets** `LIVEOBJ3_POWEROFF` on every RockMonster (`Object.cpp:990`). `LegoObject_IsActive(obj, /*ignoreUnpowered=*/false)` requires it clear (`Object.cpp:1281-1287`). Clearing it is what makes the creature count as a live, active thing. **Skipping this step yields an inert monster.** | `Object.cpp:1631` |
| 9 | `LegoObject_SetActivity(obj, Activity_Emerge, 0)` | Selects the `Emerge` animation. `Activity_Emerge == 12` (`GameCommon.h:901`) and the name is registered at `Object.cpp:107`, i.e. it is resolved from the creature's `.ae` file by name. | `Object.cpp:1633` |
| 10 | `LegoObject_UpdateActivityChange(obj)` | Actually swaps the container's animation to the activity chosen in step 9. Without it the activity is requested but never applied. | `Object.cpp:1634`; impl `Object.cpp:5582` |
| 11 | `AITask_DoAnimationWait(obj)` | Pushes an `AITask_Type_AnimationWait` (`GameCommon.h:287`) so the AI does not immediately override the emerge animation with a walk/attack task. | `Object.cpp:1635` |
| 12 | `Info_Send(Info_SlugEmerge, ...)` | Raises the panel alert. Passing the object (not a block) makes the message follow the creature. | `Object.cpp:1636` |

### 1.3 OURS vs exe address macro

This is the table that matters for the plan, because an exe macro cannot be
namespace-qualified and its expansion names `LegoRR` types unqualified — so every call
site needs `using namespace LegoRR;` in scope. That trap is already documented in-tree at
`DeepCore.cpp:79-83`.

| Symbol | Status | Evidence |
| --- | --- | --- |
| `LegoObject_TryGenerateSlugAtBlock` | **OURS** (pure custom, not hooked) | decl `Object.h:966` (no `#define`), impl `Object.cpp:1603` |
| `LegoObject_TryGenerateSlug` | **OURS**, hooked over exe | `Object.h:962`, `interop.cpp:3609` |
| `LegoObject_CreateInWorld` | **OURS**, hooked over exe | `Object.h:708-709` (macro commented out), `interop.cpp:3593` |
| `LegoObject_Create` | **OURS** | `Object.h:664-665` |
| `LegoObject_SetPositionAndHeading` | **OURS** | `Object.h:1274-1275` |
| `LegoObject_SetActivity` | **OURS** | `Object.h:1188-1189`, impl `Object.cpp:4084` |
| `LegoObject_UpdateActivityChange` | **OURS** | `Object.h:1460-1461`, impl `Object.cpp:5582` |
| `LegoObject_GetBlockPos` | **OURS** | `Object.h:1256-1257` |
| `LegoObject_GetPosition` | **OURS** | `Object.h:1240-1241` |
| `LegoObject_IsActive` | **OURS** | impl `Object.cpp:1281` |
| `LegoObject_RunThroughLists` / `…SkipUpgradeParts` | **OURS** | `Object.h:636-641`, impl `Object.cpp:621`, `Object.cpp:633` |
| `Camera_Shake` | **OURS** | `Camera.h:167`, impl `Camera.cpp:192` |
| `SFX_Random_PlaySoundNormal` | **OURS** | `SFX.h:196`, impl `SFX.cpp:311` |
| `SFX_Random_PlaySound3DOnContainer` | **OURS** | `SFX.h:211`, impl `SFX.cpp:381` |
| `Smoke_CreateSmokeArea` | **OURS**, hooked | `Smoke.h:145`, impl `Smoke.cpp:68`, `interop.cpp:4066` |
| `Text_SetNERPsMessage` | **OURS** | `TextMessages.h:135`, impl `TextMessages.cpp:177` |
| `Level_Free` | **OURS** | `Game.h:1533`, impl `Game.cpp:3302` |
| `Lego_LoadLevel2` | **OURS** (custom wrapper) | `Game.h:1379`, impl `Game.cpp:2951` |
| `Level_Block_IsRockFallFX` | **OURS** | `Game.h:1745-1746` |
| `Level_Block_IsInitiallyExposed` | **OURS** | `Game.h:1789-1790` |
| `Level_Block_IsPowered` | **OURS** | `Game.h:1653-1654` |
| — | — | — |
| `AITask_DoAnimationWait` | **exe macro** `0x00403e60` | `AITask.h:549-550` |
| `Info_Send` | **exe macro** `0x00419ab0` | `InfoMessages.h:267-268` |
| `Info_HasTypeText` | **exe macro** `0x00419a30` | `InfoMessages.h:255` |
| `Map3D_BlockToWorldPos` | **exe macro** `0x0044f900` | `Map3D.h:264-265` |
| `Map3D_WorldToBlockPos_NoZ` | **exe macro** `0x0044f990` | `Map3D.h:268-269` |
| `Map3D_BlockSize` | **exe macro** `0x00450b50` | `Map3D.h:345-346` |
| `LegoObject_TryGenerateRMonster` | **exe macro** `0x0043b1f0` | `Object.h:974-975` |
| `LegoObject_TryGenerateRMonsterAtRandomBlock` | **exe macro** `0x0043b160` | `Object.h:969-970` |
| `LegoObject_IsRockMonsterCanGather` | **exe macro** `0x004439b0` | `Object.h:1299-1300` |
| `Level_Block_IsWall` / `IsGround` / `IsBusy` / `SetBusy` / `IsLava` / `IsSurveyed` / `IsSolidBuilding` / `GetRubbleLayers` | **exe macros** | `Game.h:1757`, `1749`, `1777`, `1781`, `1797`, `1805`, `1741`, `1765` |
| `Lego_GetCrossTerrainType` | **exe macro** `0x00431cd0` | `Game.h:1645-1646` |
| `Lego_SetCallToArmsOn` | **exe macro** `0x004359d0` | `Game.h:1990-1991` |
| `Level_BlockUpdateSurface` | **exe macro** `0x0042f620` | `Game.h:1545-1546` |
| `Lego_GetObjectByName` | **exe macro** `0x0042e7e0` | `Game.cpp:3291`; already used from DeepCore via a function-scope `using` (`DeepCore.cpp:83`) |

`Lego_GetLevel()` and `Lego_GetMap()` are plain `__inline` functions, not macros
(`Game.h:1606`, `Game.h:1610`), so they are safe anywhere.

`ObjectModel` is `typedef void ObjectModel;` (`GameCommon.h:59`), so passing a
`CreatureModel*` where an `ObjectModel*` is expected is an implicit standard conversion —
no cast needed, exactly as `Object.cpp:1596-1597` already does.

### 1.4 DEFECT FOUND — `targetBlockPos` is never set, and one exe path dereferences it

This is the single most important finding in this document.

`LegoObject_Create_internal` initialises every new object's `targetBlockPos` to
`(-1.0f, -1.0f)` (`Object.cpp:1163-1164`). The emerge-completion path in
`LegoObject_Callback_Update` then does this (`Object.cpp:3214-3227`):

```cpp
	if (liveObj->flags1 & LIVEOBJ1_EXPANDING) {
		if (liveObj->animTime > 0.0f) {
			liveObj->flags1 &= ~LIVEOBJ1_EXPANDING;

			if (LegoObject_IsRockMonsterCanGather(liveObj)) {
				const Point2I target = {
					(sint32)liveObj->targetBlockPos.x,
					(sint32)liveObj->targetBlockPos.y,
				};
				Message_PostEvent(Message_GenerateRockMonsterComplete, liveObj, 0, nullptr);
				Level_Block_SetBusy(&target, false);
			}
		}
		goto objectupdate_end;
	}
```

If `LegoObject_IsRockMonsterCanGather` returns true and `targetBlockPos` is still
`(-1,-1)`, `Level_Block_SetBusy` receives block `(-1,-1)`. `Level_Block_SetBusy` is an exe
macro (`Game.h:1781`), so I cannot read its body, but every in-tree block accessor resolves
through `blockValue(l,bx,by) == l->blocks[l->width*by + bx]` (`Game.h:781-784`), which for
`(-1,-1)` is index `-(width+1)` — a **write** through a negative index into a `0x48`-byte
struct array. That is heap corruption, not a graceful failure.

`LegoObject_IsRockMonsterCanGather` is undecompiled (`Object.cpp:5186`), so
**UNDETERMINED:** I cannot prove whether it returns true for slugs. The existing slug path
has presumably survived because it does not, but that is inference, not verification.

**Consequence for the director:** the director's spawn function **must** set
`targetBlockPos` to a real, in-bounds block before the object can reach its first update,
and should mirror what the vanilla emerge path presumably does by marking that block busy:

```cpp
	obj->targetBlockPos.x = (real32)bx;
	obj->targetBlockPos.y = (real32)by;
	const Point2I blockPos = { (sint32)bx, (sint32)by };
	Level_Block_SetBusy(&blockPos, true);   // cleared again by Object.cpp:3221 on emerge completion
```

The busy flag is `BLOCK1_BUSY_FLOOR` / `BLOCK1_BUSY_WALL` (`Game.h:207`, `Game.h:218`) and
is the same mechanism drilling and reinforcing use (`Object.cpp:2956-2963`,
`Object.cpp:3000-3006`). Marking the spawn block busy also has a nice second-order effect:
it stops a Rock Raider being ordered to drill the exact block a monster is climbing out of.

I recommend the same two lines be added to `LegoObject_TryGenerateSlugAtBlock` as a
separate, small, independently-bisectable fix. That is out of scope for the director and
should be its own commit.

### 1.5 Secondary observation — the slug-hole gate is unconditional

`Object.cpp:1608-1616` means `LegoObject_TryGenerateSlugAtBlock` **cannot** be reused for
wave spawns at arbitrary blocks. Do not be tempted to relax the gate in place: that
function is on the hooked `LegoObject_TryGenerateSlug` path (`interop.cpp:3609`) and the
gate is what keeps vanilla slug behaviour correct. The director needs a sibling.

---

## 2. Finding a legal spawn block

### 2.1 What a block is

`Lego_Block` is 0x48 bytes, `#pragma pack(1)`, and is `assert_sizeof`-pinned
(`Game.h:340-369`). Fields the director cares about:

```cpp
	/*02,1*/	uint8 terrain;      // as Lego_SurfaceType
	/*08,4*/	BlockFlags1 flags1;
	/*0c,4*/	BlockFlags2 flags2;
	/*10,4*/	Construction_Zone* construct;
```

The grid lives at `Lego_Level::blocks` (`Game.h:436`), sized `width` × `height`
(`Game.h:395-396`). Access macros, `Game.h:781-787`:

```cpp
#define blockIndex(l, bx, by)     (((l)->width*(by))+(bx))
#define blockValue(l, bx, by)     ((l)->blocks[blockIndex((l),(bx),(by))])
#define blockInBounds(l, bx, by)  ((static_cast<sint32>((bx)) >= 0 && static_cast<uint32>((bx)) < (l)->width) && \
                                   (static_cast<sint32>((by)) >= 0 && static_cast<uint32>((by)) < (l)->height))
```

**`blockValue` does not bounds-check.** `blockInBounds` must be called first, every time.
`Lego_IsBuildableSurface`-style code in-tree does exactly this (`Game.cpp:3747-3750`).

### 2.2 Wall vs floor vs undiscovered — the flags, cited

`BlockFlags1`, `Game.h:182-223` (excerpt of what matters):

| Flag | Value | Meaning for us |
| --- | --- | --- |
| `BLOCK1_RUBBLE_LOW/MEDIUM/FULL` | `0x1/0x2/0x3` | low two bits are a rubble depth counter |
| `BLOCK1_SURVEYED` | `0x4` | scanner has surveyed it |
| `BLOCK1_FLOOR` | `0x8` | **walkable ground** |
| `BLOCK1_WALL` | `0x10` | **exposed wall face** |
| `BLOCK1_REINFORCED` | `0x20` | reinforced wall |
| `BLOCK1_BUILDINGSOLID` | `0x400` | building footprint |
| `BLOCK1_HIDDEN` | `0x20000` | **undiscovered** (inside an unopened cavern) |
| `BLOCK1_BUSY_FLOOR` | `0x80000` | something is working here |
| `BLOCK1_FOUNDATION` | `0x100000` | building foundation |
| `BLOCK1_CLEARED` | `0x200000` | *not* rubble |
| `BLOCK1_BUSY_WALL` | `0x8000000` | a wall being worked on |
| `BLOCK1_PATH` / `BLOCK1_LAYEDPATH` | `0x20000000/0x40000000` | power path |
| `BLOCK1_BUILDINGPATH` | `0x8000` | building's path tile |

`BlockFlags2`, `Game.h:227-246`:

| Flag | Value | Meaning for us |
| --- | --- | --- |
| `BLOCK2_TOOLSTORE` | `0x4` | **the Tool Store's own tiles** |
| `BLOCK2_SLUGHOLE_EXPOSED` | `0x20` | exposed slug hole |
| `BLOCK2_EMERGE_POINT` | `0x40` | a map emerge point |
| `BLOCK2_EMERGE_TRIGGER` | `0x80` | a map emerge trigger tile |
| `BLOCK2_POWERED` | `0x100` | powered |
| `BLOCK2_SLUGHOLE_HIDDEN` | `0x200` | hidden slug hole |

**How the game itself distinguishes the three states** — this is the decisive citation,
from the block tooltip at `Game.cpp:1911-1925`:

```cpp
	if (!(block->flags1 & BLOCK1_FLOOR)) {
		// Wall types:
		if (!(block->flags1 & BLOCK1_WALL) && (block->flags1 & BLOCK1_HIDDEN)) {
			surfType = Lego_SurfaceType_Undiscovered; // Undiscovered cavern (not necessarily a wall type)
		}
		else if (block->flags1 & BLOCK1_REINFORCED) {
			surfType = Lego_SurfaceType_Reinforcement;
		}
		else {
			surfType = (Lego_SurfaceType)block->terrain; // All other wall types
		}
	}
```

So: **`BLOCK1_WALL` is set on solid blocks whose face has been exposed to an open cavern,
and `BLOCK1_HIDDEN` marks blocks the player has not yet uncovered.** That is precisely the
"which walls are adjacent to open cavern" query the brief asks for, and it needs no
adjacency scan of its own — the engine maintains it. The radar map uses the same flag to
hide unexplored terrain (`RadarMap.cpp:868-872`).

**UNDETERMINED:** the *setter* of `BLOCK1_WALL` is `Level_BlockUpdateSurface`
(`0x0042f620`, `Game.h:1545`), which is still an exe macro. I read its call sites, not its
body. I am inferring "exposed" from the tooltip semantics above and from the fact that the
engine re-runs it on a block and its neighbours whenever terrain changes
(`Game.cpp:2535`, `Game.cpp:3823`, `Game.cpp:3840-3849`, `OpenLRR.cpp:927-929`). The
inference is strong but it is an inference.

### 2.3 Available block query functions

Almost all of the `Level_Block_*` family is still exe macros. That is fine — they are
callable — but each needs `using namespace LegoRR;` in scope, and the ones taking
`const Point2I*` need `Point2I` (global namespace, `geometry.h:88`).

| Function | Status | Line |
| --- | --- | --- |
| `Level_Block_IsGround(bx, by)` | exe macro `0x00432a80` | `Game.h:1749` |
| `Level_Block_IsWall(bx, by)` | exe macro `0x00432b00` | `Game.h:1757` |
| `Level_Block_IsSeamWall(bx, by)` | exe macro `0x00432ac0` | `Game.h:1753` |
| `Level_Block_IsCorner(bx, by)` | exe macro `0x00432d90` | `Game.h:1785` |
| `Level_Block_IsImmovable(blockPos)` | exe macro `0x00432df0` | `Game.h:1793` |
| `Level_Block_IsLava(blockPos)` | exe macro `0x00432e30` | `Game.h:1797` |
| `Level_Block_IsSurveyed(bx, by)` | exe macro `0x00432ec0` | `Game.h:1805` |
| `Level_Block_IsBusy(blockPos)` | exe macro `0x00432d00` | `Game.h:1777` |
| `Level_Block_SetBusy(blockPos, state)` | exe macro `0x00432d30` | `Game.h:1781` |
| `Level_Block_IsSolidBuilding(bx, by, includeToolStore)` | exe macro `0x004329d0` | `Game.h:1741` |
| `Level_Block_GetRubbleLayers(blockPos)` | exe macro `0x00432b80` | `Game.h:1765` |
| `Level_Block_IsPath(blockPos)` | exe macro `0x00432fc0` | `Game.h:1825` |
| `Level_Block_IsFoundationOrBusyFloor(blockPos)` | exe macro `0x00433010` | `Game.h:1829` |
| `Level_Block_IsInitiallyExposed(blockPos)` | **ours** | `Game.h:1790` |
| `Level_Block_IsRockFallFX(bx, by)` | **ours** | `Game.h:1746` |
| `Level_Block_IsPowered(blockPos)` | **ours**, impl `Game.cpp:3826-3830` | `Game.h:1654` |
| `Construction_Zone_ExistsAtBlock(blockPos)` | **ours** | used at `Game.cpp:3789` |
| `ElectricFence_HasFence(bx, by)` | **ours** | used at `Game.cpp:3792` |

**Recommendation: read the flags directly, do not go through the exe macros where a flag
test will do.** Reasons: (a) we can see the flag test in our own source and reason about
it; (b) it avoids the macro/namespace trap at every call site; (c) `Game.cpp:3749` and
`Game.cpp:3767` show the codebase already documents the flag equivalence inline
(`if (!Level_Block_IsGround(bx, by)) //if (!(block->flags1 & BLOCK1_FLOOR))`). The one
exception is `Level_Block_SetBusy`, which is a *setter* and must be called.

### 2.4 Adjacency and the four-direction convention

The tree has a single consistent idiom for orthogonal neighbours, e.g. `Game.cpp:3638-3644`
and `Game.cpp:3736-3742`:

```cpp
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};
```

`DIRECTION__COUNT == 4` (`geometry.h:17`). `Game.cpp:3646-3656` shows the exact
count-adjacent-floors pattern the director will reuse.

### 2.5 Reachability

Two options.

**(a) The engine's own pathfinder.** `LegoObject_Route_BuildListToTarget`
(`Object.h:1200-1201`, impl `Object.cpp` — a one-line forward to
`LegoObject_Route_BuildList`) is **ours**. But it:
- requires a live `LegoObject*` (it calls `Lego_GetCrossTerrainType(liveObj, …)` to decide
  passability per-unit, `Object.cpp` inside `Route_BuildList`);
- allocates the output lists with `Gods98::Mem_Alloc` which the caller must free — and
  `Object.cpp:4744-4750` documents a past double-free bug (#66) from exactly this;
- has a hard frontier cap of `Point2I blockList[2][250]` per wave with no overflow check,
  already flagged in `docs/OVERHAUL-PLAN.md:206`;
- reallocates and zeroes a `width*height` float grid per call.

That is far too much machinery, and too much shared mutable state
(`objectGlobs.routeBuildListScores`), to run speculatively over dozens of candidate blocks
every wave.

**(b) Our own flood fill.** A breadth-first fill over `BLOCK1_FLOOR` blocks starting from
every live Rock Raider, into a DLL-side `std::vector<uint8>` sized `width*height`, rebuilt
once per wave evaluation (not per candidate). Cost is O(width×height) with no allocation
after the first level tick, no shared engine state touched, and it answers exactly the
question we need: *"is there a walkable route from a Rock Raider to the floor tile in front
of this wall?"*

**Decision: (b).** Cited precedent for a DLL-side `std::vector` replacing fixed engine
storage: the PowerGrid vectors at `Game.cpp:168-170`, used e.g. at `Game.cpp:3820-3824`.

Note this fill is deliberately *raider-centric*: it does not model what a monster can
cross. That is the correct bias for fairness — a wave that emerges somewhere the player
cannot possibly reach is unfair in the other direction.

### 2.6 Where should a monster actually stand?

Vanilla emerge points are wall blocks flagged `BLOCK2_EMERGE_POINT` (`Game.h:236`) and
are consumed by the exe's `LegoObject_TryGenerateRMonster` (`Object.h:974`), which is
undecompiled. `Level_HandleEmergeTriggers` (**ours**, `Game.cpp:3040-3100`) passes
`emergePt->blockPos.x/.y` straight through.

**UNDETERMINED:** whether the exe positions the creature *on* the wall block and lets the
`Enter`/`Emerge` activities carry it out, or on an adjacent floor block. I could not
determine this without the exe body or map data, and there is real evidence for both:
`LIVEOBJ1_ENTERING_WALLHOLE` handling at `Object.cpp:3015-3030` explicitly tests
`Level_Block_IsWall(target)` for a monster's *target*, which implies monsters do interact
with wall blocks as positions.

**Decision, given that we cannot play-test:** spawn on the **floor block adjacent to the
chosen wall**, with the heading pointing away from the wall, and offset the world position
*toward* the wall using the slug's `12.75f` idiom, so the creature visually starts at the
wall face and walks out. This guarantees the creature is standing on terrain the engine
already considers walkable, which is the failure mode we cannot afford. The alternative
(spawn on the wall block itself, more faithful to vanilla) should be exposed as a config
switch, defaulting **off**, so a user with a running game can test it and report.

---

## 3. Counting living monsters (the alive budget)

### 3.1 The iterator

`LegoObject_ListSet objectListSet` is a real C++ collection defined at `Object.cpp:62` and
exported at `Object.h:545`:

```cpp
extern LegoObject_ListSet objectListSet;
```

The class (`Object.h:504-524`) derives from `ListSet::WrapperCollection<LegoObject_Globs>`
and offers `EnumerateAlive()` and `EnumerateSkipUpgradeParts()`. The legacy
callback-based `LegoObject_RunThroughLists` / `…SkipUpgradeParts` still exist and are
**ours** (`Object.cpp:621-642`), but every modern call site has been converted to
range-for, and the callback forms are literally implemented in terms of the enumerators.

It is used from at least eight translation units outside `Object.cpp` —
`Game.cpp:1134`, `GameState.cpp:2351`, `Debug.cpp:391`, `Bubbles.cpp:365`,
`RadarMap.cpp:463`, `Messages.cpp:518`, `NERPsFunctions.cpp:377`, `Objective.cpp:1119` —
so there is no linkage or visibility problem in a new file.

`.Count()` exists on the enumerable and is already used for a debug readout at
`GameState.cpp:1791`:

```cpp
Gods98::Font_PrintF(legoGlobs.fontToolTip, dbgX, dbgY+r*(row++), "Objects %i", (sint32)objectListSet.EnumerateAlive().Count());
```

### 3.2 The exact idiom for the director

```cpp
// in a .cpp inside namespace DeepCore
static sint32 _CountLiveMonsters(void)
{
	using namespace LegoRR; // required: LegoObject_IsActive's neighbours + exe macros below

	sint32 count = 0;
	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (obj->type != LegoObject_RockMonster)
			continue;
		// Count anything that is alive at all, including creatures still mid-emerge:
		// LIVEOBJ1_EXPANDING makes LegoObject_IsActive() return false (Object.cpp:1283),
		// so IsActive() alone would let the director double-spawn during the emerge window.
		if (obj->health < 0.0f)
			continue;
		if (obj->flags1 & LIVEOBJ1_CRUMBLING)   // dying (Object.h:123)
			continue;
		count++;
	}
	return count;
}
```

Three deliberate choices:

1. **`EnumerateSkipUpgradeParts`, not `EnumerateAlive`.** Upgrade parts are never monsters,
   and this is the variant the rest of the codebase uses.
2. **Do not use `LegoObject_IsActive`.** It returns false during `LIVEOBJ1_EXPANDING`
   (`Object.cpp:1283-1284`), i.e. for the entire emerge animation. A budget built on it
   would let the director fire a second wave while the first is still climbing out of the
   wall. This is the sort of thing that turns "brutal but fair" into "unplayable".
3. **Count map-emerged monsters too.** The budget is a ceiling on *total* creature pressure,
   not on director-spawned creatures. That is what keeps the director from stacking on top
   of a level that already has aggressive emerge triggers.

The director should additionally track *its own* spawns separately (a DLL-side counter) so
diagnostics can say "6 alive, 2 of them mine".

### 3.3 Also needed: are there any Rock Raiders left?

Same loop, `obj->type == LegoObject_MiniFigure`. `GameState.cpp:1445-1455` already uses a
"no pilots present" check to decide the level is lost, so a director that keeps spawning
into an empty base is both pointless and cruel. Gate on `raiderCount > 0`.

---

## 4. Where to hook the per-frame update

### 4.1 The slot

`OpenLRR.cpp:958-993`, as it stands:

```cpp
bool32 __cdecl OpenLRR_Initialise_Wrapper(void)
{
	// pre-Initialise code here...

	bool32 result = openlrrGlobs.legoState.Initialise();

	// NOTE: This call does not finish until the MainMenu has been left and a game started!!!
	// post-Initialise code...

	return result;
}

bool32 __cdecl OpenLRR_MainLoop_Wrapper(real32 elapsed)
{
	// pre-MainLoop code here...

	bool32 result = openlrrGlobs.legoState.MainLoop(elapsed);

	// NOTE: Similar to Initialise, the main loop does not run when in the FrontEnd!!!
	// post-MainLoop code here...


	if (OpenLRR_IsWobblyWorld()) {
		OpenLRR_UpdateWobblyWorld();
	}

	return result;
}
```

These are installed as the engine `Main_State` at `OpenLRR.cpp:1055-1059`, and the engine
drives `MainLoop` from `Main.cpp:882-908`.

### 4.2 Does it run in the front end? — confirmed, with a caveat

The in-source comment ("the main loop does not run when in the FrontEnd", `OpenLRR.cpp:977`)
is **correct but incomplete**, and the caveat matters.

- `Lego_Initialise` blocks on `Front_RunScreenMenuType(Menu_Screen_Title)` before the state
  loop starts (`GameState.cpp:860-879`). So no `MainLoop` ticks happen before a level.
- **However**, `Lego_EndLevel()` is called from *inside* `Lego_MainLoop`
  (`GameState.cpp:2094` and `GameState.cpp:2283`), and `Lego_EndLevel` itself calls
  `Reward_Show()`, `Level_Free()` and `Front_RunScreenMenuType(...)` inline
  (`Game.cpp:4198-4229`). Those block. When they return, `Lego_MainLoop` returns, and
  **post-MainLoop code then runs on a tick during which the level was freed and a new one
  loaded.**

Therefore the guard is not optional:

```cpp
	bool32 result = openlrrGlobs.legoState.MainLoop(elapsed);

	// post-MainLoop code here...

	/// DEEPCORE: Wave director. No-op unless the feature is enabled, and never runs
	/// outside an in-progress mission -- note that Lego_EndLevel() runs the FrontEnd
	/// menus *inside* Lego_MainLoop (GameState.cpp:2094, Game.cpp:4210-4229), so this
	/// callback can be reached on a tick where the level was swapped out from under us.
	DeepCore::Waves::Update(elapsed);
```

with `Update()` immediately returning unless `LegoRR::Lego_IsInLevel()` (`Game.h:812`):

```cpp
inline bool Lego_IsInLevel() { return Lego_IsInit() && legoGlobs.currLevel != nullptr && !(legoGlobs.flags1 & (GAME1_LEVELSTART|GAME1_LEVELENDING)); }
```

That single call covers all four hazards: engine not initialised, no level, level's first
tick not yet processed, level ending.

### 4.3 The elapsed-time unit

`elapsed` is in **"standard units", where 25.0 == one second**:

- `#define STANDARD_FRAMERATE 25.0f` (`common.h:109`).
- The engine's comment at `Main.cpp:874-875`: *"Use the MultiMedia timer to give a
  'realtime passed' value per frame to the main loop (in 25th's of a second)"*.
- Conversion: `delta = (currTime - lastTime) / (1000.0f / STANDARD_FRAMERATE);`
  (`Main.cpp:436`).
- **Hard-capped at 3.0 per tick** (`Main.cpp:440-444`, and `realMaxDeltaMS = 3.0 * (1000.0
  / STANDARD_FRAMERATE)` at `Main.cpp:395`). So a long stall — an alt-tab, a level load, a
  front-end excursion — cannot deliver a giant `elapsed` and dump six waves at once. This
  is worth knowing: it means the director does **not** need its own delta clamp.

Corroborating data-side evidence for the 25.0 factor: `Lego_Level::EmergeTimeOut` defaults
to `1500.0` (`Game.h:449`) — 60 seconds — and `SlugTime` is documented as
`(default: 60.0, mult: 25.0)` (`Game.h:450`).

### 4.4 Which elapsed, though?

`OpenLRR_MainLoop_Wrapper` receives the **raw real** elapsed. `Lego_MainLoop` derives three
values from it (`GameState.cpp:915-936`):

```cpp
	const bool32 firstTick        = (legoGlobs.flags1 & GAME1_LEVELSTART);
	const real32 elapsedReal      = (!firstTick ? elapsed : 1.0f);
	const bool32 timeStopped      = (legoGlobs.flags1 & GAME1_FREEZEINTERFACE);
	const real32 elapsedInterface = (!timeStopped ? elapsedReal : 0.0f);
	const real32 elapsedWorld     = (!fpForceGameSpeed ? (elapsedInterface * Lego_GetGameSpeed()) : (elapsedInterface * 1.0f));
```

The director must not advance on raw real time, or waves keep coming while the game is
paused in a menu. It should reconstruct the world-time value itself:

```cpp
	if (LegoRR::legoGlobs.flags1 & LegoRR::GAME1_FREEZEINTERFACE)
		return;                                  // paused: no time passes
	const real32 elapsedWorld = elapsed * LegoRR::Lego_GetGameSpeed();  // Game.h:1068
```

Note the standing in-source complaint one line above the emerge update — *"OH MY GOD,
MONSTER EMERGE TRIGGERS ARE BASED ON INTERFACE TIME PASSED, AND NOT WORLD TIME!!"*
(`GameState.cpp:955-956`). The director should use **world** time and be honest about the
fact that it therefore does not tick identically to the level's own emerge timeouts. That
is a deliberate divergence, and a better one.

### 4.5 Level transitions

The director's state must be zeroed per level. Two clean, DLL-side, no-new-hook options,
both **ours**:

- `Level_Free()` (`Game.cpp:3302`) — already the level teardown that resets camera shake
  (`Game.cpp:3341`), power-grid vectors (`Game.cpp:3334-3336`), disposable stats
  (`Game.cpp:3345`) and water (`Game.cpp:3350`). Add `DeepCore::Waves::Reset();` alongside.
- `Lego_LoadLevel2()` (`Game.cpp:2951`) — the custom load wrapper; reset on success.

**Do both.** Reset on teardown *and* on successful load, so a load path that bypasses
`Level_Free` (the very first level of a session) is still covered.

---

## 5. The director itself

### 5.1 Shape

A small state machine, evaluated once per world tick.

```
IDLE ──(graceElapsed && cooldownElapsed && aliveBudget has room && candidate exists)──▶ TELEGRAPH
TELEGRAPH ──(warnSeconds elapsed)──▶ SPAWN
SPAWN ──(N creatures placed, or placement failed)──▶ COOLDOWN
COOLDOWN ──(cooldownSeconds elapsed)──▶ IDLE
```

Keeping TELEGRAPH as an explicit state (rather than "warn and spawn in the same frame") is
the whole difference between brutal-and-fair and brutal-and-cheap. The chosen blocks are
committed at the moment of the warning, and re-validated (only) for "did a unit walk onto
it" at the moment of the spawn.

### 5.2 The threat curve

Everything scales off a single normalised threat value:

```cpp
// t = seconds of world time since the level started
static real32 _Threat(real32 t, real32 rampSeconds)
{
	if (rampSeconds <= 0.0f) return 1.0f;
	const real32 x = t / rampSeconds;
	return (x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x));
}
```

Linear, not exponential, and clamped. Deliberate: a linear ramp is legible to the player
("it keeps getting worse at a steady rate"), it is trivially reproducible in a bug report,
and it cannot run away. Default `rampSeconds = 900.0` (15 minutes), at which point the
mission is at full pressure.

From `threat` (call it `k ∈ [0,1]`):

| Quantity | Formula | At k=0 | At k=1 (defaults) |
| --- | --- | --- | --- |
| alive budget | `1 + round(k * (WaveMaxAlive - 1))` | 1 | 6 |
| wave size | `1 + (sint32)(k * (WaveMaxPerWave - 1))` | 1 | 3 |
| interval | `WaveIntervalSeconds * (1.0f - 0.5f*k)` | 90 s | 45 s |
| cooldown | `WaveCooldownSeconds * (1.0f - 0.5f*k)` | 60 s | 30 s |

The interval and cooldown are separate on purpose. *Interval* is the metronome between
evaluations. *Cooldown* is the enforced quiet period after a wave actually lands, so the
player always gets a breathing space proportional to what just happened. Without it, a
wave whose members die instantly would be followed immediately by another.

`WaveCooldownSeconds` should default to **the level's own feel**: `level->EmergeTimeOut /
STANDARD_FRAMERATE` (`Game.h:449`, default 1500.0 → 60 s), which is exactly the timeout the
map's own triggers use before they can re-fire (`Game.cpp:3081`). A map that was authored
to be slow stays slow.

### 5.3 Species selection

Reuse the existing pooling machinery rather than duplicating it. `DeepCore.cpp:77-124`
already resolves names → IDs once, bounds-checks against **both**
`legoGlobs.rockMonsterCount` and the hard `LegoObject_ID_Count == 15` ceiling
(`DeepCore.cpp:107-115`, `GameCommon.h:143`), warns once per bad name, and caches failures.

Add a sibling to `PickEmergeSpecies` (`DeepCore.cpp:127-146`):

```cpp
/// Choose the RockMonster type ID for wave `waveIndex`. Returns `fallbackId` unchanged
/// (i.e. the level's own EmergeCreature) whenever the pool is empty or unresolved, so the
/// caller can use the result unconditionally -- same contract as PickEmergeSpecies.
sint32 PickWaveSpecies(sint32 fallbackId, uint32 waveIndex);
```

with the same deterministic `pool[waveIndex % pool.size()]` selection. Determinism is the
point, and the reasoning is already written down at `DeepCore.cpp:141-145`: *"players learn
which tunnel breeds what, which is far more interesting than noise, and it keeps behaviour
reproducible when diagnosing a report."* A wave director gets the same treatment — wave 1
is always the same species, wave 2 always the next. The escalation the player feels comes
from **count and cadence**, not from a slot machine.

If `CreatureVariants` is also on, `DeepCore::ApplyCreatureVariant` already fires inside
`LegoObject_Create`'s RockMonster branch (`Object.cpp:947-961`), so director-spawned
creatures pick up scale/tint for free. No extra work.

### 5.4 Telegraphing — what is actually callable

Four channels, all verified present in the tree.

**(1) `Info_Send` — the panel alert.** Exe macro at `InfoMessages.h:267`:

```cpp
Info_Send(Info_Type infoType, OPTIONAL const char* opt_text, OPTIONAL LegoObject* liveObj, OPTIONAL const Point2I* blockPos);
```

`InfoMessageInstance` stores the `blockPos` (`InfoMessages.h:87-94`) and the engine has
`Info_GotoFirst` (`InfoMessages.h:271`) to jump the camera to a message, so **sending the
warning with the *block position* and a null object gives the player a locatable alert
before anything exists there.** That is the single best telegraph available and it costs
one call.

Relevant types (`GameCommon.h:496-541`): `Info_RockMonster = 2`,
`Info_LavaRockMonster = 3`, `Info_IceRockMonster = 4`, `Info_GenericMonster = 39`,
`Info_UnderAttack = 5`, `Info_CaveIn = 7`, `Info_SlugEmerge = 31`.

Caveat: message text, icon and sound for each `Info_Type` are configured from the user's
`Lego.cfg` by `Info_Initialise` (`GameState.cpp:218`, exe body). If a type is not
configured, the message may be silent. **Check first** — `Info_HasTypeText(infoType)` is
available (`InfoMessages.h:255`). If it returns false, warn once and fall back to sound +
shake rather than telegraphing nothing.

**(2) Sound.** `SFX_Random_PlaySoundNormal(SFX_ID, bool32 loop)` is **ours**
(`SFX.cpp:311`). Useful IDs from `GameCommon.h:1369-1421`: `SFX_RockMonster = 9`,
`SFX_RockMonster2 = 10`, `SFX_Siren = 25`, `SFX_RockBreak = 4`, `SFX_FallIn = 40`. For a
positional cue, `SFX_Random_PlaySound3DOnContainer(nullptr, sfxID, false, false, &wPos)` is
also ours (`SFX.cpp:381`) and is used exactly that way at `Game.cpp:3719`.

**(3) Screen shake.** `Camera_Shake(LegoCamera*, real32 intensity, real32 duration)` is
**ours** and is four lines (`Camera.cpp:192-197`); it just seeds
`shakeIntensity/shakeDuration/shakeTimer`, consumed by `Camera_Update`
(`Camera.cpp:464-475`). The tree's own debug keybind uses
`Camera_Shake(legoGlobs.cameraMain, 5.0f, 25.0f)` (`GameState.cpp:2465`) — intensity 5,
duration 25 standard units = 1 second — which is a good calibration anchor. For a
telegraph, something gentler and longer (e.g. `2.0f, 40.0f`) reads as a rumble rather than
an impact. `Level_Free` resets it with `Camera_Shake(legoGlobs.cameraMain, 0.0f, 0.0f)`
(`Game.cpp:3341`), so no cleanup burden.

**(4) Dust at the wall.** `Smoke_CreateSmokeArea(...)` is **ours** (`Smoke.cpp:68`, hooked
at `interop.cpp:4066`), signature at `Smoke.h:145-147`, and puts a coloured, animated dust
plume on a specific block with an optional `SFX_ID`. This is the most *diegetic* warning
available: dust falling from the wall the monster is about to come through. It is also the
heaviest: it returns a `Smoke*` that must be released via `Smoke_Remove`
(`Smoke.h:153`), and `Smoke_CreateSmokeArea` calls `Error_Fatal` on an invalid
`smokeType` (`Smoke.cpp:112`) — valid values are 0, 1, 2 only (`Smoke.h:140`). Gate it
behind its own config flag, defaulting off, and keep the returned pointer in the director's
own state so it can always be cleaned up.

**Explicitly rejected:**

- **`Lego_PTL_RockFall`** (`Game.cpp:3627`) looks like a perfect "the wall is cracking"
  telegraph and is **not**. It sets `BLOCK1_ROCKFALLFX`, marks the block busy, and posts a
  `Message_RockFallComplete` (`Game.cpp:3679-3712`) — it is a real rockfall that changes
  terrain, not a cosmetic warning.
- **`Lego_SetCallToArmsOn`** (`Game.h:1990`, exe macro) drives the red "action stations"
  ambient light (`GameState.cpp:964-975`) but also changes unit behaviour — raiders equip
  beams (`Object.cpp:2201-2208`). Too intrusive for an automatic telegraph. Offer it as an
  opt-in flag only.
- **`Text_SetNERPsMessage`** (**ours**, `TextMessages.cpp:177-188`) writes the bottom text
  panel, but note it does `textGlobs.currText = text;` — it **stores the pointer, it does
  not copy the string**. Any caller must guarantee the buffer outlives the message. If used
  at all, use a `static char` buffer in the director. Listed for completeness; not in the
  recommended default set.

**Recommended default telegraph** (all three, ~1.5 s apart is unnecessary — fire together):
`Info_Send(<monster type>, nullptr, nullptr, &spawnBlockPos)` +
`SFX_Random_PlaySoundNormal(SFX_RockMonster, false)` +
`Camera_Shake(legoGlobs.cameraMain, 2.0f, 40.0f)`, followed by the actual spawn
`WaveWarnSeconds` (default **6.0 s**) later.

### 5.5 Fairness rules

These are the rules that make the difference between a director and a punishment. Each is
stated as a hard predicate on the candidate *wall* block `W` and its chosen adjacent
*floor* block `F`.

| # | Rule | Test | Why |
| --- | --- | --- | --- |
| F1 | In bounds | `blockInBounds(level, bx, by)` for both, and for all four neighbours examined (`Game.h:786`) | `blockValue` does not bounds-check |
| F2 | `W` is an exposed wall | `(W.flags1 & BLOCK1_WALL) && !(W.flags1 & BLOCK1_FLOOR)` | `Game.h:191-192`; matches the tooltip classification `Game.cpp:1912-1916` |
| F3 | `W` is not undiscovered | `!(W.flags1 & BLOCK1_HIDDEN)` | never spawn out of terrain the player has not opened (`Game.h:205`) |
| F4 | `W` is not immovable / seam / reinforced | `W.terrain != Lego_SurfaceType_Immovable`, `!(W.flags1 & BLOCK1_REINFORCED)` | a monster bursting through *reinforced* rock reads as a bug, not a threat |
| F5 | `F` is walkable floor, cleared | `(F.flags1 & BLOCK1_FLOOR) && (F.flags1 & BLOCK1_CLEARED)` | `Game.h:191`, `Game.h:209`; the "not rubble" flag |
| F6 | `F` is not lava/water | `F.terrain != Lego_SurfaceType_Lava && F.terrain != Lego_SurfaceType_Lake` | mirrors the buildable check at `Game.cpp:3765-3773` |
| F7 | `F` is not busy | `!(F.flags1 & (BLOCK1_BUSY_FLOOR\|BLOCK1_BUSY_WALL))` | don't drop a monster on top of a raider mid-job (`Game.h:207`, `218`) |
| F8 | **Never inside the base** | `!(F.flags1 & (BLOCK1_BUILDINGSOLID\|BLOCK1_BUILDINGPATH\|BLOCK1_FOUNDATION)) && !(F.flags2 & BLOCK2_TOOLSTORE)` | `Game.h:198`, `203`, `208`, `232` |
| F9 | No construction zone | `!Construction_Zone_ExistsAtBlock(&fPos)` | matches `Game.cpp:3789` |
| F10 | **Never on top of a unit** | no live `LegoObject` (`MiniFigure`/`Vehicle`/`Building`/`RockMonster`) reports `LegoObject_GetBlockPos == F`, nor on the eight neighbours of `F` | `Object.h:1256`; a spawn inside a raider is the single most unfair thing the director can do |
| F11 | **Minimum distance from Tool Store** | Chebyshev block distance from `F` to every building with `STATS1_TOOLSTORE` ≥ `WaveMinToolStoreBlocks` (default **6**) | `Stats.h:63`; `StatsObject_GetStatsFlags1` is ours (`Stats.h:383`) |
| F12 | Minimum distance from *any* building | ≥ `WaveMinBuildingBlocks` (default **3**) | prevents a monster materialising inside the power station's blind spot |
| F13 | Reachable | `F` is in the raider flood-fill set (§2.5b) | a wave the player cannot reach is not a threat, it's a timer |
| F14 | Not a map emerge point | `!(W.flags2 & BLOCK2_EMERGE_POINT)` | leave the map's own triggers alone; the director is *additive*, not a replacement |
| F15 | Not on a slug hole | `!(F.flags2 & (BLOCK2_SLUGHOLE_EXPOSED\|BLOCK2_SLUGHOLE_HIDDEN))` | `Game.h:235`, `239`; mirrors `Game.cpp:3771-3772` |
| F16 | Spacing within a wave | ≥ 2 blocks between two spawns in the same wave | a three-monster wave that all appear on the same tile is a bug that looks like a design |
| F17 | Grace period | no waves before `WaveGraceSeconds` (default **120 s**) of world time | the player must be allowed to build a Tool Store |
| F18 | Raiders must exist | at least one live `LegoObject_MiniFigure` | see §3.3 |
| F19 | Not while ending | `Lego_IsInLevel()` false ⇒ no-op | `Game.h:812` |

If no candidate survives F1–F16, the director **does not spawn**, logs at debug level, and
re-arms after the interval. It never relaxes a rule to force a spawn.

### 5.6 Interaction with the level's own emerge triggers

The director never touches `level->emergeTriggers` (`Game.h:393`) or their `timeout` field
(`Game.h:332`), and F14 keeps it off their blocks. `Level_HandleEmergeTriggers`
(`Game.cpp:3040-3100`) continues to work exactly as it does today, including the
`MultiSpeciesEmerge` hook at `Game.cpp:3062`. The two systems compose through one shared
quantity only: the alive budget (§3.2), which counts map-emerged creatures too.

---

## 6. `DeepCore.cfg` gates

Three keys already exist in the tree — `WaveDirector`, `WaveIntervalSeconds`,
`WaveMaxAlive` — declared at `DeepCore.hpp:88-97`, parsed at `DeepCore.cpp:292` and
`DeepCore.cpp:322-342`, and documented in `data/Settings/DeepCore.cfg` under
"MONSTER DENSITY". The rest are new. **Every boolean defaults to `FALSE`, and the master
gate `WaveDirector FALSE` makes all of them inert.**

| Key | Type | Default | Meaning |
| --- | --- | --- | --- |
| `WaveDirector` | bool | **FALSE** | master gate. Off ⇒ exact vanilla. |
| `WaveIntervalSeconds` | real | 90.0 | metronome between evaluations at threat 0; halves at threat 1 |
| `WaveMaxAlive` | int | 6 | ceiling on *total* live Rock Monsters, including map-emerged ones |
| `WaveMaxPerWave` | int | 3 | creatures placed in a single wave at threat 1 |
| `WaveRampSeconds` | real | 900.0 | world seconds to reach full threat |
| `WaveGraceSeconds` | real | 120.0 | no waves before this (F17) |
| `WaveCooldownSeconds` | real | 0.0 | quiet period after a wave. **0 means "use the level's own `EmergeTimeOut`"** (`Game.h:449`), which is the recommended value |
| `WaveWarnSeconds` | real | 6.0 | telegraph lead time |
| `WaveMinToolStoreBlocks` | int | 6 | F11 |
| `WaveMinBuildingBlocks` | int | 3 | F12 |
| `WaveTelegraphInfo` | bool | **FALSE** | send the `Info_*` panel alert |
| `WaveTelegraphSound` | bool | **FALSE** | play `SFX_RockMonster` |
| `WaveTelegraphShake` | bool | **FALSE** | `Camera_Shake` rumble |
| `WaveTelegraphSmoke` | bool | **FALSE** | `Smoke_CreateSmokeArea` dust at the wall (heaviest; see §5.4) |
| `WaveTelegraphCallToArms` | bool | **FALSE** | opt-in `Lego_SetCallToArmsOn` — changes unit behaviour |
| `WaveSpawnOnWallBlock` | bool | **FALSE** | spawn on the wall block itself rather than the adjacent floor (§2.6). **Unverifiable without a running game** — that is why it defaults off |
| `WaveRespectEmergePoints` | bool | TRUE | F14. The one default-TRUE key, because it is a *restriction* |
| `WaveVerbose` | bool | **FALSE** | per-decision debug log (§7) |

Reusing the existing species pool: `EmergeSpeciesPool` (`DeepCore.cpp:294-318`) already
exists and is shared. The director should use it and, if it is empty, fall back to
`level->EmergeCreature` (`Game.h:434`), exactly like `PickEmergeSpecies` does.

Parsing follows the established discipline at `DeepCore.cpp:320-342`: **only override a
numeric default when `Config_FindItem` proves the key is present**, and warn-and-keep on an
out-of-range value rather than silently accepting it.

A `verboseStartup` echo block should be extended at `DeepCore.cpp:412-420`.

---

## 7. Failure modes, and how to make them loud

The house style is already established: `DeepCore_WarnF` / `DeepCore_LogF`
(`DeepCore.cpp:22-23`), built on `Error_WarnF2` / `Error_DebugF2` (`Errors.h:110`,
`Errors.h:107`). Warn-once-per-cause, using a `static bool` or a `std::map<std::string,
bool>` — the pattern at `DeepCore.cpp:244-262`.

| # | Failure | Silent symptom | Loud treatment |
| --- | --- | --- | --- |
| 1 | `WaveDirector TRUE` but `EmergeSpeciesPool` empty *and* the level has no `EmergeCreature` | nothing ever spawns; user assumes the feature is broken | warn once at first evaluation, naming both facts and pointing at the cfg key. Mirrors `DeepCore.cpp:313-317` |
| 2 | No candidate block survives F1–F16 | silent nothing, forever, on a sealed map | count consecutive failures; warn once at 5, stating how many wall blocks were examined and which rule rejected the most. Then keep trying |
| 3 | `LegoObject_CreateInWorld` returns `nullptr` | wave vanishes | warn once with species ID and block; abort the wave (do not retry in the same tick) |
| 4 | Species ID out of range | reads past `legoGlobs.rockMonsterData` into unrelated exe memory | already handled by the existing bounds check (`DeepCore.cpp:107-115`) — reuse it, do not write a second one |
| 5 | **`targetBlockPos` left at (-1,-1)** (§1.4) | *heap corruption*, crash minutes later, unattributable | assert in the director's own spawn helper: if `!blockInBounds` for the block about to be written, `DeepCore_WarnF` and refuse to spawn |
| 6 | `Info_Send` sent for an `Info_Type` the user's `Lego.cfg` never configured | telegraph is silent; player is ambushed | check `Info_HasTypeText` first (`InfoMessages.h:255`); warn once and fall back to sound+shake |
| 7 | Director state survives a level change | wave 14 of the previous mission fires 30 s into the new one | `Reset()` from **both** `Level_Free` (`Game.cpp:3302`) and `Lego_LoadLevel2` (`Game.cpp:2951`); reset asserts the counters are zero when `WaveVerbose` |
| 8 | Smoke telegraph leaks `Smoke*` | slow memory growth, eventual visual mess | keep the pointer in director state; `Smoke_Remove` (`Smoke.h:153`) on spawn, on `Reset()`, and on abort |
| 9 | Config asks for something impossible (`WaveMaxPerWave > WaveMaxAlive`) | budget silently clamps; user's intent lost | warn at load time, clamp, and say what value is actually being used |
| 10 | Runaway spawn loop | level floods | two independent ceilings: the alive budget (§3.2) *and* a per-tick hard cap of 1 wave. Both checked, neither trusted alone |

`WaveVerbose TRUE` should log one line per decision — threat, alive count, budget,
candidate count, chosen block, species — so a user with a running game can hand back
something diagnosable. That is the closest thing to play-testing available to this project.

---

## 8. DECISION — recommended plan

**Ship it, in four independently-bisectable commits, all gated `FALSE` by default.**

### Commit 1 — fix the `targetBlockPos` hazard (prerequisite, standalone)

`src/openlrr/game/object/Object.cpp`, inside `LegoObject_TryGenerateSlugAtBlock` after the
`CreateInWorld` succeeds (`Object.cpp:1628-1637`):

```cpp
	LegoObject* slugObj = LegoObject_CreateInWorld(objModel, objType, objID, 0, wPos.x, wPos.y, heading);
	if (slugObj != nullptr) {
		/// FIX: LegoObject_Create_internal leaves targetBlockPos at (-1,-1) (Object.cpp:1163).
		///      The LIVEOBJ1_EXPANDING completion path at Object.cpp:3214-3226 feeds
		///      targetBlockPos straight to Level_Block_SetBusy when
		///      LegoObject_IsRockMonsterCanGather() is true -- and blockValue() does not
		///      bounds-check (Game.h:783), so (-1,-1) is a negative-index write.
		slugObj->targetBlockPos.x = (real32)bx;
		slugObj->targetBlockPos.y = (real32)by;

		slugObj->flags1 |= LIVEOBJ1_EXPANDING;
		...
```

No behaviour change for a correct slug spawn; removes a latent corruption path. Zero
warning risk.

### Commit 2 — the spawn primitive

New function beside the slug one in `Object.cpp`, declared in `Object.h` next to
`LegoObject_TryGenerateSlugAtBlock` (`Object.h:966`). **Not hooked** — pure custom, exactly
like its sibling.

```cpp
/// CUSTOM: Generate a RockMonster at an arbitrary block, with no emerge-trigger and no
///         slug-hole requirement. Modelled on LegoObject_TryGenerateSlugAtBlock
///         (Object.cpp:1602), which is the proven from-scratch spawn path, minus its
///         slug-hole gate (Object.cpp:1608-1616) and plus the targetBlockPos/busy-block
///         bookkeeping the emerge-completion path at Object.cpp:3214-3226 requires.
///
/// `bx`,`by` is the block the creature is credited to (marked busy, cleared when the
/// emerge animation finishes). `wx`,`wy` is where it physically stands, which the caller
/// may offset toward a wall face. Returns nullptr on any failure; never partially
/// constructs.
LegoObject* LegoObject_SpawnRockMonsterAtBlock(ObjectModel* objModel, LegoObject_ID objID,
                                               uint32 bx, uint32 by,
                                               real32 wx, real32 wy, real32 heading,
                                               Info_Type announceInfoType);
```

Body sketch:

```cpp
LegoRR::LegoObject* LegoRR::LegoObject_SpawnRockMonsterAtBlock(ObjectModel* objModel, LegoObject_ID objID,
                                                               uint32 bx, uint32 by,
                                                               real32 wx, real32 wy, real32 heading,
                                                               Info_Type announceInfoType)
{
	Lego_Level* level = Lego_GetLevel();
	if (level == nullptr || !blockInBounds(level, (sint32)bx, (sint32)by))
		return nullptr;                     // never let (-1,-1) reach Level_Block_SetBusy

	LegoObject* obj = LegoObject_CreateInWorld(objModel, LegoObject_RockMonster, objID,
	                                           0, wx, wy, heading);
	if (obj == nullptr)
		return nullptr;

	// The block this creature is emerging from. Object.cpp:3214-3226 reads this back and
	// clears the busy flag when the Emerge animation completes.
	obj->targetBlockPos.x = (real32)bx;
	obj->targetBlockPos.y = (real32)by;
	const Point2I blockPos = { (sint32)bx, (sint32)by };
	Level_Block_SetBusy(&blockPos, true);

	obj->flags1 |= LIVEOBJ1_EXPANDING;       // inactive until the emerge anim advances
	obj->flags3 &= ~LIVEOBJ3_POWEROFF;       // Create() sets this for monsters (Object.cpp:990)

	LegoObject_SetActivity(obj, Activity_Emerge, 0);
	LegoObject_UpdateActivityChange(obj);
	AITask_DoAnimationWait(obj);             // exe macro 0x00403e60 (AITask.h:549)

	if (announceInfoType >= 0)
		Info_Send(announceInfoType, nullptr, obj, nullptr);   // exe macro 0x00419ab0

	return obj;
}
```

Macro traps handled: this lives inside a function whose declarator-id is
`LegoRR::…`, so `LegoRR` is in scope for the unqualified type names inside
`AITask_DoAnimationWait`, `Info_Send`, `Level_Block_SetBusy` and `Map3D_*`. That is the
same reason the existing slug function compiles without a `using` (`Object.cpp` has no
file-scope `using namespace LegoRR;` — see `Object.cpp:1-31`).

### Commit 3 — the director

New files `src/openlrr/game/WaveDirector.hpp` / `.cpp`, added to `openlrr.vcxproj` as
`ClInclude` / `ClCompile` next to the DeepCore entries (`openlrr.vcxproj:258`,
`openlrr.vcxproj:384`).

```cpp
// WaveDirector.hpp
#pragma once
#include "../common.h"

namespace DeepCore { namespace Waves
{
	/// Advance the director by `elapsedReal` standard units (25.0 == 1 second,
	/// common.h:109). Safe and cheap to call unconditionally: returns immediately unless
	/// settings.waveDirector is set AND LegoRR::Lego_IsInLevel() (Game.h:812).
	/// Converts to world time itself -- see GameState.cpp:915-936.
	void Update(real32 elapsedReal);

	/// Drop all per-level state. Call from Level_Free (Game.cpp:3302) and after a
	/// successful Lego_LoadLevel2 (Game.cpp:2951).
	void Reset(void);

	/// True if a wave has been telegraphed and is pending. Diagnostics only.
	bool IsWavePending(void);
}}
```

Internal state, entirely DLL-side (the `Game.cpp:168-170` pattern):

```cpp
namespace {
	struct DirectorState
	{
		bool     armed          = false;   // settings validated for this level
		real32   levelTime      = 0.0f;    // world seconds since level start
		real32   nextEvalAt     = 0.0f;
		real32   spawnAt        = 0.0f;    // world time the telegraphed wave lands
		bool     wavePending    = false;
		uint32   waveIndex      = 0;
		sint32   spawnedByUs    = 0;
		uint32   consecutiveNoCandidate = 0;

		std::vector<Point2I> pendingWall;  // chosen wall blocks
		std::vector<Point2I> pendingFloor; // matching stand-on blocks
		std::vector<LegoRR::Smoke*> pendingSmoke;

		std::vector<uint8> reachable;      // width*height, rebuilt per evaluation
		uint32 reachW = 0, reachH = 0;
	};
	DirectorState g_state;
}
```

Core loop:

```cpp
void DeepCore::Waves::Update(real32 elapsedReal)
{
	using namespace LegoRR; // required: every Level_Block_*, Info_*, Map3D_* below is an
	                        // exe address macro whose expansion names LegoRR types
	                        // unqualified, and macros are not namespace members.

	if (!DeepCore::settings.waveDirector) return;
	if (!Lego_IsInLevel())               { return; }          // Game.h:812
	if (legoGlobs.flags1 & GAME1_FREEZEINTERFACE) return;     // paused (GameState.cpp:926)

	const real32 elapsedWorld = elapsedReal * Lego_GetGameSpeed();   // Game.h:1068
	g_state.levelTime += elapsedWorld / STANDARD_FRAMERATE;         // -> seconds

	if (!g_state.armed && !_ArmForLevel())                          // one-time validation
		return;

	if (g_state.wavePending) {
		if (g_state.levelTime >= g_state.spawnAt)
			_LandWave();
		return;                                                     // one thing at a time
	}

	if (g_state.levelTime < DeepCore::settings.waveGraceSeconds) return;
	if (g_state.levelTime < g_state.nextEvalAt)                 return;

	_Evaluate();          // budget, candidates, telegraph, set spawnAt / nextEvalAt
}
```

`_Evaluate()` in outline:

```cpp
static void _Evaluate(void)
{
	using namespace LegoRR;

	const real32 k       = _Threat(g_state.levelTime, DeepCore::settings.waveRampSeconds);
	const sint32 budget  = _AliveBudget(k);
	const sint32 alive   = _CountLiveMonsters();          // section 3.2
	const sint32 raiders = _CountLiveRaiders();           // section 3.3

	g_state.nextEvalAt = g_state.levelTime + _IntervalNow(k);

	if (raiders <= 0)      return;                        // F18
	if (alive >= budget)   return;                        // budget full: wait

	const sint32 want = std::min(_WaveSizeNow(k), budget - alive);

	_RebuildReachable();                                  // section 2.5(b)
	if (!_ChooseSpawnBlocks(want, g_state.pendingWall, g_state.pendingFloor)) {
		if (++g_state.consecutiveNoCandidate == 5) {
			DeepCore_WarnF(true, "WaveDirector: no legal spawn block found %i evaluations "
				"in a row on \"%s\". Waves are effectively disabled on this map.",
				(sint32)g_state.consecutiveNoCandidate, Lego_GetLevel()->name);
		}
		return;
	}
	g_state.consecutiveNoCandidate = 0;

	_Telegraph(g_state.pendingWall);                      // section 5.4
	g_state.wavePending = true;
	g_state.spawnAt     = g_state.levelTime + DeepCore::settings.waveWarnSeconds;
}
```

`_LandWave()`:

```cpp
static void _LandWave(void)
{
	using namespace LegoRR;

	const sint32 objID = DeepCore::PickWaveSpecies((sint32)Lego_GetLevel()->EmergeCreature,
	                                               g_state.waveIndex);

	for (size_t i = 0; i < g_state.pendingFloor.size(); i++) {
		const Point2I wall  = g_state.pendingWall[i];
		const Point2I floor = g_state.pendingFloor[i];

		// Re-check only the volatile rules -- a raider may have walked onto the tile
		// during the warning window. Static terrain rules were checked at Evaluate time.
		if (!_IsStillLegal(floor)) continue;              // F7, F10

		// Face away from the wall, then nudge the world position back toward it so the
		// creature visually starts at the rock face. 12.75f is the slug constant
		// (Object.cpp:1625-1626); see the UNDETERMINED note in section 1.2 step 5.
		const real32 heading = _HeadingFromWallToFloor(wall, floor);
		Point2F wPos = { 0.0f, 0.0f };
		Map3D_BlockToWorldPos(Lego_GetMap(), (uint32)floor.x, (uint32)floor.y, &wPos.x, &wPos.y);
		wPos.x -= Maths_Sin(heading) * 12.75f;
		wPos.y -= Maths_Cos(heading) * 12.75f;

		const uint32 bx = (uint32)(DeepCore::settings.waveSpawnOnWallBlock ? wall.x : floor.x);
		const uint32 by = (uint32)(DeepCore::settings.waveSpawnOnWallBlock ? wall.y : floor.y);

		LegoObject* obj = LegoObject_SpawnRockMonsterAtBlock(
			&legoGlobs.rockMonsterData[objID], (LegoObject_ID)objID,
			bx, by, wPos.x, wPos.y, heading, Info_Type::Info_RockMonster);

		if (obj == nullptr) {
			DeepCore_WarnF(true, "WaveDirector: spawn failed for species ID %i at block %i,%i.",
				objID, floor.x, floor.y);
			continue;
		}
		g_state.spawnedByUs++;
	}

	_ClearTelegraph();                     // Smoke_Remove etc.
	g_state.pendingWall.clear();
	g_state.pendingFloor.clear();
	g_state.wavePending = false;
	g_state.waveIndex++;
	g_state.nextEvalAt  = g_state.levelTime + _CooldownNow(_Threat(g_state.levelTime, ...));
}
```

Candidate search, which is the only part with any real cost:

```cpp
// Single pass over the block grid, reservoir-sampling up to N legal candidates so we
// never allocate a full candidate list on a large map, and never bias toward the
// top-left corner (which a "first match wins" scan would).
static bool _ChooseSpawnBlocks(sint32 want, std::vector<Point2I>& outWall, std::vector<Point2I>& outFloor);
```

Call sites to add — three lines total:

- `OpenLRR.cpp`, in `OpenLRR_MainLoop_Wrapper` after the `// post-MainLoop code here...`
  marker (`OpenLRR.cpp:978`): `DeepCore::Waves::Update(elapsed);`
- `Game.cpp`, in `Level_Free` beside the other custom teardown (`Game.cpp:3341-3352`):
  `DeepCore::Waves::Reset();`
- `Game.cpp`, in `Lego_LoadLevel2` on the success path (`Game.cpp:2965`):
  `DeepCore::Waves::Reset();`

### Commit 4 — settings + documentation

- Extend `DeepCore::Settings` (`DeepCore.hpp:88-97`) with the §6 fields.
- Extend `DeepCore::Load()` (`DeepCore.cpp:289-342`) with the same
  `Config_FindItem`-guarded parsing discipline, plus the cross-field clamp warnings (§7 #9).
- Add `PickWaveSpecies` beside `PickEmergeSpecies` (`DeepCore.cpp:127`).
- Extend the `verboseStartup` echo (`DeepCore.cpp:412-420`).
- Document every key in `data/Settings/DeepCore.cfg`, in the existing house voice, under
  the MONSTER DENSITY section, with the honesty note that the feature is compile-verified
  and not play-tested.

### 8.1 What this plan explicitly does NOT do

- **No struct grows.** Nothing in `Lego_Block`, `Lego_Level`, `LegoObject`,
  `LegoObject_Globs`, `Lego_Globs` or any other `assert_sizeof` type changes by one byte.
  All director state is DLL-side statics and `std::vector`s.
- **No new object IDs.** `LegoObject_ID_Count == 15` (`GameCommon.h:143`) and roughly
  eleven RockMonster names are already spoken for (`GameCommon.h:136-150`). The director
  spawns *existing* species.
- **No new `hook_write_jmpret`.** The interop table (`interop.cpp`) is untouched.
- **No new art, no new models, no new animations.** `Activity_Emerge` already exists in
  every creature's `.ae`.
- **No claim of play-testing.** Everything ships behind `FALSE`.

### 8.2 Build-contract risk

Baseline is Debug|x86 + Release|x86, v142, 0 errors, exactly 44 warnings. Watch list, in
order of likelihood:

1. **`sint32`/`uint32`/`size_t` conversion warnings** at Warning Level 3
   (`openlrr.vcxproj:99`). The tree's style is explicit `(sint32)` / `(uint32)` casts
   everywhere (e.g. `DeepCore.cpp:112-113`, `Object.cpp:3220-3221`). Follow it rigidly.
2. **`real32` narrowing** from `double` literals — write `0.5f`, never `0.5`.
3. **Unused parameter / unreferenced local** in the early-out paths.
4. **`std::min`/`std::max` with mismatched types** — cast both operands.

None of these are unavoidable; all are style-level. The plan adds no new `#pragma warning`
suppressions.

### 8.3 Suggested order of verification (what is actually checkable here)

1. Commit 1 alone → compile both configurations, confirm 0 errors / 44 warnings.
2. Commit 2 alone → same. `LegoObject_SpawnRockMonsterAtBlock` is unreferenced at this
   point; confirm that does not produce a warning (it is an external-linkage function, so
   it should not).
3. Commit 3 with `WaveDirector` absent from the cfg → the director must be provably inert:
   `Update()` returns on the first line.
4. Commit 4 → run with `VerboseStartup TRUE` and `WaveDirector FALSE` and confirm the echo
   block reports every new key at its default.

Anything past step 4 requires a running game and is the user's to do. The `WaveVerbose`
log (§7) exists precisely so that first run produces something we can read.

---

## 9. Open questions I could not close

1. **`12.75f`** (`Object.cpp:1625`). Not derivable from any constant in the tree;
   `Map3D_BlockSize` is an exe macro (`Map3D.h:345`) and `BlockSize` is per-level
   (`Game.h:400`). If it is `BlockSize * 0.31875` for the stock `BlockSize`, the director
   should compute it, not hardcode it. Needs a running game to check.
2. **Do vanilla emerge points sit on wall blocks or floor blocks?** (§2.6). The exe's
   `LegoObject_TryGenerateRMonster` (`0x0043b1f0`, `Object.h:974`) is undecompiled. This is
   why `WaveSpawnOnWallBlock` exists and defaults off.
3. **`LegoObject_IsRockMonsterCanGather`** (`0x004439b0`, `Object.h:1299`). Undecompiled.
   Its predicate determines whether §1.4's corruption path is reachable for slugs today.
   The fix in Commit 1 is correct regardless, but the *severity* of the existing bug is
   unknown.
4. **`Level_BlockUpdateSurface`** (`0x0042f620`, `Game.h:1545`) — the setter for
   `BLOCK1_WALL`. The "exposed wall" reading (§2.2) is inferred from the tooltip
   classification at `Game.cpp:1912-1916` and from where the function is called, not from
   its body.
5. **`Level_Block_SetBusy`** (`0x00432d30`, `Game.h:1781`) — whether it bounds-checks.
   Assumed not, on the strength of every in-tree accessor going through the unchecked
   `blockValue` macro (`Game.h:783`). The plan bounds-checks before calling, so the answer
   does not change the design.
6. **Whether `Info_RockMonster` is configured in a typical `Lego.cfg`.** `Info_Initialise`
   is exe (`GameState.cpp:218`). Handled defensively via `Info_HasTypeText`
   (`InfoMessages.h:255`) rather than assumed.
