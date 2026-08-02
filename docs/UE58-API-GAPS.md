# UE 5.8 API Catalog — Gap Analysis

> Companion to [`UE58-API-CATALOG.md`](UE58-API-CATALOG.md). Produced by a
> completeness critic run over all eight verified domain surveys, tasked with
> finding what the survey *missed* rather than summarising what it found.
>
> This document is meant to shrink. Delete entries as they get answered.

# Completeness Audit — What the Eight Legs Missed

The eight surveys are strong on *rendering primitives* and weak on *game systems*. Roughly half of the actual feature list in the brief has no API answer anywhere in the corpus. Below, prioritized, every module name verified against the tree.

---

## P0 — Architectural decisions being made blind

### GAP 1: No leg covered `SignificanceManager`, and "LOD strategy for a fully procedural world" was never answered
**Missing:** The engine's built-in framework for scoring objects by importance and driving tick-rate/LOD/effect decisions off that score.
**Why it matters:** This is the single biggest omission relative to the brief. DeepCore has no Nanite (runtime-built geometry can't have it), no mesh distance fields, no HLOD (needs cooked proxies), and no World Partition. Every automatic LOD system in the engine is closed to this project. `SignificanceManager` is the *only* remaining engine-supported answer, and it works entirely on runtime C++ objects with zero assets. The agents-ai leg recommended "start with plain AActor units, migrate to Mass when profiling binds" without noting that significance-driven throttling is the cheap middle step that usually removes the need to migrate at all.
**Investigate:** `Plugins/Runtime/SignificanceManager` — `USignificanceManager::RegisterObject(UObject*, FName Tag, FManagedObjectSignificanceFunction, EPostSignificanceType, FManagedObjectPostSignificanceFunction)` at `Source/SignificanceManager/Public/SignificanceManager.h:121`. `UCLASS(MinimalAPI, config=Engine, defaultconfig)`, module Type `Runtime`, `IsBetaVersion: false`, `IsExperimentalVersion: false`, `EnabledByDefault: false` (one `.uproject` line). Note `EPostSignificanceType::Concurrent` lets the post-work run in parallel. The header explicitly warns each `RegisterObject` needs a matching `UnregisterObject` or you get a dangling reference and an eventual crash.
Pair with `Plugins/Runtime/AnimationBudgetAllocator` (Runtime, non-beta, non-experimental) if units ever get skeletal meshes.

### GAP 2: Determinism and seeding — the brief asked, no leg answered
**Missing:** Any treatment of reproducible generation. Zero mentions of `FRandomStream` across all eight legs.
**Why it matters:** This project's entire save/load story (engine-services leg) is "seed + deltas," its automation-test story is "same seed → byte-identical output," and its tuning-sweep workflow depends on comparing variants. All three silently assume determinism nobody has specified. `FMath::Rand()` / `FMath::FRand()` are global-state and *not* reproducible across frames or threads — and the runtime-geometry leg recommends building chunk meshes on worker tasks, which makes global RNG actively wrong.
**Investigate:** `Source/Runtime/Core/Public/Math/RandomStream.h:19` `FRandomStream` — `Initialize(int32)` at :63, `Initialize(FName)` at :75, `GetFraction()` at :115. Rule to establish now: one `FRandomStream` per chunk, seeded from `HashCombine(WorldSeed, ChunkCoord)`, never a shared stream across parallel work. Also unexamined: `FMath::PerlinNoise3D(const FVector&)` at `Source/Runtime/Core/Public/Math/UnrealMathUtility.h:2482` (plus 1D/2D at :2463/:2472) — CORE_API, already available, and worth diffing against whatever `DeepCoreTerrain.cpp` rolled by hand. Determinism caveat: floating-point noise is not guaranteed bit-identical across compilers/platforms; if saves must be portable, that needs a decision. `Source/Runtime/CorePreciseFP` exists and nobody looked at it.

### GAP 3: `LightWeightInstanceManager` is DEPRECATED in 5.8 — a 5.8-specific change nobody flagged
**Missing:** The destruction-physics and agents-ai legs both wrestled with "how do we represent thousands of cheap entities" and neither mentioned Light Weight Instances *or* its 5.8 replacement.
**Why it matters:** LWI is the obvious thing a developer will find when searching for "actor-lite entities," and in 5.8 it is a trap. `Source/Runtime/Engine/Classes/GameFramework/LightWeightInstanceManager.h:30` reads `UCLASS(BlueprintType, Blueprintable, Experimental, MinimalAPI, Deprecated, meta = (DeprecationMessage = "Deprecated in 5.8. Consider using InstancedActors for similar functionality"))`. Epic is pointing at `Plugins/Runtime/InstancedActors` — which is `IsExperimentalVersion: true`, so the honest answer is "both doors are shut, use ISM + your own handle table," but that conclusion should be *recorded* rather than rediscovered.
**Investigate:** `Source/Runtime/Engine/Classes/GameFramework/LightWeightInstanceSubsystem.h` (for the `FActorInstanceHandle` pattern, which is *not* deprecated and appears throughout the engine's hit/overlap APIs), and `Plugins/Runtime/InstancedActors`.

---

## P1 — Named feature areas with no API answer

### GAP 4: Audio volumes — the vfx-audio leg's dead end is wrong
**Missing:** `Plugins/AudioGameplayVolume`. The vfx-audio leg declared `AAudioVolume` a dead end ("derives from AVolume and depends on brush geometry, an editor-authored construct") and routed everything to tag-based global reverb via `ActivateReverbEffect`. That loses per-region acoustics — which is the entire art direction for a mining game.
**Why it matters:** This plugin provides exactly the brush-free path the leg concluded didn't exist. `Source/AudioGameplayVolume/Public/AudioGameplayVolumeProxy.h:28` defines an abstract `UAudioGameplayVolumeProxy` with `virtual bool ContainsPosition(const FVector&)` at :40. Two concrete subclasses matter: `UAGVPrimitiveComponentProxy` (:80, "Audio Listener in Primitives" — works off any `UPrimitiveComponent`, i.e. your existing chunk colliders) and `UAGVConditionProxy` (:107, "Arbitrary" — you supply the containment test, i.e. a direct query against the tile grid). Alongside them: `ReverbVolumeComponent.h`, `FilterVolumeComponent.h`, `AttenuationVolumeComponent.h`, `SubmixSendVolumeComponent.h`, `SubmixOverrideVolumeComponent.h`.
**Investigate:** `Plugins/AudioGameplayVolume` (`IsBetaVersion: true`, Runtime module, depends on the `AudioGameplay` plugin). This is the "tight tunnel vs. open cavern" acoustics system, driven from tile data with no volume actor and no asset.

### GAP 5: Camera control — zero coverage across eight legs
**Missing:** RTS-style camera was in the brief's feature list and appears in no survey.
**Why it matters:** Camera is a top-three feel problem for this genre and there are two competing engine answers with very different maturity, plus a hard constraint nobody checked (camera shakes and camera modifiers are `UObject` classes — asset-free — but `GameplayCameras` rigs are asset-driven).
**Investigate:** `Plugins/Cameras/EngineCameras` (the `UCameraShakeBase`/`UMatineeCameraShake` line) versus `Plugins/Cameras/GameplayCameras` — note the latter is `"EnabledByDefault": true` **and** `"IsExperimentalVersion": true`, an unusual combination meaning it is already in your build. Also unexamined: `APlayerCameraManager` camera modifiers (`UCameraModifier`, plain `UObject`, `NewObject`-able) in `Source/Runtime/Engine/Classes/Camera/`, and `Source/Runtime/CinematicCamera`.

### GAP 6: Movement — no answer for units, none at all for vehicles
**Missing:** The agents-ai leg recommended hand-rolled `AAIController` movement and explicitly noted "if you are pathing on your own tile grid, drive movement directly rather than via MoveTo." It never said what to drive movement *with*. The destruction-physics leg dismissed Chaos Vehicles as `UNCLEAR` and suggested "a simulating body plus your own suspension raycasts" — a substantial system with no engine reference.
**Why it matters:** "Drive movement directly" is a euphemism for writing a character/vehicle movement component. `UCharacterMovementComponent` is heavyweight and network-shaped; `UFloatingPawnMovement` and `UPawnMovementComponent` (`Source/Runtime/Engine/Classes/GameFramework/`) are the lightweight options nobody named.
**Investigate:** `Source/Runtime/Engine/Classes/GameFramework/FloatingPawnMovement.h` and `NavMovementComponent.h` first (asset-free, trivial). Then `Plugins/Experimental/Mover` (`IsExperimentalVersion: true`) — Epic's designated `UCharacterMovementComponent` replacement, with `Plugins/Experimental/ChaosMover` and `Plugins/Experimental/MoverExamples` alongside. Also `Plugins/Experimental/ChaosModularVehicle`, which is a better fit than `ChaosVehiclesPlugin` for a brick-built mining vehicle since it composes from modules rather than requiring a skeletal-mesh wheel rig.

### GAP 7: Water and lava hazards — not mentioned once
**Missing:** The brief lists water/lava hazards. No leg addressed fluid volumes, buoyancy, damage volumes, or surface rendering.
**Why it matters:** Likely a real dead end (the Water plugin is heavily asset- and landscape-oriented), but that verdict should be *recorded* rather than left as a hole someone spends a week rediscovering.
**Investigate:** `Plugins/Experimental/Water` (`EnabledByDefault: false`, `IsExperimentalVersion: true`) and `Plugins/Experimental/WaterAdvanced`, `Plugins/Experimental/Buoyancy`. Expect a negative result — water bodies are spline/landscape-driven actors. The asset-free substitute is probably `APhysicsVolume` (`Source/Runtime/Engine/Classes/GameFramework/PhysicsVolume.h`, has `bWaterVolume`, `TerminalVelocity`, `FluidFriction`) plus a PMC surface with the additive `EmissiveMeshMaterial` the materials leg found — but nobody has said so.

### GAP 8: Fog of war and minimap — no answer, and the obvious one is blocked
**Missing:** Both are in the brief. The vfx-audio leg mentioned `UCanvasRenderTarget2D` in passing as "an in-world minimap of the mined cave" but nobody connected it to the materials leg's hard finding that **you cannot author a material at runtime**.
**Why it matters:** Standard fog-of-war is "render visibility into an RT, sample it in the terrain material." The terrain material is `/Engine/EngineDebugMaterials/VertexColorMaterial` or `VertexPaint_4Tex`, neither of which samples an arbitrary RT with world-space UVs. So the conventional implementation is *closed*, and the fallback is per-vertex: bake visibility into `FProcMeshVertex::Color` and update via `UpdateMeshSection`. That is a materially different architecture and it needs to be decided before the chunk vertex format is frozen.
**Investigate:** `UCanvasRenderTarget2D` + `UKismetRenderingLibrary` for the *minimap* (a Slate `SImage` over an RT works fine — no material needed). For *fog of war on world geometry*, the vertex-color path, with the `CreateMeshSection_LinearColor` vs `UpdateMeshSection_LinearColor` sRGB default mismatch the materials leg found (`bSRGBConversion` defaults `false` vs `true`) as a live hazard.

### GAP 9: Day/night and power systems — unexamined
**Missing:** Both named in the brief, neither surveyed.
**Why it matters:** Day/night is arguably out of scope underground, but a *power grid* (lamps that go dark when a generator fails) is core to the genre and is the single best consumer of the lighting leg's per-light `VolumetricScatteringIntensity` and `SetIntensity` findings. Nobody connected them.
**Investigate:** For power, no engine system is needed — this is a graph over your own tile data, and the lighting leg's animate-`SetIntensity`-per-tick recommendation is the right renderer-side hook (light functions being a confirmed dead end). For day/night, `Plugins/Experimental/DaySequence` exists (`IsExperimentalVersion: true`, Runtime + Editor modules) and is asset-driven — record it as a dead end and move on.

### GAP 10: Task assignment and job scheduling — the actual AI problem, unanswered
**Missing:** The agents-ai leg answered *pathing* thoroughly (`FGraphAStar` — a good call) and *behavior authoring* honestly (StateTree dead, BT viable-but-unsupported). It never answered **who decides which miner does which job**, which it correctly identified in a dead-end note as "a scheduling/auction problem over a task queue" — and then dropped.
**Why it matters:** This is the central AI system of a mining game and it currently has no design and no engine anchor.
**Investigate:** `Source/Runtime/GameplayTasks` (`UGameplayTask`, `UGameplayTasksComponent`, `IGameplayTaskOwnerInterface`) — a Runtime engine module, no plugin, already a transitive dependency of AIModule, and completely unmentioned across all eight legs. It provides task lifecycle, priority and resource-locking (two tasks can't claim the same miner) without any asset. Also worth a look: `Plugins/Runtime/GameplayInteractions` and `Plugins/Runtime/GameplayBehaviorSmartObjects`, though both pull StateTree.

---

## P2 — Cross-cutting concerns the brief asked about

### GAP 11: Memory budgets — asked, entirely unaddressed
**Missing:** No leg discussed memory at all beyond the runtime-geometry leg's note that `FProcMeshVertex` is ~150 bytes/vertex with a permanent CPU copy per section.
**Why it matters:** That one observation is alarming on its own — a fully procedural brick world retains every vertex on the CPU forever, and the recommended migration to `UStaticMesh::BuildFromMeshDescriptions` adds a *second* copy (`bAllowCPUAccess = true` is required for runtime collision). Nobody has computed a per-chunk budget or named a measurement tool.
**Investigate:** `Source/Runtime/Core/Public/HAL/LowLevelMemTracker.h` (`LLM_SCOPE`, `DECLARE_LLM_MEMORY_STAT`) — run with `-LLM`. Also `Plugins/Performance/MemoryUsageQueries` and `Plugins/WorldMetrics` (both unexamined, both real directories). `stat memory`, `memreport -full`, and `FPlatformMemory::GetStats()`.

### GAP 12: Debug visualization for a content-free project — one system covered, the right one missed
**Missing:** The rendering-gpu leg found `ShaderPrint` (GPU-side) and the engine-services leg found Slate. Neither mentioned the gameplay-side debug framework.
**Why it matters:** A project that ships zero assets and verifies by frame capture needs strong runtime introspection, and `GameplayDebugger` is purpose-built: register a category, draw per-actor state on an apostrophe-key overlay, no asset, no UMG.
**Investigate:** `Source/Runtime/GameplayDebugger` — `GameplayDebuggerCategory.h`, `GameplayDebuggerAddonManager.h`. Critically, `Source/Runtime/GameplayDebugger/Public/GameplayDebugger.h:13-14` documents a **two-tier** compile: `WITH_GAMEPLAY_DEBUGGER` (full, absent in Shipping by default) versus `WITH_GAMEPLAY_DEBUGGER_CORE` (core only, you register categories yourself) — see `TargetRules.cs:431-436` for the Core/Full enum. That distinction is exactly the kind of silent-no-op this project keeps getting bitten by. Also `Plugins/Experimental/DrawDebugLibrary`.

### GAP 13: Iteration speed — asked, not answered
**Missing:** Hot-reload / Live Coding appears nowhere, despite the materials leg noting `RecompileShaders Changed` for shaders only.
**Why it matters:** A C++-only project with no assets lives or dies on C++ iteration time. There are known Live Coding hazards directly relevant here: the lighting leg recommends setting `ShadowCacheInvalidationBehavior` before `RegisterComponent`, and the rendering-gpu leg's `AddShaderSourceDirectoryMapping` has a `check()` on duplicate registration that will fire on a second `StartupModule` under Live Coding — that leg flagged it, but nobody generalized the concern.
**Investigate:** `Source/Programs/UnrealBuildTool` Live Coding settings, `Source/Runtime/Core/Public/Misc/HotReloadInterface.h`. Also `Plugins/Experimental/SlateIM` (`IsExperimentalVersion: true`, modules `SlateIM`/`SlateIMEngine`/`SlateIMInGame`) — immediate-mode Slate, which is dramatically faster to iterate on than retained `SCompoundWidget` for tuning panels and is a natural fit for a code-only project. Unmentioned by the engine-services leg.

### GAP 14: Keeping a content-free project testable — half-answered
**Missing:** The engine-services leg found the automation framework and the correct headless command line. It did not address the harder problem: how do you test *rendering* outcomes when the whole thesis is "verify by capturing a frame, never by reasoning"?
**Why it matters:** Three separate legs independently found silent-no-op rendering failures (PMC vanishing from the TLAS if RT wasn't enabled at proxy construction; HISM hardcoding `bVisibleInLumenScene = false`; VSM cache invalidation defeated by `bHasDeformableMesh`). Every one of those is invisible to a CPU-only automation test.
**Investigate:** `Source/Developer/ScreenShotComparison` + `Source/Developer/ScreenShotComparisonTools` for automated golden-image comparison. `FAutomationScreenshotOptions` in `Source/Runtime/Engine/Classes/Engine/`. Combined with the rendering-gpu leg's `RenderCaptureInterface::FScopedCapture`, this is how the project gets a regression net around its most dangerous failure class.

---

## P3 — Worth knowing, or worth explicitly closing

### GAP 15: PCG was never mentioned, and it is already enabled
**Why it matters:** `Plugins/PCG` has `"EnabledByDefault": true` and `"IsBetaVersion": false` — it is compiled into your build right now. It has a genuine runtime path (`EPCGComponentGenerationTrigger::GenerateAtRuntime` at `Source/PCG/Public/Components/PCGComponent.h:81`, plus a Runtime Generation Scheduler and `FPCGRuntimeGenerationRadii`). It is almost certainly a dead end here because `UPCGGraph` is an asset, but "the engine's flagship procedural-generation framework" being absent from a procedural-generation project's API catalog is a hole a reviewer will notice.
**Investigate:** `Plugins/PCG/Source/PCG/Public/Graph/` — confirm `UPCGGraph` cannot be constructed at runtime, record the verdict, close the door.

### GAP 16: Other unexamined modules worth a paragraph each
- **`Plugins/Runtime/ComputeFramework`** (`IsBetaVersion: true`) — the rendering-gpu leg recommended hand-rolled `FGlobalShader` + a `PostConfigInit` module for GPU procedural work. This is the engine's structured alternative and was never compared against.
- **`Source/Runtime/StateStream`** — new, undocumented, no leg touched it. `TransformStateStream.h`, `StateStreamManager.h`. Relevant to the game-thread→render-thread handoff the runtime-geometry leg cares about.
- **`Plugins/Runtime/Soundscape`** (`IsBetaVersion: true`) — procedural ambient audio placement. Exactly the "cave drips, distant machinery" problem the vfx-audio leg solved by hand with MetaSound graphs.
- **`Plugins/Runtime/AudioMotorSim`** (`IsExperimentalVersion: true`) — procedural engine audio for mining vehicles.
- **`Plugins/Runtime/CableComponent`** (`EnabledByDefault: true`, non-beta — already in your build) — runtime-simulated cables for tethers, power lines, winches. Zero assets.
- **`Source/Runtime/Foliage`** — `UFoliageInstancedStaticMeshComponent` / `UGrassInstancedStaticMeshComponent`. The runtime-geometry leg established HISM is a Lumen dead end (`bVisibleInLumenScene = false` hardcoded); these subclasses were never checked against that finding.
- **`Plugins/Experimental/StructUtils`**, **`Plugins/Runtime/DataRegistry`**, **`Plugins/Runtime/GameFeatures`** + **`ModularGameplay`** — data-definition patterns for a project with no data assets.
- **`Plugins/Runtime/CommonUI`** and **`ModelViewViewModel`** — the engine-services leg recommended raw Slate without comparing against these.
- **`Source/Runtime/AutoRTFM`** — transactional memory, UE-specific, entirely unexamined. Potentially relevant to speculative/undoable world edits (drill preview, build placement rollback).

### GAP 17: 5.8-specific deltas beyond what was found
The legs caught good ones (legacy GPU profiler removal, `GGPUFrameTime`, `stat gpu`, `r.RayTracing.Debug.*` → `.Visualize.*`, IMC storage split in 5.7, `SubscribeToPostProcessingPass` in 5.5). Two more surfaced in this audit and there are likely others: **`LightWeightInstanceManager` deprecated in 5.8** (GAP 3), and **`FRHIGPUMemoryReadback::Unlock` deprecated in 5.8** (caught by the rendering-gpu leg). Worth one systematic pass: `grep -rn "UE_DEPRECATED(5.8" Source/Runtime/Engine/Classes Source/Runtime/Engine/Public` to catch the rest before they're discovered one compile error at a time.

---

## Summary of the shape of the gap

The corpus is a **renderer** catalog wearing an **engine** catalog's title. Coverage by domain:

| Domain | Coverage |
|---|---|
| Geometry, lighting, materials, GPU, VFX | Excellent, adversarially verified |
| Physics, audio, input, subsystems, packaging | Good |
| Pathing | Good (one leg) |
| **Task assignment, movement, camera, selection** | **None** |
| **Water/lava, fog of war, minimap, power** | **None** |
| **Determinism, memory, LOD strategy, iteration** | **None** — all four were explicitly asked for |

The three highest-value single additions: **`SignificanceManager`** (the only LOD answer left standing), **`FRandomStream` seeding discipline** (three existing recommendations silently depend on it), and **`Plugins/AudioGameplayVolume`** (overturns a stated dead end and delivers the game's signature acoustic effect).