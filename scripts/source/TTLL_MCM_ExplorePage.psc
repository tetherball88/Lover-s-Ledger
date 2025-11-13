scriptname TTLL_MCM_ExplorePage

Function RenderPage(TTLL_MCM mcm) global
    mcm.SetCursorFillMode(mcm.TOP_TO_BOTTOM)
    RenderLeftColumn(mcm)
    mcm.SetCursorPosition(1)
    RenderRightColumn(mcm)
EndFunction

Function RenderLeftColumn(TTLL_MCM mcm) global
    mcm.AddHeaderOption("Explore")
    mcm.oid_SearchNpc = mcm.AddInputOption("Search Characters", "")
    RenderNpcsList(mcm)
EndFunction

Function RenderRightColumn(TTLL_MCM mcm) global
    
EndFunction

Function RenderNpcsList(TTLL_MCM mcm) global
    mcm.AddHeaderOption("Characters: ")
    Actor[] npcs = TTLL_Store.GetAllNPCs()
    string searchValue = TTLL_MCM_State.GetSearchValueNpc()

    int i = 0

    while(i < npcs.Length)
        Actor npc = npcs[i]
        string npcName = TTLL_Utils.GetActorName(npc)
        bool isMale = npc.GetActorBase().GetSex() == 0
        bool shouldAdd = false
        if(searchValue != "") 
            shouldAdd = StringUtil.Find(npcName, searchValue) != -1
        else
            shouldAdd = true
        endif
        
        if(shouldAdd)
            TTLL_MCM_State.AddNpcOption(mcm.AddTextOption(TTLL_MCM_State.AddColored(isMale, npcName), ""), npc)
        endif

        i += 1
    endwhile
EndFunction

Function OnOptionSelect(TTLL_MCM mcm, int option) global
    Actor npc = TTLL_MCM_State.GetNpcOption(option)
    if(npc != none)
        TTLL_MCM_State.SetSelectedNpc(npc)
        mcm.Navigate("NPC")
    endif
EndFunction

; Highlight
Function OnOptionHighlight(TTLL_MCM mcm, int option) global
    if(option == mcm.oid_SearchNpc)
        mcm.SetInfoText("Search characters by name(can be partial)")
    else
        Actor npc = TTLL_MCM_State.GetNpcOption(option)
        if(npc != none)
            mcm.SetInfoText("View " + TTLL_Utils.GetActorName(npc) + "'s data")
        endif
    endif
    
EndFunction
    
    ; Default
Function OnOptionDefault(TTLL_MCM mcm, int option) global
    
EndFunction

Function OnOptionInputOpen(TTLL_MCM mcm, int option) global
	
EndFunction

Function OnOptionInputAccept(TTLL_MCM mcm, int option, string value) global
    if (mcm.oid_SearchNpc == option)
        mcm.SetInputOptionValue(mcm.oid_SearchNpc, mcm.SearchValueNpc)
        TTLL_MCM_State.SetSearchValueNpc(value)
        mcm.ForcePageReset()
    endIf
	
EndFunction