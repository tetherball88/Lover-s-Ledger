#include <algorithm>
#include <chrono>
#include <set>

#include "LoversLedgerService.h"
#include "ThreadsCollector.h"
#include "UniqueOverrides.h"

namespace LL {

    namespace {
        // Constants
        constexpr const char* SOLO_SEX = "soloSex";
        constexpr const char* EXCLUSIVE_SEX = "exclusiveSex";
        constexpr const char* GROUP_SEX = "groupSex";
        constexpr const char* PART_OF_SAME_GROUP_SEX = "partOfSameGroupSex";
        constexpr const char* SAME_SEX_ENCOUNTER = "sameSexEncounter";
        constexpr const char* DID_ACTION = "did";
        constexpr const char* GOT_ACTION = "got";

        // Helper: Get current timestamp
        inline double GetCurrentTimestamp() {
            return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
        }

        // Helper: Determine encounter type from actor count
        inline std::string_view GetEncounterTypeFromCount(size_t actorCount) {
            if (actorCount == 1) return SOLO_SEX;
            if (actorCount == 2) return EXCLUSIVE_SEX;
            return GROUP_SEX;
        }

        // Helper: Normalize string to lowercase
        inline std::string ToLower(std::string_view str) {
            std::string result(str);
            std::transform(result.begin(), result.end(), result.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            return result;
        }

        // Helper: Parse dot-separated property path (case-insensitive)
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

    ThreadsCollector& ThreadsCollector::GetSingleton() noexcept {
        static ThreadsCollector instance;
        return instance;
    }

    // ========================================================================================
    // Thread Management
    // ========================================================================================

    ThreadData& ThreadsCollector::GetOrCreateThread(std::int32_t threadID) {
        std::unique_lock lock(_mutex);
        return _threads[threadID];
    }

    const ThreadData* ThreadsCollector::FindThread(std::int32_t threadID) const {
        std::shared_lock lock(_mutex);
        auto it = _threads.find(threadID);
        return (it != _threads.end()) ? &it->second : nullptr;
    }

    void ThreadsCollector::CleanThread(std::int32_t threadID) {
        std::unique_lock lock(_mutex);
        if (_threads.erase(threadID) > 0) {
            SKSE::log::trace("ThreadsCollector: Cleaned thread {}", threadID);
        }
    }

    void ThreadsCollector::CleanFinishedThreads() {
        std::unique_lock lock(_mutex);

        std::vector<std::int32_t> toRemove;
        toRemove.reserve(_threads.size() / 4);  // Reserve some space to reduce allocations

        for (const auto& [threadID, threadData] : _threads) {
            if (threadData.finished) {
                toRemove.push_back(threadID);
            }
        }

        for (auto threadID : toRemove) {
            _threads.erase(threadID);
        }

        if (!toRemove.empty()) {
            SKSE::log::trace("ThreadsCollector: Cleaned {} finished threads", toRemove.size());
        }
    }

    void ThreadsCollector::ClearAll() {
        std::unique_lock lock(_mutex);
        _threads.clear();
        SKSE::log::info("ThreadsCollector: All thread data cleared");
    }

    // ========================================================================================
    // Actor Management
    // ========================================================================================

    ThreadActor& ThreadsCollector::GetOrCreateActor(std::int32_t threadID, std::uint32_t actorFormID) {
        std::unique_lock lock(_mutex);
        return _threads[threadID].actors[actorFormID];
    }

    const ThreadActor* ThreadsCollector::FindActor(std::int32_t threadID, std::uint32_t actorFormID) const {
        std::shared_lock lock(_mutex);
        auto threadIt = _threads.find(threadID);
        if (threadIt == _threads.end()) {
            return nullptr;
        }

        auto actorIt = threadIt->second.actors.find(actorFormID);
        return (actorIt != threadIt->second.actors.end()) ? &actorIt->second : nullptr;
    }

    std::vector<std::uint32_t> ThreadsCollector::GetActorFormIDs(std::int32_t threadID) const {
        std::shared_lock lock(_mutex);
        std::vector<std::uint32_t> result;

        auto it = _threads.find(threadID);
        if (it != _threads.end()) {
            const auto& actors = it->second.actors;
            result.reserve(actors.size());
            for (const auto& [formID, _] : actors) {
                result.push_back(formID);
            }
        }

        return result;
    }

    // ========================================================================================
    // Excitement Contribution System
    // ========================================================================================

    ExcitementContributor& ThreadsCollector::GetOrCreateContributor(std::int32_t threadID, std::uint32_t actorFormID,
                                                                    std::uint32_t loverFormID) {
        // Caller must hold unique_lock
        return _threads[threadID].actors[actorFormID].excitementContribution.contributors[loverFormID];
    }

    const ExcitementContributor* ThreadsCollector::FindContributor(std::int32_t threadID, std::uint32_t actorFormID,
                                                                   std::uint32_t loverFormID) const {
        // Caller must hold shared_lock or unique_lock
        auto threadIt = _threads.find(threadID);
        if (threadIt == _threads.end()) {
            return nullptr;
        }

        auto actorIt = threadIt->second.actors.find(actorFormID);
        if (actorIt == threadIt->second.actors.end()) {
            return nullptr;
        }

        auto contribIt = actorIt->second.excitementContribution.contributors.find(loverFormID);
        return (contribIt != actorIt->second.excitementContribution.contributors.end()) ? &contribIt->second : nullptr;
    }

    void ThreadsCollector::ExcitementContributorOrgasm(std::int32_t threadID, std::uint32_t actorFormID) {
        std::unique_lock lock(_mutex);

        auto threadIt = _threads.find(threadID);
        if (threadIt == _threads.end()) {
            return;
        }

        auto actorIt = threadIt->second.actors.find(actorFormID);
        if (actorIt == threadIt->second.actors.end()) {
            return;
        }

        auto& contributors = actorIt->second.excitementContribution.contributors;
        if (contributors.empty()) {
            return;
        }

        // Flush accrual up to now so the time since the last rate change counts
        const double currentTime = GetCurrentTimestamp();
        float totalContribution = 0.0f;
        for (auto& [_, contrib] : contributors) {
            if (contrib.lastUpdated > 0.0) {
                contrib.total += static_cast<float>((currentTime - contrib.lastUpdated) * contrib.rate);
            }
            contrib.lastUpdated = currentTime;
            totalContribution += contrib.total;
        }

        if (totalContribution > 0.0f) {
            // Distribute orgasm shares proportionally
            for (auto& [_, contrib] : contributors) {
                contrib.orgasms += contrib.total / totalContribution;
            }
        } else {
            // Nothing accrued yet: split evenly across active partners, else across all partners
            std::vector<ExcitementContributor*> recipients;
            for (auto& [loverFormID, contrib] : contributors) {
                if (loverFormID != actorFormID && contrib.rate > 0.0f) {
                    recipients.push_back(&contrib);
                }
            }
            if (recipients.empty()) {
                for (auto& [loverFormID, contrib] : contributors) {
                    if (loverFormID != actorFormID) {
                        recipients.push_back(&contrib);
                    }
                }
            }
            for (auto* contrib : recipients) {
                contrib->orgasms += 1.0f / static_cast<float>(recipients.size());
            }
        }

        // Reset totals but keep rates, so accrual continues until the next scene change
        for (auto& [_, contrib] : contributors) {
            contrib.total = 0.0f;
        }
    }

    void ThreadsCollector::ExcitementContributorOrgasmWeighted(
        std::int32_t threadID, std::uint32_t actorFormID, const std::vector<std::pair<std::uint32_t, float>>& weights) {
        std::unique_lock lock(_mutex);

        auto threadIt = _threads.find(threadID);
        if (threadIt == _threads.end()) {
            return;
        }

        auto actorIt = threadIt->second.actors.find(actorFormID);
        if (actorIt == threadIt->second.actors.end()) {
            return;
        }

        auto& contributors = actorIt->second.excitementContribution.contributors;
        if (contributors.empty()) {
            return;
        }

        // Only weights of existing contributors count; never create one here
        float totalWeight = 0.0f;
        for (const auto& [loverFormID, weight] : weights) {
            if (weight > 0.0f && contributors.contains(loverFormID)) {
                totalWeight += weight;
            }
        }
        if (totalWeight <= 0.0f) {
            return;
        }

        for (const auto& [loverFormID, weight] : weights) {
            if (weight <= 0.0f) {
                continue;
            }
            auto it = contributors.find(loverFormID);
            if (it != contributors.end()) {
                it->second.orgasms += weight / totalWeight;
            }
        }

        // This orgasm replaced the time split: drop accrual up to now but keep rates
        const double currentTime = GetCurrentTimestamp();
        for (auto& [_, contrib] : contributors) {
            contrib.total = 0.0f;
            contrib.lastUpdated = currentTime;
        }
    }

    void ThreadsCollector::UpdateExcitementRate(std::int32_t threadID, std::uint32_t actorFormID,
                                                std::uint32_t loverFormID, float rate) {
        std::unique_lock lock(_mutex);

        auto& contributor = _threads[threadID].actors[actorFormID].excitementContribution.contributors[loverFormID];

        // Get current timestamp
        double currentTime = GetCurrentTimestamp();

        // Accrue with the previous rate since this contributor's last update
        if (contributor.lastUpdated > 0.0) {
            double delta = currentTime - contributor.lastUpdated;
            contributor.total += static_cast<float>(delta * contributor.rate);
        }

        // Update rate and timestamp
        contributor.rate = rate;
        contributor.lastUpdated = currentTime;
    }

    // ========================================================================================
    // Generic Getters/Setters with Dot-Separated Property Paths
    // ========================================================================================

    // Thread-level getters
    int ThreadsCollector::GetThreadInt(std::int32_t threadID, [[maybe_unused]] std::string_view propName) const {
        const auto* thread = FindThread(threadID);
        if (!thread) return 0;

        // No int properties at thread level currently
        return 0;
    }

    std::string ThreadsCollector::GetThreadStr(std::int32_t threadID, std::string_view propName) const {
        const auto* thread = FindThread(threadID);
        if (!thread) return "";

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1) {
            if (parts[0] == "lastsexualsceneid") {
                return thread->lastSexualSceneId;
            }
        }
        return "";
    }

