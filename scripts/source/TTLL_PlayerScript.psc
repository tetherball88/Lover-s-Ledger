Scriptname TTLL_PlayerScript extends ReferenceAlias

Event OnPlayerLoadGame()
    TTLL_MainController mainController = self.GetOwningQuest() as TTLL_MainController
    mainController.Maintenance()
EndEvent