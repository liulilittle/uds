#include <uds/server/Switches.h>
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
using uds::net::Socket;
using uds::net::IPEndPoint;
using uds::net::AddressFamily;

namespace uds {
    namespace server {
        Switches::Switches(const std::shared_ptr<uds::threading::Hosting>& hosting, const std::shared_ptr<uds::configuration::AppConfiguration>& configuration) noexcept
            : disposed_(false)
            , channel_(RandomNext(1, INT_MAX))
            , hosting_(hosting)
            , configuration_(configuration) {
            if (hosting) {
                context_ = hosting->GetContext();
            }
        }

        bool Switches::Listen() noexcept {
            if (disposed_ || !hosting_ || !context_ || !configuration_) {
                return false;
            }

            IPEndPoint inboundEP(configuration_->Inbound.IP.data(), configuration_->Inbound.Port);
            IPEndPoint outboundEP(configuration_->Outbound.IP.data(), configuration_->Outbound.Port);
            if (inboundEP.IsBroadcast() || outboundEP.IsBroadcast()) {
                return false;
            }

            std::shared_ptr<Reference> references = GetReference();
            acceptor_[0] = OpenAcceptor(inboundEP,
                [references, this](const AsioContext& context, const AsioTcpSocket& socket) noexcept {
                    return InboundAcceptClient(context, socket);
                });
            acceptor_[1] = OpenAcceptor(outboundEP,
                [references, this](const AsioContext& context, const AsioTcpSocket& socket) noexcept {
                    return OutboundAcceptClient(context, socket);
                });
            if (acceptor_[0] && acceptor_[1]) {
                return true;
            }

            CloseAcceptor();
            return false;
        }

        void Switches::Dispose() noexcept {
            if (!disposed_.exchange(true)) {
                /* Close the TCP socket acceptor function to prevent the system from continuously processing connections. */
                CloseAcceptor();

                /* Clear all timeouts. */
                Dictionary::ReleaseAllPairs(timeouts_,
                    [](TimeoutPtr& timeout) noexcept {
                        uds::threading::Hosting::Cancel(timeout);
                    });

                /* Close all connection-channels. */
                Dictionary::ReleaseAllPairs(channels_);

                /* Close all connection. */
                Dictionary::ReleaseAllPairs(connections_);
            }
        }

        void Switches::CloseAcceptor() noexcept {
            for (int i = 0, len = arraysizeof(acceptor_); i < len; i++) {
                std::shared_ptr<boost::asio::ip::tcp::acceptor>& acceptor = acceptor_[i];
                if (acceptor) {
                    Socket::Closesocket(*acceptor);
                }
                acceptor.reset();
            }
        }

        bool Switches::InboundAcceptClient(const AsioContext& context, const AsioTcpSocket& socket) noexcept {
            const std::shared_ptr<Reference> references = GetReference();
            const AsioTcpSocket network = socket;
            return HandshakeTransmission(context, socket,
                [references, this, network](const ITransmissionPtr& transmission, bool handshaked) noexcept {
                    const ITransmissionPtr inbound = transmission;
                    if (handshaked) {
                        handshaked = Connection::AcceptAsync(inbound, configuration_->Alignment,
                            [references, this, network](const ITransmissionPtr& inbound) noexcept -> int {
                                ConnectionChannelPtr channel = AllocChannel(network, inbound);
                                if (!channel) {
                                    return 0;
                                }

                                int channelId = channel->channel_;
                                if (!AddTimeout(network.get(), uds::threading::SetTimeout(hosting_,
                                    [references, this, channel, channelId, network](void* key) noexcept -> void {
                                        ClearTimeout(key);
                                        CloseChannel(channelId);
                                    }, (UInt64)configuration_->Connect.Timeout * 1000))) {
                                    channel->Close();
                                    return 0;
                                }
                                return channelId;
                            },
                            [references, this, inbound, network](bool success, int channelId) noexcept -> void {
                                if (!success) { 
                                    ClearTimeout(network.get());
                                    CloseChannel(channelId);
                                }
                            });
                        if (!handshaked) {
                            ClearTimeout(network.get());
                        }
                    }

                    if (!handshaked) {
                        if (transmission) {
                            transmission->Close();
                        }
                    }
                });
        }

        bool Switches::OutboundAcceptClient(const AsioContext& context, const AsioTcpSocket& socket) noexcept {
            const std::shared_ptr<Reference> references = GetReference();
            return HandshakeTransmission(context, socket,
                [references, this](const ITransmissionPtr& transmission, bool handshaked) noexcept {
                    const ITransmissionPtr outbound = transmission;
                    if (handshaked) {
                        handshaked = AddTimeout(outbound.get(), uds::threading::SetTimeout(hosting_,
                            [references, this, outbound](void* key) noexcept -> void {
                                ClearTimeout(key);
                                outbound->Close();
                            }, (UInt64)configuration_->Connect.Timeout * 1000));
                        handshaked = handshaked && Connection::HelloAsync(outbound);
                        handshaked = handshaked && Connection::AcceptAsync(transmission,
                            [references, this, outbound](bool success, int channelId) noexcept -> void {
                                ClearTimeout(outbound.get());
                                if (success) {
                                    success = AcceptChannel(channelId, outbound);
                                }

                                if (!success) {
                                    outbound->Close();
                                }
                            });
                        if (!handshaked) {
                            ClearTimeout(outbound.get());
                        }
                    }

                    if (!handshaked) {
                        transmission->Close();
                    }
                });
        }

