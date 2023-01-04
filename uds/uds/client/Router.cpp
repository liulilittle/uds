#include <uds/client/Router.h>
#include <uds/collections/Dictionary.h>
#include <uds/net/Ipep.h>
#include <uds/net/Socket.h>
#include <uds/net/IPEndPoint.h>
#include <uds/threading/Timer.h>
#include <uds/threading/Hosting.h>
#include <uds/transmission/Transmission.h>
#include <uds/transmission/EncryptorTransmission.h>
#include <uds/transmission/SslSocketTransmission.h>
#include <uds/transmission/WebSocketTransmission.h>
#include <uds/transmission/SslWebSocketTransmission.h>

using uds::collections::Dictionary;
using uds::net::Ipep;
using uds::net::Socket;
using uds::net::IPEndPoint;
using uds::net::AddressFamily;

namespace uds {
    namespace client {
        Router::Router(const std::shared_ptr<uds::threading::Hosting>& hosting, const std::shared_ptr<uds::configuration::AppConfiguration>& configuration) noexcept
            : disposed_(false)
            , channel_(0)
            , hosting_(hosting)
            , configuration_(configuration) {
            if (hosting) {
                context_ = hosting->GetContext();
            }
        }

        bool Router::AddTimeout(void* key, std::shared_ptr<boost::asio::deadline_timer>&& timeout) noexcept {
            if (!key || !timeout) {
                return false;
            }
            else {
                ClearTimeout(key);
            }

            bool success = Dictionary::TryAdd(timeouts_, key, timeout);
            if (!success) {
                uds::threading::ClearTimeout(timeout);
            }
            return success;
        }

        bool Router::ClearTimeout(void* key) noexcept {
            if (!key) {
                return false;
            }

            TimeoutPtr timeout;
            Dictionary::TryRemove(timeouts_, key, timeout);
            return uds::threading::ClearTimeout(timeout);
        }

        void Router::Dispose() noexcept {
            if (!disposed_.exchange(true)) {
                /* Close the TCP socket acceptor function to prevent the system from continuously processing connections. */
                CloseAcceptor();

                /* Clear all timeouts. */
                Dictionary::ReleaseAllPairs(timeouts_,
                    [](TimeoutPtr& timeout) noexcept {
                        uds::threading::Hosting::Cancel(timeout);
                    });

                /* Close all connection. */
                Dictionary::ReleaseAllPairs(connections_);
            }
        }

        void Router::CloseAcceptor() noexcept {
            std::shared_ptr<boost::asio::ip::tcp::acceptor>& acceptor = acceptor_;
            if (acceptor) {
                Socket::Closesocket(*acceptor);
            }

            std::shared_ptr<boost::asio::ip::tcp::resolver>& resolver = resolver_;
            if (resolver) {
                try {
                    resolver->cancel();
                }
                catch (std::exception&) {}
            }

            acceptor.reset();
            resolver.reset();
        }

        bool Router::Listen() noexcept {
            if (disposed_ || !hosting_ || !context_ || !configuration_) {
                return false;
            }

            if (!configuration_->Inbound.Domain) {
                IPEndPoint inboundEP(configuration_->Inbound.IP.data(), configuration_->Inbound.Port);
                if (inboundEP.IsNone()) {
                    return false;
                }
            }

            if (!configuration_->Outbound.Domain) {
                IPEndPoint outboundEP(configuration_->Outbound.IP.data(), configuration_->Outbound.Port);
                if (outboundEP.IsNone()) {
                    return false;
                }
            }

            IPEndPoint localEP(configuration_->IP.data(), configuration_->Port);
            if (IPEndPoint::IsInvalid(localEP)) {
                if (configuration_->Domain || localEP.IsBroadcast()) {
                    return false;
                }
            }

            std::shared_ptr<Reference> references = GetReference();
            acceptor_ = OpenAcceptor(localEP,
                [references, this](const AsioContext& context, const AsioTcpSocket& socket) noexcept {
                    return AcceptClient(context, socket);
                });
            if (acceptor_) {
                resolver_ = make_shared_object<boost::asio::ip::tcp::resolver>(*context_);
                return true;
            }

            CloseAcceptor();
            return false;
        }

