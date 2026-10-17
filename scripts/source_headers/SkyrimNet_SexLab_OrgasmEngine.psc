Scriptname SkyrimNet_SexLab_OrgasmEngine

; Header stub for SkyrimNet_SexLab's OrgasmEngine natives. The main build imports this file only; it never compiles it.
; Never ship a SkyrimNet_SexLab_OrgasmEngine.pex: a loose pex of this name would override SkyrimNet_SexLab's real script.
; Keep these signatures identical to theirs. Call only through TTLL_SkyrimNetSexLab, which checks the plugin first.

float Function GetEnjoyment(Actor akActor) global native
bool Function IsManaged(Actor akActor) global native
bool Function IsEngineDisabled() global native
