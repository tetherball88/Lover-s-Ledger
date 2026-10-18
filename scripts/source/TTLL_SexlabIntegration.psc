scriptname TTLL_SexlabIntegration

; SexLab (AE and P+) feeder for TTLL_ThreadsCollector.
; SexLab tids 0-14 are stored under ThreadsCollector key -1 - tid (-1..-15) so they never collide with OStim ids.
; Event handlers and the ChangeActors restart flags live on TTLL_MainController.
;
; Per-tid StorageUtil lists (on None, prefix TTLL_SL_):
;   TG_/TR_/TT_/TX_ : current stage acts (giver, receiver, action type, receiver rate)
;   EA_/EL_/ER_     : excitement pairs rated this stage (stimulated actor, lover, rate)
;   PA_/PL_         : excitement pairs rated last stage
;   ENG_ (int)      : 1 when this thread follows SkyrimNet_SexLab's OrgasmEngine (latched at StartThread)
;   YA_/YL_/YS_     : engine threads: enjoyment gain banked per pair this scene (stimulated actor, lover, share)
;   EB_/EV_         : engine threads: last enjoyment sample per actor (actor, value)

int Function Key(int tid) global
    return -1 - tid
EndFunction

bool Function IsPPlus() global
    return SKSE.GetPluginVersion("SexLabUtil") > 20000
EndFunction

Function Maintenance(TTLL_MainController mainController, SexLabFramework slf) global
    mainController.UnregisterForModEvent("HookAnimationStarting")
    mainController.RegisterForModEvent("HookAnimationStarting", "SLAnimationStarting")
    mainController.UnregisterForModEvent("HookAnimationStart")
    mainController.RegisterForModEvent("HookAnimationStart", "SLAnimationStart")
    mainController.UnregisterForModEvent("HookStageStart")
    mainController.RegisterForModEvent("HookStageStart", "SLStageStart")
    mainController.UnregisterForModEvent("HookAnimationChange")
    mainController.RegisterForModEvent("HookAnimationChange", "SLAnimationChange")
    mainController.UnregisterForModEvent("HookActorChangeStart")
    mainController.RegisterForModEvent("HookActorChangeStart", "SLActorChangeStart")
    mainController.UnregisterForModEvent("HookActorChangeEnd")
    mainController.RegisterForModEvent("HookActorChangeEnd", "SLActorChangeEnd")
    mainController.UnregisterForModEvent("HookAnimationEnd")
    mainController.RegisterForModEvent("HookAnimationEnd", "SLAnimationEnd")
    ; Once per actor orgasm on both builds; HookOrgasmStart / SexLabOrgasmSeparate would double count
    mainController.UnregisterForModEvent("SexLabOrgasm")
    mainController.RegisterForModEvent("SexLabOrgasm", "SLOrgasm")
    ; SkyrimNet_SexLab OrgasmEngine: one event per counted orgasm; EngineOwns picks which of the two counts
    mainController.UnregisterForModEvent("SkyrimNet_SexLab_Orgasm")
    mainController.RegisterForModEvent("SkyrimNet_SexLab_Orgasm", "SLNetOrgasm")

    ; Collector data is runtime only; the next StageStart rebuilds the lists
    StorageUtil.ClearAllPrefix("TTLL_SL_")
EndFunction

; ========================================================================================
; Thread lifecycle
; ========================================================================================

Function StartThread(int tid) global
    TTLL_ThreadsCollector.CleanThread(Key(tid))
    ClearLists(tid)
    ; The engine applies engine_disabled at its scene begin; a mid-scene toggle does not change this scene
    if TTLL_SkyrimNetSexLab.Installed() && !TTLL_SkyrimNetSexLab.EngineDisabled()
        StorageUtil.SetIntValue(none, "TTLL_SL_ENG_" + tid, 1)
    endif
EndFunction

Function OnEnd(int tid) global
    int threadKey = Key(tid)
    if(TTLL_ThreadsCollector.GetThreadBool(threadKey, "hadSex"))
        TTLL_ThreadsCollector.ApplyThreadToLedger(threadKey)
        TTLL_Utils.SendThreadDataEvent(threadKey)
    endif
    TTLL_ThreadsCollector.SetThreadBool(threadKey, "finished", true)
    ClearLists(tid)
EndFunction

Function ClearLists(int tid) global
    ClearActs(tid)
    StorageUtil.FormListClear(none, "TTLL_SL_EA_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_EL_" + tid)
    StorageUtil.FloatListClear(none, "TTLL_SL_ER_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_PA_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_PL_" + tid)
    StorageUtil.UnsetIntValue(none, "TTLL_SL_ENG_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_YA_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_YL_" + tid)
    StorageUtil.FloatListClear(none, "TTLL_SL_YS_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_EB_" + tid)
    StorageUtil.FloatListClear(none, "TTLL_SL_EV_" + tid)
EndFunction

Function ClearActs(int tid) global
    StorageUtil.FormListClear(none, "TTLL_SL_TG_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_TR_" + tid)
    StorageUtil.StringListClear(none, "TTLL_SL_TT_" + tid)
    StorageUtil.FloatListClear(none, "TTLL_SL_TX_" + tid)
EndFunction

; ========================================================================================
; Stage processing (idempotent: AnimationStart, StageStart and AnimationChange all call it)
; ========================================================================================

