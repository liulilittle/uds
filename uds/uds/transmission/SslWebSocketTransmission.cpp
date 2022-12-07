#include <uds/transmission/SslWebSocketTransmission.h>
#include <uds/transmission/templates/SslSocket.hpp>
#include <uds/transmission/templates/WebSocket.hpp>
#include <uds/net/Socket.h>

namespace uds {
    namespace transmission {
        SslWebSocketTransmission::SslWebSocketTransmission(
            const std::shared_ptr<uds::threading::Hosting>&             hosting,
            const std::shared_ptr<boost::asio::io_context>&             context,
            const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
            bool                                                        verify_peer,
            const std::string&                                          host,
            const std::string&                                          path,
            const std::string&                                          certificate_file,
            const std::string&                                          certificate_key_file,
            const std::string&                                          certificate_chain_file,
            const std::string&                                          certificate_key_password,
            const std::string&                                          ciphersuites,
            int                                                         alignment) noexcept
            : Transmission(hosting, context, socket, alignment)
            , disposed_(false)
            , verify_peer_(verify_peer)
            , host_(host)
            , path_(path)
            , certificate_file_(certificate_file)
            , certificate_key_file_(certificate_key_file)
            , certificate_chain_file_(certificate_chain_file)
            , certificate_key_password_(certificate_key_password)
            , ciphersuites_(ciphersuites) {
            
        }

        SslWebSocketTransmission::SslWebSocketTransmission(
            const std::shared_ptr<uds::threading::Hosting>&             hosting, 
            const std::shared_ptr<boost::asio::io_context>&             context, 
            const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
            bool                                                        verify_peer,
            const std::string&                                          host,
            const std::string&                                          path,
            const std::string&                                          ciphersuites,
            int                                                         alignment) noexcept
            : SslWebSocketTransmission(
                hosting,
                context,
                socket,
                verify_peer,
                host,
                path,
                "",
                "",
                "",
                "",
                ciphersuites,
                alignment) {
        
        }

        SslWebSocketTransmission::SslWebSocketTransmission(
            const std::shared_ptr<uds::threading::Hosting>&             hosting, 
            const std::shared_ptr<boost::asio::io_context>&             context, 
            const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
            const std::string&                                          host,
            const std::string&                                          path,
            const std::string&                                          certificate_file,
            const std::string&                                          certificate_key_file,
            const std::string&                                          certificate_chain_file,
            const std::string&                                          certificate_key_password,
            const std::string&                                          ciphersuites,
            int                                                         alignment) noexcept
            : SslWebSocketTransmission(
                hosting,
                context,
                socket,
                false,
                host,
                path,
                certificate_file,
                certificate_key_file,
                certificate_chain_file,
                certificate_key_password,
                ciphersuites,
                alignment) {
        
        }

        void SslWebSocketTransmission::Dispose() noexcept {
            if (!disposed_.exchange(true)) {
                const std::shared_ptr<Reference> reference = GetReference();
                if (!ssl_websocket_) {
                    Transmission::Dispose();
                }
                else {
                    ssl_websocket_->async_close(boost::beast::websocket::close_code::normal,
                        [reference, this](const boost::system::error_code& ec_) noexcept {
                            SslvTcpSocket* ssl_socket = addressof(ssl_websocket_->next_layer());
                            ssl_socket->async_shutdown(
                                [reference, this, ssl_socket](const boost::system::error_code& ec_) noexcept {
                                    Transmission::Dispose();
                                    uds::net::Socket::Closesocket(ssl_socket->next_layer());
                                });
                        });
                }
            }
        }

