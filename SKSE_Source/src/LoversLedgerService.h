#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "PCH.h"
#include "RelationsFinderAPI.h"

namespace LL {

    // Serialization constants
    constexpr std::uint32_t kSerializationVersion = 1;
    constexpr std::uint32_t kSerializationType = 'LLGR';  // LoversLedger

    struct InternalClimax {
        int did{0};
        int got{0};

        // Serialization
        void Save(SKSE::SerializationInterface* a_intfc) const;
        bool Load(SKSE::SerializationInterface* a_intfc);
    };

    struct LoverData {
        int exclusiveSex{0};
        int partOfSameGroupSex{0};
        float lastTime{0.0f};
        float orgasms{0.0f};
        InternalClimax internalClimax;

        // Serialization
        void Save(SKSE::SerializationInterface* a_intfc, std::uint32_t loverFormID) const;
        bool Load(SKSE::SerializationInterface* a_intfc);
    };

    struct NPCData {
        int sameSexEncounter{0};
        int soloSex{0};
        int exclusiveSex{0};
        int groupSex{0};
        float lastTime{0.0f};
        InternalClimax totalInternalClimax;

        // actions: did / got -> map actionName -> count
        std::unordered_map<std::string, int> actions_did;
        std::unordered_map<std::string, int> actions_got;

        // lovers keyed by formID
        std::unordered_map<std::uint32_t, LoverData> lovers;

        // Track if relationships have been initialized
        bool relationshipsInitialized{false};

        // Track if existing relationships have been scanned from Relations Finder
        bool existingRelationsScan{false};

        // Serialization
        void Save(SKSE::SerializationInterface* a_intfc, std::uint32_t npcFormID) const;
        bool Load(SKSE::SerializationInterface* a_intfc);
    };

    // A singleton service storing NPC/lover stats with SKSE serialization support
    class LoversLedgerService {
    public:
        static LoversLedgerService& GetSingleton() noexcept;

        // Update functions (accept Actor FormIDs to avoid RE::Actor dependency in service)
        void UpdateActions(std::uint32_t npcFormID, const std::vector<std::string>& actions, std::string_view didGot);
        void IncrementInt(std::uint32_t npcFormID, std::string_view intName);
        void UpdateLover(std::uint32_t npcFormID, std::uint32_t loverFormID, std::string_view encounterType,
                         float orgasms);
        void UpdateLastTime(std::uint32_t npcFormID, std::uint32_t loverFormID = 0);
        void UpdateInternalClimax(std::uint32_t npcFormID, std::uint32_t loverFormID, std::string_view didGot);

        // Generic getters with dot-separated property paths
        int GetNpcInt(std::uint32_t npcFormID, std::string_view propName) const;
        float GetNpcFloat(std::uint32_t npcFormID, std::string_view propName) const;

        int GetLoverInt(std::uint32_t npcFormID, std::uint32_t loverFormID, std::string_view propName) const;
        float GetLoverFloat(std::uint32_t npcFormID, std::uint32_t loverFormID, std::string_view propName) const;

        // Generic setters with dot-separated property paths
        void SetNpcInt(std::uint32_t npcFormID, std::string_view propName, int value);
        void SetNpcFloat(std::uint32_t npcFormID, std::string_view propName, float value);

        void SetLoverInt(std::uint32_t npcFormID, std::uint32_t loverFormID, std::string_view propName, int value);
        void SetLoverFloat(std::uint32_t npcFormID, std::uint32_t loverFormID, std::string_view propName, float value);

        // Get all registered NPC form IDs
        std::vector<std::uint32_t> GetAllNPCs() const;

        // Get all lover form IDs for a specific NPC, sorted by score (highest first)
        // topK: -1 returns all lovers, >0 returns only top K lovers by score
        std::vector<std::uint32_t> GetAllLovers(std::uint32_t npcFormID, int topK = -1) const;

        // Get sorted actions (by count, descending)
        // isDid: true for actions performed (did), false for actions received (got)
        // topK: -1 returns all, >0 returns only top K actions
        std::vector<std::string> GetAllActions(std::uint32_t npcFormID, bool isDid, int topK = -1) const;

        // Get count for a specific action
        // isDid: true for actions performed (did), false for actions received (got)
        // actionName: the name of the action to query
        // Returns: the count for the action, or 0 if not found
        int GetActionCount(std::uint32_t npcFormID, bool isDid, std::string_view actionName) const;

        // Calculate lover score based on encounters, orgasms, and time decay
        // Returns: calculated score, or 0.0f if NPC or lover not found
        float GetLoverScore(std::uint32_t npcFormID, std::uint32_t loverFormID) const;

        // Set the RelationsFinder API function pointer
        void SetRelationsFinderAPI(RelationsFinderAPI::GetNpcRelationshipsCallbackFn apiFunc) noexcept;

        // SKSE Serialization
        void SaveCallback(SKSE::SerializationInterface* a_intfc);
        void LoadCallback(SKSE::SerializationInterface* a_intfc);
        void RevertCallback(SKSE::SerializationInterface* a_intfc);

        // Utility: clear storage
        void ClearAll();

    private:
        LoversLedgerService() = default;
        ~LoversLedgerService() = default;
        LoversLedgerService(const LoversLedgerService&) = delete;
        LoversLedgerService& operator=(const LoversLedgerService&) = delete;

        NPCData& EnsureNPC(std::uint32_t formID);
        const NPCData* FindNPC(std::uint32_t formID) const;
        void ScanExistingRelationships(std::uint32_t npcFormID, NPCData& npcData);
        void CreateExistingLoverInternal(std::uint32_t npcFormID, std::uint32_t loverFormID, bool isSpouse,
                                         bool isCourting, bool isLover);

        std::unordered_map<std::uint32_t, NPCData> _store;
        mutable std::shared_mutex _mutex;  // allow concurrent reads
        RelationsFinderAPI::GetNpcRelationshipsCallbackFn _relationsFinderAPI{nullptr};
    };

}  // namespace LL
