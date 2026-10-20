scriptname TTLL_SexlabPPlus hidden

; SexLab P+ bridge. Compiled against the SexLab P+ sources (separate Pyro project).
; Header stub with the same signatures: scripts/source_headers/TTLL_SexlabPPlus.psc.
; Callers must check TTLL_SexlabIntegration.IsPPlus() first.

; If physics collision data is currently available for the thread
bool Function IsInteractionRegistered(Quest thread) global
    SexLabThread t = thread as SexLabThread
    if !t
        return false
    endif
    return t.IsInteractionRegistered()
EndFunction

; CTYPE values describing what akPosition does with akPartner (not commutative)
int[] Function GetInteractionTypes(Quest thread, Actor akPosition, Actor akPartner) global
    SexLabThread t = thread as SexLabThread
    if !t
        int[] result
        return result
    endif
    return t.GetInteractionTypes(akPosition, akPartner)
EndFunction

; Tag of the active stage (P+ animation proxies have empty tags)
bool Function HasStageTag(Quest thread, string tag) global
    SexLabThread t = thread as SexLabThread
    if !t
        return false
    endif
    return SexLabRegistry.IsStageTag(t.GetActiveScene(), t.GetActiveStage(), tag)
EndFunction

string Function SceneId(Quest thread) global
    SexLabThread t = thread as SexLabThread
    if !t
        return ""
    endif
    return t.GetActiveScene()
EndFunction

; 0 male, 1 female, 2 futa, 3 creature male, 4 creature female
int Function GetSex(Actor akActor) global
    return SexLabRegistry.GetSex(akActor, false)
EndFunction

; Adds to the sex counters (SexLabStatistics ids: 15 total, 12 masturbated, 10 vaginal, 11 anal, 9 oral)
Function AddCounts(Actor akActor, int total, int solo, int vaginal, int anal, int oral) global
    AddStatistic(akActor, 15, total)
    AddStatistic(akActor, 12, solo)
    AddStatistic(akActor, 10, vaginal)
    AddStatistic(akActor, 11, anal)
    AddStatistic(akActor, 9, oral)
EndFunction

Function AddStatistic(Actor akActor, int id, int amount) global
    if amount > 0
        SexLabStatistics.SetStatistic(akActor, id, SexLabStatistics.GetStatistic(akActor, id) + amount)
    endif
EndFunction

; A sex counter (same SexLabStatistics ids as AddCounts)
int Function GetCount(Actor akActor, int id) global
    return SexLabStatistics.GetStatistic(akActor, id) as int
EndFunction
