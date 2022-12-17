#include <uds/transmission/WebSocketTransmission.h>
#include <uds/transmission/templates/WebSocket.hpp>
#include <uds/net/Ipep.h>
#include <uds/net/IPEndPoint.h>

namespace uds {
    namespace transmission {
        // Construct the stream by moving in the socket.
        WebSocketTransmission::WebSocketTransmission(
            const std::shared_ptr<uds::threading::Hosting>&         hosting,
            const std::shared_ptr<boost::asio::io_context>&         context,
            const std::shared_ptr<boost::asio::ip::tcp::socket>&    socket,
            const std::string&                                      host,
            const std::string&                                      path,
            int                                                     alignment) noexcept
            : Transmission(hosting, context, socket, alignment)
            , disposed_(false)
            , host_(host)
            , path_(path)
            , websocket_(std::move(*socket)) {

        }

        bool WebSocketTransmission::HandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept {
            if (!callback) {
                return false;
            }

            class AcceptWebSocket final : public uds::transmission::templates::WebSocket<AsioWebSocket> {
            public:
                inline AcceptWebSocket(const std::shared_ptr<WebSocketTransmission>& transmission, AsioWebSocket& websocket, std::string& host, std::string& path) noexcept
                    : WebSocket(websocket, host, path)
                    , transmission_(transmission) {

                }

            public:
                virtual void Dispose() noexcept override {
                    std::shared_ptr<WebSocketTransmission> transmission = std::move(transmission_);
                    if (transmission) {
                        transmission_.reset();
                        transmission->Close();
                    }
                }

            protected:
                virtual void SetAddress(const std::string& address) noexcept override {
                    if (address.size()) {
                        std::shared_ptr<WebSocketTransmission> transmission = transmission_;
                        if (transmission) {
                            IPEndPoint remoteEP = transmission->GetRemoteEndPoint();
                            transmission->SetRemoteEndPoint(IPEndPoint(address.data(), remoteEP.Port));
                        }
                    }
                }

            private:
                std::shared_ptr<WebSocketTransmission> transmission_;
            };

            std::shared_ptr<WebSocketTransmission> transmission = Reference::CastReference<WebSocketTransmission>(GetReference());
            std::shared_ptr<AcceptWebSocket> accept =
                Reference::NewReference<AcceptWebSocket>(transmission, websocket_, host_, path_);
            return accept->HandshakeAsync(type, forward0f(callback));
        }

        std::string WebSocketTransmission::WSSOF(
            const std::string&                                      url,
            std::string&                                            host,
            std::string&                                            addr,
            std::string&                                            path,
            int&                                                    port,
            bool&                                                   tlsv) noexcept {
            typedef uds::net::IPEndPoint IPEndPoint;
            typedef uds::net::Ipep       Ipep;

            port = IPEndPoint::MinPort;
            host = "";
            path = "";
            tlsv = false;
            if (url.empty()) {
                return "";
            }

            std::string url_ = ToLower(LTrim(RTrim(url)));
            if (url_.empty()) {
                return "";
            }

            std::size_t index_ = url_.find("://");
            if (index_ == std::string::npos) {
                return "";
            }

            std::size_t n_ = index_ + 3;
            if (n_ >= url_.size()) {
                return "";
            }

            std::string host_and_path_ = url_.substr(n_);
            std::string proto_ = url_.substr(0, index_);
            std::string sdns_;
            std::string sabs_;
            std::string path_;
            int port_ = 0;

            index_ = host_and_path_.find_first_of('/');
            if (index_ != std::string::npos) {
                n_ = index_ + 1;
                if (url_.size() > n_) {
                    path_ = "/" + host_and_path_.substr(n_);
                }
                sdns_ = host_and_path_.substr(0, index_);
            }
            else {
                path_ = "/";
                sdns_ = host_and_path_;
            }

            index_ = sdns_.find_first_of('.');
            if (index_ == std::string::npos) {
                return "";
            }
            else {
                n_ = index_ + 1;
                if (n_ >= sdns_.size()) {
                    return "";
                }
            }

            index_ = sdns_.find_first_of('[');
            if (index_ != std::string::npos) {
                n_ = sdns_.find_last_of(']');
                if (n_ == std::string::npos || index_ > n_) {
                    return "";
                }

                std::size_t l_ = index_ + 1;
                sabs_ = sdns_.substr(l_, n_ - l_);
                sdns_ = sdns_.substr(0, index_) + sdns_.substr(n_ + 1);
            }

            index_ = sdns_.find_first_of(':');
            if (index_ != std::string::npos) {
                n_ = index_ + 1;
                if (n_ >= sdns_.size()) {
                    return "";
                }
                std::string sz_ = LTrim(RTrim(sdns_.substr(n_)));
                port_ = atoi(sz_.data());
                sdns_ = sdns_.substr(0, index_);
            }

            sdns_ = LTrim(RTrim(sdns_));
            path_ = LTrim(RTrim(path_));
            if (port_ <= IPEndPoint::MinPort || port_ > IPEndPoint::MaxPort) {
                if (proto_ == "ws" || proto_ == "http") {
                    port_ = 80;
                }
                else {
                    port_ = 443;
                }
            }

            if (sabs_.empty()) {
                IPEndPoint remoteEP = Ipep::GetEndPoint(sdns_, port_);
                if (!IPEndPoint::IsInvalid(remoteEP)) {
                    sabs_ = remoteEP.ToAddressString();
                }
            }

            tlsv = (proto_ != "ws" && proto_ != "http");
            host = sdns_;
            addr = sabs_;
            path = path_;
            port = port_;
            url_ = proto_ + "://" + host + ":" + std::to_string(port) + path;
            return url_;
        }

        bool WebSocketTransmission::OnWriteAsync(const BOOST_ASIO_MOVE_ARG(pmessage) message) noexcept {
            const std::shared_ptr<ITransmission> reference = GetReference();
            const pmessage messages = BOOST_ASIO_MOVE_CAST(pmessage)(constantof(message));

            websocket_.async_write(boost::asio::buffer(messages->packet.get(), messages->packet_size),
                [reference, this, messages](const boost::system::error_code& ec, size_t sz) noexcept {
                    bool success = ec ? false :true;
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

        bool WebSocketTransmission::ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept {
            if (!websocket_.is_open()) {
                return false;
            }

            return Transmission::Unpack(websocket_, forward0f(callback));
        }

        void WebSocketTransmission::Dispose() noexcept {
            if (!disposed_.exchange(true)) {
                const std::shared_ptr<ITransmission> reference = GetReference();
                websocket_.async_close(boost::beast::websocket::close_code::normal,
                    [reference, this](const boost::system::error_code& ec_) noexcept {
                        Transmission::Dispose();
                    });
            }
        }

        bool WebSocketTransmission::WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept {
            if (!websocket_.is_open()) {
                return false;
            }

            pmessage messages = Pack(buffer.get(), offset, length, forward0f(callback));
            if (!messages) {
                return false;
            }

            OnAddWriteAsync(forward0f(messages));
            return true;
        }
    }
}