#include "FirstTimes.h"

#include <algorithm>
#include <cctype>

#include "LoversLedgerService.h"
#include "UniqueOverrides.h"

// Forward-declare only the pointer we use from SkyrimNetPublicAPI.h (C linkage, as in the header).
extern "C" {
    extern std::string (*PublicGetPluginConfigValue)(const char* pluginName, const char* path,
                                                     const char* defaultValue);
}

namespace LL {

    namespace {

        // SkyrimNet dashboard: SKSE/Plugins/SkyrimNet/config/plugins/LoversLedger/manifest.yaml
        bool VirginBloodEnabled() {
#ifdef _DEBUG
            return false;  // the debug CRT must not take a std::string from SkyrimNet
#else
            if (!PublicGetPluginConfigValue) return false;
            std::string value =
                PublicGetPluginConfigValue("LoversLedger", "loversledger.first_time.virgin_blood", "false");
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value == "true" || value == "1" || value == "yes";
#endif
        }

        std::string Lower(std::string_view str) {
            std::string out(str);
            std::transform(out.begin(), out.end(), out.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return out;
        }

        bool HasTag(std::string_view tags, std::string_view tag) {
            std::size_t start = 0;
            while (start <= tags.size()) {
                std::size_t end = tags.find(',', start);
                if (end == std::string_view::npos) end = tags.size();
                std::string_view item = tags.substr(start, end - start);
                while (!item.empty() && std::isspace(static_cast<unsigned char>(item.front()))) item.remove_prefix(1);
                while (!item.empty() && std::isspace(static_cast<unsigned char>(item.back()))) item.remove_suffix(1);
                if (Lower(item) == tag) return true;
                start = end + 1;
            }
            return false;
        }

        // SexLab's anatomy when the caller gave it (gender faction override, P+ futa), else the base sex
        int Anatomy(const FirstTimePosition& pos) {
            auto* base = pos.actor ? pos.actor->GetActorBase() : nullptr;
            return AnatomyOrBaseSex(pos.anatomy, base && base->GetSex() == RE::SEXES::kMale);
        }

        bool HasPenis(const FirstTimePosition& pos) { return (Anatomy(pos) & kAnatomyPenis) != 0; }
        bool HasVagina(const FirstTimePosition& pos) { return (Anatomy(pos) & kAnatomyVagina) != 0; }

        bool IsPerson(RE::TESRace* race) { return !race || race->HasKeywordString("ActorTypeNPC"); }

        std::string RaceName(RE::TESRace* race) {
            const char* name = race ? race->GetName() : nullptr;
            return (name && *name) ? name : "";
        }

        std::string WithArticle(const std::string& noun) {
            const char first = static_cast<char>(std::tolower(static_cast<unsigned char>(noun.front())));
            const bool vowel = first == 'a' || first == 'e' || first == 'i' || first == 'o' || first == 'u';
            return (vowel ? "an " : "a ") + noun;
        }

        // "a", "a and b", "a, b, and c"
        std::string JoinList(const std::vector<std::string>& items) {
            std::string out;
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i > 0) out += (items.size() == 2) ? " and " : (i + 1 == items.size() ? ", and " : ", ");
                out += items[i];
            }
            return out;
        }

        // 2 = couple, 3 threesome, 4 foursome, 5+ orgy
        int GroupBucket(int size) { return std::min(size, 5); }

        const char* GroupClause(int size) {
            switch (GroupBucket(size)) {
                case 3: return "in a threesome";
                case 4: return "in a foursome";
                default: return "in an orgy";
            }
        }

