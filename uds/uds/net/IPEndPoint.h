#pragma once

#include <uds/stdafx.h>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/function.hpp> 
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

namespace uds {
    namespace net {
        enum AddressFamily {
            InterNetwork                                                        = AF_INET,
            InterNetworkV6                                                      = AF_INET6,
        };

        struct IPEndPoint {
        private:
            mutable Byte                                                        _AddressBytes[sizeof(struct in6_addr)]; // 16
            AddressFamily                                                       _AddressFamily;

        public:
            const int                                                           Port;

        public:
            static const int                                                    MinPort          = 0;
            static const int                                                    MaxPort          = UINT16_MAX;
            static const UInt32                                                 AnyAddress       = INADDR_ANY;
            static const UInt32                                                 NoneAddress      = INADDR_NONE;
            static const UInt32                                                 LoopbackAddress  = INADDR_LOOPBACK;
            static const UInt32                                                 BroadcastAddress = INADDR_BROADCAST;

        public:
            inline IPEndPoint() noexcept
                : IPEndPoint(NoneAddress, IPEndPoint::MinPort) {

            }
            inline IPEndPoint(UInt32 address, int port) noexcept
                : _AddressFamily(AddressFamily::InterNetwork)
                , Port(port) {
                if (port < IPEndPoint::MinPort || port > IPEndPoint::MaxPort) {
                    port = IPEndPoint::MinPort;
                }

                *(Int32*)&this->Port = port;
                *(UInt32*)this->_AddressBytes = address;
            }
            IPEndPoint(const char* address, int port) noexcept;
            IPEndPoint(AddressFamily af, const void* address_bytes, int address_size, int port) noexcept;

        public:
            inline IPEndPoint                                                   Any(int port) noexcept {
                return IPEndPoint(IPEndPoint::AnyAddress, port);
            }
            inline IPEndPoint                                                   Loopback(int port) noexcept {
                return IPEndPoint(IPEndPoint::LoopbackAddress, port);
            }
            inline IPEndPoint                                                   Broadcast(int port) noexcept {
                return IPEndPoint(IPEndPoint::BroadcastAddress, port);
            }
            inline IPEndPoint                                                   None(int port) noexcept {
                return IPEndPoint(IPEndPoint::NoneAddress, port);
            }
            inline IPEndPoint                                                   IPv6Any(int port) noexcept {
                boost::asio::ip::tcp::endpoint localEP(boost::asio::ip::address_v6::any(), port);
                return ToEndPoint(localEP);
            }
            inline IPEndPoint                                                   IPv6Loopback(int port) noexcept {
                boost::asio::ip::tcp::endpoint localEP(boost::asio::ip::address_v6::loopback(), port);
                return ToEndPoint(localEP);
            }
            inline IPEndPoint                                                   IPv6None(int port) noexcept {
                return this->IPv6Any(port);
            }

        public:
            inline bool                                                         IsBroadcast() noexcept {
                return this->IsNone();
            }
            inline bool                                                         IsNone() noexcept {
                if (AddressFamily::InterNetwork != this->_AddressFamily) {
                    int len;
                    Byte* p = this->GetAddressBytes(len);
                    return *p == 0xff;
                }
                else {
                    return this->GetAddress() == IPEndPoint::NoneAddress;
                }
            }
            inline bool                                                         IsAny() noexcept {
                if (AddressFamily::InterNetwork != this->_AddressFamily) {
                    int len;
                    Int64* p = (Int64*)this->GetAddressBytes(len);
                    return p[0] == 0 && p[1] == 0;
                }
                else {
                    return this->GetAddress() == IPEndPoint::AnyAddress;
                }
            }
            inline bool                                                         IsLoopback() noexcept {
                if (AddressFamily::InterNetwork != this->_AddressFamily) {
                    int len;
                    boost::asio::ip::address_v6::bytes_type* p =
                        (boost::asio::ip::address_v6::bytes_type*)this->GetAddressBytes(len); // IN6_IS_ADDR_LOOPBACK
                    return boost::asio::ip::address_v6(*p).is_loopback();
                }
                else {
                    return this->GetAddress() == ntohl(IPEndPoint::LoopbackAddress);
                }
            }

