#pragma once

#include <uds/threading/Hosting.h>

namespace uds {
    namespace net {
        class Socket final {
        public:
            typedef std::shared_ptr<uds::threading::Hosting>                            Hosting;
            typedef std::shared_ptr<boost::asio::ip::tcp::socket>                       AsioTcpSocket;
            typedef std::shared_ptr<boost::asio::io_context>                            AsioContext;
            typedef std::shared_ptr<boost::asio::ip::tcp::acceptor>                     AsioTcpAcceptor;
            typedef std::shared_ptr<boost::asio::ip::udp::socket>                       AsioUdpSocket;
            typedef std::function<bool(const AsioContext&, const AsioTcpSocket&)>       AcceptLoopbackCallback;
            typedef std::function<bool(const std::shared_ptr<Byte>&, int)>              ReceiveFromLoopbackCallback;

        public:
            enum SelectMode {
                SelectMode_SelectRead,
                SelectMode_SelectWrite,
                SelectMode_SelectError,
            };
            static bool                                                                 PolH(int s, int64_t microSeconds, SelectMode mode) noexcept;
            static bool                                                                 Poll(int s, int milliSeconds, SelectMode mode) noexcept;

        public:
            static void                                                                 Shutdown(int fd) noexcept;
            static void                                                                 Closesocket(int fd) noexcept;

        public:
            static bool                                                                 AcceptLoopbackAsync(
                const Hosting&                                                          hosting,
                const AsioTcpAcceptor&                                                  acceptor,
                const BOOST_ASIO_MOVE_ARG(AcceptLoopbackCallback)                       callback) noexcept;
            static bool                                                                 AcceptLoopbackAsync(
                const Hosting&                                                          hosting,
                const boost::asio::ip::tcp::acceptor&                                   acceptor,
                const BOOST_ASIO_MOVE_ARG(AcceptLoopbackCallback)                       callback) noexcept;
            static bool                                                                 OpenAcceptor(
                const boost::asio::ip::tcp::acceptor&                                   acceptor,
                const boost::asio::ip::address&                                         listenIP,
                int                                                                     listenPort,
                int                                                                     backlog,
                bool                                                                    fastOpen,
                bool                                                                    noDelay) noexcept;
            static bool                                                                 OpenSocket(
                const boost::asio::ip::udp::socket&                                     socket,
                const boost::asio::ip::address&                                         listenIP,
                int                                                                     listenPort) noexcept;

        public:
            static void                                                                 Cancel(const boost::asio::ip::udp::socket& socket) noexcept;
            static void                                                                 Cancel(const boost::asio::ip::tcp::socket& socket) noexcept;
            static void                                                                 Cancel(const boost::asio::ip::tcp::acceptor& acceptor) noexcept;

        public:
            static void                                                                 Cancel(const std::shared_ptr<boost::asio::ip::udp::socket>& socket) noexcept;
            static void                                                                 Cancel(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket) noexcept;
            static void                                                                 Cancel(const std::shared_ptr<boost::asio::ip::tcp::acceptor>& acceptor) noexcept;

        public:
            static void                                                                 Closesocket(const std::shared_ptr<boost::asio::ip::udp::socket>& socket) noexcept;
            static void                                                                 Closesocket(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket) noexcept;
            static void                                                                 Closesocket(const std::shared_ptr<boost::asio::ip::tcp::acceptor>& acceptor) noexcept;

        public:
            template<class TSocket>
            inline static int                                                           LocalPort(const TSocket& socket) noexcept {
                boost::system::error_code ec;
                int port = const_cast<TSocket&>(socket).local_endpoint(ec).port();
                return ec ? 0 : port;
            }
            template<class TSocket>
            inline static int                                                           RemotePort(const TSocket& socket) noexcept {
                boost::system::error_code ec;
                int port = const_cast<TSocket&>(socket).remote_endpoint(ec).port();
                return ec ? 0 : port;
            }

        public:
            static void                                                                 AdjustDefaultSocketOptional(int sockfd, bool in4) noexcept;
            static void                                                                 AdjustSocketOptional(const boost::asio::ip::tcp::socket& socket, bool fastOpen, bool noDealy) noexcept;
            static void                                                                 AdjustSocketOptional(const boost::asio::ip::udp::socket& socket) noexcept;

        public:
            static int                                                                  GetDefaultTTL() noexcept;
            static bool                                                                 SetTypeOfService(int fd, int tos = ~0) noexcept;
            static bool                                                                 SetSignalPipeline(int fd, bool sigpipe) noexcept;
            static bool                                                                 SetDontFragment(int fd, bool dontFragment) noexcept;
            static bool                                                                 ReuseSocketAddress(int fd, bool reuse) noexcept;

        public:
            static int                                                                  GetHandle(const boost::asio::ip::tcp::acceptor& acceptor) noexcept;
            static int                                                                  GetHandle(const boost::asio::ip::tcp::socket& socket) noexcept;
            static int                                                                  GetHandle(const boost::asio::ip::udp::socket& socket) noexcept;

        public:
            static void                                                                 Closesocket(const boost::asio::ip::tcp::acceptor& acceptor) noexcept;
            static void                                                                 Closesocket(const boost::asio::ip::tcp::socket& socket) noexcept;
            static void                                                                 Closesocket(const boost::asio::ip::udp::socket& socket) noexcept;
        };
    }
}