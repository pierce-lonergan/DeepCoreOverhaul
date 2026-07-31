# DeepCoreOverhaul ![app icon](resources/logo/icon.png)

> **This is a fan fork of [OpenLRR by trigger-segfault][Upstream].** Nearly all code here is
> upstream's work, and the full upstream commit history is preserved so authorship stays
> attributable. Upstream carries no license, so this fork claims no rights over the inherited
> code — please read **[NOTICE.md](NOTICE.md)** before using, forking, or contributing.
>
> **Status: it runs — the half of it that is ours.** Every feature is opt-in and **off by
> default**; see [`data/Settings/DeepCore.cfg`](data/Settings/DeepCore.cfg). The subsystems
> this project owns now execute in a standalone sandbox against a generated cavern, with no
> game and no copyrighted content. The 1999 executable itself still requires your own
> installed copy, and nothing here has been **play-tested** in it. See the claim tiers below,
> which this project keeps deliberately sharp.
>
> | Feature | Gate | What it does |
> | --- | --- | --- |
> | Multi-species emerges | `MultiSpeciesEmerge` | Different caverns in one mission emerge different species. Vanilla allows only one per mission. |
> | Wave director | `WaveDirector` | Escalating, budgeted, always-telegraphed waves the map does not know about. |
> | Threat audio | `ThreatAudio` | Generated voice and stings driven by real escalation state. Registers its own cues; no `Lego.cfg` edit. |
> | Creature variants | `CreatureVariants` | Per-instance scale and tint, so individuals differ visibly. No new models. |
> | Per-weapon beams | `WeaponBeamStyles` | Each weapon's laser looks like itself. |
> | Water relocation | `RelocateWaterTables` | Pool storage moved DLL-side; caps raised from 10/100 to 4096/65536. |
> | Water survival | `SurviveWaterOverflow` | Simpler stopgap: oversized maps load instead of crashing to desktop. |
> | Corruption fixes | *always on* | Eight memory-corruption bugs fixed. Never gated — the behaviour they replace is a bug, not a feature. |
>
> **What this project cannot do**, so nobody is misled: the engine allows a hard maximum of
> **15 object IDs per category**, and the base game already uses about 11 monsters, 12 vehicles
> and 10 buildings. That leaves roughly 4 monster slots, 3 vehicles and 1 buildable building.
> The limit is welded to the original 1999 executable's memory layout and **cannot be raised** —
> the reasoning, with addresses, is in [`docs/OVERHAUL-PLAN.md`](docs/OVERHAUL-PLAN.md).
> So this project chases **density and consequence**, not a longer unit list.
>
> Verified: `Debug|x86` and `Release|x86` both build with **0 errors** and no new warnings
> against the upstream baseline of 44, using the v142 toolset.
>
> This project contains **no game assets** and is not playable on its own. It loads alongside
> your own legally obtained copy of the original game.
>
> As upstream requires, the **L in "LRR" is never expanded** anywhere in this project.

## Run it

**Watch this project's systems run right now — no game required:**

```
scripts\DeepCoreSandbox.cmd
```

Or double-click **DeepCore Sandbox** on your desktop. It pulls the latest commit,
rebuilds if anything changed, and runs. In a container instead:

```
docker compose -f docker/docker-compose.yml run --rm view
```

That is the **sandbox**: it executes the wave director, the spawn-fairness rules and the
threat-audio decisions against a procedurally generated cavern. It is not the 1999 game
and does not contain any part of it.

**To run the actual game with these modifications**, you need your own installed copy:

```
scripts\run.ps1
```

That script detects an installation, builds, deploys the settings and generated audio,
launches, and tails the log. If it finds no installation it says so and stops — it never
downloads anything and never guesses a path.

### What "it runs" means here, precisely

This project keeps three claim tiers separate and never blurs them:

| Tier | Meaning | Status |
| --- | --- | --- |
| **compile-verified** | it builds | 0 errors, exactly 44 warnings, both configs |
| **sandbox-verified** | our code executed on generated data and behaved correctly | 40/40 seeds pass every invariant |
| **play-tested** | someone ran it in the real game | **nobody, yet** |

