scriptname TTLL_OstimThreadsCollector

import TTLL_JCDomain

;/
Thread object shape:
boolean - represented by 1(true) and 0(false)
{
  "TT_LoversLedger": {
    "threadsCollector": {
      [ThreadID]: { // dynamic key depends on which ostim thread 
        "finished": boolean
        "hadSex": boolean;
        "prevExcitementStarted": float;
        "actors": { // list of actors who participated in thread
          [Actor]: { // dynamic key depends on actor being updated
            "did": { // object which tracks actions current actor did as main actor at least once during thread
              action1: boolean;
            }
            "got": { // object which tracks actions current actor received as target actor at least once during thread
              action1: boolean;
            }
            "orgasmed": 1|0
            "excitementContribution": { contributors: { [Actor]: { rate: float, total: float, orgasms: float } } }
            "hadSameSexEncounter": boolean;
          }
        }
        "lastSexualSceneId": string
      }
    }
  }
}
/;

;/**
 * Retrieves or initializes the global threads collector.
 * 
 * This function attempts to fetch the threads collector object from the database.
 * If the object does not exist, it creates a new one, stores it in the database,
 * and then returns it.
 * 
 * @return int - The identifier of the threads collector object.
*/;
int Function GetThreadsCollector() global
    int res = JDB_solveObj(".TT_LoversLedger.threadsCollector")
    if(!res)
        res = JMap_object()
        JDB_solveObjSetter(".TT_LoversLedger.threadsCollector", res, true)
    endif

    return res
EndFunction

;/**
 * Retrieves or initializes the thread object for a specific thread.
 * 
 * This function attempts to fetch the thread object from the threads collector.
 * If the object does not exist, it creates a new one, stores it in the threads collector,
 * and then returns it.
 * 
 * @param {int} ThreadID - The identifier of the thread.
 * @return int - The identifier of the thread object.
*/;
int Function GetThread(int ThreadID) global
    int JThreadsCollector = GetThreadsCollector()
    int res = JMap_getObj(JThreadsCollector, ThreadID + "")
    if(!res)
        res = JMap_object()
        JMap_setObj(res, "actors", JFormMap_object())
        JMap_setObj(JThreadsCollector, ThreadID + "", res)
    endif

    return res
EndFunction