        bool Router::ResolveAddress(bool domain, const std::string& hostname, int port, ResolveAddressAsyncCallback&& callback) noexcept {
            if (domain) {
                std::shared_ptr<Reference> references = GetReference();
                ResolveAddressAsyncCallback scallback = callback;
                Ipep::GetAddressByHostName(resolver_, hostname, port,
                    make_shared_object<Ipep::GetAddressByHostNameCallback>(
                        [scallback, references, this](IPEndPoint* ep) noexcept {
                            boost::asio::ip::tcp::endpoint remoteEP;
                            if (ep) {
                                remoteEP = IPEndPoint::ToEndPoint<boost::asio::ip::tcp>(*ep);
                            }
                            scallback(remoteEP);
                        }));
                return true;
            }
            else {
                boost::asio::ip::tcp::endpoint remoteEP = IPEndPoint::ToEndPoint<boost::asio::ip::tcp>(IPEndPoint(hostname.data(), port));
                callback(remoteEP);
                return true;
            }
        }

        bool Router::AcceptClient(const AsioContext& context, const AsioTcpSocket& socket) noexcept {
            static const auto ClearTimeout = [](const TimeoutPtr& timeout) noexcept {
                if (timeout) {
                    uds::threading::ClearTimeout(constantof(timeout));
                }
            };

            const std::shared_ptr<Reference> references = GetReference();
            const AsioTcpSocket network = socket;
            const AsioContext scontext = context;
            const auto timeout = uds::threading::SetTimeout(hosting_,
                [network, references, this](void* key) noexcept {
                    Socket::Closesocket(network);
                }, (UInt64)configuration_->Connect.Timeout * 1000);

            return ResolveAddress(configuration_->Inbound.Domain, configuration_->Inbound.IP, configuration_->Inbound.Port,
                [scontext, timeout, network, references, this](const boost::asio::ip::tcp::endpoint& remoteEP) noexcept {
                    if (!ConnectConnection(scontext, 0, remoteEP,
                        [timeout, network, references, this](const ITransmissionPtr& transmission, int channelId) noexcept {
                            const ITransmissionPtr inbound = transmission;
                            return ResolveAddress(configuration_->Outbound.Domain, configuration_->Outbound.IP, configuration_->Outbound.Port,
                                [channelId, inbound, timeout, network, references, this](const boost::asio::ip::tcp::endpoint& remoteEP) noexcept {
                                    if (!ConnectConnection(inbound->GetContext(), channelId, remoteEP,
                                        [inbound, timeout, network, references, this](const ITransmissionPtr& transmission, int channelId) noexcept {
                                            ITransmissionPtr outbound = transmission;
                                            return Connection::HelloAsync(outbound,
                                                [channelId, inbound, outbound, timeout, network, references, this](bool success) noexcept {
                                                    ClearTimeout(timeout);
                                                    if (success) {
                                                        if (channelId >> 31) {
                                                            success = Accept(network, channelId, outbound, inbound);
                                                        }
                                                        else {
                                                            success = Accept(network, channelId, inbound, outbound);
                                                        }
                                                    }

                                                    if (!success) {
                                                        inbound->Close();
                                                        outbound->Close();
                                                        Socket::Closesocket(network);
                                                    }
                                                });
                                        })) {
                                        ClearTimeout(timeout);
                                        inbound->Close();
                                    }
                                });
                        })) {
                        ClearTimeout(timeout);
                        Socket::Closesocket(network);
                    }
                });
        }