        public:
            inline std::string                                                  GetAddressBytes() const noexcept {
                int datalen;
                Byte* data = this->GetAddressBytes(datalen);
                return std::string((char*)data, datalen);
            }
            inline Byte*                                                        GetAddressBytes(int& len) const {
                if (this->_AddressFamily == AddressFamily::InterNetworkV6) {
                    len = sizeof(this->_AddressBytes);
                    return this->_AddressBytes;
                }
                else {
                    len = sizeof(UInt32);
                    return this->_AddressBytes;
                }
            }
            inline UInt32                                                       GetAddress() const noexcept {
                return *(UInt32*)this->_AddressBytes;
            }
            inline AddressFamily                                                GetAddressFamily() const noexcept {
                return this->_AddressFamily;
            }
            inline bool                                                         Equals(const IPEndPoint& value) const {
                IPEndPoint* right = (IPEndPoint*)&reinterpret_cast<const char&>(value);
                if ((IPEndPoint*)this == (IPEndPoint*)right) {
                    return true;
                }
                if ((IPEndPoint*)this == (IPEndPoint*)NULL ||
                    (IPEndPoint*)right == (IPEndPoint*)NULL ||
                    this->Port != value.Port) {
                    return false;
                }
                return *this == value;
            }
            inline bool                                                         operator == (const IPEndPoint& right) const noexcept {
                if (this->_AddressFamily != right._AddressFamily) {
                    return false;
                }

                Byte* x = this->_AddressBytes;
                Byte* y = right._AddressBytes;
                if (x == y) {
                    return true;
                }

                if (this->_AddressFamily == AddressFamily::InterNetworkV6) {
                    UInt64* qx = (UInt64*)x;
                    UInt64* qy = (UInt64*)y;
                    return qx[0] == qy[0] && qx[1] == qy[1];
                }
                return *(UInt32*)x == *(UInt32*)y;
            }
            inline bool                                                         operator != (const IPEndPoint& right) const noexcept {
                bool b = (*this) == right;
                return !b;
            }
            inline IPEndPoint&                                                  operator = (const IPEndPoint& right) {
                this->_AddressFamily = right._AddressFamily;
                constantof(this->Port) = right.Port;

                int address_bytes_size;
                Byte* address_bytes = right.GetAddressBytes(address_bytes_size);
                memcpy(this->_AddressBytes, address_bytes, address_bytes_size);

                return *this;
            }
            inline std::string                                                  ToAddressString() noexcept {
                int address_bytes_size;
                Byte* address_bytes = GetAddressBytes(address_bytes_size);
                return ToAddressString(this->_AddressFamily, address_bytes, address_bytes_size);
            }
            inline int                                                          GetHashCode() const noexcept {
                int h = this->GetAddressFamily() + this->Port;
                int l = 0;
                Byte* p = this->GetAddressBytes(l);
                for (int i = 0; i < l; i++) {
                    h += *p++;
                }
                return h;
            }
            std::string                                                         ToString() noexcept;

        public:
            static std::string                                                  GetHostName() noexcept;
            static std::string                                                  ToAddressString(AddressFamily af, const Byte* address_bytes, int address_size) noexcept;
            inline static std::string                                           ToAddressString(UInt32 address) noexcept {
                return ToAddressString(AddressFamily::InterNetwork, (Byte*)&address, sizeof(address));
            }
            inline static std::string                                           ToAddressString(AddressFamily af, const std::string& address_bytes) noexcept {
                return ToAddressString(af, (Byte*)address_bytes.data(), address_bytes.size());
            }
            inline static UInt32                                                PrefixToNetmask(int prefix) noexcept {
                UInt32 mask = prefix ? (~0 << (32 - prefix)) : 0;
                return htonl(mask);
            }
            inline static int                                                   NetmaskToPrefix(UInt32 mask) noexcept {
                unsigned char* bytes = (unsigned char*)&mask;
                unsigned int bitLength = 0;
                unsigned int idx = 0;

                // find beginning 0xFF
                for (; idx < sizeof(mask) && bytes[idx] == 0xff; idx++);
                bitLength = 8 * idx;

                if (idx < sizeof(mask)) {
                    switch (bytes[idx]) {
                    case 0xFE: bitLength += 7; break;
                    case 0xFC: bitLength += 6; break;
                    case 0xF8: bitLength += 5; break;
                    case 0xF0: bitLength += 4; break;
                    case 0xE0: bitLength += 3; break;
                    case 0xC0: bitLength += 2; break;
                    case 0x80: bitLength += 1; break;
                    case 0x00: break;
                    default: // invalid bitmask
                        return ~0;
                    }
                    // remainder must be 0x00
                    for (unsigned int j = idx + 1; j < sizeof(mask); j++) {
                        unsigned char x = bytes[j];
                        if (x != 0x00) {
                            return ~0;
                        }
                    }
                }
                return bitLength;
            }
            inline static bool                                                  IsInvalid(const IPEndPoint* p) noexcept {
                IPEndPoint* __p = (IPEndPoint*)p;
                if (NULL == __p) {
                    return true;
                }
                if (__p->IsNone()) {
                    return true;
                }
                if (__p->IsAny()) {
                    return true;
                }
                return false;
            }
            inline static bool                                                  IsInvalid(const IPEndPoint& value) noexcept {
                return IPEndPoint::IsInvalid(addressof(value));
            }

