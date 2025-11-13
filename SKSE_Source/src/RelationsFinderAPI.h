#pragma once

#include <vector>

#include "../PCH.h"

namespace RelationsFinderAPI {

    // Custom message type for RelationsFinder API
    constexpr const char* kPluginName = "RelationsFinder";
    constexpr std::uint32_t kMessageType_Query = 0x524649;  // 'RFI' in hex

    // Function pointer type for GetNpcRelationships (DEPRECATED - unsafe across DLLs)
    using GetRelationshipsFn = std::vector<RE::Actor*> (*)(RE::Actor* npc, const char* associationType,
                                                           const char* hierarchy, std::int32_t minRelationshipRank,
                                                           std::int32_t exactRelationshipRank);

    // Callback function type for receiving results (SAFE across DLLs)
    using RelationshipCallbackFn = void (*)(RE::Actor* actor, void* userData);

    // New safe API function that uses callbacks instead of returning vectors
    using GetNpcRelationshipsCallbackFn = void (*)(RE::Actor* npc, const char* associationType, const char* hierarchy,
                                                   std::int32_t minRelationshipRank, std::int32_t exactRelationshipRank,
                                                   RelationshipCallbackFn callback, void* userData);

    // API Interface structure exposed to other plugins
    struct APIInterface {
        std::uint32_t version;                   // API version
        GetRelationshipsFn GetNpcRelationships;  // DEPRECATED - use GetNpcRelationshipsCallback instead
        GetNpcRelationshipsCallbackFn GetNpcRelationshipsCallback;  // NEW - safe across DLLs
    };

    // API version
    constexpr std::uint32_t kAPIVersion = 2;  // Bumped to version 2 for new callback API

    // Direct API request function type
    using RequestAPIFunction = const APIInterface* (*)();

}  // namespace RelationsFinderAPI
