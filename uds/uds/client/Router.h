#pragma once

#include <uds/IDisposable.h>
#include <uds/threading/Hosting.h>
#include <uds/configuration/AppConfiguration.h>
#include <uds/net/Socket.h>
#include <uds/net/IPEndPoint.h>
#include <uds/tunnel/Connection.h>
#include <uds/transmission/ITransmission.h>

namespace uds {
    namespace client {
        class Router : public IDisposable {
            using Socket                                                        = uds::net::Socket;
            using AsioContext                                                   = Socket::AsioContext;
            using AsioTcpSocket                                                 = Socket::AsioTcpSocket;
            using ITransmission                                                 = uds::transmission::ITransmission;
            using ITransmissionPtr                                              = std::shared_ptr<ITransmission>;
            using AppConfiguration                                              = uds::configuration::AppConfiguration;
            using HandshakeAsyncCallback                                        = std::function<void(const ITransmissionPtr&, bool)>;
            using TimeoutPtr                                                    = std::shared_ptr<boost::asio::deadline_timer>;
            using TimeoutTable                                                  = std::unordered_map<void*, TimeoutPtr>;
            using Connection                                                    = uds::tunnel::Connection;
            using ConnectionPtr                                                 = std::shared_ptr<Connection>;
            using ConnectionTable                                               = std::unordered_map<int, ConnectionPtr>;
            using ConnectClientAsyncCallback                                    = std::function<bool(const std::shared_ptr<boost::asio::ip::tcp::socket>&, bool)>;
            using ConnectTransmissionAsyncCallback                              = std::function<bool(const ITransmissionPtr&)>;
            using ConnectConnectionAsyncCallback                                = std::function<bool(const ITransmissionPtr&, int)>;

        public:
            Router(const std::shared_ptr<uds::threading::Hosting>& hosting, const std::shared_ptr<uds::configuration::AppConfiguration>& configuration) noexcept;

        public:
            const std::shared_ptr<boost::asio::io_context>&                     GetContext() noexcept;
            const std::shared_ptr<uds::threading::Hosting>&                     GetHosting() noexcept;
            const std::shared_ptr<uds::configuration::AppConfiguration>&        GetConfiguration() noexcept;
            const boost::asio::ip::tcp::endpoint                                GetLocalEndPoint() noexcept;

        private:
            bool                                                                HandshakeTransmission(const ITransmissionPtr& transmission, HandshakeAsyncCallback&& callback) noexcept;
            bool                                                                HandshakeTransmission(const AsioContext& context, const AsioTcpSocket& socket, HandshakeAsyncCallback&& callback) noexcept;

        private:
            bool                                                                ClearTimeout(void* key) noexcept;
            bool                                                                AddTimeout(void* key, std::shared_ptr<boost::asio::deadline_timer>&& timeout) noexcept;

        private:
            std::shared_ptr<boost::asio::ip::tcp::acceptor>                     OpenAcceptor(const uds::net::IPEndPoint& bindEP, const uds::net::Socket::AcceptLoopbackCallback&& loopback) noexcept;
            void                                                                CloseAcceptor() noexcept;

        private:
            bool                                                                AcceptClient(const AsioContext& context, const AsioTcpSocket& socket) noexcept;
            bool                                                                ConnectClient(const AsioContext& context, const boost::asio::ip::tcp::endpoint& remoteEP, ConnectClientAsyncCallback&& callback) noexcept;
            bool                                                                ConnectTransmission(const AsioContext& context, const boost::asio::ip::tcp::endpoint& remoteEP, ConnectTransmissionAsyncCallback&& callback) noexcept;
            bool                                                                ConnectConnection(const AsioContext& context, int channelId, const boost::asio::ip::tcp::endpoint& remoteEP, ConnectConnectionAsyncCallback&& callback) noexcept;

        public:
            virtual bool                                                        Listen() noexcept;
            virtual void                                                        Dispose() noexcept override;

        protected:
            virtual ITransmissionPtr                                            CreateTransmission(const AsioContext& context, const AsioTcpSocket& socket) noexcept;
            virtual ConnectionPtr                                               CreateConnection(int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept;
            virtual bool                                                        Accept(const AsioTcpSocket& network, int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept;

        private:
            std::atomic<bool>                                                   disposed_;
            std::atomic<int>                                                    channel_;
            std::shared_ptr<uds::threading::Hosting>                            hosting_;
            std::shared_ptr<uds::configuration::AppConfiguration>               configuration_;
            std::shared_ptr<boost::asio::io_context>                            context_;
            std::shared_ptr<boost::asio::ip::tcp::acceptor>                     acceptor_;
            TimeoutTable                                                        timeouts_;
            ConnectionTable                                                     connections_;
        };
    }
}