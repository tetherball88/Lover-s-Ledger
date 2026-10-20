#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "PCH.h"

namespace LL {

    // Serialization constants
    constexpr std::uint32_t kSerializationVersion = 2;
    constexpr std::uint32_t kSerializationType = 'LLGR';  // LoversLedger
    // Global scene counters live in their own record so older DLLs skip them cleanly
    constexpr std::uint32_t kSceneCountsVersion = 1;
    constexpr std::uint32_t kSceneCountsType = 'LLSC';  // LoversLedger Scene Counts
    // SexLab history backfill (checked NPCs, past lovers) in its own record, same reason
    constexpr std::uint32_t kSexLabHistoryVersion = 1;
    constexpr std::uint32_t kSexLabHistoryType = 'LLPL';  // LoversLedger Past Lovers
    // Partner traits for first-time lines, own record for the same reason
    constexpr std::uint32_t kPartnerTraitsVersion = 1;
    constexpr std::uint32_t kPartnerTraitsType = 'LLPT';  // LoversLedger Partner Traits

    // Counts SexLab holds that the ledger does not, copied once per NPC
    struct SexLabHistory {
        int encounters{0};
        int solo{0};
        int vaginal{0};
        int anal{0};
        int oral{0};
    };

    // Stand-in partner for backfilled history with no known partner; never a real form
    struct PastLover {
        std::uint32_t raceFormID{0};
        std::int8_t gender{0};  // 0 = male, 1 = female
        int encounters{0};
    };

    // Partner genders for PartnerTraits::genders
    constexpr std::uint8_t kPartnerMale = 1;
    constexpr std::uint8_t kPartnerFemale = 2;
    constexpr std::uint8_t kPartnerFuta = 4;
    // SexLab anatomy bits (gender faction override, P+ futa); -1 = unknown, use the base sex
    constexpr int kAnatomyPenis = 1;
    constexpr int kAnatomyVagina = 2;

    // Anatomy from the base sex when SexLab gave none
    int AnatomyOrBaseSex(int anatomy, bool baseMale);
    // Gender bit for a partner of that anatomy; 0 for creatures (race without ActorTypeNPC)
    std::uint8_t PartnerGenderBit(RE::TESRace* race, int anatomy);

    // A scene participant for RecordPartners
    struct PartnerRef {
        std::uint32_t refFormID{0};
        int anatomy{-1};
    };
    // PartnerTraits::maxGroupSize when an old save had group sex of unknown size
    constexpr std::uint8_t kGroupSizeUnknown = 255;

    // Who an NPC has had partnered sex with; seeded once from older ledger data, then kept per scene
    struct PartnerTraits {
        std::uint8_t genders{0};  // kPartnerMale | kPartnerFemale | kPartnerFuta, NPC-type partners only
        std::unordered_set<std::uint32_t> races;
        std::uint8_t maxGroupSize{0};  // most actors in one partnered scene
        bool unknownPartners{false};   // some partners' race / gender were never recorded
    };

    // What DescribeFirstTimes needs from the ledger for one NPC
    struct FirstTimeHistory {
        int partnered{0};  // exclusive + group encounters
        PartnerTraits traits;
        int vaginalDid{0};
        int vaginalGot{0};
        int analDid{0};
        int analGot{0};
    };

    // Directions for ActKey::dir
    constexpr std::uint8_t kActDirAny = 0;
    constexpr std::uint8_t kActDirGave = 1;
    constexpr std::uint8_t kActDirReceived = 2;

    // Global scene counter key; each field has an "any" value so wildcard queries are a single lookup
    struct ActKey {
        std::uint32_t raceFormID{0};  // 0 = any race
        std::int8_t gender{-1};       // -1 = any, 0 = male, 1 = female
        std::string type;             // "" = any act, lowercased
        std::uint8_t dir{kActDirAny};

        bool operator==(const ActKey&) const = default;
        auto operator<=>(const ActKey&) const = default;
    };

    struct ActKeyHash {
        std::size_t operator()(const ActKey& k) const noexcept {
            std::size_t h = std::hash<std::string>{}(k.type);
            h ^= std::hash<std::uint64_t>{}((std::uint64_t(k.raceFormID) << 16) |
                                            (std::uint64_t(std::uint8_t(k.gender)) << 8) | k.dir) +
                 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            return h;
        }
    };

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

