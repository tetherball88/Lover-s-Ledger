scriptname TTLL_Store

import TTLL_JCDomain

;/
{
    initialData: { any data coming from external json with some initial parameters
        playerRef: Actor // reference to the player actor, to get it accessible from global functions
    },
    npcs: { // list of all npcs who OStim has ever tracked
        [Actor]: { // npc key
            name: string; // npc's name
            race: string; // npc's race
            sex: string; // npc's sex/gender
            actions: { // list of actions npc participated during whole playthrough, with counter how many times
                "did": { // object which tracks actions current actor did as main actor during whole playthrough
                    actionX: integer;  // actionX - is OStim action name, integer - how many times
                }
                "got": { // object which tracks actions current actor received as target actor during whole playthrough
                    actionX: integer; // actionX - is OStim action name, integer - how many times
                }
                "didTopThree": {
                    actionX: integer;
                }
                "gotTopThree": {
                    actionX: integer;
                }
            }
            lovers: { // list of npc's lovers, other npcs with which current npc had ostim sexual interaction during whole playthrough
                [Actor]: { // lover's key
                    name: string; // lover's name
                    race: string; // lover's race
                    sex: string; // lover's sex/gender
                    exclusiveSex: integer; // 1 point
                    partOfSameGroupSex: integer; // 0.5 points
                    lastTime: float; // less than 1 day - 3 points; less than week ago - 2 points; more than week ago - 1 point, more than month ago - 0
                    orgasms: float; // amount of contributions to orgasms from lover to npc. Example orgasm in exclusive thread will count as 1.0, in group where multiple actors contributed it will count as part
                    internalClimax: {
                        did: integer; // amount of times npc climaxed inside lover
                        got: integer; // amount of times lover climaxed inside npc
                    };
                }
            }
            topThreeLovers: {
                [Actor]: float; // float - favorite score
            }
            sameSexEncounter: integer // counter of same sex encounters
            soloSex: integer; // total amount of ostim thread where npc is the only actor
            exclusiveSex: integer; // total amount of ostim thread where npc has only one partner
            groupSex: integer; // total amount of ostim thread where npc has more than one partner
            lastTime: float;
            totalInternalClimax: {
                did: integer; // amount of times npc gave internal climax
                got: integer; // amount of times npc received internal climax
            };
        }
    }
}
/;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Main Data Structure Management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;/
  Returns the namespace key for TT_LoversLedger data in JContainers.
/;
string Function GetNamespaceKey() global
    return ".TT_LoversLedger"
EndFunction

; Clear the main tracking object
Function Clear() global
    JDB_solveObjSetter(GetNamespaceKey(), JMap_object())
    ImportInitialData()
EndFunction

;/**
* Exports the main NPCs JMap object to a file.
*/;
Function ExportData() global
    JValue_writeToFile(JDB_solveObj(GetNamespaceKey()), JContainers.userDirectory() + "LoversLedger/store.json")
EndFunction

;/**
* Imports the main NPCs JMap object from a file.
*/;
Function ImportData() global
    int JObj = JValue_readFromFile(JContainers.userDirectory() + "LoversLedger/store.json")
    if(JObj == 0)
        JObj = JMap_object()
    endif
    JDB_solveObjSetter(GetNamespaceKey(), JObj, true)
    ImportInitialData()
EndFunction

Function ImportInitialData() global
    int JRoot = JDB_solveObj(GetNamespaceKey())
    if(JRoot == 0)
        JDB_solveObjSetter(GetNamespaceKey(), JMap_object())
    endif
    int jInitialObject = JValue_readFromFile("Data/SKSE/Plugins/LoversLedger/initialData.json")
    if(jInitialObject == 0)
        TTLL_Debug.err("Couldn't load data from 'Data/SKSE/Plugins/LoversLedger/initialData.json'")
    endif
    JDB_solveObjSetter(GetNamespaceKey() + ".initialData", jInitialObject, true)
EndFunction

int Function GetRoot() global
    return JDB_solveObj(GetNamespaceKey())
