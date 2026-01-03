#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace LoversLedgerUI {
    // UI State structures
    struct NPCDisplayInfo {
        std::uint32_t formId;
        std::string name;
        int totalEncounters;
        int loversCount;
        float lastTime;
        float loverScore;  // For sorting
    };

    struct LoverDisplayInfo {
        std::uint32_t formId;
        std::string name;
        int exclusiveSex;
        int groupSex;
        float orgasms;
        float lastTime;
        float score;
    };

    // Main menu sections
    namespace NPCListView {
        void __stdcall Render();
        void RefreshNPCList();
    }

    namespace NPCDetailView {
        void __stdcall Render();
        void SetSelectedNPC(std::uint32_t formId);
    }

    namespace LoverEditor {
        void __stdcall Render();
        void SetSelectedLover(std::uint32_t npcFormId, std::uint32_t loverFormId);
    }

    // Data modification functions with cascading updates
    namespace DataManager {
        // Remove a lover relationship (updates both NPCs)
        bool RemoveLoverRelationship(std::uint32_t npcFormId, std::uint32_t loverFormId);

        // Delete an NPC entirely (cleans up all lover references)
        bool DeleteNPC(std::uint32_t npcFormId);

        // Update lover data (with option to sync to partner)
        bool UpdateLoverData(std::uint32_t npcFormId, std::uint32_t loverFormId,
                            const std::string& property, int value, bool syncToPartner);
        bool UpdateLoverData(std::uint32_t npcFormId, std::uint32_t loverFormId,
                            const std::string& property, float value, bool syncToPartner);

        // Update NPC data
        bool UpdateNPCData(std::uint32_t npcFormId, const std::string& property, int value);
        bool UpdateNPCData(std::uint32_t npcFormId, const std::string& property, float value);
    }

    // Helper functions
    namespace Helpers {
        std::string GetActorName(std::uint32_t formId);
        std::string FormatGameTime(float gameTime);
        std::string FormatFormID(std::uint32_t formId);
    }

    // Registration function to be called from plugin.cpp
    void RegisterMenu();
}
