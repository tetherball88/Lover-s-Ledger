scriptname TTLL_OstimIntegration

import TTLL_JCDomain

Function OStimManager(string EventName, string StrArg, float ThreadID, Form Sender) global
    if (eventName == "ostim_thread_start")
        OStimStart(ThreadID as int)
    elseif (eventName == "ostim_thread_scenechanged")
        OStimSceneChanged(ThreadID as int)
    elseif (eventName == "ostim_actor_orgasm")   
        Actor npc = Sender as Actor
        OStimOrgasm(ThreadID as int, npc)
    elseif (eventName == "ostim_thread_end")
        OStimEnd(ThreadID as int)
    endif
EndFunction

Function OStimStart(int ThreadID) global
    TTLL_OstimThreadsCollector.CleanThread(ThreadID)
EndFunction

Function OStimSceneChanged(int ThreadID) global
    Actor[] actors = OThread.GetActors(ThreadID)
    int i = 0
    while i < actors.Length
        GetActions(ThreadID, actors[i])
        i += 1
    endwhile
EndFunction

Function OStimOrgasm(int ThreadID, Actor npc) global
    ; those who can cum inside
    if OActor.HasSchlong(npc)
        string sceneId = OThread.GetScene(ThreadID)
        int actorPos = OThread.GetActorPosition(ThreadID, npc)
        int[] Actions = OMetadata.FindActionsSuperloadCSVv2(sceneId, ActorPositions = actorPos, AnyCustomStringListRecord = ";cum")
        int i = 0

        While (i < Actions.Length)
            String[] Slots = OMetadata.GetCustomActionTargetStringList(sceneId, Actions[i], "cum") 
            Actor Target = OThread.GetActor(ThreadID, OMetadata.GetActionTarget(SceneID, Actions[i]))
            bool hasVaginal = false
            int j = 0

            while j < Slots.Length
                if (Slots[j] == "vagina")
                    hasVaginal = true
                endif
                j += 1
            endwhile

            If (hasVaginal)
                TTLL_Store.IncrementLoverInternalClimaxDid(npc, Target)
                TTLL_Store.IncrementLoverInternalClimaxGot(Target, npc)
            EndIf

            i += 1
        EndWhile
    endif

    TTLL_OstimThreadsCollector.SetOrgasmed(ThreadID, npc)

    TTLL_OstimThreadsCollector.ExcitementContributorOrgasm(ThreadID, npc)
EndFunction

Function OStimEnd(int ThreadID) global
    if(TTLL_OstimThreadsCollector.GetHadSex(ThreadID))
        UpdateOnOStimEnd(ThreadID)
    endif
    TTLL_OstimThreadsCollector.SetFinished(ThreadID, 1)
EndFunction

Function GetActions(int ThreadID, Actor npc) global
    string tag = "sexual"
    string sceneId = OThread.GetScene(ThreadID)
        int actorIndex = OThread.GetActorPosition(ThreadID, npc)
    int[] actionsByActor = OMetadata.FindActionsSuperloadCSVv2(sceneId, ParticipantPositionsAny = actorIndex + "", AnyActionTag = tag)

    if(actionsByActor.Length > 0)
        TTLL_OstimThreadsCollector.SetHadSex(ThreadID)
        TTLL_OstimThreadsCollector.SetLastSexualSceneId(ThreadID, OThread.GetScene(ThreadID))
    endif 

    int i = 0

    while i < actionsByActor.Length
        int actionIndex = actionsByActor[i]
        string actionType = OMetadata.GetActionType(sceneId, actionIndex)
        Actor osActor = OThread.GetActor(ThreadID, OMetadata.GetActionActor(sceneId, actionIndex))
        Actor osTarget = OThread.GetActor(ThreadID, OMetadata.GetActionTarget(sceneId, actionIndex))

        TTLL_OstimThreadsCollector.UpdateExcitementContributorsOnSceneChange(ThreadID, actionType, osActor, "actor", osTarget)
        TTLL_OstimThreadsCollector.UpdateExcitementContributorsOnSceneChange(ThreadID, actionType, osTarget, "target", osActor)
        
        TTLL_OstimThreadsCollector.SetDidGotProp(ThreadID, osActor, actionType, true)
        TTLL_OstimThreadsCollector.SetDidGotProp(ThreadID, osTarget, actionType, false)

        if(osActor.GetActorBase().GetSex() == osTarget.GetActorBase().GetSex())
            TTLL_OstimThreadsCollector.SetHadSameSexEncounter(ThreadID, osActor)
            TTLL_OstimThreadsCollector.SetHadSameSexEncounter(ThreadID, osTarget)
        endif
        i += 1
    endwhile