        public:
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            Transform(AddressFamily addressFamily, const boost::asio::ip::basic_endpoint<TProtocol>& remoteEP) noexcept {
                boost::asio::ip::address address = remoteEP.address();
                if (addressFamily == AddressFamily::InterNetwork) {
                    if (address.is_v4()) {
                        return remoteEP;
                    }
                    else {
                        return IPEndPoint::ToEndPoint<TProtocol>(IPEndPoint::V6ToV4(IPEndPoint::ToEndPoint(remoteEP)));
                    }
                }
                else {
                    if (address.is_v6()) {
                        return remoteEP;
                    }
                    else {
                        return IPEndPoint::ToEndPoint<TProtocol>(IPEndPoint::V4ToV6(IPEndPoint::ToEndPoint(remoteEP)));
                    }
                }
            }
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            ToEndPoint(const IPEndPoint& endpoint) noexcept {
                AddressFamily af = endpoint.GetAddressFamily();
                if (af == AddressFamily::InterNetwork) {
                    return WrapAddressV4<TProtocol>(endpoint.GetAddress(), endpoint.Port);
                }
                else {
                    int len;
                    const Byte* address = endpoint.GetAddressBytes(len);
                    return WrapAddressV6<TProtocol>(address, len, endpoint.Port);
                }
            }
            template<class TProtocol>
            inline static IPEndPoint                                            ToEndPoint(const boost::asio::ip::basic_endpoint<TProtocol>& endpoint) noexcept {
                boost::asio::ip::address address = endpoint.address();
                if (address.is_v4()) {
                    return IPEndPoint(ntohl(address.to_v4().to_ulong()), endpoint.port());
                }
                else {
                    boost::asio::ip::address_v6::bytes_type bytes = address.to_v6().to_bytes();
                    return IPEndPoint(AddressFamily::InterNetworkV6, bytes.data(), bytes.size(), endpoint.port());
                }
            }
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            NewAddress(const char* address, int port) noexcept {
                typedef boost::asio::ip::basic_endpoint<TProtocol> protocol_endpoint;

                if (NULL == address || *address == '\x0') {
                    address = "0.0.0.0";
                }

                if (port < IPEndPoint::MinPort || port > IPEndPoint::MaxPort) {
                    port = IPEndPoint::MinPort;
                }

                boost::system::error_code ec_;
                boost::asio::ip::address ba_ = boost::asio::ip::address::from_string(address, ec_);
                if (ec_) {
                    ba_ = boost::asio::ip::address_v4(IPEndPoint::NoneAddress);
                }
                return protocol_endpoint(ba_, port);
            }
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            WrapAddressV4(UInt32 address, int port) noexcept {
                typedef boost::asio::ip::basic_endpoint<TProtocol> protocol_endpoint;

                return protocol_endpoint(boost::asio::ip::address_v4(ntohl(address)), port);
            }
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            WrapAddressV6(const void* address, int size, int port) noexcept {
                typedef boost::asio::ip::basic_endpoint<TProtocol> protocol_endpoint;

                if (size < 0) {
                    size = 0;
                }

                boost::asio::ip::address_v6::bytes_type address_bytes;
                unsigned char* p = &address_bytes[0];
                memcpy(p, address, size);
                memset(p, 0, address_bytes.size() - size);

                return protocol_endpoint(boost::asio::ip::address_v6(address_bytes), port);
            }
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            AnyAddressV4(int port) noexcept {
                typedef boost::asio::ip::basic_endpoint<TProtocol> protocol_endpoint;

                if (port < IPEndPoint::MinPort || port > IPEndPoint::MaxPort) {
                    port = IPEndPoint::MinPort;
                }
                return protocol_endpoint(boost::asio::ip::address_v4::any(), port);
            }
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            LocalAddress(boost::asio::ip::basic_resolver<TProtocol>& resolver, int port) noexcept {
                return GetAddressByHostName<TProtocol>(resolver, GetHostName(), port);
            }
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            GetAddressByHostName(boost::asio::ip::basic_resolver<TProtocol>& resolver, const std::string& hostname, int port) noexcept {
                typedef boost::asio::ip::basic_resolver<TProtocol> protocol_resolver;

                typename protocol_resolver::query q(hostname.c_str(), std::to_string(port).c_str());
#ifndef _WIN32
                typename protocol_resolver::iterator i = resolver.resolve(q);
                typename protocol_resolver::iterator l;

                if (i == l) {
                    return AnyAddressV4<TProtocol>(port);
                }
#else
                typename protocol_resolver::results_type results = resolver.resolve(q);
                if (results.empty()) {
                    return AnyAddressV4<TProtocol>(port);
                }

                typename protocol_resolver::iterator i = results.begin();
                typename protocol_resolver::iterator l = results.end();
#endif
                typename protocol_resolver::iterator tail = i;
                typename protocol_resolver::iterator endl = l;
                for (; tail != endl; ++tail) {
                    boost::asio::ip::basic_endpoint<TProtocol> localEP = *tail;
                    if (localEP.address().is_v4()) {
                        return localEP;
                    }
                }

                tail = i;
                endl = l;
                for (; tail != endl; ++tail) {
                    boost::asio::ip::basic_endpoint<TProtocol> localEP = *tail;
                    if (localEP.address().is_v6()) {
                        return localEP;
                    }
                }
                return AnyAddressV4<TProtocol>(port);
            }
#if !defined(NCOROUTINE)
            template<class TProtocol>
            inline static boost::asio::ip::basic_endpoint<TProtocol>            GetAddressByHostName(boost::asio::ip::basic_resolver<TProtocol>& resolver, const std::string& hostname, int port, const boost::asio::yield_context& y) noexcept {
                typedef boost::asio::ip::basic_resolver<TProtocol> protocol_resolver;

                boost::system::error_code ec_;
                typename protocol_resolver::query q(hostname.c_str(), std::to_string(port).c_str());
#ifndef _WIN32
                typename protocol_resolver::iterator i;
                typename protocol_resolver::iterator l;
                try {
                    i = resolver.async_resolve(q, y[ec_]);
                    if (ec_) {
                        return AnyAddressV4<TProtocol>(port);
                    }
                }
                catch (std::exception&) {
                    return AnyAddressV4<TProtocol>(port);
                }

                if (i == l) {
                    return AnyAddressV4<TProtocol>(port);
                }
#else
                typename protocol_resolver::results_type results;
                try {
                    results = resolver.async_resolve(q, y[ec_]);
                    if (ec_) {
                        return AnyAddressV4<TProtocol>(port);
                    }
                }
                catch (std::exception&) {
                    return AnyAddressV4<TProtocol>(port);
                }

                if (results.empty()) {
                    return AnyAddressV4<TProtocol>(port);
                }

                typename protocol_resolver::iterator i = results.begin();
                typename protocol_resolver::iterator l = results.end();
#endif
                typename protocol_resolver::iterator tail = i;
                typename protocol_resolver::iterator endl = l;
                for (; tail != endl; ++tail) {
                    boost::asio::ip::basic_endpoint<TProtocol> localEP = *tail;
                    if (localEP.address().is_v4()) {
                        return localEP;
                    }
                }

                tail = i;
                endl = l;
                for (; tail != endl; ++tail) {
                    boost::asio::ip::basic_endpoint<TProtocol> localEP = *tail;
                    if (localEP.address().is_v6()) {
                        return localEP;
                    }
                }
                return AnyAddressV4<TProtocol>(port);
            }
#endif
            template<class TProtocol>
            inline static bool                                                  Equals(const boost::asio::ip::basic_endpoint<TProtocol>& x, const boost::asio::ip::basic_endpoint<TProtocol>& y) noexcept {
                if (x != y) {
                    return false;
                }
                return x.address() == y.address() && x.port() == y.port();
            }
        
