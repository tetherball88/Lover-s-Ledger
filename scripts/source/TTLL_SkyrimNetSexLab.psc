scriptname TTLL_SkyrimNetSexLab

; Soft dependency on SkyrimNet_SexLab's OrgasmEngine. The only call site for SkyrimNet_SexLab_OrgasmEngine.
; Every call checks Installed() first, so the script works without the mod.

bool Function Installed() global
    return SKSE.GetPluginVersion("SkyrimNet_SexLab") >= 0
EndFunction

bool Function EngineDisabled() global
    if !Installed()
        return true
    endif
    return SkyrimNet_SexLab_OrgasmEngine.IsEngineDisabled()
EndFunction

bool Function IsManaged(Actor akActor) global
    if !akActor || !Installed()
        return false
    endif
    return SkyrimNet_SexLab_OrgasmEngine.IsManaged(akActor)
EndFunction

float Function GetEnjoyment(Actor akActor) global
    if !akActor || !Installed()
        return 0.0
    endif
    return SkyrimNet_SexLab_OrgasmEngine.GetEnjoyment(akActor)
EndFunction
