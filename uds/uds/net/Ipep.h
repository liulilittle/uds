#pragma once 

#include <uds/net/IPEndPoint.h>

namespace uds {
    namespace net {
        class Ipep final {
        public:
            typedef std::function<void(std::vector<IPEndPoint>&)>               GetAddressesByHostNameCallback;
            typedef std::function<void(IPEndPoint*)>                            GetAddressByHostNameCallback;

        public:
            static IPEndPoint                                                   GetEndPoint(const std::string& address, bool resolver = true) noexcept;
            static IPEndPoint                                                   GetEndPoint(const std::string& host, int port, bool resolver = true) noexcept;
            inline static std::string                                           ToIpepAddress(const IPEndPoint& ep) noexcept {
                return ToIpepAddress(addressof(ep));
            }
            static std::string                                                  ToIpepAddress(const IPEndPoint* ep) noexcept;
            static bool                                                         SetDnsAddresses(const std::vector<std::string>& addresses) noexcept;
            static bool                                                         ToEndPoint(const std::string& addresses, std::vector<std::string>& out) noexcept;
            static bool                                                         IsDomainAddress(const std::string& domain) noexcept;

        public:
            template<class TProtocol>
            inline static void                                                  GetAddressByHostName(const std::shared_ptr<boost::asio::ip::basic_resolver<TProtocol> >& resolver, const std::string& hostname, int port, const std::shared_ptr<GetAddressByHostNameCallback>& callback) {
                if (NULL == resolver) {
                    throw std::runtime_error("Argument \"resolver\" is not allowed to be a NULL references.");
                }
                GetAddressByHostName(*resolver, hostname, port, callback);
            }
            template<class TProtocol>
            inline static void                                                  GetAddressByHostName(boost::asio::ip::basic_resolver<TProtocol>& resolver, const std::string& hostname, int port, const std::shared_ptr<GetAddressByHostNameCallback>& callback) {
                if (NULL == callback) {
                    throw std::runtime_error("Argument \"callback\" is not allowed to be a NULL references.");
                }
                std::shared_ptr<GetAddressByHostNameCallback> callback_ = callback;
                GetAddressesByHostName(resolver, hostname, port, make_shared_object<GetAddressesByHostNameCallback>(
                    [callback_](std::vector<IPEndPoint>& addresses) noexcept {
                        IPEndPoint* address = NULL;
                        if (NULL == address) {
                            for (size_t i = 0, l = addresses.size(); i < l; i++) {
                                const IPEndPoint& r = addresses[i];
                                if (r.GetAddressFamily() == AddressFamily::InterNetwork) {
                                    address = (IPEndPoint*)&reinterpret_cast<const char&>(r);
                                    break;
                                }
                            }
                        }
                        if (NULL == address) {
                            for (size_t i = 0, l = addresses.size(); i < l; i++) {
                                const IPEndPoint& r = addresses[i];
                                if (r.GetAddressFamily() == AddressFamily::InterNetworkV6) {
                                    address = (IPEndPoint*)&reinterpret_cast<const char&>(r);
                                    break;
                                }
                            }
                        }
                        (*callback_)(address);
                    }));
            }
            template<class TProtocol>
            inline static void                                                  GetAddressesByHostName(const std::shared_ptr<boost::asio::ip::basic_resolver<TProtocol> >& resolver, const std::string& hostname, int port, const std::shared_ptr<GetAddressesByHostNameCallback>& callback) {
                if (NULL == resolver) {
                    throw std::runtime_error("Argument \"resolver\" is not allowed to be a NULL references.");
                }
                GetAddressesByHostName(*resolver, hostname, port, callback);
            }
            template<class TProtocol>
            inline static void                                                  GetAddressesByHostName(boost::asio::ip::basic_resolver<TProtocol>& resolver, const std::string& hostname, int port, const std::shared_ptr<GetAddressesByHostNameCallback>& callback) {
                typedef boost::asio::ip::basic_resolver<TProtocol> protocol_resolver;

                if (NULL == callback) {
                    throw std::runtime_error("Argument \"callback\" is not allowed to be a NULL references.");
                }

                IPEndPoint localEP = IPEndPoint(hostname.data(), port);
                if (!localEP.IsNone()) {
                    std::vector<IPEndPoint> addresses;
                    addresses.push_back(localEP);
                    (*callback)(addresses);
                    return;
                }

                std::shared_ptr<GetAddressesByHostNameCallback> callback_ = callback;
                auto completion_resolve = [](
                    std::vector<IPEndPoint>& addresses,
                    typename protocol_resolver::iterator& i,
                    typename protocol_resolver::iterator& l,
                    const std::shared_ptr<GetAddressesByHostNameCallback>& callback) noexcept {
                        for (; i != l; ++i) {
                            boost::asio::ip::basic_endpoint<TProtocol> localEP = std::move(*i);
                            if (!localEP.address().is_v4()) {
                                continue;
                            }
                            addresses.push_back(IPEndPoint::ToEndPoint<TProtocol>(localEP));
                        }
                };

                typename protocol_resolver::query q(hostname.c_str(), std::to_string(port).c_str());
#ifndef _WIN32
                resolver.async_resolve(q,
                    [completion_resolve, callback_](const boost::system::error_code& ec, typename protocol_resolver::iterator results) noexcept {
                        std::vector<IPEndPoint> addresses;
                        if (!ec) {
                            typename protocol_resolver::iterator i = std::move(results);
                            typename protocol_resolver::iterator l;

                            completion_resolve(addresses, i, l, callback_);
                        }
                        (*callback_)(addresses);
                    });
#else
                resolver.async_resolve(q,
                    [completion_resolve, callback_](const boost::system::error_code& ec, typename protocol_resolver::results_type results) noexcept {
                        std::vector<IPEndPoint> addresses;
                        if (!ec) {
                            if (!results.empty()) {
                                typename protocol_resolver::iterator i = results.begin();
                                typename protocol_resolver::iterator l = results.end();

                                completion_resolve(addresses, i, l, callback_);
                            }
                        }
                        (*callback_)(addresses);
                    });
#endif
            }
        };
    }
}