EndFunction

;/**
  * Updates the properties for all NPCs involved in a thread when the thread ends.
  * @param {int} ThreadID - The ID of the thread.
*/;
Function UpdateOnOStimEnd(int ThreadID) global
    int JActors = TTLL_OstimThreadsCollector.GetActors(ThreadID)
    int actorsCount = JFormMap_count(JActors)

    string encounterType = TTLL_Utils.GetEcnounterType(actorsCount)

    CountThreadActions(ThreadID)

    Actor npc = JFormMap_nextKey(JActors, previousKey=none, endKey=none) as Actor

    while npc != none
        TTLL_Store.UpdateNpcLastTime(npc)

        if(encounterType == "couple")
            TTLL_Store.IncrementExclusiveSexCount(npc)
        elseif(encounterType == "group")
            TTLL_Store.IncrementGroupSexCount(npc)
        elseif(encounterType == "solo")
            TTLL_Store.IncrementSoloSexCount(npc)
        endif

        bool hadSameSexEncounter = TTLL_OstimThreadsCollector.GetHadSameSexEncounter(ThreadID, npc)

        if(hadSameSexEncounter)
            TTLL_Store.IncrementSameSexEncounterCount(npc)
        endif

        int JExcitementContributors = TTLL_OstimThreadsCollector.GetExcitementContributors(ThreadID, npc)
        Actor contributor = JFormMap_nextKey(JExcitementContributors, previousKey=none, endKey=none) as Actor
        
        while contributor != none
            if(contributor != npc)
                int JContributor = TTLL_OstimThreadsCollector.GetExcitementContributor(ThreadID, npc, contributor)
                TTLL_Store.UpdateLover(npc, contributor, JFormMap_count(JActors), JMap_getFlt(JContributor, "orgasms"))
            endif
            
            contributor = JFormMap_nextKey(JExcitementContributors, previousKey=contributor, endKey=none) as Actor
        endwhile
        
        npc = JFormMap_nextKey(JActors, previousKey=npc, endKey=none) as Actor
    endwhile

    TTLL_Utils.SendThreadDataEvent(ThreadID)
EndFunction

;/**
  * Updates the actions for all NPCs involved in a thread.
  * @param {int} ThreadID - The ID of the thread.
*/;
Function CountThreadActions(int ThreadID) global
    int OStimThreadActorsData = TTLL_OstimThreadsCollector.GetActors(ThreadID)

    Actor npc = JFormMap_nextKey(OStimThreadActorsData, previousKey=none, endKey=none) as Actor
    while npc != none
        CountActions(npc, ThreadID, "did")
        CountActions(npc, ThreadID, "got")
        
        npc = JFormMap_nextKey(OStimThreadActorsData, previousKey=npc, endKey=none) as Actor
    endwhile
EndFunction

;/**
  * Counts the actions for an NPC based on the type and thread actions.
  * @param {actor} npc - The NPC actor.
  * @param {string} type - The type of actions ("did" or "got").
  * @param {int} JThreadActions - The JMap object containing the thread actions.
*/;
int Function CountActions(actor npc, int ThreadID, string actionType) global
    int JThreadActions
    int JNpcActions

    if(actionType == "did")
        JThreadActions = TTLL_OstimThreadsCollector.getDidObj(ThreadID, npc)
    else
        JThreadActions = TTLL_OstimThreadsCollector.getGotObj(ThreadID, npc)
    endif

    string threadAction = JMap_nextKey(JThreadActions, previousKey="", endKey="")
    while(threadAction != "")
        if(actionType == "did")
            TTLL_Store.RecordNpcDidAction(npc, threadAction)
        else
            TTLL_Store.RecordNpcGotAction(npc, threadAction)
        endif

        threadAction = JMap_nextKey(JThreadActions, previousKey=threadAction, endKey="")
    endwhile
EndFunction