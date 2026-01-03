#include "UI.h"
#include "LoversLedgerService.h"
#include "../include/SKSEMenuFramework.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

// Use ImGui types and functions from SKSEMenuFramework
using namespace ImGuiMCP;
namespace ImGui = ImGuiMCP::ImGui;

namespace LoversLedgerUI {

    // UI State
    namespace State {
        // NPC List
        static std::vector<NPCDisplayInfo> npcList;
        static char searchBuffer[256] = "";
        static int sortColumn = 0;  // 0=Name, 1=Encounters, 2=Lovers, 3=LastTime
        static bool sortAscending = true;

        // NPC Detail Window State
        static bool showNPCDetailWindow = false;
        static std::uint32_t detailNPC = 0;
        static std::vector<LoverDisplayInfo> loversList;

        // Edit buffers for NPC details
        static int editSameSexEncounter = 0;
        static int editSoloSex = 0;
        static int editExclusiveSex = 0;
        static int editGroupSex = 0;
        static float editLastTime = 0.0f;
        static int editTotalInternalDidCount = 0;
        static int editTotalInternalGotCount = 0;

        // Lover Editor Window State
        static bool showLoverEditorWindow = false;
        static std::uint32_t editorNPC = 0;
        static std::uint32_t editorLover = 0;

        // Edit buffers for lover data
        static int editLoverExclusiveSex = 0;
        static int editLoverGroupSex = 0;
        static float editLoverOrgasms = 0.0f;
        static float editLoverLastTime = 0.0f;
        static int editLoverInternalDidCount = 0;
        static int editLoverInternalGotCount = 0;
        static bool syncToPartner = true;
    }

    // Helper function implementations
    namespace Helpers {
        std::string GetActorName(std::uint32_t formId) {
            if (formId == 0) return "Unknown";

            auto* form = RE::TESForm::LookupByID(formId);
            if (!form) return std::format("Unknown (0x{:08X})", formId);

            auto* actor = form->As<RE::Actor>();
            if (actor && actor->GetActorBase()) {
                return actor->GetActorBase()->GetFullName();
            }

            auto* actorBase = form->As<RE::TESNPC>();
            if (actorBase) {
                return actorBase->GetFullName();
            }

            return std::format("Unknown (0x{:08X})", formId);
        }

        std::string FormatGameTime(float gameTime) {
            if (gameTime <= 0.0f) return "Never";

            auto* calendar = RE::Calendar::GetSingleton();
            if (!calendar) return std::format("{:.1f} days ago", gameTime);

            float currentTime = calendar->GetCurrentGameTime();
            float daysSince = currentTime - gameTime;

            if (daysSince < 0) daysSince = 0;

            if (daysSince < 1.0f) {
                int hours = static_cast<int>(daysSince * 24);
                return std::format("{} hours ago", hours);
            } else if (daysSince < 7.0f) {
                return std::format("{:.1f} days ago", daysSince);
            } else if (daysSince < 30.0f) {
                int weeks = static_cast<int>(daysSince / 7);
                return std::format("{} weeks ago", weeks);
            } else if (daysSince < 365.0f) {
                int months = static_cast<int>(daysSince / 30);
                return std::format("{} months ago", months);
            } else {
                int years = static_cast<int>(daysSince / 365);
                return std::format("{} years ago", years);
            }
        }

        std::string FormatFormID(std::uint32_t formId) {
            return std::format("0x{:08X}", formId);
        }

    }

