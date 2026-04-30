#pragma once

#include <unordered_set>

#include "PCH.h"

namespace LL {

    class UniqueOverrides {
    public:
        static UniqueOverrides& GetSingleton() noexcept;

        // Scan Data/SKSE/Plugins/LoversLedger/UniqueOverride/*.ini and populate the override set.
        // Must be called after kDataLoaded so form lookups succeed.
        void Load();

        [[nodiscard]] bool IsOverridden(std::uint32_t baseFormID) const noexcept;

    private:
        UniqueOverrides() = default;

        void LoadFile(const std::filesystem::path& path);

        std::unordered_set<std::uint32_t> _overrides;
    };

    // Returns the stable, plugin-defined TESNPC base for an actor.
    // Resolves through the runtime-merged face NPC chain so leveled NPCs return
    // their original base (e.g. dunPOIWitchAnise) rather than the FF-prefixed copy.
    // Returns nullptr if the actor has no base.
    [[nodiscard]] RE::TESNPC* GetStableBase(RE::Actor* actor) noexcept;

    // Returns true if the NPC is unique according to game data OR a user config override.
    [[nodiscard]] bool IsEffectivelyUnique(RE::TESNPC* base) noexcept;

}  // namespace LL
