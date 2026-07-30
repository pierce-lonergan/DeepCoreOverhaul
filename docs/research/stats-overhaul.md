# The Total Stats Overhaul

**Question asked:** can the eleven existing creature species be made to feel like genuinely
different enemies using only data plus C++ this project already owns?

**Answer in one line:** yes — but only about **half** the knob surface is ours to give *meaning*
to, the shipping channel everyone reaches for first (`-cfgadd`) is **actively dangerous** for this
particular block, and the correct vehicle is a small post-pass at the end of `Stats_Initialise`
driven by our own `Settings\DeepCore.cfg`.

**We cannot run the game.** No claim below has been play-tested. Everything is derived from source
with `file:line`, or from arithmetic done by hand. Anything that could only be settled by running
is marked **UNDETERMINED**.

Terminology, used consistently throughout:

- **OURS** — a real C++ body exists in this tree.
- **OURS+HOOKED** — a real C++ body exists *and* `interop.cpp` installs it over the executable, so
  original 1999 machine code calling that address runs our code.
- **EXE** — still an address macro (`#define X ((sig)0xADDR)`); 1999 machine code runs.

---

## 0. Evidence base and the two facts that shape everything

| Kind | Source |
| --- | --- |
| Parser | `src/openlrr/game/object/Stats.cpp:124-882` — `Stats_Initialise`, **OURS+HOOKED** at `interop.cpp:4096` |
| Field layout | `src/openlrr/game/object/Stats.h:149-213`, pinned by `assert_sizeof(ObjectStats, 0x150)` (`Stats.h:213`) |
| Flag bits | `Stats.h:40-139` (`StatsFlags1/2/3`) |
| Accessors | `Stats.cpp:941-1376` — **every one of them OURS+HOOKED**, `interop.cpp:4098-4162` |
| Species names | `GameCommon.h:137-150` |
| Consumers | `grep` over `src/openlrr`, quoted inline |
| Config engine | `engine/core/Config.cpp:337-364, 732-810, 834-848` — `Config_FindItem`/`Config_FindArray`/`Config_GetNextItem`/`Config_AppendConfig`, all **OURS+HOOKED** (`interop.cpp:381,382,395`) |

Two structural facts govern the whole design.

**Fact 1 — every accessor is ours, so every *value* reaches the executable's AI.**
All 57 getters are installed over the exe (`interop.cpp:4098-4162`). When undecompiled monster AI
at some address asks for `AttackRadius`, it calls `StatsObject_GetAttackRadius`
(`Stats.cpp:1212`), which is our code reading our parsed data. So **changing a number always
works**, whoever consumes it.

**Fact 2 — but changing a *meaning* only works where the consumer is ours.**
A flag whose only reader is undecompiled exe code can be switched on and off, and the exe will do
whatever it has always done with it. We cannot redefine it, cannot observe it, and cannot fix it if
it misbehaves. That is the axis the inventory in §1 is sorted on, and §2 is the subset that
survives it.

A third fact, inherited: **there is no hook trampoline.** `hook_write_jmpret` overwrites a prologue
with `E9 rel32` + `C3`, every restore path in `hook.cpp:30-32,47-49` is commented out, and 1515
installations pass zero backup buffers. Nothing in this document proposes wrapping an original
implementation, because that is not possible.

---

## 1. THE COMPLETE KNOB INVENTORY

### 1.0 How to read the columns

- **Key** — the config key name, looked up as `<gameName>::Stats::<ObjectName>::<Key>`
  (`Stats.cpp:119`, `#define Stats_ID`).
- **Default** — what the field holds when the key is **absent**. `Config_GetRealValue` /
  `Config_GetIntValue` return `0` for a missing key (`Config.h:323-324` →
  `Config.cpp:851-868`), and the whole level array is `memset` to zero first
  (`Stats.cpp:196`). So *absent means zero* unless a line of code says otherwise.
- **Per-level** — `PER-LEVEL` keys are colon-separated lists tokenised by
  `Util_Tokenise(str, argv, ":")` and indexed with `argv_minLevels` (`Stats.cpp:121`), which is
  `argv[min(argcLevels-1, lvl)]` — **the last supplied value repeats for all higher levels**.
  `REPLICATED` keys are read once and written into every level in a fill loop
  (`Stats.cpp:344-376` and `Stats.cpp:850-873`).
- **Consumer** — the honest answer to "does implemented C++ read this?".

> **Authoring trap, applies to eight keys.** `RubbleCoef`, `PathCoef`, the six `Flocks_*` reals,
> `Flocks_Size`, `WakeRadius` and `RandomMoveTime` are defaulted with the idiom
> `if (x == 0.0f) x = <default>;` (`Stats.cpp:318-319, 780-846`). **You cannot configure these to
> exactly zero** — zero is the sentinel meaning "give me the default". The post-pass in §4 writes
> the struct field directly and therefore *can* write zero, which is precisely why it needs the
> guard rails in §5.

### 1.1 Numeric fields — PER-LEVEL

| Key | Field | Type | Default | Parse | Read by implemented C++? |
| --- | --- | --- | --- | --- | --- |
| `Levels` | *(→ `statsGlobs.objectLevels`)* | `uint32` | 0 | `Stats.cpp:184`; fatal if > `OBJECT_MAXLEVELS` (16, `GameCommon.h:76`) | **YES** — `Stats_GetLevels` `Stats.cpp:1176`; `StatsObject_SetObjectLevel` `Stats.cpp:973` |
| `RouteSpeed` | `RouteSpeed` | `real32` | 0.0 | `Stats.cpp:199-208` | **NO** — getter OURS (`Stats.cpp:996`) but the only caller is `LegoObject_CalculateSpeeds` @`0x004468d0`, **EXE** (`Object.cpp:5531`, still commented out) |
| `SoilDrillTime` | `DrillTimes[Soil]` | `real32` | 0.0 | `Stats.cpp:212,220-237`, **×25.0** (`STANDARD_FRAMERATE`, `common.h:109`) | **YES** — `Object.cpp:2350`, `Object.cpp:3436` |
| `LooseDrillTime` | `DrillTimes[Loose]` | `real32` | 0.0 | ditto (`Stats.cpp:213`) | **YES** — same |
| `MedDrillTime` | `DrillTimes[Medium]` | `real32` | 0.0 | ditto (`Stats.cpp:214`) | **YES** — same |
| `HardDrillTime` | `DrillTimes[Hard]` | `real32` | 0.0 | ditto (`Stats.cpp:215`) | **YES** — same |
| `SeamDrillTime` | `DrillTimes[OreSeam]` **and** `[CrystalSeam]` | `real32` | 0.0 | `Stats.cpp:217-218` — one key, two slots | **YES** — same |
| `SingleWidthDig` | `flags1 \|= STATS1_SINGLEWIDTHDIG` | flag | off | `Stats.cpp:379-389` | **YES** — `Object.cpp:2351` (halves drill time) |
| `RepairValue` | `RepairValue` | `real32` | 0.0 | `Stats.cpp:391-399` | **NO** — getter `Stats.cpp:1170`, hooked `interop.cpp:4129`, no C++ caller |
| `SurveyRadius` | `SurveyRadius` | `sint32` | 0 | `Stats.cpp:401-409` | **YES** — `Object.cpp:2029` (only if `ABILITY_FLAG_SCANNER`) |
| `MaxCarry` | `MaxCarry` | `sint32` | 0 | `Stats.cpp:411-419` | **YES** — `Object.cpp:3629` |
| `CarryStart` | `CarryStart` | `sint32` | 0 | `Stats.cpp:421-429` | **NO** — getter `Stats.cpp:962`, hooked `interop.cpp:4104` |
| `CrystalDrain` | `CrystalDrain` | `sint32` | 0 | `Stats.cpp:431-439` | **YES** — `Object.cpp:2081` (buildings only) |
| `NumOfToolsCanCarry` | `NumOfToolsCanCarry` | `sint32` | 0 | `Stats.cpp:441-449` | **YES** — `Game.cpp:1752` |
| `UpgradeTime` | `UpgradeTime` | `real32` | 0.0 | `Stats.cpp:451-460`, **×25.0** | **YES** — `Object.cpp:2855` |
| `FunctionCoef` | `FunctionCoef` | `real32` | 0.0 | `Stats.cpp:462-470` | **YES** — `Object.cpp:2856`, `Weapons.cpp:1376` |
| `UpgradeCostOre` | `UpgradeCostOre` | `sint32` | 0 | `Stats.cpp:476-488` — **level index is the upgrade type, filled RIGHT-to-LEFT** (`upg=3..0` → `lvl=0..3`), unrelated to the object's real level (`Stats.cpp:1249`) | **YES** — `Object.cpp:2885` |
| `UpgradeCostStuds` | `UpgradeCostStuds` | `sint32` | 0 | `Stats.cpp:489-501`, same inversion | **YES** — `Object.cpp:2892` |
| `TrainPilot` | `flags2 \|= STATS2_TRAINPILOT` | flag | off | `Stats.cpp:616-626` | **NO** |
| `TrainSailor` | `STATS2_TRAINSAILOR` | flag | off | `Stats.cpp:628-638` | **NO** |
| `TrainDriver` | `STATS2_TRAINDRIVER` | flag | off | `Stats.cpp:640-650` | **NO** |
| `TrainDynamite` | `STATS2_TRAINDYNAMITE` | flag | off | `Stats.cpp:652-662` | **NO** |
| `TrainRepair` | `STATS2_TRAINREPAIR` | flag | off | `Stats.cpp:664-674` | **NO** |
| `TrainScanner` | `STATS2_TRAINSCANNER` | flag | off | `Stats.cpp:676-686` | **NO** |

### 1.2 Numeric fields — REPLICATED across every level

