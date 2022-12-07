// https://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
// #define ENOENT           2      /* No such file or directory */
// #define EAGAIN          11      /* Try again */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <assert.h>

#include <sys/types.h>
#ifdef _WIN32
#include <stdint.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <error.h>
#include <sys/poll.h>
#endif
#include <fcntl.h>
#include <errno.h>

#include <uds/net/Socket.h>
#include <uds/net/IPEndPoint.h>
#include <uds/threading/Hosting.h>

namespace uds {
    namespace net {
        bool Socket::Poll(int s, int milliSeconds, SelectMode mode) noexcept {
            int64_t microSeconds = milliSeconds;
            microSeconds *= 1000;
            return Socket::PolH(s, microSeconds, mode);
        }

        bool Socket::PolH(int s, int64_t microSeconds, SelectMode mode) noexcept {
            if (s == -1) {
                return false;
            }

#ifdef _WIN32
            struct fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(s, &fdset);

            struct timeval tv;
            if (microSeconds < 0) {
                tv.tv_sec = INFINITY;
                tv.tv_usec = INFINITY;
            }
            else {
                tv.tv_sec = microSeconds / 1000000;
                tv.tv_usec = microSeconds;
            }

            int hr = -1;
            if (mode == SelectMode_SelectRead) {
                hr = select(s + 1, &fdset, NULL, NULL, &tv) > 0;
            }
            elif(mode == SelectMode_SelectWrite) {
                hr = select(s + 1, NULL, &fdset, NULL, &tv) > 0;
            }
            else {
                hr = select(s + 1, NULL, NULL, &fdset, &tv) > 0;
            }

            if (hr > 0) {
                return FD_ISSET(s, &fdset);
            }
#else
            struct pollfd fds[1];
            memset(fds, 0, sizeof(fds));

            int events = POLLERR;
            if (mode == SelectMode_SelectRead) {
                events = POLLIN;
            }
            elif(mode == SelectMode_SelectWrite) {
                events = POLLOUT;
            }
            else {
                events = POLLERR;
            }

            fds->fd = s;
            fds->events = events;

            int hr;
            if (microSeconds < 0) {
                int timeout_ = (int)INFINITY;
                hr = poll(fds, 1, timeout_);
            }
            else {
                int timeout_ = (int)(microSeconds / 1000);
                hr = poll(fds, 1, timeout_);
            }

            if (hr > 0) {
                if ((fds->revents & events) == events) {
                    return true;
                }
            }
#endif
            return false;
        }

        void Socket::Cancel(const boost::asio::ip::udp::socket& socket) noexcept {
            boost::asio::ip::udp::socket& s = const_cast<boost::asio::ip::udp::socket&>(socket);
            if (s.is_open()) {
                boost::system::error_code ec;
                try {
                    s.cancel(ec);
                }
                catch (std::exception&) {}
            }
        }

        void Socket::Cancel(const boost::asio::ip::tcp::socket& socket) noexcept {
            boost::asio::ip::tcp::socket& s = const_cast<boost::asio::ip::tcp::socket&>(socket);
            if (s.is_open()) {
                boost::system::error_code ec;
                try {
                    s.cancel(ec);
                }
                catch (std::exception&) {}
            }
        }

        void Socket::Cancel(const boost::asio::ip::tcp::acceptor& acceptor) noexcept {
            boost::asio::ip::tcp::acceptor& s = const_cast<boost::asio::ip::tcp::acceptor&>(acceptor);
            if (s.is_open()) {
                boost::system::error_code ec;
                try {
                    s.cancel(ec);
                }
                catch (std::exception&) {}
            }
        }

        void Socket::Cancel(const std::shared_ptr<boost::asio::ip::udp::socket>& socket) noexcept {
            if (NULL != socket) {
                Cancel(*socket);
            }
        }

        void Socket::Cancel(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket) noexcept {
            if (NULL != socket) {
                Cancel(*socket);
            }
        }

        void Socket::Cancel(const std::shared_ptr<boost::asio::ip::tcp::acceptor>& acceptor) noexcept {
            if (NULL != acceptor) {
                Cancel(*acceptor);
            }
        }

