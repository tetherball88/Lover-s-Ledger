scriptname TTLL_Utils

import TTLL_JCDomain



;/**
  * Joins the keys of a JMap object into a string with a separator.
  * @param {int} JObj - The JMap object.
  * @param {string} separator - The separator string.
  * @returns {string} - The joined keys as a string.
*/;
string Function JoinJMapKeys(int JObj, string separator = ", ") global
    string res = ""
    
    string keyName = JMap_nextKey(JObj, previousKey="", endKey="")

    while(keyName != "")
        res += keyName + separator
        keyName = JMap_nextKey(JObj, previousKey=keyName, endKey="")
    endwhile

    return res
EndFunction

string Function GetActorName(actor akActor) global
  if akActor == TTLL_Store.GetPlayerRef()
    return akActor.GetActorBase().GetName()
  else
    return akActor.GetDisplayName()
  EndIf
EndFunction

;/**
  * Determines the encounter type based on the number of participants.
  * @param {int} participants - The number of participants in the encounter.
  * @returns {string} - The encounter type ("solo", "couple", or "group").
*/;
string Function GetEcnounterType(int participants) global
  if(participants == 1)
      return "solo"
  elseif(participants == 2)
      return "couple"
  endif
      
  return "group"
EndFunction

Function SendUpdateLoverDataEvent(Actor npc, Actor lover) global
    int handler = ModEvent.Create("TTLL_UpdateLoverDataEvent")
    if(handler)
        ModEvent.PushForm(handler, npc as Form)
        ModEvent.PushForm(handler, lover as Form)
        ModEvent.Send(handler)
        Utility.Wait(0.1)
    endif
EndFunction