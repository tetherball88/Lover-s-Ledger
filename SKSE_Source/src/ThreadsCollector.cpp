#include <algorithm>
#include <chrono>

#include "LoversLedgerService.h"
#include "ThreadsCollector.h"

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

        // Calculate total contribution
        float totalContribution = 0.0f;
        for (const auto& [_, contrib] : contributors) {
            totalContribution += contrib.total;
        }

        if (totalContribution <= 0.0f) {
            return;
        }

        // Distribute orgasm shares proportionally
        for (auto& [_, contrib] : contributors) {
            float share = contrib.total / totalContribution;
            contrib.orgasms += share;
            contrib.total = 0.0f;
            contrib.rate = 0.0f;
        }
    }

    void ThreadsCollector::UpdateExcitementRate(std::int32_t threadID, std::uint32_t actorFormID,
                                                std::uint32_t loverFormID, float rate) {
        std::unique_lock lock(_mutex);

        auto& thread = _threads[threadID];
        auto& contributor = thread.actors[actorFormID].excitementContribution.contributors[loverFormID];

        // Get current timestamp
        double currentTime = GetCurrentTimestamp();

        // Calculate delta time and add accumulated excitement
        if (thread.prevExcitementStarted > 0.0) {
            double delta = currentTime - thread.prevExcitementStarted;
            contributor.total += static_cast<float>(delta * contributor.rate);
        }

        // Update rate and timestamp
        contributor.rate = rate;
        thread.prevExcitementStarted = currentTime;
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
                                      [[maybe_unused]] std::string_view propName) const {
        const auto* actor = FindActor(threadID, actorFormID);
        if (!actor) return 0;

        // No int properties at actor level currently
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
    void ThreadsCollector::SetActorInt(std::int32_t threadID, std::uint32_t actorFormID,
                                       [[maybe_unused]] std::string_view propName, [[maybe_unused]] int value) {
        [[maybe_unused]] std::unique_lock lock(_mutex);
        [[maybe_unused]] auto& actor = _threads[threadID].actors[actorFormID];

        // No int properties at actor level currently
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

        // Helper: Apply actor's basic data to ledger
        void ApplyActorBasicData(LL::LoversLedgerService& ledger, std::uint32_t actorFormID,
                                 const ThreadActor& actorData, std::string_view encounterType) {
            // Update last time
            ledger.UpdateLastTime(actorFormID);

            // Increment encounter type counter
            ledger.IncrementInt(actorFormID, encounterType);

            // Increment same-sex encounter if applicable
            if (actorData.hadSameSexEncounter) {
                ledger.IncrementInt(actorFormID, SAME_SEX_ENCOUNTER);
            }
        }

        // Helper: Apply actor's actions to ledger
        void ApplyActorActions(LL::LoversLedgerService& ledger, std::uint32_t actorFormID,
                               const ThreadActor& actorData) {
            // Update "did" actions
            if (!actorData.did.empty()) {
                auto didActions = ExtractActions(actorData.did);
                ledger.UpdateActions(actorFormID, didActions, DID_ACTION);
            }

            // Update "got" actions
            if (!actorData.got.empty()) {
                auto gotActions = ExtractActions(actorData.got);
                ledger.UpdateActions(actorFormID, gotActions, GOT_ACTION);
            }
        }

        // Helper: Apply excitement contributions to ledger
        void ApplyExcitementContributions(LL::LoversLedgerService& ledger, std::uint32_t actorFormID,
                                          const ThreadActor& actorData, size_t actorCount) {
            SKSE::log::info("Actor 0x{:X} has {} excitement contributors", actorFormID,
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

                // Update lover with orgasm contributions
                SKSE::log::info("  Calling UpdateLover(npc=0x{:X}, lover=0x{:X}, type={}, orgasms={})", actorFormID,
                                loverFormID, loverEncounterType, contributor.orgasms);
                ledger.UpdateLover(actorFormID, loverFormID, loverEncounterType, contributor.orgasms);
            }
        }
    }  // namespace

    void ThreadsCollector::ApplyThreadToLedger(std::int32_t threadID) {
        std::shared_lock lock(_mutex);

        const auto* thread = FindThread(threadID);
        if (!thread) {
            SKSE::log::warn("ThreadsCollector::ApplyThreadToLedger - Thread {} not found", threadID);
            return;
        }

        const size_t actorCount = thread->actors.size();
        const std::string_view encounterType = GetEncounterTypeFromCount(actorCount);

        auto& ledger = LL::LoversLedgerService::GetSingleton();

        // Process each actor in the thread
        for (const auto& [actorFormID, actorData] : thread->actors) {
            ApplyActorBasicData(ledger, actorFormID, actorData, encounterType);
            ApplyActorActions(ledger, actorFormID, actorData);
            ApplyExcitementContributions(ledger, actorFormID, actorData, actorCount);
        }

        SKSE::log::info("ThreadsCollector: Applied thread {} data to ledger ({} actors)", threadID, actorCount);
    }

}  // namespace LL