        // lovers keyed by TESNPC base FormID (v2) or Actor refFormID (v1 legacy)
        std::unordered_map<std::uint32_t, LoverData> lovers;

        // v1 migration staging: keyed by Actor refFormID; drained lazily as lover cells load.
        // Never serialized. Populated during MigrateOneLegacyEntry for persistent-ref lovers
        // whose cells were not loaded at migration time.
        std::unordered_map<std::uint32_t, LoverData> legacyLovers;

        // Count of encounters with non-unique partners (bandits, generic NPCs, etc.)
        int othersCount{0};

        // Serialization
        void Save(SKSE::SerializationInterface* a_intfc, std::uint32_t npcFormID) const;
        bool Load(SKSE::SerializationInterface* a_intfc, std::uint32_t version);
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

        // Increment othersCount for a unique NPC (non-unique partner encounter)
        void UpdateOthers(std::uint32_t npcBaseFormID);

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

        // Global scene counters (all participants, unique or not)
        // keys: per-scene deduplicated set, already wildcard-expanded; each key is bumped once
        void RecordScene(const std::vector<ActKey>& keys, int actorCount);
        // Scenes where a participant matched race/gender/type/dir (0 / -1 / "" / kActDirAny = any)
        int HowManyTimes(std::uint32_t raceFormID, int gender, std::string_view type, std::uint8_t dir) const;
        // Scenes with exactly numActors participants; <= 0 returns all recorded scenes
        int HowManyTimesWith(int numActors) const;

        // Load action-name synonym clusters from Data/SKSE/Plugins/LoversLedger/synonyms.json.
        // Each cluster's first member is its stored word; actions are recorded and queried under it.
        void LoadSynonyms();
        // Lowercased name mapped to its cluster word (unchanged if in no cluster). Map is fixed after data load.
        std::string Canonical(std::string_view name) const;

        // SKSE Serialization
        void SaveCallback(SKSE::SerializationInterface* a_intfc);
        void LoadCallback(SKSE::SerializationInterface* a_intfc);
        void RevertCallback(SKSE::SerializationInterface* a_intfc);

        // Utility: clear storage
        void ClearAll();

        // Direct data access for UI (returns pointer, nullptr if not found)
        NPCData* GetNPCData(std::uint32_t npcFormID);
        const NPCData* GetNPCData(std::uint32_t npcFormID) const;

        // Delete NPC data entirely
        void DeleteNPCData(std::uint32_t npcFormID);

        // Cleanup: remove invalid FormIDs (NPCs/lovers that no longer exist)
        // Returns the number of NPCs and lovers removed
        std::pair<int, int> CleanupInvalidFormIDs();

        // Iterate _legacyStore, migrate what can be migrated, log summary.
        void TryMigrateLegacyStore();

        // Scan all BGSRelationship forms in the loaded game and seed history for
        // spouse/courting/lover pairs not yet in the ledger. Runs at most once per
        // playthrough (flag persisted in the SKSE save). Thread-safe.
        void ScanAllGameRelationships();

        // SexLab backfill: true the first time an NPC is claimed (and marks it), false after
        bool ClaimSexLabCheck(std::uint32_t npcFormID);
        // Adds the history to the NPC's game relationship partner (spouse > lover > courting).
        // Returns false and changes nothing when the NPC has no such partner.
        bool AddHistoryToPartner(std::uint32_t npcFormID, const SexLabHistory& history);
        // Adds the history to the NPC's past lover of that race and gender
        void AddPastLover(std::uint32_t npcFormID, std::uint32_t raceFormID, int gender, const SexLabHistory& history);
        std::vector<PastLover> GetPastLovers(std::uint32_t npcFormID) const;

        // First-time lines: adds the scene's other participants to the NPC's partner traits.
        // Call before the scene is applied to the ledger, so the one-time seed sees only older data.
        void RecordPartners(std::uint32_t npcFormID, const std::vector<PartnerRef>& partners, int actorCount);
        // Ledger history the first-time lines compare a scene with (seeds the traits on first use)
        FirstTimeHistory GetFirstTimeHistory(std::uint32_t npcFormID) const;