;/**
* Retrieves the actors involved in a specific thread.
* @param {int} ThreadID - The ID of the thread.
* @returns {int} - The JMap object containing the actors.
/;
int Function GetActors(int ThreadID) global
    int JThread = GetThread(ThreadID)
    return JMap_getObj(JThread, "actors")
EndFunction

Form[] Function GetActorsForms(int ThreadID) global
    int JActors = GetActors(ThreadID)

    return JFormMap_allKeysPArray(JActors)
EndFunction

;/**
* Retrieves or creates the actor object for a specific thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {int} - The JMap object representing the actor.
/;
int Function GetActor(int ThreadID, actor npc) global
    int JActors = GetActors(ThreadID)
    int res = JFormMap_getObj(JActors, npc)
    if(!res)
        res = JMap_object()
        
        JMap_setObj(res, "did", JMap_object())
        JMap_setObj(res, "got", JMap_object())

        int JExcitementContribution = JMap_object()
        JMap_setObj(JExcitementContribution, "contributors", JFormMap_object())

        JMap_setObj(res, "excitementContribution", JExcitementContribution)

        JFormMap_setObj(JActors, npc, res)
    endif
    
    return res
EndFunction

;/**
* Retrieves the "did" object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {int} - The JMap object representing the "did" actions.
/;
int Function getDidObj(int ThreadID, actor npc) global
    return JMap_getObj(GetActor(ThreadID, npc), "did")
EndFunction

;/**
* Retrieves the "got" object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {int} - The JMap object representing the "got" actions.
/;
int Function getGotObj(int ThreadID, actor npc) global
    return JMap_getObj(GetActor(ThreadID, npc), "got")
EndFunction

;/**
* Sets a property in the "did" or "got" object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @param {string} act - The action name.
* @param {bool} did - Whether the action is "did" (true) or "got" (false).
/;
Function SetDidGotProp(int ThreadID, actor npc, string act, bool did) global
    int JActor = GetActor(ThreadID, npc)
    int JObj

    if(did)
        JObj = getDidObj(ThreadID, npc)
    else
        JObj = getGotObj(ThreadID, npc)
    endif

    JMap_setInt(JObj, act, 1)
EndFunction

;/**
* Retrieves a property from the "did" or "got" object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @param {string} act - The action name.
* @param {bool} did - Whether the action is "did" (true) or "got" (false).
* @returns {bool} - True if the property exists and is set to 1, otherwise false.
/;
bool Function GetDidGotProp(int ThreadID, actor npc, string act, bool did) global
    int JActor = GetActor(ThreadID, npc)
    int JObj

    if(did)
        JObj = getDidObj(ThreadID, npc)
    else
        JObj = getGotObj(ThreadID, npc)
    endif
    return JMap_getInt(JObj, act) == 1
EndFunction

;/**
* Sets a simple boolean property for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @param {string} propName - The name of the property to set.
/;
Function SetSimpleBool(int ThreadID, actor npc, string propName) global
    int JActor = GetActor(ThreadID, npc)

    JMap_setInt(JActor, propName, 1)
EndFunction

;/**
* Retrieves a simple boolean property for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @param {string} propName - The name of the property to retrieve.
* @returns {bool} - True if the property exists and is set to 1, otherwise false.
/;
bool Function GetSimpleBool(int ThreadID, actor npc, string propName) global
    int JActor = GetActor(ThreadID, npc)

    return JMap_getInt(JActor, propName, 0) == 1
EndFunction

;/**
* Marks an actor as having orgasmed in a specific thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
/;
Function SetOrgasmed(int ThreadID, actor npc) global
    SetSimpleBool(ThreadID, npc, "orgasmed")
EndFunction 

;/**
* Checks if an actor has orgasmed in a specific thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {bool} - True if the actor has orgasmed, otherwise false.
/;
bool Function GetOrgasmed(int ThreadID, actor npc) global
    return GetSimpleBool(ThreadID, npc, "orgasmed")
EndFunction 

Function SetHadSameSexEncounter(int ThreadID, Actor npc) global
    SetSimpleBool(ThreadID, npc, "hadSameSexEncounter")
EndFunction

bool Function GetHadSameSexEncounter(int ThreadID, Actor npc) global
    return GetSimpleBool(ThreadID, npc, "hadSameSexEncounter")
EndFunction

;/**
* Sets the encounter type for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @param {string} encounterType - The encounter type to set.
/;
Function SetEncounterType(int ThreadID, actor npc, string encounterType)
    int JActor = GetActor(ThreadID, npc)

    JMap_setStr(JActor, "encounterType", encounterType)
EndFunction

;/**
* Retrieves the encounter type for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {string} - The encounter type.
/;
string Function GetEncounterType(int ThreadID, actor npc)
    int JActor = GetActor(ThreadID, npc)

    return JMap_getStr(JActor, "encounterType")
EndFunction

;/**
* Cleans up the thread object for a specific thread.
* @param {int} ThreadID - The ID of the thread.
/;
Function CleanThread(int ThreadID) global
    int JThreadsCollection = GetThreadsCollector()
    JMap_setObj(JThreadsCollection, ThreadID + "", 0)
EndFunction

Function CleanFinishedThreads() global
    int JCollector = GetThreadsCollector()

    string nextThread = JMap_nextKey(JCollector)

    while(nextThread)
        if(GetFinished(nextThread as int))
            CleanThread(nextThread as int)
        endif
        nextThread = JMap_nextKey(JCollector, nextThread)
    endwhile
EndFunction

Function SetHadSex(int ThreadID) global
    int JThread = GetThread(ThreadID)

    JMap_setInt(JThread, "hadSex", 1)
EndFunction

bool Function GetHadSex(int ThreadID) global
    int JThread = GetThread(ThreadID)

    return JMap_getInt(JThread, "hadSex") == 1
EndFunction

Function SetFinished(int ThreadID, int value) global
    int JThread = GetThread(ThreadID)

    JMap_setInt(JThread, "finished", value)
EndFunction

bool Function GetFinished(int ThreadID) global
    int JThread = GetThread(ThreadID)

    return JMap_getInt(JThread, "finished") == 1
EndFunction

Function SetLastSexualSceneId(int ThreadID, string sceneId) global
    int JThread = GetThread(ThreadID)

    JMap_setStr(JThread, "lastSexualSceneId", sceneId)
EndFunction

string Function GetLastSexualSceneId(int ThreadID) global
    int JThread = GetThread(ThreadID)

    return JMap_getStr(JThread, "lastSexualSceneId")
EndFunction

;/**
* Retrieves the excitement contribution object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {int} - The JMap object representing the excitement contribution.
/;
float Function GetExcitement(actor npc, string actionType, string role) global
    int roleInt = 4
    if(role == "actor")
        roleInt = 1
    elseif(role == "target")
        roleInt = 2
    endif
    return OData.GetActionStimulation(roleInt, npc.GetActorBase().GetFormID(), actionType)
EndFunction

;/**
* Retrieves the excitement contribution object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {int} - The JMap object representing the excitement contribution.
/;
int Function GetExcitementContribution(int ThreadID, Actor npc) global
    int JNpc = GetActor(ThreadID, npc)

    return JMap_getObj(JNpc, "excitementContribution")
EndFunction

;/**
* Sets the time at which the excitement contribution started for a specific thread.
* @param {int} ThreadID - The ID of the thread.
/;
Function SetExcitementContributionStartedAt(int ThreadID) global
    int JThread = GetThread(ThreadID)

    JMap_setFlt(JThread, "prevExcitementStarted", Utility.GetCurrentRealTime())
EndFunction

;/**
* Retrieves the time at which the excitement contribution started for a specific thread.
* @param {int} ThreadID - The ID of the thread.
* @returns {float} - The time at which the excitement contribution started.
/;
float Function GetExcitementContributionStartedAt(int ThreadID) global
    int JThread = GetThread(ThreadID)

    return JMap_getFlt(JThread, "prevExcitementStarted")
EndFunction

;/**
* Retrieves the excitement contributors object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @returns {int} - The JMap object representing the excitement contributors.
/;
int Function GetExcitementContributors(int ThreadID, Actor npc) global
    int JExcitementContribution = GetExcitementContribution(ThreadID, npc)

    return JMap_getObj(JExcitementContribution, "contributors")
EndFunction

;/**
* Retrieves the excitement contributor object for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @param {actor} lover - The lover actor.
* @returns {int} - The JMap object representing the excitement contributor.
/;
int Function GetExcitementContributor(int ThreadID, Actor npc, Actor lover) global
    int JExcitementContributors = GetExcitementContributors(ThreadID, npc)
    int res = JFormMap_getObj(JExcitementContributors, lover)

    if(!res)
        res = JMap_object()
        JMap_setFlt(res, "rate", 0)
        JMap_setFlt(res, "total", 0)
        JFormMap_setObj(JExcitementContributors, lover, res)
    endif

    return res
EndFunction

;/**
* Updates the excitement contribution for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
* @param {actor} lover - The lover actor.
* @param {float} rate - The excitement contribution rate.
/;
int Function UpdateExcitementContributor(int ThreadID, Actor npc, Actor lover, float rate) global
    int JExcitementContributors = GetExcitementContributors(ThreadID, npc)
    int JExcitementContributor = GetExcitementContributor(ThreadID, npc, lover)
    float currentRate = JMap_getFlt(JExcitementContributor, "rate")

    if(currentRate)
        float elapsed = Utility.GetCurrentRealTime() - GetExcitementContributionStartedAt(ThreadID)
        JMap_setFlt(JExcitementContributor, "total", JMap_getFlt(JExcitementContributor, "total") + currentRate * elapsed)
    endif

    JMap_setFlt(JExcitementContributor, "rate", rate)
EndFunction

;/**
* Distributes the orgasm contributions among the excitement contributors for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {actor} npc - The NPC actor.
/;
Function ExcitementContributorOrgasm(int ThreadID, Actor npc) global
    int JExcitementContributors = GetExcitementContributors(ThreadID, npc)

    Actor contributor = JFormMap_nextKey(JExcitementContributors, previousKey=none, endKey=none) as Actor
    float totalContribution = 0

    ; Calculate total contribution once
    while(contributor != none)
        totalContribution += JMap_getFlt(GetExcitementContributor(ThreadID, npc, contributor), "total")
        contributor = JFormMap_nextKey(JExcitementContributors, previousKey=contributor, endKey=none) as Actor
    endwhile

    if(totalContribution == 0)
        return
    endif

    ; Distribute orgasm contributions
    contributor = JFormMap_nextKey(JExcitementContributors, previousKey=none, endKey=none) as Actor
    while(contributor != none)
        int JExcitementContributor = GetExcitementContributor(ThreadID, npc, contributor)
        float contributorEffort = JMap_getFlt(JExcitementContributor, "total")
        JMap_setFlt(JExcitementContributor, "orgasms", JMap_getFlt(JExcitementContributor, "orgasms") + (contributorEffort / totalContribution))
        JMap_setFlt(JExcitementContributor, "total", 0)
        JMap_setFlt(JExcitementContributor, "rate", 0)
        contributor = JFormMap_nextKey(JExcitementContributors, previousKey=contributor, endKey=none) as Actor
    endwhile
EndFunction

;/**
* Updates the excitement contributors when the scene changes for a specific actor in a thread.
* @param {int} ThreadID - The ID of the thread.
* @param {string} actionType - The action type.
* @param {actor} npc - The NPC actor.
* @param {string} role - The role of the actor.
* @param {actor} lover - The lover actor.
/;
Function UpdateExcitementContributorsOnSceneChange(int ThreadID, string actionType, Actor npc, string role, Actor lover) global
    SetExcitementContributionStartedAt(ThreadID)
    float rate = GetExcitement(npc, actionType, role)
    UpdateExcitementContributor(ThreadID, npc, lover, rate)
EndFunction