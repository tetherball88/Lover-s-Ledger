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

    // Constants
    namespace Constants {
        constexpr float ACTIVE_DAYS_THRESHOLD = 7.0f;  // Days for "active only" filter
    }

    // UI State
    namespace State {
        // NPC List
        static std::vector<NPCDisplayInfo> npcList;
        static char searchBuffer[256] = "";
        static int sortColumn = 0;  // 0=Name, 1=Encounters, 2=Lovers, 3=LastTime
        static bool sortAscending = true;

        // Advanced filters
        static int genderFilter = 0;  // 0=All, 1=Female, 2=Male
        static int minEncounters = 0;
        static int minLovers = 0;
        static bool showOnlyActive = false;  // Only show NPCs with recent activity

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

    // Color scheme constants
    namespace Colors {
        // Button colors
        const ImVec4 SaveButton = ImVec4(0.2f, 0.7f, 0.3f, 1.0f);         // Green
        const ImVec4 SaveButtonHovered = ImVec4(0.3f, 0.8f, 0.4f, 1.0f);
        const ImVec4 SaveButtonActive = ImVec4(0.15f, 0.6f, 0.25f, 1.0f);

        const ImVec4 DeleteButton = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);       // Red
        const ImVec4 DeleteButtonHovered = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        const ImVec4 DeleteButtonActive = ImVec4(0.7f, 0.15f, 0.15f, 1.0f);

        const ImVec4 EditButton = ImVec4(0.3f, 0.5f, 0.8f, 1.0f);         // Blue
        const ImVec4 EditButtonHovered = ImVec4(0.4f, 0.6f, 0.9f, 1.0f);
        const ImVec4 EditButtonActive = ImVec4(0.25f, 0.45f, 0.7f, 1.0f);

        const ImVec4 NormalButton = ImVec4(0.4f, 0.4f, 0.5f, 1.0f);       // Gray
        const ImVec4 NormalButtonHovered = ImVec4(0.5f, 0.5f, 0.6f, 1.0f);
        const ImVec4 NormalButtonActive = ImVec4(0.35f, 0.35f, 0.45f, 1.0f);

        // Table colors
        const ImVec4 TableRowBg = ImVec4(0.12f, 0.12f, 0.17f, 1.0f);
        const ImVec4 TableRowBgAlt = ImVec4(0.15f, 0.15f, 0.20f, 1.0f);
        const ImVec4 TableRowBgHovered = ImVec4(0.2f, 0.2f, 0.3f, 1.0f);
        const ImVec4 TableHeaderBg = ImVec4(0.2f, 0.2f, 0.28f, 1.0f);

        // Frame/Input colors
        const ImVec4 FrameBg = ImVec4(0.15f, 0.15f, 0.2f, 1.0f);
        const ImVec4 FrameBgHovered = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
        const ImVec4 FrameBgActive = ImVec4(0.25f, 0.25f, 0.3f, 1.0f);

        // Border colors
        const ImVec4 Border = ImVec4(0.3f, 0.3f, 0.4f, 1.0f);
        const ImVec4 SeparatorColor = ImVec4(0.35f, 0.35f, 0.45f, 1.0f);

        // Header colors
        const ImVec4 Header = ImVec4(0.25f, 0.25f, 0.35f, 1.0f);
        const ImVec4 HeaderHovered = ImVec4(0.3f, 0.3f, 0.4f, 1.0f);
        const ImVec4 HeaderActive = ImVec4(0.35f, 0.35f, 0.45f, 1.0f);
    }

    // Helper function implementations
    namespace Helpers {
        // Helper to get TESNPC from formId (DRY for all gender-related functions)
        RE::TESNPC* GetActorBase(std::uint32_t formId) {
            if (formId == 0) return nullptr;

            auto* form = RE::TESForm::LookupByID(formId);
            if (!form) return nullptr;

            // Try to get as Actor first
            auto* actor = form->As<RE::Actor>();
            if (actor && actor->GetActorBase()) {
                return actor->GetActorBase();
            }

            // Try as TESNPC
            return form->As<RE::TESNPC>();
        }

        std::string GetActorName(std::uint32_t formId) {
            if (formId == 0) return "Unknown";

            auto* actorBase = GetActorBase(formId);
            if (actorBase) {
                return actorBase->GetFullName();
            }

            return std::format("Unknown (0x{:08X})", formId);
        }

        std::string FormatGameTime(float gameTime) {
            auto* calendar = RE::Calendar::GetSingleton();

            float daysSince;
            if (gameTime <= 0.0f) {
                // Negative time means days before game start, use absolute value
                daysSince = std::abs(gameTime);
            } else if (calendar) {
                float currentTime = calendar->GetCurrentGameTime();
                daysSince = currentTime - gameTime;
                if (daysSince < 0) daysSince = 0;
            } else {
                // No calendar, assume gameTime is days ago
                daysSince = gameTime;
            }

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

        bool IsFemale(std::uint32_t formId) {
            auto* actorBase = GetActorBase(formId);
            return actorBase ? actorBase->IsFemale() : false;
        }

        std::string GetGenderIcon(std::uint32_t formId) {
            auto* actorBase = GetActorBase(formId);
            if (!actorBase) return "";

            // FontAwesome codes: Venus = 0xf221, Mars = 0xf222
            return actorBase->IsFemale()
                ? FontAwesome::UnicodeToUtf8(0xf221)  // Venus ♀
                : FontAwesome::UnicodeToUtf8(0xf222); // Mars ♂
        }

        ImVec4 GetGenderColor(std::uint32_t formId) {
            auto* actorBase = GetActorBase(formId);
            if (!actorBase) return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            return actorBase->IsFemale()
                ? ImVec4(1.0f, 0.4f, 0.7f, 1.0f)   // Pink/magenta for female
                : ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Light blue for male
        }

        // Button styling helpers
        void PushSaveButtonStyle() {
            ImGui::PushStyleColor(ImGuiCol_Button, Colors::SaveButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::SaveButtonHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::SaveButtonActive);
        }

        void PushDeleteButtonStyle() {
            ImGui::PushStyleColor(ImGuiCol_Button, Colors::DeleteButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::DeleteButtonHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::DeleteButtonActive);
        }

        void PushEditButtonStyle() {
            ImGui::PushStyleColor(ImGuiCol_Button, Colors::EditButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::EditButtonHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::EditButtonActive);
        }

        void PushNormalButtonStyle() {
            ImGui::PushStyleColor(ImGuiCol_Button, Colors::NormalButton);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::NormalButtonHovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::NormalButtonActive);
        }

        void PopButtonStyle() {
            ImGui::PopStyleColor(3);  // Pop all 3 button colors
        }

        // Tooltip helper
        void ShowTooltip(const char* text) {
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(text);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        // Apply global style improvements
        void ApplyGlobalStyle() {
            ImGuiStyle* style = ImGui::GetStyle();

            // Window rounding
            style->WindowRounding = 6.0f;
            style->ChildRounding = 4.0f;
            style->FrameRounding = 4.0f;
            style->GrabRounding = 4.0f;
            style->PopupRounding = 4.0f;
            style->ScrollbarRounding = 4.0f;
            style->TabRounding = 4.0f;

            // Padding and spacing
            style->WindowPadding = ImVec2(12, 12);
            style->FramePadding = ImVec2(8, 4);
            style->ItemSpacing = ImVec2(8, 6);
            style->ItemInnerSpacing = ImVec2(6, 4);
            style->IndentSpacing = 22.0f;

            // Borders
            style->WindowBorderSize = 1.0f;
            style->ChildBorderSize = 1.0f;
            style->FrameBorderSize = 1.0f;
            style->PopupBorderSize = 1.0f;

            // Colors
            ImGui::PushStyleColor(ImGuiCol_Border, Colors::Border);
            ImGui::PushStyleColor(ImGuiCol_Separator, Colors::SeparatorColor);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::FrameBg);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Colors::FrameBgHovered);
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Colors::FrameBgActive);
            ImGui::PushStyleColor(ImGuiCol_Header, Colors::Header);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Colors::HeaderHovered);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Colors::HeaderActive);
            ImGui::PushStyleColor(ImGuiCol_TableRowBg, Colors::TableRowBg);
            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, Colors::TableRowBgAlt);
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, Colors::TableHeaderBg);
        }

        void PopGlobalStyle() {
            ImGui::PopStyleColor(11);  // Pop all global colors
        }

        // Helper to render gender icon with color (DRY for repeated gender display)
        void RenderGenderIcon(std::uint32_t formId) {
            FontAwesome::PushSolid();
            ImGui::TextColored(GetGenderColor(formId), "%s", GetGenderIcon(formId).c_str());
            FontAwesome::Pop();
        }

        // Helper to clamp NPC data values to non-negative (DRY for relationship removal)
        void ClampNPCDataToNonNegative(LL::NPCData* data) {
            if (!data) return;

            if (data->exclusiveSex < 0) data->exclusiveSex = 0;
            if (data->groupSex < 0) data->groupSex = 0;
            if (data->totalInternalClimax.did < 0) data->totalInternalClimax.did = 0;
            if (data->totalInternalClimax.got < 0) data->totalInternalClimax.got = 0;
        }

        // Helper to render confirmation modal (DRY for delete/remove confirmations)
        bool RenderConfirmationModal(const char* modalId, const char* message, const char* details = nullptr) {
            bool confirmed = false;

            if (ImGui::BeginPopupModal(modalId, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%s", message);
                if (details) {
                    ImGui::Text("%s", details);
                }
                ImGui::Separator();

                // Confirm button (red)
                PushDeleteButtonStyle();
                if (ImGui::Button("Yes")) {
                    confirmed = true;
                    ImGui::CloseCurrentPopup();
                }
                PopButtonStyle();

                ImGui::SameLine();

                // Cancel button (normal)
                PushNormalButtonStyle();
                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }
                PopButtonStyle();

                ImGui::EndPopup();
            }

            return confirmed;
        }

        // Helper to render editable int field with save button (DRY for repeated input patterns)
        template<typename T>
        bool RenderEditFieldWithSave(const char* label, T& editValue, const char* id,
                                     const char* tooltip = nullptr, T minValue = 0) {
            bool saved = false;

            ImGui::Text("%s", label);
            if (tooltip) {
                ShowTooltip(tooltip);
            }
            ImGui::NextColumn();

            if constexpr (std::is_integral_v<T>) {
                if (ImGui::InputInt(std::format("##{}", id).c_str(), reinterpret_cast<int*>(&editValue))) {
                    if (editValue < minValue) editValue = minValue;
                }
            } else if constexpr (std::is_floating_point_v<T>) {
                if (ImGui::InputFloat(std::format("##{}", id).c_str(), &editValue)) {
                    if (editValue < minValue) editValue = minValue;
                }
            }

            ImGui::SameLine();
            PushSaveButtonStyle();
            if (ImGui::Button(std::format("Save##{}", id).c_str())) {
                saved = true;
            }
            PopButtonStyle();
            ImGui::NextColumn();

            return saved;
        }

        // Helper to calculate total encounters for an NPC (DRY for repeated calculation)
        int CalculateTotalEncounters(const LL::NPCData* npcData) {
            if (!npcData) return 0;
            return npcData->exclusiveSex + npcData->groupSex + npcData->soloSex;
        }

        // Helper to render a stat row in 2-column layout (DRY for stats display)
        void RenderStatRow(const char* label, const char* value, const char* tooltip = nullptr) {
            ImGui::Text("%s", label);
            if (tooltip) {
                ShowTooltip(tooltip);
            }
            ImGui::NextColumn();
            ImGui::Text("%s", value);
            ImGui::NextColumn();
        }

        void RenderStatRow(const char* label, int value, const char* tooltip = nullptr) {
            RenderStatRow(label, std::format("{}", value).c_str(), tooltip);
        }

        void RenderStatRow(const char* label, float value, const char* tooltip = nullptr) {
            RenderStatRow(label, std::format("{:.1f}", value).c_str(), tooltip);
        }

    }

    // Data Manager implementations - using existing service methods
    namespace DataManager {
        // Helper to subtract lover's contribution and clamp (DRY for RemoveLoverRelationship)
        void SubtractLoverContribution(LL::NPCData* data, const LL::LoverData& loverDataEntry) {
            if (!data) return;

            data->exclusiveSex -= loverDataEntry.exclusiveSex;
            data->groupSex -= loverDataEntry.partOfSameGroupSex;
            data->totalInternalClimax.did -= loverDataEntry.internalClimax.did;
            data->totalInternalClimax.got -= loverDataEntry.internalClimax.got;

            Helpers::ClampNPCDataToNonNegative(data);
        }

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
                SubtractLoverContribution(npcData, npcLoversIt->second);
                npcData->lovers.erase(npcLoversIt);
            }

            if (loverNpcsIt != loverData->lovers.end()) {
                SubtractLoverContribution(loverData, loverNpcsIt->second);
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
                    SubtractLoverContribution(loverData, it->second);
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
                info.totalEncounters = Helpers::CalculateTotalEncounters(npcData);
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

                // Apply gender filter
                if (State::genderFilter != 0) {
                    bool isFemale = Helpers::IsFemale(formId);
                    if ((State::genderFilter == 1 && !isFemale) || (State::genderFilter == 2 && isFemale)) {
                        continue;  // Skip this NPC
                    }
                }

                // Apply minimum encounters filter
                if (info.totalEncounters < State::minEncounters) {
                    continue;
                }

                // Apply minimum lovers filter
                if (info.loversCount < State::minLovers) {
                    continue;
                }

                // Apply "active only" filter
                if (State::showOnlyActive) {
                    auto* calendar = RE::Calendar::GetSingleton();
                    if (calendar) {
                        float currentTime = calendar->GetCurrentGameTime();
                        float daysSince = currentTime - info.lastTime;
                        if (daysSince > Constants::ACTIVE_DAYS_THRESHOLD) {
                            continue;
                        }
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
            // Refresh NPC list every 60 frames (~1 second) to capture new NPCs added during gameplay
            static int frameCounter = 0;
            if (frameCounter++ % 60 == 0) {
                RefreshNPCList();
            }

            // Apply global styling
            Helpers::ApplyGlobalStyle();

            // Set window background to be opaque
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));

            ImGui::Text("Lover's Ledger - NPC Database");
            ImGui::Separator();

            // Search and filters section
            ImGui::Text("Search:");
            ImGui::SameLine();
            if (ImGui::InputText("##search", State::searchBuffer, sizeof(State::searchBuffer))) {
                RefreshNPCList();
            }
            Helpers::ShowTooltip("Search NPCs by name");

            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                RefreshNPCList();
            }
            Helpers::ShowTooltip("Manually refresh the NPC list");

            // Advanced Filters
            if (ImGui::CollapsingHeader("Filters")) {
                ImGui::Indent();

                // Gender filter
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Gender:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                const char* genderItems[] = { "All", "Female", "Male" };
                if (ImGui::Combo("##genderFilter", &State::genderFilter, genderItems, 3)) {
                    RefreshNPCList();
                }
                Helpers::ShowTooltip("Filter NPCs by gender");

                // Minimum encounters
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Min Encounters:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                if (ImGui::InputInt("##minEncounters", &State::minEncounters)) {
                    if (State::minEncounters < 0) State::minEncounters = 0;
                    RefreshNPCList();
                }
                Helpers::ShowTooltip("Show only NPCs with at least this many encounters");

                // Minimum lovers
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Min Lovers:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                if (ImGui::InputInt("##minLovers", &State::minLovers)) {
                    if (State::minLovers < 0) State::minLovers = 0;
                    RefreshNPCList();
                }
                Helpers::ShowTooltip("Show only NPCs with at least this many lovers");

                // Active only checkbox
                ImGui::AlignTextToFramePadding();
                if (ImGui::Checkbox("Active Only (Last 7 Days)", &State::showOnlyActive)) {
                    RefreshNPCList();
                }
                Helpers::ShowTooltip("Show only NPCs with activity in the last 7 in-game days");

                // Clear filters button
                if (ImGui::Button("Clear All Filters")) {
                    State::genderFilter = 0;
                    State::minEncounters = 0;
                    State::minLovers = 0;
                    State::showOnlyActive = false;
                    State::searchBuffer[0] = '\0';
                    RefreshNPCList();
                }
                Helpers::ShowTooltip("Reset all filters to default values");

                ImGui::Unindent();
            }

            ImGui::Separator();

            // Statistics Dashboard
            if (ImGui::CollapsingHeader("Statistics Dashboard", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& service = LL::LoversLedgerService::GetSingleton();
                auto allNPCs = service.GetAllNPCs();

                // Calculate aggregate statistics
                int totalNPCs = static_cast<int>(allNPCs.size());
                int totalEncounters = 0;
                int totalLovers = 0;
                int totalFemales = 0;
                int totalMales = 0;
                std::uint32_t mostActiveNPC = 0;
                int mostEncounters = 0;
                std::uint32_t mostLovedNPC = 0;
                int mostLovers = 0;

                for (auto formId : allNPCs) {
                    auto npcData = service.GetNPCData(formId);
                    if (!npcData) continue;

                    int encounters = Helpers::CalculateTotalEncounters(npcData);
                    totalEncounters += encounters;
                    int loversCount = static_cast<int>(npcData->lovers.size());
                    totalLovers += loversCount;

                    // Track most active
                    if (encounters > mostEncounters) {
                        mostEncounters = encounters;
                        mostActiveNPC = formId;
                    }

                    // Track most lovers
                    if (loversCount > mostLovers) {
                        mostLovers = loversCount;
                        mostLovedNPC = formId;
                    }

                    // Count genders
                    if (Helpers::IsFemale(formId)) {
                        totalFemales++;
                    } else {
                        totalMales++;
                    }
                }

                float avgEncounters = totalNPCs > 0 ? static_cast<float>(totalEncounters) / totalNPCs : 0.0f;
                float avgLovers = totalNPCs > 0 ? static_cast<float>(totalLovers) / totalNPCs : 0.0f;

                ImGui::Columns(2, "StatsColumns");

                Helpers::RenderStatRow("Total NPCs:", totalNPCs);
                Helpers::RenderStatRow("Displayed NPCs:", static_cast<int>(State::npcList.size()),
                                      "Number of NPCs matching current filters");
                Helpers::RenderStatRow("Total Encounters:", totalEncounters,
                                      "Sum of all sexual encounters across all NPCs");
                Helpers::RenderStatRow("Avg Encounters/NPC:", avgEncounters);
                Helpers::RenderStatRow("Avg Lovers/NPC:", avgLovers);
                Helpers::RenderStatRow("Gender Ratio (F/M):",
                                      std::format("{} / {}", totalFemales, totalMales).c_str());

                if (mostActiveNPC != 0) {
                    ImGui::Text("Most Active NPC:");
                    Helpers::ShowTooltip("NPC with the most total encounters");
                    ImGui::NextColumn();
                    ImGui::TextColored(Helpers::GetGenderColor(mostActiveNPC), "%s (%d)",
                                      Helpers::GetActorName(mostActiveNPC).c_str(), mostEncounters);
                    ImGui::NextColumn();
                }

                if (mostLovedNPC != 0) {
                    ImGui::Text("Most Lovers:");
                    Helpers::ShowTooltip("NPC with the most unique lovers");
                    ImGui::NextColumn();
                    ImGui::TextColored(Helpers::GetGenderColor(mostLovedNPC), "%s (%d)",
                                      Helpers::GetActorName(mostLovedNPC).c_str(), mostLovers);
                    ImGui::NextColumn();
                }

                ImGui::Columns(1);
            }

            ImGui::Separator();

            // Table with hover effect
            if (ImGui::BeginTable("NPCTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                 ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY |
                                 ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit |
                                 ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Encounters", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Lovers", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Last Activity", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 250.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                // Add tooltips to headers
                if (ImGui::TableGetColumnFlags(0) & ImGuiTableColumnFlags_IsHovered) {
                    Helpers::ShowTooltip("NPC name with gender icon (♀/♂). Click header to sort.");
                }
                if (ImGui::TableGetColumnFlags(1) & ImGuiTableColumnFlags_IsHovered) {
                    Helpers::ShowTooltip("Total sexual encounters (exclusive + group + solo). Click to sort.");
                }
                if (ImGui::TableGetColumnFlags(2) & ImGuiTableColumnFlags_IsHovered) {
                    Helpers::ShowTooltip("Number of unique lovers this NPC has been with. Click to sort.");
                }
                if (ImGui::TableGetColumnFlags(3) & ImGuiTableColumnFlags_IsHovered) {
                    Helpers::ShowTooltip("Time since last sexual activity. Click to sort.");
                }

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
                    // Display gender icon with color
                    Helpers::RenderGenderIcon(npc.formId);
                    ImGui::SameLine();
                    ImGui::Text("%s", npc.name.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", npc.totalEncounters);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", npc.loversCount);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", Helpers::FormatGameTime(npc.lastTime).c_str());

                    ImGui::TableNextColumn();
                    ImGui::PushID(npc.formId);

                    // View Details button (blue)
                    Helpers::PushEditButtonStyle();
                    if (ImGui::Button("View Details")) {
                        NPCDetailView::SetSelectedNPC(npc.formId);
                        State::showNPCDetailWindow = true;
                    }
                    Helpers::PopButtonStyle();
                    Helpers::ShowTooltip("View detailed statistics and lover relationships for this NPC");

                    ImGui::SameLine();

                    // Delete button (red)
                    Helpers::PushDeleteButtonStyle();
                    if (ImGui::Button("Delete")) {
                        ImGui::OpenPopup("ConfirmDelete");
                    }
                    Helpers::PopButtonStyle();
                    Helpers::ShowTooltip("Permanently delete this NPC and all their relationships");

                    std::string confirmMsg = std::format("Are you sure you want to delete {}?", npc.name);
                    if (Helpers::RenderConfirmationModal("ConfirmDelete", confirmMsg.c_str(),
                                                         "This will remove all lover relationships and cannot be undone.")) {
                        DataManager::DeleteNPC(npc.formId);
                        RefreshNPCList();
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            // Render popup windows
            NPCDetailView::Render();
            LoverEditor::Render();

            // Pop window background color style
            ImGui::PopStyleColor();

            // Pop global style
            Helpers::PopGlobalStyle();
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

            // Set window background to be fully opaque (non-transparent)
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));

            ImGui::SetNextWindowSize(ImVec2{800, 900}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin(windowTitle.c_str(), &State::showNPCDetailWindow, ImGuiWindowFlags_None)) {
                // Display gender icon with name
                Helpers::RenderGenderIcon(State::detailNPC);
                ImGui::SameLine();
                ImGui::Text("%s", npcName.c_str());
                ImGui::Text("FormID: %s", Helpers::FormatFormID(State::detailNPC).c_str());
                ImGui::Separator();

            // Basic Stats Section
            if (ImGui::CollapsingHeader("Basic Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Columns(2, "BasicStats");

                if (Helpers::RenderEditFieldWithSave("Same-Sex Encounters:", State::editSameSexEncounter, "sameSex",
                                                     "Number of sexual encounters with same-gender NPCs")) {
                    DataManager::UpdateNPCData(State::detailNPC, "sameSexEncounter", State::editSameSexEncounter);
                    SetSelectedNPC(State::detailNPC);
                }

                if (Helpers::RenderEditFieldWithSave("Solo Sex:", State::editSoloSex, "soloSex",
                                                     "Number of masturbation or solo sexual activities")) {
                    DataManager::UpdateNPCData(State::detailNPC, "soloSex", State::editSoloSex);
                    SetSelectedNPC(State::detailNPC);
                }

                ImGui::Text("Exclusive Sex (Total):");
                Helpers::ShowTooltip("Total one-on-one encounters across all lovers (read-only, calculated from lovers)");
                ImGui::NextColumn();
                ImGui::Text("%d (read-only, sum from lovers)", npcData->exclusiveSex);
                ImGui::NextColumn();

                ImGui::Text("Group Sex (Total):");
                Helpers::ShowTooltip("Total group encounters across all lovers (read-only, calculated from lovers)");
                ImGui::NextColumn();
                ImGui::Text("%d (read-only, sum from lovers)", npcData->groupSex);
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
                Helpers::ShowTooltip("NPCs that this character has had sexual encounters with");

                if (ImGui::BeginTable("LoversTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Exclusive", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                    ImGui::TableSetupColumn("Orgasms", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    // Add tooltips to lovers table headers
                    if (ImGui::TableGetColumnFlags(0) & ImGuiTableColumnFlags_IsHovered) {
                        Helpers::ShowTooltip("Lover's name with gender icon");
                    }
                    if (ImGui::TableGetColumnFlags(1) & ImGuiTableColumnFlags_IsHovered) {
                        Helpers::ShowTooltip("One-on-one encounters with this lover");
                    }
                    if (ImGui::TableGetColumnFlags(2) & ImGuiTableColumnFlags_IsHovered) {
                        Helpers::ShowTooltip("Group encounters where both participated");
                    }
                    if (ImGui::TableGetColumnFlags(3) & ImGuiTableColumnFlags_IsHovered) {
                        Helpers::ShowTooltip("Total orgasms experienced together");
                    }
                    if (ImGui::TableGetColumnFlags(4) & ImGuiTableColumnFlags_IsHovered) {
                        Helpers::ShowTooltip("Relationship score based on encounters and intimacy");
                    }
                    if (ImGui::TableGetColumnFlags(5) & ImGuiTableColumnFlags_IsHovered) {
                        Helpers::ShowTooltip("Edit or remove this lover relationship");
                    }

                    for (const auto& lover : State::loversList) {
                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        // Display gender icon with color
                        Helpers::RenderGenderIcon(lover.formId);
                        ImGui::SameLine();
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

                        // Edit button (blue)
                        Helpers::PushEditButtonStyle();
                        if (ImGui::Button("Edit")) {
                            LoverEditor::SetSelectedLover(State::detailNPC, lover.formId);
                            State::showLoverEditorWindow = true;
                        }
                        Helpers::PopButtonStyle();
                        Helpers::ShowTooltip("Edit detailed relationship statistics for this lover");

                        ImGui::SameLine();

                        // Remove button (red)
                        Helpers::PushDeleteButtonStyle();
                        if (ImGui::Button("Remove")) {
                            ImGui::OpenPopup("ConfirmRemoveLover");
                        }
                        Helpers::PopButtonStyle();
                        Helpers::ShowTooltip("Remove this lover relationship (updates both NPCs)");

                        std::string confirmMsg = std::format("Remove lover relationship with {}?", lover.name);
                        if (Helpers::RenderConfirmationModal("ConfirmRemoveLover", confirmMsg.c_str(),
                                                             "This will update both NPCs' data.")) {
                            DataManager::RemoveLoverRelationship(State::detailNPC, lover.formId);
                            SetSelectedNPC(State::detailNPC);  // Refresh
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }

            // Actions Section
            if (ImGui::CollapsingHeader("Actions Performed/Received")) {
                ImGui::Columns(2, "ActionsColumns", true);

                // Left column - Actions Performed
                ImGui::Text("Actions Performed (Did):");
                ImGui::Separator();
                ImGui::BeginChild("ActionsDid", ImVec2(0, 200), false);
                for (const auto& [action, count] : npcData->actions_did) {
                    ImGui::Text("%s: %d", action.c_str(), count);
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                // Right column - Actions Received
                ImGui::Text("Actions Received (Got):");
                ImGui::Separator();
                ImGui::BeginChild("ActionsGot", ImVec2(0, 200), false);
                for (const auto& [action, count] : npcData->actions_got) {
                    ImGui::Text("%s: %d", action.c_str(), count);
                }
                ImGui::EndChild();

                ImGui::Columns(1);
            }
            }
            ImGui::End();

            // Pop window background color style
            ImGui::PopStyleColor();
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

            // Set window background to be fully opaque (non-transparent)
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));

            ImGui::SetNextWindowSize(ImVec2{800, 700}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin(windowTitle.c_str(), &State::showLoverEditorWindow, ImGuiWindowFlags_None)) {
                // Display NPC with gender icon
                Helpers::RenderGenderIcon(State::editorNPC);
                ImGui::SameLine();
                ImGui::Text("%s", npcName.c_str());

                ImGui::SameLine();
                ImGui::Text(" <-> ");

                // Display lover with gender icon
                ImGui::SameLine();
                Helpers::RenderGenderIcon(State::editorLover);
                ImGui::SameLine();
                ImGui::Text("%s", loverName.c_str());

            ImGui::Separator();

            ImGui::Checkbox("Sync changes to partner", &State::syncToPartner);
            Helpers::ShowTooltip("When enabled, changes will be mirrored to the partner. For internal climax, 'did' and 'got' are swapped automatically.");
            ImGui::TextWrapped("When enabled, changes will be applied to both NPCs in the relationship.");

            ImGui::Separator();

            ImGui::Columns(2, "LoverEditor");

            if (Helpers::RenderEditFieldWithSave("Exclusive Sex:", State::editLoverExclusiveSex, "loverExclusive",
                                                 "Number of one-on-one sexual encounters between these two lovers")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "exclusiveSex", State::editLoverExclusiveSex, State::syncToPartner);
                NPCDetailView::SetSelectedNPC(State::editorNPC);
            }

            if (Helpers::RenderEditFieldWithSave("Group Sex:", State::editLoverGroupSex, "loverGroup",
                                                 "Number of group sexual encounters where both were participants")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "partOfSameGroupSex", State::editLoverGroupSex, State::syncToPartner);
                NPCDetailView::SetSelectedNPC(State::editorNPC);
            }

            if (Helpers::RenderEditFieldWithSave("Orgasms:", State::editLoverOrgasms, "loverOrgasms",
                                                 "Total number of orgasms experienced together")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "orgasms", State::editLoverOrgasms, State::syncToPartner);
                NPCDetailView::SetSelectedNPC(State::editorNPC);
            }

            ImGui::Text("Last Time Together:");
            Helpers::ShowTooltip("Last time these two had a sexual encounter together");
            ImGui::NextColumn();
            ImGui::Text("%s", Helpers::FormatGameTime(State::editLoverLastTime).c_str());
            ImGui::NextColumn();

            if (Helpers::RenderEditFieldWithSave("Internal Climax (Did):", State::editLoverInternalDidCount, "loverInternalDid",
                                                 "Number of times this NPC finished inside their partner (creampies given)")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "internalClimax.did", State::editLoverInternalDidCount, State::syncToPartner);
                NPCDetailView::SetSelectedNPC(State::editorNPC);
            }

            if (Helpers::RenderEditFieldWithSave("Internal Climax (Got):", State::editLoverInternalGotCount, "loverInternalGot",
                                                 "Number of times this NPC received an internal climax from their partner (creampies received)")) {
                DataManager::UpdateLoverData(State::editorNPC, State::editorLover,
                                            "internalClimax.got", State::editLoverInternalGotCount, State::syncToPartner);
                NPCDetailView::SetSelectedNPC(State::editorNPC);
            }

            ImGui::Columns(1);
            }
            ImGui::End();

            // Pop window background color style
            ImGui::PopStyleColor();
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
