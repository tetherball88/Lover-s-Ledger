scriptname TTLL_MCM_State

import TTLL_JCDomain

int Function GetJMCM() global
    int res = JDB_solveObj(".TT_LoversLedger.MCM")
    if(!res)
        res = JMap_object()
        JDB_solveObjSetter(".TT_LoversLedger.MCM", res, true)
    endif

    return res
EndFunction

Function SetCurrentPage(string page) global
    int JMCM = GetJMCM()
    JMap_setStr(JMCM, "currentPage", page)
EndFunction

string Function GetCurrentPage() global
    int JMCM = GetJMCM()
    string page = JMap_getStr(JMCM, "currentPage")
    if(page != "") 
        return page
    endif

    return "Explore"
EndFunction

int Function GetNpcOptions() global
    int JMCM = GetJMCM()
    int res = JMap_getObj(JMCM, "npcs")

    if(!res)
        res = JMap_object()
        JMap_setObj(JMCM, "npcs", res)
    endif

    return res
EndFunction

Function AddNpcOption(int id, Actor npc) global
    int JNpcs = GetNpcOptions()
    JMap_setForm(JNpcs, id, npc)
EndFunction

Actor Function GetNpcOption(int id) global
    int JNpcs = GetNpcOptions()
    return JMap_getForm(JNpcs, id) as Actor
EndFunction

Function SetSelectedNpc(Actor npc) global
    int JMCM = GetJMCM()

    JMap_setForm(JMCM, "selectedNpc", npc)
EndFunction

Actor Function GetSelectedNpc() global
    int JMCM = GetJMCM()

    return JMap_getForm(JMCM, "selectedNpc") as Actor
EndFunction

Function SetSearchValueNpc(string value) global
    int JMCM = GetJMCM()

    JMap_setStr(JMCM, "searchValue", value)
EndFunction

string Function GetSearchValueNpc() global
    int JMCM = GetJMCM()

    return JMap_getStr(JMCM, "searchValue")
EndFunction

Function SetSearchValueLover(string value) global
    int JMCM = GetJMCM()

    JMap_setStr(JMCM, "searchValueLover", value)
EndFunction

string Function GetSearchValueLover() global
    int JMCM = GetJMCM()

    return JMap_getStr(JMCM, "searchValueLover")
EndFunction

Function Clean() global
    JDB_solveObjSetter(".TT_LoversLedger.MCM", 0, true)
EndFunction


Function CleanSelectedLover() global
    int JMCM = GetJMCM()
    JMap_setForm(JMCM, "selectedLover", none)    
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

int Function GetLoverOptions() global
    int JMCM = GetJMCM()
    int res = JMap_getObj(JMCM, "lovers")

    if(!res)
        res = JMap_object()
        JMap_setObj(JMCM, "lovers", res)
    endif

    return res
EndFunction

Function AddLoverOption(int id, Actor npc) global
    int JLovers = GetLoverOptions()
    JMap_setForm(JLovers, id, npc)
EndFunction

Actor Function GetLoverOption(int id) global
    int JLovers = GetLoverOptions()
    return JMap_getForm(JLovers, id) as Actor
EndFunction

Function SetSelectedLover(Actor npc) global
    int JMCM = GetJMCM()

    JMap_setForm(JMCM, "selectedLover", npc)
EndFunction

Actor Function GetSelectedLover() global
    int JMCM = GetJMCM()

    return JMap_getForm(JMCM, "selectedLover") as Actor
EndFunction