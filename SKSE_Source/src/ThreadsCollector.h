#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "PCH.h"

namespace LL {

    // Excitement contributor data for a specific lover
    struct ExcitementContributor {
        float rate{0.0f};     // Current excitement contribution rate
        float total{0.0f};    // Total accumulated excitement
        float orgasms{0.0f};  // Orgasm contribution share

        void Reset() {
            rate = 0.0f;
            total = 0.0f;
            orgasms = 0.0f;
        }
    };

    // Excitement contribution tracking for an actor in a thread
    struct ExcitementContribution {
        std::unordered_map<std::uint32_t, ExcitementContributor> contributors;  // Key: lover FormID

        void Clear() { contributors.clear(); }
    };

    // Per-actor data within a thread
    struct ThreadActor {
        std::unordered_map<std::string, bool> did;  // Actions this actor performed
        std::unordered_map<std::string, bool> got;  // Actions this actor received

        bool orgasmed{false};
        bool hadSameSexEncounter{false};
        std::string encounterType;

        ExcitementContribution excitementContribution;

        void Clear() {
            did.clear();
            got.clear();
            orgasmed = false;
            hadSameSexEncounter = false;
            encounterType.clear();
            excitementContribution.Clear();
        }
    };

    // Per-thread data
    struct ThreadData {
        bool finished{false};
        bool hadSex{false};
        double prevExcitementStarted{0.0};
        std::string lastSexualSceneId;

        std::unordered_map<std::uint32_t, ThreadActor> actors;  // Key: actor FormID

        void Clear() {
            finished = false;
            hadSex = false;
            prevExcitementStarted = 0.0f;
            lastSexualSceneId.clear();
            actors.clear();
        }
    };

    /**
     * ThreadsCollector - Runtime-only thread tracking service
     * This service tracks OStim animation threads and actor interactions during sex scenes.
     * Data is NOT persisted to save games - it's only valid during runtime.
     */
    class ThreadsCollector {
    public:
        static ThreadsCollector& GetSingleton() noexcept;

        // Thread management
        ThreadData& GetOrCreateThread(std::int32_t threadID);
        const ThreadData* FindThread(std::int32_t threadID) const;
        void CleanThread(std::int32_t threadID);
        void CleanFinishedThreads();
        void ClearAll();

        // Actor management within threads
        ThreadActor& GetOrCreateActor(std::int32_t threadID, std::uint32_t actorFormID);
        const ThreadActor* FindActor(std::int32_t threadID, std::uint32_t actorFormID) const;
        std::vector<std::uint32_t> GetActorFormIDs(std::int32_t threadID) const;

        // Excitement contribution system
        void ExcitementContributorOrgasm(std::int32_t threadID, std::uint32_t actorFormID);
        void UpdateExcitementRate(std::int32_t threadID, std::uint32_t actorFormID, std::uint32_t loverFormID,
                                  float rate);

        // Generic getters/setters with dot-separated property paths
        // Thread-level properties (no actor parameter)
        int GetThreadInt(std::int32_t threadID, std::string_view propName) const;
        std::string GetThreadStr(std::int32_t threadID, std::string_view propName) const;
        bool GetThreadBool(std::int32_t threadID, std::string_view propName) const;

        void SetThreadInt(std::int32_t threadID, std::string_view propName, int value);
        void SetThreadStr(std::int32_t threadID, std::string_view propName, std::string_view value);
        void SetThreadBool(std::int32_t threadID, std::string_view propName, bool value);

        // Actor-level properties (requires actor parameter)
        int GetActorInt(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName) const;
        float GetActorFlt(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName) const;
        bool GetActorBool(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName) const;

        void SetActorInt(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName, int value);
        void SetActorFlt(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName, float value);
        void SetActorBool(std::int32_t threadID, std::uint32_t actorFormID, std::string_view propName, bool value);

        // Apply thread data to LoversLedgerService
        void ApplyThreadToLedger(std::int32_t threadID);

    private:
        ThreadsCollector() = default;
        ~ThreadsCollector() = default;
        ThreadsCollector(const ThreadsCollector&) = delete;
        ThreadsCollector& operator=(const ThreadsCollector&) = delete;

        ExcitementContributor& GetOrCreateContributor(std::int32_t threadID, std::uint32_t actorFormID,
                                                      std::uint32_t loverFormID);
        const ExcitementContributor* FindContributor(std::int32_t threadID, std::uint32_t actorFormID,
                                                     std::uint32_t loverFormID) const;

        std::unordered_map<std::int32_t, ThreadData> _threads;
        mutable std::shared_mutex _mutex;
    };

}  // namespace LL