Function ProcessStage(SexLabFramework slf, int tid) global
    sslThreadController thread = slf.GetController(tid)
    if !thread
        return
    endif
    Actor[] positions = thread.Positions
    if positions.Length == 0
        return
    endif
    bool pplus = IsPPlus()

    ; Bank the gain for the pairs that were acting, before ApplyActs replaces them
    SampleThread(tid)
    ClearActs(tid)
    if positions.Length == 1
        AddSoloAct(slf, thread, tid, positions[0], pplus)
    else
        bool usedInteractions = false
        if pplus && TTLL_SexlabPPlus.IsInteractionRegistered(thread)
            usedInteractions = AddInteractionActs(thread, tid, positions)
        endif
        AddTagActs(slf, thread, tid, positions, pplus, usedInteractions)
    endif

    ApplyActs(slf, thread, tid, pplus)
EndFunction

Function AddAct(int tid, Actor giver, Actor receiver, string actionType, float rate) global
    if !giver || !receiver
        return
    endif
    ; One row per (giver, receiver, action): duplicates would sum into an outsized rate
    int count = StorageUtil.FormListCount(none, "TTLL_SL_TG_" + tid)
    int i = 0
    while i < count
        if StorageUtil.FormListGet(none, "TTLL_SL_TG_" + tid, i) == giver && StorageUtil.FormListGet(none, "TTLL_SL_TR_" + tid, i) == receiver && StorageUtil.StringListGet(none, "TTLL_SL_TT_" + tid, i) == actionType
            return
        endif
        i += 1
    endwhile
    StorageUtil.FormListAdd(none, "TTLL_SL_TG_" + tid, giver)
    StorageUtil.FormListAdd(none, "TTLL_SL_TR_" + tid, receiver)
    StorageUtil.StringListAdd(none, "TTLL_SL_TT_" + tid, actionType)
    StorageUtil.FloatListAdd(none, "TTLL_SL_TX_" + tid, rate)
EndFunction

Function AddSoloAct(SexLabFramework slf, sslThreadController thread, int tid, Actor npc, bool pplus) global
    if !HasTag(thread, pplus, "Masturbation") && !HasTag(thread, pplus, "Solo")
        return
    endif
    if HasPenis(slf, npc, pplus)
        AddAct(tid, npc, npc, "malemasturbation", 1.0)
    else
        AddAct(tid, npc, npc, "femalemasturbation", 1.0)
    endif
EndFunction

; P+ collision data. GetInteractionTypes(position, partner) describes what position does with partner.
bool Function AddInteractionActs(sslThreadController thread, int tid, Actor[] positions) global
    bool found = false
    int i = 0
    while i < positions.Length
        int j = 0
        while j < positions.Length
            if i != j
                Actor pos = positions[i]
                Actor partner = positions[j]
                int[] types = TTLL_SexlabPPlus.GetInteractionTypes(thread, pos, partner)
                int t = 0
                while t < types.Length
                    int ctype = types[t]
                    if ctype == 1 ; position is penetrated vaginally
                        AddAct(tid, partner, pos, "vaginalsex", 1.0)
                        found = true
                    elseif ctype == 2 ; position is penetrated anally
                        AddAct(tid, partner, pos, "analsex", 1.0)
                        found = true
                    elseif ctype == 3 ; position licks/sucks partner
                        int partnerSex = TTLL_SexlabPPlus.GetSex(partner)
                        if partnerSex == 1 || partnerSex == 4
                            AddAct(tid, pos, partner, "cunnilingus", 0.8)
                        else
                            AddAct(tid, pos, partner, "blowjob", 0.8)
                        endif
                        found = true
                    elseif ctype == 4 ; grinding, mutual: record the pair once
                        if i < j || TTLL_SexlabPPlus.GetInteractionTypes(thread, partner, pos).Find(4) < 0
                            AddAct(tid, partner, pos, "tribbing", 0.8)
                            AddAct(tid, pos, partner, "tribbing", 0.8)
                        endif
                        found = true
                    elseif ctype == 5
                        AddAct(tid, pos, partner, "deepthroat", 0.8)
                        found = true
                    elseif ctype == 7
                        AddAct(tid, pos, partner, "blowjob", 0.8)
                        found = true
                    elseif ctype == 8
                        AddAct(tid, pos, partner, "footjob", 0.5)
                        found = true
                    elseif ctype == 9
                        AddAct(tid, pos, partner, "handjob", 0.5)
                        found = true
                    endif
                    ; 6 skullfuck, 10 kissing, 11 facial, 12 anim object, 13 toes: not recorded
                    t += 1
                endwhile
            endif
            j += 1
        endwhile
        i += 1
    endwhile
    return found
EndFunction

