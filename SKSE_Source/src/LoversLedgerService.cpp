#include "LoversLedgerService.h"

#include <algorithm>
#include <cstdlib>
#include <functional>

#include "UniqueOverrides.h"

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
        totalInternalClimax.Save(a_intfc);

        SaveStringIntMap(a_intfc, actions_did, "NPCData.actions_did");
        SaveStringIntMap(a_intfc, actions_got, "NPCData.actions_got");

        // Save lovers (keys are TESNPC base FormIDs in v2)
        std::uint32_t loversCount = static_cast<std::uint32_t>(lovers.size());
        WriteData(a_intfc, loversCount, "NPCData.lovers count");
        for (const auto& [loverID, loverData] : lovers) {
            loverData.Save(a_intfc, loverID);
        }

        // v2: othersCount (non-unique partner encounter tally)
        WriteData(a_intfc, othersCount, "NPCData.othersCount");
    }

    bool NPCData::Load(SKSE::SerializationInterface* a_intfc, std::uint32_t version) {
        SKSE::log::trace("NPCData::Load: reading basic fields (version={})", version);

        if (!ReadData(a_intfc, sameSexEncounter, "NPCData.sameSexEncounter") ||
            !ReadData(a_intfc, soloSex, "NPCData.soloSex") ||
            !ReadData(a_intfc, exclusiveSex, "NPCData.exclusiveSex") ||
            !ReadData(a_intfc, groupSex, "NPCData.groupSex") ||
            !ReadData(a_intfc, lastTime, "NPCData.lastTime")) {
            return false;
        }

        // v1 had 'relationshipsInitialized' + 'existingRelationsScan' bools here; discard both.
        if (version == 1) {
            bool discarded1 = false, discarded2 = false;
            if (!ReadData(a_intfc, discarded1, "NPCData.relationshipsInitialized (v1 legacy skip)") ||
                !ReadData(a_intfc, discarded2, "NPCData.existingRelationsScan (v1 legacy skip)")) {
                SKSE::log::warn("NPCData::Load: failed to skip v1 legacy bool fields");
                return false;
            }
            SKSE::log::trace("NPCData::Load: skipped v1 legacy bools relInit={}, existingScan={}", discarded1, discarded2);
        }

        SKSE::log::trace("NPCData::Load: basic fields ok — sameSex={}, solo={}, excl={}, group={}, lastTime={}",
                         sameSexEncounter, soloSex, exclusiveSex, groupSex, lastTime);

        if (!totalInternalClimax.Load(a_intfc)) {
            SKSE::log::error("NPCData::Load: failed to read totalInternalClimax");
            return false;
        }

        SKSE::log::trace("NPCData::Load: totalInternalClimax ok — did={}, got={}",
                         totalInternalClimax.did, totalInternalClimax.got);

        if (!LoadStringIntMap(a_intfc, actions_did, "NPCData.actions_did")) {
            SKSE::log::error("NPCData::Load: failed to read actions_did");
            return false;
        }
        SKSE::log::trace("NPCData::Load: actions_did ok ({} entries)", actions_did.size());

        if (!LoadStringIntMap(a_intfc, actions_got, "NPCData.actions_got")) {
            SKSE::log::error("NPCData::Load: failed to read actions_got");
            return false;
        }
        SKSE::log::trace("NPCData::Load: actions_got ok ({} entries)", actions_got.size());

        // Load lovers
        std::uint32_t loversCount = 0;
        if (!ReadData(a_intfc, loversCount, "NPCData.lovers count")) {
            SKSE::log::error("NPCData::Load: failed to read lovers count");
            return false;
        }
        SKSE::log::trace("NPCData::Load: loading {} lovers (version={})", loversCount, version);

        lovers.clear();
        for (std::uint32_t i = 0; i < loversCount; ++i) {
            std::uint32_t loverFormID = 0;
            if (!ReadData(a_intfc, loverFormID, "lover formID")) {
                return false;
            }

            if (version >= 2) {
                // v2: keys are TESNPC base FormIDs — resolve via plugin load order remapping
                std::uint32_t resolvedLoverID = 0;
                if (!a_intfc->ResolveFormID(loverFormID, resolvedLoverID)) {
                    SKSE::log::warn("Failed to resolve lover base FormID 0x{:X}, skipping", loverFormID);
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
            } else {
                // v1: keys are raw Actor refFormIDs — store as-is; migration will resolve them later
                LoverData loverData;
                if (!loverData.Load(a_intfc)) {
                    return false;
                }
                lovers[loverFormID] = loverData;
            }
        }

        // v2: read othersCount
        if (version >= 2) {
            if (!ReadData(a_intfc, othersCount, "NPCData.othersCount")) {
                othersCount = 0;  // graceful fallback if field missing
            }
        } else {
            othersCount = 0;
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

        // Lazy migration preamble: if any legacy entry's actor resolves to this base FormID, migrate it now.
        // Handles the "comes back into range" case for v1 → v2 migration.
        if (!_legacyStore.empty()) {
            for (auto legacyIt = _legacyStore.begin(); legacyIt != _legacyStore.end(); ) {
                auto* legacyForm = RE::TESForm::LookupByID(legacyIt->first);
                auto* legacyActor = legacyForm ? legacyForm->As<RE::Actor>() : nullptr;
                if (legacyActor) {
                    auto* legacyBase = LL::GetStableBase(legacyActor);
                    if (legacyBase && legacyBase->GetFormID() == formID) {
                        MigrateOneLegacyEntry(legacyIt->first, legacyIt->second);
                        legacyIt = _legacyStore.erase(legacyIt);
                        continue;
                    }
                }
                ++legacyIt;
            }
        }

        auto& npcData = _store[formID];

        return npcData;
    }

    const NPCData* LoversLedgerService::FindNPC(std::uint32_t formID) const {
        // Caller must hold shared_lock or unique_lock
        auto it = _store.find(formID);
        return (it != _store.end()) ? &it->second : nullptr;
    }

    bool LoversLedgerService::MigrateOneLegacyEntry(std::uint32_t npcRefFormID, NPCData& legacyData) {
        // Caller must hold unique_lock.
        // Returns true  → remove this entry from _legacyStore.
        // Returns false → keep entry (NPC or at least one lover still deferred).

        // --- Classify the NPC itself ---
        auto* npcForm = RE::TESForm::LookupByID(npcRefFormID);
        auto* npcActor = npcForm ? npcForm->As<RE::Actor>() : nullptr;

        if (!npcActor) {
            if (IsTempFormID(npcRefFormID)) {
                // Temp actor (0xFF prefix) — definitely non-unique, silently drop
                SKSE::log::trace("MigrateOneLegacyEntry: 0x{:X} is a temp formID, dropping", npcRefFormID);
                return true;
            }
            // Persistent ref but cell not loaded — defer
            return false;
        }

        auto* npcRootBase = LL::GetStableBase(npcActor);
        if (!npcRootBase) {
            SKSE::log::warn("MigrateOneLegacyEntry: NPC 0x{:X} has no ActorBase, dropping", npcRefFormID);
            return true;
        }

        if (!LL::IsEffectivelyUnique(npcRootBase)) {
            // Non-unique subject (bandit, generic NPC) — no per-NPC stats preserved
            SKSE::log::trace("MigrateOneLegacyEntry: NPC 0x{:X} ({}) is non-unique, dropping",
                             npcRefFormID, npcRootBase->GetName());
            return true;
        }

        const std::uint32_t npcBaseFormID = npcRootBase->GetFormID();

        // --- Migrate NPC-level stats (merge: take the larger value to avoid overwriting newer data) ---
        auto& dest = _store[npcBaseFormID];
        dest.sameSexEncounter     = std::max(dest.sameSexEncounter,     legacyData.sameSexEncounter);
        dest.soloSex              = std::max(dest.soloSex,              legacyData.soloSex);
        dest.exclusiveSex         = std::max(dest.exclusiveSex,         legacyData.exclusiveSex);
        dest.groupSex             = std::max(dest.groupSex,             legacyData.groupSex);
        dest.lastTime             = std::max(dest.lastTime,             legacyData.lastTime);
        dest.totalInternalClimax.did = std::max(dest.totalInternalClimax.did, legacyData.totalInternalClimax.did);
        dest.totalInternalClimax.got = std::max(dest.totalInternalClimax.got, legacyData.totalInternalClimax.got);
        for (auto& [action, count] : legacyData.actions_did) {
            dest.actions_did[action] = std::max(dest.actions_did[action], count);
        }
        for (auto& [action, count] : legacyData.actions_got) {
            dest.actions_got[action] = std::max(dest.actions_got[action], count);
        }
        // --- Classify each lover ---
        int loversDeferred = 0;
        for (auto& [loverRefFormID, loverData] : legacyData.lovers) {
            auto* loverForm = RE::TESForm::LookupByID(loverRefFormID);
            auto* loverActor = loverForm ? loverForm->As<RE::Actor>() : nullptr;

            if (loverActor) {
                auto* loverRootBase = LL::GetStableBase(loverActor);
                if (loverRootBase && LL::IsEffectivelyUnique(loverRootBase)) {
                    // Unique lover — merge into dest.lovers
                    const std::uint32_t loverBaseFormID = loverRootBase->GetFormID();
                    auto& destLover = dest.lovers[loverBaseFormID];
                    destLover.exclusiveSex       = std::max(destLover.exclusiveSex,       loverData.exclusiveSex);
                    destLover.partOfSameGroupSex = std::max(destLover.partOfSameGroupSex,  loverData.partOfSameGroupSex);
                    destLover.lastTime           = std::max(destLover.lastTime,            loverData.lastTime);
                    destLover.orgasms            = std::max(destLover.orgasms,             loverData.orgasms);
                    destLover.internalClimax.did = std::max(destLover.internalClimax.did,  loverData.internalClimax.did);
                    destLover.internalClimax.got = std::max(destLover.internalClimax.got,  loverData.internalClimax.got);
                } else {
                    // Non-unique lover — fold into othersCount
                    dest.othersCount += loverData.exclusiveSex + loverData.partOfSameGroupSex;
                }
            } else if (IsTempFormID(loverRefFormID)) {
                // Temp lover — definitely non-unique, fold into othersCount
                dest.othersCount += loverData.exclusiveSex + loverData.partOfSameGroupSex;
            } else {
                // Persistent ref but cell not loaded — move to dest.legacyLovers for lazy drain
                dest.legacyLovers[loverRefFormID] = loverData;
                ++loversDeferred;
            }
        }

        // NPC is always considered migrated once it resolves as unique;
        // any unresolved lovers now live in dest.legacyLovers and drain lazily.
        SKSE::log::trace("MigrateOneLegacyEntry: NPC 0x{:X} → base 0x{:X} migrated ({} lover(s) deferred to legacyLovers)",
                         npcRefFormID, npcBaseFormID, loversDeferred);
        return true;
    }

    void LoversLedgerService::TryMigrateLegacyStore() {
        std::unique_lock lock(_mutex);

        if (_legacyStore.empty()) {
            return;
        }

        SKSE::log::info("TryMigrateLegacyStore: {} legacy entries to process", _legacyStore.size());

        int migrated = 0;
        int dropped  = 0;
        int deferred = 0;

        for (auto it = _legacyStore.begin(); it != _legacyStore.end(); ) {
            bool remove = MigrateOneLegacyEntry(it->first, it->second);
            if (remove) {
                // Determine whether it was migrated or just dropped (temp/non-unique)
                auto* f = RE::TESForm::LookupByID(it->first);
                auto* a = f ? f->As<RE::Actor>() : nullptr;
                auto* _base = LL::GetStableBase(a);
                bool wasUnique = _base && LL::IsEffectivelyUnique(_base);
                if (wasUnique) ++migrated; else ++dropped;
                it = _legacyStore.erase(it);
            } else {
                ++deferred;
                ++it;
            }
        }

        SKSE::log::info("TryMigrateLegacyStore: migrated={}, dropped={}, deferred={}, remaining legacy={}",
                        migrated, dropped, deferred, _legacyStore.size());

        // Second pass: drain legacyLovers in already-migrated store entries
        int legacyLoversDrained  = 0;
        int legacyLoversDeferred = 0;
        for (auto& [npcBaseFormID, npcData] : _store) {
            if (npcData.legacyLovers.empty()) continue;

            for (auto loverIt = npcData.legacyLovers.begin(); loverIt != npcData.legacyLovers.end(); ) {
                const std::uint32_t loverRefFormID = loverIt->first;
                const LoverData& loverData = loverIt->second;

                auto* loverForm = RE::TESForm::LookupByID(loverRefFormID);
                auto* loverActor = loverForm ? loverForm->As<RE::Actor>() : nullptr;

                if (loverActor) {
                    auto* loverRootBase = LL::GetStableBase(loverActor);
                    if (loverRootBase && LL::IsEffectivelyUnique(loverRootBase)) {
                        const std::uint32_t loverBaseFormID = loverRootBase->GetFormID();
                        auto& destLover = npcData.lovers[loverBaseFormID];
                        destLover.exclusiveSex       = std::max(destLover.exclusiveSex,       loverData.exclusiveSex);
                        destLover.partOfSameGroupSex = std::max(destLover.partOfSameGroupSex,  loverData.partOfSameGroupSex);
                        destLover.lastTime           = std::max(destLover.lastTime,            loverData.lastTime);
                        destLover.orgasms            = std::max(destLover.orgasms,             loverData.orgasms);
                        destLover.internalClimax.did = std::max(destLover.internalClimax.did,  loverData.internalClimax.did);
                        destLover.internalClimax.got = std::max(destLover.internalClimax.got,  loverData.internalClimax.got);
                    } else {
                        npcData.othersCount += loverData.exclusiveSex + loverData.partOfSameGroupSex;
                    }
                    ++legacyLoversDrained;
                    loverIt = npcData.legacyLovers.erase(loverIt);
                } else if (IsTempFormID(loverRefFormID)) {
                    npcData.othersCount += loverData.exclusiveSex + loverData.partOfSameGroupSex;
                    ++legacyLoversDrained;
                    loverIt = npcData.legacyLovers.erase(loverIt);
                } else {
                    ++legacyLoversDeferred;
                    ++loverIt;
                }
            }
        }

        if (legacyLoversDrained > 0 || legacyLoversDeferred > 0) {
            SKSE::log::info("TryMigrateLegacyStore: legacy lovers drained={}, still deferred={}",
                            legacyLoversDrained, legacyLoversDeferred);
        }
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
                {"groupsex", [](const NPCData& n) { return n.groupSex; }},
                {"otherscount", [](const NPCData& n) { return n.othersCount; }}};

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

        bool needsRecalc = false;

        // Single property names
        if (parts.size() == 1) {
            static const std::unordered_map<std::string, std::function<void(LoverData&, int)>> intSetters = {
                {"exclusivesex", [](LoverData& l, int v) { l.exclusiveSex = v; }},
                {"partofsamegroupsex", [](LoverData& l, int v) { l.partOfSameGroupSex = v; }}};

            auto it = intSetters.find(parts[0]);
            if (it != intSetters.end()) {
                it->second(lover, value);
                needsRecalc = true;
            }
        }

        // Nested properties
        if (parts.size() == 2 && parts[0] == "internalclimax") {
            if (parts[1] == "did") {
                lover.internalClimax.did = value;
                needsRecalc = true;
            } else if (parts[1] == "got") {
                lover.internalClimax.got = value;
                needsRecalc = true;
            }
        }

        // Recalculate NPC totals from all lovers
        if (needsRecalc) {
            npc.exclusiveSex = 0;
            npc.groupSex = 0;
            npc.totalInternalClimax.did = 0;
            npc.totalInternalClimax.got = 0;
            npc.lastTime = 0.0f;

            for (const auto& [loverID, loverData] : npc.lovers) {
                npc.exclusiveSex += loverData.exclusiveSex;
                npc.groupSex += loverData.partOfSameGroupSex;
                npc.totalInternalClimax.did += loverData.internalClimax.did;
                npc.totalInternalClimax.got += loverData.internalClimax.got;
                npc.lastTime = std::max(npc.lastTime, loverData.lastTime);
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

                // Update NPC's lastTime if this lover's lastTime changed
                if (parts[0] == "lasttime") {
                    npc.lastTime = 0.0f;
                    for (const auto& [loverID, loverData] : npc.lovers) {
                        npc.lastTime = std::max(npc.lastTime, loverData.lastTime);
                    }
                }
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
        _legacyStore.clear();
        _globalRelationsScanDone = false;
        SKSE::log::info("LoversLedgerService: All data cleared");
    }

    void LoversLedgerService::UpdateOthers(std::uint32_t npcBaseFormID) {
        if (npcBaseFormID == 0) return;
        std::unique_lock lock(_mutex);
        auto& npc = EnsureNPC(npcBaseFormID);
        ++npc.othersCount;
        SKSE::log::trace("UpdateOthers: npc base=0x{:X}, othersCount now {}", npcBaseFormID, npc.othersCount);
    }

    std::pair<int, int> LoversLedgerService::CleanupInvalidFormIDs() {
        std::unique_lock lock(_mutex);

        int npcsRemoved = 0;
        int loversRemoved = 0;

        SKSE::log::info("CleanupInvalidFormIDs: Starting cleanup...");

        // Helper: validate a base FormID as a unique TESNPC (base forms are always resident,
        // so a null lookup or non-unique result means the entry is corrupt/stale)
        auto isValidUniqueBase = [](std::uint32_t baseFormID) -> bool {
            auto* form = RE::TESForm::LookupByID(baseFormID);
            if (!form) return false;
            auto* npc = form->As<RE::TESNPC>();
            return npc && npc->IsUnique();
        };

        // Pass 1: remove invalid NPC entries from _store
        std::vector<std::uint32_t> npcsToRemove;
        for (const auto& [npcFormID, npcData] : _store) {
            if (!isValidUniqueBase(npcFormID)) {
                npcsToRemove.push_back(npcFormID);
            }
        }
        for (auto npcFormID : npcsToRemove) {
            _store.erase(npcFormID);
            ++npcsRemoved;
            SKSE::log::debug("  Removed invalid NPC base FormID 0x{:X}", npcFormID);
        }

        // Pass 2: remove invalid lover entries from remaining _store NPCs
        for (auto& [npcFormID, npcData] : _store) {
            std::vector<std::uint32_t> loversToRemove;
            for (const auto& [loverFormID, loverData] : npcData.lovers) {
                if (!isValidUniqueBase(loverFormID)) {
                    loversToRemove.push_back(loverFormID);
                }
            }
            for (auto loverFormID : loversToRemove) {
                npcData.lovers.erase(loverFormID);
                ++loversRemoved;
                SKSE::log::debug("  Removed invalid lover base FormID 0x{:X} from NPC 0x{:X}", loverFormID, npcFormID);
            }
        }

        // Pass 3: purge temp (0xFF) entries from _legacyStore — these can never be migrated
        std::vector<std::uint32_t> legacyToRemove;
        for (const auto& [refFormID, legacyData] : _legacyStore) {
            if (IsTempFormID(refFormID)) {
                legacyToRemove.push_back(refFormID);
            }
        }
        for (auto refFormID : legacyToRemove) {
            _legacyStore.erase(refFormID);
            SKSE::log::debug("  Purged temp legacy NPC ref FormID 0x{:X}", refFormID);
        }

        SKSE::log::info("CleanupInvalidFormIDs: Cleanup complete - {} NPCs removed, {} lovers removed, {} temp legacy entries purged",
                       npcsRemoved, loversRemoved, static_cast<int>(legacyToRemove.size()));

        return {npcsRemoved, loversRemoved};
    }

    void LoversLedgerService::ScanAllGameRelationships() {
        std::unique_lock lock(_mutex);

        if (_globalRelationsScanDone) {
            SKSE::log::info("ScanAllGameRelationships: Already completed for this playthrough, skipping.");
            return;
        }

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            SKSE::log::error("ScanAllGameRelationships: TESDataHandler not available.");
            return;
        }

        constexpr std::uint32_t kAssocSpouse   = 0x000142CA;
        constexpr std::uint32_t kAssocCourting = 0x0001EE23;

        const auto& relForms = dataHandler->GetFormArray<RE::BGSRelationship>();
        SKSE::log::info("ScanAllGameRelationships: Examining {} BGSRelationship forms...", relForms.size());

        int examined = 0;
        int seeded   = 0;
        int skipped  = 0;

        for (auto* relForm : relForms) {
            if (!relForm) continue;
            ++examined;

            RE::TESNPC* npc1 = relForm->npc1;
            RE::TESNPC* npc2 = relForm->npc2;
            if (!npc1 || !npc2) continue;
            if (!npc1->IsUnique() || !npc2->IsUnique()) continue;

            const std::uint32_t id1 = npc1->GetFormID();
            const std::uint32_t id2 = npc2->GetFormID();
            if (id1 == 0 || id2 == 0 || id1 == id2) continue;

            // Classify the relationship
            bool isSpouse   = false;
            bool isCourting = false;
            bool isLover    = false;

            if (relForm->assocType) {
                const std::uint32_t assocID = relForm->assocType->GetFormID();
                if (assocID == kAssocSpouse)   isSpouse   = true;
                else if (assocID == kAssocCourting) isCourting = true;
            }

            if (!isSpouse && !isCourting &&
                relForm->level == RE::BGSRelationship::RELATIONSHIP_LEVEL::kLover) {
                isLover = true;
            }

            if (!isSpouse && !isCourting && !isLover) continue;

            // Skip if the pair is already in the ledger (either direction already seeded)
            auto it1 = _store.find(id1);
            if (it1 != _store.end() && it1->second.lovers.count(id2) > 0) {
                ++skipped;
                continue;
            }
            auto it2 = _store.find(id2);
            if (it2 != _store.end() && it2->second.lovers.count(id1) > 0) {
                ++skipped;
                continue;
            }

            // Ensure npc1 is in the store — CreateExistingLoverInternal requires it.
            // (CreateExistingLoverInternal handles creating npc2's entry itself.)
            if (_store.find(id1) == _store.end()) {
                _store[id1] = NPCData{};
            }

            // Seed both sides (CreateExistingLoverInternal handles the reciprocal entry)
            CreateExistingLoverInternal(id1, id2, isSpouse, isCourting, isLover);
            ++seeded;
        }

        _globalRelationsScanDone = true;
        SKSE::log::info(
            "ScanAllGameRelationships: Done — examined={}, seeded={} pair(s), skipped={} (already in ledger).",
            examined, seeded, skipped);
    }

    void LoversLedgerService::CreateExistingLoverInternal(std::uint32_t npcFormID, std::uint32_t loverFormID,
                                                          bool isSpouse, bool isCourting, bool isLover) {
        // IMPORTANT: Called from ScanAllGameRelationships which holds a unique_lock on _mutex.
        // Do NOT acquire the lock here.

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
        npcIt->second.lastTime = lastTime;
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
                    partnerIt->second.lastTime = lastTime;
                    partnerIt->second.totalInternalClimax.did += gotInternal;
                    partnerIt->second.totalInternalClimax.got += didInternal;

                    SKSE::log::debug(
                        "  Created reciprocal relationship for partner 0x{:X} -> NPC 0x{:X}, sexTimes:{}, "
                        "didInternal:{}, gotInternal:{}, loverLastTime:{}",
                        loverFormID, npcFormID, sexTimes, gotInternal, didInternal, lastTime);
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

        // v2: persist the global relationship scan flag so the scan does not
        // repeat on subsequent loads of the same playthrough.
        if (!a_intfc->WriteRecordData(&_globalRelationsScanDone, sizeof(_globalRelationsScanDone))) {
            SKSE::log::error("Failed to write _globalRelationsScanDone");
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

            if (version != 1 && version != 2) {
                SKSE::log::error("Unsupported serialization version: {}", version);
                continue;
            }

            // v2+: read the global relationship scan flag
            bool globalScanDone = false;
            if (version >= 2) {
                std::uint32_t readBytes2 = a_intfc->ReadRecordData(&globalScanDone, sizeof(globalScanDone));
                if (readBytes2 != sizeof(globalScanDone)) {
                    SKSE::log::warn("Failed to read _globalRelationsScanDone, defaulting to false");
                    globalScanDone = false;
                }
            }

            std::uint32_t numNPCs = 0;
            std::uint32_t readBytes = a_intfc->ReadRecordData(&numNPCs, sizeof(numNPCs));
            if (readBytes != sizeof(numNPCs)) {
                SKSE::log::error("Failed to read NPC count");
                continue;
            }

            SKSE::log::info("Loading {} NPC records (format version {})", numNPCs, version);

            {
                std::unique_lock lock(_mutex);
                _store.clear();
                _legacyStore.clear();
                _globalRelationsScanDone = globalScanDone;
                SKSE::log::info("Loaded _globalRelationsScanDone={}", _globalRelationsScanDone);

                for (std::uint32_t i = 0; i < numNPCs; ++i) {
                    std::uint32_t npcFormID = 0;
                    readBytes = a_intfc->ReadRecordData(&npcFormID, sizeof(npcFormID));
                    if (readBytes != sizeof(npcFormID)) {
                        SKSE::log::error("Failed to read NPC formID");
                        continue;
                    }

                    if (version >= 2) {
                        // v2: NPC key is a TESNPC base FormID — resolve via plugin load order remapping
                        std::uint32_t resolvedNPCID = 0;
                        if (!a_intfc->ResolveFormID(npcFormID, resolvedNPCID)) {
                            SKSE::log::warn("Failed to resolve NPC base FormID 0x{:X}, skipping", npcFormID);
                            NPCData tempData;
                            if (!tempData.Load(a_intfc, version)) {
                                SKSE::log::error("Failed to skip invalid NPC data for 0x{:X}", npcFormID);
                            }
                            continue;
                        }
                        NPCData npcData;
                        if (!npcData.Load(a_intfc, version)) {
                            SKSE::log::error("Failed to load NPC data for base FormID 0x{:X}", resolvedNPCID);
                            continue;
                        }
                        _store[resolvedNPCID] = std::move(npcData);
                    } else {
                        // v1: NPC key is a raw Actor refFormID — store as-is in legacyStore for later migration
                        NPCData npcData;
                        if (!npcData.Load(a_intfc, version)) {
                            SKSE::log::error("Failed to load legacy NPC data for formID 0x{:X}", npcFormID);
                            continue;
                        }
                        _legacyStore[npcFormID] = std::move(npcData);
                    }
                }

                SKSE::log::info("LoversLedgerService: Load complete - {} NPCs in store, {} in legacy store",
                                _store.size(), _legacyStore.size());
            }  // lock released here
        }
    }

    void LoversLedgerService::RevertCallback([[maybe_unused]] SKSE::SerializationInterface* a_intfc) {
        SKSE::log::info("LoversLedgerService: Reverting data (new game)...");
        ClearAll();
    }

    NPCData* LoversLedgerService::GetNPCData(std::uint32_t npcFormID) {
        std::shared_lock lock(_mutex);
        auto it = _store.find(npcFormID);
        if (it == _store.end()) {
            return nullptr;
        }
        return &it->second;
    }

    const NPCData* LoversLedgerService::GetNPCData(std::uint32_t npcFormID) const {
        std::shared_lock lock(_mutex);
        auto it = _store.find(npcFormID);
        if (it == _store.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void LoversLedgerService::DeleteNPCData(std::uint32_t npcFormID) {
        std::unique_lock lock(_mutex);
        auto it = _store.find(npcFormID);
        if (it != _store.end()) {
            SKSE::log::info("DeleteNPCData: Deleting NPC 0x{:X}", npcFormID);
            _store.erase(it);
        } else {
            SKSE::log::warn("DeleteNPCData: NPC 0x{:X} not found", npcFormID);
        }
    }

}  // namespace LL