        // "with a man", "with a Nord", "in a threesome" for what the actor at index has never had
        std::vector<std::string> PartnerClauses(const std::vector<FirstTimePosition>& positions, std::size_t index,
                                                const PartnerTraits& traits) {
            std::vector<std::string> clauses;

            std::uint8_t sceneGenders = 0;
            std::vector<std::string> knownRaces;
            for (auto raceID : traits.races) {
                knownRaces.push_back(Lower(RaceName(RE::TESForm::LookupByID<RE::TESRace>(raceID))));
            }
            std::vector<std::string> newRaces;
            for (std::size_t j = 0; j < positions.size(); ++j) {
                RE::Actor* other = positions[j].actor;
                if (j == index || !other) continue;
                auto* race = other->GetRace();
                sceneGenders |= PartnerGenderBit(race, Anatomy(positions[j]));

                // By name: a vampire Nord is still a Nord
                std::string name = RaceName(race);
                if (name.empty()) continue;
                if (!IsPerson(race)) name = Lower(name);
                const std::string key = Lower(name);
                if (std::find(knownRaces.begin(), knownRaces.end(), key) != knownRaces.end()) continue;
                knownRaces.push_back(key);
                newRaces.push_back(WithArticle(name));
            }

            if (!traits.unknownPartners) {
                const std::uint8_t newGenders = sceneGenders & ~traits.genders;
                std::vector<std::string> genders;
                if (newGenders & kPartnerMale) genders.emplace_back("a man");
                if (newGenders & kPartnerFemale) genders.emplace_back("a woman");
                if (newGenders & kPartnerFuta) genders.emplace_back("a futa");
                if (!genders.empty()) clauses.push_back("with " + JoinList(genders));
                if (!newRaces.empty()) clauses.push_back("with " + JoinList(newRaces));
            }

            const int size = static_cast<int>(positions.size());
            if (size >= 3 && traits.maxGroupSize != kGroupSizeUnknown &&
                GroupBucket(size) > GroupBucket(traits.maxGroupSize)) {
                clauses.emplace_back(GroupClause(size));
            }
            return clauses;
        }

    }  // namespace

    std::string DescribeFirstTimes(const std::vector<FirstTimePosition>& positions, std::string_view tags) {
        if (positions.size() < 2) return "";

        // SexLab roles: position 0 receives; givers are 1+ with a penis, else position 1 (strap-on)
        const bool analScene = HasTag(tags, "anal");
        const bool vaginalScene = HasTag(tags, "vaginal") && HasVagina(positions[0]);
        std::vector<std::size_t> givers;
        for (std::size_t i = 1; i < positions.size(); ++i) {
            if (HasPenis(positions[i])) givers.push_back(i);
        }
        if (givers.empty()) givers.push_back(1);

        const bool blood = VirginBloodEnabled();
        auto& ledger = LoversLedgerService::GetSingleton();
        std::vector<std::string> lines;

        for (std::size_t i = 0; i < positions.size(); ++i) {
            const auto& pos = positions[i];
            auto* base = GetStableBase(pos.actor);
            if (!base || !IsEffectivelyUnique(base)) continue;

            const FirstTimeHistory history = ledger.GetFirstTimeHistory(base->GetFormID());
            const char* displayName = pos.actor->GetDisplayFullName();
            const std::string name = (displayName && *displayName) ? displayName : base->GetName();
            const bool isGiver = std::find(givers.begin(), givers.end(), i) != givers.end();

            // SexLab counts both sides of an act; more than the ledger holds is history of unknown direction
            const bool vaginalUnknown = pos.slVaginal > history.vaginalDid + history.vaginalGot;
            const bool analUnknown = pos.slAnal > history.analDid + history.analGot;
            const bool firstVaginalGot = vaginalScene && i == 0 && history.vaginalGot == 0 && !vaginalUnknown;
            const bool firstAnalGot = analScene && i == 0 && history.analGot == 0 && !analUnknown;
            const bool firstVaginalDid = vaginalScene && isGiver && history.vaginalDid + history.vaginalGot == 0 &&
                                         !vaginalUnknown;
            const bool firstAnalDid = analScene && isGiver && history.analDid == 0 && !analUnknown;

            const std::string vaginaBlood = name + "'s vagina is a little sore and is bleeding.";
            const std::string assBlood = name + "'s asshole is a little sore and is bleeding.";

            std::vector<std::string> sentences;
            if (std::max(history.partnered, pos.slTotal) == 0) {
                sentences.push_back("This is the first time " + name + " has had sex.");
                if (blood && firstVaginalGot) sentences.push_back(vaginaBlood);
                if (blood && firstAnalGot) sentences.push_back(assBlood);
            } else {
                // SexLab history the ledger never attributed: its partners are unknown
                if (pos.slTotal <= history.partnered) {
                    auto clauses = PartnerClauses(positions, i, history.traits);
                    if (!clauses.empty()) {
                        sentences.push_back("This is the first time " + name + " has had sex " + JoinList(clauses) +
                                            ".");
                    }
                }
                if (firstVaginalGot) {
                    sentences.push_back("This is the first time " + name + " has had vaginal sex.");
                    if (blood) sentences.push_back(vaginaBlood);
                }
                if (firstAnalGot) {
                    sentences.push_back("This is the first time " + name + " has received anal sex.");
                    if (blood) sentences.push_back(assBlood);
                }
                if (firstVaginalDid) {
                    sentences.push_back("This is the first time " + name + " has had vaginal sex.");
                }
                if (firstAnalDid) {
                    sentences.push_back("This is the first time " + name + " has given anal sex.");
                }
            }
            if (sentences.empty()) continue;

            std::string line;
            for (const auto& sentence : sentences) {
                if (!line.empty()) line += ' ';
                line += sentence;
            }
            SKSE::log::debug("DescribeFirstTimes: 0x{:X} (base 0x{:X}): {}", pos.actor->GetFormID(),
                             base->GetFormID(), line);
            lines.push_back(std::move(line));
        }

        std::string out;
        for (const auto& line : lines) {
            if (!out.empty()) out += '\n';
            out += line;
        }
        return out;
    }

}  // namespace LL
