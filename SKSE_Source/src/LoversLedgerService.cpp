#include "LoversLedgerService.h"

#include <algorithm>
#include <cstdlib>
#include <functional>

namespace LL {

    // ========================================================================================
    // Serialization Helper Functions (DRY)
    // ========================================================================================

    namespace {
        // Helper to write data with error logging
        template <typename T>
        bool WriteData(SKSE::SerializationInterface* a_intfc, const T& data, std::string_view fieldName) {
            if (!a_intfc->WriteRecordData(&data, sizeof(data))) {
                SKSE::log::error("Failed to write {}", fieldName);
                return false;
            }
            return true;
        }

        // Helper to read data with error logging
        template <typename T>
        bool ReadData(SKSE::SerializationInterface* a_intfc, T& data, std::string_view fieldName) {
            std::uint32_t readBytes = a_intfc->ReadRecordData(&data, sizeof(data));
            if (readBytes != sizeof(data)) {
                SKSE::log::error("Failed to read {}", fieldName);
                return false;
            }
            return true;
        }

        // Helper to get current game time
        float GetCurrentGameTime() {
            RE::Calendar* calendar = RE::Calendar::GetSingleton();
            return calendar ? calendar->GetDaysPassed() : 0.0f;
        }

        // Helper to select action map based on isDid flag
        template <typename MapType>
        const MapType& SelectActionMap(const NPCData& npc, bool isDid) {
            return isDid ? npc.actions_did : npc.actions_got;
        }

        template <typename MapType>
        MapType& SelectActionMap(NPCData& npc, bool isDid) {
            return isDid ? npc.actions_did : npc.actions_got;
        }

        // Helper to save a string
        bool SaveString(SKSE::SerializationInterface* a_intfc, std::string_view str, std::string_view fieldName) {
            std::uint32_t len = static_cast<std::uint32_t>(str.size());
            if (!WriteData(a_intfc, len, fieldName)) {
                return false;
            }
            if (!a_intfc->WriteRecordData(str.data(), len)) {
                SKSE::log::error("Failed to write {} data", fieldName);
                return false;
            }
            return true;
        }

        // Helper to load a string
        bool LoadString(SKSE::SerializationInterface* a_intfc, std::string& str, std::string_view fieldName) {
            std::uint32_t len = 0;
            if (!ReadData(a_intfc, len, fieldName)) {
                return false;
            }
            str.resize(len);
            std::uint32_t readBytes = a_intfc->ReadRecordData(str.data(), len);
            if (readBytes != len) {
                SKSE::log::error("Failed to read {} data", fieldName);
                return false;
            }
            return true;
        }

        // Helper to save a map<string, int>
        bool SaveStringIntMap(SKSE::SerializationInterface* a_intfc, const std::unordered_map<std::string, int>& map,
                              std::string_view mapName) {
            std::uint32_t count = static_cast<std::uint32_t>(map.size());
            if (!WriteData(a_intfc, count, mapName)) {
                return false;
            }
            for (const auto& [key, value] : map) {
                if (!SaveString(a_intfc, key, mapName) || !WriteData(a_intfc, value, mapName)) {
                    return false;
                }
            }
            return true;
        }

        // Helper to load a map<string, int>
        bool LoadStringIntMap(SKSE::SerializationInterface* a_intfc, std::unordered_map<std::string, int>& map,
                              std::string_view mapName) {
            std::uint32_t count = 0;
            if (!ReadData(a_intfc, count, mapName)) {
                return false;
            }
            map.clear();
            map.reserve(count);  // Optimize: reserve space to avoid rehashing
            for (std::uint32_t i = 0; i < count; ++i) {
                std::string key;
                int value;
                if (!LoadString(a_intfc, key, mapName) || !ReadData(a_intfc, value, mapName)) {
                    return false;
                }
                map[key] = value;
            }
            return true;
        }

        // Helper to normalize string to lowercase (Papyrus case-insensitive compatibility)
        std::string ToLower(std::string_view str) {
            std::string result(str);
            std::transform(result.begin(), result.end(), result.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            return result;
        }

        // Helper to parse dot-separated property path (normalized to lowercase)
        std::vector<std::string> ParsePropertyPath(std::string_view propName) {
            std::vector<std::string> parts;
            size_t start = 0;
            size_t end = propName.find('.');

            while (end != std::string_view::npos) {
                parts.emplace_back(ToLower(propName.substr(start, end - start)));
                start = end + 1;
                end = propName.find('.', start);
            }
            parts.emplace_back(ToLower(propName.substr(start)));

            return parts;
        }
    }  // namespace

    // ========================================================================================
    // InternalClimax Serialization
    // ========================================================================================

    void InternalClimax::Save(SKSE::SerializationInterface* a_intfc) const {
        WriteData(a_intfc, did, "InternalClimax.did");
        WriteData(a_intfc, got, "InternalClimax.got");
    }

    bool InternalClimax::Load(SKSE::SerializationInterface* a_intfc) {
        return ReadData(a_intfc, did, "InternalClimax.did") && ReadData(a_intfc, got, "InternalClimax.got");
    }

    // ========================================================================================
    // LoverData Serialization
    // ========================================================================================

    void LoverData::Save(SKSE::SerializationInterface* a_intfc, std::uint32_t loverFormID) const {
        WriteData(a_intfc, loverFormID, "LoverData.loverFormID");
        WriteData(a_intfc, exclusiveSex, "LoverData.exclusiveSex");
        WriteData(a_intfc, partOfSameGroupSex, "LoverData.partOfSameGroupSex");
        WriteData(a_intfc, lastTime, "LoverData.lastTime");
        WriteData(a_intfc, orgasms, "LoverData.orgasms");
        internalClimax.Save(a_intfc);
    }

    bool LoverData::Load(SKSE::SerializationInterface* a_intfc) {
        return ReadData(a_intfc, exclusiveSex, "LoverData.exclusiveSex") &&
               ReadData(a_intfc, partOfSameGroupSex, "LoverData.partOfSameGroupSex") &&
               ReadData(a_intfc, lastTime, "LoverData.lastTime") && ReadData(a_intfc, orgasms, "LoverData.orgasms") &&
               internalClimax.Load(a_intfc);
    }