        public:
            inline static IPEndPoint                                            V6ToV4(const IPEndPoint& destinationEP) noexcept {
                if (destinationEP.GetAddressFamily() == AddressFamily::InterNetwork) {
                    return destinationEP;
                }

#pragma pack(push, 1)
                struct IPV62V4ADDR {
                    uint64_t R1;
                    uint16_t R2;
                    uint16_t R3;
                    uint32_t R4;
                };
#pragma pack(pop)

                int len;
                IPV62V4ADDR* inaddr = (IPV62V4ADDR*)destinationEP.GetAddressBytes(len);
                if (inaddr->R1 || inaddr->R1 || inaddr->R3 != UINT16_MAX) {
                    return destinationEP;
                }
                return IPEndPoint(inaddr->R4, destinationEP.Port);
            }
            inline static IPEndPoint                                            V4ToV6(const IPEndPoint& destinationEP) noexcept {
                if (destinationEP.GetAddressFamily() == AddressFamily::InterNetworkV6) {
                    return destinationEP;
                }

#pragma pack(push, 1)
                struct IPV62V4ADDR {
                    uint64_t R1;
                    uint16_t R2;
                    uint16_t R3;
                    uint32_t R4;
                };
#pragma pack(pop)

                IPV62V4ADDR inaddr;
                inaddr.R1 = 0;
                inaddr.R2 = 0;
                inaddr.R3 = UINT16_MAX;
                inaddr.R4 = destinationEP.GetAddress();
                return IPEndPoint(AddressFamily::InterNetworkV6, &inaddr, sizeof(IPV62V4ADDR), destinationEP.Port);
            }
        };
    }
}