    // Data Manager implementations - using existing service methods
    namespace DataManager {
        bool RemoveLoverRelationship(std::uint32_t npcFormId, std::uint32_t loverFormId) {
            auto& service = LL::LoversLedgerService::GetSingleton();

            auto npcData = service.GetNPCData(npcFormId);
            auto loverData = service.GetNPCData(loverFormId);

            if (!npcData || !loverData) {
                SKSE::log::warn("RemoveLoverRelationship: NPC data not found for {} or {}", npcFormId, loverFormId);
                return false;
            }

            // Get the lover relationship data
            auto npcLoversIt = npcData->lovers.find(loverFormId);
            auto loverNpcsIt = loverData->lovers.find(npcFormId);

            if (npcLoversIt == npcData->lovers.end() && loverNpcsIt == loverData->lovers.end()) {
                SKSE::log::warn("RemoveLoverRelationship: No relationship found between {} and {}", npcFormId, loverFormId);
                return false;
            }

            // Update both NPCs' data
            if (npcLoversIt != npcData->lovers.end()) {
                const auto& loverDataEntry = npcLoversIt->second;

                // Subtract the lover's contribution from NPC's totals
                npcData->exclusiveSex -= loverDataEntry.exclusiveSex;
                npcData->groupSex -= loverDataEntry.partOfSameGroupSex;
                npcData->totalInternalClimax.did -= loverDataEntry.internalClimax.did;
                npcData->totalInternalClimax.got -= loverDataEntry.internalClimax.got;

                // Ensure no negative values
                if (npcData->exclusiveSex < 0) npcData->exclusiveSex = 0;
                if (npcData->groupSex < 0) npcData->groupSex = 0;
                if (npcData->totalInternalClimax.did < 0) npcData->totalInternalClimax.did = 0;
                if (npcData->totalInternalClimax.got < 0) npcData->totalInternalClimax.got = 0;

                // Remove the lover entry
                npcData->lovers.erase(npcLoversIt);
            }

            if (loverNpcsIt != loverData->lovers.end()) {
                const auto& loverDataEntry = loverNpcsIt->second;

                // Subtract the NPC's contribution from lover's totals
                loverData->exclusiveSex -= loverDataEntry.exclusiveSex;
                loverData->groupSex -= loverDataEntry.partOfSameGroupSex;
                loverData->totalInternalClimax.did -= loverDataEntry.internalClimax.did;
                loverData->totalInternalClimax.got -= loverDataEntry.internalClimax.got;

                // Ensure no negative values
                if (loverData->exclusiveSex < 0) loverData->exclusiveSex = 0;
                if (loverData->groupSex < 0) loverData->groupSex = 0;
                if (loverData->totalInternalClimax.did < 0) loverData->totalInternalClimax.did = 0;
                if (loverData->totalInternalClimax.got < 0) loverData->totalInternalClimax.got = 0;

                // Remove the NPC entry
                loverData->lovers.erase(loverNpcsIt);
            }

            SKSE::log::info("Removed lover relationship between {} and {}", npcFormId, loverFormId);
            return true;
        }

        bool DeleteNPC(std::uint32_t npcFormId) {
            auto& service = LL::LoversLedgerService::GetSingleton();

            auto npcData = service.GetNPCData(npcFormId);
            if (!npcData) {
                SKSE::log::warn("DeleteNPC: NPC data not found for {}", npcFormId);
                return false;
            }

            // Get all lovers of this NPC
            std::vector<std::uint32_t> loversToUpdate;
            for (const auto& [loverFormId, loverData] : npcData->lovers) {
                loversToUpdate.push_back(loverFormId);
            }

            // Remove this NPC from all their lovers' data
            for (auto loverFormId : loversToUpdate) {
                auto loverData = service.GetNPCData(loverFormId);
                if (!loverData) continue;

                auto it = loverData->lovers.find(npcFormId);
                if (it != loverData->lovers.end()) {
                    const auto& relationshipData = it->second;

                    // Subtract this relationship from lover's totals
                    loverData->exclusiveSex -= relationshipData.exclusiveSex;
                    loverData->groupSex -= relationshipData.partOfSameGroupSex;
                    loverData->totalInternalClimax.did -= relationshipData.internalClimax.did;
                    loverData->totalInternalClimax.got -= relationshipData.internalClimax.got;

                    // Ensure no negative values
                    if (loverData->exclusiveSex < 0) loverData->exclusiveSex = 0;
                    if (loverData->groupSex < 0) loverData->groupSex = 0;
                    if (loverData->totalInternalClimax.did < 0) loverData->totalInternalClimax.did = 0;
                    if (loverData->totalInternalClimax.got < 0) loverData->totalInternalClimax.got = 0;

                    loverData->lovers.erase(it);
                }
            }

            // Delete the NPC from the store using existing service method
            service.DeleteNPCData(npcFormId);

            SKSE::log::info("Deleted NPC {} and cleaned up {} lover relationships", npcFormId, loversToUpdate.size());
            return true;
        }

