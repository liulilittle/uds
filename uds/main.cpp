#include <uds/configuration/AppConfiguration.h>
#include <uds/io/File.h>
#include <uds/net/IPEndPoint.h>
#include <uds/threading/Hosting.h>
#include <uds/cryptography/Encryptor.h>
#include <uds/client/Router.h>
#include <uds/server/Switches.h>

#ifndef UDS_VERSION
#define UDS_VERSION ("1.0.0.0")
#endif

#ifndef UDS_APPNAME
#define UDS_APPNAME BOOST_BEAST_VERSION_STRING
#endif

using uds::Reference;
using uds::configuration::AppConfiguration;
using uds::net::IPEndPoint;
using uds::io::File;
using uds::io::FileAccess;
using uds::threading::Hosting;

static std::shared_ptr<AppConfiguration>
LoadAppConfiguration(int argc, const char* argv[]) noexcept {
    std::string config_paths[] = { 
        uds::GetCommandArgument("-c", argc, argv),
        uds::GetCommandArgument("--c", argc, argv),
        uds::GetCommandArgument("-conf", argc, argv),
        uds::GetCommandArgument("--conf", argc, argv),
        uds::GetCommandArgument("-config", argc, argv),
        uds::GetCommandArgument("--config", argc, argv),
    };

    const int max_default_file = 10;
    const char* default_files[max_default_file] = {
        config_paths[0].data(), 
        config_paths[1].data(),
        config_paths[2].data(),
        config_paths[3].data(),
        config_paths[4].data(),
        config_paths[5].data(),
        "uds.ini",
        "udsd.ini",
        "udsc.ini", 
        "udss.ini",
    };
    for (int i = 0; i < max_default_file; i++) {
        const char* config_path = default_files[i];
        if (!File::CanAccess(config_path, FileAccess::Read)) {
            continue;
        }

        std::shared_ptr<AppConfiguration> configuration = AppConfiguration::LoadIniFile(config_path);
        if (configuration) {
            return configuration;
        }
    }
    return NULL;
}

int main(int argc, const char* argv[]) noexcept {
#ifdef _WIN32
    SetConsoleTitle(TEXT(UDS_APPNAME));
#endif

    std::shared_ptr<AppConfiguration> configuration = NULL;
    if (!uds::IsInputHelpCommand(argc, argv)) {
        configuration = LoadAppConfiguration(argc, argv);
    }

    if (!configuration) {
        std::string messages_ = "Copyright (C) 2017 ~ 2022 SupersocksR ORG. All rights reserved.\r\n";
        messages_ += "UPSTREAMAND DOWNSTREAM TRAFFIC SEPARATIONS.(X) %s Version\r\n\r\n";
        messages_ += "Cwd:\r\n    " + uds::GetCurrentDirectoryPath() + "\r\n";
        messages_ += "Usage:\r\n";
        messages_ += "    .%s%s -c [config.ini] \r\n";
        messages_ += "Contact us:\r\n";
        messages_ += "    https://t.me/supersocksr_group \r\n";

#ifdef _WIN32
        fprintf(stdout, messages_.data(), UDS_VERSION, "\\", uds::GetExecutionFileName().data());
        system("pause");
#else
        fprintf(stdout, messages_.data(), UDS_VERSION, "/", uds::GetExecutionFileName().data());
#endif
        return 0;
    }

#ifndef _WIN32
    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
#endif

    uds::SetProcessPriorityToMaxLevel();
    uds::SetThreadPriorityToMaxLevel();
    uds::cryptography::Encryptor::Initialize(); /* Prepare the OpenSSL cryptography library environment. */

    std::shared_ptr<Hosting> hosting = Reference::NewReference<Hosting>();
    hosting->Run(
        [configuration, hosting]() noexcept {
            auto protocol = [](AppConfiguration* config) noexcept {
                switch (config->Protocol)
                {
                case AppConfiguration::ProtocolType_SSL:
                    return "ssl";
                case AppConfiguration::ProtocolType_TLS:
                    return "tls";
                case AppConfiguration::ProtocolType_Encryptor:
                    return "encryptor";
                case AppConfiguration::ProtocolType_WebSocket:
                    return "websocket";
                case AppConfiguration::ProtocolType_WebSocket_SSL:
                    return "websocket+ssl";
                case AppConfiguration::ProtocolType_WebSocket_TLS:
                    return "websocket+tls";
                default:
                    return "tcp";
                };
            };
            if (configuration->Mode == AppConfiguration::LoopbackMode_Server) {
                std::shared_ptr<uds::server::Switches> server = Reference::NewReference<uds::server::Switches>(hosting, configuration);
                if (server->Listen()) {
#ifdef _WIN32
                    SetConsoleTitle(TEXT(UDS_APPNAME " -- server"));
#endif
                    fprintf(stdout, "%s\r\nLoopback:\r\n", "Application started. Press Ctrl+C to shut down.");
                    fprintf(stdout, "Mode                  : %s\r\n", "server");
                    fprintf(stdout, "Process               : %d\r\n", uds::GetCurrentProcessId());
                    fprintf(stdout, "Protocol              : %s\r\n", protocol(configuration.get()));
                    fprintf(stdout, "Cwd                   : %s\r\n", uds::GetCurrentDirectoryPath().data());
                    fprintf(stdout, "TCP/IP RX             : %s\r\n", IPEndPoint::ToEndPoint(server->GetLocalEndPoint(true)).ToString().data());
                    fprintf(stdout, "TCP/IP TX             : %s\r\n", IPEndPoint::ToEndPoint(server->GetLocalEndPoint(false)).ToString().data());
                }
                else {
                    exit(0);
                }
            }
            else {
                std::shared_ptr<uds::client::Router> client = Reference::NewReference<uds::client::Router>(hosting, configuration);
                if (client->Listen()) {
#ifdef _WIN32
                    SetConsoleTitle(TEXT(UDS_APPNAME " -- client"));
#endif
                    fprintf(stdout, "%s\r\nLoopback:\r\n", "Application started. Press Ctrl+C to shut down.");
                    fprintf(stdout, "Mode                  : %s\r\n", "client");
                    fprintf(stdout, "Process               : %d\r\n", uds::GetCurrentProcessId());
                    fprintf(stdout, "Protocol              : %s\r\n", protocol(configuration.get()));
                    fprintf(stdout, "Cwd                   : %s\r\n", uds::GetCurrentDirectoryPath().data());
                    fprintf(stdout, "TCP/IP                : %s\r\n", IPEndPoint::ToEndPoint(client->GetLocalEndPoint()).ToString().data());
                }
                else {
                    exit(0);
                }
            }
        });
    return 0;
}
