#include "Papyrus_ThreadsCollector.h"

#include "ThreadsCollector.h"

namespace OTC_Papyrus {

    using VM = RE::BSScript::IVirtualMachine;
    using StackID = RE::VMStackID;

    // ========================================================================================
    // Thread Management
    // ========================================================================================

    void CleanThread(RE::StaticFunctionTag*, std::int32_t threadID) {
        SKSE::log::debug("CleanThread: threadID={}", threadID);
        LL::ThreadsCollector::GetSingleton().CleanThread(threadID);
    }

    void CleanFinishedThreads(RE::StaticFunctionTag*) {
        SKSE::log::debug("CleanFinishedThreads called");
        LL::ThreadsCollector::GetSingleton().CleanFinishedThreads();
    }

    // ========================================================================================
    // Actor Management
    // ========================================================================================

    std::vector<RE::Actor*> GetActors(RE::StaticFunctionTag*, std::int32_t threadID) {
        std::vector<RE::Actor*> result;

        auto formIDs = LL::ThreadsCollector::GetSingleton().GetActorFormIDs(threadID);
        result.reserve(formIDs.size());

        SKSE::log::debug("GetActors: threadID={}, found {} actors", threadID, formIDs.size());

        for (auto formID : formIDs) {
            if (auto form = RE::TESForm::LookupByID(formID)) {
                if (auto actor = form->As<RE::Actor>()) {
                    result.push_back(actor);
                } else {
                    SKSE::log::warn("GetActors: FormID 0x{:X} is not an actor", formID);
                }
            } else {
                SKSE::log::debug("GetActors: Failed to lookup FormID 0x{:X}", formID);
            }
        }

        return result;
    }

    // ========================================================================================
    // Excitement Contribution
    // ========================================================================================

    void ExcitementContributorOrgasm(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor) {
        if (!actor) {
            SKSE::log::warn("ExcitementContributorOrgasm: null actor");
            return;
        }
        auto formID = actor->GetFormID();
        const char* actorName = actor->GetName();
        SKSE::log::debug("ExcitementContributorOrgasm: threadID={}, actor=0x{:X} ({})", threadID, formID,
                         actorName ? actorName : "Unknown");
        LL::ThreadsCollector::GetSingleton().ExcitementContributorOrgasm(threadID, formID);
    }

    void UpdateExcitementRate(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor, RE::Actor* lover,
                              float rate) {
        if (!actor || !lover) {
            SKSE::log::warn("UpdateExcitementRate: null actor (actor={}, lover={})", fmt::ptr(actor), fmt::ptr(lover));
            return;
        }
        auto actorID = actor->GetFormID();
        auto loverID = lover->GetFormID();
        SKSE::log::trace("UpdateExcitementRate: threadID={}, actor=0x{:X}, lover=0x{:X}, rate={}", threadID, actorID,
                         loverID, rate);
        LL::ThreadsCollector::GetSingleton().UpdateExcitementRate(threadID, actorID, loverID, rate);
    }

    // ========================================================================================
    // Apply Thread to Ledger
    // ========================================================================================

    void ApplyThreadToLedger(RE::StaticFunctionTag*, std::int32_t threadID) {
        SKSE::log::info("ApplyThreadToLedger: threadID={}", threadID);
        LL::ThreadsCollector::GetSingleton().ApplyThreadToLedger(threadID);
    }

    // ========================================================================================
    // Generic Getters/Setters
    // ========================================================================================

    // Thread-level getters
    std::int32_t GetThreadInt(RE::StaticFunctionTag*, std::int32_t threadID, RE::BSFixedString propName) {
        return LL::ThreadsCollector::GetSingleton().GetThreadInt(threadID, propName.data());
    }

    RE::BSFixedString GetThreadStr(RE::StaticFunctionTag*, std::int32_t threadID, RE::BSFixedString propName) {
        auto result = LL::ThreadsCollector::GetSingleton().GetThreadStr(threadID, propName.data());
        return RE::BSFixedString(result.c_str());
    }

    bool GetThreadBool(RE::StaticFunctionTag*, std::int32_t threadID, RE::BSFixedString propName) {
        return LL::ThreadsCollector::GetSingleton().GetThreadBool(threadID, propName.data());
    }

    // Thread-level setters
    void SetThreadInt(RE::StaticFunctionTag*, std::int32_t threadID, RE::BSFixedString propName, std::int32_t value) {
        LL::ThreadsCollector::GetSingleton().SetThreadInt(threadID, propName.data(), value);
    }

    void SetThreadStr(RE::StaticFunctionTag*, std::int32_t threadID, RE::BSFixedString propName,
                      RE::BSFixedString value) {
        LL::ThreadsCollector::GetSingleton().SetThreadStr(threadID, propName.data(), value.data());
    }

