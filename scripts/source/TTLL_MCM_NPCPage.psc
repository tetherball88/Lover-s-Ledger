scriptname TTLL_MCM_NPCPage

import TTLL_JCDomain

Function RenderPage(TTLL_MCM mcm) global
    mcm.SetCursorFillMode(mcm.TOP_TO_BOTTOM)
    RenderLeftColumn(mcm)
    mcm.SetCursorPosition(1)
    RenderRightColumn(mcm)
    TTLL_MCM_State.CleanSelectedLover()
EndFunction

Function RenderLeftColumn(TTLL_MCM mcm) global
    Actor npc = TTLL_MCM_State.GetSelectedNpc()
    string npcName = TTLL_Store.GetNpcName(npc)
    bool isMale = TTLL_Store.GetNpcSex(npc) == 0
    string raceVal = TTLL_Store.GetNpcRace(npc)

    mcm.oid_ReturnToExplore = mcm.AddTextOption("", "Return to explore")
    mcm.AddHeaderOption(TTLL_MCM_State.AddColored(isMale, TTLL_Store.GetNpcName(npc) + "'s data"))
    mcm.AddTextOption("Race", raceVal)
    mcm.AddTextOption("Last time had sex", TTLL_Store.GetNpcLastTimeString(npc))
    mcm.AddTextOption("Participated in solo encounter", TTLL_Store.GetSoloSexCount(npc) + " times")
    mcm.AddTextOption("Participated in couple encounter", TTLL_Store.GetExclusiveSexCount(npc) + " times")
    mcm.AddTextOption("Participated in group encounter", TTLL_Store.GetGroupSexCount(npc) + " times")
    mcm.AddTextOption("Had encounter with same sex character", TTLL_Store.GetSameSexEncounterCount(npc) + " times")
    int climaxInsideDid = TTLL_Store.GetNpcInternalClimaxDid(npc)
    if(climaxInsideDid != 0)
        mcm.AddTextOption(npcName + " climaxed inside lover", climaxInsideDid + " times")
    endif
    
    int climaxInsideGot = TTLL_Store.GetNpcInternalClimaxGot(npc)
    if(climaxInsideGot != 0)
        mcm.AddTextOption("Lover climaxed inside " + npcName, climaxInsideGot + " times")
    endif
    
    mcm.oid_NpcTopThreeActions = mcm.AddTextOption("Top three actions", "")
    mcm.oid_NpcTopThreeLovers = mcm.AddTextOption("Top three lovers", "")

    mcm.AddHeaderOption("Actions performed(actor role):")
    int JDidActions = TTLL_Store.GetNpcDidActions(npc)
    if(JMap_count(JDidActions) == 0)
        mcm.AddTextOption(npcName + " didn't perform any actions", "")
    endif
    string didAction = JMap_nextKey(JDidActions, previousKey="")
    while didAction != ""
        mcm.AddTextOption(didAction + " (" + TTLL_Store.GetNpcDidActionCount(npc, didAction) + ")", "")
        didAction = JMap_nextKey(JDidActions, previousKey=didAction)
    endwhile

    mcm.AddHeaderOption("Actions received(target role):")
    int JGotActions = TTLL_Store.GetNpcGotActions(npc)
    if(JMap_count(JGotActions) == 0)
        mcm.AddTextOption(npcName + " didn't receive any actions", "")
    endif
    string gotAction = JMap_nextKey(JGotActions, previousKey="")
    while gotAction != ""
        mcm.AddTextOption(gotAction + " (" + TTLL_Store.GetNpcGotActionCount(npc, gotAction) + ")", "")
        gotAction = JMap_nextKey(JGotActions, previousKey=gotAction)
    endwhile
EndFunction

Function RenderRightColumn(TTLL_MCM mcm) global
    mcm.oid_SearchLover = mcm.AddInputOption("Search Lover", "")
    mcm.AddHeaderOption("Lovers:")
    Actor npc = TTLL_MCM_State.GetSelectedNpc()
    string searchValue = TTLL_MCM_State.GetSearchValueLover()
    string npcName = TTLL_Store.GetNpcName(npc)
    Actor selectedLover = TTLL_MCM_State.GetSelectedLover()
    int JLovers = TTLL_Store.GetNpcLovers(npc)
    if(JFormMap_count(JLovers) == 0)
        mcm.AddTextOption(npcName + " doesnt't have any lovers", "")
    endif
    Actor lover = JFormMap_nextKey(JLovers, previousKey=none) as Actor
    while(lover != none)
        string loverName = TTLL_Store.GetLoverName(npc, lover)
        bool shouldAdd = false
        if(searchValue != "") 
            shouldAdd = StringUtil.Find(loverName, searchValue) != -1
        else
            shouldAdd = true
        endif
        if(shouldAdd)
            int opt = mcm.AddTextOption(TTLL_Store.GetLoverName(npc, lover) + " with score - " + TTLL_Store.GetLoverScore(npc, lover), "")
            TTLL_MCM_State.AddLoverOption(opt, lover)
            if(lover == selectedLover)
                RenderLoverData(mcm, npc, lover)
            endif
        endif
        
        lover = JFormMap_nextKey(JLovers, previousKey=lover) as Actor
    endwhile
EndFunction

