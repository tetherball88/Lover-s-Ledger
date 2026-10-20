Scriptname TTLL_MainController extends Quest

Actor Property PlayerRef  Auto

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

    if Game.GetModByName("SexLab.esm") != 255
        TTLL_SexlabIntegration.Maintenance(self)
    EndIf

    StorageUtil.ClearAllPrefix("TTLNLDec_")
EndFunction

Function OstimSetup()
    RegisterForModEvent("ostim_thread_start", "OStimStart")
    RegisterForModEvent("ostim_thread_scenechanged", "OStimSceneChanged")
    RegisterForModEvent("ostim_actor_orgasm", "OStimOrgasm")
    RegisterForModEvent("ostim_thread_end", "OStimEnd")
EndFunction

Function OStimStart(string EventName, string StrArg, float ThreadID, Form Sender)
    int tid = ThreadID as int
    TTLL_OstimINtegration.OStimStart(tid)
    if Game.GetModByName("SexLab.esm") != 255
        TTLL_SexlabIntegration.CheckThreadHistory(self, TTLL_OstimINtegration.GetActors(tid))
    endif
EndFunction

Function OStimSceneChanged(string EventName, string StrArg, float ThreadID, Form Sender)
    TTLL_OstimINtegration.OStimSceneChanged(ThreadID as int)
EndFunction

Function OStimOrgasm(string EventName, string SceneID, float ThreadID, Form Sender)
    TTLL_OstimINtegration.OStimOrgasm(ThreadID as int, Sender as Actor)
EndFunction

Function OStimEnd(string EventName, string StrArg, float ThreadID, Form Sender)
    int tid = ThreadID as int
    TTLL_OstimINtegration.OStimEnd(tid)
    if Game.GetModByName("SexLab.esm") != 255
        TTLL_SexlabIntegration.AddOStimStats(tid)
        TTLL_SexlabIntegration.ArmUpdate(self, 3.0)
    else
        RegisterForSingleUpdate(3)
    endif
EndFunction

; ========================================================================================
; SexLab events (registered in TTLL_SexlabIntegration.Maintenance)
; ========================================================================================

Event SLAnimationStarting(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnAnimationStarting(self, tid)
EndEvent

Event SLAnimationStart(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnAnimationStart(self, tid)
EndEvent

Event SLStageStart(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnStageStart(tid)
EndEvent

Event SLAnimationChange(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnAnimationChange(tid)
EndEvent

Event SLPositionChange(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnPositionChange(tid)
EndEvent

Event SLActorChangeStart(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnActorChangeStart(tid)
EndEvent

Event SLActorChangeEnd(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnActorChangeEnd(self, tid)
EndEvent

Event SLAnimationEnd(int tid, bool hasPlayer)
    TTLL_SexlabIntegration.OnAnimationEnd(self, tid)
EndEvent

Event SLOrgasm(Form actorForm, int enjoyment, int orgasms)
    TTLL_SexlabIntegration.OnSexLabOrgasm(actorForm as Actor)
EndEvent

Event SLNetOrgasm(string eventName, string strArg, float numArg, Form sender)
    TTLL_SexlabIntegration.OnNetOrgasm(sender as Actor)
EndEvent

Function OnPastLoverDecision(String answer, String resultJson, int success)
    TTLL_SexlabIntegration.OnPastLoverDecision(self, answer, resultJson, success)
EndFunction

Event OnUpdate()
    TTLL_ThreadsCollector.CleanFinishedThreads()
    if Game.GetModByName("SexLab.esm") != 255
        TTLL_SexlabIntegration.OnUpdate(self)
    endif
EndEvent
