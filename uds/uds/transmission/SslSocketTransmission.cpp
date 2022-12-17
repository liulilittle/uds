#include <uds/transmission/SslSocketTransmission.h>
#include <uds/transmission/templates/SslSocket.hpp>
#include <uds/net/Socket.h>

namespace uds {
    namespace transmission {
        SslSocketTransmission::SslSocketTransmission(
            const std::shared_ptr<uds::threading::Hosting>&         hosting, 
            const std::shared_ptr<boost::asio::io_context>&         context, 
            const std::shared_ptr<boost::asio::ip::tcp::socket>&    socket,
            bool                                                    verify_peer,
            const std::string&                                      host,
            const std::string&                                      certificate_file,
            const std::string&                                      certificate_key_file,
            const std::string&                                      certificate_chain_file,
            const std::string&                                      certificate_key_password,
            const std::string&                                      ciphersuites,
            int                                                     alignment) noexcept
            : Transmission(hosting, context, socket, alignment)
            , disposed_(false)
            , verify_peer_(verify_peer)
            , host_(host)
            , certificate_file_(certificate_file)
            , certificate_key_file_(certificate_key_file)
            , certificate_chain_file_(certificate_chain_file)
            , certificate_key_password_(certificate_key_password)
            , ciphersuites_(ciphersuites) {

        }

        SslSocketTransmission::SslSocketTransmission(
            const std::shared_ptr<uds::threading::Hosting>&         hosting, 
            const std::shared_ptr<boost::asio::io_context>&         context, 
            const std::shared_ptr<boost::asio::ip::tcp::socket>&    socket,
            bool                                                    verify_peer,
            const std::string&                                      host,
            const std::string&                                      ciphersuites,
            int                                                     alignment) noexcept
            : SslSocketTransmission(
                hosting, 
                context, 
                socket, 
                verify_peer, 
                host, 
                "", 
                "", 
                "", 
                "", 
                ciphersuites,
                alignment) {
            
        }
                                                                    
        SslSocketTransmission::SslSocketTransmission(               
            const std::shared_ptr<uds::threading::Hosting>&         hosting, 
            const std::shared_ptr<boost::asio::io_context>&         context, 
            const std::shared_ptr<boost::asio::ip::tcp::socket>&    socket,
            const std::string&                                      host,
            const std::string&                                      certificate_file,
            const std::string&                                      certificate_key_file,
            const std::string&                                      certificate_chain_file,
            const std::string&                                      certificate_key_password,
            const std::string&                                      ciphersuites,
            int                                                     alignment) noexcept
            : SslSocketTransmission(
                hosting,
                context, 
                socket,
                false, 
                host, 
                certificate_file, 
                certificate_key_file, 
                certificate_chain_file,
                certificate_key_password,
                ciphersuites,
                alignment) {
        
        }

        bool SslSocketTransmission::OnWriteAsync(const BOOST_ASIO_MOVE_ARG(pmessage) message) noexcept {
            const std::shared_ptr<ITransmission> reference = GetReference();
            const pmessage messages = BOOST_ASIO_MOVE_CAST(pmessage)(constantof(message));

            boost::asio::async_write(*ssl_socket_, boost::asio::buffer(messages->packet.get(), messages->packet_size),
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

        void SslSocketTransmission::Dispose() noexcept {
            if (!disposed_.exchange(true)) {
                const std::shared_ptr<SslSocket> ssl_socket = ssl_socket_;
                if (!ssl_socket) {
                    Transmission::Dispose();
                }
                else {
                    const std::shared_ptr<ITransmission> reference = GetReference();
                    ssl_socket->async_shutdown(
                        [reference, this, ssl_socket](const boost::system::error_code& ec_) noexcept {
                            Transmission::Dispose();
                            uds::net::Socket::Closesocket(ssl_socket->next_layer());
                        });
                }
            }
        }

        bool SslSocketTransmission::HandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept {
            typedef std::shared_ptr<SslSocket> SslSocketPtr;
            if (!callback) {
                return false;
            }

            class AsyncSslSocket final : public uds::transmission::templates::SslSocket<SslSocketPtr> {
            public:
                inline AsyncSslSocket(const std::shared_ptr<SslSocketTransmission>& transmission,
                    std::shared_ptr<boost::asio::ip::tcp::socket>& tcp_socket,
                    std::shared_ptr<boost::asio::ssl::context>& ssl_context,
                    SslSocketPtr& ssl_socket,
                    bool& verify_peer,
                    std::string& host,
                    std::string& certificate_file,
                    std::string& certificate_key_file,
                    std::string& certificate_chain_file,
                    std::string& certificate_key_password,
                    std::string& ciphersuites) noexcept
                    : SslSocket(tcp_socket, ssl_context, ssl_socket, verify_peer, host, certificate_file, certificate_key_file, certificate_chain_file, certificate_key_password, ciphersuites)
                    , transmission_(transmission) {

                }

            public:
                virtual void Dispose() noexcept override {
                    std::shared_ptr<SslSocketTransmission> transmission = std::move(transmission_);
                    if (transmission) {
                        transmission_.reset();
                        transmission->Close();
                    }
                }

            protected:
                virtual SSL* GetSslHandle() noexcept override {
                    return GetSslSocket()->native_handle();
                }
                virtual bool PerformSslHandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept override {
                    const std::shared_ptr<Reference> reference_ = GetReference();
                    const HandshakeAsyncCallback callback_ = BOOST_ASIO_MOVE_CAST(HandshakeAsyncCallback)(constantof(callback));

                    // Perform the SSL handshake.
                    boost::asio::ssl::stream_base::handshake_type handshakeType = type != HandshakeType::HandshakeType_Client ?
                        boost::asio::ssl::stream_base::server : boost::asio::ssl::stream_base::client;
                    GetSslSocket()->async_handshake(handshakeType,
                        [reference_, this, callback_](const boost::system::error_code& ec) noexcept {
                            bool success = ec ? false : true;
                            if (!success) {
                                Close();
                            }

                            callback_(success);
                        });
                    return true;
                }

            private:
                std::shared_ptr<SslSocketTransmission> transmission_;
            };

            std::shared_ptr<SslSocketTransmission> transmission = Reference::CastReference<SslSocketTransmission>(GetReference());
            std::shared_ptr<AsyncSslSocket> accept =
                Reference::NewReference<AsyncSslSocket>(transmission, GetSocket(), ssl_context_, ssl_socket_, verify_peer_, host_, certificate_file_, certificate_key_file_, certificate_chain_file_, certificate_key_password_, ciphersuites_);
            return accept->HandshakeAsync(type, forward0f(callback));
        }
        
        bool SslSocketTransmission::WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept {
            if (!ssl_socket_) {
                return false;
            }
            
            boost::asio::ip::tcp::socket& nativeSocket = ssl_socket_->next_layer();
            if (!nativeSocket.is_open()) {
                return false;
            }

            pmessage messages = Pack(buffer.get(), offset, length, forward0f(callback));
            if (!messages) {
                return false;
            }

            OnAddWriteAsync(forward0f(messages));
            return true;
        }
       
        bool SslSocketTransmission::ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept {
            if (!ssl_socket_) {
                return false;
            }

            boost::asio::ip::tcp::socket& nativeSocket = ssl_socket_->next_layer();
            if (!nativeSocket.is_open()) {
                return false;
            }

            return Transmission::Unpack(*ssl_socket_, forward0f(callback));
        }
    }
}