        // Use existing SetLoverInt/SetLoverFloat methods
        bool UpdateLoverData(std::uint32_t npcFormId, std::uint32_t loverFormId,
                            const std::string& property, int value, bool syncToPartner) {
            auto& service = LL::LoversLedgerService::GetSingleton();

            service.SetLoverInt(npcFormId, loverFormId, property, value);

            if (syncToPartner) {
                // Sync to partner (swap did/got for internal climax)
                if (property == "internalClimax.did") {
                    service.SetLoverInt(loverFormId, npcFormId, "internalClimax.got", value);
                } else if (property == "internalClimax.got") {
                    service.SetLoverInt(loverFormId, npcFormId, "internalClimax.did", value);
                } else {
                    service.SetLoverInt(loverFormId, npcFormId, property, value);
                }
            }

            return true;
        }

        bool UpdateLoverData(std::uint32_t npcFormId, std::uint32_t loverFormId,
                            const std::string& property, float value, bool syncToPartner) {
            auto& service = LL::LoversLedgerService::GetSingleton();

            service.SetLoverFloat(npcFormId, loverFormId, property, value);

            if (syncToPartner) {
                service.SetLoverFloat(loverFormId, npcFormId, property, value);
            }

            return true;
        }

        // Use existing SetNpcInt/SetNpcFloat methods
        bool UpdateNPCData(std::uint32_t npcFormId, const std::string& property, int value) {
            auto& service = LL::LoversLedgerService::GetSingleton();
            service.SetNpcInt(npcFormId, property, value);
            return true;
        }

        bool UpdateNPCData(std::uint32_t npcFormId, const std::string& property, float value) {
            auto& service = LL::LoversLedgerService::GetSingleton();
            service.SetNpcFloat(npcFormId, property, value);
            return true;
        }
    }

    // NPC List View implementations
    namespace NPCListView {
        void RefreshNPCList() {
            State::npcList.clear();

            auto& service = LL::LoversLedgerService::GetSingleton();
            auto allNPCs = service.GetAllNPCs();

            for (auto formId : allNPCs) {
                auto npcData = service.GetNPCData(formId);
                if (!npcData) continue;

                NPCDisplayInfo info;
                info.formId = formId;
                info.name = Helpers::GetActorName(formId);
                info.totalEncounters = npcData->exclusiveSex + npcData->groupSex + npcData->soloSex;
                info.loversCount = static_cast<int>(npcData->lovers.size());
                info.lastTime = npcData->lastTime;

                // Calculate average lover score for sorting using service method
                float totalScore = 0.0f;
                for (const auto& [loverFormId, loverData] : npcData->lovers) {
                    totalScore += service.GetLoverScore(formId, loverFormId);
                }
                info.loverScore = info.loversCount > 0 ? totalScore / info.loversCount : 0.0f;

                // Apply search filter
                if (State::searchBuffer[0] != '\0') {
                    std::string searchLower = State::searchBuffer;
                    std::string nameLower = info.name;
                    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
                    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

                    if (nameLower.find(searchLower) == std::string::npos) {
                        continue;  // Skip this NPC
                    }
                }

                State::npcList.push_back(info);
            }

            // Sort the list
            std::sort(State::npcList.begin(), State::npcList.end(), [](const NPCDisplayInfo& a, const NPCDisplayInfo& b) {
                bool result;
                switch (State::sortColumn) {
                    case 0: result = a.name < b.name; break;
                    case 1: result = a.totalEncounters < b.totalEncounters; break;
                    case 2: result = a.loversCount < b.loversCount; break;
                    case 3: result = a.lastTime < b.lastTime; break;
                    default: result = a.name < b.name; break;
                }
                return State::sortAscending ? result : !result;
            });
        }