        Switches::ConnectionChannelPtr Switches::PopChannel(int channel) noexcept {
            if (!channel) {
                return NULL;
            }

            ConnectionChannelPtr connection;
            if (!Dictionary::TryRemove(channels_, channel, connection)) {
                return NULL;
            }
            return std::move(connection);
        }

        Switches::ConnectionChannelPtr Switches::AllocChannel(const AsioTcpSocket& network, const ITransmissionPtr& inbound) noexcept {
            if (!inbound) {
                return NULL;
            }

            for (;;) {
                int channelId = ++channel_;
                if (channelId < 1) {
                    channel_ = 0;
                    continue;
                }

                if (Dictionary::ContainsKey(channels_, channelId)) {
                    continue;
                }

                ConnectionChannelPtr channel = make_shared_object<ConnectionChannel>();
                if (!channel) {
                    continue;
                }

                if (configuration_->Inversion) {
                    int inversion = RandomNext() & 1;
                    if (inversion) {
                        channelId |= (int)(1 << 31);
                    }
                }
                channel->channel_ = channelId;
                channel->inbound_ = inbound;
                channel->network_ = network;
                channels_[channelId] = channel;
                return std::move(channel);
            }
        }

        bool Switches::CloseChannel(int channel) noexcept {
            ConnectionChannelPtr connection = PopChannel(channel);
            if (!connection) {
                return false;
            }

            connection->Close();
            return true;
        }

        bool Switches::Accept(int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept {
            if (!inbound || !outbound || !channel) {
                return false;
            }

            /* CHANNEL: S <-> C: RX(inbound) <-> TX(outbound). */
            ConnectionPtr connection = CreateConnection(channel, inbound, outbound);
            if (connection) {
                std::shared_ptr<Reference> references = GetReference();
                connection->DisposedEvent = [references, this](Connection* connection) noexcept {
                    Dictionary::TryRemove(connections_, connection->Id);
                };
                if (connection->Listen(NULL)) {
                    if (Dictionary::TryAdd(connections_, channel, connection)) {
                        return true;
                    }
                }
                connection->Close();
            }
            return false;
        }

        bool Switches::AcceptChannel(int channel, const ITransmissionPtr& outbound) noexcept {
            if (!channel || !outbound) {
                return false;
            }

            ConnectionChannelPtr link = PopChannel(channel);
            if (!link) {
                return false;
            }

            bool success = ClearTimeout(link->network_.get());
            if (success) {
                if (channel >> 31) {
                    success = Accept(channel, outbound, link->inbound_);
                }
                else {
                    success = Accept(channel, link->inbound_, outbound);
                }
            }

            if (!success) {
                link->Close();
            }
            return success;
        }

        bool Switches::HandshakeTransmission(const ITransmissionPtr& transmission, HandshakeAsyncCallback&& callback) noexcept {
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

            return transmission->HandshakeAsync(uds::transmission::ITransmission::HandshakeType_Server, /* In order to extend the transport layer medium. */
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

        bool Switches::HandshakeTransmission(const AsioContext& context, const AsioTcpSocket& socket, HandshakeAsyncCallback&& callback) noexcept {
            if (!callback) {
                return false;
            }

            const std::shared_ptr<uds::transmission::ITransmission> transmission = CreateTransmission(context, socket);
            if (!transmission) {
                return false;
            }

            return HandshakeTransmission(transmission, std::forward<HandshakeAsyncCallback>(callback));
        }

        Switches::ITransmissionPtr Switches::CreateTransmission(const AsioContext& context, const AsioTcpSocket& socket) noexcept {
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

        Switches::ConnectionPtr Switches::CreateConnection(int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept {
            if (!channel || !inbound || !outbound) {
                return NULL;
            }

            return NewReference<Connection>(configuration_, channel, inbound, outbound);
        }

        bool Switches::AddTimeout(void* key, std::shared_ptr<boost::asio::deadline_timer>&& timeout) noexcept {
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

        bool Switches::ClearTimeout(void* key) noexcept {
            if (!key) {
                return false;
            }

            TimeoutPtr timeout;
            Dictionary::TryRemove(timeouts_, key, timeout);
            return uds::threading::ClearTimeout(timeout);
        }

        std::shared_ptr<boost::asio::ip::tcp::acceptor> Switches::OpenAcceptor(const uds::net::IPEndPoint& bindEP, const uds::net::Socket::AcceptLoopbackCallback&& loopback) noexcept {
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

        const std::shared_ptr<boost::asio::io_context>& Switches::GetContext() noexcept {
            return context_;
        }

        const std::shared_ptr<uds::threading::Hosting>& Switches::GetHosting() noexcept {
            return hosting_;
        }

        const std::shared_ptr<uds::configuration::AppConfiguration>& Switches::GetConfiguration() noexcept {
            return configuration_;
        }

        const boost::asio::ip::tcp::endpoint Switches::GetLocalEndPoint(bool RX) noexcept {
            std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor = acceptor_[RX ? 0 : 1];
            if (acceptor) {
                boost::system::error_code ec;
                return acceptor->local_endpoint(ec);
            }
            return boost::asio::ip::tcp::endpoint();
        }

        void Switches::ConnectionChannel::Close() noexcept {
            ITransmissionPtr inbound = std::move(inbound_);
            if (inbound) {
                inbound_.reset();
                inbound->Close();
            }

            AsioTcpSocket network = std::move(network_);
            if (network) {
                network_.reset();
                Socket::Closesocket(network);
            }
        }
    }
}