        bool SslWebSocketTransmission::HandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept {
            typedef std::shared_ptr<SslvWebSocket> SslvWebSocketPtr;
            if (!callback) {
                return false;
            }

            class AcceptSslvWebSocket final : public uds::transmission::templates::WebSocket<SslvWebSocket> {
            public:
                inline AcceptSslvWebSocket(const std::shared_ptr<SslWebSocketTransmission>& transmission, SslvWebSocket& websocket, std::string& host, std::string& path) noexcept
                    : WebSocket(websocket, host, path)
                    , transmission_(transmission) {

                }

            public:
                virtual void Dispose() noexcept override {
                    std::shared_ptr<SslWebSocketTransmission> transmission = std::move(transmission_);
                    if (transmission) {
                        transmission_.reset();
                        transmission->Close();
                    }
                }

            protected:
                virtual void SetAddress(const std::string& address) noexcept override {
                    if (address.size()) {
                        std::shared_ptr<SslWebSocketTransmission> transmission = transmission_;
                        if (transmission) {
                            IPEndPoint remoteEP = transmission->GetRemoteEndPoint();
                            transmission->SetRemoteEndPoint(IPEndPoint(address.data(), remoteEP.Port));
                        }
                    }
                }

            private:
                std::shared_ptr<SslWebSocketTransmission> transmission_;
            };

            class AsyncSslvWebSocket final : public uds::transmission::templates::SslSocket<SslvWebSocketPtr> {
            public:
                inline AsyncSslvWebSocket(const std::shared_ptr<SslWebSocketTransmission>& transmission,
                    std::shared_ptr<boost::asio::ip::tcp::socket>& tcp_socket,
                    std::shared_ptr<boost::asio::ssl::context>& ssl_context,
                    SslvWebSocketPtr& ssl_websocket,
                    bool& verify_peer,
                    std::string& host,
                    std::string& path,
                    std::string& certificate_file,
                    std::string& certificate_key_file,
                    std::string& certificate_chain_file,
                    std::string& certificate_key_password,
                    std::string& ciphersuites) noexcept
                    : SslSocket(tcp_socket, ssl_context, ssl_websocket, verify_peer, host, certificate_file, certificate_key_file, certificate_chain_file, certificate_key_password, ciphersuites)
                    , path_(path)
                    , transmission_(transmission) {

                }

            public:
                virtual void Dispose() noexcept override {
                    std::shared_ptr<SslWebSocketTransmission> transmission = std::move(transmission_);
                    if (transmission) {
                        transmission_.reset();
                        transmission->Close();
                    }
                }

            protected:
                inline bool  PerformWebSocketHandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept {
                    SslvWebSocketPtr& ssl_websocket_ = GetSslSocket();
                    std::shared_ptr<AcceptSslvWebSocket> accept =
                        NewReference<AcceptSslvWebSocket>(transmission_, *ssl_websocket_, host_, path_);
                    return accept->HandshakeAsync(type, forward0f(callback));
                }
                virtual SSL* GetSslHandle() noexcept override {
                    SslvWebSocketPtr& ssl_websocket_ = GetSslSocket();
                    SslvTcpSocket& ssl_socket_ = ssl_websocket_->next_layer();
                    return ssl_socket_.native_handle();
                }
                virtual bool PerformSslHandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept override {
                    const std::shared_ptr<Reference> reference_ = GetReference();
                    const HandshakeAsyncCallback callback_ = BOOST_ASIO_MOVE_CAST(HandshakeAsyncCallback)(constantof(callback));

                    boost::asio::ssl::stream_base::handshake_type handshakeType = type != HandshakeType::HandshakeType_Client ?
                        boost::asio::ssl::stream_base::server : boost::asio::ssl::stream_base::client;
                    
                    // Perform the SSL handshake.
                    SslvWebSocketPtr& ssl_websocket_ = GetSslSocket();
                    SslvTcpSocket& ssl_socket_ = ssl_websocket_->next_layer();
                    ssl_socket_.async_handshake(handshakeType,
                        [reference_, this, type, callback_](const boost::system::error_code& ec) noexcept {
                            bool success = ec ? false : true;
                            if (success) {
                                success = PerformWebSocketHandshakeAsync(type, forward0f(callback_));
                            }

                            if (!success) {
                                Close();
                                callback_(success);
                            }
                        });
                    return true;
                }
                
            private:
                std::string& path_;
                std::shared_ptr<SslWebSocketTransmission> transmission_;
            };

            std::shared_ptr<SslWebSocketTransmission> transmission = CastReference<SslWebSocketTransmission>(GetReference());
            std::shared_ptr<AsyncSslvWebSocket> accept =
                NewReference<AsyncSslvWebSocket>(transmission, GetSocket(), ssl_context_, ssl_websocket_, verify_peer_, host_, path_, certificate_file_, certificate_key_file_, certificate_chain_file_, certificate_key_password_, ciphersuites_);
            return accept->HandshakeAsync(type, forward0f(callback));
        }

        bool SslWebSocketTransmission::OnWriteAsync(const BOOST_ASIO_MOVE_ARG(pmessage) message) noexcept {
            const std::shared_ptr<Reference> reference = GetReference();
            const pmessage messages = BOOST_ASIO_MOVE_CAST(pmessage)(constantof(message));

            ssl_websocket_->async_write(boost::asio::buffer(messages->packet.get(), messages->packet_size),
                [reference, this, messages](const boost::system::error_code& ec, size_t sz) noexcept {
                    bool success = ec ? false : true;
                    if (!success) {
                        Close();
                    }

                    const WriteAsyncCallback& callback = messages->callback;
                    if (callback) {
                        callback(success);
                    }
                    OnAsyncWrite(true);
                });
            return true;
        }

        bool SslWebSocketTransmission::WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept {
            if (!ssl_websocket_ || !ssl_websocket_->is_open()) {
                return false;
            }

            pmessage messages = Pack(buffer.get(), offset, length, forward0f(callback));
            if (!messages) {
                return false;
            }

            OnAddWriteAsync(forward0f(messages));
            return true;
        }

        bool SslWebSocketTransmission::ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept {
            if (!ssl_websocket_ || !ssl_websocket_->is_open()) {
                return false;
            }

            return Transmission::Unpack(*ssl_websocket_, forward0f(callback));
        }
    }
}