    bool ThreadsCollector::GetThreadBool(std::int32_t threadID, std::string_view propName) const {
        const auto* thread = FindThread(threadID);
        if (!thread) return false;

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1) {
            if (parts[0] == "finished") {
                return thread->finished;
            } else if (parts[0] == "hadsex") {
                return thread->hadSex;
            }
        }
        return false;
    }

    // Thread-level setters
    void ThreadsCollector::SetThreadInt(std::int32_t threadID, [[maybe_unused]] std::string_view propName,
                                        [[maybe_unused]] int value) {
        [[maybe_unused]] std::unique_lock lock(_mutex);
        [[maybe_unused]] auto& thread = _threads[threadID];

        // No int properties at thread level currently
    }

    void ThreadsCollector::SetThreadStr(std::int32_t threadID, std::string_view propName, std::string_view value) {
        std::unique_lock lock(_mutex);
        auto& thread = _threads[threadID];

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1) {
            if (parts[0] == "lastsexualsceneid") {
                thread.lastSexualSceneId = value;
            }
        }
    }

    void ThreadsCollector::SetThreadBool(std::int32_t threadID, std::string_view propName, bool value) {
        std::unique_lock lock(_mutex);
        auto& thread = _threads[threadID];

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1) {
            if (parts[0] == "finished") {
                thread.finished = value;
            } else if (parts[0] == "hadsex") {
                thread.hadSex = value;
            }
        }
    }

    // Actor-level getters
    int ThreadsCollector::GetActorInt(std::int32_t threadID, std::uint32_t actorFormID,
                                      std::string_view propName) const {
        const auto* actor = FindActor(threadID, actorFormID);
        if (!actor) return 0;

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1 && parts[0] == "anatomy") {
            return actor->anatomy;
        }
        return 0;
    }

    float ThreadsCollector::GetActorFlt(std::int32_t threadID, std::uint32_t actorFormID,
                                        [[maybe_unused]] std::string_view propName) const {
        const auto* actor = FindActor(threadID, actorFormID);
        if (!actor) return 0.0f;

        // No float properties at actor level currently
        return 0.0f;
    }

    bool ThreadsCollector::GetActorBool(std::int32_t threadID, std::uint32_t actorFormID,
                                        std::string_view propName) const {
        const auto* actor = FindActor(threadID, actorFormID);
        if (!actor) return false;

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1) {
            if (parts[0] == "orgasmed") {
                return actor->orgasmed;
            } else if (parts[0] == "hadsamesexencounter") {
                return actor->hadSameSexEncounter;
            }
        } else if (parts.size() == 2) {
            // Support for did.action and got.action
            const auto& category = parts[0];
            const auto& actionName = parts[1];

            if (category == "did") {
                auto it = actor->did.find(actionName);
                return (it != actor->did.end()) ? it->second : false;
            } else if (category == "got") {
                auto it = actor->got.find(actionName);
                return (it != actor->got.end()) ? it->second : false;
            }
        }
        return false;
    }

    // Actor-level setters
    void ThreadsCollector::SetActorInt(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName,
                                       int value) {
        std::unique_lock lock(_mutex);
        auto& actor = _threads[threadID].actors[actorFormID];

        // Any other name only creates the actor (feeders use "participant")
        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1 && parts[0] == "anatomy") {
            actor.anatomy = value;
        }
    }

    void ThreadsCollector::SetActorFlt(std::int32_t threadID, std::uint32_t actorFormID,
                                       [[maybe_unused]] std::string_view propName, [[maybe_unused]] float value) {
        [[maybe_unused]] std::unique_lock lock(_mutex);
        [[maybe_unused]] auto& actor = _threads[threadID].actors[actorFormID];

        // No float properties at actor level currently
    }

    void ThreadsCollector::SetActorBool(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName,
                                        bool value) {
        std::unique_lock lock(_mutex);
        auto& actor = _threads[threadID].actors[actorFormID];

        auto parts = ParsePropertyPath(propName);
        if (parts.size() == 1) {
            if (parts[0] == "orgasmed") {
                actor.orgasmed = value;
            } else if (parts[0] == "hadsamesexencounter") {
                actor.hadSameSexEncounter = value;
            }
        } else if (parts.size() == 2) {
            // Support for did.action and got.action
            const auto& category = parts[0];
            const auto& actionName = parts[1];

            if (category == "did") {
                actor.did[actionName] = value;
            } else if (category == "got") {
                actor.got[actionName] = value;
            }
        }
    }

    // ========================================================================================
    // Apply Thread Data to LoversLedgerService
    // ========================================================================================

    namespace {
        // Helper: Extract actions from action map
        std::vector<std::string> ExtractActions(const std::unordered_map<std::string, bool>& actionMap) {
            std::vector<std::string> actions;
            actions.reserve(actionMap.size());
            for (const auto& [actionName, _] : actionMap) {
                actions.push_back(actionName);
            }
            return actions;
        }

        // Helper: Resolve an Actor reference FormID to a TESNPC base FormID.
        // Returns {baseFormID, true} if the actor is loaded and IsUnique(); {0, false} otherwise.
        std::pair<std::uint32_t, bool> ResolveUniqueBaseID(std::uint32_t refFormID) {
            auto* form = RE::TESForm::LookupByID(refFormID);
            auto* actor = form ? form->As<RE::Actor>() : nullptr;
            if (!actor) return {0, false};
            auto* base = LL::GetStableBase(actor);
            if (!base) return {0, false};
            SKSE::log::debug("ResolveUniqueBaseID: 0x{:X}({}) -> base 0x{:X} ({})", refFormID, actor->GetName(), base->GetFormID(), base->GetName());
            if (!LL::IsEffectivelyUnique(base)) return {0, false};
            return {base->GetFormID(), true};
        }

        // Helper: Apply actor's basic data to ledger
        void ApplyActorBasicData(LL::LoversLedgerService& ledger, std::uint32_t actorFormID,
                                 const ThreadActor& actorData, std::string_view encounterType) {
            auto [baseFormID, isUnique] = ResolveUniqueBaseID(actorFormID);
            if (!isUnique) {
                SKSE::log::trace("ApplyActorBasicData: skipping non-unique actor 0x{:X}", actorFormID);
                return;
            }
            // Update last time
            ledger.UpdateLastTime(baseFormID);

            // Increment encounter type counter
            ledger.IncrementInt(baseFormID, encounterType);

            // Increment same-sex encounter if applicable
            if (actorData.hadSameSexEncounter) {
                ledger.IncrementInt(baseFormID, SAME_SEX_ENCOUNTER);
            }
        }

        // Helper: Apply actor's actions to ledger
        void ApplyActorActions(LL::LoversLedgerService& ledger, std::uint32_t actorFormID,
                               const ThreadActor& actorData) {
            auto [baseFormID, isUnique] = ResolveUniqueBaseID(actorFormID);
            if (!isUnique) {
                SKSE::log::trace("ApplyActorActions: skipping non-unique actor 0x{:X}", actorFormID);
                return;
            }

            // Update "did" actions
            if (!actorData.did.empty()) {
                auto didActions = ExtractActions(actorData.did);
                ledger.UpdateActions(baseFormID, didActions, DID_ACTION);
            }

            // Update "got" actions
            if (!actorData.got.empty()) {
                auto gotActions = ExtractActions(actorData.got);
                ledger.UpdateActions(baseFormID, gotActions, GOT_ACTION);
            }
        }

        // Helper: Apply excitement contributions to ledger
        void ApplyExcitementContributions(LL::LoversLedgerService& ledger, std::uint32_t actorFormID,
                                          const ThreadActor& actorData, size_t actorCount) {
            auto [npcBaseFormID, npcUnique] = ResolveUniqueBaseID(actorFormID);
            if (!npcUnique) {
                SKSE::log::trace("ApplyExcitementContributions: skipping non-unique actor 0x{:X}", actorFormID);
                return;
            }

            SKSE::log::info("Actor 0x{:X} (base 0x{:X}) has {} excitement contributors", actorFormID, npcBaseFormID,
                            actorData.excitementContribution.contributors.size());

            for (const auto& [loverFormID, contributor] : actorData.excitementContribution.contributors) {
                SKSE::log::info("  Contributor: loverFormID=0x{:X}, orgasms={}", loverFormID, contributor.orgasms);

                if (loverFormID == actorFormID) {
                    continue;  // Don't count self
                }

                // Determine encounter type for lover update
                std::string_view loverEncounterType;
                if (actorCount == 2) {
                    loverEncounterType = EXCLUSIVE_SEX;
                } else if (actorCount > 2) {
                    loverEncounterType = PART_OF_SAME_GROUP_SEX;
                } else {
                    continue;  // Solo - no lover to update
                }

                auto [loverBaseFormID, loverUnique] = ResolveUniqueBaseID(loverFormID);
                if (!loverUnique) {
                    // Non-unique or unloaded lover — count toward othersCount
                    SKSE::log::info("  Lover 0x{:X} is non-unique/unloaded, bumping othersCount for NPC base 0x{:X}",
                                    loverFormID, npcBaseFormID);
                    ledger.UpdateOthers(npcBaseFormID);
                } else {
                    SKSE::log::info("  Calling UpdateLover(npc base=0x{:X}, lover base=0x{:X}, type={}, orgasms={})",
                                    npcBaseFormID, loverBaseFormID, loverEncounterType, contributor.orgasms);
                    ledger.UpdateLover(npcBaseFormID, loverBaseFormID, loverEncounterType, contributor.orgasms);
                }
            }
        }

        // Helper: Add the wildcard-expanded global counter keys for one actor's acts.
        // Counts every participant (unique or not); skips actors that are no longer loaded.
        void CollectActKeys(std::uint32_t actorFormID, const ThreadActor& actorData, std::set<LL::ActKey>& keys) {
            auto* form = RE::TESForm::LookupByID(actorFormID);
            auto* actor = form ? form->As<RE::Actor>() : nullptr;
            auto* base = actor ? actor->GetActorBase() : nullptr;
            if (!base) {
                SKSE::log::trace("CollectActKeys: actor 0x{:X} not loaded, skipping", actorFormID);
                return;
            }
            auto* race = actor->GetRace();
            const std::uint32_t raceID = race ? race->GetFormID() : 0;
            const auto gender = static_cast<std::int8_t>(base->GetSex());

            auto addAct = [&](const std::string& rawType, std::uint8_t dir) {
                // Cluster word: two synonyms in one scene collapse to one key in the set
                const std::string type = LL::LoversLedgerService::GetSingleton().Canonical(rawType);
                for (std::uint32_t r : {raceID, 0u}) {
                    for (std::int8_t g : {gender, std::int8_t(-1)}) {
                        for (const std::string& t : {type, std::string()}) {
                            for (std::uint8_t d : {dir, LL::kActDirAny}) {
                                keys.insert(LL::ActKey{r, g, t, d});
                            }
                        }
                    }
                }
            };
            for (const auto& [type, _] : actorData.did) {
                addAct(type, LL::kActDirGave);
            }
            for (const auto& [type, _] : actorData.got) {
                addAct(type, LL::kActDirReceived);
            }
        }
    }  // namespace

    void ThreadsCollector::ApplyThreadToLedger(std::int32_t threadID) {
        std::shared_lock lock(_mutex);

        // _mutex is not recursive: look up directly instead of FindThread
        auto threadIt = _threads.find(threadID);
        const auto* thread = (threadIt != _threads.end()) ? &threadIt->second : nullptr;
        if (!thread) {
            SKSE::log::warn("ThreadsCollector::ApplyThreadToLedger - Thread {} not found", threadID);
            return;
        }

        const size_t actorCount = thread->actors.size();
        const std::string_view encounterType = GetEncounterTypeFromCount(actorCount);

        auto& ledger = LL::LoversLedgerService::GetSingleton();

        // First-time partner traits, before this scene changes the totals their one-time seed reads
        if (actorCount >= 2) {
            for (const auto& [actorFormID, _] : thread->actors) {
                auto [baseFormID, isUnique] = ResolveUniqueBaseID(actorFormID);
                if (!isUnique) continue;
                std::vector<LL::PartnerRef> partners;
                for (const auto& [otherFormID, other] : thread->actors) {
                    if (otherFormID != actorFormID) partners.push_back(LL::PartnerRef{otherFormID, other.anatomy});
                }
                ledger.RecordPartners(baseFormID, partners, static_cast<int>(actorCount));
            }
        }

        // Process each actor in the thread
        std::set<LL::ActKey> actKeys;
        for (const auto& [actorFormID, actorData] : thread->actors) {
            ApplyActorBasicData(ledger, actorFormID, actorData, encounterType);
            ApplyActorActions(ledger, actorFormID, actorData);
            ApplyExcitementContributions(ledger, actorFormID, actorData, actorCount);
            CollectActKeys(actorFormID, actorData, actKeys);
        }
        ledger.RecordScene({actKeys.begin(), actKeys.end()}, static_cast<int>(actorCount));

        SKSE::log::info("ThreadsCollector: Applied thread {} data to ledger ({} actors)", threadID, actorCount);
    }

}  // namespace LL