        void __stdcall Render() {
            ImGui::Text("Lover's Ledger - NPC Database");
            ImGui::Separator();

            // Search bar
            ImGui::Text("Search:");
            ImGui::SameLine();
            if (ImGui::InputText("##search", State::searchBuffer, sizeof(State::searchBuffer))) {
                RefreshNPCList();
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                RefreshNPCList();
            }

            ImGui::Separator();

            // Stats
            ImGui::Text("Total NPCs: %d", static_cast<int>(State::npcList.size()));

            ImGui::Separator();

            // Table
            if (ImGui::BeginTable("NPCTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY |
                                 ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Encounters", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Lovers", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Last Activity", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 250.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                // Handle column sorting
                if (auto* sortSpecs = ImGui::TableGetSortSpecs()) {
                    if (sortSpecs->SpecsDirty) {
                        if (sortSpecs->SpecsCount > 0) {
                            State::sortColumn = sortSpecs->Specs[0].ColumnIndex;
                            State::sortAscending = sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                            RefreshNPCList();
                        }
                        sortSpecs->SpecsDirty = false;
                    }
                }

                for (const auto& npc : State::npcList) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", npc.name.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", npc.totalEncounters);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", npc.loversCount);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", Helpers::FormatGameTime(npc.lastTime).c_str());

                    ImGui::TableNextColumn();
                    ImGui::PushID(npc.formId);
                    if (ImGui::Button("View Details")) {
                        NPCDetailView::SetSelectedNPC(npc.formId);
                        State::showNPCDetailWindow = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete")) {
                        ImGui::OpenPopup("ConfirmDelete");
                    }

                    if (ImGui::BeginPopupModal("ConfirmDelete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Text("Are you sure you want to delete %s?", npc.name.c_str());
                        ImGui::Text("This will remove all lover relationships and cannot be undone.");
                        ImGui::Separator();

                        if (ImGui::Button("Yes, Delete")) {
                            DataManager::DeleteNPC(npc.formId);
                            RefreshNPCList();
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel")) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            // Render popup windows
            NPCDetailView::Render();
            LoverEditor::Render();
        }
    }

    // NPC Detail View implementations
    namespace NPCDetailView {
        void SetSelectedNPC(std::uint32_t formId) {
            State::detailNPC = formId;

            auto& service = LL::LoversLedgerService::GetSingleton();

            // Load edit buffers using service methods (they handle locking properly)
            State::editSameSexEncounter = service.GetNpcInt(formId, "sameSexEncounter");
            State::editSoloSex = service.GetNpcInt(formId, "soloSex");
            State::editExclusiveSex = service.GetNpcInt(formId, "exclusiveSex");
            State::editGroupSex = service.GetNpcInt(formId, "groupSex");
            State::editLastTime = service.GetNpcFloat(formId, "lastTime");
            State::editTotalInternalDidCount = service.GetNpcInt(formId, "totalInternalClimax.did");
            State::editTotalInternalGotCount = service.GetNpcInt(formId, "totalInternalClimax.got");

            // Build lovers list - need to get NPC data for this
            auto npcData = service.GetNPCData(formId);
            State::loversList.clear();
            if (npcData) {
            for (const auto& [loverFormId, loverData] : npcData->lovers) {
                LoverDisplayInfo info;
                info.formId = loverFormId;
                info.name = Helpers::GetActorName(loverFormId);
                info.exclusiveSex = loverData.exclusiveSex;
                info.groupSex = loverData.partOfSameGroupSex;
                info.orgasms = loverData.orgasms;
                info.lastTime = loverData.lastTime;
                info.score = service.GetLoverScore(formId, loverFormId);  // Use service method
                State::loversList.push_back(info);
            }

            // Sort by score
            std::sort(State::loversList.begin(), State::loversList.end(),
                     [](const LoverDisplayInfo& a, const LoverDisplayInfo& b) {
                return a.score > b.score;
            });
            }
        }

        void __stdcall Render() {
            if (!State::showNPCDetailWindow) {
                return;
            }

            auto& service = LL::LoversLedgerService::GetSingleton();
            auto npcData = service.GetNPCData(State::detailNPC);
            if (!npcData) {
                State::showNPCDetailWindow = false;
                return;
            }

            std::string npcName = Helpers::GetActorName(State::detailNPC);
            std::string windowTitle = "NPC Details: " + npcName;

            ImGui::SetNextWindowSize(ImVec2{800, 600}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin(windowTitle.c_str(), &State::showNPCDetailWindow, ImGuiWindowFlags_None)) {
                ImGui::Text("FormID: %s", Helpers::FormatFormID(State::detailNPC).c_str());
                ImGui::Separator();

            // Basic Stats Section
            if (ImGui::CollapsingHeader("Basic Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Columns(2, "BasicStats");

                ImGui::Text("Same-Sex Encounters:");
                ImGui::NextColumn();
                if (ImGui::InputInt("##sameSex", &State::editSameSexEncounter)) {
                    if (State::editSameSexEncounter < 0) State::editSameSexEncounter = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("Save##sameSex")) {
                    DataManager::UpdateNPCData(State::detailNPC, "sameSexEncounter", State::editSameSexEncounter);
                }
                ImGui::NextColumn();

                ImGui::Text("Solo Sex:");
                ImGui::NextColumn();
                if (ImGui::InputInt("##soloSex", &State::editSoloSex)) {
                    if (State::editSoloSex < 0) State::editSoloSex = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("Save##soloSex")) {
                    DataManager::UpdateNPCData(State::detailNPC, "soloSex", State::editSoloSex);
                }
                ImGui::NextColumn();

                ImGui::Text("Exclusive Sex (Total):");
                ImGui::NextColumn();
                if (ImGui::InputInt("##exclusiveSex", &State::editExclusiveSex)) {
                    if (State::editExclusiveSex < 0) State::editExclusiveSex = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("Save##exclusiveSex")) {
                    DataManager::UpdateNPCData(State::detailNPC, "exclusiveSex", State::editExclusiveSex);
                }
                ImGui::NextColumn();

                ImGui::Text("Group Sex (Total):");
                ImGui::NextColumn();
                if (ImGui::InputInt("##groupSex", &State::editGroupSex)) {
                    if (State::editGroupSex < 0) State::editGroupSex = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("Save##groupSex")) {
                    DataManager::UpdateNPCData(State::detailNPC, "groupSex", State::editGroupSex);
                }
                ImGui::NextColumn();

                ImGui::Text("Last Activity:");
                ImGui::NextColumn();
                ImGui::Text("%s", Helpers::FormatGameTime(npcData->lastTime).c_str());
                ImGui::NextColumn();

                ImGui::Columns(1);
            }

            ImGui::Separator();

            // Lovers Section
            if (ImGui::CollapsingHeader("Lovers", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Total Lovers: %d", static_cast<int>(State::loversList.size()));

                if (ImGui::BeginTable("LoversTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Exclusive", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                    ImGui::TableSetupColumn("Orgasms", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    for (const auto& lover : State::loversList) {
                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        ImGui::Text("%s", lover.name.c_str());

                        ImGui::TableNextColumn();
                        ImGui::Text("%d", lover.exclusiveSex);

                        ImGui::TableNextColumn();
                        ImGui::Text("%d", lover.groupSex);

                        ImGui::TableNextColumn();
                        ImGui::Text("%.1f", lover.orgasms);

                        ImGui::TableNextColumn();
                        ImGui::Text("%.1f", lover.score);

                        ImGui::TableNextColumn();
                        ImGui::PushID(lover.formId);
                        if (ImGui::Button("Edit")) {
                            LoverEditor::SetSelectedLover(State::detailNPC, lover.formId);
                            State::showLoverEditorWindow = true;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Remove")) {
                            ImGui::OpenPopup("ConfirmRemoveLover");
                        }

                        if (ImGui::BeginPopupModal("ConfirmRemoveLover", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("Remove lover relationship with %s?", lover.name.c_str());
                            ImGui::Text("This will update both NPCs' data.");
                            ImGui::Separator();

                            if (ImGui::Button("Yes, Remove")) {
                                DataManager::RemoveLoverRelationship(State::detailNPC, lover.formId);
                                SetSelectedNPC(State::detailNPC);  // Refresh
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Cancel")) {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }

            // Actions Section
            if (ImGui::CollapsingHeader("Actions Performed/Received")) {
                ImGui::Columns(2);

                ImGui::Text("Actions Performed (Did):");
                ImGui::Separator();
                for (const auto& [action, count] : npcData->actions_did) {
                    ImGui::Text("%s: %d", action.c_str(), count);
                }

                ImGui::NextColumn();

                ImGui::Text("Actions Received (Got):");
                ImGui::Separator();
                for (const auto& [action, count] : npcData->actions_got) {
                    ImGui::Text("%s: %d", action.c_str(), count);
                }

                ImGui::Columns(1);
            }
            }
            ImGui::End();
        }
    }

    // Lover Editor implementations
    namespace LoverEditor {
        void SetSelectedLover(std::uint32_t npcFormId, std::uint32_t loverFormId) {
            State::editorNPC = npcFormId;
            State::editorLover = loverFormId;

            auto& service = LL::LoversLedgerService::GetSingleton();

            // Use existing service methods to get values
            State::editLoverExclusiveSex = service.GetLoverInt(npcFormId, loverFormId, "exclusiveSex");
            State::editLoverGroupSex = service.GetLoverInt(npcFormId, loverFormId, "partOfSameGroupSex");
            State::editLoverOrgasms = service.GetLoverFloat(npcFormId, loverFormId, "orgasms");
            State::editLoverLastTime = service.GetLoverFloat(npcFormId, loverFormId, "lastTime");
            State::editLoverInternalDidCount = service.GetLoverInt(npcFormId, loverFormId, "internalClimax.did");
            State::editLoverInternalGotCount = service.GetLoverInt(npcFormId, loverFormId, "internalClimax.got");
        }

        void __stdcall Render() {
            if (!State::showLoverEditorWindow) {
                return;
            }

            std::string npcName = Helpers::GetActorName(State::editorNPC);
            std::string loverName = Helpers::GetActorName(State::editorLover);
            std::string windowTitle = "Edit Relationship: " + npcName + " <-> " + loverName;

            ImGui::SetNextWindowSize(ImVec2{600, 400}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin(windowTitle.c_str(), &State::showLoverEditorWindow, ImGuiWindowFlags_None)) {

            ImGui::Separator();

            ImGui::Checkbox("Sync changes to partner", &State::syncToPartner);
            ImGui::TextWrapped("When enabled, changes will be applied to both NPCs in the relationship.");

            ImGui::Separator();

            ImGui::Columns(2, "LoverEditor");

            ImGui::Text("Exclusive Sex:");
            ImGui::NextColumn();
            if (ImGui::InputInt("##loverExclusive", &State::editLoverExclusiveSex)) {
                if (State::editLoverExclusiveSex < 0) State::editLoverExclusiveSex = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save##loverExclusive")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "exclusiveSex", State::editLoverExclusiveSex, State::syncToPartner);
            }
            ImGui::NextColumn();

            ImGui::Text("Group Sex:");
            ImGui::NextColumn();
            if (ImGui::InputInt("##loverGroup", &State::editLoverGroupSex)) {
                if (State::editLoverGroupSex < 0) State::editLoverGroupSex = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save##loverGroup")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "partOfSameGroupSex", State::editLoverGroupSex, State::syncToPartner);
            }
            ImGui::NextColumn();

            ImGui::Text("Orgasms:");
            ImGui::NextColumn();
            if (ImGui::InputFloat("##loverOrgasms", &State::editLoverOrgasms)) {
                if (State::editLoverOrgasms < 0.0f) State::editLoverOrgasms = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save##loverOrgasms")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "orgasms", State::editLoverOrgasms, State::syncToPartner);
            }
            ImGui::NextColumn();

            ImGui::Text("Last Time Together:");
            ImGui::NextColumn();
            ImGui::Text("%s", Helpers::FormatGameTime(State::editLoverLastTime).c_str());
            ImGui::NextColumn();

            ImGui::Text("Internal Climax (Did):");
            ImGui::NextColumn();
            if (ImGui::InputInt("##loverInternalDid", &State::editLoverInternalDidCount)) {
                if (State::editLoverInternalDidCount < 0) State::editLoverInternalDidCount = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save##loverInternalDid")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "internalClimax.did", State::editLoverInternalDidCount, State::syncToPartner);
            }
            ImGui::NextColumn();

            ImGui::Text("Internal Climax (Got):");
            ImGui::NextColumn();
            if (ImGui::InputInt("##loverInternalGot", &State::editLoverInternalGotCount)) {
                if (State::editLoverInternalGotCount < 0) State::editLoverInternalGotCount = 0;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save##loverInternalGot")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "internalClimax.got", State::editLoverInternalGotCount, State::syncToPartner);
            }
            ImGui::NextColumn();

            ImGui::Columns(1);
            }
            ImGui::End();
        }
    }

    // Registration function
    void RegisterMenu() {
        // Check if SKSEMenuFramework is installed
        if (!SKSEMenuFramework::IsInstalled()) {
            SKSE::log::warn("SKSEMenuFramework not installed, menu will not be registered");
            return;
        }

        SKSE::log::info("Registering Lover's Ledger menu with SKSEMenuFramework");

        // Set section name
        SKSEMenuFramework::SetSection("Lover's Ledger");

        // Register only the main NPC list menu (detail views are popup windows)
        SKSEMenuFramework::AddSectionItem("NPC Database", NPCListView::Render);

        // Initialize NPC list
        NPCListView::RefreshNPCList();

        SKSE::log::info("Lover's Ledger menu registered successfully");
    }
}
