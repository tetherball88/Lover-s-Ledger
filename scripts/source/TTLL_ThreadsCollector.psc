scriptname TTLL_ThreadsCollector hidden

; ========================================================================================
; Generic Getter/Setter Functions (Dot-Separated Property Paths)
; ========================================================================================

; Thread-level property getters (no actor parameter)
; Examples:
;   GetThreadBool(threadID, "finished") - get thread finished status
;   GetThreadBool(threadID, "hadSex") - get thread had sex status
;   GetThreadStr(threadID, "lastSexualSceneId") - get last scene ID
int Function GetThreadInt(int ThreadID, string propName) global native
string Function GetThreadStr(int ThreadID, string propName) global native
bool Function GetThreadBool(int ThreadID, string propName) global native

; Thread-level property setters (no actor parameter)
Function SetThreadInt(int ThreadID, string propName, int value) global native
Function SetThreadStr(int ThreadID, string propName, string value) global native
Function SetThreadBool(int ThreadID, string propName, bool value) global native

; Actor-level property getters (requires actor parameter)
; Examples:
;   GetActorBool(threadID, actor, "orgasmed") - get actor orgasm status
;   GetActorBool(threadID, actor, "hadSameSexEncounter") - get same-sex encounter flag
;   GetActorBool(threadID, actor, "did.VaginalSex") - check if actor did "Vaginal" action
int Function GetActorInt(int ThreadID, Actor npc, string propName) global native
float Function GetActorFlt(int ThreadID, Actor npc, string propName) global native
bool Function GetActorBool(int ThreadID, Actor npc, string propName) global native

; Actor-level property setters (requires actor parameter)
Function SetActorInt(int ThreadID, Actor npc, string propName, int value) global native
Function SetActorFlt(int ThreadID, Actor npc, string propName, float value) global native
Function SetActorBool(int ThreadID, Actor npc, string propName, bool value) global native


; ========================================================================================
; Thread Management
; ========================================================================================

Function CleanThread(int ThreadID) global native

Function CleanFinishedThreads() global native

; ========================================================================================
; Actor Management
; ========================================================================================

Actor[] Function GetActors(int ThreadID) global native

; ========================================================================================
; Excitement Contribution
; ========================================================================================

Function ExcitementContributorOrgasm(int ThreadID, Actor npc) global native

;/**
 * Updates the excitement contribution rate for a lover.
 * This function:
 * - Calculates time delta since last update
 * - Accumulates excitement using previous rate
 * - Sets new contribution rate
 * - Updates timestamp for next calculation
 * 
 * @param {int} ThreadID - The ID of the thread
 * @param {Actor} npc - The actor whose excitement is being updated
 * @param {Actor} lover - The lover contributing to the excitement
 * @param {float} rate - The new excitement contribution rate
*/;
Function UpdateExcitementRate(int ThreadID, Actor npc, Actor lover, float rate) global native

; ========================================================================================
; Apply Thread Data to Ledger
; ========================================================================================

;/**
 * Summarizes all thread data and applies it to the TTLL_Store.
 * This processes all actors in the thread, updating their statistics:
 * - Action counts (did/got)
 * - Encounter type counters (solo/couple/group)
 * - Same-sex encounter tracking
 * - Lover relationships and orgasm contributions
 * 
 * Call this when a thread finishes to persist the data to the main ledger.
 * @param {int} ThreadID - The ID of the thread to process
*/;
Function ApplyThreadToLedger(int ThreadID) global native