; Tag convention: position 0 receives, positions 1+ give toward position 0.
; With P+ interactions, only acts that have no CTYPE are read from tags.
Function AddTagActs(SexLabFramework slf, sslThreadController thread, int tid, Actor[] positions, bool pplus, bool usedInteractions) global
    Actor receiver = positions[0]
    Actor partner = positions[1]
    int i

    if !usedInteractions
        bool matched = false
        if HasTag(thread, pplus, "Vaginal")
            string penetration = "analsex"
            if HasVagina(slf, receiver, pplus)
                penetration = "vaginalsex"
            endif
            AddPenetrationActs(slf, thread, tid, positions, penetration, pplus)
            matched = true
        endif
        if HasTag(thread, pplus, "Anal")
            AddPenetrationActs(slf, thread, tid, positions, "analsex", pplus)
            matched = true
        endif

        bool blowjob = HasTag(thread, pplus, "Blowjob")
        bool cunnilingus = HasTag(thread, pplus, "Cunnilingus")
        if blowjob
            AddAct(tid, receiver, partner, "blowjob", 0.8)
        endif
        if cunnilingus
            ; 3+ positions: a vagina at 1+ is licked by another position 1+ that is not penetrating position 0
            int licked = -1
            int licker = -1
            if positions.Length > 2
                i = 1
                while i < positions.Length && licked < 0
                    if HasVagina(slf, positions[i], pplus)
                        licked = i
                    endif
                    i += 1
                endwhile
                i = 1
                while licked >= 0 && i < positions.Length && licker < 0
                    if i != licked && !IsActGiver(tid, positions[i], positions[0])
                        licker = i
                    endif
                    i += 1
                endwhile
            endif
            if licked >= 0 && licker >= 0
                AddAct(tid, positions[licker], positions[licked], "cunnilingus", 0.8)
            else
                AddAct(tid, partner, receiver, "cunnilingus", 0.8)
            endif
        endif
        if !blowjob && !cunnilingus && HasTag(thread, pplus, "Oral")
            if HasPenis(slf, partner, pplus)
                AddAct(tid, receiver, partner, "blowjob", 0.8)
            else
                AddAct(tid, partner, receiver, "cunnilingus", 0.8)
            endif
            matched = true
        endif
        matched = matched || blowjob || cunnilingus

        if HasTag(thread, pplus, "Handjob")
            AddAct(tid, receiver, partner, "handjob", 0.5)
            matched = true
        endif
        if HasTag(thread, pplus, "Footjob")
            AddAct(tid, receiver, partner, "footjob", 0.5)
            matched = true
        endif

        if !matched && (HasTag(thread, pplus, "Tribadism") || HasTag(thread, pplus, "Grinding") || HasTag(thread, pplus, "Lesbian"))
            AddAct(tid, partner, receiver, "tribbing", 0.8)
            AddAct(tid, receiver, partner, "tribbing", 0.8)
        endif
    endif

    ; No CTYPE exists for these
    if HasTag(thread, pplus, "Boobjob")
        AddAct(tid, receiver, partner, "boobjob", 0.5)
    endif
    if HasTag(thread, pplus, "Fisting")
        AddAct(tid, partner, receiver, "vaginalfisting", 0.6)
    elseif HasTag(thread, pplus, "Fingering")
        AddAct(tid, partner, receiver, "vaginalfingering", 0.6)
    endif
EndFunction

; Positions 1+ wearing a strap-on penetrate position 0; else those with a penis; with none, position 1 does
Function AddPenetrationActs(SexLabFramework slf, sslThreadController thread, int tid, Actor[] positions, string penetration, bool pplus) global
    bool found = false
    int i = 1
    while i < positions.Length
        if UsesStrapon(thread, positions[i], pplus)
            AddAct(tid, positions[i], positions[0], penetration, 1.0)
            found = true
        endif
        i += 1
    endwhile
    i = 1
    while !found && i < positions.Length
        if HasPenis(slf, positions[i], pplus)
            AddAct(tid, positions[i], positions[0], penetration, 1.0)
            found = true
        endif
        i += 1
    endwhile
    if !found
        AddAct(tid, positions[1], positions[0], penetration, 1.0)
    endif
EndFunction

Function ApplyActs(SexLabFramework slf, sslThreadController thread, int tid, bool pplus) global
    int threadKey = Key(tid)
    MoveExcitementToPrevious(tid)

    int count = StorageUtil.FormListCount(none, "TTLL_SL_TG_" + tid)
    int i = 0
    if count > 0
        TTLL_ThreadsCollector.SetThreadBool(threadKey, "hadSex", true)
        TTLL_ThreadsCollector.SetThreadStr(threadKey, "lastSexualSceneId", SceneId(thread, pplus))
        ; Positions are participants even without an act (2-position tag acts in a 3-way): the actor count decides exclusive / group.
        ; Setting anatomy creates the actor and gives first-time partner traits SexLab's gender (override, futa).
        Actor[] positions = thread.Positions
        while i < positions.Length
            TTLL_ThreadsCollector.SetActorInt(threadKey, positions[i], "anatomy", Anatomy(slf, positions[i], pplus))
            i += 1
        endwhile
    endif

    i = 0
    while i < count
        Actor giver = StorageUtil.FormListGet(none, "TTLL_SL_TG_" + tid, i) as Actor
        Actor receiver = StorageUtil.FormListGet(none, "TTLL_SL_TR_" + tid, i) as Actor
        string actionType = StorageUtil.StringListGet(none, "TTLL_SL_TT_" + tid, i)
        float rate = StorageUtil.FloatListGet(none, "TTLL_SL_TX_" + tid, i)

        TTLL_ThreadsCollector.SetActorBool(threadKey, giver, "did." + actionType, true)
        TTLL_ThreadsCollector.SetActorBool(threadKey, receiver, "got." + actionType, true)

        if giver != receiver
            ; SexLab gender (faction override, P+ futa), the same rule the act mapping uses
            if HasPenis(slf, giver, pplus) == HasPenis(slf, receiver, pplus) && HasVagina(slf, giver, pplus) == HasVagina(slf, receiver, pplus)
                TTLL_ThreadsCollector.SetActorBool(threadKey, giver, "hadSameSexEncounter", true)
                TTLL_ThreadsCollector.SetActorBool(threadKey, receiver, "hadSameSexEncounter", true)
            endif
            ; Both directions need a contributor, or the giver gets no lover row
            AddExcitement(tid, receiver, giver, rate)
            AddExcitement(tid, giver, receiver, GiverRate(actionType, rate))
        endif
        i += 1
    endwhile

    ApplyExcitement(tid)
