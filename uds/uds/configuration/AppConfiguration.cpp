#include <uds/configuration/AppConfiguration.h>
#include <uds/configuration/Ini.h>
#include <uds/ssl/SSL.h>
#include <uds/net/IPEndPoint.h>
#include <uds/threading/Hosting.h>
#include <uds/cryptography/Encryptor.h>

namespace uds {
    namespace configuration {
        static bool AppConfiguration_VerityEncryptorConfiguration(const AppConfiguration& config) noexcept {
            const std::string& evp_method = config.Protocols.Encryptor.Method;
            const std::string& evp_passwd = config.Protocols.Encryptor.Password;

            if (evp_method.empty()) {
                return false;
            }

            if (evp_passwd.empty()) {
                return false;
            }
            return uds::cryptography::Encryptor::Support(evp_method);
        }

        static bool AppConfiguration_VeritySslConfiguration(const AppConfiguration& config, bool hostVerify) noexcept {
            typedef AppConfiguration::LoopbackMode LoopbackMode;

            const bool& ssl_verify_peer = config.Protocols.Ssl.VerifyPeer;
            const std::string& ssl_host = config.Protocols.Ssl.Host;
            const std::string& ssl_certificate_file = config.Protocols.Ssl.CertificateFile;
            const std::string& ssl_certificate_key_file = config.Protocols.Ssl.CertificateKeyFile;
            const std::string& ssl_certificate_chain_file = config.Protocols.Ssl.CertificateChainFile;
            const std::string& ssl_certificate_key_password = config.Protocols.Ssl.CertificateKeyPassword;
            const std::string& ssl_ciphersuites = config.Protocols.Ssl.Ciphersuites;

            if (hostVerify && ssl_host.empty()) {
                return false;
            }

            if (ssl_ciphersuites.empty()) {
                return false;
            }

            if (config.Mode == LoopbackMode::LoopbackMode_Server) {
                if (!uds::ssl::SSL::VerifySslCertificate(
                    ssl_certificate_file,
                    ssl_certificate_key_file,
                    ssl_certificate_chain_file)) {
                    return false;
                }
            }
            return true;
        }

        static bool AppConfiguration_VerityWebSocketConfiguration(const AppConfiguration& config) noexcept {
            const std::string& websocket_host = config.Protocols.WebSocket.Host;
            const std::string& websocket_path = config.Protocols.WebSocket.Path;
            if (websocket_host.empty()) {
                return false;
            }

            if (websocket_path.empty()) {
                return false;
            }

            if (websocket_path[0] != '/') {
                return false;
            }
            return true;
        }

        static bool AppConfiguration_LoadEncryptorConfiguration(std::shared_ptr<AppConfiguration>& configuration, Ini::Section& section) noexcept {
            std::string& evp_method = configuration->Protocols.Encryptor.Method;
            std::string& evp_passwd = configuration->Protocols.Encryptor.Password;

            evp_method = section["protocol.encryptor.method"];
            evp_passwd = section["protocol.encryptor.password"];

            if (evp_method.empty()) {
                return false;
            }

            if (evp_passwd.empty()) {
                return false;
            }
            return true;
        }

