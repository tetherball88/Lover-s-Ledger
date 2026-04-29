## TT Lover's Ledger

A lightweight SKSE plugin + Papyrus library that records adult-framework (OStim and basic SexLab) activity and exposes a programmatic ledger of relationship and encounter statistics.

This README is a concise reference for what the mod actually implements, how to use it from Papyrus, and what it tracks.

## What this mod implements

- Runtime thread collector for OStim scenes (non-persistent): `ThreadsCollector`
    - Tracks per-thread actors, actions (did/got), orgasms, and excitement contribution data
    - Exposes getters/setters to Papyrus to read/write per-thread and per-actor properties
    - Provides ApplyThreadToLedger() to convert a finished thread into persistent ledger updates

- Persistent ledger service: `LoversLedgerService`
    - Stores per-NPC statistics and per-lover relationship data in memory and persists via SKSE serialization
    - **Keyed on `TESNPC` base FormID** (not Actor reference FormID) — data is stable across cell loads/unloads
    - Only tracks **unique** NPCs; non-unique actors (bandits, generic NPCs) are silently skipped for per-NPC stats; encounters with them increment `othersCount` instead
    - Tracks: exclusive/group/solo counts, same-sex encounters, last encounter time, total internal climax (did/got), `othersCount`
    - Tracks action counts (actions_did / actions_got) keyed by normalized action name
    - Maintains lover entries with: exclusiveSex, partOfSameGroupSex, lastTime, orgasms, internalClimax
    - Serialization version 2 (auto-migrates v1 data from Actor refFormIDs to TESNPC base FormIDs)

- Papyrus bindings
    - `TTLL_Store` (core API): get/set generic properties, record actions, query lovers/actions/score
    - `TTLL_ThreadsCollector` (runtime API): thread lifecycle, actor properties, excitement contributions
    - `TTLL_MainController`, `TTLL_OstimIntegration`: Papyrus-side glue that listens for OStim events and applies thread data to the ledger
    - `GetAllNPCs()` and `GetAllLovers()` now return `Form[]` (TESNPC base forms) instead of `Actor[]`

- Relationship seeding (built-in, no external plugin required)
    - On `kPostLoadGame` and `kNewGame`, the plugin scans all `BGSRelationship` forms in the loaded game to seed spouse/courting/lover history
    - Runs **at most once per playthrough** (the completion flag is persisted in the SKSE save)
    - No dependency on RelationsFinder; the scan is done natively using CommonLibSSE

- SkyrimNet integration (optional)
    - If `SkyrimNet.dll` is present (v5+), the plugin registers two decorators at `kDataLoaded`:
        - `ttll_get_npc_sexual_behavior` — prose summary of an NPC's sexual orientation and encounter preferences
        - `ttll_get_npc_lovers` — Markdown list of significant sexual partners with bond strength and encounter stats
    - Prompt submodule files are shipped with the mod for SkyrimNet character bio injection:
        - `SKSE/Plugins/SkyrimNet/prompts/submodules/character_bio/0301_personality_lovers_ledger_insights.prompt`
        - `SKSE/Plugins/SkyrimNet/prompts/submodules/character_bio/0601_relations_lovers_ledger_insights.prompt`
    - If SkyrimNet is not installed, the integration is silently skipped

- SKSE Serialization
    - Ledger data is saved/loaded using SKSE serialization (unique ID: 'LLGR', format version 2)
    - v1 saves (Actor refFormID keys) are automatically migrated to v2 (TESNPC base FormID keys) on load
    - Serialization logs failures and attempts to skip/continue on invalid entries for robustness

## Papyrus API (short reference)

**TTLL_Store** (global functions registered in Papyrus)
  - GetNpcInt(Actor npc, String propName) -> Int
  - GetNpcFlt(Actor npc, String propName) -> Float
  - GetLoverInt(Actor npc, Actor lover, String propName) -> Int
  - GetLoverFlt(Actor npc, Actor lover, String propName) -> Float
  - SetNpcInt(Actor npc, String propName, Int value)
  - SetNpcFlt(Actor npc, String propName, Float value)
  - SetLoverInt(Actor npc, Actor lover, String propName, Int value)
  - SetLoverFlt(Actor npc, Actor lover, String propName, Float value)
  - RecordAction(Actor npc, String actionName, Bool isDid)
  - GetAllActions(Actor npc, Bool isDid, Int topK = -1) -> String[]
  - GetActionCount(Actor npc, Bool isDid, String actionName) -> Int
  - IncrementInt(Actor npc, String intName)
  - GetAllNPCs() -> **Form[]** (returns TESNPC base forms; cast to `ActorBase` for name/gender, `Actor` may be None if cell is unloaded)
  - GetAllLovers(Actor npc, Int topK = -1) -> **Form[]** (same as above)
  - GetLoverScore(Actor npc, Actor lover) -> Float

**TTLL_ThreadsCollector** (global functions registered in Papyrus)
  - CleanThread(Int ThreadID)
  - CleanFinishedThreads()
  - GetActors(Int ThreadID) -> Actor[]
  - ExcitementContributorOrgasm(Int ThreadID, Actor npc)
  - UpdateExcitementRate(Int ThreadID, Actor npc, Actor lover, Float rate)
  - ApplyThreadToLedger(Int ThreadID)
  - GetThreadInt/Str/Bool, SetThreadInt/Str/Bool
  - GetActorInt/Flt/Bool, SetActorInt/Flt/Bool