    // ========================================================================================
    // NPCData Serialization
    // ========================================================================================

    void NPCData::Save(SKSE::SerializationInterface* a_intfc, std::uint32_t npcFormID) const {
        WriteData(a_intfc, npcFormID, "NPCData.npcFormID");
        WriteData(a_intfc, sameSexEncounter, "NPCData.sameSexEncounter");
        WriteData(a_intfc, soloSex, "NPCData.soloSex");
        WriteData(a_intfc, exclusiveSex, "NPCData.exclusiveSex");
        WriteData(a_intfc, groupSex, "NPCData.groupSex");
        WriteData(a_intfc, lastTime, "NPCData.lastTime");
        WriteData(a_intfc, relationshipsInitialized, "NPCData.relationshipsInitialized");
        WriteData(a_intfc, existingRelationsScan, "NPCData.existingRelationsScan");
        totalInternalClimax.Save(a_intfc);

        SaveStringIntMap(a_intfc, actions_did, "NPCData.actions_did");
        SaveStringIntMap(a_intfc, actions_got, "NPCData.actions_got");

        // Save lovers
        std::uint32_t loversCount = static_cast<std::uint32_t>(lovers.size());
        WriteData(a_intfc, loversCount, "NPCData.lovers count");
        for (const auto& [loverID, loverData] : lovers) {
            loverData.Save(a_intfc, loverID);
        }
    }

    bool NPCData::Load(SKSE::SerializationInterface* a_intfc) {
        if (!ReadData(a_intfc, sameSexEncounter, "NPCData.sameSexEncounter") ||
            !ReadData(a_intfc, soloSex, "NPCData.soloSex") ||
            !ReadData(a_intfc, exclusiveSex, "NPCData.exclusiveSex") ||
            !ReadData(a_intfc, groupSex, "NPCData.groupSex") || !ReadData(a_intfc, lastTime, "NPCData.lastTime") ||
            !ReadData(a_intfc, relationshipsInitialized, "NPCData.relationshipsInitialized")) {
            return false;
        }

        // Try to read existingRelationsScan if available (for backwards compatibility)
        if (!ReadData(a_intfc, existingRelationsScan, "NPCData.existingRelationsScan")) {
            existingRelationsScan = false;
        }

        if (!totalInternalClimax.Load(a_intfc)) {
            return false;
        }

        if (!LoadStringIntMap(a_intfc, actions_did, "NPCData.actions_did") ||
            !LoadStringIntMap(a_intfc, actions_got, "NPCData.actions_got")) {
            return false;
        }

        // Load lovers
        std::uint32_t loversCount = 0;
        if (!ReadData(a_intfc, loversCount, "NPCData.lovers count")) {
            return false;
        }

        lovers.clear();
        for (std::uint32_t i = 0; i < loversCount; ++i) {
            std::uint32_t loverFormID = 0;
            if (!ReadData(a_intfc, loverFormID, "lover formID")) {
                return false;
            }

            // Resolve formID after load
            std::uint32_t resolvedLoverID = 0;
            if (!a_intfc->ResolveFormID(loverFormID, resolvedLoverID)) {
                SKSE::log::warn("Failed to resolve lover formID 0x{:X}, skipping", loverFormID);
                // Skip this lover's data
                LoverData tempData;
                if (!tempData.Load(a_intfc)) {
                    return false;
                }
                continue;
            }

            LoverData loverData;
            if (!loverData.Load(a_intfc)) {
                return false;
            }
            lovers[resolvedLoverID] = loverData;
        }

        return true;
    }

    // ========================================================================================
    // LoversLedgerService Implementation
    // ========================================================================================

    LoversLedgerService& LoversLedgerService::GetSingleton() noexcept {
        static LoversLedgerService s;
        return s;
    }

    NPCData& LoversLedgerService::EnsureNPC(std::uint32_t formID) {
        // Caller must hold unique_lock

        auto& npcData = _store[formID];

        // Check if we need to scan existing relationships
        if (!npcData.existingRelationsScan) {
            if (_relationsFinderAPI) {
                try {
                    SKSE::log::info("Scanning existing relationships for NPC 0x{:X}", formID);
                    npcData.existingRelationsScan = true;
                    ScanExistingRelationships(formID, npcData);

                    SKSE::log::info("Scan completed successfully for NPC 0x{:X}", formID);
                } catch (const std::exception& e) {
                    SKSE::log::error("Exception during scan for NPC 0x{:X}: {}", formID, e.what());
                    npcData.existingRelationsScan = true;  // Mark as scanned to prevent retry
                } catch (...) {
                    SKSE::log::error("Unknown exception during scan for NPC 0x{:X}", formID);
                    npcData.existingRelationsScan = true;  // Mark as scanned to prevent retry
                }
            } else {
                SKSE::log::warn("EnsureNPC: RelationsFinder API not available yet for NPC 0x{:X}", formID);
            }
        }

        return npcData;
    }

    const NPCData* LoversLedgerService::FindNPC(std::uint32_t formID) const {
        // Caller must hold shared_lock or unique_lock
        auto it = _store.find(formID);
        return (it != _store.end()) ? &it->second : nullptr;
    }

    void LoversLedgerService::UpdateActions(std::uint32_t npcFormID, const std::vector<std::string>& actions,
                                            std::string_view didGot) {
        if (npcFormID == 0 || actions.empty()) {
            SKSE::log::warn("UpdateActions: Invalid parameters (npcFormID=0x{:X}, actions.size={})", npcFormID,
                            actions.size());
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);

        const bool isDid = (ToLower(didGot) == "did");
        auto& actionMap = SelectActionMap<std::unordered_map<std::string, int>>(npc, isDid);

        SKSE::log::debug("UpdateActions: npc=0x{:X}, didGot={}, {} actions", npcFormID, didGot, actions.size());

        for (const auto& action : actions) {
            ++actionMap[ToLower(action)];
            SKSE::log::trace("  Action '{}' count now: {}", action, actionMap[ToLower(action)]);
        }
    }