Function RenderLoverData(TTLL_MCM mcm, Actor npc, Actor lover) global
    string npcName = TTLL_Store.GetNpcName(npc)
    string loverName = TTLL_Store.GetLoverName(npc, lover)

    mcm.AddHeaderOption("-- START " + loverName + "'s data --")
    mcm.AddTextOption("Lover's race", TTLL_Store.GetLoverRace(npc, lover))
    mcm.oid_LoverLastTime = mcm.AddTextOption("Last time had sex with", TTLL_Store.GetLoverLastTimeString(npc, lover))
    mcm.oid_LoverExclusive = mcm.AddTextOption("Participated in couple encounter", TTLL_Store.GetLoverExclusiveSexCount(npc, lover) + " times")
    mcm.oid_LoverGroup = mcm.AddTextOption("Participated in group encounter", TTLL_Store.GetLoverGroupSexCount(npc, lover) + " times")
    mcm.oid_LoverOrgasms = mcm.AddTextOption("Orgasms", TTLL_Store.GetLoverOrgasms(npc, lover) + " times")

    int climaxInsideDid = TTLL_Store.GetLoverInternalClimaxDid(npc, lover)
    if(climaxInsideDid != 0)
        mcm.AddTextOption(npcName + " climaxed inside " + loverName, climaxInsideDid + " times")
    endif
    
    int climaxInsideGot = TTLL_Store.GetLoverInternalClimaxGot(npc, lover)
    if(climaxInsideGot != 0)
        mcm.AddTextOption(loverName + " climaxed inside " + npcName, climaxInsideGot + " times")
    endif
    
    mcm.AddHeaderOption("-- END " + loverName + "'s data --")
EndFunction

Function OnOptionSelect(TTLL_MCM mcm, int option) global
    if(option == mcm.oid_ReturnToExplore)
        mcm.Navigate("Explore")
    else
        Actor lover = TTLL_MCM_State.GetLoverOption(option)
        if(lover != none)
            Actor selectedLover = TTLL_MCM_State.GetSelectedLover()
            if(selectedLover == lover)
                TTLL_MCM_State.SetSelectedLover(none)
            else
                TTLL_MCM_State.SetSelectedLover(lover)
            endif
            
            mcm.ForcePageReset()
        endif
    endif
EndFunction

; Highlight
Function OnOptionHighlight(TTLL_MCM mcm, int option) global
    Actor npc = TTLL_MCM_State.GetSelectedNpc()
    string npcName = TTLL_Store.GetNpcName(npc)
    Actor selectedLover = TTLL_MCM_State.GetSelectedLover()
    string selectedLoverName = TTLL_Store.GetLoverName(npc, selectedLover)
    if(option == mcm.oid_NpcTopThreeActions)
        mcm.SetInfoText("Did: " + TTLL_Utils.JoinJMapKeys(TTLL_Store.GetNpcDidTopThreeActions(npc)) + "\nReceived: " + TTLL_Utils.JoinJMapKeys(TTLL_Store.GetNpcGotTopThreeActions(npc)))
    elseif(option == mcm.oid_NpcTopThreeLovers)
        int lovers = TTLL_Store.GetNpcTopThreeLovers(npc)
        string info = ""

        Actor lover = JFormMap_nextKey(lovers, previousKey=none) as Actor
        
        while(lover != none)
            info += TTLL_Store.GetLoverName(npc, lover) + " with score - " + TTLL_Store.GetLoverScore(npc, lover) + "\n"
            lover = JFormMap_nextKey(lovers, previousKey=lover) as Actor
        endwhile
        
        mcm.SetInfoText(info)
    elseif(option == mcm.oid_LoverLastTime)
        mcm.SetInfoText("Last time " + npcName + " and " + selectedLoverName + " had sexual encounter")
    elseif(option == mcm.oid_LoverExclusive)
        mcm.SetInfoText("How many times " + npcName + " and " + selectedLoverName + " had sexual encounter as couple")
    elseif(option == mcm.oid_LoverGroup)
        mcm.SetInfoText("How many times " + npcName + " and " + selectedLoverName + " had sexual encounter as part of group")
    elseif(option == mcm.oid_LoverOrgasms)
        mcm.SetInfoText("Represents " + selectedLoverName + "'s contribution to " + npcName + " reaching orgasm. \nIt's decimal because multiple actors could contribute to target's orgasm at the same moment.")
    elseif(option == mcm.oid_SearchLover)
        mcm.SetInfoText("Search characters by name(can be partial)")
    else
        Actor lover = TTLL_MCM_State.GetLoverOption(option)
        string loverName = TTLL_Store.GetLoverName(npc, lover)
        if(lover != none)
            mcm.SetInfoText("Click to get more insights between " + npcName + " and their lover " + loverName)
        endif
    endif
EndFunction
    
    ; Default
Function OnOptionDefault(TTLL_MCM mcm, int option) global
    if(option == mcm.oid_SearchNpc)
    endif
EndFunction

Function OnOptionInputOpen(TTLL_MCM mcm, int option) global
	; if (option == inputOID_I)
	; 	; Fill input box with current value
	; 	SetInputDialogStartText(myName)
	; endIf
EndFunction

Function OnOptionInputAccept(TTLL_MCM mcm, int option, string value) global
	if (mcm.oid_SearchLover == option)
        mcm.SetInputOptionValue(mcm.oid_SearchLover, mcm.SearchValueLover)
        TTLL_MCM_State.SetSearchValueLover(value)
        mcm.ForcePageReset()
    endIf
EndFunction