Notes on property paths: several functions accept dot-separated property paths (example: `"totalInternalClimax.did"`, `"actions_did.Vaginal"`, `"internalClimax.got"`). Property names are normalized to lowercase.

**TESNPC vs Actor note:** All ledger functions accept `Actor` parameters, but internally resolve to the TESNPC base FormID. Non-unique actors (bandits, generic NPCs) are silently ignored for per-NPC stats; their encounters are tracked via `othersCount` on each unique partner.

## Behavior & important details

  - Thread data is runtime-only and will not be persisted. Use ApplyThreadToLedger when a scene finishes to persist results into the ledger.
  - Ledger data is persisted across saves using SKSE serialization (format version 2).
  - Relationship seeding runs automatically at game load/new game. It scans all BGSRelationship forms and seeds spouse/courting/lover history for unique NPCs not already in the ledger. The scan runs at most once per playthrough.
  - Non-unique actors (0xFF temp FormIDs, non-unique TESNPC) are never stored as NPC or lover entries. Encounters with non-unique partners are tallied in `othersCount` on the unique actor's record.
  - Action and property names are normalized to lowercase before storage and lookup for Papyrus compatibility.
  - Existing v1 saves (keyed by Actor refFormID) are automatically migrated to v2 (keyed by TESNPC base FormID) on the first load after updating.

## Logging and debugging

  - The plugin writes logs to the SKSE log directory (file: `LoversLedger.log`). It logs at trace/debug/info/warn/error and includes contextual information such as FormIDs and actor names when available.
  - Papyrus scripts include debug helpers (`TTLL_Debug.psc`) used by the mod to print events and processing details to the console/log.

## Requirements

  - Skyrim Special Edition
  - SKSE (matching game version)
  - [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
  - OStim (for full OStim integration)
  - SexLab (optional — the project includes a stub `TTLL_SexlabIntegration.psc` for custom integration)
  - [SkyrimNet](https://www.nexusmods.com/skyrimspecialedition/mods/123453) v5+ (optional) — enables NPC bio decorators for AI-driven dialogue

## Installation

  1. Install SKSE and required dependencies
  2. Place `TT Lover's Ledger` in your Data folder (via your mod manager)
  3. Ensure it loads after OStim/SexLab if you want integration

## Troubleshooting

  - If ledger entries aren't appearing:
    - Check `LoversLedger.log` in the SKSE log folder for errors/warnings
    - Verify `ApplyThreadToLedger` is called on thread end (OStim integration calls this automatically when `hadSex` is true)

## Developer notes

  - Keys and paths used in Papyrus functions are intentionally normalized; use lowercase when possible.
  - The service provides public functions in C++ exposed to Papyrus; see `SKSE_Source/src` for the exact signatures and behavior.

## Changelog

  - v1.0.0.dev (commit af8bcf0):
    - **TESNPC-based tracking**: ledger now keys on TESNPC base FormIDs instead of Actor reference FormIDs, fixing data loss when cells unload/reload
    - Non-unique actors (bandits, generic NPCs) are skipped for per-NPC stats; encounters with them increment the new `othersCount` field on each unique partner's record
    - `GetAllNPCs()` and `GetAllLovers()` now return `Form[]` (TESNPC base forms) instead of `Actor[]`
    - **Dropped RelationsFinder dependency**: relationship seeding is now built-in via a one-time scan of all `BGSRelationship` forms (runs at `kPostLoadGame` / `kNewGame`, once per playthrough)
    - **Optional SkyrimNet integration** (v5+): merged Lover's Neural Ledger decorator logic directly into this plugin; registers `ttll_get_npc_sexual_behavior` and `ttll_get_npc_lovers` decorators; ships SkyrimNet prompt submodule files for character bio injection
    - Serialization version bumped to 2; automatic v1 → v2 migration on first load (legacy Actor refFormID data is lazily resolved and merged)
    - Added `otherscount` NPC property (accessible via `GetNpcInt(npc, "otherscount")`)
    - Added `nlohmann_json` vcpkg dependency
    - Papyrus `Maintenance()` now clears legacy `TTLNLDec_` StorageUtil keys from the old Neural Ledger standalone mod

  - v0.1.0.dev:
    - **Major Architecture Change**: Migrated from JContainers-based Papyrus implementation to native SKSE plugin
    - Implemented `LoversLedgerService` in C++ with SKSE serialization for persistent ledger data
    - Implemented `ThreadsCollector` in C++ for runtime thread tracking and excitement contribution system
    - Added Papyrus native function bindings for `TTLL_Store` and `TTLL_ThreadsCollector`
    - Removed dependencies on JContainers (JCData) and related Papyrus scripts
    - Significantly reduced Papyrus script complexity and overhead
    - Added `RelationsFinderAPI.h` integration for optional relationship seeding
    - Improved performance and reliability through native C++ implementation
    - Added comprehensive logging to `LoversLedger.log` in SKSE log directory

  - v0.0.2:
    - Add runtime event `ttll_thread_data_event` (fired after ledger update)
    - Track `orgasmed` per-thread actor
    - Add `finished` thread flag to allow delayed cleaning
    - Improve logging across Papyrus bindings and core services

  - v0.0.1: initial release

