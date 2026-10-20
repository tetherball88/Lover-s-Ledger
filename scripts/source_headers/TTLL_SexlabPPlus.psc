scriptname TTLL_SexlabPPlus hidden

; Header stub for the SexLab P+ bridge. The main build imports this file only; it never compiles it.
; The real script is scripts/source_pplus/TTLL_SexlabPPlus.psc, compiled against the SexLab P+ sources.
; Keep both signature sets identical. Callers must check TTLL_SexlabIntegration.IsPPlus() first.

bool Function IsInteractionRegistered(Quest thread) global
    return false
EndFunction

int[] Function GetInteractionTypes(Quest thread, Actor akPosition, Actor akPartner) global
    int[] result
    return result
EndFunction

bool Function HasStageTag(Quest thread, string tag) global
    return false
EndFunction

string Function SceneId(Quest thread) global
    return ""
EndFunction

int Function GetSex(Actor akActor) global
    return 0
EndFunction

Function AddCounts(Actor akActor, int total, int solo, int vaginal, int anal, int oral) global
EndFunction

int Function GetCount(Actor akActor, int id) global
    return 0
EndFunction