EndFunction

Actor Function GetPlayerRef() global
    return JDB_solveForm(GetNamespaceKey() + ".initialData.playerRef") as Actor
EndFunction


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; NPC Management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Get the npcs object
int Function GetNpcs() global
    return TTLL_JUtils._GetOrCreateJFormMap(GetRoot(), "npcs")
EndFunction

; Get NPC data object
int Function GetNpc(Actor npc, bool createIfEmpty = true) global
    if(!npc)
        return 0
    endif
    int JNpcs = GetNpcs()
    int JNpcObj = JFormMap_getObj(JNpcs, npc)
    if(JNpcObj == 0 && createIfEmpty)
        ; Create new NPC entry if it doesn't exist
        JNpcObj = JMap_object()
        JFormMap_setObj(JNpcs, npc, JNpcObj)
        ActorBase aBase = npc.GetActorBase()
        
        ; Initialize basic properties
        JMap_setStr(JNpcObj, "name", TTLL_Utils.GetActorName(npc))
        JMap_setStr(JNpcObj, "race", aBase.GetRace().GetName())
        JMap_setInt(JNpcObj, "sex", aBase.GetSex())
        
        ; Initialize counters
        JMap_setInt(JNpcObj, "sameSexEncounter", 0)
        JMap_setInt(JNpcObj, "soloSex", 0)
        JMap_setInt(JNpcObj, "exclusiveSex", 0)
        JMap_setInt(JNpcObj, "groupSex", 0)
        JMap_setFlt(JNpcObj, "lastTime", 0.0)
        
        ; Initialize sub-objects
        int actions = JMap_object()
        JMap_setObj(JNpcObj, "actions", actions)
        
        JMap_setObj(actions, "did", JMap_object())
        
        JMap_setObj(actions, "got", JMap_object())
        
        JMap_setObj(actions, "didTopThree", JMap_object())
        
        JMap_setObj(actions, "gotTopThree", JMap_object())
        
        JMap_setObj(JNpcObj, "lovers", JFormMap_object())
        JMap_setObj(JNpcObj, "topThreeLovers", JFormMap_object())

        ; find for npc any existing relationships it should be done once per new npc
        if(npc != GetPlayerRef())
            Actor spouse = TTRF_Store.GetSpouse(npc)
            Actor courting = TTRF_Store.GetCourting(npc)
            bool isLover = npc.GetRelationshipRank(spouse) == 4
            
            if(courting)
                TTLL_Store.CreateExistingLoverWithNpc(npc, courting, false, true, isLover)
            endif

            if(spouse)
                TTLL_Store.CreateExistingLoverWithNpc(npc, spouse, true, false, isLover)
            endif
            
            Form[] lovers = TTRF_Store.GetLovers(npc)
            int i = 0
            while(i < lovers.Length)
                Actor lover = lovers[i] as Actor
                CheckLover(npc, lover, spouse, courting)

                i += 1
            endwhile
        endif

        int internalClimax = JMap_object()
        JMap_setObj(JNpcObj, "totalInternalClimax", internalClimax)
        JMap_setInt(internalClimax, "did", 0)
        JMap_setInt(internalClimax, "got", 0)
    endif

    return JNpcObj
EndFunction

Function CheckLover(Actor npc, Actor lover, Actor spouse, Actor courting) global
    bool isSpouse = spouse == lover
    bool isCourting = courting == lover
    ; spouse and courting already processed at this point
    if(!lover || isSpouse || isCourting)
        return
    endif

    TTLL_Store.CreateExistingLoverWithNpc(npc, lover, false, false, true)
EndFunction

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; NPC Basic Properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Get/Set NPC name
string Function GetNpcName(Actor npc) global
    return JMap_getStr(GetNpc(npc), "name")
EndFunction

Function SetNpcName(Actor npc, string name) global
    JMap_setStr(GetNpc(npc), "name", name)
EndFunction

