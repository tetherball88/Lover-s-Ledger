# TTLL_Store Global Functions Reference

This document provides a comprehensive reference for all global functions available in the `TTLL_Store` script. These functions manage NPC data storage, relationships, and statistics tracking for the TT Lover's Ledger mod.

## Table of Contents
- [Data Structure Management](#data-structure-management)
- [NPC Management](#npc-management)
- [NPC Basic Properties](#npc-basic-properties)
- [NPC Counter Properties](#npc-counter-properties)
- [NPC Actions Management](#npc-actions-management)
- [Lover Management](#lover-management)
- [Top Three Lovers Management](#top-three-lovers-management)

## Data Structure Management

### GetNamespaceKey
```papyrus
string Function GetNamespaceKey() global
```
Returns the namespace key for TT_LoversLedger data in JContainers.

### Clear
```papyrus
Function Clear() global
```
Clears the main tracking object.

### ExportData
```papyrus
Function ExportData() global
```
Exports the main NPCs JMap object to a file.

### ImportData
```papyrus
Function ImportData() global
```
Imports the main NPCs JMap object from a file.

### ImportInitialData
```papyrus
Function ImportInitialData() global
```
Imports initial data from the mod's default data file.

### GetRoot
```papyrus
int Function GetRoot() global
```
Returns the root JContainers object for the mod's data.

### GetPlayerRef
```papyrus
Actor Function GetPlayerRef() global
```
Returns the player actor reference.

## NPC Management

### GetNpcs
```papyrus
int Function GetNpcs() global
```
Returns the JContainers object containing all tracked NPCs.

### GetNpc
```papyrus
int Function GetNpc(Actor npc) global
```
Returns the JContainers object for a specific NPC, creating it if it doesn't exist.

### CheckLover
```papyrus
Function CheckLover(Actor npc, Actor lover, Actor spouse, Actor courting) global
```
Validates and processes a lover relationship for an NPC.

## NPC Basic Properties

### GetNpcName / SetNpcName
```papyrus
string Function GetNpcName(Actor npc) global
Function SetNpcName(Actor npc, string name) global
```
Get or set an NPC's name.

### GetNpcRace / SetNpcRace
```papyrus
string Function GetNpcRace(Actor npc) global
Function SetNpcRace(Actor npc, string raceVal) global
```
Get or set an NPC's race.

### GetNpcSex / SetNpcSex
```papyrus
int Function GetNpcSex(Actor npc) global
Function SetNpcSex(Actor npc, int sex) global
```
Get or set an NPC's sex/gender.

### GetNpcLastTime / SetNpcLastTime
```papyrus
float Function GetNpcLastTime(Actor npc) global
string Function GetNpcLastTimeString(Actor npc) global
Function SetNpcLastTime(Actor npc, float time) global
Function UpdateNpcLastTime(Actor npc) global
```
Manage the timestamp of NPC's last romantic interaction.

## NPC Counter Properties

### Same Sex Encounters
```papyrus
int Function GetSameSexEncounterCount(Actor npc) global
int Function IncrementSameSexEncounterCount(Actor npc) global
```
Track same-sex encounters for an NPC.

### Solo Sex
```papyrus
int Function GetSoloSexCount(Actor npc) global
int Function IncrementSoloSexCount(Actor npc) global
```
Track solo encounters for an NPC.

### Exclusive Sex
```papyrus
int Function GetExclusiveSexCount(Actor npc) global
int Function IncrementExclusiveSexCount(Actor npc) global
```
Track exclusive (one partner) encounters for an NPC.

### Group Sex
```papyrus
int Function GetGroupSexCount(Actor npc) global
int Function IncrementGroupSexCount(Actor npc) global
```
Track group encounters for an NPC.

### Internal Climax
```papyrus
int Function GetNpcInternalClimax(Actor npc) global
int Function GetNpcInternalClimaxDid(Actor npc) global
int Function IncrementNpcInternalClimaxDid(Actor npc) global
int Function GetNpcInternalClimaxGot(Actor npc) global
int Function IncrementNpcInternalClimaxGot(Actor npc) global
```
Track internal climax statistics for an NPC.

## NPC Actions Management

### Action Objects
```papyrus
int Function GetNpcActions(Actor npc) global
int Function GetNpcDidActions(Actor npc) global
int Function GetNpcGotActions(Actor npc) global
int Function GetNpcDidTopThreeActions(Actor npc) global
int Function GetNpcGotTopThreeActions(Actor npc) global
```
Access various action tracking objects for an NPC.

### Record Actions
```papyrus
Function RecordNpcDidAction(Actor npc, string actionName) global
Function RecordNpcGotAction(Actor npc, string actionName) global
```
Record performed or received actions for an NPC.

### Get Action Counts
```papyrus
int Function GetNpcDidActionCount(Actor npc, string actionName) global
int Function GetNpcGotActionCount(Actor npc, string actionName) global
```
Get counts of performed or received actions.

### UpdateTopThreeActions
```papyrus
Function UpdateTopThreeActions(Actor npc, string actionType, string actionName, int newCount) global
```
Updates the top three most frequent actions for an NPC.

## Lover Management

### Lover Objects
```papyrus
int Function GetNpcLovers(Actor npc) global
int Function GetLoverObject(Actor npc, Actor lover) global
```
Access lover tracking objects.

### Create Lovers
```papyrus
int Function CreateNewJLoverByActor(Actor lover) global
int Function CreateExistingJLoverByActor(Actor lover, bool isSpouse, bool isCourting, bool isLover) global
int Function CreateExistingLoverWithNpc(Actor npc, Actor lover, bool isSpouse, bool isCourting, bool isLover) global
```
Create new lover entries with various relationship states.

### Lover Properties
```papyrus
string Function GetLoverName(Actor npc, Actor lover) global
string Function GetLoverRace(Actor npc, Actor lover) global
int Function GetLoverSex(Actor npc, Actor lover) global
```
Access basic lover properties.

### Lover Counters
```papyrus
int Function GetLoverExclusiveSexCount(Actor npc, Actor lover) global
int Function IncrementLoverExclusiveSexCount(Actor npc, Actor lover) global
int Function GetLoverGroupSexCount(Actor npc, Actor lover) global
int Function IncrementLoverGroupSexCount(Actor npc, Actor lover) global
```
Track encounter statistics for specific lovers.

### Lover Timestamps
```papyrus
float Function GetLoverLastTime(Actor npc, Actor lover) global
string Function GetLoverLastTimeString(Actor npc, Actor lover) global
Function SetLoverLastTime(Actor npc, Actor lover, float time) global
Function UpdateLoverLastTime(Actor npc, Actor lover) global
```
Manage timestamps for lover interactions.

### Lover Statistics
```papyrus
float Function GetLoverOrgasms(Actor npc, Actor lover) global
float Function AddLoverOrgasms(Actor npc, Actor lover, float amount) global
```
Track orgasm statistics for lovers.

### Lover Internal Climax
```papyrus
int Function GetLoverInternalClimax(Actor npc, Actor lover) global
int Function GetLoverInternalClimaxDid(Actor npc, Actor lover) global
int Function IncrementLoverInternalClimaxDid(Actor npc, Actor lover) global
int Function GetLoverInternalClimaxGot(Actor npc, Actor lover) global
int Function IncrementLoverInternalClimaxGot(Actor npc, Actor lover) global
```
Track internal climax statistics for specific lovers.

### UpdateLover
```papyrus
Function UpdateLover(Actor npc, Actor lover, int participants, float orgasms) global
```
Updates all lover statistics after an encounter.

## Top Three Lovers Management

### Top Three Functions
```papyrus
int Function GetNpcTopThreeLovers(Actor npc) global
bool Function IsNpcTopThreeLover(Actor npc, Actor lover) global
Function UpdateTopThreeLovers(Actor npc, Actor currentLover) global
float Function GetLoverScore(Actor npc, Actor lover) global
```
Manage and track an NPC's top three lovers based on various criteria.