# Work log

Append-only. Newest entries at the bottom. Every entry states what changed, what was
verified, and what remains unproven.

**Standing caveat on every entry below:** this project has no installation of the original
game available to it. Everything is **compile-verified, never play-tested**. Where an entry
says "verified", that means the build succeeded and/or a number was re-derived by hand from
source — it never means the behaviour was observed in a running game.

---

## 2026-07-29

- Forked `trigger-segfault/OpenLRR` into DeepCoreOverhaul. Full 352-commit upstream history
  preserved so authorship stays attributable via `git log`/`git blame`.
- `NOTICE.md`: recorded upstream provenance, the absence of an upstream license (a documented
  consequence of the Gods98 abandonware base, per `src/openlrr/engine/README.md:3`), the
  inherited "never expand the L" naming rule, and non-affiliation.
- **Removed two tracked executables.** `OpenLRR.exe` and `OpenLRR-d.exe` (737,280 B each) were
  not project-authored launchers — PE analysis showed each to be the original 1999 game
  executable with a 512-byte `.idata2` section appended importing `openlrr.dll!StartOpenLRR`:
  `ImageBase 0x400000`, entry `0x8f2c0`, `.text 0x9d0b5` (644 KB of original game code),
  plus the strings `"Rock Raiders"` and `"Lego.cfg"`. Purged from the tip **and from all 352
  commits**; verified 0 commits contained them afterwards; force-pushed. Backup bundle at
  `Documents/GitHub/DeepCoreOverhaul-prepurge-backup.bundle` (do not delete).
- Both post-build `xcopy` steps in `openlrr.vcxproj` now print guidance instead of failing
  with MSB3073 when no launcher is present.
- Shipped the settings layer: `src/openlrr/game/DeepCore.{hpp,cpp}` +
  `data/Settings/DeepCore.cfg`, loaded beside Shortcuts in `Lego_Initialise`. Every gate
  defaults **off = vanilla**.
- Shipped **multi-species emerges**. Vanilla passes `level->EmergeCreature` to every trigger
  (`Game.cpp:3059`), so a mission can only ever produce one species out of eleven
  (`GameCommon.h:137-150`). `DeepCore::PickEmergeSpecies` now selects per trigger, bounds-checked
  against both `legoGlobs.rockMonsterCount` and `LegoObject_ID_Count`.
- Shipped **per-instance creature variants** (scale + tint) in the `LegoObject_RockMonster`
  branch of `LegoObject_Create` (`Object.cpp:954`), applied after `Creature_Clone` so the shared
  template is never mutated.
- Shipped **water overflow survival**: six `Error_Fatal` process-kills in `world/Water.cpp`
  now degrade instead of terminating, gated `SurviveWaterOverflow`.
- Recorded recon output as `docs/OVERHAUL-PLAN.md` and `docs/HOOK-ARCHITECTURE.md`.

## 2026-07-30

- **Built the address-space linter** — `tools/addrlint/addrlint.py`. Parses every
  `assert_sizeof` (376 types found) and every overlaid binding of the form
  `T & name = *(T*)0xADDR;`, joins them, sorts by address, and detects overlaps and slack.
  Emits `docs/ADDRESS-MAP.md` and `docs/address-map.json`.
  Result on the current tree: **113 sized regions, 1 unsized, 0 overlaps.**
  This is the safety net that makes any future struct relocation provable rather than hoped-for.
- `tools/addrlint/test_addrlint.py`: 12 self-tests, all passing. One of them caught a genuine
  error in my own test expectation (a fully-contained region's overlap is its own size, not the
  distance to the outer region's end) — the linter was right, the test was wrong.
- **CORRECTED A LOAD-BEARING NUMBER.** Prior sessions recorded "8 bytes of slack" between
  `statsGlobs` and `textGlobs`. Re-derived by hand from the linter's output:

  ```
  statsGlobs  0x00503bd8 + 0x5b0        = 0x00504188   (Stats.cpp:41, Stats.h:232)
  g_Teleporter_BOOL_00504188            @ 0x00504188, 4 bytes  (Teleporter.cpp:17)
  textGlobs                             @ 0x00504190   (TextMessages.cpp:24)
  raw gap 8 bytes − 4 occupied = 4 BYTES ACTUALLY FREE
  ```

  So raising `LegoObject_ID_Count` 15→16 (+80 bytes to `Stats_Globs`) does not merely run 72
  bytes into `Text_Globs` — it **first destroys a live Teleporter flag**, then runs into
  `Text_Globs`. The conclusion is unchanged and strengthened; the stated slack was wrong.
  A regression test pins this exact layout (`test_scalar_inside_gap_is_counted`).
- **Fixed broken CI.** The inherited workflow did `Move-Item OpenLRR.exe` — a file deleted
  yesterday — so every run would have failed. Rewrote `.github/workflows/build_artifacts.yml`:
  dropped the game-executable staging, upgraded the deprecated action versions (v2/v3 → v4/v5),
  and added two gating jobs that must pass before either build runs:
  - `Address_space_lint` — self-tests, `--check`, and a staleness check on `docs/ADDRESS-MAP.md`.
  - `No_game_binaries` — fails if any `.exe`/`.dll`/`.wad` is ever tracked again. The hard line
    from the mandate is now enforced by machine rather than by memory.
- Wrote `docs/DIRECTORY.md` (full recursive inventory with per-module ownership: implemented
  C++ vs still-exe address macros).