; Get/Set NPC race
string Function GetNpcRace(Actor npc) global
    return JMap_getStr(GetNpc(npc), "race")
EndFunction

Function SetNpcRace(Actor npc, string raceVal) global
    JMap_setStr(GetNpc(npc), "race", raceVal)
EndFunction

; Get/Set NPC sex
int Function GetNpcSex(Actor npc) global
    return JMap_getInt(GetNpc(npc), "sex")
EndFunction

Function SetNpcSex(Actor npc, int sex) global
    JMap_setInt(GetNpc(npc), "sex", sex)
EndFunction

; Get/Set NPC last time
float Function GetNpcLastTime(Actor npc) global
    return JMap_getFlt(GetNpc(npc), "lastTime")
EndFunction

string Function GetNpcLastTimeString(Actor npc) global
    return Utility.GameTimeToString(GetNpcLastTime(npc))
EndFunction

Function SetNpcLastTime(Actor npc, float time) global
    JMap_setFlt(GetNpc(npc), "lastTime", time)
EndFunction

; Update NPC last time to current game time
Function UpdateNpcLastTime(Actor npc) global
    float currentTime = Utility.GetCurrentGameTime()
    SetNpcLastTime(npc, currentTime)
EndFunction

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; NPC Counter Properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Get/Increment Same Sex Encounter counter
int Function GetSameSexEncounterCount(Actor npc) global
    return JMap_getInt(GetNpc(npc), "sameSexEncounter")
EndFunction

int Function IncrementSameSexEncounterCount(Actor npc) global
    return TTLL_JUtils.JMapIntIncrement(GetNpc(npc), "sameSexEncounter")
EndFunction

; Get/Increment Solo Sex counter
int Function GetSoloSexCount(Actor npc) global
    return JMap_getInt(GetNpc(npc), "soloSex")
EndFunction

int Function IncrementSoloSexCount(Actor npc) global
    return TTLL_JUtils.JMapIntIncrement(GetNpc(npc), "soloSex")
EndFunction

; Get/Increment Exclusive Sex counter
int Function GetExclusiveSexCount(Actor npc) global
    return JMap_getInt(GetNpc(npc), "exclusiveSex")
EndFunction

int Function IncrementExclusiveSexCount(Actor npc, int incr = 1) global
    return TTLL_JUtils.JMapIntIncrement(GetNpc(npc), "exclusiveSex", incr)
EndFunction

; Get/Increment Group Sex counter
int Function GetGroupSexCount(Actor npc) global
    return JMap_getInt(GetNpc(npc), "groupSex")
EndFunction

int Function IncrementGroupSexCount(Actor npc) global
    return TTLL_JUtils.JMapIntIncrement(GetNpc(npc), "groupSex")
EndFunction

; Get/Increment Internal Climax counters
int Function GetNpcInternalClimax(Actor npc) global
    return JMap_getObj(GetNpc(npc), "totalInternalClimax")
EndFunction
int Function GetNpcInternalClimaxDid(Actor npc) global
    return JMap_getInt(GetNpcInternalClimax(npc), "did")
EndFunction
int Function IncrementNpcInternalClimaxDid(Actor npc, int incr = 1) global
    return TTLL_JUtils.JMapIntIncrement(GetNpcInternalClimax(npc), "did", incr)
EndFunction
int Function GetNpcInternalClimaxGot(Actor npc) global
    return JMap_getInt(GetNpcInternalClimax(npc), "got")
EndFunction
int Function IncrementNpcInternalClimaxGot(Actor npc, int incr = 1) global
    return TTLL_JUtils.JMapIntIncrement(GetNpcInternalClimax(npc), "got", incr)
EndFunction

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; NPC Actions Management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Get actions object
int Function GetNpcActions(Actor npc) global
    return JMap_getObj(GetNpc(npc), "actions")
EndFunction

; Get "did" actions object
int Function GetNpcDidActions(Actor npc) global
    return JMap_getObj(GetNpcActions(npc), "did")
EndFunction

