# Changelog

https://github.com/tetherball88/Lover-s-Ledger/releases/tag/v1.1.0

Requires SKSE, SKSE Menu Framework and OStim. Optional: SexLab AE or SexLab P+, SkyrimNet 0.25.0+.

## v0.1.1 (unreleased)

### SexLab support
- **SexLab AE and P+ scenes are recorded**: `TTLL_SexlabIntegration` (was a placeholder) feeds `TTLL_ThreadsCollector` the same way `TTLL_OstimIntegration` does. SexLab threads use collector ids `-1` to `-15` (`-1 - tid`), so they never collide with OStim ids. SexLab acts are mapped to OStim action names, so counts from both frameworks merge.
- Acts come from P+ collision interactions (`CTYPE`), with a stage-tag fallback, and from animation tags on AE. Excitement is rated per acting pair in both directions; only pairs that stopped are zeroed on a stage change.
- Internal climax comes from AE cum data or a P+ vaginal interaction (`TTLL_Utils.RecordInternalClimax`).
- `TTLL_MainController`: SexLab `Hook*` and `SexLabOrgasm` handlers. P+ `ChangeActors` restart flags keep a restart as one encounter; if the restart fails, a 5 s fallback applies the thread. Restart arrays are reset on load.
- `TTLL_SexlabPPlus` bridge for P+-only calls: the real script is in `scripts/source_pplus` (built against the P+ sources); `scripts/source_headers` holds the stub the main build compiles against.
- **3-way and strap-on attribution**: strap-on wearers (AE `IsUsingStrapon`) at position 1+ penetrate position 0 ahead of penis-havers; 3+ actor cunnilingus goes from a non-penetrator to the position 1+ vagina-haver; strap-ons count toward the internal-climax penis tally. New helpers `UsesStrapon`, `IsActGiver`.
- **OStim scenes count in SexLab stats**: `TTLL_SexlabIntegration.AddOStimStats` (AE) and `TTLL_SexlabPPlus.AddCounts` (P+) add +1 to each participant's SexLab sex / vaginal / anal / oral counters at OStim scene end.
- `Papyrus_API_Keys.md`: thread id ranges; `otherscount` is not an `IncrementInt` key. README: SexLab notes and stats sync.

### Orgasm attribution
- **Excitement credited to every partner in a burst**: `ThreadsCollector.h` adds `ExcitementContributor::lastUpdated` and drops the shared `ThreadData::prevExcitementStarted`. `UpdateExcitementRate` accrues each contributor from its own timestamp, so every pair updated in the same tick gets its elapsed time (before, only the first call did).
- `ExcitementContributorOrgasm` flushes accrual up to now before splitting, splits evenly across active partners when nothing has accrued yet, and resets totals but keeps rates, so a second orgasm in the same scene is credited.
- **SkyrimNet_SexLab OrgasmEngine**: when SkyrimNet_SexLab is installed and its engine is on at scene start, orgasms are counted from `SkyrimNet_SexLab_Orgasm` instead of `SexLabOrgasm`, and each lover's share is weighted by how much enjoyment rose during their acts. Without the mod, or with `engine_disabled`, the `SexLabOrgasm` path is unchanged.
  - `TTLL_SkyrimNetSexLab.psc`: soft-dependency wrappers (`Installed`, `EngineDisabled`, `IsManaged`, `GetEnjoyment`). `scripts/source_headers/SkyrimNet_SexLab_OrgasmEngine.psc` is a header stub and is never compiled.
  - `TTLL_SexlabIntegration`: the engine mode is latched per thread at `StartThread` (`TTLL_SL_ENG_`); `EngineOwns` decides which event counts; enjoyment is sampled each stage and every 1 s and the gain is banked per pair (`YA_` / `YL_` / `YS_`); `CreditOrgasm` uses the weighted split when gain was banked.
  - `TTLL_MainController`: `SLNetOrgasm` handler; `SLOrgasm` skips actors the engine owns; `ArmUpdate` keeps the 1 s poll from being pushed back by longer re-arms.
  - New native `ExcitementContributorOrgasmWeighted` (never creates contributors, resets totals). Both orgasm natives log `Orgasm credit: ... mode=time|engine` at info.

### Scene counters and action names
- **Global scene counters**: `TTLL_Store.HowManyTimes(race, gender, actionType, direction)` and `HowManyTimesWith(numActors)` count one per scene across every participant (unique or not), saved in the SKSE co-save.
- **Action-name synonyms**: new `SKSE/Plugins/LoversLedger/synonyms.json`, clusters copied from SkyrimNet_SexLab `synonyms-broad.json` plus ledger names (`analsex`, `vaginalsex`, `deepthroat`, `*masturbation`, `fingering`). The first member of a cluster is the stored word.
- `LoversLedgerService::LoadSynonyms` runs at `kDataLoaded`; `Canonical()` maps recorded actions, scene keys and `GetActionCount` / `HowManyTimes` queries to the cluster word. Old saves are merged to cluster words on load. README documents both.

### SexLab history backfill
- **SexLab stats copied in on first sight**: `TTLL_SexlabIntegration.CheckHistory` runs the first time an actor is in a scene (SexLab or OStim) and copies SexLab counts the ledger lacks (sex, masturbation, vaginal, anal, oral).
- The counts go to the actor's game spouse / lover / courting partner (`AddHistoryToPartner`). With none, they go to a stand-in past lover whose race and gender the SkyrimNet decision model picks (`prompts/decisions/loversledger/past_lover.prompt`, `TTLL_MainController.OnPastLoverDecision`); fallback is own race, opposite gender.
- SKSE natives `ClaimSexLabCheck`, `AddHistoryToPartner`, `AddPastLover`. The checked set and past lovers are saved in a new `LLPL` record (`LLGR` stays v2).
- The `ttll_get_npc_lovers` decorator (`SkyrimNetDecorators.cpp`) lists past lovers ("An unnamed <race> woman/man from the past").
- `TTLL_SexlabPPlus.GetCount` (+ header stub).
- **Past-lover decision never fired**: `decnpc` rejected the quoted hex `npc_uuid` in the decision context. New `TTLL_Store.UuidToDecimal`; `SendPastLover` sends `npc_uuid` as a JSON number.

### First-time lines
- **First-time sentence per scene**: new `SKSE_Source/src/FirstTimes.h/.cpp` builds one line per unique actor whose scene is a first: first sex, new partner gender / race / group size, first vaginal / anal given or received, and an optional virgin-blood line.
- `LoversLedgerService`: `PartnerTraits` / `FirstTimeHistory`, seeded from older data, saved as `LLPT`, cleared with the NPC. `ThreadsCollector` calls `RecordPartners` before the scene is applied.
- `TTLL_Store.DescribeFirstTimes` native and the `TTLL_SexlabIntegration.GetFirstTimes(positions, tags)` wrapper, which fills SexLab counts and anatomy.
- SkyrimNet dashboard toggle "Add virgin blood to first sexual encounter" (`config/plugins/LoversLedger/manifest.yaml`, off by default). SkyrimNet plugin `manifest.json` version 1.0.3.

### Bug fixes
- **Stale threads after a load**: `ThreadsCollector` is cleared on load and new game.
- **Possible deadlock**: removed recursive `_mutex` locks in `LoversLedgerService::ScoreLover` and `ApplyThreadToLedger`.
- `ResolveUniqueBaseID` null-checks the base; cleanup uses `IsEffectivelyUnique`.
