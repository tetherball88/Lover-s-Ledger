scriptname TTLL_MCM_NPCPage

Function RenderPage(TTLL_MCM mcm) global
    mcm.SetCursorFillMode(mcm.TOP_TO_BOTTOM)
    RenderLeftColumn(mcm)
    mcm.SetCursorPosition(1)
    RenderRightColumn(mcm)
    TTLL_MCM_State.CleanSelectedLover()
EndFunction

Function RenderLeftColumn(TTLL_MCM mcm) global
    Actor npc = TTLL_MCM_State.GetSelectedNpc()
    string npcName = TTLL_Utils.GetActorName(npc)
    ActorBase npcAB = npc.GetActorBase()
    bool isMale = npcAB.GetSex() == 0
    string raceVal = npcAB.GetRace().GetName()

    mcm.oid_ReturnToExplore = mcm.AddTextOption("", "Return to explore")
    mcm.AddHeaderOption(TTLL_MCM_State.AddColored(isMale, npcName + "'s data"))
    mcm.AddTextOption("Race", raceVal)
    mcm.AddTextOption("Last time had sex", TTLL_Store.GetNpcFlt(npc, "lastTime"))
    mcm.AddTextOption("Participated in solo encounter", TTLL_Store.GetNpcInt(npc, "soloSex") + " times")
    mcm.AddTextOption("Participated in couple encounter", TTLL_Store.GetNpcInt(npc, "exclusiveSex") + " times")
    mcm.AddTextOption("Participated in group encounter", TTLL_Store.GetNpcInt(npc, "groupSex") + " times")
    mcm.AddTextOption("Had encounter with same sex character", TTLL_Store.GetNpcInt(npc, "sameSexEncounter") + " times")
    int climaxInsideDid = TTLL_Store.GetNpcInt(npc, "internalClimax.did")
    if(climaxInsideDid != 0)
        mcm.AddTextOption(npcName + " climaxed inside lover", climaxInsideDid + " times")
    endif

    int climaxInsideGot = TTLL_Store.GetNpcInt(npc, "internalClimax.got")
    if(climaxInsideGot != 0)
        mcm.AddTextOption("Lover climaxed inside " + npcName, climaxInsideGot + " times")
    endif
    
    mcm.oid_NpcTopThreeActions = mcm.AddTextOption("Top three actions", "")
    mcm.oid_NpcTopThreeLovers = mcm.AddTextOption("Top three lovers", "")

    mcm.AddHeaderOption("Actions performed(actor role):")
    string[] didActions = TTLL_Store.GetAllActions(npc, true)
    if(didActions.Length == 0)
        mcm.AddTextOption(npcName + " didn't perform any actions", "")
    endif
    int i = 0
    while(i < didActions.Length)
        mcm.AddTextOption(didActions[i] + " (" + TTLL_Store.GetActionCount(npc, true, didActions[i]) + ")", "")
        i += 1
    endwhile

    mcm.AddHeaderOption("Actions received(target role):")
    string[] gotActions = TTLL_Store.GetAllActions(npc, false)
    if(gotActions.Length == 0)
        mcm.AddTextOption(npcName + " didn't receive any actions", "")
    endif
    int j = 0
    while(j < gotActions.Length)
        mcm.AddTextOption(gotActions[j] + " (" + TTLL_Store.GetActionCount(npc, false, gotActions[j]) + ")", "")
        j += 1
    endwhile
EndFunction

Function RenderRightColumn(TTLL_MCM mcm) global
    mcm.oid_SearchLover = mcm.AddInputOption("Search Lover", "")
    mcm.AddHeaderOption("Lovers:")
    Actor npc = TTLL_MCM_State.GetSelectedNpc()
    string searchValue = TTLL_MCM_State.GetSearchValueLover()
    string npcName = TTLL_Utils.GetActorName(npc)
    Actor selectedLover = TTLL_MCM_State.GetSelectedLover()
    Actor[] lovers = TTLL_Store.GetAllLovers(npc)
    if(lovers.Length == 0)
        mcm.AddTextOption(npcName + " doesnt't have any lovers", "")
    endif
    int i = 0
    while(i < lovers.Length)
        Actor lover = lovers[i]
        string loverName = TTLL_Utils.GetActorName(lover)
        bool shouldAdd = false
        if(searchValue != "") 
            shouldAdd = StringUtil.Find(loverName, searchValue) != -1
        else
            shouldAdd = true
        endif
        if(shouldAdd)
            int opt = mcm.AddTextOption(loverName + " with score - " + TTLL_Store.GetLoverScore(npc, lover), "")
            TTLL_MCM_State.AddLoverOption(opt, lover)
            if(lover == selectedLover)
                RenderLoverData(mcm, npc, lover)
            endif
        endif
        
        i += 1
    endwhile
EndFunction

Function RenderLoverData(TTLL_MCM mcm, Actor npc, Actor lover) global
    string npcName = TTLL_Utils.GetActorName(npc)
    string loverName = TTLL_Utils.GetActorName(lover)
    string raceVal = lover.GetActorBase().GetRace().GetName()

    mcm.AddHeaderOption("-- START " + loverName + "'s data --")
    mcm.AddTextOption("Lover's race", raceVal)
    mcm.oid_LoverLastTime = mcm.AddTextOption("Last time had sex with", TTLL_Store.GetLoverFlt(npc, lover, "lastTime"))
    mcm.oid_LoverExclusive = mcm.AddTextOption("Participated in couple encounter", TTLL_Store.GetLoverInt(npc, lover, "exclusivesex") + " times")
    mcm.oid_LoverGroup = mcm.AddTextOption("Participated in group encounter", TTLL_Store.GetLoverInt(npc, lover, "partofsamegroupsex") + " times")
    mcm.oid_LoverOrgasms = mcm.AddTextOption("Orgasms", TTLL_Store.GetLoverFlt(npc, lover, "orgasms") + " times")

    int climaxInsideDid = TTLL_Store.GetLoverInt(npc, lover, "internalClimax.did")
    if(climaxInsideDid != 0)
        mcm.AddTextOption(npcName + " climaxed inside " + loverName, climaxInsideDid + " times")
    endif

    int climaxInsideGot = TTLL_Store.GetLoverInt(npc, lover, "internalClimax.got")
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
    string npcName = TTLL_Utils.GetActorName(npc)
    Actor selectedLover = TTLL_MCM_State.GetSelectedLover()
    string selectedLoverName = TTLL_Utils.GetActorName(selectedLover)
    if(option == mcm.oid_NpcTopThreeActions)
        mcm.SetInfoText("Did: " + TTLL_Store.GetAllActions(npc, true, 3) + "\nReceived: " + TTLL_Store.GetAllActions(npc, false, 3))
    elseif(option == mcm.oid_NpcTopThreeLovers)
        Actor[] lovers = TTLL_Store.GetAllLovers(npc, 3)
        string info = ""

        int i = 0
        while(i < lovers.Length)
            info += TTLL_Utils.GetActorName(lovers[i]) + " with score - " + TTLL_Store.GetLoverScore(npc, lovers[i]) + "\n"
            i += 1
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
        string loverName = TTLL_Utils.GetActorName(lover)
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

