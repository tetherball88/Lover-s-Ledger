# Lover's Ledger Papyrus API Keys

This document lists all valid keys (property names) for the `TTLL_Store` Papyrus script functions. All keys are case-insensitive.

## NPC Properties

These keys are used with `GetNpcInt`, `SetNpcInt`, `GetNpcFlt`, and `SetNpcFlt`.

### Integer Properties (`GetNpcInt` / `SetNpcInt`)

| Key | Description |
| :--- | :--- |
| `samesexencounter` | Total count of same-sex encounters. |
| `solosex` | Total count of solo sex acts. |
| `exclusivesex` | Total count of exclusive sex across all lovers. |
| `groupsex` | Total count of group sex participation across all lovers. |
| `otherscount` | Total encounters with non-unique partners (bandits, generic NPCs, etc.). Read-only in practice; incremented automatically when a scene involves a non-unique partner. |
| `totalinternalclimax.did` | Total internal climaxes given to all partners. |
| `totalinternalclimax.got` | Total internal climaxes received from all partners. |
| `actions_did.<ActionName>` | Count of a specific action performed (e.g., `actions_did.Vaginal`). |
| `actions_got.<ActionName>` | Count of a specific action received (e.g., `actions_got.Oral`). |

### Float Properties (`GetNpcFlt` / `SetNpcFlt`)

| Key | Description |
| :--- | :--- |
| `lasttime` | The game time (in days) of the last recorded encounter. |

## Lover Properties

These keys are used with `GetLoverInt`, `SetLoverInt`, `GetLoverFlt`, and `SetLoverFlt`. These functions require both an NPC and a Lover actor.

### Integer Properties (`GetLoverInt` / `SetLoverInt`)

| Key | Description |
| :--- | :--- |
| `exclusivesex` | Count of exclusive sex with this specific lover. |
| `partofsamegroupsex` | Count of group sex incidents involving this specific lover. |
| `internalclimax.did` | Internal climaxes given to this specific lover. |
| `internalclimax.got` | Internal climaxes received from this specific lover. |

### Float Properties (`GetLoverFlt` / `SetLoverFlt`)

| Key | Description |
| :--- | :--- |
| `lasttime` | The game time (in days) of the last encounter with this specific lover. |
| `orgasms` | Total orgasm score/count with this specific lover. |

## Increment Counters

These keys are used with `IncrementInt` to increase a value by 1.

| Key |
| :--- |
| `samesexencounter` |
| `solosex` |
| `exclusivesex` |
| `groupsex` |
| `otherscount` |

## Action Recording

Used with `RecordAction(Actor npc, String actionName, Bool isDid)`.

| Parameter | Description |
| :--- | :--- |
| `actionName` | Any string identifier for the act (e.g., `"Vaginal"`, `"Anal"`, `"Oral"`). |
| `isDid` | `true` if the NPC performed the act; `false` if they received it. |

*Note: Calling `RecordAction` automatically updates the corresponding `actions_did.<ActionName>` or `actions_got.<ActionName>` counter.*

## Return Type Notes

`GetAllNPCs()` and `GetAllLovers()` return `Form[]` containing **TESNPC base forms** (not Actor references).

- Cast to `ActorBase` to read name, gender, race, etc.: `(someForm as ActorBase).GetName()`
- Casting to `Actor` returns `None` when the NPC's cell is not loaded — this is expected and safe.
- All other `TTLL_Store` functions still accept `Actor` parameters; the plugin resolves the base FormID internally.

> **Non-unique actors** (bandits, generic NPCs with a non-unique TESNPC base) are never stored as NPC or lover entries. Encounters with them are instead accumulated in the `otherscount` field of each unique NPC that participated in the scene.