        static bool AppConfiguration_LoadSslConfiguration(std::shared_ptr<AppConfiguration>& configuration, Ini::Section& section) noexcept {
            typedef AppConfiguration::ProtocolType ProtocolType;

            bool& ssl_verify_peer = configuration->Protocols.Ssl.VerifyPeer;
            std::string& ssl_host = configuration->Protocols.Ssl.Host;
            std::string& ssl_certificate_file = configuration->Protocols.Ssl.CertificateFile;
            std::string& ssl_certificate_key_file = configuration->Protocols.Ssl.CertificateKeyFile;
            std::string& ssl_certificate_chain_file = configuration->Protocols.Ssl.CertificateChainFile;
            std::string& ssl_certificate_key_password = configuration->Protocols.Ssl.CertificateKeyPassword;
            std::string& ssl_ciphersuites = configuration->Protocols.Ssl.Ciphersuites;

            if (configuration->Protocol == ProtocolType::ProtocolType_SSL ||
                configuration->Protocol == ProtocolType::ProtocolType_WebSocket_SSL) {
                ssl_verify_peer = section.GetValue<bool>("protocol.ssl.verify-peer");
                ssl_host = section["protocol.ssl.host"];
                ssl_certificate_file = section["protocol.ssl.certificate-file"];
                ssl_certificate_key_file = section["protocol.ssl.certificate-key-file"];
                ssl_certificate_chain_file = section["protocol.ssl.certificate-chain-file"];
                ssl_certificate_key_password = section["protocol.ssl.certificate-key-password"];
                ssl_ciphersuites = section["protocol.ssl.ciphersuites"];
            }
            else {
                ssl_verify_peer = section.GetValue<bool>("protocol.tls.verify-peer");
                ssl_host = section["protocol.tls.host"];
                ssl_certificate_file = section["protocol.tls.certificate-file"];
                ssl_certificate_key_file = section["protocol.tls.certificate-key-file"];
                ssl_certificate_chain_file = section["protocol.tls.certificate-chain-file"];
                ssl_certificate_key_password = section["protocol.tls.certificate-key-password"];
                ssl_ciphersuites = section["protocol.tls.ciphersuites"];
            }

            if (ssl_ciphersuites.empty()) {
                ssl_ciphersuites = uds::ssl::SSL::GetSslCiphersuites();
            }

            return ssl_host.size() > 0;
        }

        static bool AppConfiguration_LoadWebSocketConfiguration(std::shared_ptr<AppConfiguration>& configuration, Ini::Section& section) noexcept {
            typedef AppConfiguration::ProtocolType ProtocolType;
            typedef AppConfiguration::LoopbackMode LoopbackMode;

            std::string& websocket_host = configuration->Protocols.WebSocket.Host;
            std::string& websocket_path = configuration->Protocols.WebSocket.Path;

            websocket_host = section["protocol.websocket.host"];
            websocket_path = section["protocol.websocket.path"];

            if (websocket_host.empty()) {
                return false;
            }
            elif(websocket_path.empty()) {
                websocket_path = "/";
            }
            elif(websocket_path[0] != '/') {
                return false;
            }

            AppConfiguration_LoadSslConfiguration(configuration, section);
            if (AppConfiguration_VeritySslConfiguration(*configuration, false)) {
                configuration->Protocols.Ssl.Host.clear();
            }
            else {
                configuration->Protocols.Ssl.ReleaseAllPairs();
                configuration->Protocol = ProtocolType::ProtocolType_WebSocket;
            }
            return true;
        }