; Get "got" actions object
int Function GetNpcGotActions(Actor npc) global
    return JMap_getObj(GetNpcActions(npc), "got")
EndFunction

; Get "didTopThree" actions object
int Function GetNpcDidTopThreeActions(Actor npc) global
    return JMap_getObj(GetNpcActions(npc), "didTopThree")
EndFunction

; Get "gotTopThree" actions object
int Function GetNpcGotTopThreeActions(Actor npc) global
    return JMap_getObj(GetNpcActions(npc), "gotTopThree")
EndFunction

; Record that NPC did an action
Function RecordNpcDidAction(Actor npc, string actionName) global
    int newCount = TTLL_JUtils.JMapIntIncrement(GetNpcDidActions(npc), actionName)
    
    ; Update top three if needed
    UpdateTopThreeActions(npc, "didTopThree", actionName, newCount)
EndFunction

; Record that NPC got an action
Function RecordNpcGotAction(Actor npc, string actionName) global
    int newCount = TTLL_JUtils.JMapIntIncrement(GetNpcGotActions(npc), actionName)
    
    ; Update top three if needed
    UpdateTopThreeActions(npc, "gotTopThree", actionName, newCount)
EndFunction

; Get count of times NPC did an action
int Function GetNpcDidActionCount(Actor npc, string actionName) global
    return JMap_getInt(GetNpcDidActions(npc), actionName)
EndFunction

; Get count of times NPC got an action
int Function GetNpcGotActionCount(Actor npc, string actionName) global
    return JMap_getInt(GetNpcGotActions(npc), actionName)
EndFunction

; Update top three actions
Function UpdateTopThreeActions(Actor npc, string actionType, string actionName, int newCount) global
    int JTopThreeActions
    int JNpcActions = GetNpcActions(npc)
    
    if actionType == "didTopThree"
      JTopThreeActions = JMap_getObj(JNpcActions, "didTopThree")
    else
      JTopThreeActions = JMap_getObj(JNpcActions, "gotTopThree")
    endif

    if(JMap_hasKey(JTopThreeActions, actionName) || JMap_count(JTopThreeActions) < 3)
        JMap_setInt(JTopThreeActions, actionName, newCount)
        return
    endif
 
    int min = 2147483647 
    string minKey
    string keyName = JMap_nextKey(JTopThreeActions, previousKey="", endKey="")
    while(keyName != "")
        int val = JMap_getInt(JTopThreeActions, keyName)
        if(val < min)
            min = val
            minKey = keyName
        endif
        keyName = JMap_nextKey(JTopThreeActions, previousKey=keyName, endKey="")
    endwhile

    if(newCount > min)
        JMap_removeKey(JTopThreeActions, minKey)
        JMap_setInt(JTopThreeActions, actionName, newCount)
    endif
EndFunction

int Function CreateExistingLoverWithNpc(Actor npc, Actor lover, bool isSpouse, bool isCourting, bool isLover) global
    int JLovers = GetNpcLovers(npc)
    
    int JLover = CreateExistingJLoverByActor(npc, lover, isSpouse, isCourting, isLover)
    JFormMap_setObj(JLovers, lover, JLover)

    UpdateTopThreeLovers(npc, lover)

    return JLover
EndFunction

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Lover Management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Get lovers object
int Function GetNpcLovers(Actor npc, bool createIfEmpty = true) global
    int JNpc = GetNpc(npc, createIfEmpty)
    return JMap_getObj(JNpc, "lovers")
EndFunction

int Function CreateNewJLoverByActor(Actor lover) global
    int JLover = JMap_object()
    ActorBase aBase = lover.GetActorBase()
        
    ; Initialize lover data
    JMap_setStr(JLover, "name", TTLL_Utils.GetActorName(lover))
    JMap_setStr(JLover, "race", aBase.GetRace().GetName())
    JMap_setInt(JLover, "sex", aBase.GetSex())
    
    JMap_setInt(JLover, "exclusiveSex", 0)
    JMap_setInt(JLover, "partOfSameGroupSex", 0)
    JMap_setFlt(JLover, "lastTime", 0.0)
    JMap_setFlt(JLover, "orgasms", 0.0)

    int internalClimax = JMap_object()
    JMap_setObj(JLover, "internalClimax", internalClimax)
    JMap_setInt(internalClimax, "did", 0)
    JMap_setInt(internalClimax, "got", 0)
    return JLover
