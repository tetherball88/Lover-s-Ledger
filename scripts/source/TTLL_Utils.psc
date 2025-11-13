scriptname TTLL_Utils


Actor Function GetPlayerRef() global
    return Game.GetPlayer()
EndFunction

string Function GetActorName(actor akActor) global
    if akActor == GetPlayerRef()
        return akActor.GetActorBase().GetName()
    else
        return akActor.GetDisplayName()
    EndIf
EndFunction

Function SendThreadDataEvent(int ThreadID) global
    int handler = ModEvent.Create("ttll_thread_data_event")
    if(handler)
        ModEvent.PushInt(handler, ThreadID)
        ModEvent.Send(handler)
        Utility.Wait(0.1)
    endif
EndFunction