EndFunction

float Function GiverRate(string actionType, float rate) global
    if actionType == "vaginalsex" || actionType == "analsex" || actionType == "tribbing"
        return rate
    endif
    return 0.2
EndFunction

; ========================================================================================
; Excitement: rate only pairs that act; zero only pairs rated last stage
; ========================================================================================

Function MoveExcitementToPrevious(int tid) global
    StorageUtil.FormListClear(none, "TTLL_SL_PA_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_PL_" + tid)
    int count = StorageUtil.FormListCount(none, "TTLL_SL_EA_" + tid)
    int i = 0
    while i < count
        StorageUtil.FormListAdd(none, "TTLL_SL_PA_" + tid, StorageUtil.FormListGet(none, "TTLL_SL_EA_" + tid, i))
        StorageUtil.FormListAdd(none, "TTLL_SL_PL_" + tid, StorageUtil.FormListGet(none, "TTLL_SL_EL_" + tid, i))
        i += 1
    endwhile
    StorageUtil.FormListClear(none, "TTLL_SL_EA_" + tid)
    StorageUtil.FormListClear(none, "TTLL_SL_EL_" + tid)
    StorageUtil.FloatListClear(none, "TTLL_SL_ER_" + tid)
EndFunction

int Function FindPair(string prefixA, string prefixL, int tid, Actor npc, Actor lover) global
    int count = StorageUtil.FormListCount(none, prefixA + tid)
    int i = 0
    while i < count
        if StorageUtil.FormListGet(none, prefixA + tid, i) == npc && StorageUtil.FormListGet(none, prefixL + tid, i) == lover
            return i
        endif
        i += 1
    endwhile
    return -1
EndFunction

Function AddExcitement(int tid, Actor npc, Actor lover, float rate) global
    int index = FindPair("TTLL_SL_EA_", "TTLL_SL_EL_", tid, npc, lover)
    if index >= 0
        StorageUtil.FloatListSet(none, "TTLL_SL_ER_" + tid, index, StorageUtil.FloatListGet(none, "TTLL_SL_ER_" + tid, index) + rate)
    else
        StorageUtil.FormListAdd(none, "TTLL_SL_EA_" + tid, npc)
        StorageUtil.FormListAdd(none, "TTLL_SL_EL_" + tid, lover)
        StorageUtil.FloatListAdd(none, "TTLL_SL_ER_" + tid, rate)
    endif
EndFunction

Function ApplyExcitement(int tid) global
    int threadKey = Key(tid)
    int count = StorageUtil.FormListCount(none, "TTLL_SL_EA_" + tid)
    int i = 0
    while i < count
        Actor npc = StorageUtil.FormListGet(none, "TTLL_SL_EA_" + tid, i) as Actor
        Actor lover = StorageUtil.FormListGet(none, "TTLL_SL_EL_" + tid, i) as Actor
        TTLL_ThreadsCollector.UpdateExcitementRate(threadKey, npc, lover, StorageUtil.FloatListGet(none, "TTLL_SL_ER_" + tid, i))
        i += 1
    endwhile

    count = StorageUtil.FormListCount(none, "TTLL_SL_PA_" + tid)
    i = 0
    while i < count
        Actor prevNpc = StorageUtil.FormListGet(none, "TTLL_SL_PA_" + tid, i) as Actor
        Actor prevLover = StorageUtil.FormListGet(none, "TTLL_SL_PL_" + tid, i) as Actor
        if FindPair("TTLL_SL_EA_", "TTLL_SL_EL_", tid, prevNpc, prevLover) < 0
            TTLL_ThreadsCollector.UpdateExcitementRate(threadKey, prevNpc, prevLover, 0.0)
        endif
        i += 1
    endwhile
EndFunction

; ========================================================================================
; Orgasm
; ========================================================================================

Function OnOrgasm(SexLabFramework slf, Actor npc) global
    if !npc
        return
    endif
    int tid = slf.FindActorController(npc)
    if tid < 0
        return
    endif
    int threadKey = Key(tid)
    ; Never add a bystander to the collector: the actor count decides solo / exclusive / group
    if TTLL_ThreadsCollector.GetActors(threadKey).Find(npc) < 0
        return
    endif

    TTLL_ThreadsCollector.SetActorBool(threadKey, npc, "orgasmed", true)

    bool pplus = IsPPlus()
    if HasPenis(slf, npc, pplus)
        Actor target = FindInternalClimaxTarget(slf, tid, npc, pplus)
        if target
            TTLL_Utils.RecordInternalClimax(npc, target)
        endif
    endif

    CreditOrgasm(tid, npc)
EndFunction