EndFunction

; Create new lover entry for npc with random number based on existing relationship
; Updates target npc numbers as well
; Create npc entry from lover for consistency
int Function CreateExistingJLoverByActor(Actor npc, Actor lover, bool isSpouse, bool isCourting, bool isLover) global
    int JLover = JMap_object()
    ActorBase loverAB = lover.GetActorBase()
    int sexTimes = 0
    float lastTime = Utility.GetCurrentGameTime()
    int didInternal = 0
    int gotInternal = 0
    int sex = loverAB.GetSex()
    bool isMale = sex == 0
    float orgasms = 0.0

    ; if target lover already have target npc as lover take numbers from that object
    if(GetLoverObject(lover, npc, false) != 0)
        sexTimes = GetLoverExclusiveSexCount(lover, npc)
        lastTime = GetLoverLastTime(lover, npc)
        orgasms = GetLoverOrgasms(lover, npc)
        didInternal = GetLoverInternalClimaxDid(lover, npc)
        gotInternal = GetLoverInternalClimaxGot(lover, npc)
    else ; otherwise generate random numbers
        if(isSpouse)
            sexTimes = Utility.RandomInt(40, 100)
        elseif(isCourting)
            sexTimes = Utility.RandomInt(0, 10)
        elseif(isLover)
            sexTimes = Utility.RandomInt(10, 40)
        endif

        if(isLover)
            lastTime -= Utility.RandomInt(1, 6) ; they are active lovers so last time randomly picked in the past week
        else
            lastTime -= Utility.RandomInt(180, 360) ; they aren't lovers anymore(in case of spouses), last time picked from between year and 2 ago
        endif

        orgasms = sexTimes * Utility.RandomFloat(0.7, 1) ; just random amount from all encounters to end by orgasm

        if(isSpouse)
            if(isMale)
                didInternal = (orgasms / 2) as int
            else
                gotInternal = (orgasms / 2) as int
            endif
        endif
    endif

    ; Initialize lover data
    JMap_setStr(JLover, "name", TTLL_Utils.GetActorName(lover))
    JMap_setStr(JLover, "race", loverAB.GetRace().GetName())
    JMap_setInt(JLover, "sex", sex)
    
    JMap_setInt(JLover, "exclusiveSex", sexTimes)
    IncrementExclusiveSexCount(npc, sexTimes)
    
    JMap_setInt(JLover, "partOfSameGroupSex", 0) ; assume existing lovers always exclusive

    JMap_setFlt(JLover, "lastTime", lastTime)
    SetNpcLastTime(npc, lastTime)

    JMap_setFlt(JLover, "orgasms", orgasms)
    
    int internalClimax = JMap_object()
    JMap_setObj(JLover, "internalClimax", internalClimax)
    
    JMap_setInt(internalClimax, "did", didInternal)
    IncrementNpcInternalClimaxDid(npc, didInternal)
    
    JMap_setInt(internalClimax, "got", gotInternal)
    IncrementNpcInternalClimaxGot(npc, gotInternal)

    return JLover
EndFunction

; Get lover data object
int Function GetLoverObject(Actor npc, Actor lover, bool createIfEmpty = true) global
    int JLovers = GetNpcLovers(npc, createIfEmpty)
    int JLover = JFormMap_getObj(JLovers, lover)
    if (JLover == 0 && createIfEmpty)
        JLover = CreateNewJLoverByActor(lover)
        
        JFormMap_setObj(JLovers, lover, JLover)
    endif
    return JLover
EndFunction

Actor Function NextLover(Actor npc, Actor lover = none) global
    Actor nextLover = JFormMap_nextKey(GetNpcLovers(npc), lover) as Actor
    return nextLover
