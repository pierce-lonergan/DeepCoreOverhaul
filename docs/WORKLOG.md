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
- **Fixed a memory-corruption bug in `Stats_Initialise`.** `type` and `id` came from
  `Lego_GetObjectByName` (exe code, `Game.h:1511`) and indexed fixed arrays inside the
  exe-overlaid `statsGlobs` with no bounds check whatsoever. Now checked once before any
  write, with the entry skipped and a `Config_FatalItemF` naming file, line and both indices.
  Unconditional, not gated — the behaviour replaced is corruption, so there is no vanilla
  semantics to preserve.
- **Fixed the same defect in `Weapon_Initialise`** (`Weapons.cpp:156`), where `objType`/`objID`
  fed both `Stats_GetLevels` (an out-of-bounds *read* of the overlaid `statsGlobs`) and an
  out-of-bounds *write* to `objectCoefs[20][15][16]` inside a heap-allocated `WeaponStats`.
- **Shipped per-weapon beam appearance.** Weapon types were already uncapped, but every laser
  rendered identically because the appearance was hardcoded in `Weapon_Lazer_Add`, so a bigger
  roster read as one weapon. New `Weapon_Lazer_AddStyled` carries the weapon identity;
  `Weapon_Lazer_Add` keeps its exact hooked signature (`interop.cpp:4369`) and forwards -1,
  so exe-originated lasers stay stock. Styles live DLL-side keyed by weapon index — no
  exe-overlaid struct touched.

### Tier 3 (raising `LegoObject_ID_Count`) is a proven dead end. Recorded so nobody retries it.

Full analysis in `docs/research/type-loader-reimplementation.md` §4. Two findings settle it:

1. **A 16th monster does not overflow — it aliases the Tool Store.** In a `[20][15]` table the
   linear index of `[3][15]` is `3*15 + 15 = 60`, which is `[4][0]`. `LegoObject_Building == 4`
   (`GameCommon.h:1075`), so every RockMonster ID-15 access silently reads and writes
   **Building ID 0's row** — the Tool Store in stock data. No crash, no warning, no
   `assert_sizeof` trip. Just a Tool Store whose stats drift as monsters spawn. This is a
   far nastier failure than "writes past the end", and it is exactly what the two bounds
   checks above now refuse.
2. **The price of a usable ID 15 is ~200 exe functions**, not a design change. A DLL-side
   mirror only works if every toucher of a table is C++. Of the six tables a monster ID
   reaches, three are ours — but `Dependencies` (0 of 11 implemented), `Interface` (3 of 99)
   and `AITask` (9 of 105) are not, and they are fed by `Lego_GetObjectByName`, which is exe
   code with seven exe callers. You cannot mirror a table whose writer is 1999 machine code,
   and there is no "safe" index to hand those consumers, because every index ≥ 15 aliases
   Building 0.

Also rejected, deliberately: **reimplementing `Lego_LoadRockMonsterTypes` in C++.** It unlocks
nothing — the free ID slots below 15 already work with no code at all, because
`Lego_GetObjectByName` and `Stats_Initialise` are count-driven — while carrying the one risk
this project cannot absorb: an unverifiable behavioural regression in startup code on a project
that cannot run the game. Every gain attributed to it (validation, loud errors, a load-time
hook) was available in C++ we already own, which is where it was built instead.

The ambition redirects, it does not shrink: variety comes from density, per-instance
differentiation, weapon identity, and level/campaign content — none of which is ID-capped.
