scriptname TTLL_OstimIntegration

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
    TTLL_ThreadsCollector.CleanThread(ThreadID)
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
                TTLL_Store.SetLoverInt(npc, Target, "internalClimax.did", TTLL_Store.GetLoverInt(npc, Target, "internalClimax.did") + 1)
                TTLL_Store.SetLoverInt(Target, npc, "internalClimax.got", TTLL_Store.GetLoverInt(Target, npc, "internalClimax.got") + 1)
            EndIf

            i += 1
        EndWhile
    endif

    TTLL_ThreadsCollector.SetActorBool(ThreadID, npc, "orgasmed", true)

    TTLL_ThreadsCollector.ExcitementContributorOrgasm(ThreadID, npc)
EndFunction

Function OStimEnd(int ThreadID) global
    if(TTLL_ThreadsCollector.GetThreadBool(ThreadID, "hadSex"))
        UpdateOnOStimEnd(ThreadID)
    endif
    TTLL_ThreadsCollector.SetThreadBool(ThreadID, "finished", true)
EndFunction

float Function GetExcitement(actor npc, string actionType, string role) global
    int roleInt = 4
    if(role == "actor")
        roleInt = 1
    elseif(role == "target")
        roleInt = 2
    endif
    return OData.GetActionStimulation(roleInt, npc.GetActorBase().GetFormID(), actionType)
EndFunction

Function UpdateExcitementContributorsOnSceneChange(int ThreadID, string actionType, Actor npc, string role, Actor lover) global
    float rate = GetExcitement(npc, actionType, role)
    TTLL_ThreadsCollector.UpdateExcitementRate(ThreadID, npc, lover, rate)
EndFunction

Function GetActions(int ThreadID, Actor npc) global
    string tag = "sexual"
    string sceneId = OThread.GetScene(ThreadID)
    int actorIndex = OThread.GetActorPosition(ThreadID, npc)
    int[] actionsByActor = OMetadata.FindActionsSuperloadCSVv2(sceneId, ParticipantPositionsAny = actorIndex + "", AnyActionTag = tag)

    if(actionsByActor.Length > 0)
        TTLL_ThreadsCollector.SetThreadBool(ThreadID, "hadSex", true)
        TTLL_ThreadsCollector.SetThreadStr(ThreadID, "lastSexualSceneId", OThread.GetScene(ThreadID))
    endif 

    int i = 0

    while i < actionsByActor.Length
        int actionIndex = actionsByActor[i]
        string actionType = OMetadata.GetActionType(sceneId, actionIndex)
        Actor osActor = OThread.GetActor(ThreadID, OMetadata.GetActionActor(sceneId, actionIndex))
        Actor osTarget = OThread.GetActor(ThreadID, OMetadata.GetActionTarget(sceneId, actionIndex))

        UpdateExcitementContributorsOnSceneChange(ThreadID, actionType, osActor, "actor", osTarget)
        UpdateExcitementContributorsOnSceneChange(ThreadID, actionType, osTarget, "target", osActor)
        
        TTLL_ThreadsCollector.SetActorBool(ThreadID, osActor, "did." + actionType, true)
        TTLL_ThreadsCollector.SetActorBool(ThreadID, osTarget, "got." + actionType, true)

        if(osActor.GetActorBase().GetSex() == osTarget.GetActorBase().GetSex() && osActor != osTarget)
            TTLL_ThreadsCollector.SetActorBool(ThreadID, osActor, "hadSameSexEncounter", true)
            TTLL_ThreadsCollector.SetActorBool(ThreadID, osTarget, "hadSameSexEncounter", true)
        endif
        i += 1
    endwhile
EndFunction

;/**
  * Updates the properties for all NPCs involved in a thread when the thread ends.
  * @param {int} ThreadID - The ID of the thread.
*/;
Function UpdateOnOStimEnd(int ThreadID) global
    TTLL_ThreadsCollector.ApplyThreadToLedger(ThreadID)
    TTLL_Utils.SendThreadDataEvent(ThreadID)
EndFunction