    void SetThreadBool(RE::StaticFunctionTag*, std::int32_t threadID, RE::BSFixedString propName, bool value) {
        LL::ThreadsCollector::GetSingleton().SetThreadBool(threadID, propName.data(), value);
    }

    // Actor-level getters
    std::int32_t GetActorInt(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor,
                             RE::BSFixedString propName) {
        if (!actor) return 0;
        return LL::ThreadsCollector::GetSingleton().GetActorInt(threadID, actor->GetFormID(), propName.data());
    }

    float GetActorFlt(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor, RE::BSFixedString propName) {
        if (!actor) return 0.0f;
        return LL::ThreadsCollector::GetSingleton().GetActorFlt(threadID, actor->GetFormID(), propName.data());
    }

    bool GetActorBool(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor, RE::BSFixedString propName) {
        if (!actor) return false;
        return LL::ThreadsCollector::GetSingleton().GetActorBool(threadID, actor->GetFormID(), propName.data());
    }

    // Actor-level setters
    void SetActorInt(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor, RE::BSFixedString propName,
                     std::int32_t value) {
        if (!actor) return;
        LL::ThreadsCollector::GetSingleton().SetActorInt(threadID, actor->GetFormID(), propName.data(), value);
    }

    void SetActorFlt(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor, RE::BSFixedString propName,
                     float value) {
        if (!actor) return;
        LL::ThreadsCollector::GetSingleton().SetActorFlt(threadID, actor->GetFormID(), propName.data(), value);
    }

    void SetActorBool(RE::StaticFunctionTag*, std::int32_t threadID, RE::Actor* actor, RE::BSFixedString propName,
                      bool value) {
        if (!actor) return;
        LL::ThreadsCollector::GetSingleton().SetActorBool(threadID, actor->GetFormID(), propName.data(), value);
    }

    // ========================================================================================
    // Registration
    // ========================================================================================

    bool RegisterThreadsCollectorPapyrus(VM* a_vm) {
        if (!a_vm) {
            SKSE::log::error("ThreadsCollector: VM is null, cannot register Papyrus functions!");
            return false;
        }

        // Thread management
        a_vm->RegisterFunction("CleanThread", "TTLL_ThreadsCollector", CleanThread);
        a_vm->RegisterFunction("CleanFinishedThreads", "TTLL_ThreadsCollector", CleanFinishedThreads);

        // Actor management
        a_vm->RegisterFunction("GetActors", "TTLL_ThreadsCollector", GetActors);

        // Excitement contribution
        a_vm->RegisterFunction("ExcitementContributorOrgasm", "TTLL_ThreadsCollector", ExcitementContributorOrgasm);
        a_vm->RegisterFunction("UpdateExcitementRate", "TTLL_ThreadsCollector", UpdateExcitementRate);

        // Apply thread data to ledger
        a_vm->RegisterFunction("ApplyThreadToLedger", "TTLL_ThreadsCollector", ApplyThreadToLedger);

        // Generic getters/setters - Thread level
        a_vm->RegisterFunction("GetThreadInt", "TTLL_ThreadsCollector", GetThreadInt);
        a_vm->RegisterFunction("GetThreadStr", "TTLL_ThreadsCollector", GetThreadStr);
        a_vm->RegisterFunction("GetThreadBool", "TTLL_ThreadsCollector", GetThreadBool);
        a_vm->RegisterFunction("SetThreadInt", "TTLL_ThreadsCollector", SetThreadInt);
        a_vm->RegisterFunction("SetThreadStr", "TTLL_ThreadsCollector", SetThreadStr);
        a_vm->RegisterFunction("SetThreadBool", "TTLL_ThreadsCollector", SetThreadBool);

        // Generic getters/setters - Actor level
        a_vm->RegisterFunction("GetActorInt", "TTLL_ThreadsCollector", GetActorInt);
        a_vm->RegisterFunction("GetActorFlt", "TTLL_ThreadsCollector", GetActorFlt);
        a_vm->RegisterFunction("GetActorBool", "TTLL_ThreadsCollector", GetActorBool);
        a_vm->RegisterFunction("SetActorInt", "TTLL_ThreadsCollector", SetActorInt);
        a_vm->RegisterFunction("SetActorFlt", "TTLL_ThreadsCollector", SetActorFlt);
        a_vm->RegisterFunction("SetActorBool", "TTLL_ThreadsCollector", SetActorBool);

        SKSE::log::info("ThreadsCollector: Papyrus functions registered successfully");
        return true;
    }

}  // namespace OTC_Papyrus

// Global function for plugin.cpp
bool RegisterThreadsCollectorPapyrus(RE::BSScript::IVirtualMachine* a_vm) {
    return OTC_Papyrus::RegisterThreadsCollectorPapyrus(a_vm);
}
