Scriptname TTLL_MCM extends SKI_ConfigBase  

int property oid_SearchNpc auto
int property oid_SearchLover auto
int property oid_ReturnToExplore auto
int property oid_SettingsClearData auto
int property oid_SettingsExportData auto
int property oid_SettingsImportData auto
int property oid_NpcTopThreeActions auto
int property oid_NpcTopThreeLovers auto

int property oid_LoverLastTime auto
int property oid_LoverExclusive auto
int property oid_LoverGroup auto
int property oid_LoverOrgasms auto
string property SearchValueNpc auto
string property SearchValueLover auto

string selectedPage

Event OnConfigInit()
    ModName = "Lover's Ledger"
    Pages = new string[2]

    Pages[0] = "Explore"
    Pages[1] = "Settings"
    TTLL_MCM_State.SetCurrentPage("Explore")
EndEvent

Event OnPageReset(string page)
    if(page == "Explore")
        string subPage = TTLL_MCM_State.GetCurrentPage()
        if(subPage == "NPC")
            TTLL_MCM_NPCPage.RenderPage(self)
        else
            TTLL_MCM_ExplorePage.RenderPage(self)
        endif
    elseif(page == "Settings")
        TTLL_MCM_SettingsPage.RenderPage(self)
        TTLL_MCM_State.Clean()
    endif
EndEvent

; Select
event OnOptionSelect(int option)
    if(currentPage == "Explore")
        string subPage = TTLL_MCM_State.GetCurrentPage()
        if(subPage == "NPC")
            TTLL_MCM_NPCPage.OnOptionSelect(self, option)
        else
            TTLL_MCM_ExplorePage.OnOptionSelect(self, option)
        endif
    elseif(currentPage == "Settings")
        TTLL_MCM_SettingsPage.OnOptionSelect(self, option)
    endif
endevent

    ; Highlight
event OnOptionHighlight(int option)
    if(currentPage == "Explore")
        string subPage = TTLL_MCM_State.GetCurrentPage()
    if(subPage == "NPC")
        TTLL_MCM_NPCPage.OnOptionHighlight(self, option)
    else
        TTLL_MCM_ExplorePage.OnOptionHighlight(self, option)
    endif
    elseif(currentPage == "Settings")
        TTLL_MCM_SettingsPage.OnOptionHighlight(self, option)
    endif
endevent
    
    ; Default
event OnOptionDefault(int option)
    if(currentPage == "Explore")
        string subPage = TTLL_MCM_State.GetCurrentPage()
        if(subPage == "NPC")
            TTLL_MCM_NPCPage.OnOptionDefault(self, option)
        else
            TTLL_MCM_ExplorePage.OnOptionDefault(self, option)
        endif
    elseif(currentPage == "Settings")
        TTLL_MCM_SettingsPage.OnOptionDefault(self, option)
    endif
endevent

event OnOptionInputOpen(int option)
    if(currentPage == "Explore")
        string subPage = TTLL_MCM_State.GetCurrentPage()
        if(subPage == "NPC")
            TTLL_MCM_NPCPage.OnOptionInputOpen(self, option)
        else
            TTLL_MCM_ExplorePage.OnOptionInputOpen(self, option)
        endif
    elseif(currentPage == "Settings")
        TTLL_MCM_SettingsPage.OnOptionInputOpen(self, option)
    endif
endEvent

event OnOptionInputAccept(int option, string value)
    if(currentPage == "Explore")
        string subPage = TTLL_MCM_State.GetCurrentPage()
        if(subPage == "NPC")
            TTLL_MCM_NPCPage.OnOptionInputAccept(self, option, value)
        else
            TTLL_MCM_ExplorePage.OnOptionInputAccept(self, option, value)
        endif
    elseif(currentPage == "Settings")
        TTLL_MCM_SettingsPage.OnOptionInputAccept(self, option, value)
    endif
endEvent

event OnConfigClose()
    TTLL_MCM_State.Clean()
endEvent

Function Navigate(string page)
    TTLL_MCM_State.SetCurrentPage(page)
    ForcePageReset()
EndFunction