        void Socket::Closesocket(const std::shared_ptr<boost::asio::ip::udp::socket>& socket) noexcept {
            if (NULL != socket) {
                Closesocket(*socket);
            }
        }

        void Socket::Closesocket(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket) noexcept {
            if (NULL != socket) {
                Closesocket(*socket);
            }
        }

        void Socket::Closesocket(const std::shared_ptr<boost::asio::ip::tcp::acceptor>& acceptor) noexcept {
            if (NULL != acceptor) {
                Closesocket(*acceptor);
            }
        }

        void Socket::Shutdown(int fd) noexcept {
            if (fd != -1) {
                int how;
#ifdef _WIN32
                how = SD_SEND;
#else
                how = SHUT_WR;
#endif

                shutdown(fd, how);
            }
        }

        void Socket::Closesocket(int fd) noexcept {
            if (fd != -1) {
#ifdef _WIN32
                closesocket(fd);
#else   
                close(fd);
#endif
            }
        }

        int Socket::GetDefaultTTL() noexcept {
            static int defaultTtl = 0;
            static UInt64 lastTime = 0;

            UInt64 nowTime = GetTickCount();
            if (defaultTtl < 1 || (nowTime - lastTime) >= 1000 || lastTime > nowTime) {
                int ttl = 32;
                int fd = socket(AF_INET, SOCK_DGRAM, 0);
                if (fd != -1) {
                    socklen_t len = sizeof(ttl);
                    if (getsockopt(fd, SOL_IP, IP_TTL, (char*)&ttl, &len) < 0 || ttl < 1) {
                        ttl = 32;
                    }
                    close(fd);
                }
                defaultTtl = ttl;
                lastTime = nowTime;
            }
            return defaultTtl;
        }

        bool Socket::SetTypeOfService(int fd, int tos) noexcept {
            if (fd == -1) {
                return false;
            }

            if (tos < 0) {
                tos = 0x68; // FLASH
            }

            Byte b = tos;
            return ::setsockopt(fd, SOL_IP, IP_TOS, (char*)&b, sizeof(b)) == 0;
        }

        bool Socket::SetSignalPipeline(int fd, bool sigpipe) noexcept {
            if (fd == -1) {
                return false;
            }

            int err = 0;
#ifdef SO_NOSIGPIPE
            int opt = sigpipe ? 0 : 1;
            err = ::setsockopt(serverfd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&opt, sizeof(opt));
#endif
            return err == 0;
        }

        bool Socket::SetDontFragment(int fd, bool dontFragment) noexcept {
            if (fd == -1) {
                return false;
            }

            int err = 0;
#ifdef _WIN32 
            int val = dontFragment ? 1 : 0;
            err = ::setsockopt(fd, IPPROTO_IP, IP_DONTFRAGMENT, (char*)&val, sizeof(val));
#elif IP_MTU_DISCOVER
            int val = dontFragment ? IP_PMTUDISC_DO : IP_PMTUDISC_WANT;
            err = ::setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, (char*)&val, sizeof(val));
#endif
            return err == 0;
        }

        bool Socket::ReuseSocketAddress(int fd, bool reuse) noexcept {
            if (fd == -1) {
                return false;
            }
            int flag = reuse ? 1 : 0;
            return ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)) == 0;
        }

        void Socket::AdjustDefaultSocketOptional(int sockfd, bool in4) noexcept {
            if (sockfd != -1) {
                uint8_t tos = 0x68;
                if (in4) {
                    ::setsockopt(sockfd, SOL_IP, IP_TOS, (char*)&tos, sizeof(tos));

#ifdef _WIN32
                    int dont_frag = 0;
                    ::setsockopt(sockfd, IPPROTO_IP, IP_DONTFRAGMENT, (char*)&dont_frag, sizeof(dont_frag));
#elif IP_MTU_DISCOVER
                    int dont_frag = IP_PMTUDISC_WANT;
                    ::setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &dont_frag, sizeof(dont_frag));
#endif
                }
                else {
                    ::setsockopt(sockfd, SOL_IPV6, IP_TOS, (char*)&tos, sizeof(tos));

#ifdef _WIN32
                    int dont_frag = 0;
                    ::setsockopt(sockfd, IPPROTO_IPV6, IP_DONTFRAGMENT, (char*)&dont_frag, sizeof(dont_frag));
#elif IPV6_MTU_DISCOVER
                    int dont_frag = IPV6_PMTUDISC_WANT;
                    ::setsockopt(sockfd, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &dont_frag, sizeof(dont_frag));
#endif
                }
#ifdef SO_NOSIGPIPE
                int no_sigpipe = 1;
                ::setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe));