; Engine threads split by banked enjoyment gain; otherwise (or with no gain banked) the time split
Function CreditOrgasm(int tid, Actor npc) global
    int threadKey = Key(tid)
    int count = StorageUtil.FormListCount(none, "TTLL_SL_YA_" + tid)
    int rows = 0
    float sum = 0.0
    int i = 0
    while i < count
        if StorageUtil.FormListGet(none, "TTLL_SL_YA_" + tid, i) == npc
            rows += 1
            sum += StorageUtil.FloatListGet(none, "TTLL_SL_YS_" + tid, i)
        endif
        i += 1
    endwhile

    if sum > 0.0
        Actor[] lovers = PapyrusUtil.ActorArray(rows)
        float[] weights = PapyrusUtil.FloatArray(rows)
        int n = 0
        i = 0
        while i < count
            if StorageUtil.FormListGet(none, "TTLL_SL_YA_" + tid, i) == npc
                lovers[n] = StorageUtil.FormListGet(none, "TTLL_SL_YL_" + tid, i) as Actor
                weights[n] = StorageUtil.FloatListGet(none, "TTLL_SL_YS_" + tid, i)
                n += 1
            endif
            i += 1
        endwhile
        TTLL_ThreadsCollector.ExcitementContributorOrgasmWeighted(threadKey, npc, lovers, weights)
    else
        TTLL_ThreadsCollector.ExcitementContributorOrgasm(threadKey, npc)
    endif

    if !IsEngineLatched(tid)
        return
    endif
    ; This orgasm used up the banked gain; enjoyment is 0 again
    i = count - 1
    while i >= 0
        if StorageUtil.FormListGet(none, "TTLL_SL_YA_" + tid, i) == npc
            StorageUtil.FormListRemoveAt(none, "TTLL_SL_YA_" + tid, i)
            StorageUtil.FormListRemoveAt(none, "TTLL_SL_YL_" + tid, i)
            StorageUtil.FloatListRemoveAt(none, "TTLL_SL_YS_" + tid, i)
        endif
        i -= 1
    endwhile
    SetBaseline(tid, npc, 0.0)
EndFunction

; ========================================================================================
; SkyrimNet_SexLab OrgasmEngine (scenes latched at StartThread)
; ========================================================================================

bool Function IsEngineLatched(int tid) global
    return StorageUtil.GetIntValue(none, "TTLL_SL_ENG_" + tid) == 1
EndFunction

bool Function AnyEngineLatched() global
    int tid = 0
    while tid < 15
        if IsEngineLatched(tid)
            return true
        endif
        tid += 1
    endwhile
    return false
EndFunction

; The engine counts this actor's orgasms: SkyrimNet_SexLab_Orgasm is the count, SexLabOrgasm is an echo
bool Function EngineOwns(SexLabFramework slf, Actor npc) global
    if !npc
        return false
    endif
    int tid = slf.FindActorController(npc)
    return tid >= 0 && IsEngineLatched(tid) && TTLL_SkyrimNetSexLab.IsManaged(npc)
EndFunction

Function SampleEnjoyment() global
    int tid = 0
    while tid < 15
        SampleThread(tid)
        tid += 1
    endwhile
EndFunction

; Never adds collector actors or rates; only banks enjoyment gain for the current pairs
Function SampleThread(int tid) global
    if !IsEngineLatched(tid)
        return
    endif
    Actor[] actors = TTLL_ThreadsCollector.GetActors(Key(tid))
    int i = 0
    while i < actors.Length
        Actor npc = actors[i]
        if npc && TTLL_SkyrimNetSexLab.IsManaged(npc)
            SampleActor(tid, npc, TTLL_SkyrimNetSexLab.GetEnjoyment(npc))
        endif
        i += 1
    endwhile
EndFunction

Function SampleActor(int tid, Actor npc, float value) global
    int b = StorageUtil.FormListFind(none, "TTLL_SL_EB_" + tid, npc)
    if b < 0
        ; First sample: the scene's starting enjoyment is not credit for these partners
        SetBaseline(tid, npc, value)
        return
    endif
    float delta = value - StorageUtil.FloatListGet(none, "TTLL_SL_EV_" + tid, b)
    StorageUtil.FloatListSet(none, "TTLL_SL_EV_" + tid, b, value)
    if delta <= 0.0
        return
    endif

    int count = StorageUtil.FormListCount(none, "TTLL_SL_EA_" + tid)
    float rateSum = 0.0
    int i = 0
    while i < count
        if StorageUtil.FormListGet(none, "TTLL_SL_EA_" + tid, i) == npc
            rateSum += StorageUtil.FloatListGet(none, "TTLL_SL_ER_" + tid, i)
        endif
        i += 1
    endwhile
    if rateSum <= 0.0
        return
    endif

    i = 0
    while i < count
        if StorageUtil.FormListGet(none, "TTLL_SL_EA_" + tid, i) == npc
            float rate = StorageUtil.FloatListGet(none, "TTLL_SL_ER_" + tid, i)
            if rate > 0.0
                AddShare(tid, npc, StorageUtil.FormListGet(none, "TTLL_SL_EL_" + tid, i) as Actor, delta * rate / rateSum)
            endif
        endif
        i += 1
    endwhile
EndFunction

Function AddShare(int tid, Actor npc, Actor lover, float share) global
    int index = FindPair("TTLL_SL_YA_", "TTLL_SL_YL_", tid, npc, lover)
    if index >= 0
        StorageUtil.FloatListSet(none, "TTLL_SL_YS_" + tid, index, StorageUtil.FloatListGet(none, "TTLL_SL_YS_" + tid, index) + share)
    else
        StorageUtil.FormListAdd(none, "TTLL_SL_YA_" + tid, npc)
        StorageUtil.FormListAdd(none, "TTLL_SL_YL_" + tid, lover)
        StorageUtil.FloatListAdd(none, "TTLL_SL_YS_" + tid, share)
    endif
EndFunction

Function SetBaseline(int tid, Actor npc, float value) global
    int b = StorageUtil.FormListFind(none, "TTLL_SL_EB_" + tid, npc)
    if b >= 0
        StorageUtil.FloatListSet(none, "TTLL_SL_EV_" + tid, b, value)
    else
        StorageUtil.FormListAdd(none, "TTLL_SL_EB_" + tid, npc)
        StorageUtil.FloatListAdd(none, "TTLL_SL_EV_" + tid, value)
    endif
EndFunction

