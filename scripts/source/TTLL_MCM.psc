Scriptname TTLL_MCM extends SKI_ConfigBase  

Event OnConfigInit()
    ; ModName = "Lover's Ledger"
    ; Pages = new string[2]

    ; Pages[0] = "Explore"
    ; Pages[1] = "Settings"
    ; TTLL_MCM_State.SetCurrentPage("Explore")
EndEvent

Event OnPageReset(string page)
    AddTextOption("MCM was removed in favor of SKSE Menu Framework.", "")
    AddTextOption("Make sure to install it.", "")
    AddTextOption("https://www.nexusmods.com/skyrimspecialedition/mods/120352", "")
    AddTextOption("default key to open SKSE Menu Framework is F1.", "")
EndEvent
