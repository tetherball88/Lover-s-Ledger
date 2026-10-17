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
