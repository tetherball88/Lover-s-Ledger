#include <charconv>
#include <optional>

#include "FirstTimes.h"
#include "LoversLedgerService.h"
#include "PCH.h"
#include "UniqueOverrides.h"

namespace LL_Papyrus {

    // Use fully-qualified types from CommonLibSSE

    // Returns the TESNPC base FormID if the actor is non-null and unique; nullopt otherwise.
    static std::optional<std::uint32_t> UniqueBaseID(RE::Actor* actor) {
        if (!actor) return std::nullopt;
        auto* base = LL::GetStableBase(actor);
        if (!base || !LL::IsEffectivelyUnique(base)) return std::nullopt;
        return base->GetFormID();
    }

    // Record an action for npc (did or got)
    void RecordAction(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString actionName, bool isDid) {
        if (!npc) { SKSE::log::warn("RecordAction: null actor"); return; }
        auto id = UniqueBaseID(npc);
        if (!id) { SKSE::log::trace("RecordAction: skipping non-unique NPC 0x{:X}", npc->GetFormID()); return; }
        SKSE::log::trace("RecordAction: npc=0x{:X} (base=0x{:X}), action={}, isDid={}",
                         npc->GetFormID(), *id, actionName.data(), isDid);
        std::vector<std::string> actions{actionName.data()};
        LL::LoversLedgerService::GetSingleton().UpdateActions(*id, actions, isDid ? "did" : "got");
    }

