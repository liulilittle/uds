#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <uds/net/Ipep.h>
#include <uds/net/IPEndPoint.h>

#define IP_END_POINT_USE_BOOST_INET_PTON

namespace uds {
    namespace net {
        IPEndPoint::IPEndPoint(const char* address, int port) noexcept
            : _AddressFamily(AddressFamily::InterNetwork)
            , Port(port) {

            if (NULL == address || *address == '\x0') {
                *(UInt32*)this->_AddressBytes = IPEndPoint::NoneAddress;
                this->_AddressFamily = AddressFamily::InterNetwork;
            }
            else {
#ifndef IP_END_POINT_USE_BOOST_INET_PTON
                struct sockaddr_in6 in6;
                int err = inet_pton(AF_INET6, address, &in6);
                if (err > 0) {
                    this->_AddressFamily = AddressFamily::InterNetworkV6;
                    memcpy(this->_AddressBytes, &in6.sin6_addr, sizeof(this->_AddressBytes));
                }
                else {
                    *(UInt32*)this->_AddressBytes = inet_addr(address);
                    this->_AddressFamily = AddressFamily::InterNetwork;
                }
#else
                boost::system::error_code ec;
                boost::asio::ip::address host = boost::asio::ip::address::from_string(address, ec);
                if (ec) {
                    this->_AddressFamily = AddressFamily::InterNetwork;
                    *(UInt32*)this->_AddressBytes = IPEndPoint::NoneAddress;
                }
                elif(host.is_v6()) {
                    this->_AddressFamily = AddressFamily::InterNetworkV6;
                    boost::asio::ip::address_v6::bytes_type buf = host.to_v6().to_bytes();
                    memcpy(this->_AddressBytes, buf.data(), buf.size());
                }
                else {
                    this->_AddressFamily = AddressFamily::InterNetwork;
                    boost::asio::ip::address_v4::bytes_type buf = host.to_v4().to_bytes();
                    memcpy(this->_AddressBytes, buf.data(), buf.size());
                }
#endif
            }
        }

        IPEndPoint::IPEndPoint(AddressFamily af, const void* address_bytes, int address_size, int port) noexcept
            : _AddressFamily(af)
            , Port(port) {
            int limit_size = 0;
            if (af == AddressFamily::InterNetworkV6) {
                limit_size = sizeof(struct in6_addr);
            }
            else {
                af = AddressFamily::InterNetwork;
                limit_size = sizeof(struct in_addr);
            }
            memset(this->_AddressBytes, 0, limit_size);
            if (NULL != address_bytes && address_size > 0) {
                memcpy(this->_AddressBytes, address_bytes, std::min<int>(address_size, limit_size));
            }
            this->_AddressFamily = af;
        }

        std::string IPEndPoint::GetHostName() noexcept {
            char sz[256]; // 域名规定不超过64字节（但是几乎大部分实现为64-1字节）
            sz[0x00] = 0;
            sz[0xff] = 0;
            gethostname(sz, 0xff);
            return sz[0] == 0x00 ? "localhost" : sz;
        }

        std::string IPEndPoint::ToString() noexcept {
            return Ipep::ToIpepAddress(this);
        }

        std::string IPEndPoint::ToAddressString(AddressFamily af, const Byte* address_bytes, int address_size) noexcept {
            if (NULL == address_bytes || address_size < 1) {
                return "0.0.0.0";
            }

            if (af == AddressFamily::InterNetworkV6) {
                if (address_size < (int)sizeof(struct in6_addr)) {
                    return "0.0.0.0";
                }

                char sz[INET6_ADDRSTRLEN];
                if (!inet_ntop(AF_INET6, (struct in6_addr*)address_bytes, sz, sizeof(sz))) {
                    return "0.0.0.0";
                }
                return sz;
            }
            else {
                if (address_size < (int)sizeof(struct in_addr)) {
                    return "0.0.0.0";
                }

                char sz[INET_ADDRSTRLEN];
                if (!inet_ntop(AF_INET, (struct in_addr*)address_bytes, sz, sizeof(sz))) {
                    return "0.0.0.0";
                }
                return sz; // inet_ntoa(*(struct in_addr*)address);
            }
        }
    }
}