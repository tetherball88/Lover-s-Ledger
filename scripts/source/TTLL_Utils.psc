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

;/**
  * Records one internal climax: npc did it to target, target got it from npc.
*/;
Function RecordInternalClimax(Actor npc, Actor target) global
    TTLL_Store.SetLoverInt(npc, target, "internalClimax.did", TTLL_Store.GetLoverInt(npc, target, "internalClimax.did") + 1)
    TTLL_Store.SetLoverInt(target, npc, "internalClimax.got", TTLL_Store.GetLoverInt(target, npc, "internalClimax.got") + 1)
EndFunction