EndFunction

; Get/Set lover basic properties
string Function GetLoverName(Actor npc, Actor lover) global
    return JMap_getStr(GetLoverObject(npc, lover), "name")
EndFunction

string Function GetLoverRace(Actor npc, Actor lover) global
    return JMap_getStr(GetLoverObject(npc, lover), "race")
EndFunction

int Function GetLoverSex(Actor npc, Actor lover) global
    return JMap_getInt(GetLoverObject(npc, lover), "sex")
EndFunction

; Get/Increment lover counters
int Function GetLoverExclusiveSexCount(Actor npc, Actor lover) global
    return JMap_getInt(GetLoverObject(npc, lover), "exclusiveSex")
EndFunction

int Function IncrementLoverExclusiveSexCount(Actor npc, Actor lover) global
    int newCount = TTLL_JUtils.JMapIntIncrement(GetLoverObject(npc, lover), "exclusiveSex")
    UpdateTopThreeLovers(npc, lover)
    return newCount
EndFunction

int Function GetLoverGroupSexCount(Actor npc, Actor lover) global
    return JMap_getInt(GetLoverObject(npc, lover), "partOfSameGroupSex")
EndFunction

int Function IncrementLoverGroupSexCount(Actor npc, Actor lover) global
    int newCount = TTLL_JUtils.JMapIntIncrement(GetLoverObject(npc, lover), "partOfSameGroupSex")
    UpdateTopThreeLovers(npc, lover)
    return newCount
EndFunction

; Get/Set lover last time
float Function GetLoverLastTime(Actor npc, Actor lover) global
    return JMap_getFlt(GetLoverObject(npc, lover), "lastTime")
EndFunction

string Function GetLoverLastTimeString(Actor npc, Actor lover) global
    return Utility.GameTimeToString(GetLoverLastTime(npc, lover))
  EndFunction

Function SetLoverLastTime(Actor npc, Actor lover, float time) global
    JMap_setFlt(GetLoverObject(npc, lover), "lastTime", time)
    UpdateTopThreeLovers(npc, lover)
EndFunction

Function UpdateLoverLastTime(Actor npc, Actor lover) global
    float currentTime = Utility.GetCurrentGameTime()
    SetLoverLastTime(npc, lover, currentTime)
EndFunction

; Get/Add lover orgasms
float Function GetLoverOrgasms(Actor npc, Actor lover) global
    return JMap_getFlt(GetLoverObject(npc, lover), "orgasms")
EndFunction

float Function AddLoverOrgasms(Actor npc, Actor lover, float amount) global
    int JLover = GetLoverObject(npc, lover)
    float newOrgasms = JMap_getFlt(JLover, "orgasms") + amount
    JMap_setFlt(JLover, "orgasms", newOrgasms)
    UpdateTopThreeLovers(npc, lover)
    return newOrgasms
EndFunction

; Get/Set lover internal climax
int Function GetLoverInternalClimax(Actor npc, Actor lover) global
    return JMap_getObj(GetLoverObject(npc, lover), "internalClimax")
EndFunction

int Function GetLoverInternalClimaxDid(Actor npc, Actor lover) global
    return JMap_getInt(GetLoverInternalClimax(npc, lover), "did")
EndFunction

int Function IncrementLoverInternalClimaxDid(Actor npc, Actor lover) global
    int newCount = TTLL_JUtils.JMapIntIncrement(GetLoverInternalClimax(npc, lover), "did")
    IncrementNpcInternalClimaxDid(npc)
    return newCount
EndFunction

int Function GetLoverInternalClimaxGot(Actor npc, Actor lover) global
    return JMap_getInt(GetLoverInternalClimax(npc, lover), "got")
EndFunction

int Function IncrementLoverInternalClimaxGot(Actor npc, Actor lover) global
    int newCount = TTLL_JUtils.JMapIntIncrement(GetLoverInternalClimax(npc, lover), "got")
    IncrementNpcInternalClimaxGot(npc)
    return newCount
