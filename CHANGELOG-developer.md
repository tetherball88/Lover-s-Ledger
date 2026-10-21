https://github.com/tetherball88/Lover-s-Ledger/releases/tag/v1.1.0

Requires SKSE, SKSE Menu Framework and OStim. Optional: SexLab AE or SexLab P+, SkyrimNet 0.25.0+.

## v0.1.1 (unreleased)

### SexLab support
- **SexLab AE and P+ scenes are recorded** into the same ledger fields as OStim. SexLab acts are stored under OStim action names, so `GetActionCount` and friends return merged counts.
- **Thread ids**: `TTLL_ThreadsCollector` functions and the `ttll_thread_data_event` mod event share one id space. `0` and above are OStim thread ids; `-1` to `-15` are SexLab threads stored as `-1 - tid` (SexLab tid `0` is `-1`). See `Papyrus_API_Keys.md`.
- **`otherscount` is not an `IncrementInt` key**. It is incremented automatically when a scene has a non-unique partner.
- A P+ `ChangeActors` restart stays one encounter (one apply at the real end).
- Strap-on wearers (AE) count as penetrators, including for internal climax.
- Each OStim scene adds +1 to every participant's SexLab sex (total or masturbation), vaginal, anal and oral counters, so SexLab's stats include OStim encounters.

### Orgasm attribution
- **Orgasm credit** is now split by each contributor's own accrued excitement (per-contributor timestamps). With nothing accrued, it splits evenly across active partners. A second orgasm in the same scene is credited.
- **SkyrimNet_SexLab OrgasmEngine**: when it is installed and on at scene start, orgasms count from `SkyrimNet_SexLab_Orgasm` (not `SexLabOrgasm`) and each lover's share is weighted by the enjoyment gain during their acts. The mode is fixed per scene at start. Needs a SkyrimNet_SexLab build with `IsEngineDisabled`.
- New native **`TTLL_ThreadsCollector.ExcitementContributorOrgasmWeighted(int ThreadID, Actor npc, Actor[] lovers, float[] weights)`**: credits one orgasm by the passed weights. Ignores lovers that are not contributors, resets accrued totals, no-op when weights sum to 0 or less.
- `LoversLedger.log` logs `Orgasm credit: ... mode=time|engine` at info for each credit.

### Scene counters and action names
- **`TTLL_Store.HowManyTimes(Race akRace = None, Int gender = -1, String actionType = "", String direction = "any")`**: scenes where a participant of that race / gender gave or received the act (`direction` `any` / `gave` / `received`). Counts every participant, unique or not, once per scene.
- **`TTLL_Store.HowManyTimesWith(Int numActors = 0)`**: scenes with exactly that many participants; `<= 0` returns all.
- **`SKSE/Plugins/LoversLedger/synonyms.json`**: `clusters` of action-name synonyms. The first member is the stored word; recorded actions and `GetActionCount` / `HowManyTimes` queries are mapped to it (`analsex` and `ass` both read `anal`). Names outside any cluster are kept as recorded. `GetAllActions` returns raw names. Existing saves are merged to cluster words on load.

### SexLab history backfill
- The first time an actor is in a scene, SexLab counts the ledger lacks are copied in: to the actor's spouse / lover / courting partner, or else to a stand-in past lover whose race and gender the SkyrimNet decision model picks (`prompts/decisions/loversledger/past_lover.prompt`).
- New natives: **`TTLL_Store.ClaimSexLabCheck(Actor npc)`**, **`AddHistoryToPartner(Actor npc, Int encounters, Int solo, Int vaginal, Int anal, Int oral)`**, **`AddPastLover(Actor npc, Race akRace, Int gender, Int encounters, Int solo, Int vaginal, Int anal, Int oral)`**.
- Past lovers are not actors: `GetAllLovers` does not return them. The **`ttll_get_npc_lovers`** decorator lists them under "Sexual Partners" as "An unnamed <race> woman/man from the past".
- **`TTLL_Store.UuidToDecimal(String hexUuid)`**: SkyrimNet entity UUID as decimal digits. Put it unquoted in decision context JSON; `decnpc()` needs a number.
- New co-save record `LLPL` (`LLGR` stays version 2).

### First-time lines
- **`TTLL_Store.DescribeFirstTimes(Actor[] positions, String tags, Int[] slTotal, Int[] slVaginal, Int[] slAnal, Int[] anatomy)`**: one line per unique position whose scene is a first (first sex; new partner gender, race or group size; first vaginal / anal given or received), `""` when nothing is new. Call it before the scene ends.
- SexLab callers use **`TTLL_SexlabIntegration.GetFirstTimes(Actor[] positions, String tags)`**, which fills the SexLab counts and anatomy.
- On SexLab AE, StorageUtil int `SkyrimNet_SexLab_ChosenSex` = 2 on an actor (SkyrimNet_SexLab Actor Info intersex pick) gives anatomy 3 (penis + vagina) in every SexLab check, as P+ futa does. The partner gender for anatomy 3 reads "an intersex partner".
- New co-save record `LLPT`.

### Bug fixes
- Thread data is cleared on load and new game, so a save loaded mid-scene no longer leaves stale threads.
- Fixed a possible deadlock in lover scoring and thread apply.