    // Increment a named counter
    void IncrementInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString intName) {
        if (!npc) { SKSE::log::warn("IncrementInt: null actor"); return; }
        auto id = UniqueBaseID(npc);
        if (!id) { SKSE::log::trace("IncrementInt: skipping non-unique NPC 0x{:X}", npc->GetFormID()); return; }
        SKSE::log::trace("IncrementInt: npc=0x{:X} (base=0x{:X}), intName={}",
                         npc->GetFormID(), *id, intName.data());
        LL::LoversLedgerService::GetSingleton().IncrementInt(*id, intName.data());
    }

    // Generic getters with dot-separated property paths
    std::int32_t GetNpcInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName) {
        if (!npc) { SKSE::log::warn("GetNpcInt: null actor"); return 0; }
        auto id = UniqueBaseID(npc);
        if (!id) { return 0; }
        auto result = LL::LoversLedgerService::GetSingleton().GetNpcInt(*id, propName.data());
        SKSE::log::trace("GetNpcInt: npc base=0x{:X}, prop={}, result={}", *id, propName.data(), result);
        return result;
    }

    float GetNpcFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName) {
        if (!npc) { SKSE::log::warn("GetNpcFlt: null actor"); return 0.0f; }
        auto id = UniqueBaseID(npc);
        if (!id) { return 0.0f; }
        auto result = LL::LoversLedgerService::GetSingleton().GetNpcFloat(*id, propName.data());
        SKSE::log::trace("GetNpcFlt: npc base=0x{:X}, prop={}, result={}", *id, propName.data(), result);
        return result;
    }

    std::int32_t GetLoverInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName) {
        if (!npc || !lover) { SKSE::log::warn("GetLoverInt: null actor"); return 0; }
        auto id  = UniqueBaseID(npc);
        auto lid = UniqueBaseID(lover);
        if (!id || !lid) { return 0; }
        auto result = LL::LoversLedgerService::GetSingleton().GetLoverInt(*id, *lid, propName.data());
        SKSE::log::trace("GetLoverInt: npc base=0x{:X}, lover base=0x{:X}, prop={}, result={}",
                         *id, *lid, propName.data(), result);
        return result;
    }

    float GetLoverFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName) {
        if (!npc || !lover) { SKSE::log::warn("GetLoverFlt: null actor"); return 0.0f; }
        auto id  = UniqueBaseID(npc);
        auto lid = UniqueBaseID(lover);
        if (!id || !lid) { return 0.0f; }
        auto result = LL::LoversLedgerService::GetSingleton().GetLoverFloat(*id, *lid, propName.data());
        SKSE::log::trace("GetLoverFlt: npc base=0x{:X}, lover base=0x{:X}, prop={}, result={}",
                         *id, *lid, propName.data(), result);
        return result;
    }

    // Generic setters with dot-separated property paths
    void SetNpcInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName, std::int32_t value) {
        if (!npc) { SKSE::log::warn("SetNpcInt: null actor"); return; }
        auto id = UniqueBaseID(npc);
        if (!id) { SKSE::log::trace("SetNpcInt: skipping non-unique NPC 0x{:X}", npc->GetFormID()); return; }
        SKSE::log::trace("SetNpcInt: npc base=0x{:X}, prop={}, value={}", *id, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetNpcInt(*id, propName.data(), value);
    }

    void SetNpcFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::BSFixedString propName, float value) {
        if (!npc) { SKSE::log::warn("SetNpcFlt: null actor"); return; }
        auto id = UniqueBaseID(npc);
        if (!id) { SKSE::log::trace("SetNpcFlt: skipping non-unique NPC 0x{:X}", npc->GetFormID()); return; }
        SKSE::log::trace("SetNpcFlt: npc base=0x{:X}, prop={}, value={}", *id, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetNpcFloat(*id, propName.data(), value);
    }

    void SetLoverInt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName,
                     std::int32_t value) {
        if (!npc || !lover) { SKSE::log::warn("SetLoverInt: null actor"); return; }
        auto id = UniqueBaseID(npc);
        if (!id) { SKSE::log::trace("SetLoverInt: skipping non-unique NPC 0x{:X}", npc->GetFormID()); return; }
        auto lid = UniqueBaseID(lover);
        if (!lid) {
            SKSE::log::trace("SetLoverInt: non-unique lover 0x{:X}, bumping othersCount for NPC base 0x{:X}",
                             lover->GetFormID(), *id);
            LL::LoversLedgerService::GetSingleton().UpdateOthers(*id);
            return;
        }
        SKSE::log::trace("SetLoverInt: npc base=0x{:X}, lover base=0x{:X}, prop={}, value={}",
                         *id, *lid, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetLoverInt(*id, *lid, propName.data(), value);
    }

    void SetLoverFlt(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover, RE::BSFixedString propName,
                     float value) {
        if (!npc || !lover) { SKSE::log::warn("SetLoverFlt: null actor"); return; }
        auto id = UniqueBaseID(npc);
        if (!id) { SKSE::log::trace("SetLoverFlt: skipping non-unique NPC 0x{:X}", npc->GetFormID()); return; }
        auto lid = UniqueBaseID(lover);
        if (!lid) {
            SKSE::log::trace("SetLoverFlt: non-unique lover 0x{:X}, bumping othersCount for NPC base 0x{:X}",
                             lover->GetFormID(), *id);
            LL::LoversLedgerService::GetSingleton().UpdateOthers(*id);
            return;
        }
        SKSE::log::trace("SetLoverFlt: npc base=0x{:X}, lover base=0x{:X}, prop={}, value={}",
                         *id, *lid, propName.data(), value);
        LL::LoversLedgerService::GetSingleton().SetLoverFloat(*id, *lid, propName.data(), value);
    }

    // Get all registered NPCs (returns TESNPC base forms — always valid, even when cells are unloaded)
    std::vector<RE::TESForm*> GetAllNPCs(RE::StaticFunctionTag*) {
        auto formIDs = LL::LoversLedgerService::GetSingleton().GetAllNPCs();
        std::vector<RE::TESForm*> forms;
        forms.reserve(formIDs.size());

        SKSE::log::debug("GetAllNPCs: Found {} NPCs in ledger", formIDs.size());

        for (auto formID : formIDs) {
            auto* form = RE::TESForm::LookupByID(formID);
            if (form) {
                forms.push_back(form);
            } else {
                SKSE::log::warn("GetAllNPCs: Failed to lookup base FormID 0x{:X}", formID);
            }
        }

        return forms;
    }

    // Get all lovers for an NPC (returns TESNPC base forms — always valid, even when cells are unloaded)
    std::vector<RE::TESForm*> GetAllLovers(RE::StaticFunctionTag*, RE::Actor* npc, std::int32_t topK) {
        if (!npc) { SKSE::log::warn("GetAllLovers: null actor"); return {}; }
        auto id = UniqueBaseID(npc);
        if (!id) { return {}; }

        auto loverFormIDs = LL::LoversLedgerService::GetSingleton().GetAllLovers(*id, topK);
        std::vector<RE::TESForm*> forms;
        forms.reserve(loverFormIDs.size());

        SKSE::log::debug("GetAllLovers: npc base=0x{:X}, topK={}, found {} lovers", *id, topK, loverFormIDs.size());

        for (auto loverID : loverFormIDs) {
            auto* form = RE::TESForm::LookupByID(loverID);
            if (form) {
                forms.push_back(form);
            } else {
                SKSE::log::warn("GetAllLovers: Failed to lookup lover base FormID 0x{:X}", loverID);
            }
        }

        return forms;
    }

    // Get sorted actions that NPC did or got (by count, descending)
    std::vector<RE::BSFixedString> GetAllActions(RE::StaticFunctionTag*, RE::Actor* npc, bool isDid,
                                                 std::int32_t topK) {
        if (!npc) { SKSE::log::warn("GetAllActions: null actor"); return {}; }
        auto id = UniqueBaseID(npc);
        if (!id) { return {}; }
        auto actions = LL::LoversLedgerService::GetSingleton().GetAllActions(*id, isDid, topK);
        SKSE::log::debug("GetAllActions: npc base=0x{:X}, isDid={}, topK={}, found {} actions",
                         *id, isDid, topK, actions.size());
        std::vector<RE::BSFixedString> result;
        result.reserve(actions.size());
        for (const auto& action : actions) {
            result.push_back(RE::BSFixedString(action.c_str()));
        }
        return result;
    }

    // Get count for a specific action
    std::int32_t GetActionCount(RE::StaticFunctionTag*, RE::Actor* npc, bool isDid, RE::BSFixedString actionName) {
        if (!npc) { SKSE::log::warn("GetActionCount: null actor"); return 0; }
        auto id = UniqueBaseID(npc);
        if (!id) { return 0; }
        auto result = LL::LoversLedgerService::GetSingleton().GetActionCount(*id, isDid, actionName.data());
        SKSE::log::trace("GetActionCount: npc base=0x{:X}, isDid={}, action={}, result={}",
                         *id, isDid, actionName.data(), result);
        return result;
    }

    // Global scene counters: None / -1 / "" = any; direction "gave" / "received", anything else = any
    std::int32_t HowManyTimes(RE::StaticFunctionTag*, RE::TESRace* race, std::int32_t gender,
                              RE::BSFixedString actionType, RE::BSFixedString direction) {
        std::uint8_t dir = LL::kActDirAny;
        if (_stricmp(direction.c_str(), "gave") == 0) {
            dir = LL::kActDirGave;
        } else if (_stricmp(direction.c_str(), "received") == 0) {
            dir = LL::kActDirReceived;
        }
        auto result = LL::LoversLedgerService::GetSingleton().HowManyTimes(
            race ? race->GetFormID() : 0, gender, actionType.c_str(), dir);
        SKSE::log::trace("HowManyTimes: race=0x{:X}, gender={}, type={}, dir={}, result={}",
                         race ? race->GetFormID() : 0, gender, actionType.c_str(), dir, result);
        return result;
    }

    std::int32_t HowManyTimesWith(RE::StaticFunctionTag*, std::int32_t numActors) {
        return LL::LoversLedgerService::GetSingleton().HowManyTimesWith(numActors);
    }

    // Calculate lover score based on encounters, orgasms, and time decay
    float GetLoverScore(RE::StaticFunctionTag*, RE::Actor* npc, RE::Actor* lover) {
        if (!npc || !lover) { SKSE::log::warn("GetLoverScore: null actor"); return 0.0f; }
        auto id  = UniqueBaseID(npc);
        auto lid = UniqueBaseID(lover);
        if (!id || !lid) { return 0.0f; }
        auto result = LL::LoversLedgerService::GetSingleton().GetLoverScore(*id, *lid);
        SKSE::log::trace("GetLoverScore: npc base=0x{:X}, lover base=0x{:X}, score={}", *id, *lid, result);
        return result;
    }

    // SexLab backfill: true once per unique NPC (the first call claims it)
    bool ClaimSexLabCheck(RE::StaticFunctionTag*, RE::Actor* npc) {
        auto id = UniqueBaseID(npc);
        return id && LL::LoversLedgerService::GetSingleton().ClaimSexLabCheck(*id);
    }

    // Copies SexLab history onto the NPC's game relationship partner; false if it has none
    bool AddHistoryToPartner(RE::StaticFunctionTag*, RE::Actor* npc, std::int32_t encounters, std::int32_t solo,
                             std::int32_t vaginal, std::int32_t anal, std::int32_t oral) {
        auto id = UniqueBaseID(npc);
        if (!id) return false;
        return LL::LoversLedgerService::GetSingleton().AddHistoryToPartner(
            *id, LL::SexLabHistory{encounters, solo, vaginal, anal, oral});
    }

    // Copies SexLab history onto a stand-in past lover of that race and gender (0 male, 1 female)
    void AddPastLover(RE::StaticFunctionTag*, RE::Actor* npc, RE::TESRace* race, std::int32_t gender,
                      std::int32_t encounters, std::int32_t solo, std::int32_t vaginal, std::int32_t anal,
                      std::int32_t oral) {
        auto id = UniqueBaseID(npc);
        if (!id || !race) { SKSE::log::warn("AddPastLover: non-unique NPC or null race"); return; }
        LL::LoversLedgerService::GetSingleton().AddPastLover(*id, race->GetFormID(), gender,
                                                             LL::SexLabHistory{encounters, solo, vaginal, anal, oral});
    }

    // First-time lines for a scene; the Int arrays are per position: SexLab counts (missing = 0) and
    // SexLab anatomy, 1 penis | 2 vagina (missing or -1 = base sex)
    RE::BSFixedString DescribeFirstTimes(RE::StaticFunctionTag*, std::vector<RE::Actor*> positions,
                                         RE::BSFixedString tags, std::vector<std::int32_t> slTotal,
                                         std::vector<std::int32_t> slVaginal, std::vector<std::int32_t> slAnal,
                                         std::vector<std::int32_t> anatomy) {
        auto at = [](const std::vector<std::int32_t>& values, std::size_t i, std::int32_t missing = 0) {
            return i < values.size() ? values[i] : missing;
        };
        std::vector<LL::FirstTimePosition> scene;
        scene.reserve(positions.size());
        for (std::size_t i = 0; i < positions.size(); ++i) {
            if (!positions[i]) { SKSE::log::warn("DescribeFirstTimes: null actor at position {}", i); return ""; }
            scene.push_back(LL::FirstTimePosition{positions[i], at(slTotal, i), at(slVaginal, i), at(slAnal, i),
                                                  at(anatomy, i, -1)});
        }
        auto result = LL::DescribeFirstTimes(scene, tags.c_str());
        SKSE::log::trace("DescribeFirstTimes: {} positions, tags={}, result={}", positions.size(), tags.c_str(),
                         result);
        return RE::BSFixedString(result.c_str());
    }

    // SkyrimNet entity UUID (hex, from GetEntityUUID) as a decimal string; "" if not hex.
    // Decision context needs it as an unquoted JSON number: decnpc() rejects a string.
    RE::BSFixedString UuidToDecimal(RE::StaticFunctionTag*, RE::BSFixedString hexUuid) {
        std::string_view in(hexUuid.c_str() ? hexUuid.c_str() : "");
        if (in.size() >= 2 && in[0] == '0' && (in[1] == 'x' || in[1] == 'X')) in.remove_prefix(2);
        std::uint64_t value = 0;
        auto [end, ec] = std::from_chars(in.data(), in.data() + in.size(), value, 16);
        if (in.empty() || ec != std::errc() || end != in.data() + in.size()) {
            SKSE::log::warn("UuidToDecimal: not a hex UUID '{}'", hexUuid.c_str() ? hexUuid.c_str() : "");
            return "";
        }
        return RE::BSFixedString(std::to_string(value).c_str());
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
        a_vm->RegisterFunction("HowManyTimes", "TTLL_Store", HowManyTimes);
        a_vm->RegisterFunction("HowManyTimesWith", "TTLL_Store", HowManyTimesWith);

        a_vm->RegisterFunction("IncrementInt", "TTLL_Store", IncrementInt);

        a_vm->RegisterFunction("GetAllNPCs", "TTLL_Store", GetAllNPCs);
        a_vm->RegisterFunction("GetAllLovers", "TTLL_Store", GetAllLovers);

        a_vm->RegisterFunction("GetLoverScore", "TTLL_Store", GetLoverScore);

        a_vm->RegisterFunction("ClaimSexLabCheck", "TTLL_Store", ClaimSexLabCheck);
        a_vm->RegisterFunction("AddHistoryToPartner", "TTLL_Store", AddHistoryToPartner);
        a_vm->RegisterFunction("AddPastLover", "TTLL_Store", AddPastLover);

        a_vm->RegisterFunction("DescribeFirstTimes", "TTLL_Store", DescribeFirstTimes);

        a_vm->RegisterFunction("UuidToDecimal", "TTLL_Store", UuidToDecimal);

        return true;
    }

}  // namespace LL_Papyrus

// Expose the registration function in global namespace for plugin.cpp
bool RegisterLoversLedgerPapyrus(RE::BSScript::IVirtualMachine* a_vm) {
    return LL_Papyrus::RegisterLoversLedgerPapyrus(a_vm);
}
