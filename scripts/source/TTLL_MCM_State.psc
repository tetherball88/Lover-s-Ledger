scriptname TTLL_MCM_State

Function SetCurrentPage(string page) global
    StorageUtil.SetStringValue(none, "TTLL_MCM_currentPage", page)
EndFunction

string Function GetCurrentPage() global
    string page = StorageUtil.GetStringValue(none, "TTLL_MCM_currentPage")
    if(page != "") 
        return page
    endif

    return "Explore"
EndFunction

Function AddNpcOption(int id, Actor npc) global
    StorageUtil.SetFormValue(none, "TTLL_MCM_npcOptions_"+id, npc)
EndFunction

Actor Function GetNpcOption(int id) global
    return StorageUtil.GetFormValue(none, "TTLL_MCM_npcOptions_"+id) as Actor
EndFunction

Function SetSelectedNpc(Actor npc) global
    StorageUtil.SetFormValue(none, "TTLL_MCM_selectedNpc", npc)
EndFunction

Actor Function GetSelectedNpc() global
    return StorageUtil.GetFormValue(none, "TTLL_MCM_selectedNpc") as Actor
EndFunction

Function SetSearchValueNpc(string value) global
    StorageUtil.SetStringValue(none, "TTLL_MCM_searchValue", value)
EndFunction

string Function GetSearchValueNpc() global
    return StorageUtil.GetStringValue(none, "TTLL_MCM_searchValue")
EndFunction

Function SetSearchValueLover(string value) global
    StorageUtil.SetStringValue(none, "TTLL_MCM_searchValueLover", value)
EndFunction

string Function GetSearchValueLover() global
    return StorageUtil.GetStringValue(none, "TTLL_MCM_searchValueLover")
EndFunction

Function Clean() global
    StorageUtil.ClearAllPrefix("TTLL_MCM_")
EndFunction


Function CleanSelectedLover() global
    StorageUtil.SetFormValue(none, "TTLL_MCM_selectedLover", none)
EndFunction


string Function AddColored(bool isBlue, string value) global
	String Blue = "#6699ff"
	String Pink = "#ff3389"
	String Color
	If isBlue
		Color = Blue
	Else
		Color = Pink
	EndIf

	return "<font color='" + Color +"'>" + value
EndFunction

Function AddLoverOption(int id, Actor npc) global
    StorageUtil.SetFormValue(none, "TTLL_MCM_loverOptions_"+id, npc)
EndFunction

Actor Function GetLoverOption(int id) global
    return StorageUtil.GetFormValue(none, "TTLL_MCM_loverOptions_"+id) as Actor
EndFunction

Function SetSelectedLover(Actor npc) global
    StorageUtil.SetFormValue(none, "TTLL_MCM_selectedLover", npc)
EndFunction

Actor Function GetSelectedLover() global
    return StorageUtil.GetFormValue(none, "TTLL_MCM_selectedLover") as Actor
EndFunction