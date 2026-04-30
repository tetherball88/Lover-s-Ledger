#include "UniqueOverrides.h"

#include <fstream>
#include <string>

namespace LL {

    UniqueOverrides& UniqueOverrides::GetSingleton() noexcept {
        static UniqueOverrides instance;
        return instance;
    }

    bool UniqueOverrides::IsOverridden(std::uint32_t baseFormID) const noexcept {
        return _overrides.contains(baseFormID);
    }

    RE::TESNPC* GetStableBase(RE::Actor* actor) noexcept {
        if (!actor) return nullptr;
        auto* base = actor->GetActorBase();
        if (!base) return nullptr;
        // Runtime-merged NPCs (leveled actors) get an FF-prefixed form ID each session.
        // Their faceNPC points one step back to the original plugin-defined NPC.
        // We follow faceNPC exactly once — NOT recursively — so that a face patch applied
        // to the plugin-defined NPC by a mod like SkyPatcher (which sets faceNPC on the
        // plugin NPC itself) does not cause us to resolve to the donor NPC instead.
        if ((base->GetFormID() >> 24) == 0xFF) {
            return base->faceNPC ? base->faceNPC : base;
        }
        return base;
    }

    bool IsEffectivelyUnique(RE::TESNPC* base) noexcept {
        return base && (base->IsUnique() || UniqueOverrides::GetSingleton().IsOverridden(base->GetFormID()));
    }

    namespace {
        std::string Trim(std::string_view sv) {
            const auto start = sv.find_first_not_of(" \t\r\n");
            if (start == std::string_view::npos) return {};
            const auto end = sv.find_last_not_of(" \t\r\n");
            return std::string(sv.substr(start, end - start + 1));
        }
    }

    void UniqueOverrides::LoadFile(const std::filesystem::path& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            SKSE::log::warn("UniqueOverrides: failed to open '{}'", path.string());
            return;
        }

        bool inUniqueNPCsSection = false;
        int addedFromFile = 0;
        std::string line;

        while (std::getline(file, line)) {
            const std::string trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;

            // Section header
            if (trimmed[0] == '[') {
                const auto close = trimmed.find(']');
                if (close != std::string::npos) {
                    inUniqueNPCsSection = (Trim(trimmed.substr(1, close - 1)) == "UniqueNPCs");
                }
                continue;
            }

            if (!inUniqueNPCsSection) continue;

            // Key=value — only the key matters
            const auto eq = trimmed.find('=');
            const std::string key = Trim(eq != std::string::npos ? trimmed.substr(0, eq) : trimmed);
            if (key.empty()) continue;

            RE::TESNPC* npc = nullptr;

            const auto colon = key.find(':');
            if (colon != std::string::npos) {
                // Plugin:LocalFormID format  e.g. "Skyrim.esm:0x000638DF"
                const std::string pluginName = Trim(key.substr(0, colon));
                const std::string localIDStr = Trim(key.substr(colon + 1));
                try {
                    const std::uint32_t localID = std::stoul(localIDStr, nullptr, 0);
                    if (auto* handler = RE::TESDataHandler::GetSingleton()) {
                        npc = handler->LookupForm<RE::TESNPC>(localID, pluginName);
                    }
                } catch (const std::exception&) {
                    SKSE::log::warn("UniqueOverrides: bad FormID '{}' in '{}'", key, path.filename().string());
                    continue;
                }
            } else {
                // EditorID format  e.g. "Illia"
                auto* form = RE::TESForm::LookupByEditorID(key);
                npc = form ? form->As<RE::TESNPC>() : nullptr;
            }

            if (npc) {
                _overrides.insert(npc->GetFormID());
                ++addedFromFile;
                SKSE::log::debug("UniqueOverrides: '{}' resolved to base 0x{:X} ({})",
                                 key, npc->GetFormID(), npc->GetName());
            } else {
                SKSE::log::warn("UniqueOverrides: could not resolve '{}' in '{}' "
                                "(make sure you are using the TESNPC base form EditorID/FormID, not an Actor reference ID)",
                                key, path.filename().string());
            }
        }

        SKSE::log::info("UniqueOverrides: loaded {} override(s) from '{}'",
                        addedFromFile, path.filename().string());
    }

    void UniqueOverrides::Load() {
        _overrides.clear();

        const std::filesystem::path dir = "Data/SKSE/Plugins/LoversLedger/UniqueOverride";

        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec)) {
            SKSE::log::info("UniqueOverrides: directory '{}' not found, no overrides loaded", dir.string());
            return;
        }

        int fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".ini") continue;
            LoadFile(entry.path());
            ++fileCount;
        }

        SKSE::log::info("UniqueOverrides: processed {} file(s), {} total override(s)",
                        fileCount, _overrides.size());
    }

}  // namespace LL