    private:
        LoversLedgerService() = default;
        ~LoversLedgerService() = default;
        LoversLedgerService(const LoversLedgerService&) = delete;
        LoversLedgerService& operator=(const LoversLedgerService&) = delete;

        NPCData& EnsureNPC(std::uint32_t formID);
        const NPCData* FindNPC(std::uint32_t formID) const;
        // Caller must hold shared_lock or unique_lock
        static float ScoreLover(const LoverData& lover);

        // Scene counter record; SaveSceneCounts expects the caller to hold the lock
        void SaveSceneCounts(SKSE::SerializationInterface* a_intfc) const;
        void LoadSceneCounts(SKSE::SerializationInterface* a_intfc, std::uint32_t version);

        // SexLab history record; SaveSexLabHistory expects the caller to hold the lock
        void SaveSexLabHistory(SKSE::SerializationInterface* a_intfc) const;
        void LoadSexLabHistory(SKSE::SerializationInterface* a_intfc, std::uint32_t version);
        // Caller must hold unique_lock. NPC totals and did/got actions for the history.
        void AddHistoryToNPC(NPCData& npc, bool npcMale, bool partnerMale, const SexLabHistory& history);

        // Partner traits record; SavePartnerTraits expects the caller to hold the lock
        void SavePartnerTraits(SKSE::SerializationInterface* a_intfc) const;
        void LoadPartnerTraits(SKSE::SerializationInterface* a_intfc, std::uint32_t version);
        // Caller must hold a lock. Traits derived from lovers / past lovers / totals (older data).
        PartnerTraits SeedTraits(std::uint32_t npcFormID) const;
        // Caller must hold unique_lock. The NPC's traits, seeded on first use.
        PartnerTraits& EnsureTraits(std::uint32_t npcFormID);
        // Caller must hold unique_lock. Adds a known partner to traits that already exist (backfill paths).
        void AddTraitIfSeeded(std::uint32_t npcFormID, std::uint32_t raceFormID, bool male, int groupSize);

        // v1 → v2 migration helpers
        static bool IsTempFormID(std::uint32_t id) noexcept { return (id >> 24) == 0xFF; }
        // Classify and migrate one legacy entry. Mutates legacyData.lovers to remove resolved lovers.
        // Returns true if the legacy entry should be removed (NPC migrated + all lovers handled, or NPC dropped).
        // Returns false if the NPC or any lovers are still deferred (keep entry in _legacyStore).
        bool MigrateOneLegacyEntry(std::uint32_t npcRefFormID, NPCData& legacyData);
        void CreateExistingLoverInternal(std::uint32_t npcFormID, std::uint32_t loverFormID, bool isSpouse,
                                         bool isCourting, bool isLover);

        // v2 store: keyed by TESNPC base FormID
        std::unordered_map<std::uint32_t, NPCData> _store;

        // v1 migration staging: keyed by Actor refFormID; never serialized
        // Drained lazily as NPCs come back into range
        std::unordered_map<std::uint32_t, NPCData> _legacyStore;

        // Set to true once ScanAllGameRelationships completes; persisted in the SKSE save
        // so the scan does not repeat on subsequent loads of the same playthrough.
        bool _globalRelationsScanDone{false};

        // Global scene counters; saved in the kSceneCountsType record
        std::unordered_map<ActKey, int, ActKeyHash> _actCounts;
        std::unordered_map<int, int> _actorCountScenes;  // key 0 = all scenes

        // SexLab backfill state; saved in the kSexLabHistoryType record
        std::unordered_set<std::uint32_t> _sexlabChecked;
        std::unordered_map<std::uint32_t, std::vector<PastLover>> _pastLovers;

        // First-time partner traits; saved in the kPartnerTraitsType record
        std::unordered_map<std::uint32_t, PartnerTraits> _partnerTraits;

        // Action-name synonyms: member -> cluster word; loaded once at data load
        std::unordered_map<std::string, std::string> _canonical;
        void CanonicalizeActions(NPCData& npc) const;

        mutable std::shared_mutex _mutex;  // allow concurrent reads
    };

}  // namespace LL
