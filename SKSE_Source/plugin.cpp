#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "LoversLedgerService.h"
#include "PCH.h"
#include "ThreadsCollector.h"
#include "src/Papyrus_LoversLedger.h"
#include "src/Papyrus_ThreadsCollector.h"
#include "src/RelationsFinderAPI.h"

using namespace SKSE;

namespace {
    // Store the RelationsFinder API interface (using callback-based API for DLL safety)
    RelationsFinderAPI::GetNpcRelationshipsCallbackFn g_relationsFinderAPI = nullptr;

    void SetupLogging() {
        auto logDir = SKSE::log::log_directory();
        if (!logDir) {
            if (auto* console = RE::ConsoleLog::GetSingleton()) {
                console->Print("LoversLedger: log directory unavailable");
            }
            return;
        }

        std::filesystem::path logPath = *logDir;
        if (!std::filesystem::is_directory(logPath)) {
            logPath = logPath.parent_path();
        }
        logPath /= "LoversLedger.log";

        std::error_code ec;
        std::filesystem::create_directories(logPath.parent_path(), ec);
        if (ec) {
            if (auto* console = RE::ConsoleLog::GetSingleton()) {
                console->Print("LoversLedger: failed to create log folder (%s)", ec.message().c_str());
            }
            return;
        }

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
        auto logger = std::make_shared<spdlog::logger>("LoversLedger", std::move(sink));
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::info);
        logger->set_pattern("[%H:%M:%S] [%l] %v");

        spdlog::set_default_logger(std::move(logger));
        spdlog::info("Logging to {}", logPath.string());
    }

    void PrintToConsole(std::string_view message) {
        SKSE::log::info("{}", message);
        if (auto* console = RE::ConsoleLog::GetSingleton()) {
            console->Print("%s", message.data());
        }
    }
}

SKSEPluginLoad(const LoadInterface* skse) {
    SKSE::Init(skse);

    SetupLogging();
    SKSE::log::info("LoversLedger plugin loading...");

    // Register Papyrus functions
    if (auto papyrus = SKSE::GetPapyrusInterface()) {
        if (!papyrus->Register(RegisterLoversLedgerPapyrus)) {
            SKSE::log::error("Failed to register LoversLedger Papyrus functions");
        } else {
            SKSE::log::info("LoversLedger Papyrus functions registered");
        }

        if (!papyrus->Register(RegisterThreadsCollectorPapyrus)) {
            SKSE::log::error("Failed to register ThreadsCollector Papyrus functions");
        } else {
            SKSE::log::info("ThreadsCollector Papyrus functions registered");
        }
    } else {
        SKSE::log::warn("Papyrus interface unavailable; papyrus functions not registered");
    }

    if (const auto* messaging = SKSE::GetMessagingInterface()) {
        if (!messaging->RegisterListener([](SKSE::MessagingInterface::Message* message) {
                switch (message->type) {
                    case SKSE::MessagingInterface::kPreLoadGame:
                        SKSE::log::info("PreLoadGame...");
                        break;

                    case SKSE::MessagingInterface::kPostLoadGame:
                    case SKSE::MessagingInterface::kNewGame:
                        SKSE::log::info("New game/Load...");
                        break;

                    case SKSE::MessagingInterface::kDataLoaded: {
                        SKSE::log::info("Data loaded successfully.");
                        if (auto* console = RE::ConsoleLog::GetSingleton()) {
                            console->Print("LoversLedger: Ready");
                        }

                        // Query RelationsFinder API via exported function
                        SKSE::log::info("Querying RelationsFinder API...");
                        HMODULE handle = GetModuleHandleA("RelationsFinder.dll");
                        if (handle) {
                            auto requestAPI = reinterpret_cast<RelationsFinderAPI::RequestAPIFunction>(
                                GetProcAddress(handle, "RequestRelationsFinderAPI"));
                            if (requestAPI) {
                                auto* api = requestAPI();
                                if (api && api->version == RelationsFinderAPI::kAPIVersion) {
                                    // Use the new safe callback API instead of the deprecated vector-returning one
                                    g_relationsFinderAPI = api->GetNpcRelationshipsCallback;
                                    SKSE::log::info("RelationsFinder callback API acquired successfully (version {})",
                                                    api->version);
                                    LL::LoversLedgerService::GetSingleton().SetRelationsFinderAPI(g_relationsFinderAPI);
                                } else {
                                    SKSE::log::warn(
                                        "RelationsFinder API version mismatch or null (expected: {}, got: {})",
                                        RelationsFinderAPI::kAPIVersion, api ? api->version : 0);
                                }
                            } else {
                                SKSE::log::warn("RequestRelationsFinderAPI function not found");
                            }
                        } else {
                            SKSE::log::warn("RelationsFinder.dll not loaded");
                        }
                        break;
                    }

                    default:
                        break;
                }
            })) {
            SKSE::log::critical("Failed to register messaging listener.");
            return false;
        }
    } else {
        SKSE::log::critical("Messaging interface unavailable.");
        return false;
    }

    // Register SKSE serialization callbacks for save/load
    if (const auto* serialization = SKSE::GetSerializationInterface()) {
        serialization->SetUniqueID(LL::kSerializationType);
        serialization->SetSaveCallback([](SKSE::SerializationInterface* a_intfc) {
            LL::LoversLedgerService::GetSingleton().SaveCallback(a_intfc);
        });
        serialization->SetLoadCallback([](SKSE::SerializationInterface* a_intfc) {
            LL::LoversLedgerService::GetSingleton().LoadCallback(a_intfc);
        });
        serialization->SetRevertCallback([](SKSE::SerializationInterface* a_intfc) {
            LL::LoversLedgerService::GetSingleton().RevertCallback(a_intfc);
        });
        SKSE::log::info("LoversLedger serialization callbacks registered");
    } else {
        SKSE::log::warn("Serialization interface unavailable; data will not persist across saves.");
    }

    return true;
}