Actor Function FindInternalClimaxTarget(SexLabFramework slf, int tid, Actor npc, bool pplus) global
    sslThreadController thread = slf.GetController(tid)
    if !thread
        return none
    endif
    Actor[] positions = thread.Positions
    int i

    if pplus
        ; Partner who is vaginally penetrated by npc
        if TTLL_SexlabPPlus.IsInteractionRegistered(thread)
            i = 0
            while i < positions.Length
                if positions[i] != npc && TTLL_SexlabPPlus.GetInteractionTypes(thread, positions[i], npc).Find(1) >= 0
                    return positions[i]
                endif
                i += 1
            endwhile
        endif
    else
        sslBaseAnimation anim = thread.Animation
        int source = positions.Find(npc)
        if anim && source >= 0
            int stage = thread.Stage
            int penisCount = 0
            i = 0
            while i < positions.Length
                if HasPenis(slf, positions[i], false) || UsesStrapon(thread, positions[i], false)
                    penisCount += 1
                endif
                i += 1
            endwhile

            bool hasCumData = false
            i = 0
            while i < positions.Length
                if i != source
                    int cumId = anim.GetCumID(i, stage)
                    if cumId == 1 || cumId == 4 || cumId == 5 || cumId == 7
                        hasCumData = true
                        int cumSource = anim.GetCumSource(i, stage)
                        ; -1 means any source: only trust it with a single penis in the scene
                        if cumSource == source || (cumSource == -1 && penisCount == 1)
                            return positions[i]
                        endif
                    endif
                endif
                i += 1
            endwhile
            if hasCumData
                ; Vaginal cum data exists but names another source, or is ambiguous
                return none
            endif
        endif
    endif

    ; Fallback: this stage's vaginalsex act with npc as giver
    int count = StorageUtil.FormListCount(none, "TTLL_SL_TG_" + tid)
    i = 0
    while i < count
        if StorageUtil.StringListGet(none, "TTLL_SL_TT_" + tid, i) == "vaginalsex" && StorageUtil.FormListGet(none, "TTLL_SL_TG_" + tid, i) == npc
            return StorageUtil.FormListGet(none, "TTLL_SL_TR_" + tid, i) as Actor
        endif
        i += 1
    endwhile
    return none
EndFunction

; ========================================================================================
; SexLab stats: SexLab counts its own scenes; an OStim scene adds +1 per counter, like SexLab does.
; Never overwrites: SexLab holds history the ledger does not (pre-install, unmapped scenes).
; ========================================================================================

Function AddOStimStats(SexLabFramework slf, int threadID) global
    if !TTLL_ThreadsCollector.GetThreadBool(threadID, "hadSex")
        return
    endif
    bool pplus = IsPPlus()
    Actor[] actors = TTLL_ThreadsCollector.GetActors(threadID)
    int total = (actors.Length > 1) as int
    int solo = (actors.Length == 1) as int
    int i = 0
    while i < actors.Length
        Actor npc = actors[i]
        if npc
            int vaginal = HadAction(threadID, npc, "vaginalsex") as int
            int anal = HadAction(threadID, npc, "analsex") as int
            int oral = (HadAction(threadID, npc, "blowjob") || HadAction(threadID, npc, "deepthroat") \
                || HadAction(threadID, npc, "cunnilingus") || HadAction(threadID, npc, "anilingus") \
                || HadAction(threadID, npc, "rimjob") || HadAction(threadID, npc, "lickingpenis") \
                || HadAction(threadID, npc, "lickingvagina") || HadAction(threadID, npc, "lickingtesticles")) as int
            if pplus
                TTLL_SexlabPPlus.AddCounts(npc, total, solo, vaginal, anal, oral)
            else
                AdjustAE(slf, npc, "SexCount", total)
                AdjustAE(slf, npc, "Masturbation", solo)
                AdjustAE(slf, npc, "VaginalCount", vaginal)
                AdjustAE(slf, npc, "AnalCount", anal)
                AdjustAE(slf, npc, "OralCount", oral)
            endif
        endif
        i += 1
    endwhile
EndFunction

bool Function HadAction(int threadID, Actor npc, string actionName) global
    return TTLL_ThreadsCollector.GetActorBool(threadID, npc, "did." + actionName) || TTLL_ThreadsCollector.GetActorBool(threadID, npc, "got." + actionName)
EndFunction

Function AdjustAE(SexLabFramework slf, Actor npc, string skill, int amount) global
    if amount > 0
        slf.Stats.AdjustSkill(npc, skill, amount)
    endif
EndFunction

; ========================================================================================
; SexLab history backfill: the first time the ledger sees an actor in a scene, SexLab counts the ledger
; lacks go to the actor's game relationship partner (seeded by the native relationship scan), else to a
; past lover whose race and gender the SkyrimNet decision model picks (TTLL_MainController.OnPastLoverDecision).
; Pending past lovers (on None, one decision in flight): TTLL_PL_A actors, TTLL_PL_C 5 counts per actor,
; TTLL_PL_Busy 1 while a decision is out.
; ========================================================================================

