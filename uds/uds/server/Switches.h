#pragma once

#include <uds/IDisposable.h>
#include <uds/threading/Hosting.h>
#include <uds/configuration/AppConfiguration.h>
#include <uds/net/Socket.h>
#include <uds/net/IPEndPoint.h>
#include <uds/tunnel/Connection.h>
#include <uds/transmission/ITransmission.h>

namespace uds {
    namespace server {
        class Switches : public IDisposable {
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

        private:
            class ConnectionChannel final {
            public:
                int                                                             channel_;
                ITransmissionPtr                                                inbound_;
                AsioTcpSocket                                                   network_;

            public:
                void                                                            Close() noexcept;
            };
            using ConnectionChannelPtr                                          = std::shared_ptr<ConnectionChannel>;
            using ConnectionChannelTable                                        = std::unordered_map<int, ConnectionChannelPtr>;

        public:
            Switches(const std::shared_ptr<uds::threading::Hosting>& hosting, const std::shared_ptr<uds::configuration::AppConfiguration>& configuration) noexcept;

        public:
            const std::shared_ptr<boost::asio::io_context>&                     GetContext() noexcept;
            const std::shared_ptr<uds::threading::Hosting>&                     GetHosting() noexcept;
            const std::shared_ptr<uds::configuration::AppConfiguration>&        GetConfiguration() noexcept;
            const boost::asio::ip::tcp::endpoint                                GetLocalEndPoint(bool RX) noexcept;

        public:
            virtual bool                                                        Listen() noexcept;
            virtual void                                                        Dispose() noexcept override;

        private:
            std::shared_ptr<boost::asio::ip::tcp::acceptor>                     OpenAcceptor(const uds::net::IPEndPoint& bindEP, const uds::net::Socket::AcceptLoopbackCallback&& loopback) noexcept;
            void                                                                CloseAcceptor() noexcept;

        private:
            bool                                                                InboundAcceptClient(const AsioContext& context, const AsioTcpSocket& socket) noexcept;
            bool                                                                OutboundAcceptClient(const AsioContext& context, const AsioTcpSocket& socket) noexcept;
            
        private:
            bool                                                                HandshakeTransmission(const ITransmissionPtr& transmission, HandshakeAsyncCallback&& callback) noexcept;
            bool                                                                HandshakeTransmission(const AsioContext& context, const AsioTcpSocket& socket, HandshakeAsyncCallback&& callback) noexcept;

        private:
            bool                                                                ClearTimeout(void* key) noexcept;
            bool                                                                AddTimeout(void* key, std::shared_ptr<boost::asio::deadline_timer>&& timeout) noexcept;
            
        private:
            ConnectionChannelPtr                                                PopChannel(int channel) noexcept;
            ConnectionChannelPtr                                                AllocChannel(const AsioTcpSocket& network, const ITransmissionPtr& inbound) noexcept;
            bool                                                                CloseChannel(int channel) noexcept;
            bool                                                                AcceptChannel(int channel, const ITransmissionPtr& outbound) noexcept;

        protected:
            virtual ITransmissionPtr                                            CreateTransmission(const AsioContext& context, const AsioTcpSocket& socket) noexcept;
            virtual ConnectionPtr                                               CreateConnection(int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept;
            virtual bool                                                        Accept(int channel, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept;

        private:
            std::atomic<bool>                                                   disposed_;
            std::atomic<int>                                                    channel_;
            std::shared_ptr<uds::threading::Hosting>                            hosting_;
            std::shared_ptr<uds::configuration::AppConfiguration>               configuration_;
            std::shared_ptr<boost::asio::io_context>                            context_;
            std::shared_ptr<boost::asio::ip::tcp::acceptor>                     acceptor_[2];
            TimeoutTable                                                        timeouts_;
            ConnectionChannelTable                                              channels_;
            ConnectionTable                                                     connections_;
        };
    }
}