| Key | Field | Type | Default | Parse | Read by implemented C++? |
| --- | --- | --- | --- | --- | --- |
| `CollRadius` | `CollRadius` | `real32` | 0.0 | `Stats.cpp:325`; non-zero also sets `STATS1_COLLRADIUS` (`:327`) | **YES, heavily** — `Object.cpp:1980, 5414, 5425-5426, 5436, 5459, 5475, 5500, 5569-5570`; `Weapons.cpp:836, 1068` |
| `CollBox` | `CollBox` (`Size2F`) | 2×`real32` | {0,0} | `Stats.cpp:329-341`; **only consulted when `CollRadius` is absent/zero**; derives `CollRadius = max(w,h)*0.5` and sets `STATS1_COLLBOX` | **YES** — `Weapons.cpp:768` |
| `AlertRadius` | `AlertRadius` | `real32` | 0.0 | `Stats.cpp:262` | **YES** — `Object.cpp:5389`, gating 5437/5460/5476/5501 |
| `CollHeight` | `CollHeight` | `real32` | 0.0 | `Stats.cpp:265` | **YES** — `Weapons.cpp:1320` (beam aim = centre of collision box), `Bubbles.cpp:403,444,531`, `DamageText.cpp:92`, `Game.cpp:1195,1345`, `ElectricFence.cpp:771` |
| `PickSphere` | `PickSphere` | `real32` | 0.0 | `Stats.cpp:274` | **YES** — `Game.cpp:1215-1216, 1365-1366` (mouse picking) |
| `TrackDist` | `TrackDist` | `real32` | 0.0 | `Stats.cpp:259` | **YES** — `Game.cpp:1502` (`Lego_TrackObjectInRadar` camera distance) |
| `HealthDecayRate` | `HealthDecayRate` | `real32` | 0.0 | `Stats.cpp:268` | **YES** — `Object.cpp:3931` (continuous self-damage), `Object.cpp:3885` (reward quota) |
| `EnergyDecayRate` | `EnergyDecayRate` | `real32` | 0.0 | `Stats.cpp:271` | **YES** — `Object.cpp:3942` |
| `RestPercent` | `RestPercent` | `real32` | 0.0 | `Stats.cpp:292` | **NO** — getter `Stats.cpp:1200`, hooked `interop.cpp:4134` |
| `CarryMinHealth` | `CarryMinHealth` | `real32` | 0.0 | `Stats.cpp:295` | **NO** — getter `Stats.cpp:1206`, hooked `interop.cpp:4135` |
| `StampRadius` | `StampRadius` | `real32` | 0.0 | `Stats.cpp:298` | **YES** — `Object.cpp:1574` (stamp a carried crystal out of a raider's hands), `Object.cpp:5449` |
| `AttackRadius` | `AttackRadius` | `real32` | 0.0 | `Stats.cpp:301` | **NO** — getter `Stats.cpp:1212`, hooked `interop.cpp:4136`; consumed by exe AI only |
| `WakeRadius` | `WakeRadius` | `real32` | **20.0** | `Stats.cpp:841-842` | **YES** — `Object.cpp:5511` (`LegoObject_RockMonster_DoWakeUp` trigger distance) |
| `BuildingBase` | `BuildingBase` | `sint32` | −1 | `Stats.cpp:241-246` | **NO** — and dead upstream: no `BuildingBase` names exist in the lookup table (`Stats.cpp:243-244`) |
| `CostOre` | `CostOre` | `sint32` | 0 | `Stats.cpp:252` | **YES** — `Construction.cpp:80,110,118,130,232,672,1054`, `Game.cpp:1841`, `Object.cpp:5883` |
| `CostCrystal` | `CostCrystal` | `sint32` | 0 | `Stats.cpp:249` | **YES** — `Construction.cpp:67,129,227,666,1053`, `Object.cpp:3739,3764,5877,5953` |
| `CostRefinedOre` | `CostRefinedOre` | `sint32` | 0 | `Stats.cpp:255` | **YES** — `Construction.cpp:95,115,235,673,1056`, `Object.cpp:5884` |
| `DrillSound` | `DrillSound` | `SFX_ID` | `SFX_Drill` | `Stats.cpp:280-282` | **YES** — `Object.cpp:2313,2404,3440,3564`, `Game.cpp:1438` |
| `DrillFadeSound` | `DrillFadeSound` | `SFX_ID` | `SFX_DrillFade` | `Stats.cpp:283-285` | **YES** — same accessor, `fade=true` (`Stats.cpp:1190`) |
| `EngineSound` | `EngineSound` | `SFX_ID` | `SFX_NULL` | `Stats.cpp:287-289` | **YES** — `Object.cpp:2216`, `Game.cpp:1450` |
| `WaterEntrances` | `WaterEntrances` | `sint32` | 0 | `Stats.cpp:304` | **YES** — `Object.cpp:5824` (building placement) |
| `RubbleCoef` | `RubbleCoef` | `real32` | **1.0** | `Stats.cpp:307,318` | **NO** — getter `Stats.cpp:1014` (clamps to 1.5 under the faster-unit cheat); no C++ caller |
| `PathCoef` | `PathCoef` | `real32` | **1.0** | `Stats.cpp:310,319` | **NO** — getter `Stats.cpp:1030`, same clamp; no C++ caller |
| `OxygenCoef` | `OxygenCoef` | `real32` | 0.0 (negative consumes) | `Stats.cpp:313` | **YES** — `Game.cpp:1042` |
| `Capacity` | `Capacity` | `sint32` | 0 | `Stats.cpp:316` | **YES** — `Object.cpp:2114` (`LegoObject_CheckCanSteal`) |
| `AwarenessRange` | `AwarenessRange` | `real32` | 0.0 | `Stats.cpp:813-814` | **YES** — `Weapons.cpp:1463` (`Weapon_LegoObject_WithinAwarenessRange`; that function is OURS, its callers are exe) |
| `PainThreshold` | `PainThreshold` | `real32` | 0.0 | `Stats.cpp:817-818` | **YES** — `Object.cpp:857` (`LegoObject_CanShootObject`), `Object.cpp:5447` (stamping gate) |
| `FreezerTime` | `FreezerTime` | `real32` | 0.0 | `Stats.cpp:821-822` | **YES** — `Weapons.cpp:478` (≤0 falls back to 10.0 at the use site) |
| `FreezerDamage` | `FreezerDamage` | `real32` | 0.0 | `Stats.cpp:825-826` | **YES** — `Weapons.cpp:481` |
| `PusherDist` | `PusherDist` | `real32` | 0.0 | `Stats.cpp:829-830` | **YES** — `Weapons.cpp:464` |
| `PusherDamage` | `PusherDamage` | `real32` | 0.0 | `Stats.cpp:833-834` | **YES** — `Weapons.cpp:465` |
| `LaserDamage` | `LaserDamage` | `real32` | 0.0 | `Stats.cpp:837-838` | **YES** — `Weapons.cpp:454` |
| `RandomMoveTime` | `RandomMoveTime` | `real32` | **200.0** | `Stats.cpp:845-846` | **NO** — no getter at all; exe reads the field directly |
| `Flocks_Turn` | `Flocks_Turn` | `real32` | **0.06** | `Stats.cpp:779-780` | **NO** — getter `Stats.cpp:1293`; `LegoObject_Flocks_Initialise` is **EXE** (`Object.h:1684`) |
| `Flocks_Speed` | `Flocks_Speed` | `real32` | **2.0** | `Stats.cpp:783-784` | **NO** — same |
| `Flocks_Tightness` | `Flocks_Tightness` | `real32` | **2.0** | `Stats.cpp:787-788` | **NO** — same |
| `Flocks_GoalUpdate` | `Flocks_GoalUpdate` | `real32` | **2.0** | `Stats.cpp:791-792` | **NO** — same |
| `Flocks_Height` | `Flocks_Height` | `real32` | **30.0** | `Stats.cpp:795-796` | **NO** — same |
| `Flocks_Randomness` | `Flocks_Randomness` | `real32` | **2.0** | `Stats.cpp:799-800` | **NO** — same |
| `Flocks_AttackTime` | `Flocks_AttackTime` | `real32` | **300.0** | `Stats.cpp:809-810` | **NO** — same |
| `Flocks_Size` | `Flocks_Size` | `sint32` | **5** | `Stats.cpp:804-805` | **NO** — same |
| *(none)* | `unused_typeName[30]` | `char[30]` | zeroed | never parsed | no |
| *(none)* | `padding` | `undefined2` | 0 | never parsed | no |
| *(none)* | `unused_viewAngle` | `real32` | 0.0 | never parsed | no |
| *(none)* | `unused_viewDist` | `real32` | 0.0 | never parsed | no |

That is **44 configurable numeric keys** (plus `Levels`), of which **26 have an implemented C++
reader**. Four struct fields are never parsed by anything.

### 1.3 `StatsFlags1` — all 32 bits

All are `Config_GetBoolOrFalse` (`Config.h:331`) and REPLICATED, OR-ed into every level at
`Stats.cpp:851`, except the two noted PER-LEVEL and the two derived ones.

| Bit | Key | Value | Read by implemented C++? |
| --- | --- | --- | --- |
| `0x1` `SINGLEWIDTHDIG` | `SingleWidthDig` **(PER-LEVEL)** | `Stats.cpp:379-389` | **YES** — `Object.cpp:2352` |
| `0x2` `PROCESSORE` | `ProcessOre` | `:506` | **YES** — `Object.cpp:550-551, 1233, 3130, 3602` |
| `0x4` `PROCESSCRYSTAL` | `ProcessCrystal` | `:509` | **YES** — `Object.cpp:550, 1236, 3130, 3602, 3658` |
| `0x8` `STOREOBJECTS` | `StoreObjects` | `:512` | **YES** — `Object.cpp:257, 1230`; `Stats.cpp:938` (construction barriers) |
| `0x10` `SMALLTELEPORTER` | `SmallTeleporter` | `:518` | **YES** — `Object.cpp:1012, 3734`, `Construction.cpp:1076` |
| `0x20` `BIGTELEPORTER` | `BigTeleporter` | `:521` | **YES** — `Object.cpp:1015, 3735`, `Construction.cpp:1079` |
| `0x40` `WATERTELEPORTER` | `WaterTeleporter` | `:524` | **YES** — `Object.cpp:1018, 3736, 3768`, `Construction.cpp:1082` |
| `0x80` `COLLRADIUS` | *(derived, not a key)* | set at `:327` when `CollRadius != 0` | **NO** — nothing outside `Stats.cpp` tests this bit |
| `0x100` `COLLBOX` | *(derived, not a key)* | set at `:337` | **YES** — `Weapons.cpp:752` |
| `0x200` `CAUSESLIP` | `CauseSlip` | `:533` | **YES** — `Object.cpp:5424` — and note it **kills the monster** (`Object.cpp:5430-5431`) |
| `0x400` `RANDOMMOVE` | `RandomMove` | `:536` | **NO** — exe AI only |
| `0x800` `CANSCARE` | `CanScare` | `:539` | **YES** — `Object.cpp:5435` |
| `0x1000` `RANDOMENTERWALL` | `RandomEnterWall` | `:545` | **NO** — exe AI only |
| `0x2000` `SCAREDBYPLAYER` | `ScaredByPlayer` | `:548` | **YES** — `Object.cpp:5456` |
| `0x4000` `SNAXULIKE` | `SnaxULike` | `:527` | **YES** — `Object.cpp:1021` |
| `0x8000` `GRABMINIFIGURE` | `GrabMinifigure` | `:551` | **YES** — `Object.cpp:5397` |
| `0x10000` `CANCLEARRUBBLE` | `CanClearRubble` | `:560` | **NO** — exe AI only |
| `0x20000` `CANBEDRIVEN` | `CanBeDriven` | `:554` | **YES** — `Object.cpp:1913, 3773, 5646`, `Messages.cpp:548,622,756,775,790,915` |
| `0x40000` `CANSCARESCORPION` | `CanScareScorpion` | `:542` | **YES** — `Object.cpp:5493`; target species name `"Scorpion"` is **hardcoded** at `Object.cpp:5497` |
| `0x80000` `CANSTEAL` | `CanSteal` | `:557` | **YES** — `Object.cpp:2113` |
| `0x100000` `TOOLSTORE` | `ToolStore` | `:530` | **YES** — `Construction.cpp:920` |
| `0x200000` `FLOCKS` | `Flocks` | `:569` | **YES** — `Object.cpp:589, 973, 5469` |
| `0x400000` `FLOCKS_DEBUG` | `Flocks_Debug` | `:572` | **YES** — `Object.cpp:978` (off ⇒ the flock is hidden and unselectable) |
| `0x800000` `FLOCKS_SMOOTH` | `Flocks_Smooth` | `:575` | **NO** |
| `0x1000000` `CROSSWATER` | `CrossWater` | `:578` | **YES** — `Object.cpp:791, 5646`(ctx) |
| `0x2000000` `CROSSLAND` | `CrossLand` | `:581` | **YES** — `Object.cpp:794, 914`, `Messages.cpp:627,637,751,770` |
| `0x4000000` `FLOCKS_ONGROUND` | `Flocks_OnGround` | `:584` | **NO** |
| `0x8000000` `FLOCKS_QUICKDESTROY` | `Flocks_QuickDestroy` | `:587` | **NO** |
| `0x10000000` `FLOCKS_ANIMATEBYPITCH` | `Flocks_AnimateByPitch` | `:590` | **NO** |
| `0x20000000` `ROUTEAVOIDANCE` | `RouteAvoidance` | `:563` | **YES** — `Object.cpp:4625` (random 0.3–0.7 offset within a block instead of dead centre) |
| `0x40000000` `BUMPDAMAGE` | `BumpDamage` | `:566` | **SPLIT** — the *gate* is exe; the *body*, `LegoObject_BumpDamageRouteObject`, is OURS+HOOKED (`Object.cpp:5556-5577`, `interop.cpp:3721`) and hardcodes **10 damage** at reach `collRadius*2 + targetCollRadius` |
| `0x80000000` `MANTELEPORTER` | `ManTeleporter` | `:515` | **YES** — `Object.cpp:3737` |

### 1.4 `StatsFlags2` — all 32 bits

| Bit | Key | Value | Read by implemented C++? |
| --- | --- | --- | --- |
| `0x1` `SCAREDBYBIGBANGS` | `ScaredByBigBangs` | `Stats.cpp:595` | **NO** |
| `0x2` `UPGRADEBUILDING` | `UpgradeBuilding` | `:598` | **YES** — `Object.cpp:2877` |
| `0x4` `TRAINPILOT` | `TrainPilot` **(PER-LEVEL)** | `:616-626` | **NO** (only the SuperToolStore cheat, `Stats.cpp:1137`) |
| `0x8` `TRAINSAILOR` | `TrainSailor` **(PER-LEVEL)** | `:628-638` | **NO** — same |
| `0x10` `TRAINDRIVER` | `TrainDriver` **(PER-LEVEL)** | `:640-650` | **NO** — same |
| `0x20` `TRAINDYNAMITE` | `TrainDynamite` **(PER-LEVEL)** | `:652-662` | **NO** — same |
| `0x40` `TRAINREPAIR` | `TrainRepair` **(PER-LEVEL)** | `:664-674` | **NO** — same |
| `0x80` `TRAINSCANNER` | `TrainScanner` **(PER-LEVEL)** | `:676-686` | **NO** — same |
| `0x100` `TRACKER` | `Tracker` | `:688` | **YES** — `Weapons.cpp:1327` |
| `0x200` `GENERATEPOWER` | *(via `PowerBuilding`)* | `:691` sets `POWERBUILDING = GENERATEPOWER\|SELFPOWERED` | **YES** — `Object.cpp:1332, 2088, 2098`, `Construction.cpp:928`, `NERPsFunctions.cpp:368` |
| `0x400` `SELFPOWERED` | `SelfPowered` | `:694` | **YES** — `Object.cpp:2088`, `ElectricFence.cpp:307`, `Stats.cpp:1370` |
| `0x800` `USEBIGTELEPORTER` | `UseBigTeleporter` | `:604` | **YES** — `Object.cpp:3735` |
| `0x1000` `USESMALLTELEPORTER` | `UseSmallTeleporter` | `:607` | **YES** — `Object.cpp:3734, 3785` |
| `0x2000` `USEWATERTELEPORTER` | `UseWaterTeleporter` | `:610` | **YES** — `Object.cpp:3736` |
| `0x4000` `ATTACKPATHS` | `AttackPaths` | `:697` | **NO** — exe AI only |
| `0x8000` `SPLITONZEROHEALTH` | `SplitOnZeroHealth` | `:700` | **YES** — `Object.cpp:1841` → `LegoObject_StartCrumbling` (OURS+HOOKED, `interop.cpp:3677`) |
| `0x10000` `CANBEHITBYFENCE` | `CanBeHitByFence` | `:703` | **YES** — `Object.cpp:3972` |
| `0x20000` `CANDOUBLESELECT` | `CanDoubleSelect` | `:706` | **YES** — `GameState.cpp:2350` |
| `0x40000` `CANBESHOTAT` | `CanBeShotAt` | `:601` | **YES** — `Object.cpp:856` (`LegoObject_CanShootObject`, OURS+HOOKED `interop.cpp:3565`), called at spawn `Object.cpp:985` |
| `0x80000` `DRAINPOWER` | `DrainPower` | `:709` | **YES** — `Object.cpp:2769` |
| `0x100000` `USEHOLES` | `UseHoles` | `:712` | **YES** — `Object.cpp:1847, 3045`, `Game.cpp:2625` |
| `0x200000` `CROSSLAVA` | `CrossLava` | `:715` | **NO** — exe routing only |
| `0x400000` `USELEGOMANTELEPORTER` | `UseLegoManTeleporter` | `:613` | **YES** — `Object.cpp:3737` |
| `0x800000` `DAMAGECAUSESCALLTOARMS` | `DamageCausesCallToArms` | `:718` | **YES** — `Object.cpp:3910` |
| `0x1000000` `CANFREEZE` | `CanFreeze` | `:721` | **YES** — `Weapons.cpp:477`; **absent ⇒ the target takes no freezer effect at all** |
| `0x2000000` `CANLASER` | `CanLaser` | `:724` | **YES** — `Weapons.cpp:453`; **absent ⇒ immune to the laser** |
| `0x4000000` `CANPUSH` | `CanPush` | `:727` | **YES** — `Weapons.cpp:463`; **absent ⇒ immune to the pusher** |
| `0x8000000` `DONTSHOWDAMAGE` | `DontShowDamage` | `:730` | **YES** — `DamageText.cpp:304` |
| `0x10000000` `REMOVEREINFORCEMENT` | `RemoveReinforcement` | `:733` | **YES** — `Object.cpp:3061` |
| `0x20000000` `DONTSHOWONRADAR` | `DontShowOnRadar` | `:736` | **YES** — `RadarMap.cpp:626` |
| `0x40000000` `INVISIBLEDRIVER` | `InvisibleDriver` | `:739` | **NO** |
| `0x80000000` `UNSELECTABLE` | `Unselectable` | `:742` | **YES** — `Object.cpp:1150`, and *only* at object creation (comment `Object.cpp:1148`) |

### 1.5 `StatsFlags3` — all 10 defined bits

| Bit | Key | Value | Read by implemented C++? |
| --- | --- | --- | --- |
| `0x1` `CARRYVEHICLES` | `CarryVehicles` | `Stats.cpp:747` | **YES** — `Object.cpp:1905` |
| `0x2` `VEHICLECANBECARRIED` | `VehicleCanBeCarried` | `:750` | **NO** |
| `0x4` `CANSTRAFE` | `CanStrafe` | `:753` | **NO** |
| `0x8` `CLASSASLARGE` | `ClassAsLarge` | `:756` | **NO** |
| `0x10` `GETOUTATLAND` | `GetOutAtLand` | `:759` | **NO** |
| `0x20` `GETINATLAND` | `GetInAtLand` | `:762` | **NO** |
| `0x40` `TAKECARRYINGDRIVERS` | `TakeCarryingDrivers` | `:765` | **NO** |
| `0x80` `SHOWHEALTHBAR` | `ShowHealthBar` | `:768` | **YES** — `Bubbles.cpp:300` (`Bubble_ShowHealthBar` early-outs), `Game.cpp:2624` |
| `0x100` `NEEDSPILOT` | `NeedsPilot` | `:771` | **YES** — `Object.cpp:786` |
| `0x200` `ENTERTOOLSTORE` | `EnterToolStore` | `:774` | **NO** |

### 1.6 Inventory totals

| | Count | With an implemented C++ reader |
| --- | --- | --- |
| Numeric keys | 44 (+`Levels`) | 26 |
| `StatsFlags1` bits | 32 (30 keys + 2 derived) | 21 (+1 split) |
| `StatsFlags2` bits | 32 | 21 |
| `StatsFlags3` bits | 10 | 3 |
| **Total** | **118 knobs** | **71 whose behaviour we can shape** |

**What "NO" means, precisely.** It does **not** mean the knob is inert. `RouteSpeed` is the
clearest case: no implemented C++ reads it, yet it is one of the most *felt* stats in the game,
because exe routing calls our hooked `StatsObject_GetRouteSpeed` (`Stats.cpp:996`) every tick. "NO"
means: **we can change the number, but not what the number means, and we cannot read the code that
acts on it.** Those knobs are usable — they just carry an irreducible verification gap, which
matters a great deal in a project that cannot run the game.

---

## 2. THE LEVERS THAT ACTUALLY WORK

Filtering §1 to what is relevant to a *creature*, and grouping by the implemented function that
gives it meaning. This is the palette §3 draws from.

### 2.1 Tier A — contact behaviour, all inside one function we own

`LegoObject_Callback_SlipAndScare` (`Object.cpp:5369-5519`, OURS+HOOKED at `interop.cpp:3718`) is
the single richest piece of implemented monster behaviour in the tree. Every branch is a lever:

| Lever | Effect, with the exact condition | Line |
| --- | --- | --- |
| `GrabMinifigure` | Throws a raider, but only if the spot 15 units ahead of the monster is neither wall nor solid building, and the raider is within **its own** `CollRadius` of that spot | `5397-5421` |
| `CauseSlip` + both `CollRadius` | Raider slips when `dist < collRadius + monsterCollRadius` — **and the monster then dies** (`health = -1`, `LIVEOBJ3_REMOVING`) | `5424-5433` |
| `CanScare` + `AlertRadius` | Raider runs away when `dist < collRadius + alertRadius`; **inert if `AlertRadius == 0`** | `5435-5441` |
| `PainThreshold` + `StampRadius` | Monster stamps a nearby raider/vehicle only while `monster.health > PainThreshold` | `5447-5453` |
| `ScaredByPlayer` + `AlertRadius` | The **monster** flees the player; note this branch does *not* require `AlertRadius != 0` | `5456-5465` |
| `Flocks` + `AlertRadius` | Flock damages a raider using the weapon literally named `"BatAttack"` and pushes a follow task | `5469-5490` |
| `CanScareScorpion` + `AlertRadius` | Scares only the object resolved from the hardcoded name `"Scorpion"` | `5493-5506` |
| `WakeRadius` | While the monster is `LIVEOBJ3_POWEROFF` (which is how every monster spawns, `Object.cpp:990`), a unit within `WakeRadius` wakes it | `5511-5516` |

### 2.2 Tier A — how a species dies, and what kills it

| Lever | Effect | Where |
| --- | --- | --- |
| `CanLaser` / `LaserDamage` | Laser hit applies `LaserDamage` to **the object hit**. Flag absent ⇒ the laser does **nothing** to it | `Weapons.cpp:452-456` (`Weapon_GunHitObject`, OURS+HOOKED `interop.cpp:4357`) |
| `CanPush` / `PusherDamage` / `PusherDist` | Damage **and** knockback distance; flag absent ⇒ immune | `Weapons.cpp:461-470` |
| `CanFreeze` / `FreezerDamage` / `FreezerTime` | Damage plus freeze duration (`≤0` falls back to 10.0 at the use site); flag absent ⇒ cannot be frozen | `Weapons.cpp:476-486` |
| `CanBeShotAt` + `PainThreshold` | Whether raiders will target it **at all**: `CANBESHOTAT && health > PainThreshold` | `Object.cpp:853-862`, evaluated at spawn `Object.cpp:985` |
| `SplitOnZeroHealth` | Death → `LegoObject_StartCrumbling` (OURS+HOOKED) instead of removal | `Object.cpp:1841-1845` |
| `UseHoles` | Death → burrow away through a wall hole; also drives the wall-entry branch | `Object.cpp:1847-1852`, `3045-3062` |
| `RemoveReinforcement` | Entering a reinforced wall strips the reinforcement | `Object.cpp:3061` |
| `HealthDecayRate` | Continuous self-damage, applied as `rate / 25.0 * elapsed` per tick | `Object.cpp:3929-3933` |
| `CanBeHitByFence` | Electric fences can spark it | `Object.cpp:3972` |
| `DamageCausesCallToArms` | Any damage it takes turns on Call To Arms | `Object.cpp:3910` |

The one thing **not** in `ObjectStats`: how much damage a monster *deals*, and how much a given
weapon deals *to* it, live in the weapon table — `weaponStats->objectCoefs[type][id][level]`,
parsed by `Weapon_Initialise` (OURS, `Weapons.cpp:40-175`) from per-object-name keys inside each
`WeaponTypes` entry, read by `Weapon_GetDamageForObject` (`Weapons.cpp:254-262`). That is a
genuine second axis of species identity, and it lives in the user's `Lego.cfg`, so §4's shipping
problem applies to it identically.

### 2.3 Tier A — presentation and readability

| Lever | Effect | Where |
| --- | --- | --- |
| `ShowHealthBar` | Health bar suppressed entirely if absent; **also** changes the "destroy all" path (`!SHOWHEALTHBAR \|\| USEHOLES` ⇒ force-remove) | `Bubbles.cpp:296-301`, `Game.cpp:2624-2630` |
| `DontShowDamage` | Suppresses floating damage numbers | `DamageText.cpp:301-305` |
| `DontShowOnRadar` | Removes it from the radar — the cheapest "ambush" knob in the game | `RadarMap.cpp:625-626` |
| `TrackDist` | Camera distance when tracking this species | `Game.cpp:1497-1505` |
| `CollHeight` | Beam aim point, health-bar and damage-number anchor height | `Weapons.cpp:1320`, `Bubbles.cpp:403,444,531` |
| `PickSphere` | Mouse-pick radius | `Game.cpp:1215-1216,1365-1366` |
| `RouteAvoidance` | Wanders to a random 0.3–0.7 point in each block instead of the centre | `Object.cpp:4620-4632` |
| `DrillSound` / `DrillFadeSound` / `EngineSound` | Per-species audio signature | `Object.cpp:2216,2313,2404,3440,3564` |

### 2.4 Tier B — real, effective, but exe-defined

Use these freely for *feel*; do not expect to be able to explain or debug them.
`RouteSpeed` (movement), `RubbleCoef`, `PathCoef`, `AttackRadius`, `RandomMove` +
`RandomMoveTime`, `RandomEnterWall`, `CanClearRubble`, `AttackPaths`, `ScaredByBigBangs`,
`CrossLava`, `BumpDamage` (gate exe, body ours), and the whole `Flocks_*` numeric family plus
`Flocks_Smooth` / `Flocks_OnGround` / `Flocks_QuickDestroy` / `Flocks_AnimateByPitch`
(`LegoObject_Flocks_Initialise` is **EXE**, `Object.h:1684`).

### 2.5 Interaction with what already shipped

- **Per-instance scale + tint** (`Object.cpp:961`, `DeepCore::ApplyCreatureVariant`) is applied to
  the cloned `CreatureModel` at spawn and is **cosmetic only** — it does not touch `ObjectStats`.
  So a 1.6× brute and a 0.6× sprite of the same species share one stat block. **Design
  consequence:** do not use variant scale to imply a stat difference the stats do not have. Use
  variants *within* a species identity (three shades of the same threat), and use stats *between*
  species. The one exception worth flagging: `CollRadius` is a stat, not a model property, so a
  visually huge variant still has the base species' reach. That mismatch is a feature if you keep
  variant scales inside roughly ±25%, and a bug if you go to 2×.
- **Per-weapon beam styles** (`Weapons.cpp:817-864`) change what the beam *looks* like. Pair each
  species' immunity (§2.2) with a distinct beam style so the player can learn "the blue beam is
  the one that works on ice" visually rather than from a damage number they cannot see when
  `DontShowDamage` is on.
- **Multi-species emerge** (`Game.cpp:3055`, `DeepCore::PickEmergeSpecies`) is deterministic per
  trigger index. That is what makes this whole overhaul legible: a given cavern always breeds the
  same species, so distinct stat identities become *learnable* rather than noise.

---

## 3. ELEVEN DISTINCT ENEMY IDENTITIES

Ground rules used for all eleven:

1. Only levers from §2. Where a Tier B knob is used, it is marked **(B)**.
2. Nothing in here can make a mission unwinnable on its own — see §5 for the invariants, which are
   enforced in code, not by discipline.
3. Every species keeps **at least one** of `CanLaser` / `CanPush` / `CanFreeze`.
4. Numbers are *relative deltas against whatever the user's `Lego.cfg` already has*, expressed as
   absolute values only where the field has an implemented, well-understood meaning. Absolute
   values below are proposals, not measurements: **UNDETERMINED** until someone with the game runs
   them.

### 3.1 RockMonster — "the wall that walks"

*The player learns:* it is slow, it is unstoppable at contact range, and it must be fought at
distance with the laser.

| Knob | Value | Why |
| --- | --- | --- |
| `RouteSpeed` **(B)** | 0.85 | the reference brute; everything else is fast or slow relative to this |
| `GrabMinifigure` | TRUE | signature move (`Object.cpp:5397`) |
| `StampRadius` | 12 | crushes carried crystals (`Object.cpp:1574`) |
| `PainThreshold` | 30 | stops stamping when badly hurt — wounded means less dangerous |
| `CanBeShotAt` | TRUE | must remain targetable |
| `CanLaser` / `LaserDamage` | TRUE / 10 | laser is the answer |
| `CanPush` / `PusherDamage` / `PusherDist` | TRUE / 4 / 15 | shoves, doesn't fly |
| `CanFreeze` / `FreezerDamage` / `FreezerTime` | TRUE / 3 / 8 | freezable but not solved by it |
| `SplitOnZeroHealth` | TRUE | crumbles into smaller pieces |
| `ShowHealthBar` | TRUE | the one enemy whose HP you watch |
| `DamageCausesCallToArms` | TRUE | hurting it summons the base |
| `WakeRadius` | 20 | vanilla default |
| `AlertRadius` | 0 | it does not frighten; it arrives |
| `TrackDist` | 90 | camera pulls back — it reads as big |

### 3.2 IceMonster — "the one you must not freeze"

*The player learns:* the freezer is worse than useless here; it is fast, it skids, and it shatters.

| Knob | Value | Why |
| --- | --- | --- |
| `RouteSpeed` **(B)** | 1.30 | the fast brute |
| `CanFreeze` | **FALSE** | the identity. `Weapons.cpp:477` — a freezer hit does literally nothing |
| `CanPush` / `PusherDist` / `PusherDamage` | TRUE / **45** / 6 | slides absurdly far on ice; the pusher is the crowd-control answer |
| `CanLaser` / `LaserDamage` | TRUE / 16 | brittle to laser |
| `SplitOnZeroHealth` | TRUE | shatters |
| `GrabMinifigure` | FALSE | it hits and moves, it does not grapple |
| `StampRadius` / `PainThreshold` | 9 / 20 | |
| `CollHeight` | −10% vs RockMonster | lower silhouette, beams aim lower |
| `WakeRadius` | 30 | wakes from further — it hears you coming |
| `DrillSound` | distinct `SFX_ID` | audio tell before the visual tell |

### 3.3 LavaMonster — "the one on a fuse"

*The player learns:* freeze it or survive it; it burns itself out but takes the room with it.

| Knob | Value | Why |
| --- | --- | --- |
| `HealthDecayRate` | 0.25 | self-damage `rate/25*elapsed` per tick (`Object.cpp:3931`). At 100 starting health that is a long fuse, not a free win. **Units UNDETERMINED** — see §5.3 |
| `CanFreeze` / `FreezerDamage` / `FreezerTime` | TRUE / 12 / **20** | freezing is *the* counter-play |
| `CanLaser` / `LaserDamage` | TRUE / **2** | laser barely scratches it |
| `CanPush` | FALSE | immovable |
| `DamageCausesCallToArms` | TRUE | |
| `CanBeHitByFence` | TRUE | fences are a viable static defence |
| `UseHoles` | FALSE | it does not retreat |
| `SplitOnZeroHealth` | TRUE | |
| `CrossLava` **(B)** | TRUE | uses terrain nothing else can |
| `StampRadius` / `PainThreshold` | 14 / 0 | dangerous right up to the end |
| `TrackDist` | 100 | |

### 3.4 TinyRM — "the one that trips you"

*The player learns:* individually trivial, collectively a crystal-delivery denial weapon.

| Knob | Value | Why |
| --- | --- | --- |
| `CauseSlip` | TRUE | contact makes a raider slip **and kills the monster** (`Object.cpp:5430-5431`) — a one-shot mine, which is exactly right for a tiny |
| `CollRadius` | small (≈40% of RockMonster) | must actually touch you |
| `RouteSpeed` **(B)** | 1.5 | |
| `RouteAvoidance` | TRUE | skitters off block centres (`Object.cpp:4625`) |
| `ShowHealthBar` | FALSE | no HP drama for chaff — also force-removes on "destroy all" (`Game.cpp:2624`) |
| `DontShowDamage` | TRUE | keeps the screen clean during swarms |
| `CanBeShotAt` | TRUE | still killable |
| `CanLaser` / `LaserDamage` | TRUE / 40 | one shot each |
| `CanPush` / `PusherDist` | TRUE / 60 | the pusher is the *fun* answer: bowling |
| `CanFreeze` | FALSE | too small to freeze |
| `WakeRadius` | 12 | short fuse, tight ambushes |

### 3.5 TinyIM — "the one you never see coming"

*The player learns:* check the radar — and then learn that the radar lies about these.

| Knob | Value | Why |
| --- | --- | --- |
| `DontShowOnRadar` | TRUE | `RadarMap.cpp:626`. The whole identity in one bit |
| `CanScare` + `AlertRadius` | TRUE + 24 | frightens raiders into dropping what they carry (`Object.cpp:5435`) |
| `CauseSlip` | FALSE | contrast with TinyRM: this one *lives* |
| `CanFreeze` | FALSE | consistent with IceMonster; teaches a family rule |
| `CanLaser` / `LaserDamage` | TRUE / 30 | |
| `CanPush` / `PusherDist` | TRUE / 50 | |
| `RouteSpeed` **(B)** | 1.45 | |
| `ShowHealthBar` | FALSE | |
| `WakeRadius` | 35 | wakes early, so the ambush is *its* choice |
| `TrackDist` | 55 | close camera; unsettling |

### 3.6 Bat — "atmosphere with teeth"

*The player learns:* you cannot fight these, only route around them.

| Knob | Value | Why |
| --- | --- | --- |
| `Flocks` | TRUE | required; `Object.cpp:973` allocates the flock, `5469` runs the attack |
| `Flocks_Debug` | FALSE | keep them hidden + unselectable, as vanilla (`Object.cpp:978-982`) |
| `AlertRadius` | 18 | the flock's bite radius (`Object.cpp:5476`) |
| `Flocks_Size` **(B)** | 8 | denser than the default 5 |
| `Flocks_Speed` / `Flocks_Turn` / `Flocks_Randomness` **(B)** | 3.0 / 0.10 / 3.0 | erratic, fast |
| `Flocks_Height` **(B)** | 22 | lower — they read as *in the tunnel with you*, not scenery |
| `CanBeShotAt` | FALSE | they are weather, not enemies. **Requires the §5 audit**: never assign Bat to a level whose objective counts monster kills |
| `ShowHealthBar` / `DontShowDamage` | FALSE / TRUE | |
| `DontShowOnRadar` | TRUE | |
| `CanBeHitByFence` | FALSE | |

Their damage comes from the weapon named `"BatAttack"` (`Object.cpp:5478`), not from `ObjectStats`
— so bat lethality is tuned in the weapon table, per §2.2.

### 3.7 Slug — "the parasite"

*The player learns:* it is not trying to kill you, it is trying to make you poor.

| Knob | Value | Why |
| --- | --- | --- |
| `CanSteal` | TRUE | `Object.cpp:2113` |
| `Capacity` | **2** | the per-slug crystal cap (`Object.cpp:2114`). **Must be ≥ 1** — see §5.2 |
| `UseHoles` | TRUE | escapes through a hole instead of dying (`Object.cpp:1847`) |
| `RouteSpeed` **(B)** | 0.6 | slow enough that you *can* intercept it — that is the game |
| `DontShowOnRadar` | FALSE | you must be able to find it |
| `ShowHealthBar` | FALSE | with `UseHoles` this also force-removes it on "destroy all" (`Game.cpp:2624-2630`), which is correct: it burrows, it does not die on screen |
| `CanBeShotAt` | TRUE | |
| `CanLaser` / `LaserDamage` | TRUE / 25 | |
| `CanFreeze` / `FreezerTime` | TRUE / 15 | freezing it is the clean answer — it drops nothing and goes nowhere |
| `CanPush` | FALSE | too heavy/low |
| `WakeRadius` | 40 | it notices your crystals from a long way off |
| `TrackDist` | 45 | |

Spawning is via `LegoObject_TryGenerateSlugAtBlock` (OURS, `Object.cpp:1603-1664`), which requires
a registered slug hole; nothing in this overhaul changes that.

### 3.8 SmallSpider — "the flinch"

*The player learns:* harmless, and that is the joke — until one appears mid-haul.

| Knob | Value | Why |
| --- | --- | --- |
| `CanScare` + `AlertRadius` | TRUE + **34** | the biggest scare radius in the roster; raiders bolt (`Object.cpp:5435-5441`) |
| `CanBeShotAt` | FALSE | not a combat unit (same §5 audit as Bat) |
| `RouteAvoidance` | TRUE | never walks a straight line |
| `RouteSpeed` **(B)** | 1.8 | fastest thing in the game |
| `RandomMove` + `RandomMoveTime` **(B)** | TRUE + 60 | twitchy (exe-defined; use, do not rely on) |
| `DontShowOnRadar` | TRUE | |
| `ShowHealthBar` / `DontShowDamage` | FALSE / TRUE | |
| `CollRadius` | very small | |
| `CanPush` / `PusherDist` | TRUE / 80 | punting one across the cavern should be a delight |
| `CanLaser` / `CanFreeze` | TRUE / FALSE | |
| `TrackDist` | 35 | uncomfortably close |

### 3.9 Spider *(unused)* — "the ambusher"

*The player learns:* it does not chase; it waits where you must walk.

| Knob | Value | Why |
| --- | --- | --- |
| `RouteSpeed` **(B)** | 0.5 | deliberately slower than SmallSpider — a different animal, not a bigger one |
| `StampRadius` / `PainThreshold` | **18** / **60** | the widest stamp in the roster, but it stops the moment it drops below 60 health. Hit it once and it stops taking your crystals |
| `CanScare` + `AlertRadius` | TRUE + 20 | |
| `GrabMinifigure` | FALSE | |
| `CanBeShotAt` | TRUE | |
| `CanLaser` / `LaserDamage` | TRUE / 8 | armoured |
| `CanFreeze` / `FreezerTime` | TRUE / 25 | freeze is the counter |
| `CanPush` | FALSE | anchored |
| `DontShowOnRadar` | TRUE | ambush requires invisibility |
| `ShowHealthBar` | TRUE | |
| `UseHoles` | FALSE | |

### 3.10 Snake *(unused)* — "the vandal"

*The player learns:* it does not fight your raiders, it undoes your engineering.

| Knob | Value | Why |
| --- | --- | --- |
| `RemoveReinforcement` | TRUE | strips reinforcement when it enters a wall (`Object.cpp:3061`) — the only species that does |
| `UseHoles` | TRUE | leaves rather than dies |
| `RouteSpeed` **(B)** | 1.6 | hit and run |
| `RandomEnterWall` **(B)** | TRUE | unpredictable entry points |
| `DontShowOnRadar` | TRUE | |
| `CanBeShotAt` | TRUE | you *can* punish it if you are quick |
| `PainThreshold` | 0 | |
| `CanLaser` / `LaserDamage` | TRUE / 35 | glass cannon in reverse: fragile, not dangerous |
| `CanPush` / `PusherDist` | TRUE / 70 | knock it away from the wall it is ruining |
| `CanFreeze` | FALSE | |
| `ShowHealthBar` | FALSE | |
| `GrabMinifigure` / `CauseSlip` | FALSE / FALSE | |

### 3.11 Scorpion *(unused)* — "the one your enemies fear"

*The player learns:* the other monsters give it room — and so should your vehicles.

| Knob | Value | Why |
| --- | --- | --- |
| *(target of)* `CanScareScorpion` | — | `Object.cpp:5493-5506` resolves the name `"Scorpion"` **hardcoded**. Give `CanScareScorpion TRUE` + `AlertRadius > 0` to RockMonster and LavaMonster and the roster acquires a visible pecking order at zero code cost |
| `ScaredByPlayer` + `AlertRadius` | TRUE + 26 | it also flees *you* (`Object.cpp:5456`) — a skittish apex predator |
| `CanBeHitByFence` | TRUE | fences are the designed answer |
| `CanPush` | **FALSE** | immune to the pusher: it cannot be kited |
| `CanLaser` / `LaserDamage` | TRUE / 14 | |
| `CanFreeze` / `FreezerTime` | TRUE / 10 | |
| `StampRadius` / `PainThreshold` | 15 / 45 | |
| `CanBeShotAt` | TRUE | |
| `ShowHealthBar` | TRUE | |
| `RouteSpeed` **(B)** | 1.15 | |
| `DamageCausesCallToArms` | TRUE | |
| `TrackDist` | 80 | |

### 3.12 The roster read as a whole

The point of the table below is that a player can derive a **rule** from it, which is the
difference between eleven enemies and one enemy with eleven skins:

| Species | Laser | Pusher | Freezer | Dies by | Radar | Signature |
| --- | --- | --- | --- | --- | --- | --- |
| RockMonster | ✔ 10 | ✔ 15 | ✔ 8 | crumble | ✔ | throws raiders |
| IceMonster | ✔ 16 | ✔ **45** | ✘ | crumble | ✔ | skids |
| LavaMonster | ✔ **2** | ✘ | ✔ **20** | crumble + fuse | ✔ | burns out |
| TinyRM | ✔ 40 | ✔ 60 | ✘ | removed | ✔ | one-shot trip mine |
| TinyIM | ✔ 30 | ✔ 50 | ✘ | removed | **✘** | invisible scare |
| Bat | ✘ *(unshootable)* | ✘ | ✘ | removed | ✘ | flock |
| Slug | ✔ 25 | ✘ | ✔ 15 | burrows | ✔ | steals |
| SmallSpider | ✔ | ✔ **80** | ✘ | removed | ✘ | panic |
| Spider | ✔ 8 | ✘ | ✔ 25 | crumble | ✘ | wide stamp, quits when hurt |
| Snake | ✔ 35 | ✔ 70 | ✘ | burrows | ✘ | strips reinforcement |
| Scorpion | ✔ 14 | ✘ | ✔ 10 | crumble | ✔ | feared, and skittish |

Two learnable rules fall out: *ice-family things cannot be frozen*, and *anything that burrows
cannot be pushed*. Both are consequences of the flag layout, not of extra code.

> **Hard caveat on the last three.** `Spider`, `Snake` and `Scorpion` are names in
> `GameCommon.h:148-150`, not guaranteed entries in the user's `Lego.cfg RockMonsterTypes` block.
> `Lego_GetObjectByName` is **EXE** (`Game.h:1511`) and this project has never read a real
> `Lego.cfg`. If those names do not resolve, the §4 mechanism warns and skips, exactly as
> `_ResolveSpeciesPool` already does (`DeepCore.cpp:96-99`). Whether they resolve is
> **UNDETERMINED** and is the same open experiment recorded in `HANDOFF-2026-07-30.md` §6.

---

## 4. HOW IT SHIPS

### 4.1 `-cfgadd`, investigated — and why it is the wrong channel here

The option exists and works. `Main.cpp:1371-1384` collects every `-cfgadd <file>` into
`mainOptions.configAppends` (`Main.h:307`); `GameState.cpp:194-201` loads each one with
`Config_Load2(..., FILE_FLAG_DATAPRIORITY)` and calls `Config_AppendConfig`
(`Config.cpp:834-848`), which walks to the tail of the root chain and links the new root on the
end.

**Do scalar keys override? Yes — last one wins.** This is not obvious and is worth deriving,
because it is the opposite of what "first match" would suggest. `Config_FindItem`
(`Config.cpp:732-810`) walks `linkNext` from the root and, on a match, sets `foundConf` and
`break`s **only if the match was not a wildcard** (`Config.cpp:790-793`). The `wildcard` flag is
re-assigned by every `Config_MatchItemName` call, including the parent walk
(`Config.cpp:778`), so its final value is the one from the **depth-0 parent**. The root item in a
stock config is `Lego*`, and `-gamename` is documented as always starting with `Lego`
(`Main.cpp:1246-1247`), so the depth-0 match is a **wildcard** match (`Config.cpp:713-724`) — the
loop therefore never breaks, runs to the end of the chain, and returns the **last** match.
Appended = later = **wins**. Confirmed for the `Lego*` case; a config whose root is a literal
full-match name also ends up letting the appended file win, by the same code taking the other
branch.

**Do arrays merge? No — and the failure mode is severe.** `Config_FindArray`
(`Config.cpp:337-349`) is built on the same `Config_FindItem`, so it also returns the **last**
matching block, and then hands back that block's first child. `Config_GetNextItem`
(`Config.cpp:352-364`) stops the moment depth drops below the starting depth. So the iteration in
`Stats_Initialise` (`Stats.cpp:134`) enumerates **only the appended file's `Stats` entries**; the
original block is never reached.

Everything not restated in the overlay is then **never allocated**:
`statsGlobs.objectStats[type]` stays `nullptr` (`Stats.cpp:174-181`), and
`Stats_GetStatsFlags1(objType, objID)` (`Stats.cpp:1154`) dereferences it. That is a null
dereference on the first non-overlaid object, not a graceful degradation. `Weapon_Initialise`
uses the identical `Config_FindArray` idiom twice (`Weapons.cpp:47, 108`) and has the same
exposure.

Derived from source, **not observed**. But the asymmetry is decisive: `-cfgadd` is safe for
scalar keys in blocks read by path (`Lego*::Main::...`) and is a **loaded gun** for any block read
by array iteration, which `Stats` is.

There is a second, softer reason to refuse it: an overlay that *did* restate the whole `Stats`
block would be a redistribution of the retail game's data. This project does not ship game data
(CI enforces the binary half of that rule via the `No_game_binaries` job,
`HANDOFF-2026-07-30.md` §1).

### 4.2 The mechanism that ships: a post-pass in `Stats_Initialise`

`Stats_Initialise` is **OURS** (`Stats.cpp:124-882`) and **HOOKED** (`interop.cpp:4096`).
`DeepCore::Load()` runs at `GameState.cpp:146`, long before `Stats_Initialise` at
`GameState.cpp:566`, so settings are available. `Stats_Initialise` runs **once per process**, not
per mission, so a delta applies for the whole session and never needs re-applying.

The insertion is one call immediately before `return true;` (`Stats.cpp:881`):

```cpp
    Stats_AddToolTaskType(LegoObject_ToolType_Spanner, AITask_Type_Repair);

    /// DEEPCORE: Apply the species stat overlay from Settings\DeepCore.cfg.
    /// Runs AFTER the vanilla parse so it can only ever modify entries the user's own
    /// Lego.cfg actually declared -- an unknown species name is reported and skipped,
    /// never allocated. No-op unless StatsOverhaul is on. See docs/research/stats-overhaul.md.
    DeepCore::ApplyStatsOverhaul();

    return true;
}
```

Why this is the right shape:

- **Additive, never replacing.** Absent keys keep whatever the user's `Lego.cfg` said. `-cfgadd`
  and `-cfgfile` both have the opposite property, which is what makes them dangerous here.
- **No `Lego.cfg` edit, no game data redistributed.** `Settings\DeepCore.cfg` is this project's own
  file, already copied into `$(OutDir)Settings\` by the build (`openlrr.vcxproj:118,148`).
- **One boolean from vanilla**, matching the existing gate discipline (`DeepCore.hpp:3-9`).
- **No new struct storage.** Deltas are `std::vector` members of `DeepCore::Settings`, which is
  DLL-side and carries no `assert_sizeof`. `ObjectStats` is not touched — the cardinal rule holds
  trivially, because we only *write existing fields*.

Sketch (**OURS**, new code in `DeepCore.cpp`; `ObjectStats` and `statsGlobs` are exe-overlaid
references, `Stats.cpp:41`):

```cpp
// DeepCore.cpp -- pointer-to-member bindings, so a struct change is a compile error,
// never a silent offset drift.
namespace {
    using LegoRR::ObjectStats;

    struct RealBinding { const char* key; real32 ObjectStats::* field; };
    struct IntBinding  { const char* key; sint32 ObjectStats::* field; };
    struct Flag1Binding { const char* key; LegoRR::StatsFlags1 bit; };
    struct Flag2Binding { const char* key; LegoRR::StatsFlags2 bit; };
    struct Flag3Binding { const char* key; LegoRR::StatsFlags3 bit; };

    const RealBinding kReals[] = {
        { "RouteSpeed",      &ObjectStats::RouteSpeed      },
        { "CollRadius",      &ObjectStats::CollRadius      },
        { "CollHeight",      &ObjectStats::CollHeight      },
        { "AlertRadius",     &ObjectStats::AlertRadius     },
        { "WakeRadius",      &ObjectStats::WakeRadius      },
        { "StampRadius",     &ObjectStats::StampRadius     },
        { "AttackRadius",    &ObjectStats::AttackRadius    },
        { "PainThreshold",   &ObjectStats::PainThreshold   },
        { "PickSphere",      &ObjectStats::PickSphere      },
        { "TrackDist",       &ObjectStats::TrackDist       },
        { "HealthDecayRate", &ObjectStats::HealthDecayRate },
        { "LaserDamage",     &ObjectStats::LaserDamage     },
        { "PusherDamage",    &ObjectStats::PusherDamage    },
        { "PusherDist",      &ObjectStats::PusherDist      },
        { "FreezerDamage",   &ObjectStats::FreezerDamage   },
        { "FreezerTime",     &ObjectStats::FreezerTime     },
        { "AwarenessRange",  &ObjectStats::AwarenessRange  },
        { "RubbleCoef",      &ObjectStats::RubbleCoef      },
        { "PathCoef",        &ObjectStats::PathCoef        },
        { "RandomMoveTime",  &ObjectStats::RandomMoveTime  },
        { "Flocks_Speed",    &ObjectStats::Flocks_Speed    },
        { "Flocks_Turn",     &ObjectStats::Flocks_Turn     },
        { "Flocks_Height",   &ObjectStats::Flocks_Height   },
        { "Flocks_Randomness", &ObjectStats::Flocks_Randomness },
    };

    const IntBinding kInts[] = {
        { "Capacity",   &ObjectStats::Capacity   },
        { "MaxCarry",   &ObjectStats::MaxCarry   },
        { "Flocks_Size", &ObjectStats::Flocks_Size },
    };

    const Flag2Binding kFlags2[] = {
        { "CanLaser",               LegoRR::STATS2_CANLASER               },
        { "CanPush",                LegoRR::STATS2_CANPUSH                },
        { "CanFreeze",              LegoRR::STATS2_CANFREEZE              },
        { "CanBeShotAt",            LegoRR::STATS2_CANBESHOTAT            },
        { "CanBeHitByFence",        LegoRR::STATS2_CANBEHITBYFENCE        },
        { "SplitOnZeroHealth",      LegoRR::STATS2_SPLITONZEROHEALTH      },
        { "UseHoles",               LegoRR::STATS2_USEHOLES               },
        { "RemoveReinforcement",    LegoRR::STATS2_REMOVEREINFORCEMENT    },
        { "DamageCausesCallToArms", LegoRR::STATS2_DAMAGECAUSESCALLTOARMS },
        { "DontShowDamage",         LegoRR::STATS2_DONTSHOWDAMAGE         },
        { "DontShowOnRadar",        LegoRR::STATS2_DONTSHOWONRADAR        },
        { "CrossLava",              LegoRR::STATS2_CROSSLAVA              },
    };
    // kFlags1: CauseSlip, CanScare, ScaredByPlayer, CanScareScorpion, GrabMinifigure,
    //          CanSteal, RouteAvoidance, RandomMove, RandomEnterWall, Flocks, Flocks_Debug
    // kFlags3: ShowHealthBar
}

void DeepCore::ApplyStatsOverhaul(void)
{
    // Lego_GetObjectByName is an exe address macro whose expansion names LegoRR types
    // UNQUALIFIED, and macros are not namespace members -- so this using-directive is
    // load-bearing, exactly as in _ResolveSpeciesPool (DeepCore.cpp:86).
    using namespace LegoRR;

    if (!settings.statsOverhaul || settings.statDeltas.empty())
        return;

    for (const Settings::StatDelta& d : settings.statDeltas) {

        LegoObject_Type type = LegoObject_Type::LegoObject_None;
        LegoObject_ID   id   = (LegoObject_ID)0;

        if (!Lego_GetObjectByName(d.objectName.c_str(), &type, &id, nullptr)) {
            DeepCore_WarnF(true, "StatDeltas \"%s\": no object named \"%s\"; skipped.",
                d.label.c_str(), d.objectName.c_str());
            continue;
        }

        // Same bounds contract as Stats_Initialise (Stats.cpp:162-172): statsGlobs is
        // overlaid on the exe's data segment at 0x00503bd8 and pinned by
        // assert_sizeof(Stats_Globs, 0x5b0). An out-of-range index is a corrupting write,
        // so we complain AND skip -- the continue is what actually prevents it.
        if ((uint32)type >= (uint32)LegoObject_Type_Count ||
            (uint32)id   >= (uint32)LegoObject_ID_Count)
        {
            DeepCore_WarnF(true, "StatDeltas \"%s\": out-of-range indices (type %i, id %i); skipped.",
                d.label.c_str(), (sint32)type, (sint32)id);
            continue;
        }

        // Never allocate. If Lego.cfg never declared Stats for this object, there is
        // nothing to overlay and inventing an entry would change more than we can verify.
        if (statsGlobs.objectStats[type] == nullptr ||
            statsGlobs.objectStats[type][id] == nullptr)
        {
            DeepCore_WarnF(true, "StatDeltas \"%s\": \"%s\" has no Stats block in Lego.cfg; skipped.",
                d.label.c_str(), d.objectName.c_str());
            continue;
        }

        const uint32 levels = statsGlobs.objectLevels[type][id];
        for (uint32 lvl = 0; lvl < levels; lvl++) {
            _ApplyDelta(statsGlobs.objectStats[type][id][lvl], d);
        }

        if (settings.verboseStartup) {
            DeepCore_LogF("StatDeltas %s: %s.%s = %f (%i levels)",
                d.label.c_str(), d.objectName.c_str(), d.key.c_str(), d.value, (sint32)levels);
        }
    }

    _AuditStatsOverhaul(); // §5.2 invariants, after everything has landed.
}
```

`_ApplyDelta` is a linear scan of the four binding tables plus the clamps from §5.2; it writes
`stats.*binding->field = d.value` for reals, `(sint32)d.value` for ints, and
`stats.flags2 |= bit` / `&= ~bit` for flags. Parsing follows the `Variants` block precedent
exactly (`DeepCore.cpp:427-466`): `Config_FindArray` + `Config_GetNextItem` over
`DeepCore_ID("StatDeltas")`, then `_SplitFields` on the data string.

### 4.3 Sample overlay fragment

Appended to the shipped `data/Settings/DeepCore.cfg` (which is copied to
`$(OutDir)Settings\DeepCore.cfg` by `openlrr.vcxproj:118,148`):

```
Lego* {

	DeepCore {

		; Apply the species stat overlay. FALSE (default) = your Lego.cfg, untouched.
		StatsOverhaul					TRUE

		; One delta per line:   <label>   <ObjectName> <Key> <Value>
		;
		; <label>     free text, diagnostics only -- keep it unique so log lines are greppable
		; <ObjectName> a name from YOUR Lego.cfg RockMonsterTypes block. A name that does not
		;              resolve is REPORTED AND SKIPPED, never guessed at.
		; <Key>       any key from the table in docs/research/stats-overhaul.md
		; <Value>     a number, or TRUE/FALSE for flag keys
		;
		; Keys you do NOT list are left exactly as your Lego.cfg set them. This block never
		; replaces your Stats block -- it edits the parsed result afterwards.

		StatDeltas {

			; ---- RockMonster: the wall that walks -------------------------
			rm_speed        RockMonster  RouteSpeed              0.85
			rm_grab         RockMonster  GrabMinifigure          TRUE
			rm_stamp        RockMonster  StampRadius             12
			rm_pain         RockMonster  PainThreshold           30
			rm_laser        RockMonster  LaserDamage             10
			rm_push         RockMonster  PusherDist              15
			rm_freezetime   RockMonster  FreezerTime             8
			rm_split        RockMonster  SplitOnZeroHealth       TRUE
			rm_bar          RockMonster  ShowHealthBar           TRUE
			rm_cta          RockMonster  DamageCausesCallToArms  TRUE
			rm_track        RockMonster  TrackDist               90
			rm_scorpion     RockMonster  CanScareScorpion        TRUE
			rm_alert        RockMonster  AlertRadius             22

			; ---- IceMonster: the one you must not freeze -------------------
			im_speed        IceMonster   RouteSpeed              1.30
			im_nofreeze     IceMonster   CanFreeze               FALSE
			im_push         IceMonster   PusherDist              45
			im_pushdmg      IceMonster   PusherDamage            6
			im_laser        IceMonster   LaserDamage             16
			im_split        IceMonster   SplitOnZeroHealth       TRUE
			im_wake         IceMonster   WakeRadius              30

			; ---- LavaMonster: the one on a fuse ----------------------------
			lm_decay        LavaMonster  HealthDecayRate         0.25
			lm_freezetime   LavaMonster  FreezerTime             20
			lm_freezedmg    LavaMonster  FreezerDamage           12
			lm_laser        LavaMonster  LaserDamage             2
			lm_nopush       LavaMonster  CanPush                 FALSE
			lm_fence        LavaMonster  CanBeHitByFence         TRUE
			lm_lava         LavaMonster  CrossLava               TRUE
			lm_stamp        LavaMonster  StampRadius             14

			; ---- TinyRM: the one that trips you ----------------------------
			trm_slip        TinyRM       CauseSlip               TRUE
			trm_speed       TinyRM       RouteSpeed              1.50
			trm_avoid       TinyRM       RouteAvoidance          TRUE
			trm_nobar       TinyRM       ShowHealthBar           FALSE
			trm_nodmg       TinyRM       DontShowDamage          TRUE
			trm_laser       TinyRM       LaserDamage             40
			trm_push        TinyRM       PusherDist              60
			trm_nofreeze    TinyRM       CanFreeze               FALSE
			trm_wake        TinyRM       WakeRadius              12

			; ---- Slug: the parasite ---------------------------------------
			sl_steal        Slug         CanSteal                TRUE
			sl_cap          Slug         Capacity                2
			sl_holes        Slug         UseHoles                TRUE
			sl_speed        Slug         RouteSpeed              0.60
			sl_nobar        Slug         ShowHealthBar           FALSE
			sl_freezetime   Slug         FreezerTime             15
			sl_nopush       Slug         CanPush                 FALSE
			sl_wake         Slug         WakeRadius              40

			; ---- Unused species: harmless if your Lego.cfg lacks them ------
			; Every line below is reported and skipped unless the name resolves.
			sn_rein         Snake        RemoveReinforcement     TRUE
			sn_holes        Snake        UseHoles                TRUE
			sn_speed        Snake        RouteSpeed              1.60
			sc_scared       Scorpion     ScaredByPlayer          TRUE
			sc_alert        Scorpion     AlertRadius             26
			sc_nopush       Scorpion     CanPush                 FALSE
			sp_stamp        Spider       StampRadius             18
			sp_pain         Spider       PainThreshold           60
		}
	}
}
```

### 4.4 The `-cfgadd` channel, documented honestly for power users

It remains the right tool for a user who wants to override **scalar** keys and who owns their own
`Lego.cfg`:

```
; overlay.cfg -- place in the Data directory, run:  OpenLRR.exe -cfgadd overlay.cfg
Lego* {
	Main {
		; last-wins: this beats the same key in Lego.cfg (Config.cpp:790-793)
		MinEnergyForEat		30
	}
}
```

**Never put a `Stats` or `WeaponTypes` block in a `-cfgadd` file** — see §4.1. If a future session
wants `-cfgadd` to be safe for arrays, the fix is a real one and belongs in `Config_AppendConfig`:
splice appended array *children* into the matching existing block rather than linking a second
block at the tail. That is a change to `Config.cpp` with 100+ callers and is out of scope here.

---

## 5. RISKS, AND THE SAFEST ORDERING

### 5.1 The five ways to make a level unwinnable

| # | Mistake | Mechanism | Consequence |
| --- | --- | --- | --- |
| R1 | `PainThreshold >= 100` | `LegoObject_CanShootObject` requires `health > PainThreshold` (`Object.cpp:857`); monsters start at 100 health (`Object.cpp:878`) | Raiders never target the species. A "destroy all monsters" objective can never complete |
| R2 | `CanBeShotAt FALSE` on a species an objective counts | `Object.cpp:856`, evaluated at spawn (`Object.cpp:985`) | Same as R1. This is *intended* for Bat and SmallSpider — which is exactly why it needs an audit, not a ban |
| R3 | `CanLaser`, `CanPush` and `CanFreeze` all FALSE | `Weapons.cpp:452-486` — every known-beam branch is gated on its flag | Invulnerable to every beam in the game |
| R4 | `CanSteal TRUE` with `Capacity 0` | `Object.cpp:2114` only enforces a cap when `capacity != 0` | Uncapped stealing. A crystal-collection objective becomes unwinnable |
| R5 | `WakeRadius 0` | `Object.cpp:5511-5516`; monsters spawn `LIVEOBJ3_POWEROFF` (`Object.cpp:990`). The parser's `==0 ⇒ 20.0` default (`Stats.cpp:842`) does **not** protect the post-pass, which writes the field directly | Monsters never wake. Not unwinnable, but it silently deletes the threat — the worst kind of bug, because it looks like nothing happened |

**These are not left to discipline.** `_AuditStatsOverhaul()` runs after every delta has landed and
clamps or refuses:

```cpp
static void _AuditStatsOverhaul(void)
{
    using namespace LegoRR;
    for (uint32 type = 0; type < (uint32)LegoObject_Type_Count; type++) {
        if (statsGlobs.objectStats[type] == nullptr) continue;
        for (uint32 id = 0; id < (uint32)LegoObject_ID_Count; id++) {
            if (statsGlobs.objectStats[type][id] == nullptr) continue;
            const uint32 levels = statsGlobs.objectLevels[type][id];
            for (uint32 lvl = 0; lvl < levels; lvl++) {
                ObjectStats& s = statsGlobs.objectStats[type][id][lvl];

                // R1
                if (s.PainThreshold >= 100.0f) {
                    DeepCore_WarnF(true, "StatDeltas: PainThreshold %f >= 100 would make this "
                        "object permanently unshootable; clamped to 99.", s.PainThreshold);
                    s.PainThreshold = 99.0f;
                }
                // R3
                if (!(s.flags2 & (STATS2_CANLASER|STATS2_CANPUSH|STATS2_CANFREEZE))) {
                    DeepCore_WarnF(true, "%s", "StatDeltas: all three of CanLaser/CanPush/CanFreeze "
                        "are off; that object is immune to every beam. Restoring CanLaser.");
                    s.flags2 |= STATS2_CANLASER;
                }
                // R4
                if ((s.flags1 & STATS1_CANSTEAL) && s.Capacity < 1) {
                    DeepCore_WarnF(true, "%s", "StatDeltas: CanSteal with Capacity 0 means "
                        "uncapped stealing; raising Capacity to 1.");
                    s.Capacity = 1;
                }
                // R5
                if (s.WakeRadius <= 0.0f) {
                    DeepCore_WarnF(true, "%s", "StatDeltas: WakeRadius 0 means the object never "
                        "wakes; restoring the engine default of 20.");
                    s.WakeRadius = 20.0f;
                }
            }
        }
    }
}
```

R2 cannot be enforced in code — whether a mission's objective needs a given species dead lives in
its NERPs script. It gets a **documented rule** instead: `CanBeShotAt FALSE` is permitted only for
Bat and SmallSpider, and `docs/` must say so next to the setting.

### 5.2 Lower-severity risks

- **`HealthDecayRate` on the wrong object.** The field is read for *every* object type
  (`Object.cpp:3929-3933`), not just monsters. A typo'd object name that happens to resolve to a
  building puts that building on a death timer. Mitigation: the post-pass logs every applied delta
  under `VerboseStartup`, and `HealthDecayRate` should additionally warn whenever the resolved
  `type != LegoObject_RockMonster`.
- **`HealthDecayRate` units are UNDETERMINED.** The formula is `rate / STANDARD_FRAMERATE * elapsed`
  with `STANDARD_FRAMERATE == 25.0f` (`common.h:109`). *If* `elapsed` is in 25-per-second frame
  units, `rate` is HP/second and 0.25 gives a ~400-second fuse. That conditional is doing real work
  and nobody here can discharge it.
- **`CauseSlip` kills the monster** (`Object.cpp:5430-5431`). Whether that death counts toward a
  kill quota is **UNDETERMINED** (`RewardQuota_*` calls sit in `LegoObject_AddDamage2`,
  `Object.cpp:3874-3892`, and this path bypasses them by setting `health` directly).
- **`SplitOnZeroHealth` on a model with no split parts.** `LegoObject_StartCrumbling` is
  OURS+HOOKED (`interop.cpp:3677`) but what it does with a model lacking the expected geometry is
  **UNDETERMINED**. Only apply it to species that already have it in stock data.
- **`CollRadius` inflation.** It is read by routing, collision, weapons and the bump-damage body
  (`Object.cpp:5569-5570`). Large values risk pathing failures. Keep deltas inside ±30%.
- **`CollHeight 0`** puts beam aim, health bars and damage numbers at the object's feet
  (`Weapons.cpp:1320`, `Bubbles.cpp:403`).
- **`Flocks TRUE` on a non-flocking species.** `Object.cpp:977` calls
  `LegoObject_Flocks_Initialise`, which is **EXE**, and `Object.cpp:5471` dereferences
  `monsterObj->flocks` unconditionally inside the `FLOCKS` branch. Do not add `Flocks` to any
  species that does not already have it.
- **Global blast radius.** `Stats_Initialise` runs once at startup for the whole session
  (`GameState.cpp:566`), so a bad delta affects every mission, including ones already in progress
  in a save. There is no per-level scoping and this design does not add one.
- **`STATS1_COLLRADIUS` / `STATS1_COLLBOX` are derived, not authored.** Setting `CollRadius` via
  the post-pass does **not** set the `COLLRADIUS` bit (that only happens at parse time,
  `Stats.cpp:327`). Nothing outside `Stats.cpp` reads `COLLRADIUS`, but `Weapons.cpp:752` does read
  `COLLBOX` — so never write `CollRadius` on an object whose stock config used `CollBox`.

### 5.3 Safest ordering — strictly increasing blast radius

| Phase | Contents | Worst case if wrong |
| --- | --- | --- |
| **0** | Mechanism only: gate off, empty `StatDeltas`, `VerboseStartup` logging | nothing changes |
| **1** | Presentation: `DontShowOnRadar`, `DontShowDamage`, `ShowHealthBar`, `TrackDist`, `PickSphere`, sounds | ugly, never unwinnable |
| **2** | Motion: `RouteSpeed`, `RouteAvoidance`, `RandomMoveTime`, small `CollRadius`/`CollHeight` deltas | monsters feel wrong; pathing degrades |
| **3** | Perception: `AlertRadius`, `WakeRadius`, `AwarenessRange`, `CanScare`, `ScaredByPlayer`, `CanScareScorpion` | too easy or too hard |
| **4** | Damage *numbers* only, all three beam flags left TRUE: `LaserDamage`, `PusherDamage`, `PusherDist`, `FreezerDamage`, `FreezerTime` | balance only |
| **5** | Immunities: turning **one** beam flag off per species | first phase that can soft-lock; R3 audit is live from Phase 0 |
| **6** | Death behaviour: `SplitOnZeroHealth`, `UseHoles`, `RemoveReinforcement`, `HealthDecayRate` | levels end wrong |
| **7** | Contact behaviour: `GrabMinifigure`, `CauseSlip`, `StampRadius`, `PainThreshold`, `CanSteal` + `Capacity`, `CanBeShotAt FALSE` | the two objective-breaking risks, R2 and R4 |

Phases 1–4 alone already deliver most of §3.12's *readability*: distinct silhouettes on the radar,
distinct camera framing, distinct speeds and distinct damage feel, with **no way to break a
mission**. That is a genuinely shippable milestone.

---

## 6. DECISION

**Build it as a gated post-pass in `Stats_Initialise` fed by `Settings\DeepCore.cfg`. Do not use
`-cfgadd` for the `Stats` block; document it as unsafe for arrays.**

Ranked build order — each step compiles and is independently revertible, and the build contract
(`Debug|x86` + `Release|x86`, v142, 0 errors / exactly 44 warnings) must hold at every step:

1. **`DeepCore.hpp`** — add `bool statsOverhaul = false;`, `struct StatDelta { std::string label,
   objectName, key; real32 value; }`, `std::vector<StatDelta> statDeltas;`, and declare
   `void ApplyStatsOverhaul(void);`. Add `statsOverhaul` to `IsAnyFeatureEnabled()`. **Adds no
   behaviour**, so it can land and be verified on its own.
2. **`DeepCore.cpp`** — parse `StatsOverhaul` and the `StatDeltas` array, reusing the `Variants`
   walk verbatim (`DeepCore.cpp:427-466`). Implement `ApplyStatsOverhaul` + `_ApplyDelta` +
   `_AuditStatsOverhaul` with §5.1's clamps live from the first commit. Still inert: the shipped
   config has the gate FALSE and the block empty.
3. **`Stats.cpp:881`** — one call before `return true;`. This is the only edit to an existing
   hooked function, and it is additive: it runs after the vanilla parse completes and cannot change
   what the parse did.
4. **`data/Settings/DeepCore.cfg`** — ship the block **commented out**, with Phase-1 deltas only
   (presentation), and the full §3 table as comments. Nothing changes until a user deletes a `;`.
5. **Phase 2–4 deltas**, one phase per commit.
6. **Phase 5–7 deltas**, one *species* per commit, so a bisect names the species.
7. **Optional follow-up, separate decision:** per-species bump damage. The gate is exe but the body
   is ours (`Object.cpp:5556-5577`) and it hardcodes 10 damage; reading a stat there instead is a
   ~5-line change to code we already own.

**The exact first file to write: `src/openlrr/game/DeepCore.hpp`.**

It is already in the project (`openlrr.vcxproj:258`), so it needs no `.vcxproj` edit; it is
header-only declarations, so it cannot change runtime behaviour; and both step 2 and step 3 depend
on it. It is the largest reduction in uncertainty available for the smallest possible diff.

**What this document cannot tell you.** Whether any of it is *fun*. Whether `Spider`, `Snake` and
`Scorpion` resolve at all. Whether `HealthDecayRate`'s units are HP/second. Whether
`SplitOnZeroHealth` is safe on every model. Those need someone with the game installed, and the
cheapest way to find out is Phase 0 plus `VerboseStartup TRUE`, reading the log rather than
watching the screen — the failures here are silent by design.
