Scriptname TTLL_MainController extends Quest

Actor Property PlayerRef  Auto

; SexLab state (plain variables, not properties: no VMAD change). Indexed by SexLab tid 0-14.
SexLabFramework slf
bool[] slRestarting ; ChangeActors in progress on this tid
bool[] slSawEnd     ; P+ ChangeActors sent AnimationEnd, ActorChangeEnd not seen yet
float[] slEndAt     ; real time of that AnimationEnd, for the fallback apply


Int Function GetVersion()
    return 1
EndFunction

Event OnInit()
    Maintenance()
EndEvent

Function Maintenance()
    if Game.GetModByName("OStim.esp") != 255
        OstimSetup()
    EndIf

    slf = Game.GetFormFromFile(0xD62, "SexLab.esm") as SexLabFramework

    if slf != none
        if !slRestarting
            slRestarting = new bool[15]
            slSawEnd = new bool[15]
            slEndAt = new float[15]
        endif
        TTLL_SexlabIntegration.Maintenance(self, slf)
    endif

    StorageUtil.ClearAllPrefix("TTLNLDec_")
EndFunction

Function OstimSetup()
    RegisterForModEvent("ostim_thread_start", "OStimStart")
    RegisterForModEvent("ostim_thread_scenechanged", "OStimSceneChanged")
    RegisterForModEvent("ostim_actor_orgasm", "OStimOrgasm")
    RegisterForModEvent("ostim_thread_end", "OStimEnd")
EndFunction

Function OStimStart(string EventName, string StrArg, float ThreadID, Form Sender)
    TTLL_OstimINtegration.OStimStart(ThreadID as int)
EndFunction

Function OStimSceneChanged(string EventName, string StrArg, float ThreadID, Form Sender)
    TTLL_OstimINtegration.OStimSceneChanged(ThreadID as int)
EndFunction

Function OStimOrgasm(string EventName, string SceneID, float ThreadID, Form Sender)
    TTLL_OstimINtegration.OStimOrgasm(ThreadID as int, Sender as Actor)
EndFunction

Function OStimEnd(string EventName, string StrArg, float ThreadID, Form Sender)
    TTLL_OstimINtegration.OStimEnd(ThreadID as int)

    RegisterForSingleUpdate(3)
EndFunction

; ========================================================================================
; SexLab events (registered in TTLL_SexlabIntegration.Maintenance)
; ========================================================================================

bool Function IsSexLabTid(int tid)
    return slf != none && slRestarting && tid >= 0 && tid < 15
EndFunction

Event SLAnimationStarting(int tid, bool hasPlayer)
    ; A P+ ChangeActors restart re-sends AnimationStarting; keep the encounter
    if IsSexLabTid(tid) && !slRestarting[tid]
        TTLL_SexlabIntegration.StartThread(tid)
    endif
EndEvent

Event SLAnimationStart(int tid, bool hasPlayer)
    if IsSexLabTid(tid)
        TTLL_SexlabIntegration.ProcessStage(slf, tid)
        slRestarting[tid] = false
        slSawEnd[tid] = false
    endif
EndEvent

Event SLStageStart(int tid, bool hasPlayer)
    if IsSexLabTid(tid)
        TTLL_SexlabIntegration.ProcessStage(slf, tid)
    endif
EndEvent

Event SLAnimationChange(int tid, bool hasPlayer)
    if IsSexLabTid(tid)
        TTLL_SexlabIntegration.ProcessStage(slf, tid)
    endif
EndEvent

Event SLActorChangeStart(int tid, bool hasPlayer)
    if IsSexLabTid(tid)
        slRestarting[tid] = true
    endif
EndEvent

Event SLActorChangeEnd(int tid, bool hasPlayer)
    if IsSexLabTid(tid)
        if slSawEnd[tid]
            ; P+: the restart is confirmed; AnimationStarting / AnimationStart are still to come
            slSawEnd[tid] = false
        else
            ; AE: same thread, nothing ended
            slRestarting[tid] = false
        endif
    endif
EndEvent

Event SLAnimationEnd(int tid, bool hasPlayer)
    if !IsSexLabTid(tid)
        return
    endif
    if slRestarting[tid]
        ; P+ ChangeActors: hold the apply. If ActorChangeEnd never comes the restart failed; OnUpdate applies.
        slSawEnd[tid] = true
        slEndAt[tid] = Utility.GetCurrentRealTime()
        RegisterForSingleUpdate(SexLabRestartFallback())
    else
        SexLabEnd(tid)
    endif
EndEvent

Event SLOrgasm(Form actorForm, int enjoyment, int orgasms)
    if slf != none
        TTLL_SexlabIntegration.OnOrgasm(slf, actorForm as Actor)
    endif
EndEvent

; Seconds to wait for a P+ ChangeActors restart before applying the ended thread
float Function SexLabRestartFallback()
    return 5.0
EndFunction

Function SexLabEnd(int tid)
    TTLL_SexlabIntegration.OnEnd(tid)

    RegisterForSingleUpdate(3)
EndFunction

; Applies a P+ thread whose ChangeActors restart never confirmed. Returns seconds until the next pending one (0 = none).
float Function CheckRestartFallback()
    if !slSawEnd
        return 0.0
    endif
    float now = Utility.GetCurrentRealTime()
    float nextWait = 0.0
    int i = 0
    while i < slSawEnd.Length
        if slSawEnd[i]
            float remaining = SexLabRestartFallback() - (now - slEndAt[i])
            if remaining <= 0.0
                slSawEnd[i] = false
                slRestarting[i] = false
                SexLabEnd(i)
            elseif nextWait == 0.0 || remaining < nextWait
                nextWait = remaining
            endif
        endif
        i += 1
    endwhile
    return nextWait
EndFunction

Event OnUpdate()
    TTLL_ThreadsCollector.CleanFinishedThreads()

    float nextWait = CheckRestartFallback()
    if nextWait > 0.0
        RegisterForSingleUpdate(nextWait)
    endif
EndEvent
