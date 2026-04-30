#include "SkyrimNetDecorators.h"

#include "LoversLedgerService.h"
#include "PCH.h"
#include "UniqueOverrides.h"

#include <functional>
#include <sstream>
#include <string>

// Forward-declare only the pointer we use from SkyrimNetPublicAPI.h.
// Must use extern "C" to match the C linkage given in the header.
extern "C" {
    extern bool (*PublicRegisterDecorator)(const char* name, const char* description,
                                           std::function<std::string(RE::Actor*)> callback);
}

namespace {

    // Returns the NPC base FormID if the actor is non-null and unique; 0 otherwise.
    static std::uint32_t GetUniqueBaseID(RE::Actor* actor) {
        auto* base = LL::GetStableBase(actor);
        if (!base || !LL::IsEffectivelyUnique(base)) return 0;
        return base->GetFormID();
    }

    // Get the display name for a base-form FormID.
    static std::string GetNameForFormID(std::uint32_t formID) {
        auto* form = RE::TESForm::LookupByID(formID);
        if (!form) return "Unknown";
        auto* npc = form->As<RE::TESNPC>();
        if (npc) {
            const char* name = npc->GetName();
            if (name && name[0]) return name;
        }
        return "Unknown";
    }

    // Derive a human-readable bond level from the lover score.
    // Thresholds chosen to align with the template's "weak" / stronger classifications.
    static std::string BondFromScore(float score) {
        if (score < 5.0f)  return "weak";
        if (score < 15.0f) return "casual";
        if (score < 30.0f) return "regular";
        if (score < 60.0f) return "close";
        return "intimate";
    }

    // Convert a game-time difference (in days) to a natural-language recency string.
    static std::string RecencyFromDiff(float diffDays) {
        if (diffDays < 0.0f) diffDays = 0.0f;

        if (diffDays < 1.0f) return "today";

        int days = static_cast<int>(diffDays);
        if (days < 7) {
            return std::to_string(days) + (days == 1 ? " day ago" : " days ago");
        }

        int weeks = days / 7;
        if (weeks < 5) {
            return std::to_string(weeks) + (weeks == 1 ? " week ago" : " weeks ago");
        }

        int months = days / 30;
        if (months < 13) {
            return std::to_string(months) + (months == 1 ? " month ago" : " months ago");
        }

        return "over a year ago";
    }

    // Decorator: ttll_get_npc_sexual_behavior
    // Returns a short prose description of an NPC's sexual behavior patterns,
    // mirroring the logic in 0301_personality_lovers_ledger_insights.prompt.
    static std::string GetNpcSexualBehaviorDecorator(RE::Actor* actor) {
        std::uint32_t baseID = GetUniqueBaseID(actor);
        if (baseID == 0) return "";

        auto& service = LL::LoversLedgerService::GetSingleton();

        int soloSex           = service.GetNpcInt(baseID, "solosex");
        int exclusiveSex      = service.GetNpcInt(baseID, "exclusivesex");
        int groupSex          = service.GetNpcInt(baseID, "groupsex");
        int sameSexEncounter  = service.GetNpcInt(baseID, "samesexencounter");
        int totalEncounters   = soloSex + exclusiveSex + groupSex;

        if (totalEncounters == 0) return "";

        float sameSexRatio  = static_cast<float>(sameSexEncounter) / totalEncounters;
        float soloRatio     = static_cast<float>(soloSex)          / totalEncounters;
        float exclusiveRatio = static_cast<float>(exclusiveSex)    / totalEncounters;
        float groupRatio    = static_cast<float>(groupSex)         / totalEncounters;

        std::string orientation;
        if (sameSexRatio > 0.8f)
            orientation = "primarily same-sex";
        else if (sameSexRatio > 0.4f)
            orientation = "bisexual";
        else if (sameSexRatio > 0.1f)
            orientation = "mostly opposite-sex with some same-sex experience";
        else
            orientation = "exclusively opposite-sex";

        std::string preference;
        if (soloRatio > 0.5f) preference += "frequently solo";
        if (exclusiveRatio > 0.7f)
            preference += (preference.empty() ? "" : ", ") + std::string("prefers one-on-one encounters");
        else if (groupRatio > 0.5f)
            preference += (preference.empty() ? "" : ", ") + std::string("comfortable with group settings");
        else
            preference += (preference.empty() ? "" : ", ") + std::string("mix of private and group encounters");

        std::ostringstream out;
        out << "### Character Sexual Behavior\n"
            << "**Sexual orientation:** " << orientation << "\n"
            << "**Sexual preference:** " << preference << "\n";
        return out.str();
    }