        std::shared_ptr<AppConfiguration> AppConfiguration::LoadIniFile(const std::string& iniFile) noexcept {
            typedef uds::configuration::Ini Ini;
            typedef uds::net::IPEndPoint    IPEndPoint;

            std::shared_ptr<Ini> config = Ini::LoadFile(iniFile);
            if (!config) {
                return NULL;
            }

            std::shared_ptr<AppConfiguration> configuration = make_shared_object<AppConfiguration>();
            if (!configuration) {
                return NULL;
            }

            Ini& ini = *config;
            /* Reading app section */
            {
                Ini::Section& section = ini["app"];
                configuration->Mode = LoopbackMode::LoopbackMode_Client;
                configuration->Alignment = section.GetValue<int>("alignment");
                configuration->Backlog = section.GetValue<int>("backlog");
                configuration->IP = section["ip"];
                configuration->Port = section.GetValue<int>("port");
                configuration->Inbound.IP = section["inbound-ip"];
                configuration->Inbound.Port = section.GetValue<int>("inbound-port");
                configuration->Outbound.IP = section["outbound-ip"];
                configuration->Outbound.Port = section.GetValue<int>("outbound-port");
                configuration->FastOpen = section.GetValue<bool>("fast-open");
                configuration->Turbo = section.GetValue<bool>("turbo");
                configuration->Connect.Timeout = section.GetValue<int>("connect.timeout");
                configuration->Handshake.Timeout = section.GetValue<int>("handshake.timeout");

                IPEndPoint ip(configuration->IP.data(), configuration->Port);
                if (IPEndPoint::IsInvalid(ip)) {
                    configuration->IP = boost::asio::ip::address_v6::any().to_string();
                }
                else {
                    configuration->IP = ip.ToAddressString();
                }

                IPEndPoint inboundIP(configuration->Inbound.IP.data(), configuration->Inbound.Port);
                if (IPEndPoint::IsInvalid(inboundIP)) {
                    configuration->Inbound.IP = boost::asio::ip::address_v6::any().to_string();
                }
                else {
                    configuration->Inbound.IP = inboundIP.ToAddressString();
                }

                IPEndPoint outboundIP(configuration->Outbound.IP.data(), configuration->Outbound.Port);
                if (IPEndPoint::IsInvalid(outboundIP)) {
                    configuration->Outbound.IP = boost::asio::ip::address_v6::any().to_string();
                }
                else {
                    configuration->Outbound.IP = outboundIP.ToAddressString();
                }

                int& connectTimeout = configuration->Connect.Timeout;
                if (connectTimeout < 1) {
                    connectTimeout = 10;
                }

                int& handshakeTimeout = configuration->Connect.Timeout;
                if (handshakeTimeout < 1) {
                    handshakeTimeout = 5;
                }

                int& alignment = configuration->Alignment;
                if (alignment < (UINT8_MAX << 1)) {
                    alignment = (UINT8_MAX << 1);
                }

                int& port = configuration->Port;
                if (port < IPEndPoint::MinPort || port > IPEndPoint::MaxPort) {
                    port = IPEndPoint::MinPort;
                }

                int& inboundPort = configuration->Inbound.Port;
                if (inboundPort < IPEndPoint::MinPort || inboundPort > IPEndPoint::MaxPort) {
                    inboundPort = IPEndPoint::MinPort;
                }

                int& outboundPort = configuration->Outbound.Port;
                if (outboundPort < IPEndPoint::MinPort || outboundPort > IPEndPoint::MaxPort) {
                    outboundPort = IPEndPoint::MinPort;
                }

                std::string mode = section["mode"];
                if (mode.size()) {
                    int ch = tolower(mode[0]);
                    if (ch == 's') {
                        configuration->Mode = LoopbackMode::LoopbackMode_Server;
                    }
                    elif(ch >= '0' && ch <= '9') {
                        configuration->Mode = (LoopbackMode)(ch - '0');
                        if (configuration->Mode < LoopbackMode::LoopbackMode_None || configuration->Mode >= LoopbackMode::LoopbackMode_MaxType) {
                            return NULL;
                        }
                    }
                }

                std::string protocol = section["protocol"];
                std::size_t protocol_size = protocol.size();
                if (protocol_size) {
                    const std::size_t MAX_PCH_COUNT = 2;

                    /* Defines and formats an array of characters for comparison. */
                    int pch[MAX_PCH_COUNT];
                    memset(pch, 0, sizeof(pch));

                    /* The padding protocol is used to compare character arrays. */
                    for (int i = 0, loop = std::min<std::size_t>(protocol_size, MAX_PCH_COUNT); i < loop; i++) {
                        pch[i] = tolower(protocol[i]);
                    }

                    if (pch[0] == 'w') { // websocket
                        std::string subprotocol = ToLower(protocol);
                        if (subprotocol.rfind("tls") != std::string::npos) {
                            configuration->Protocol = ProtocolType::ProtocolType_WebSocket_TLS;
                        }
                        elif(subprotocol.rfind("ssl") != std::string::npos) {
                            configuration->Protocol = ProtocolType::ProtocolType_WebSocket_SSL;
                        }
                        else {
                            configuration->Protocol = ProtocolType::ProtocolType_WebSocket;
                        }
                    }
                    elif(pch[0] == 'e') { // EVP
                        configuration->Protocol = ProtocolType::ProtocolType_Encryptor;
                    }
                    elif(pch[0] == 's') { // SSL
                        configuration->Protocol = ProtocolType::ProtocolType_SSL;
                    }
                    elif(pch[0] == 't' && pch[1] == 'l') { // TLS
                        configuration->Protocol = ProtocolType::ProtocolType_TLS;
                    }
                    elif(pch[0] >= '0' && pch[0] <= '9') { // number optional
                        configuration->Protocol = (ProtocolType)(pch[0] - '0');
                        if (configuration->Protocol < ProtocolType::ProtocolType_None || configuration->Protocol >= ProtocolType::ProtocolType_MaxType) {
                            return NULL;
                        }
                    }
                }

                /* Loading protocol websocket settings. */
                if (configuration->Protocol == ProtocolType::ProtocolType_WebSocket ||
                    configuration->Protocol == ProtocolType::ProtocolType_WebSocket_SSL ||
                    configuration->Protocol == ProtocolType::ProtocolType_WebSocket_TLS) {
                    if (!AppConfiguration_LoadWebSocketConfiguration(configuration, section)) {
                        return NULL;
                    }
                }

                /* Loading protocol ssl/tls settings. */
                if (configuration->Protocol == ProtocolType::ProtocolType_SSL ||
                    configuration->Protocol == ProtocolType::ProtocolType_TLS) {
                    if (!AppConfiguration_LoadSslConfiguration(configuration, section)) {
                        return NULL;
                    }
                }

                /* Loading protocol evp settings. */
                if (configuration->Protocol == ProtocolType::ProtocolType_Encryptor) {
                    if (!AppConfiguration_LoadEncryptorConfiguration(configuration, section)) {
                        return NULL;
                    }
                }

                /* Remove app sections. */
                ini.Remove(section.Name);
            }
            return IsInvalid(configuration) ? NULL : std::move(configuration);
        }