    void LoversLedgerService::IncrementInt(std::uint32_t npcFormID, std::string_view intName) {
        if (npcFormID == 0) {
            SKSE::log::warn("IncrementInt: Invalid npcFormID (0)");
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);

        // Normalize intName to lowercase for Papyrus compatibility
        std::string normalizedIntName = ToLower(intName);

        // Use static map for property access (lookup table pattern)
        static const std::unordered_map<std::string, std::function<void(NPCData&)>> incrementors = {
            {"samesexencounter", [](NPCData& n) { ++n.sameSexEncounter; }},
            {"solosex", [](NPCData& n) { ++n.soloSex; }},
            {"exclusivesex", [](NPCData& n) { ++n.exclusiveSex; }},
            {"groupsex", [](NPCData& n) { ++n.groupSex; }}};

        auto it = incrementors.find(normalizedIntName);
        if (it != incrementors.end()) {
            it->second(npc);
            SKSE::log::debug("IncrementInt: npc=0x{:X}, intName={} incremented", npcFormID, intName);
        } else {
            SKSE::log::warn("LoversLedgerService::IncrementInt - unknown intName: {}", intName);
        }
    }

    void LoversLedgerService::UpdateLover(std::uint32_t npcFormID, std::uint32_t loverFormID,
                                          std::string_view encounterType, float orgasms) {
        if (npcFormID == 0 || loverFormID == 0) {
            SKSE::log::warn("UpdateLover: Invalid FormID - npcFormID=0x{:X}, loverFormID=0x{:X}", npcFormID,
                            loverFormID);
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);
        auto& lover = npc.lovers[loverFormID];

        // Normalize encounterType to lowercase for Papyrus compatibility
        std::string normalizedType = ToLower(encounterType);

        SKSE::log::debug("UpdateLover: npc=0x{:X}, lover=0x{:X}, type={}, orgasms={}", npcFormID, loverFormID,
                         encounterType, orgasms);

        if (normalizedType == "exclusivesex") {
            ++lover.exclusiveSex;
            SKSE::log::trace("  exclusiveSex now: {}", lover.exclusiveSex);
        } else if (normalizedType == "partofsamegroupsex") {
            ++lover.partOfSameGroupSex;
            SKSE::log::trace("  partOfSameGroupSex now: {}", lover.partOfSameGroupSex);
        }

        lover.orgasms += orgasms;
        SKSE::log::trace("  total orgasms now: {}", lover.orgasms);

        // Update lover's last time to current game time
        float currentTime = GetCurrentGameTime();
        if (currentTime > 0.0f) {
            lover.lastTime = currentTime;
            // Update NPC's last time if lover's time is newer
            npc.lastTime = std::max(npc.lastTime, lover.lastTime);
            SKSE::log::trace("  lastTime updated to: {}", currentTime);
        } else {
            SKSE::log::warn("  Failed to get current game time");
        }
    }

    void LoversLedgerService::UpdateLastTime(std::uint32_t npcFormID, std::uint32_t loverFormID) {
        if (npcFormID == 0) {
            return;
        }

        float daysPassed = GetCurrentGameTime();
        if (daysPassed == 0.0f) {
            SKSE::log::warn("LoversLedgerService::UpdateLastTime - Calendar not available");
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);
        npc.lastTime = daysPassed;

        if (loverFormID != 0) {
            npc.lovers[loverFormID].lastTime = daysPassed;
        }
    }

    void LoversLedgerService::UpdateInternalClimax(std::uint32_t npcFormID, std::uint32_t loverFormID,
                                                   std::string_view didGot) {
        if (npcFormID == 0) {
            SKSE::log::warn("UpdateInternalClimax: Invalid npcFormID (0)");
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);

        const bool isDid = (ToLower(didGot) == "did");
        auto& npcClimax = isDid ? npc.totalInternalClimax.did : npc.totalInternalClimax.got;
        ++npcClimax;

        SKSE::log::debug("UpdateInternalClimax: npc=0x{:X}, lover=0x{:X}, didGot={}, total now: {}", npcFormID,
                         loverFormID, didGot, npcClimax);

        if (loverFormID != 0) {
            auto& loverClimax =
                isDid ? npc.lovers[loverFormID].internalClimax.did : npc.lovers[loverFormID].internalClimax.got;
            ++loverClimax;
            SKSE::log::trace("  Lover climax now: {}", loverClimax);
        }
    }

    int LoversLedgerService::GetNpcInt(std::uint32_t npcFormID, std::string_view propName) const {
        if (npcFormID == 0) {
            SKSE::log::warn("GetNpcInt: Invalid npcFormID (0)");
            return 0;
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc) {
            SKSE::log::trace("GetNpcInt: NPC 0x{:X} not found in ledger, prop={}", npcFormID, propName);
            return 0;
        }

        auto parts = ParsePropertyPath(propName);
        if (parts.empty()) {
            SKSE::log::warn("GetNpcInt: Empty property path for NPC 0x{:X}", npcFormID);
            return 0;
        }

        // Single property lookup
        if (parts.size() == 1) {
            static const std::unordered_map<std::string, std::function<int(const NPCData&)>> intGetters = {
                {"samesexencounter", [](const NPCData& n) { return n.sameSexEncounter; }},
                {"solosex", [](const NPCData& n) { return n.soloSex; }},
                {"exclusivesex", [](const NPCData& n) { return n.exclusiveSex; }},
                {"groupsex", [](const NPCData& n) { return n.groupSex; }}};

            auto it = intGetters.find(parts[0]);
            if (it == intGetters.end()) {
                SKSE::log::trace("GetNpcInt: Unknown property '{}' for NPC 0x{:X}", propName, npcFormID);
            }
            return (it != intGetters.end()) ? it->second(*npc) : 0;
        }

        // Nested property lookup
        if (parts.size() == 2) {
            const auto& category = parts[0];
            const auto& field = parts[1];

            if (category == "totalinternalclimax") {
                return (field == "did")   ? npc->totalInternalClimax.did
                       : (field == "got") ? npc->totalInternalClimax.got
                                          : 0;
            }

            const auto& actionMap =
                SelectActionMap<std::unordered_map<std::string, int>>(*npc, category == "actions_did");
            if (category == "actions_did" || category == "actions_got") {
                auto it = actionMap.find(field);
                return (it != actionMap.end()) ? it->second : 0;
            }
        }

        SKSE::log::trace("GetNpcInt: Property path '{}' not recognized for NPC 0x{:X}", propName, npcFormID);
        return 0;
    }