Sandbox-verified says nothing about the 1999 executable, nothing about Direct3D Retained
Mode, and nothing about how a wave feels in an actual mission.

## Upstream description

An open source re-implementation of [LEGO Rock Raiders (PC)][Wikipedia_LRR]. This is created by slowly implementing and replacing game functionality, while relying on the original executable and game assets for everything else.

OpenLRR is not associated with The LEGO Group or Data Design Interactive. When using the name "OpenLRR" within this project, the **L** must never be expanded (i.e. do not write "Open _LEGO_ Rock Raiders").


## Instructions

Running OpenLRR requires a working installation of LEGO Rock Raiders, and building in Visual Studio 2019.

* [Installing LEGO Rock Raiders][Wiki_RunningLRR]
    * [Your Master Guide to get LRR to run][Guide_Master]
* [Building and running OpenLRR][Wiki_RunningOpenLRR]


## Contributing

OpenLRR is still missing documentation to aid in contribution, but submittions and/or help is still encouraged.

* [Decompilation and research][Wiki_Decompiling]
* Implementing functions and modules in the **game/** folder.
* Improving and refactoring **engine/** code.
* Fixing bugs or implementing new features (please create an Issue if one doesn't exist).

Submitting decompiled functions for game code is a very involved process. Most game module functions, structures, and enums are still going through heavy refactoring and renaming, with most names not accurately describing their purpose. However, assigning more-accurate names is **not required**.



***

## See also

* [Rock Raiders United][Page_RRU]
* [RRU: Knowledge Base][Page_RRUKB]

### Other LRR projects

* [Manic Miners][Project_ManicMiners]
* [ProjectReversio/LegoRockRaiders][Project_ProjectReversio]

### Similar open source projects

* [OpenRCT2][Project_OpenRCT2]
* [OpenTTD][Project_OpenTTD]



<!-- REFERENCES -->

[Upstream]: <https://github.com/trigger-segfault/OpenLRR> "trigger-segfault/OpenLRR: the upstream project this fork is derived from"

[Page_RRU]: <https://rockraidersunited.com/> "Rock Raiders United"
[Page_RRUKB]: <https://kb.rockraidersunited.com/Main_Page> "Rock Raiders United: Knowledge Base"

[Project_ManicMiners]: <https://manicminers.baraklava.com/> "Manic Miners: The Rock Raiders remake"
[Project_RRX]: <https://rockraidersx.com/> "Rock Raiders X - Rock Raiders recreated - This site is gone!"
[Project_ProjectReversio]: <https://github.com/ProjectReversio/LegoRockRaiders> "ProjectReversio/LegoRockRaiders: Decompiling and implementing LRR from the top down"

[Project_OpenRCT2]: <https://github.com/OpenRCT2/OpenRCT2> "An open source re-implementation of RollerCoaster Tycoon 2"
[Project_OpenTTD]: <https://github.com/OpenTTD/OpenTTD> "An open source simulation game based upon Transport Tycoon Deluxe"

[Wikipedia_LRR]: <https://wikipedia.org/wiki/Lego_Rock_Raiders_%28video_game%29> "Wikipedia: LEGO Rock Raiders (PC)"
[Guide_Master]: <https://rockraidersunited.com/guides/your-master-guide-to-get-lrr-to-run-r12/> "Your Master Guide to get LRR to run"
[Guide_MusicFix]: <https://rockraidersunited.com/guides/lrr-music-without-cd-fix-r11/> "Rock Raiders Music without CD Fix"

[Wiki_RunningLRR]: <https://github.com/trigger-segfault/OpenLRR/wiki/Running-LEGO-Rock-Raiders> "OpenLRR Wiki: Running LEGO Rock Raiders"
[Wiki_RunningOpenLRR]: <https://github.com/trigger-segfault/OpenLRR/wiki/Running-LEGO-Rock-Raiders> "OpenLRR Wiki: Running OpenLRR"
[Wiki_Decompiling]: <https://github.com/trigger-segfault/OpenLRR/wiki/Decompiling-LEGO-Rock-Raiders> "OpenLRR Wiki: Decompiling LEGO Rock Raiders"
