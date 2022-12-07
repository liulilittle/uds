#pragma once

#include <uds/IDisposable.h>
#include <uds/net/IPEndPoint.h>

namespace uds {
    namespace transmission {
        namespace templates {
            template<class T>
            class WebSocket : public IDisposable {
            public:
                typedef uds::transmission::ITransmission            ITransmission;
                typedef ITransmission::HandshakeAsyncCallback       HandshakeAsyncCallback;
                typedef ITransmission::HandshakeType                HandshakeType;
                typedef uds::net::IPEndPoint                        IPEndPoint;
                typedef boost::beast::http::dynamic_body            dynamic_body;
                typedef boost::beast::http::request<dynamic_body>   http_request;

            public:
                inline WebSocket(
                    T&                                              websocket,
                    std::string&                                    host,
                    std::string&                                    path) noexcept 
                    : host_(host)
                    , path_(path)
                    , websocket_(websocket) {
                    websocket_.binary(true);
                }

            protected:
                virtual void                                        SetAddress(const std::string& address) = 0;

            public:
                inline void                                         Close() noexcept {
                    Dispose();
                }
                inline bool                                         HandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept {
                    if (!callback || host_.empty() || path_.empty()) {
                        return false;
                    }

                    const std::shared_ptr<Reference> reference = GetReference();
                    const HandshakeAsyncCallback callback_ = BOOST_ASIO_MOVE_CAST(HandshakeAsyncCallback)(constantof(callback));

                    if (type == HandshakeType::HandshakeType_Client) {
                        websocket_.async_handshake(host_, path_,
                            [reference, this, callback_](const boost::system::error_code& ec) noexcept {
                                bool success = ec ? false : true;
                                if (!success) {
                                    Close();
                                }

                                callback_(success);
                            });
                    }
                    else {
                        // This buffer is used for reading and must be persisted
                        std::shared_ptr<boost::beast::flat_buffer> buffer = make_shared_object<boost::beast::flat_buffer>();

                        // Declare a container to hold the response
                        std::shared_ptr<http_request> req = make_shared_object<http_request>();

                        // Receive the HTTP response
                        boost::beast::http::async_read(websocket_.next_layer(), *buffer, *req,
                            [reference, this, buffer, req, callback_](boost::system::error_code ec, std::size_t sz) noexcept {
                                if (ec == boost::beast::http::error::end_of_stream) {
                                    ec = boost::beast::websocket::error::closed;
                                }

                                bool success = false;
                                do {
                                    if (ec) {
                                        break;
                                    }

                                    // Set suggested timeout settings for the websocket
                                    websocket_.set_option(
                                        boost::beast::websocket::stream_base::timeout::suggested(
                                            boost::beast::role_type::server));

                                    // Set a decorator to change the Server of the handshake
                                    websocket_.set_option(boost::beast::websocket::stream_base::decorator(
                                        [](boost::beast::websocket::response_type& res) noexcept {
                                            res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING));
                                        }));

                                    success = CheckPath(path_, req->target());
                                    if (!success) {
                                        ec = boost::beast::websocket::error::closed;
                                    }
                                    else {
                                        websocket_.async_accept(*req,
                                            [reference, this, req, callback_](const boost::system::error_code& ec) noexcept {
                                                bool success = ec ? false : true;
                                                if (!success) {
                                                    Close();
                                                }
                                                callback_(success);
                                            });
                                        SetAddress(GetAddress(*req));
                                    }
                                } while (0);

                                if (!success) {
                                    Close();
                                    callback_(success);
                                }
                            });
                    }
                    return true;
                }
                
            private:
                static std::string                                  GetAddress(http_request& req) noexcept {
                    static const int _RealIpHeadersSize = 5;
                    static const char* _RealIpHeaders[_RealIpHeadersSize] = {
                        "CF-Connecting-IP",
                        "True-Client-IP",
                        "X-Real-IP",
                        "REMOTE-HOST",
                        "X-Forwarded-For",
                    };
                    // proxy_set_header Host $host;
                    // proxy_set_header X-Real-IP $remote_addr;
                    // proxy_set_header REMOTE-HOST $remote_addr;
                    // proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
                    for (int i = 0; i < _RealIpHeadersSize; i++) {
                        http_request::iterator tail = req.find(_RealIpHeaders[i]);
                        http_request::iterator endl = req.end();
                        if (tail == endl) {
                            continue;
                        }

                        const boost::beast::string_view& sw = tail->value();
                        if (sw.empty()) {
                            continue;
                        }

                        const std::string address = std::string(sw.data(), sw.size());
                        IPEndPoint localEP(address.c_str(), IPEndPoint::MinPort);
                        if (IPEndPoint::IsInvalid(localEP)) {
                            continue;
                        }

                        return localEP.ToAddressString();
                    }
                    return std::string();
                }
                static bool                                         CheckPath(std::string& root, const boost::beast::string_view& sw) noexcept {
                    if (root.size() <= 1) {
                        return true;
                    }

                    std::string path_ = "/";
                    if (sw.size()) {
                        path_ = ToLower(LTrim(RTrim(std::string(sw.data(), sw.size()))));
                        if (path_.empty()) {
                            return false;
                        }
                    }

                    std::size_t sz_ = path_.find_first_of('?');
                    if (sz_ == std::string::npos) {
                        sz_ = path_.find_first_of('#');
                    }

                    if (sz_ != std::string::npos) {
                        path_ = path_.substr(0, sz_);
                    }

                    if (path_.size() < root.size()) {
                        return false;
                    }

                    std::string lroot_ = ToLower(root);
                    if (path_ == lroot_) {
                        return true;
                    }

                    if (path_.size() == lroot_.size()) {
                        return false;
                    }

                    int ch = path_[lroot_.size()];
                    return ch == '/';
                }

            private:
                std::string&                                        host_;
                std::string&                                        path_;
                T&                                                  websocket_;
            };
        }
    }
}