Scriptname TTLL_MainController extends Quest  

Actor Property PlayerRef  Auto  

Int Function GetVersion()
    return 1
EndFunction

Event OnInit()
    Maintenance()
EndEvent

Function Maintenance()
    TTLL_Store.ImportInitialData()

    if Game.GetModByName("OStim.esp") != 255
        OstimSetup()
    EndIf

    SexLabFramework slf = Game.GetFormFromFile(0xD62, "SexLab.esm") as SexLabFramework

    if slf != none
        TTLL_SexlabIntegration.Maintenance(self, slf)
    endif

    TTLL_Store.SetHasTTRF(Game.GetModByName("TT_RelationsFinder.esp") != 255)
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

Event OnUpdate()
    TTLL_OstimThreadsCollector.CleanFinishedThreads()
EndEvent

