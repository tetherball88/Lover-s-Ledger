scriptname TTLL_MCM_SettingsPage

Function RenderPage(TTLL_MCM mcm) global
    mcm.SetCursorFillMode(mcm.TOP_TO_BOTTOM)
    RenderLeftColumn(mcm)
    mcm.SetCursorPosition(1)
    RenderRightColumn(mcm)
EndFunction

Function RenderLeftColumn(TTLL_MCM mcm) global
    mcm.AddHeaderOption("Settings")
    mcm.oid_SettingsExportData = mcm.AddTextOption("", "Export data to file")
    mcm.oid_SettingsImportData = mcm.AddTextOption("", "Import data from file")
    mcm.oid_SettingsClearData = mcm.AddTextOption("", "Clear whole data")
EndFunction

Function RenderRightColumn(TTLL_MCM mcm) global
    
EndFunction

Function OnOptionSelect(TTLL_MCM mcm, int option) global
    if (mcm.oid_SettingsClearData == option)
        bool yes = mcm.ShowMessage("Are you sure you want to clear all data?")
        if(yes)
            TTLL_Store.Clear()
        endif
    elseif (mcm.oid_SettingsExportData == option)
        TTLL_Store.ExportData()
    elseif (mcm.oid_SettingsImportData == option)
        TTLL_Store.ImportData()
    endif
EndFunction

; Highlight
Function OnOptionHighlight(TTLL_MCM mcm, int option) global
    if(option == mcm.oid_SettingsClearData)
        mcm.SetInfoText("Clears whole data from save")
    elseif(option == mcm.oid_SettingsExportData)
        mcm.SetInfoText("Exports json data to file in Documents\\My Games\\Skyrim Special Edition\\JCUser\\LoversLedger\\main.json")
    elseif(option == mcm.oid_SettingsClearData)
        mcm.SetInfoText("Imports data from file in Documents\\My Games\\Skyrim Special Edition\\JCUser\\LoversLedger\\main.json")
    endif
    
EndFunction
    
; Default
Function OnOptionDefault(TTLL_MCM mcm, int option) global
    
EndFunction

Function OnOptionInputOpen(TTLL_MCM mcm, int option) global
	
EndFunction

Function OnOptionInputAccept(TTLL_MCM mcm, int option, string value) global
    
EndFunction