    // Decorator: ttll_get_npc_lovers
    // Returns a Markdown-formatted section listing all non-weak sexual partners of the NPC,
    // mirroring the logic in 0601_relations_lovers_ledger_insights.prompt.
    static std::string GetNpcLoversDecorator(RE::Actor* actor) {
        std::uint32_t baseID = GetUniqueBaseID(actor);
        if (baseID == 0) return "";

        auto& service = LL::LoversLedgerService::GetSingleton();

        // Retrieve all lover FormIDs sorted by score (highest first)
        std::vector<std::uint32_t> loverIDs = service.GetAllLovers(baseID, -1);
        int othersCount = service.GetNpcInt(baseID, "otherscount");

        if (loverIDs.empty() && othersCount == 0) return "";

        // Get current game time for recency computation
        float currentTime = 0.0f;
        if (auto* calendar = RE::Calendar::GetSingleton()) {
            currentTime = calendar->GetDaysPassed();
        }

        std::ostringstream out;
        bool headerWritten = false;
        int weakCount = 0;

        for (std::uint32_t loverID : loverIDs) {
            float score      = service.GetLoverScore(baseID, loverID);
            std::string bond = BondFromScore(score);

            if (bond == "weak") {
                // Tally weak-bond unique lovers into the insignificant count
                weakCount += 1;
                continue;
            }

            int exclusiveSex = service.GetLoverInt(baseID, loverID, "exclusivesex");
            int groupSex     = service.GetLoverInt(baseID, loverID, "partofsamegroupsex");
            int total        = exclusiveSex + groupSex;
            float lastTime   = service.GetLoverFloat(baseID, loverID, "lasttime");

            std::string recency = (lastTime > 0.0f) ? RecencyFromDiff(currentTime - lastTime) : "unknown";
            std::string name    = GetNameForFormID(loverID);

            if (!headerWritten) {
                out << "### Sexual Partners\n";
                headerWritten = true;
            }

            out << "- **" << name << "** (" << bond << "): "
                << total << " encounter" << (total != 1 ? "s" : "")
                << " (" << exclusiveSex << " private, " << groupSex << " group)"
                << ", last " << recency << "\n";
        }

        int insignificantTotal = othersCount + weakCount;
        if (insignificantTotal > 0) {
            if (!headerWritten) {
                out << "### Sexual Partners\n";
            }
            out << "- **Insignificant one-time partners** (fleeting encounters with no emotional weight or lasting connection): "
                << insignificantTotal << (insignificantTotal != 1 ? "s" : "") << "\n";
        }

        return out.str();
    }

}  // namespace

void RegisterSkyrimNetDecorators() {
    if (!PublicRegisterDecorator) {
        SKSE::log::warn("RegisterSkyrimNetDecorators: PublicRegisterDecorator is null — SkyrimNet v5+ required");
        return;
    }

    bool ok = PublicRegisterDecorator(
        "ttll_get_npc_lovers",
        "Returns a formatted list of the NPC's sexual partners with bond strength and encounter stats.",
        GetNpcLoversDecorator);

    if (ok) {
        SKSE::log::info("RegisterSkyrimNetDecorators: 'ttll_get_npc_lovers' registered successfully");
    } else {
        SKSE::log::warn("RegisterSkyrimNetDecorators: failed to register 'ttll_get_npc_lovers' (name conflict?)");
    }

    ok = PublicRegisterDecorator(
        "ttll_get_npc_sexual_behavior",
        "Returns a prose summary of the NPC's sexual orientation and encounter preferences.",
        GetNpcSexualBehaviorDecorator);

    if (ok) {
        SKSE::log::info("RegisterSkyrimNetDecorators: 'ttll_get_npc_sexual_behavior' registered successfully");
    } else {
        SKSE::log::warn("RegisterSkyrimNetDecorators: failed to register 'ttll_get_npc_sexual_behavior' (name conflict?)");
    }
}