; First-time lines for a SexLab animation (see TTLL_Store.DescribeFirstTimes); passes each position's SexLab counts
String Function GetFirstTimes(Actor[] positions, string tags) global
    SexLabFramework slf = Game.GetFormFromFile(0xD62, "SexLab.esm") as SexLabFramework
    bool pplus = IsPPlus()
    int[] total = PapyrusUtil.IntArray(positions.Length)
    int[] vaginal = PapyrusUtil.IntArray(positions.Length)
    int[] anal = PapyrusUtil.IntArray(positions.Length)
    int[] anatomy = PapyrusUtil.IntArray(positions.Length, -1)
    int i = 0
    while i < positions.Length
        if positions[i]
            if pplus || slf
                anatomy[i] = Anatomy(slf, positions[i], pplus)
            endif
            if pplus
                total[i] = TTLL_SexlabPPlus.GetCount(positions[i], 15)
                vaginal[i] = TTLL_SexlabPPlus.GetCount(positions[i], 10)
                anal[i] = TTLL_SexlabPPlus.GetCount(positions[i], 11)
            elseif slf
                total[i] = slf.Stats.GetSkill(positions[i], "SexCount")
                vaginal[i] = slf.Stats.GetSkill(positions[i], "VaginalCount")
                anal[i] = slf.Stats.GetSkill(positions[i], "AnalCount")
            endif
        endif
        i += 1
    endwhile
    return TTLL_Store.DescribeFirstTimes(positions, tags, total, vaginal, anal, anatomy)
EndFunction

; SexLab anatomy for the native side: 1 penis + 2 vagina (gender faction override, P+ futa)
int Function Anatomy(SexLabFramework slf, Actor npc, bool pplus) global
    int bits = 0
    if HasPenis(slf, npc, pplus)
        bits += 1
    endif
    if HasVagina(slf, npc, pplus)
        bits += 2
    endif
    return bits
EndFunction

Function CheckThreadHistory(TTLL_MainController mainController, SexLabFramework slf, Actor[] actors) global
    int i = 0
    while i < actors.Length
        CheckHistory(mainController, slf, actors[i])
        i += 1
    endwhile
EndFunction

Function CheckHistory(TTLL_MainController mainController, SexLabFramework slf, Actor npc) global
    ; Called before the scene is counted, so both sides hold the history before this scene
    if !npc || !TTLL_Store.ClaimSexLabCheck(npc)
        return
    endif
    int total
    int solo
    int vaginal
    int anal
    int oral
    if IsPPlus()
        total = TTLL_SexlabPPlus.GetCount(npc, 15)
        solo = TTLL_SexlabPPlus.GetCount(npc, 12)
        vaginal = TTLL_SexlabPPlus.GetCount(npc, 10)
        anal = TTLL_SexlabPPlus.GetCount(npc, 11)
        oral = TTLL_SexlabPPlus.GetCount(npc, 9)
    else
        total = slf.Stats.GetSkill(npc, "SexCount")
        solo = slf.Stats.GetSkill(npc, "Masturbation")
        vaginal = slf.Stats.GetSkill(npc, "VaginalCount")
        anal = slf.Stats.GetSkill(npc, "AnalCount")
        oral = slf.Stats.GetSkill(npc, "OralCount")
    endif
    total = Missing(total, TTLL_Store.GetNpcInt(npc, "exclusiveSex") + TTLL_Store.GetNpcInt(npc, "groupSex"))
    solo = Missing(solo, TTLL_Store.GetNpcInt(npc, "soloSex"))
    vaginal = Missing(vaginal, ActionCount(npc, "vaginalsex"))
    anal = Missing(anal, ActionCount(npc, "analsex"))
    ; deepthroat is stored as blowjob (synonyms.json)
    oral = Missing(oral, ActionCount(npc, "blowjob") + ActionCount(npc, "cunnilingus") + ActionCount(npc, "anilingus") \
        + ActionCount(npc, "rimjob") + ActionCount(npc, "lickingpenis") + ActionCount(npc, "lickingvagina") \
        + ActionCount(npc, "lickingtesticles"))
    if total + solo + vaginal + anal + oral == 0
        return
    endif
    if TTLL_Store.AddHistoryToPartner(npc, total, solo, vaginal, anal, oral)
        return
    endif
    StorageUtil.FormListAdd(none, "TTLL_PL_A", npc)
    StorageUtil.IntListAdd(none, "TTLL_PL_C", total)
    StorageUtil.IntListAdd(none, "TTLL_PL_C", solo)
    StorageUtil.IntListAdd(none, "TTLL_PL_C", vaginal)
    StorageUtil.IntListAdd(none, "TTLL_PL_C", anal)
    StorageUtil.IntListAdd(none, "TTLL_PL_C", oral)
    SendPastLover(mainController)
EndFunction

int Function Missing(int sexlab, int ledger) global
    if sexlab > ledger
        return sexlab - ledger
    endif
    return 0
EndFunction

int Function ActionCount(Actor npc, string actionName) global
    return TTLL_Store.GetActionCount(npc, true, actionName) + TTLL_Store.GetActionCount(npc, false, actionName)
EndFunction

; Asks the decision model for the first pending past lover; without SkyrimNet, files it with the fallbacks
Function SendPastLover(TTLL_MainController mainController) global
    if StorageUtil.GetIntValue(none, "TTLL_PL_Busy") == 1 || StorageUtil.FormListCount(none, "TTLL_PL_A") == 0
        return
    endif
    Actor npc = StorageUtil.FormListGet(none, "TTLL_PL_A", 0) as Actor
    if npc && Game.GetModByName("SkyrimNet.esp") != 255
        ; decnpc() needs the UUID as a JSON number; GetEntityUUID returns hex
        string uuid = TTLL_Store.UuidToDecimal(SkyrimNetApi.GetEntityUUID(npc))
        if uuid != ""
            string context = "{\"npc_uuid\":" + uuid + "}"
            if SkyrimNetApi.SendCustomDecisionToLLM("loversledger/past_lover", context, mainController, \
                    "TTLL_MainController", "OnPastLoverDecision") == 1
                StorageUtil.SetIntValue(none, "TTLL_PL_Busy", 1)
                return
            endif
        endif
    endif
    FinishPastLover(mainController, "", "")
