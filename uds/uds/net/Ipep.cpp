#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <netdb.h>
#endif

#include <uds/net/Ipep.h>

namespace uds {
    namespace net {
        IPEndPoint Ipep::GetEndPoint(const std::string& address, bool resolver) noexcept {
            if (address.empty()) {
                return Ipep::GetEndPoint(address, 0);
            }

            size_t index = address.find('[');
            if (index != std::string::npos) {
                size_t right = address.find(']', index);
                if (right == std::string::npos) {
                    return IPEndPoint(IPEndPoint::NoneAddress, 0);
                }

                size_t LENT = right - index - 1;
                if (LENT == 0) {
                    return IPEndPoint(IPEndPoint::NoneAddress, 0);
                }

                std::string host = address.substr(index + 1, LENT);
                if (host.empty()) {
                    return IPEndPoint(IPEndPoint::NoneAddress, 0);
                }

                index = address.rfind(':');
                if (index == std::string::npos) {
                    return Ipep::GetEndPoint(host, 0);
                }

                std::string port = address.substr(index + 1);
                return Ipep::GetEndPoint(host, atoi(port.data()), resolver);
            }
            else {
                index = address.rfind(':');
                if (index == std::string::npos) {
                    return Ipep::GetEndPoint(address, 0);
                }

                std::string host = address.substr(0, index);
                std::string port = address.substr(index + 1);
                return Ipep::GetEndPoint(host, atoi(port.data()), resolver);
            }
        }

        std::string Ipep::ToIpepAddress(const IPEndPoint* ep) noexcept {
            if (NULL == ep) {
                return "0.0.0.0:0";
            }

            int address_bytes_size;
            Byte* address_bytes = ep->GetAddressBytes(address_bytes_size);
            std::string address_text = IPEndPoint::ToAddressString(ep->GetAddressFamily(), address_bytes, address_bytes_size);
            if (ep->GetAddressFamily() == AddressFamily::InterNetwork) {
                char sz[0xff];
                sprintf(sz, "%s:%u", address_text.data(), ep->Port);
                return sz;
            }
            else {
                char sz[0xff];
                sprintf(sz, "[%s]:%u", address_text.data(), ep->Port);
                return sz;
            }
        }

        IPEndPoint Ipep::GetEndPoint(const std::string& host, int port, bool resolver) noexcept {
            if (port < IPEndPoint::MinPort || port > IPEndPoint::MaxPort) {
                port = IPEndPoint::MinPort;
            }

            IPEndPoint localEP = IPEndPoint(host.data(), port);
            if (resolver && localEP.IsNone()) {
                struct addrinfo req, * hints, * p;
                memset(&req, 0, sizeof(req));

                req.ai_family = AF_UNSPEC;
                req.ai_socktype = SOCK_STREAM;

                if (getaddrinfo(host.data(), NULL, &req, &hints)) {
                    return IPEndPoint(0u, port);
                }

                for (p = hints; NULL != p; p = p->ai_next) {
                    if (p->ai_family == AF_INET) {
                        struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
                        return IPEndPoint(AddressFamily::InterNetwork,
                            (Byte*)&(ipv4->sin_addr), sizeof(ipv4->sin_addr), port);
                    }
                }

                for (p = hints; NULL != p; p = p->ai_next) {
                    if (p->ai_family == AF_INET6) {
                        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
                        return IPEndPoint(AddressFamily::InterNetworkV6,
                            (Byte*)&(ipv6->sin6_addr), sizeof(ipv6->sin6_addr), port);
                    }
                }
            }
            return localEP;
        }

        bool Ipep::IsDomainAddress(const std::string& domain) noexcept {
            if (domain.empty()) {
                return false;
            }
            elif(domain == "localhost") {
                return true;
            }
            else {
                static std::regex pattern("^(?=^.{3,255}$)[a-zA-Z0-9][-a-zA-Z0-9]{0,63}(\\.[a-zA-Z0-9][-a-zA-Z0-9]{0,63})+$");
                return std::regex_match(domain, pattern);
            }
        }
    }
}