        bool AppConfiguration::IsInvalid(const std::shared_ptr<AppConfiguration>& config) noexcept {
            if (NULL == config) {
                return true;
            }
            return IsInvalid(*config);
        }

        bool AppConfiguration::IsInvalid(const AppConfiguration& config) noexcept {
            typedef uds::net::IPEndPoint                IPEndPoint;

            if (config.Protocol < ProtocolType::ProtocolType_None || config.Protocol >= ProtocolType::ProtocolType_MaxType) {
                return true;
            }
            elif(config.Mode < LoopbackMode::LoopbackMode_None || config.Mode >= LoopbackMode::LoopbackMode_MaxType) {
                return true;
            }
            elif(config.Inbound.Port <= IPEndPoint::MinPort || config.Inbound.Port > IPEndPoint::MaxPort) {
                return true;
            }
            elif(config.Outbound.Port <= IPEndPoint::MinPort || config.Outbound.Port > IPEndPoint::MaxPort) {
                return true;
            }
            elif(config.Port <= IPEndPoint::MinPort || config.Port > IPEndPoint::MaxPort) {
                return true;
            }
            elif(config.Mode == LoopbackMode::LoopbackMode_Client) {
                if (IPEndPoint(config.IP.data(), config.Port).IsBroadcast()) {
                    return false;
                }

                if (IPEndPoint::IsInvalid(IPEndPoint(config.Inbound.IP.data(), config.Inbound.Port))) {
                    return false;
                }

                if (IPEndPoint::IsInvalid(IPEndPoint(config.Outbound.IP.data(), config.Outbound.Port))) {
                    return false;
                }
            }
            else {
                if (IPEndPoint::IsInvalid(IPEndPoint(config.IP.data(), config.Port))) {
                    return false;
                }

                if (IPEndPoint(config.Inbound.IP.data(), config.Inbound.Port).IsBroadcast()) {
                    return false;
                }

                if (IPEndPoint(config.Outbound.IP.data(), config.Outbound.Port).IsBroadcast()) {
                    return false;
                }
            }

            if (config.Protocol == ProtocolType::ProtocolType_WebSocket ||
                config.Protocol == ProtocolType::ProtocolType_WebSocket_SSL) {
                if (!AppConfiguration_VerityWebSocketConfiguration(config)) {
                    return true;
                }
            }
            elif(config.Protocol == ProtocolType::ProtocolType_SSL ||
                config.Protocol == ProtocolType::ProtocolType_TLS) {
                if (!AppConfiguration_VeritySslConfiguration(config, true)) {
                    return true;
                }
            }
            elif(config.Protocol == ProtocolType::ProtocolType_Encryptor) {
                if (!AppConfiguration_VerityEncryptorConfiguration(config)) {
                    return true;
                }
            }

            if (config.Backlog < 1) {
                return true;
            }

            if (config.Alignment < (UINT8_MAX << 1) || config.Alignment > uds::threading::Hosting::BufferSize) {
                return false;
            }

            if (config.Connect.Timeout < 1) {
                return false;
            }

            if (config.Mode == LoopbackMode::LoopbackMode_Server) {
                if (config.Handshake.Timeout < 1) {
                    return false;
                }
            }
            return false;
        }
    }
}