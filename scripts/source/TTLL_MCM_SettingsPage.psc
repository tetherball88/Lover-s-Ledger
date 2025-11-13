scriptname TTLL_MCM_SettingsPage

Function RenderPage(TTLL_MCM mcm) global
    mcm.SetCursorFillMode(mcm.TOP_TO_BOTTOM)
    RenderLeftColumn(mcm)
    mcm.SetCursorPosition(1)
    RenderRightColumn(mcm)
EndFunction

Function RenderLeftColumn(TTLL_MCM mcm) global
EndFunction

Function RenderRightColumn(TTLL_MCM mcm) global
EndFunction

Function OnOptionSelect(TTLL_MCM mcm, int option) global
    
EndFunction

; Highlight
Function OnOptionHighlight(TTLL_MCM mcm, int option) global
    
EndFunction
    
; Default
Function OnOptionDefault(TTLL_MCM mcm, int option) global
    
EndFunction

Function OnOptionInputOpen(TTLL_MCM mcm, int option) global
	
EndFunction

Function OnOptionInputAccept(TTLL_MCM mcm, int option, string value) global
    
EndFunction