    float LoversLedgerService::GetNpcFloat(std::uint32_t npcFormID, std::string_view propName) const {
        if (npcFormID == 0) {
            return 0.0f;
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc) {
            return 0.0f;
        }

        auto parts = ParsePropertyPath(propName);
        // Normalized to lowercase by ParsePropertyPath
        if (parts.size() == 1 && parts[0] == "lasttime") {
            return npc->lastTime;
        }

        return 0.0f;
    }

    int LoversLedgerService::GetLoverInt(std::uint32_t npcFormID, std::uint32_t loverFormID,
                                         std::string_view propName) const {
        if (npcFormID == 0 || loverFormID == 0) {
            return 0;
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc) {
            return 0;
        }

        auto loverIt = npc->lovers.find(loverFormID);
        if (loverIt == npc->lovers.end()) {
            return 0;
        }

        const auto& lover = loverIt->second;
        auto parts = ParsePropertyPath(propName);
        if (parts.empty()) {
            return 0;
        }

        // Single property names - use lookup table (keys are lowercase due to ParsePropertyPath normalization)
        if (parts.size() == 1) {
            static const std::unordered_map<std::string, std::function<int(const LoverData&)>> intGetters = {
                {"exclusivesex", [](const LoverData& l) { return l.exclusiveSex; }},
                {"partofsamegroupsex", [](const LoverData& l) { return l.partOfSameGroupSex; }}};

            auto it = intGetters.find(parts[0]);
            return (it != intGetters.end()) ? it->second(lover) : 0;
        }

        // Nested properties
        if (parts.size() == 2 && parts[0] == "internalclimax") {
            if (parts[1] == "did") return lover.internalClimax.did;
            if (parts[1] == "got") return lover.internalClimax.got;
        }

        return 0;
    }

    float LoversLedgerService::GetLoverFloat(std::uint32_t npcFormID, std::uint32_t loverFormID,
                                             std::string_view propName) const {
        if (npcFormID == 0 || loverFormID == 0) {
            return 0.0f;
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc) {
            return 0.0f;
        }

        auto loverIt = npc->lovers.find(loverFormID);
        if (loverIt == npc->lovers.end()) {
            return 0.0f;
        }

        const auto& lover = loverIt->second;
        auto parts = ParsePropertyPath(propName);

        // Use lookup table for float properties (keys are lowercase due to ParsePropertyPath normalization)
        if (parts.size() == 1) {
            static const std::unordered_map<std::string, std::function<float(const LoverData&)>> floatGetters = {
                {"lasttime", [](const LoverData& l) { return l.lastTime; }},
                {"orgasms", [](const LoverData& l) { return l.orgasms; }}};

            auto it = floatGetters.find(parts[0]);
            return (it != floatGetters.end()) ? it->second(lover) : 0.0f;
        }

        return 0.0f;
    }

    void LoversLedgerService::SetNpcInt(std::uint32_t npcFormID, std::string_view propName, int value) {
        if (npcFormID == 0) {
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);

        auto parts = ParsePropertyPath(propName);
        if (parts.empty()) {
            return;
        }

        // Single property lookup
        if (parts.size() == 1) {
            static const std::unordered_map<std::string, std::function<void(NPCData&, int)>> intSetters = {
                {"samesexencounter", [](NPCData& n, int v) { n.sameSexEncounter = v; }},
                {"solosex", [](NPCData& n, int v) { n.soloSex = v; }},
                {"exclusivesex", [](NPCData& n, int v) { n.exclusiveSex = v; }},
                {"groupsex", [](NPCData& n, int v) { n.groupSex = v; }}};

            auto it = intSetters.find(parts[0]);
            if (it != intSetters.end()) {
                it->second(npc, value);
            }
            return;
        }

        // Nested property lookup
        if (parts.size() == 2) {
            const auto& category = parts[0];
            const auto& field = parts[1];

            if (category == "totalinternalclimax") {
                if (field == "did") {
                    npc.totalInternalClimax.did = value;
                } else if (field == "got") {
                    npc.totalInternalClimax.got = value;
                }
                return;
            }

            if (category == "actions_did" || category == "actions_got") {
                auto& actionMap = SelectActionMap<std::unordered_map<std::string, int>>(npc, category == "actions_did");
                actionMap[field] = value;
                return;
            }
        }
    }

    void LoversLedgerService::SetNpcFloat(std::uint32_t npcFormID, std::string_view propName, float value) {
        if (npcFormID == 0) {
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1 && parts[0] == "lasttime") {
            npc.lastTime = value;
        }
    }

    void LoversLedgerService::SetLoverInt(std::uint32_t npcFormID, std::uint32_t loverFormID, std::string_view propName,
                                          int value) {
        if (npcFormID == 0 || loverFormID == 0) {
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);
        auto& lover = npc.lovers[loverFormID];

        auto parts = ParsePropertyPath(propName);
        if (parts.empty()) {
            return;
        }

        // Single property names
        if (parts.size() == 1) {
            static const std::unordered_map<std::string, std::function<void(LoverData&, int)>> intSetters = {
                {"exclusivesex", [](LoverData& l, int v) { l.exclusiveSex = v; }},
                {"partofsamegroupsex", [](LoverData& l, int v) { l.partOfSameGroupSex = v; }}};

            auto it = intSetters.find(parts[0]);
            if (it != intSetters.end()) {
                it->second(lover, value);
            }
            return;
        }

        // Nested properties
        if (parts.size() == 2 && parts[0] == "internalclimax") {
            if (parts[1] == "did") {
                lover.internalClimax.did = value;
            } else if (parts[1] == "got") {
                lover.internalClimax.got = value;
            }
        }
    }

