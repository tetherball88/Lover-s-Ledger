#include "LoversLedgerService.h"
#include "PCH.h"

namespace LL_Papyrus {

    // Use fully-qualified types from CommonLibSSE

    // Record an action for npc (did or got)
    void RecordAction(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString actionName, bool isDid) {
        if (!npc) {
            SKSE::log::warn("RecordAction: null actor");
            return;
        }
        std::uint32_t id = npc->GetFormID();
        const char* npcName = npc->GetName();
        SKSE::log::trace("RecordAction: npc=0x{:X} ({}), action={}, isDid={}", id, npcName ? npcName : "Unknown",
                         actionName.data(), isDid);
        std::vector<std::string> actions{actionName.data()};
        LL::LoversLedgerService::GetSingleton().UpdateActions(id, actions, isDid ? "did" : "got");
    }

    // Increment a named counter (same names as in your request)
    void IncrementInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString intName) {
        if (!npc) {
            SKSE::log::warn("IncrementInt: null actor");
            return;
        }
        std::uint32_t id = npc->GetFormID();
        const char* npcName = npc->GetName();
        SKSE::log::trace("IncrementInt: npc=0x{:X} ({}), intName={}", id, npcName ? npcName : "Unknown",
                         intName.data());
        LL::LoversLedgerService::GetSingleton().IncrementInt(id, intName.data());
    }

    // Generic getters with dot-separated property paths
    std::int32_t GetNpcInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName) {
        if (!npc) {
            SKSE::log::warn("GetNpcInt: null actor");
            return 0;
        }
        std::uint32_t id = npc->GetFormID();
        auto result = LL::LoversLedgerService::GetSingleton().GetNpcInt(id, propName.data());
        SKSE::log::trace("GetNpcInt: npc=0x{:X}, prop={}, result={}", id, propName.data(), result);
        return result;
    }

    float GetNpcFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName) {
        if (!npc) {
            SKSE::log::warn("GetNpcFlt: null actor");
            return 0.0f;
        }
        std::uint32_t id = npc->GetFormID();
        auto result = LL::LoversLedgerService::GetSingleton().GetNpcFloat(id, propName.data());
        SKSE::log::trace("GetNpcFlt: npc=0x{:X}, prop={}, result={}", id, propName.data(), result);
        return result;
    }

    std::int32_t GetLoverInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName) {
        if (!npc || !lover) {
            SKSE::log::warn("GetLoverInt: null actor (npc={}, lover={})", fmt::ptr(npc), fmt::ptr(lover));
            return 0;
        }
        std::uint32_t id = npc->GetFormID();
        std::uint32_t lid = lover->GetFormID();
        auto result = LL::LoversLedgerService::GetSingleton().GetLoverInt(id, lid, propName.data());
        SKSE::log::trace("GetLoverInt: npc=0x{:X}, lover=0x{:X}, prop={}, result={}", id, lid, propName.data(), result);
        return result;
    }

    float GetLoverFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName) {
        if (!npc || !lover) {
            SKSE::log::warn("GetLoverFlt: null actor (npc={}, lover={})", fmt::ptr(npc), fmt::ptr(lover));
            return 0.0f;
        }
        std::uint32_t id = npc->GetFormID();
        std::uint32_t lid = lover->GetFormID();
        auto result = LL::LoversLedgerService::GetSingleton().GetLoverFloat(id, lid, propName.data());
        SKSE::log::trace("GetLoverFlt: npc=0x{:X}, lover=0x{:X}, prop={}, result={}", id, lid, propName.data(), result);
        return result;
    }

    // Generic setters with dot-separated property paths
    void SetNpcInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName, std::int32_t value) {
        if (!npc) {
            SKSE::log::warn("SetNpcInt: null actor");
            return;
        }
        std::uint32_t id = npc->GetFormID();
        SKSE::log::trace("SetNpcInt: npc=0x{:X}, prop={}, value={}", id, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetNpcInt(id, propName.data(), value);
    }

    void SetNpcFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName, float value) {
        if (!npc) {
            SKSE::log::warn("SetNpcFlt: null actor");
            return;
        }
        std::uint32_t id = npc->GetFormID();
        SKSE::log::trace("SetNpcFlt: npc=0x{:X}, prop={}, value={}", id, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetNpcFloat(id, propName.data(), value);
    }

    void SetLoverInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName,
                     std::int32_t value) {
        if (!npc || !lover) {
            SKSE::log::warn("SetLoverInt: null actor (npc={}, lover={})", fmt::ptr(npc), fmt::ptr(lover));
            return;
        }
        std::uint32_t id = npc->GetFormID();
        std::uint32_t lid = lover->GetFormID();
        SKSE::log::trace("SetLoverInt: npc=0x{:X}, lover=0x{:X}, prop={}, value={}", id, lid, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetLoverInt(id, lid, propName.data(), value);
    }

    void SetLoverFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName,
                     float value) {
        if (!npc || !lover) {
            SKSE::log::warn("SetLoverFlt: null actor (npc={}, lover={})", fmt::ptr(npc), fmt::ptr(lover));
            return;
        }
        std::uint32_t id = npc->GetFormID();
        std::uint32_t lid = lover->GetFormID();
        SKSE::log::trace("SetLoverFlt: npc=0x{:X}, lover=0x{:X}, prop={}, value={}", id, lid, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetLoverFloat(id, lid, propName.data(), value);
    }

    // Get all registered NPCs
    std::vector<RE::Actor*> GetAllNPCs(RE::StaticFunctionTag*) {
        auto formIDs = LL::LoversLedgerService::GetSingleton().GetAllNPCs();
        std::vector<RE::Actor*> actors;
        actors.reserve(formIDs.size());

        SKSE::log::debug("GetAllNPCs: Found {} NPCs in ledger", formIDs.size());

        for (auto formID : formIDs) {
            auto* form = RE::TESForm::LookupByID(formID);
            if (form) {
                auto* actor = form->As<RE::Actor>();
                if (actor) {
                    actors.push_back(actor);
                } else {
                    SKSE::log::warn("GetAllNPCs: FormID 0x{:X} is not an actor", formID);
                }
            } else {
                SKSE::log::debug("GetAllNPCs: Failed to lookup FormID 0x{:X}", formID);
            }
        }

        return actors;
    }

    // Get all lovers for an NPC
    std::vector<RE::Actor*> GetAllLovers(RE::StaticFunctionTag*, RE::Actor* npc, std::int32_t topK) {
        if (!npc) {
            SKSE::log::warn("GetAllLovers: null actor");
            return {};
        }

        std::uint32_t id = npc->GetFormID();
        auto loverFormIDs = LL::LoversLedgerService::GetSingleton().GetAllLovers(id, topK);
        std::vector<RE::Actor*> lovers;
        lovers.reserve(loverFormIDs.size());

        SKSE::log::debug("GetAllLovers: npc=0x{:X}, topK={}, found {} lovers", id, topK, loverFormIDs.size());

        for (auto loverID : loverFormIDs) {
            auto* form = RE::TESForm::LookupByID(loverID);
            if (form) {
                auto* lover = form->As<RE::Actor>();
                if (lover) {
                    lovers.push_back(lover);
                } else {
                    SKSE::log::warn("GetAllLovers: Lover FormID 0x{:X} is not an actor", loverID);
                }
            } else {
                SKSE::log::debug("GetAllLovers: Failed to lookup lover FormID 0x{:X}", loverID);
            }
        }

        return lovers;
    }

    // Get sorted actions that NPC did or got (by count, descending)
    std::vector<RE::BSFixedString> GetAllActions(RE::StaticFunctionTag*, RE::Actor* npc, bool isDid,
                                                 std::int32_t topK) {
        if (!npc) {
            SKSE::log::warn("GetAllActions: null actor");
            return {};
        }

        std::uint32_t id = npc->GetFormID();
        auto actions = LL::LoversLedgerService::GetSingleton().GetAllActions(id, isDid, topK);

        SKSE::log::debug("GetAllActions: npc=0x{:X}, isDid={}, topK={}, found {} actions", id, isDid, topK,
                         actions.size());

        std::vector<RE::BSFixedString> result;
        result.reserve(actions.size());
        for (const auto& action : actions) {
            result.push_back(RE::BSFixedString(action.c_str()));
        }

        return result;
    }

    // Get count for a specific action
    std::int32_t GetActionCount(RE::StaticFunctionTag*, RE::Actor* npc, bool isDid, RE::BSFixedString actionName) {
        if (!npc) {
            SKSE::log::warn("GetActionCount: null actor");
            return 0;
        }

        std::uint32_t id = npc->GetFormID();
        auto result = LL::LoversLedgerService::GetSingleton().GetActionCount(id, isDid, actionName.data());
        SKSE::log::trace("GetActionCount: npc=0x{:X}, isDid={}, action={}, result={}", id, isDid, actionName.data(),
                         result);
        return result;
    }

    // Calculate lover score based on encounters, orgasms, and time decay
    float GetLoverScore(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover) {
        if (!npc || !lover) {
            SKSE::log::warn("GetLoverScore: null actor (npc={}, lover={})", fmt::ptr(npc), fmt::ptr(lover));
            return 0.0f;
        }

        std::uint32_t id = npc->GetFormID();
        std::uint32_t lid = lover->GetFormID();
        auto result = LL::LoversLedgerService::GetSingleton().GetLoverScore(id, lid);
        SKSE::log::trace("GetLoverScore: npc=0x{:X}, lover=0x{:X}, score={}", id, lid, result);
        return result;
    }

    // Register function called by SKSE Papyrus interface
    bool RegisterLoversLedgerPapyrus(RE::BSScript::IVirtualMachine* a_vm) {
        // Generic getters with dot-separated property paths
        a_vm->RegisterFunction("GetNpcInt", "TTLL_Store", GetNpcInt);
        a_vm->RegisterFunction("GetNpcFlt", "TTLL_Store", GetNpcFlt);
        a_vm->RegisterFunction("GetLoverInt", "TTLL_Store", GetLoverInt);
        a_vm->RegisterFunction("GetLoverFlt", "TTLL_Store", GetLoverFlt);

        // Generic setters with dot-separated property paths
        a_vm->RegisterFunction("SetNpcInt", "TTLL_Store", SetNpcInt);
        a_vm->RegisterFunction("SetNpcFlt", "TTLL_Store", SetNpcFlt);
        a_vm->RegisterFunction("SetLoverInt", "TTLL_Store", SetLoverInt);
        a_vm->RegisterFunction("SetLoverFlt", "TTLL_Store", SetLoverFlt);

        // Consolidated action functions
        a_vm->RegisterFunction("RecordAction", "TTLL_Store", RecordAction);
        a_vm->RegisterFunction("GetAllActions", "TTLL_Store", GetAllActions);
        a_vm->RegisterFunction("GetActionCount", "TTLL_Store", GetActionCount);

        a_vm->RegisterFunction("IncrementInt", "TTLL_Store", IncrementInt);

        a_vm->RegisterFunction("GetAllNPCs", "TTLL_Store", GetAllNPCs);
        a_vm->RegisterFunction("GetAllLovers", "TTLL_Store", GetAllLovers);

        a_vm->RegisterFunction("GetLoverScore", "TTLL_Store", GetLoverScore);

        return true;
    }

}  // namespace LL_Papyrus

// Expose the registration function in global namespace for plugin.cpp
bool RegisterLoversLedgerPapyrus(RE::BSScript::IVirtualMachine* a_vm) {
    return LL_Papyrus::RegisterLoversLedgerPapyrus(a_vm);
}