EndFunction

;/**
  * Updates the lover properties for an NPC based on the encounter.
  * @param {Actor} npc - The NPC actor.
  * @param {Actor} lover - The lover actor.
  * @param {int} participants - The number of participants in the encounter.
  * @param {float} orgasms - The number of orgasms.
*/;
Function UpdateLover(Actor npc, Actor lover, int participants, float orgasms) global
    int JLover = GetLoverObject(npc, lover)
    string encounterType = TTLL_Utils.GetEncounterType(participants)

    if(encounterType == "couple")
        IncrementLoverExclusiveSexCount(npc, lover)
    elseif(encounterType == "group")
        IncrementLoverGroupSexCount(npc, lover)
    endif
    
    UpdateLoverLastTime(npc, lover)

    AddLoverOrgasms(npc, lover, orgasms)

    TTLL_Utils.SendUpdateLoverDataEvent(npc, lover)
EndFunction 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Top Three Lovers Management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Get top three lovers object
int Function GetNpcTopThreeLovers(Actor npc) global
    return JMap_getObj(GetNpc(npc), "topThreeLovers")
EndFunction

bool Function IsNpcTopThreeLover(Actor npc, Actor lover) global
    return JFormMap_hasKey(GetNpcTopThreeLovers(npc), lover)
EndFunction

; Update top three lovers based on criteria
Function UpdateTopThreeLovers(Actor npc, Actor currentLover) global
    int JNpc = GetNpc(npc)
    int JTopThreeLovers = JMap_getObj(JNpc, "topThreeLovers")
    
    float newFavScore = GetLoverScore(npc, currentLover)

    if(JFormMap_hasKey(JTopThreeLovers, currentLover) || JFormMap_count(JTopThreeLovers) < 3)
        JFormMap_setFlt(JTopThreeLovers, currentLover, newFavScore)
        return ; don't need to proceed
    endif

    Actor lessFavoriteLover
    float lessFavoriteScore = 2147483647
    Actor lover = JFormMap_nextKey(JTopThreeLovers, previousKey=none, endKey=none) as Actor
    while lover != none
        float loverScore = GetLoverScore(npc, lover)

        if(loverScore < lessFavoriteScore)
            lessFavoriteLover = lover
            lessFavoriteScore = loverScore
        endif
        
        lover = JFormMap_nextKey(JTopThreeLovers, previousKey=lover, endKey=none) as Actor
    endwhile

    if(newFavScore > lessFavoriteScore)
        JFormMap_removeKey(JTopThreeLovers, lessFavoriteLover)
        JFormMap_setFlt(JTopThreeLovers, currentLover, newFavScore)
    endif
EndFunction

; Get lover score from top three
float Function GetLoverScore(Actor npc, Actor lover) global
    float orgasms = GetLoverOrgasms(npc, lover)
    int exclSex = GetLoverExclusiveSexCount(npc, lover)
    int groupSex = GetLoverGroupSexCount(npc, lover)
    float lastTime = GetLoverLastTime(npc, lover)
    float currentTime = Utility.GetCurrentGameTime()
    float diff = currentTime - lastTime

    float baseScore = Math.sqrt(exclSex) * 6.0 + Math.sqrt(groupSex) * 2.0 + Math.sqrt(orgasms) * 5.0

    ; Time as a decay multiplier
    float timeMultiplier = 0.0
    if(diff < 1)
        timeMultiplier = 1.0      ; Full value if recent
    elseif(diff < 7)
        timeMultiplier = 0.8      ; 80% value if within a week
    elseif(diff < 30)
        timeMultiplier = 0.6      ; 60% if within a month
    elseif(diff < 180)
        timeMultiplier = 0.3      ; 30% if within 6 months
    elseif(diff < 365)
        timeMultiplier = 0.1      ; 10% if within a year
    else
        timeMultiplier = 0.05     ; Minimal connection after a year
    endif

    return baseScore * timeMultiplier
EndFunction