    void LoversLedgerService::SetLoverFloat(std::uint32_t npcFormID, std::uint32_t loverFormID,
                                            std::string_view propName, float value) {
        if (npcFormID == 0 || loverFormID == 0) {
            return;
        }

        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcFormID);
        auto& lover = npc.lovers[loverFormID];

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1) {
            static const std::unordered_map<std::string, std::function<void(LoverData&, float)>> floatSetters = {
                {"lasttime", [](LoverData& l, float v) { l.lastTime = v; }},
                {"orgasms", [](LoverData& l, float v) { l.orgasms = v; }}};

            auto it = floatSetters.find(parts[0]);
            if (it != floatSetters.end()) {
                it->second(lover, value);
            }
        }
    }

    std::vector<std::uint32_t> LoversLedgerService::GetAllNPCs() const {
        std::shared_lock lock(_mutex);
        std::vector<std::uint32_t> npcFormIDs;
        npcFormIDs.reserve(_store.size());

        for (const auto& [formID, npcData] : _store) {
            npcFormIDs.push_back(formID);
        }

        return npcFormIDs;
    }

    std::vector<std::uint32_t> LoversLedgerService::GetAllLovers(std::uint32_t npcFormID, int topK) const {
        if (npcFormID == 0) {
            return {};
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc || npc->lovers.empty()) {
            return {};
        }

        // Create vector of (formID, score) pairs
        std::vector<std::pair<std::uint32_t, float>> loverScores;
        loverScores.reserve(npc->lovers.size());

        for (const auto& [loverID, loverData] : npc->lovers) {
            // Calculate score for each lover - reuse GetLoverScore logic
            float score = GetLoverScore(npcFormID, loverID);
            loverScores.emplace_back(loverID, score);
        }

        // Sort by score in descending order (highest score first)
        std::sort(loverScores.begin(), loverScores.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        // Extract formIDs, respecting topK
        size_t resultSize = (topK <= 0) ? loverScores.size() : std::min(static_cast<size_t>(topK), loverScores.size());
        std::vector<std::uint32_t> loverFormIDs;
        loverFormIDs.reserve(resultSize);

        for (size_t i = 0; i < resultSize; ++i) {
            loverFormIDs.push_back(loverScores[i].first);
        }

        return loverFormIDs;
    }

    std::vector<std::string> LoversLedgerService::GetAllActions(std::uint32_t npcFormID, bool isDid, int topK) const {
        if (npcFormID == 0) {
            return {};
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc) {
            return {};
        }

        const auto& actionMap = SelectActionMap<std::unordered_map<std::string, int>>(*npc, isDid);

        // Create sorted vector of (actionName, count) pairs
        std::vector<std::pair<std::string, int>> actionPairs(actionMap.begin(), actionMap.end());
        std::sort(actionPairs.begin(), actionPairs.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        // Extract action names, respecting topK
        size_t resultSize = (topK <= 0) ? actionPairs.size() : std::min(static_cast<size_t>(topK), actionPairs.size());
        std::vector<std::string> result;
        result.reserve(resultSize);

        for (size_t i = 0; i < resultSize; ++i) {
            result.push_back(actionPairs[i].first);
        }

        return result;
    }

    int LoversLedgerService::GetActionCount(std::uint32_t npcFormID, bool isDid, std::string_view actionName) const {
        if (npcFormID == 0) {
            return 0;
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc) {
            return 0;
        }

        const auto& actionMap = SelectActionMap<std::unordered_map<std::string, int>>(*npc, isDid);
        auto it = actionMap.find(ToLower(actionName));
        return (it != actionMap.end()) ? it->second : 0;
    }

    float LoversLedgerService::GetLoverScore(std::uint32_t npcFormID, std::uint32_t loverFormID) const {
        if (npcFormID == 0 || loverFormID == 0) {
            return 0.0f;
        }

        std::shared_lock lock(_mutex);
        const auto* npc = FindNPC(npcFormID);
        if (!npc) {
            return 0.0f;
        }

        auto loverIt = npc->lovers.find(loverFormID);
        if (loverIt == npc->lovers.end()) {
            return 0.0f;
        }

        const auto& lover = loverIt->second;

        // Extract lover data
        float orgasms = lover.orgasms;
        int exclSex = lover.exclusiveSex;
        int groupSex = lover.partOfSameGroupSex;
        float lastTime = lover.lastTime;

        // Get current game time
        float currentTime = GetCurrentGameTime();
        if (currentTime == 0.0f) {
            SKSE::log::warn("GetLoverScore: Calendar not available, using time diff = 0");
        }

        float diff = currentTime - lastTime;

        // Calculate base score using square root scaling
        float baseScore = std::sqrt(static_cast<float>(exclSex)) * 6.0f +
                          std::sqrt(static_cast<float>(groupSex)) * 2.0f + std::sqrt(orgasms) * 5.0f;

        // Time decay multiplier
        float timeMultiplier = 0.0f;
        if (diff < 1.0f) {
            timeMultiplier = 1.0f;  // Full value if recent (less than 1 day)
        } else if (diff < 7.0f) {
            timeMultiplier = 0.8f;  // 80% value if within a week
        } else if (diff < 30.0f) {
            timeMultiplier = 0.6f;  // 60% if within a month
        } else if (diff < 180.0f) {
            timeMultiplier = 0.3f;  // 30% if within 6 months
        } else if (diff < 365.0f) {
            timeMultiplier = 0.1f;  // 10% if within a year
        } else {
            timeMultiplier = 0.05f;  // Minimal connection after a year
        }

        return baseScore * timeMultiplier;
    }

    void LoversLedgerService::ClearAll() {
        std::unique_lock lock(_mutex);
        _store.clear();
        SKSE::log::info("LoversLedgerService: All data cleared");
    }

    std::pair<int, int> LoversLedgerService::CleanupInvalidFormIDs() {
        std::unique_lock lock(_mutex);

        int npcsRemoved = 0;
        int loversRemoved = 0;

        SKSE::log::info("CleanupInvalidFormIDs: Starting cleanup...");

        // First pass: remove invalid NPCs
        std::vector<std::uint32_t> npcsToRemove;
        for (const auto& [npcFormID, npcData] : _store) {
            auto* form = RE::TESForm::LookupByID(npcFormID);
            if (!form || !form->As<RE::Actor>()) {
                npcsToRemove.push_back(npcFormID);
            }
        }

        for (auto npcFormID : npcsToRemove) {
            _store.erase(npcFormID);
            npcsRemoved++;
            SKSE::log::debug("  Removed invalid NPC FormID 0x{:X}", npcFormID);
        }

        // Second pass: remove invalid lovers from remaining NPCs
        for (auto& [npcFormID, npcData] : _store) {
            std::vector<std::uint32_t> loversToRemove;

            for (const auto& [loverFormID, loverData] : npcData.lovers) {
                auto* form = RE::TESForm::LookupByID(loverFormID);
                if (!form || !form->As<RE::Actor>()) {
                    loversToRemove.push_back(loverFormID);
                }
            }

            for (auto loverFormID : loversToRemove) {
                npcData.lovers.erase(loverFormID);
                loversRemoved++;
                SKSE::log::debug("  Removed invalid lover FormID 0x{:X} from NPC 0x{:X}", loverFormID, npcFormID);
            }
        }

        SKSE::log::info("CleanupInvalidFormIDs: Cleanup complete - {} NPCs removed, {} lovers removed",
                       npcsRemoved, loversRemoved);

        return {npcsRemoved, loversRemoved};
    }

    void LoversLedgerService::SetRelationsFinderAPI(
        RelationsFinderAPI::GetNpcRelationshipsCallbackFn apiFunc) noexcept {
        _relationsFinderAPI = apiFunc;
        SKSE::log::info("LoversLedgerService: RelationsFinder API set (ptr: {})", fmt::ptr(apiFunc));
    }

    void LoversLedgerService::ScanExistingRelationships(std::uint32_t npcFormID, [[maybe_unused]] NPCData& npcData) {
        // IMPORTANT: This method is called from EnsureNPC, which already holds a unique_lock on _mutex
        // Do NOT acquire the lock again here to avoid deadlock
        // We re-lookup npcData from _store after each lover creation to handle potential map rehashing

        if (!_relationsFinderAPI) {
            SKSE::log::warn("ScanExistingRelationships: RelationsFinder API not available");
            return;
        }

        // Get the actor from the formID
        RE::TESForm* npcForm = RE::TESForm::LookupByID(npcFormID);
        if (!npcForm) {
            SKSE::log::warn("ScanExistingRelationships: Failed to lookup form 0x{:X}", npcFormID);
            return;
        }

        RE::Actor* npc = npcForm->As<RE::Actor>();
        if (!npc) {
            SKSE::log::warn("ScanExistingRelationships: Form 0x{:X} is not an actor", npcFormID);
            return;
        }

        // Skip scanning if this is the player
        if (npc->IsPlayerRef()) {
            SKSE::log::info("ScanExistingRelationships: Skipping scan for player");
            return;
        }

        const char* npcName = npc->GetName();
        SKSE::log::info("ScanExistingRelationships: Scanning relationships for NPC 0x{:X} ({})", npcFormID,
                        npcName ? npcName : "Unknown");

        // Collect all lover FormIDs using callback API (safe across DLLs)
        std::vector<std::pair<std::uint32_t, int>> loversToCreate;  // <formID, type: 0=spouse, 1=courting, 2=lover>

        // Callback lambda to collect spouse FormIDs
        auto collectSpouses = [](RE::Actor* actor, void* userData) {
            if (actor) {
                auto* lovers = static_cast<std::vector<std::pair<std::uint32_t, int>>*>(userData);
                lovers->emplace_back(actor->GetFormID(), 0);  // 0 = spouse
            }
        };

        // Query spouses (associationType = "Spouse", minRank = 0)
        _relationsFinderAPI(npc, "Spouse", "any", -4, -999, collectSpouses, &loversToCreate);
        SKSE::log::info("  Found {} spouses", loversToCreate.size());

        // Callback lambda to collect courting relationships
        auto collectCourting = [](RE::Actor* actor, void* userData) {
            if (actor) {
                auto* lovers = static_cast<std::vector<std::pair<std::uint32_t, int>>*>(userData);
                lovers->emplace_back(actor->GetFormID(), 1);  // 1 = courting
            }
        };

        // Query courting relationships (associationType = "PotentialMarriageFaction", minRank = 0)
        size_t beforeCourting = loversToCreate.size();
        _relationsFinderAPI(npc, "courting", "any", -4, -999, collectCourting, &loversToCreate);
        SKSE::log::info("  Found {} courting relationships", loversToCreate.size() - beforeCourting);

        // Callback lambda to collect lovers
        auto collectLovers = [](RE::Actor* actor, void* userData) {
            if (actor) {
                auto* lovers = static_cast<std::vector<std::pair<std::uint32_t, int>>*>(userData);
                lovers->emplace_back(actor->GetFormID(), 2);  // 2 = lover
            }
        };

        // Query lovers (any relationship rank >= 3, which corresponds to "Lover" in the enum)
        size_t beforeLovers = loversToCreate.size();
        _relationsFinderAPI(npc, "", "any", 4, 4, collectLovers, &loversToCreate);
        SKSE::log::info("  Found {} lover relationships", loversToCreate.size() - beforeLovers);

        // Now create all lovers - this avoids passing npcData reference which could be invalidated
        if (!loversToCreate.empty()) {
            // Re-lookup and reserve space in lovers map to minimize rehashing
            auto it = _store.find(npcFormID);
            if (it != _store.end()) {
                it->second.lovers.reserve(it->second.lovers.size() + loversToCreate.size());
            }
        }

        for (const auto& [loverFormID, type] : loversToCreate) {
            // Re-lookup npcData each time to avoid invalidation
            auto it = _store.find(npcFormID);
            if (it == _store.end()) {
                SKSE::log::error("NPC 0x{:X} disappeared from store!", npcFormID);
                return;
            }

            if (it->second.lovers.find(loverFormID) == it->second.lovers.end()) {
                bool isSpouse = (type == 0);
                bool isCourting = (type == 1);
                bool isLover = (type == 2);
                CreateExistingLoverInternal(npcFormID, loverFormID, isSpouse, isCourting, isLover);
            }
        }

        SKSE::log::info("ScanExistingRelationships: Scan complete for NPC 0x{:X}", npcFormID);
    }

    void LoversLedgerService::CreateExistingLoverInternal(std::uint32_t npcFormID, std::uint32_t loverFormID,
                                                          bool isSpouse, bool isCourting, bool isLover) {
        // IMPORTANT: Called from ScanExistingRelationships which is called from EnsureNPC (holds unique_lock)
        // Do NOT acquire lock here

        SKSE::log::debug("CreateExistingLoverInternal: START - NPC 0x{:X}, Lover 0x{:X}", npcFormID, loverFormID);

        if (loverFormID == 0 || loverFormID == npcFormID) {
            SKSE::log::warn("CreateExistingLoverInternal: Invalid lover FormID (0x{:X}) for NPC 0x{:X}", loverFormID,
                            npcFormID);
            return;
        }

        float currentTime = GetCurrentGameTime();
        int sexTimes = 0;
        float lastTime = currentTime;
        float orgasms = 0.0f;
        int didInternal = 0;
        int gotInternal = 0;

        // Check if lover already has NPC as a lover (reciprocal relationship)
        auto loverIt = _store.find(loverFormID);
        bool hasReciprocalData = false;

        if (loverIt != _store.end()) {
            auto reciprocalLoverIt = loverIt->second.lovers.find(npcFormID);
            if (reciprocalLoverIt != loverIt->second.lovers.end()) {
                // Use existing data from the reciprocal relationship
                const auto& reciprocalData = reciprocalLoverIt->second;
                sexTimes = reciprocalData.exclusiveSex;
                lastTime = reciprocalData.lastTime;
                orgasms = reciprocalData.orgasms;
                didInternal = reciprocalData.internalClimax.got;  // Flip: their "got" is our "did"
                gotInternal = reciprocalData.internalClimax.did;  // Flip: their "did" is our "got"
                hasReciprocalData = true;
                SKSE::log::debug("  Using reciprocal data from lover 0x{:X}", loverFormID);
            }
        }

        // Generate stats if no reciprocal data exists
        if (!hasReciprocalData) {
            // Generate sexTimes based on relationship type
            if (isSpouse) {
                sexTimes = 40 + (std::rand() % 61);  // 40-100
            } else if (isCourting) {
                sexTimes = std::rand() % 11;  // 0-10
            } else if (isLover) {
                sexTimes = 10 + (std::rand() % 31);  // 10-40
            }

            // Generate lastTime based on relationship activity
            if (isLover) {
                lastTime -= (1 + (std::rand() % 6));  // 1-6 days ago (active lovers)
            } else {
                lastTime -= (180 + (std::rand() % 181));  // 180-360 days ago (inactive)
            }

            // Generate orgasms (70%-100% of encounters)
            orgasms = sexTimes * (0.7f + static_cast<float>(std::rand() % 31) / 100.0f);

            // Generate internal climax for spouses based on NPC gender
            if (isSpouse) {
                RE::TESForm* npcForm = RE::TESForm::LookupByID(npcFormID);
                if (npcForm) {
                    RE::Actor* npcActor = npcForm->As<RE::Actor>();
                    if (npcActor) {
                        RE::TESNPC* actorBase = npcActor->GetActorBase();
                        if (actorBase) {
                            bool isMale = (actorBase->GetSex() == RE::SEXES::kMale);
                            int halfOrgasms = static_cast<int>(orgasms / 2.0f);
                            if (isMale) {
                                didInternal = halfOrgasms;
                            } else {
                                gotInternal = halfOrgasms;
                            }
                        }
                    }
                }
            }
        }

        // Update NPC data (lookup fresh to avoid invalidation)
        auto npcIt = _store.find(npcFormID);
        if (npcIt == _store.end()) {
            SKSE::log::error("CreateExistingLoverInternal: NPC 0x{:X} not in store", npcFormID);
            return;
        }

        // Create and populate lover entry
        auto& loverData = npcIt->second.lovers[loverFormID];
        loverData.exclusiveSex = sexTimes;
        loverData.partOfSameGroupSex = 0;
        loverData.lastTime = lastTime;
        loverData.orgasms = orgasms;
        loverData.internalClimax.did = didInternal;
        loverData.internalClimax.got = gotInternal;

        // Re-lookup NPC (map may have reallocated)
        npcIt = _store.find(npcFormID);
        if (npcIt == _store.end()) {
            SKSE::log::error("CreateExistingLoverInternal: NPC 0x{:X} disappeared", npcFormID);
            return;
        }

        // Update NPC totals
        npcIt->second.exclusiveSex += sexTimes;
        npcIt->second.lastTime = std::max(npcIt->second.lastTime, lastTime);
        npcIt->second.totalInternalClimax.did += didInternal;
        npcIt->second.totalInternalClimax.got += gotInternal;

        // Ensure partner is also in the Lover's Ledger with reciprocal data
        // Only create reciprocal if we generated new data (not using existing reciprocal data)
        if (!hasReciprocalData) {
            // Check if partner exists in store, if not create them
            auto partnerIt = _store.find(loverFormID);
            if (partnerIt == _store.end()) {
                // Partner doesn't exist, create entry
                _store[loverFormID] = NPCData{};
                partnerIt = _store.find(loverFormID);
                SKSE::log::debug("  Created new NPCData entry for partner 0x{:X}", loverFormID);
            }

            // Check if partner already has this NPC as a lover
            auto& partnerData = partnerIt->second;
            if (partnerData.lovers.find(npcFormID) == partnerData.lovers.end()) {
                // Create reciprocal relationship with flipped did/got for internal climax
                auto& reciprocalLoverData = partnerData.lovers[npcFormID];
                reciprocalLoverData.exclusiveSex = sexTimes;
                reciprocalLoverData.partOfSameGroupSex = 0;
                reciprocalLoverData.lastTime = lastTime;
                reciprocalLoverData.orgasms = orgasms;
                reciprocalLoverData.internalClimax.did = gotInternal;  // Flip: our "got" is their "did"
                reciprocalLoverData.internalClimax.got = didInternal;  // Flip: our "did" is their "got"

                // Re-lookup partner (map may have reallocated)
                partnerIt = _store.find(loverFormID);
                if (partnerIt != _store.end()) {
                    // Update partner's totals
                    partnerIt->second.exclusiveSex += sexTimes;
                    partnerIt->second.lastTime = std::max(partnerIt->second.lastTime, lastTime);
                    partnerIt->second.totalInternalClimax.did += gotInternal;
                    partnerIt->second.totalInternalClimax.got += didInternal;

                    SKSE::log::debug(
                        "  Created reciprocal relationship for partner 0x{:X} -> NPC 0x{:X}, sexTimes:{}, "
                        "didInternal:{}, gotInternal:{}",
                        loverFormID, npcFormID, sexTimes, gotInternal, didInternal);
                }
            } else {
                SKSE::log::debug("  Partner 0x{:X} already has NPC 0x{:X} as lover, skipping reciprocal creation",
                                 loverFormID, npcFormID);
            }
        }

        SKSE::log::debug(
            "CreateExistingLoverInternal: COMPLETE - NPC 0x{:X}, Lover 0x{:X}, sexTimes:{}, orgasms:{}, "
            "didInternal:{}, gotInternal:{}",
            npcFormID, loverFormID, sexTimes, orgasms, didInternal, gotInternal);
    }

    // ========================================================================================
    // SKSE Serialization Callbacks
    // ========================================================================================

    void LoversLedgerService::SaveCallback(SKSE::SerializationInterface* a_intfc) {
        SKSE::log::info("LoversLedgerService: Saving data...");

        std::shared_lock lock(_mutex);

        std::uint32_t numNPCs = static_cast<std::uint32_t>(_store.size());
        SKSE::log::info("Saving {} NPC records", numNPCs);

        if (!a_intfc->OpenRecord(kSerializationType, kSerializationVersion)) {
            SKSE::log::error("Failed to open LoversLedger save record");
            return;
        }

        if (!a_intfc->WriteRecordData(&numNPCs, sizeof(numNPCs))) {
            SKSE::log::error("Failed to write NPC count");
            return;
        }

        for (const auto& [npcFormID, npcData] : _store) {
            npcData.Save(a_intfc, npcFormID);
        }

        SKSE::log::info("LoversLedgerService: Save complete");
    }

    void LoversLedgerService::LoadCallback(SKSE::SerializationInterface* a_intfc) {
        SKSE::log::info("LoversLedgerService: Loading data...");

        std::uint32_t type;
        std::uint32_t version;
        std::uint32_t length;

        while (a_intfc->GetNextRecordInfo(type, version, length)) {
            if (type != kSerializationType) {
                continue;
            }

            if (version != kSerializationVersion) {
                SKSE::log::error("Unsupported serialization version: {}", version);
                continue;
            }

            std::uint32_t numNPCs = 0;
            std::uint32_t readBytes = a_intfc->ReadRecordData(&numNPCs, sizeof(numNPCs));
            if (readBytes != sizeof(numNPCs)) {
                SKSE::log::error("Failed to read NPC count");
                continue;
            }

            SKSE::log::info("Loading {} NPC records", numNPCs);

            {
                std::unique_lock lock(_mutex);
                _store.clear();

                for (std::uint32_t i = 0; i < numNPCs; ++i) {
                    std::uint32_t npcFormID = 0;
                    readBytes = a_intfc->ReadRecordData(&npcFormID, sizeof(npcFormID));
                    if (readBytes != sizeof(npcFormID)) {
                        SKSE::log::error("Failed to read NPC formID");
                        continue;
                    }

                    // Resolve formID after load
                    std::uint32_t resolvedNPCID = 0;
                    if (!a_intfc->ResolveFormID(npcFormID, resolvedNPCID)) {
                        SKSE::log::warn("Failed to resolve NPC formID 0x{:X}, skipping", npcFormID);
                        // Skip this NPC's data
                        NPCData tempData;
                        if (!tempData.Load(a_intfc)) {
                            SKSE::log::error("Failed to skip invalid NPC data");
                        }
                        continue;
                    }

                    NPCData npcData;
                    if (!npcData.Load(a_intfc)) {
                        SKSE::log::error("Failed to load NPC data for formID 0x{:X}", resolvedNPCID);
                        continue;
                    }

                    _store[resolvedNPCID] = std::move(npcData);
                }

                SKSE::log::info("LoversLedgerService: Load complete - {} NPCs loaded", _store.size());
            }  // lock is released here

            // Automatically cleanup invalid FormIDs after loading
            SKSE::log::info("Running automatic cleanup of invalid FormIDs...");
            auto [npcsRemoved, loversRemoved] = CleanupInvalidFormIDs();
            if (npcsRemoved > 0 || loversRemoved > 0) {
                SKSE::log::info("Automatic cleanup removed {} invalid NPCs and {} invalid lovers", npcsRemoved,
                               loversRemoved);
            }
        }
    }

    void LoversLedgerService::RevertCallback([[maybe_unused]] SKSE::SerializationInterface* a_intfc) {
        SKSE::log::info("LoversLedgerService: Reverting data (new game)...");
        ClearAll();
    }

}  // namespace LL
