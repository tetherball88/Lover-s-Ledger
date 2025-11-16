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
    - Tracks: exclusive/group/solo counts, same-sex encounters, last encounter time, total internal climax (did/got)
    - Tracks action counts (actions_did / actions_got) keyed by normalized action name
    - Maintains lover entries with: exclusiveSex, partOfSameGroupSex, lastTime, orgasms, internalClimax

- Papyrus bindings
    - `TTLL_Store` (core API): get/set generic properties, record actions, query lovers/actions/score
    - `TTLL_ThreadsCollector` (runtime API): thread lifecycle, actor properties, excitement contributions
    - `TTLL_MainController`, `TTLL_OstimIntegration`: Papyrus-side glue that listens for OStim events and applies thread data to the ledger

- RelationsFinder integration
    - If `RelationsFinder.dll` is present, the plugin will query it on data load and scan existing spouse/courting/lover relationships to generate seed data for the ledger

- SKSE Serialization
    - Ledger data is saved/loaded using SKSE serialization (unique ID: 'LLGR', versioned)
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
  - GetAllNPCs() -> Actor[]
  - GetAllLovers(Actor npc, Int topK = -1) -> Actor[]
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

## Behavior & important details

  - Thread data is runtime-only and will not be persisted. Use ApplyThreadToLedger when a scene finishes to persist results into the ledger.
  - Ledger data is persisted across saves using SKSE serialization.
  - When RelationsFinder is available at data load, the plugin will scan and seed existing relationships to produce realistic historical data (spouse/courting/lover). Missing forms are skipped with warnings.
  - Action and property names are normalized to lowercase before storage and lookup for Papyrus compatibility.

## Logging and debugging

  - The plugin writes logs to the SKSE log directory (file: `LoversLedger.log`). It logs at trace/debug/info/warn/error and includes contextual information such as FormIDs and actor names when available.
  - Papyrus scripts include debug helpers (`TTLL_Debug.psc`) used by the mod to print events and processing details to the console/log.

## Requirements

  - Skyrim Special Edition
  - SKSE (matching game version)
  - OStim (for full OStim integration)
  - SexLab (optional — the project includes a stub `TTLL_SexlabIntegration.psc` for custom integration)
  - RelationsFinder (optional) to auto-seed relationships

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