        bool Router::Accept(const AsioTcpSocket& network, int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept {
            if (!network || !inbound || !outbound || !channel) {
                return false;
            }

            /* CHANNEL: C <-> S: RX(outbound) <-> TX(inbound). */
            ConnectionPtr connection = CreateConnection(channel, outbound, inbound);
            if (connection) {
                std::shared_ptr<Reference> references = GetReference();
                connection->DisposedEvent = [references, this](Connection* connection) noexcept {
                    Dictionary::TryRemove(connections_, connection->Id);
                };
                if (connection->Listen(network)) {
                    if (Dictionary::TryAdd(connections_, channel, connection)) {
                        return true;
                    }
                }
                connection->Close();
            }
            return false;
        }

        bool Router::ConnectConnection(const AsioContext& context, int channelId, const boost::asio::ip::tcp::endpoint& remoteEP, ConnectConnectionAsyncCallback&& callback) noexcept {
            const std::shared_ptr<Reference> references = GetReference();
            const ConnectConnectionAsyncCallback scallback = callback;

            return ConnectTransmission(context, remoteEP,
                [channelId, scallback, references, this](const ITransmissionPtr& transmission) noexcept {
                    using ConnectAsyncCallback = Connection::ConnectAsyncCallback;

                    ITransmissionPtr transmission_ = transmission;
                    if (!AddTimeout(transmission_.get(), uds::threading::SetTimeout(hosting_,
                        [references, this, transmission_](void* key) noexcept {
                            ClearTimeout(key);
                            transmission_->Close();
                        }, (UInt64)configuration_->Connect.Timeout * 1000))) {
                        return false;
                    }

                    ConnectAsyncCallback f = [scallback, transmission_, references, this](bool handshaked, int channelId) noexcept {
                        ClearTimeout(transmission_.get());
                        if (handshaked) {
                            handshaked = scallback(transmission_, channelId);
                        }

                        if (!handshaked) {
                            transmission_->Close();
                        }
                    };

                    bool success = false;
                    if (channelId) {
                        success = Connection::ConnectAsync(transmission, configuration_->Alignment, channelId, std::forward<ConnectAsyncCallback>(f));
                    }
                    else {
                        success = Connection::ConnectAsync(transmission, std::forward<ConnectAsyncCallback>(f));
                    }

                    if (!success) {
                        ClearTimeout(transmission_.get());
                    }
                    return success;
                });
        }

        bool Router::ConnectTransmission(const AsioContext& context, const boost::asio::ip::tcp::endpoint& remoteEP, ConnectTransmissionAsyncCallback&& callback) noexcept {
            const std::shared_ptr<Reference> references = GetReference();
            const AsioContext scontext = context;
            const ConnectTransmissionAsyncCallback scallback = callback;

            return ConnectClient(context, remoteEP,
                [scallback, scontext, references, this](const std::shared_ptr<boost::asio::ip::tcp::socket>& network, bool success) noexcept {
                    if (!success) {
                        return false;
                    }

                    const std::shared_ptr<boost::asio::ip::tcp::socket> snetwork = network;
                    return HandshakeTransmission(scontext, network,
                        [snetwork, scallback, references, this](const ITransmissionPtr& transmission, bool handshaked) noexcept {
                            if (handshaked) {
                                handshaked = scallback(transmission);
                            }

                            if (!handshaked) {
                                if (transmission) {
                                    transmission->Close();
                                }
                                Socket::Closesocket(*snetwork);
                            }
                        });
                });
        }

        bool Router::ConnectClient(const AsioContext& context, const boost::asio::ip::tcp::endpoint& remoteEP, ConnectClientAsyncCallback&& callback) noexcept {
            const std::shared_ptr<boost::asio::ip::tcp::socket> network = Connection::NewRemoteSocket(configuration_, context, remoteEP);
            if (!network) {
                return false;
            }

            const std::shared_ptr<Reference> references = GetReference();
            const ConnectClientAsyncCallback scallback = callback;

            if (!AddTimeout(network.get(), uds::threading::SetTimeout(hosting_,
                [references, this, network](void* key) noexcept {
                    ClearTimeout(key);
                    Socket::Closesocket(*network);
                }, (UInt64)configuration_->Connect.Timeout * 1000))) {
                Socket::Closesocket(*network);
                return false;
            }

            network->async_connect(remoteEP,
                [references, this, network, scallback](const boost::system::error_code& ec) noexcept {
                    Router::ClearTimeout(network.get());

                    bool success = ec ? false : true;
                    if (success) {
                        success = scallback(network, success);
                    }

                    if (!success) {
                        Socket::Closesocket(*network);
                    }
                });
            return true;
        }

        std::shared_ptr<boost::asio::ip::tcp::acceptor> Router::OpenAcceptor(const uds::net::IPEndPoint& bindEP, const uds::net::Socket::AcceptLoopbackCallback&& loopback) noexcept {
            int listenPort = bindEP.Port;
            if (listenPort < IPEndPoint::MinPort || listenPort > IPEndPoint::MaxPort) {
                listenPort = IPEndPoint::MinPort;
            }

            boost::asio::ip::address address = IPEndPoint::ToEndPoint<boost::asio::ip::tcp>(bindEP).address();
            std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor = NewReference<boost::asio::ip::tcp::acceptor>(*context_);
            if (!Socket::OpenAcceptor(*acceptor, address, listenPort, configuration_->Backlog, configuration_->FastOpen, configuration_->Turbo)) {
                Socket::Closesocket(*acceptor);
                return NULL;
            }

            if (!Socket::AcceptLoopbackAsync(hosting_, acceptor, BOOST_ASIO_MOVE_CAST(Socket::AcceptLoopbackCallback)(constantof(loopback)))) {
                Socket::Closesocket(*acceptor);
                return NULL;
            }
            return std::move(acceptor);
        }

        const std::shared_ptr<boost::asio::io_context>& Router::GetContext() noexcept {
            return context_;
        }

        const std::shared_ptr<uds::threading::Hosting>& Router::GetHosting() noexcept {
            return hosting_;
        }

        const std::shared_ptr<uds::configuration::AppConfiguration>& Router::GetConfiguration() noexcept {
            return configuration_;
        }

        const boost::asio::ip::tcp::endpoint Router::GetLocalEndPoint() noexcept {
            std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor = acceptor_;
            if (acceptor) {
                boost::system::error_code ec;
                return acceptor->local_endpoint(ec);
            }
            return boost::asio::ip::tcp::endpoint();
        }

        bool Router::HandshakeTransmission(const ITransmissionPtr& transmission, HandshakeAsyncCallback&& callback) noexcept {
            if (!transmission || !callback) {
                return false;
            }

            const std::shared_ptr<Reference> reference = GetReference();
            const std::shared_ptr<uds::transmission::ITransmission> network = transmission;
            const HandshakeAsyncCallback scallback = callback;

            if (!AddTimeout(network.get(), uds::threading::SetTimeout(hosting_,
                [reference, this, network, scallback](void* key) noexcept {
                    ClearTimeout(key);
                    network->Close();
                    scallback(NULL, false);
                }, (UInt64)configuration_->Handshake.Timeout * 1000))) {
                return false;
            }

            return transmission->HandshakeAsync(uds::transmission::ITransmission::HandshakeType_Client, /* In order to extend the transport layer medium. */
                [reference, this, network, scallback](bool handshaked) noexcept {
                    ClearTimeout(network.get());
                    if (handshaked) {
                        scallback(network, handshaked);
                    }
                    else {
                        network->Close();
                        scallback(NULL, handshaked);
                    }
                });
        }

        bool Router::HandshakeTransmission(const AsioContext& context, const AsioTcpSocket& socket, HandshakeAsyncCallback&& callback) noexcept {
            if (!callback) {
                return false;
            }

            const std::shared_ptr<uds::transmission::ITransmission> transmission = CreateTransmission(context, socket);
            if (!transmission) {
                return false;
            }

            return HandshakeTransmission(transmission, std::forward<HandshakeAsyncCallback>(callback));
        }

        Router::ConnectionPtr Router::CreateConnection(int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept {
            if (!channel || !inbound || !outbound) {
                return NULL;
            }

            return NewReference<Connection>(configuration_, channel, inbound, outbound);
        }

        Router::ITransmissionPtr Router::CreateTransmission(const AsioContext& context, const AsioTcpSocket& socket) noexcept {
            std::shared_ptr<uds::transmission::ITransmission> transmission;
            if (configuration_->Protocol == AppConfiguration::ProtocolType_SSL ||
                configuration_->Protocol == AppConfiguration::ProtocolType_TLS) {
                transmission = NewReference2<uds::transmission::ITransmission, uds::transmission::SslSocketTransmission>(hosting_, context, socket,
                    configuration_->Protocols.Ssl.Host,
                    configuration_->Protocols.Ssl.CertificateFile,
                    configuration_->Protocols.Ssl.CertificateKeyFile,
                    configuration_->Protocols.Ssl.CertificateChainFile,
                    configuration_->Protocols.Ssl.CertificateKeyPassword,
                    configuration_->Protocols.Ssl.Ciphersuites,
                    configuration_->Alignment);
            }
            elif(configuration_->Protocol == AppConfiguration::ProtocolType_Encryptor) {
                transmission = NewReference2<uds::transmission::ITransmission, uds::transmission::EncryptorTransmission>(hosting_, context, socket,
                    configuration_->Protocols.Encryptor.Method,
                    configuration_->Protocols.Encryptor.Password,
                    configuration_->Alignment);
            }
            elif(configuration_->Protocol == AppConfiguration::ProtocolType_WebSocket) {
                transmission = NewReference2<uds::transmission::ITransmission, uds::transmission::WebSocketTransmission>(hosting_, context, socket,
                    configuration_->Protocols.WebSocket.Host,
                    configuration_->Protocols.WebSocket.Path,
                    configuration_->Alignment);
            }
            elif(configuration_->Protocol == AppConfiguration::ProtocolType_WebSocket_SSL ||
                configuration_->Protocol == AppConfiguration::ProtocolType_WebSocket_TLS) {
                transmission = NewReference2<uds::transmission::ITransmission, uds::transmission::SslWebSocketTransmission>(hosting_, context, socket,
                    configuration_->Protocols.WebSocket.Host,
                    configuration_->Protocols.WebSocket.Path,
                    configuration_->Protocols.Ssl.CertificateFile,
                    configuration_->Protocols.Ssl.CertificateKeyFile,
                    configuration_->Protocols.Ssl.CertificateChainFile,
                    configuration_->Protocols.Ssl.CertificateKeyPassword,
                    configuration_->Protocols.Ssl.Ciphersuites,
                    configuration_->Alignment);
            }
            else {
                transmission = NewReference2<uds::transmission::ITransmission, uds::transmission::Transmission>(hosting_, context, socket, configuration_->Alignment);
            }
            return transmission->Constructor(transmission);
        }
    }
}