EndFunction

; Files the first pending past lover. raceKey / genderKey "" = fallback (own race, opposite gender).
Function FinishPastLover(TTLL_MainController mainController, string raceKey, string genderKey) global
    if StorageUtil.FormListCount(none, "TTLL_PL_A") == 0
        StorageUtil.UnsetIntValue(none, "TTLL_PL_Busy")
        return
    endif
    Actor npc = StorageUtil.FormListGet(none, "TTLL_PL_A", 0) as Actor
    int total = StorageUtil.IntListGet(none, "TTLL_PL_C", 0)
    int solo = StorageUtil.IntListGet(none, "TTLL_PL_C", 1)
    int vaginal = StorageUtil.IntListGet(none, "TTLL_PL_C", 2)
    int anal = StorageUtil.IntListGet(none, "TTLL_PL_C", 3)
    int oral = StorageUtil.IntListGet(none, "TTLL_PL_C", 4)
    StorageUtil.FormListRemoveAt(none, "TTLL_PL_A", 0)
    int i = 0
    while i < 5
        StorageUtil.IntListRemoveAt(none, "TTLL_PL_C", 0)
        i += 1
    endwhile
    StorageUtil.UnsetIntValue(none, "TTLL_PL_Busy")

    if npc
        Race pastRace = RaceFromKey(raceKey)
        if !pastRace
            pastRace = npc.GetRace()
        endif
        int gender = 1 - npc.GetActorBase().GetSex()
        if genderKey == "male"
            gender = 0
        elseif genderKey == "female"
            gender = 1
        endif
        TTLL_Store.AddPastLover(npc, pastRace, gender, total, solo, vaginal, anal, oral)
    endif
    SendPastLover(mainController)
EndFunction

; Option keys of prompts/decisions/loversledger/past_lover.prompt; vanilla playable races (Skyrim.esm)
Race Function RaceFromKey(string raceKey) global
    int formId = 0
    if raceKey == "nord"
        formId = 0x13746
    elseif raceKey == "imperial"
        formId = 0x13744
    elseif raceKey == "breton"
        formId = 0x13741
    elseif raceKey == "redguard"
        formId = 0x13748
    elseif raceKey == "altmer"
        formId = 0x13743
    elseif raceKey == "bosmer"
        formId = 0x13749
    elseif raceKey == "dunmer"
        formId = 0x13742
    elseif raceKey == "orc"
        formId = 0x13747
    elseif raceKey == "khajiit"
        formId = 0x13745
    elseif raceKey == "argonian"
        formId = 0x13740
    endif
    if formId == 0
        return none
    endif
    return Game.GetFormFromFile(formId, "Skyrim.esm") as Race
EndFunction

; On load: a decision sent before the save never answers, so resend the first pending one
Function ResumePastLovers(TTLL_MainController mainController) global
    StorageUtil.UnsetIntValue(none, "TTLL_PL_Busy")
    SendPastLover(mainController)
EndFunction

; ========================================================================================
; Framework helpers
; ========================================================================================

; P+ animation proxies have empty tags (HasTag is always true), so P+ reads the active stage's tags
bool Function HasTag(sslThreadController thread, bool pplus, string tag) global
    if pplus
        return TTLL_SexlabPPlus.HasStageTag(thread, tag)
    endif
    sslBaseAnimation anim = thread.Animation
    return anim && anim.HasTag(tag)
EndFunction

string Function SceneId(sslThreadController thread, bool pplus) global
    if pplus
        return TTLL_SexlabPPlus.SceneId(thread)
    endif
    sslBaseAnimation anim = thread.Animation
    if anim
        return anim.Registry
    endif
    return ""
EndFunction

; AE GetGender: 0 male, 1 female, 2 creature male, 3 creature female (gender faction overrides apply)
; P+ GetSex: 0 male, 1 female, 2 futa, 3 creature male, 4 creature female
bool Function HasPenis(SexLabFramework slf, Actor npc, bool pplus) global
    if pplus
        int sex = TTLL_SexlabPPlus.GetSex(npc)
        return sex == 0 || sex == 2 || sex == 3
    endif
    int gender = slf.GetGender(npc)
    return gender == 0 || gender == 2
EndFunction

; AE equips a strap-on on female positions; P+ models this through GetSex (futa) instead
bool Function UsesStrapon(sslThreadController thread, Actor npc, bool pplus) global
    if pplus
        return false
    endif
    sslActorAlias slot = thread.ActorAlias(npc)
    return slot && slot.IsUsingStrapon()
EndFunction

; True when this stage already has an act from giver to receiver
bool Function IsActGiver(int tid, Actor giver, Actor receiver) global
    int count = StorageUtil.FormListCount(none, "TTLL_SL_TG_" + tid)
    int i = 0
    while i < count
        if StorageUtil.FormListGet(none, "TTLL_SL_TG_" + tid, i) == giver && StorageUtil.FormListGet(none, "TTLL_SL_TR_" + tid, i) == receiver
            return true
        endif
        i += 1
    endwhile
    return false
EndFunction

bool Function HasVagina(SexLabFramework slf, Actor npc, bool pplus) global
    if pplus
        int sex = TTLL_SexlabPPlus.GetSex(npc)
        return sex == 1 || sex == 2 || sex == 4
    endif
    int gender = slf.GetGender(npc)
    return gender == 1 || gender == 3
EndFunction