#endif
            }
        }

        // https://source.android.google.cn/devices/tech/debug/native-crash?hl=zh-cn
        // https://android.googlesource.com/platform/bionic/+/master/docs/fdsan.md
        void Socket::Closesocket(const boost::asio::ip::tcp::socket& socket) noexcept {
            boost::asio::ip::tcp::socket& s = const_cast<boost::asio::ip::tcp::socket&>(socket);
            if (s.is_open()) {
                boost::system::error_code ec;
                try {
                    s.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
                }
                catch (std::exception&) {}
                try {
                    s.close(ec);
                }
                catch (std::exception&) {}
            }
        }

        void Socket::Closesocket(const boost::asio::ip::tcp::acceptor& acceptor) noexcept {
            boost::asio::ip::tcp::acceptor& s = const_cast<boost::asio::ip::tcp::acceptor&>(acceptor);
            if (s.is_open()) {
                boost::system::error_code ec;
                try {
                    s.close(ec);
                }
                catch (std::exception&) {}
            }
        }

        void Socket::Closesocket(const boost::asio::ip::udp::socket& socket) noexcept {
            boost::asio::ip::udp::socket& s = const_cast<boost::asio::ip::udp::socket&>(socket);
            if (s.is_open()) {
                boost::system::error_code ec;
                try {
                    s.close(ec);
                }
                catch (std::exception&) {}
            }
        }

        int Socket::GetHandle(const boost::asio::ip::tcp::socket& socket) noexcept {
            boost::asio::ip::tcp::socket& s = const_cast<boost::asio::ip::tcp::socket&>(socket);
            if (!socket.is_open()) {
                return -1;
            }

            try {
                int ndfs = s.native_handle();
                return ndfs;
            }
            catch (std::exception&) {
                return -1;
            }
        }

        int Socket::GetHandle(const boost::asio::ip::tcp::acceptor& acceptor) noexcept {
            boost::asio::ip::tcp::acceptor& s = const_cast<boost::asio::ip::tcp::acceptor&>(acceptor);
            if (!s.is_open()) {
                return -1;
            }

            try {
                int ndfs = s.native_handle();
                return ndfs;
            }
            catch (std::exception&) {
                return -1;
            }
        }

        int Socket::GetHandle(const boost::asio::ip::udp::socket& socket) noexcept {
            boost::asio::ip::udp::socket& s = const_cast<boost::asio::ip::udp::socket&>(socket);
            if (!s.is_open()) {
                return -1;
            }

            try {
                int ndfs = s.native_handle();
                return ndfs;
            }
            catch (std::exception&) {
                return -1;
            }
        }

        bool Socket::AcceptLoopbackAsync(
            const Hosting&                                          hosting,
            const AsioTcpAcceptor&                                  acceptor,
            const BOOST_ASIO_MOVE_ARG(AcceptLoopbackCallback)       callback) noexcept {
            if (!acceptor || !acceptor->is_open()) {
                return false;
            }

            if (!hosting || !callback) {
                Closesocket(acceptor);
                return false;
            }

            const AsioTcpAcceptor acceptor_ = acceptor;
            const AcceptLoopbackCallback callback_ = BOOST_ASIO_MOVE_CAST(AcceptLoopbackCallback)(constantof(callback));
            if (Socket::AcceptLoopbackAsync(hosting, *acceptor,
                [acceptor_, callback_](const AsioContext& context, const AsioTcpSocket& socket) noexcept {
                    return callback_(context, socket);
                })) {
                return true;
            }
            else {
                Closesocket(acceptor);
                return false;
            }
        }

        bool Socket::AcceptLoopbackAsync(
            const Hosting&                                          hosting,
            const boost::asio::ip::tcp::acceptor&                   acceptor,
            const BOOST_ASIO_MOVE_ARG(AcceptLoopbackCallback)       callback) noexcept {
            if (!acceptor.is_open()) {
                return false;
            }

            if (!hosting || !callback) {
                Closesocket(acceptor);
                return false;
            }

            const AsioContext context_ = hosting->GetContext();
            if (!context_) {
                Closesocket(acceptor);
                return false;
            }

            boost::asio::ip::tcp::acceptor* const acceptor_ = addressof(acceptor);
            const Hosting                         hosting_ = hosting;
            const AcceptLoopbackCallback          accept_ = BOOST_ASIO_MOVE_CAST(AcceptLoopbackCallback)(constantof(callback));
            const AsioTcpSocket                   socket_ = make_shared_object<boost::asio::ip::tcp::socket>(*context_);

            acceptor_->async_accept(*socket_,
                [hosting_, context_, acceptor_, accept_, socket_](const boost::system::error_code& ec) noexcept {
                    if (ec == boost::system::errc::operation_canceled) {
                        Closesocket(*acceptor_);
                        return;
                    }

                    bool success = false;
                    do { /* boost::system::errc::connection_aborted */
                        if (ec) { /* ECONNABORTED */
                            break;
                        }

                        int handle_ = socket_->native_handle();
                        Socket::AdjustDefaultSocketOptional(handle_, false);
                        Socket::SetTypeOfService(handle_);
                        Socket::SetSignalPipeline(handle_, false);
                        Socket::SetDontFragment(handle_, false);
                        Socket::ReuseSocketAddress(handle_, true);

                        /* Accept Socket?? */
                        success = accept_(context_, socket_);
                    } while (0);
                    if (!success) {
                        Closesocket(socket_);
                    }

                    success = AcceptLoopbackAsync(hosting_, *acceptor_, forward0f(accept_));
                    if (!success) {
                        Closesocket(*acceptor_);
                    }
                });
            return true;
        }

        bool Socket::OpenAcceptor(
            const boost::asio::ip::tcp::acceptor&                   acceptor,
            const boost::asio::ip::address&                         listenIP,
            int                                                     listenPort,
            int                                                     backlog,
            bool                                                    fastOpen,
            bool                                                    noDelay) noexcept {
            typedef uds::net::IPEndPoint IPEndPoint;

            if (listenPort < IPEndPoint::MinPort || listenPort > IPEndPoint::MaxPort) {
                listenPort = IPEndPoint::MinPort;
            }

            boost::asio::ip::address address_ = listenIP;
            if (address_.is_unspecified() || address_.is_multicast()) {
                address_ = boost::asio::ip::address_v6::any();
            }

            boost::asio::ip::tcp::acceptor& acceptor_ = const_cast<boost::asio::ip::tcp::acceptor&>(acceptor);
            if (acceptor_.is_open()) {
                return false;
            }

            boost::system::error_code ec;
            if (address_.is_v4()) {
                acceptor_.open(boost::asio::ip::tcp::v4(), ec);
            }
            else {
                acceptor_.open(boost::asio::ip::tcp::v6(), ec);
            }

            if (ec) {
                return false;
            }

            int handle = acceptor_.native_handle();
            uds::net::Socket::AdjustDefaultSocketOptional(handle, false);
            uds::net::Socket::SetTypeOfService(handle);
            uds::net::Socket::SetSignalPipeline(handle, false);
            uds::net::Socket::SetDontFragment(handle, false);
            uds::net::Socket::ReuseSocketAddress(handle, listenPort);

            acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(listenPort), ec);
            if (ec) {
                return false;
            }

            acceptor_.set_option(boost::asio::ip::tcp::no_delay(noDelay), ec);
            acceptor_.set_option(boost::asio::detail::socket_option::boolean<IPPROTO_TCP, TCP_FASTOPEN>(fastOpen), ec);

            acceptor_.bind(boost::asio::ip::tcp::endpoint(address_, listenPort), ec);
            if (ec) {
                if (listenPort != IPEndPoint::MinPort) {
                    acceptor_.bind(boost::asio::ip::tcp::endpoint(address_, IPEndPoint::MinPort), ec);
                    if (ec) {
                        return false;
                    }
                }
            }

            if (backlog < 1) {
                backlog = 511;
            }

            acceptor_.listen(backlog, ec);
            if (ec) {
                return false;
            }
            return true;
        }

        bool Socket::OpenSocket(
            const boost::asio::ip::udp::socket&                     socket,
            const boost::asio::ip::address&                         listenIP,
            int                                                     listenPort) noexcept {
            typedef uds::net::IPEndPoint IPEndPoint;

            if (listenPort < IPEndPoint::MinPort || listenPort > IPEndPoint::MaxPort) {
                listenPort = IPEndPoint::MinPort;
            }

            boost::asio::ip::address address_ = listenIP;
            if (address_.is_multicast()) {
                address_ = boost::asio::ip::address_v6::any();
            }

            boost::asio::ip::udp::socket& socket_ = const_cast<boost::asio::ip::udp::socket&>(socket);
            if (socket_.is_open()) {
                return false;
            }

            boost::system::error_code ec;
            if (address_.is_v4()) {
                socket_.open(boost::asio::ip::udp::v4(), ec);
            }
            else {
                socket_.open(boost::asio::ip::udp::v6(), ec);
            }

            if (ec) {
                return false;
            }

            int handle = socket_.native_handle();
            uds::net::Socket::AdjustDefaultSocketOptional(handle, false);
            uds::net::Socket::SetTypeOfService(handle);
            uds::net::Socket::SetSignalPipeline(handle, false);
            uds::net::Socket::SetDontFragment(handle, false);
            uds::net::Socket::ReuseSocketAddress(handle, listenPort);

            socket_.set_option(boost::asio::ip::udp::socket::reuse_address(listenPort), ec);
            if (ec) {
                return false;
            }

            socket_.bind(boost::asio::ip::udp::endpoint(address_, listenPort), ec);
            if (ec) {
                if (listenPort != IPEndPoint::MinPort) {
                    socket_.bind(boost::asio::ip::udp::endpoint(address_, IPEndPoint::MinPort), ec);
                    if (ec) {
                        return false;
                    }
                }
            }
            return true;
        }

        void Socket::AdjustSocketOptional(const boost::asio::ip::tcp::socket& socket, bool fastOpen, bool noDealy) noexcept {
            boost::asio::ip::tcp::socket& s = const_cast<boost::asio::ip::tcp::socket&>(socket);
            int handle = s.native_handle();

            uds::net::Socket::AdjustDefaultSocketOptional(handle, false);
            uds::net::Socket::SetTypeOfService(handle);
            uds::net::Socket::SetSignalPipeline(handle, false);
            uds::net::Socket::SetDontFragment(handle, false);
            uds::net::Socket::ReuseSocketAddress(handle, true);

            boost::system::error_code ec;
            s.set_option(boost::asio::ip::tcp::no_delay(noDealy), ec);
            s.set_option(boost::asio::detail::socket_option::boolean<IPPROTO_TCP, TCP_FASTOPEN>(fastOpen), ec);
        }

        void Socket::AdjustSocketOptional(const boost::asio::ip::udp::socket& socket) noexcept {
            boost::asio::ip::udp::socket& s = const_cast<boost::asio::ip::udp::socket&>(socket);
            int handle = s.native_handle();

            uds::net::Socket::AdjustDefaultSocketOptional(handle, false);
            uds::net::Socket::SetTypeOfService(handle);
            uds::net::Socket::SetSignalPipeline(handle, false);
            uds::net::Socket::SetDontFragment(handle, false);
            uds::net::Socket::ReuseSocketAddress(handle, true);
        }
    }
}