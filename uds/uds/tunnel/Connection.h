#pragma once

#include <uds/IDisposable.h>
#include <uds/threading/Hosting.h>
#include <uds/io/Stream.h>
#include <uds/io/BinaryReader.h>
#include <uds/io/MemoryStream.h>
#include <uds/net/Socket.h>
#include <uds/net/IPEndPoint.h>
#include <uds/transmission/ITransmission.h>
#include <uds/configuration/AppConfiguration.h>

namespace uds {
    namespace tunnel {
        class Connection : public IDisposable {
        public:
            using ITransmission                 = uds::transmission::ITransmission;
            using ITransmissionPtr              = std::shared_ptr<ITransmission>;
            using AppConfiguration              = uds::configuration::AppConfiguration;
            using AppConfigurationPtr           = std::shared_ptr<AppConfiguration>;
            using Socket                        = uds::net::Socket;
            using IPEndPoint                    = uds::net::IPEndPoint;
            using AsyncTcpSocketPtr             = Socket::AsioTcpSocket;
            using AsyncTcpSocket                = boost::asio::ip::tcp::socket;
            using AsyncContextPtr               = std::shared_ptr<boost::asio::io_context>;
            using AsyncResolverPtr              = std::shared_ptr<boost::asio::ip::tcp::resolver>;
            using AsyncDeadlineTimerPtr         = std::shared_ptr<boost::asio::deadline_timer>;
            using Stream                        = uds::io::Stream;
            using BinaryReader                  = uds::io::BinaryReader;
            using MemoryStream                  = uds::io::MemoryStream;

        public:
            using DisposedEventHandler          = std::function<void(Connection*)>;
            using AcceptAsyncCallback           = std::function<void(bool, int)>;
            using AcceptAsyncMeasureChannelId   = std::function<int(const ITransmissionPtr&)>;
            using ConnectAsyncCallback          = AcceptAsyncCallback;
            using HelloAsyncCallback            = std::function<void(bool)>;

        public:
            const int                           ECONNECTION_MSS = uds::threading::Hosting::BufferSize;
            const int                           Id;
            DisposedEventHandler                DisposedEvent;

        public:
            Connection(const AppConfigurationPtr& configuration, int id, const ITransmissionPtr& inbound, const ITransmissionPtr& outbound) noexcept;

        public:
            virtual bool                        Listen(const AsyncTcpSocketPtr& network) noexcept;
            virtual void                        Dispose() noexcept override;
            void                                Close() noexcept;
            bool                                Available() noexcept;

        public:
            static bool                         AcceptAsync(const ITransmissionPtr& inbound, int alignment, AcceptAsyncMeasureChannelId&& measure, AcceptAsyncCallback&& handler) noexcept;
            static bool                         AcceptAsync(const ITransmissionPtr& outbound, AcceptAsyncCallback&& handler) noexcept;
            static bool                         ConnectAsync(const ITransmissionPtr& outbound, int alignment, int channelId, ConnectAsyncCallback&& handler) noexcept;
            static bool                         ConnectAsync(const ITransmissionPtr& inbound, ConnectAsyncCallback&& handler) noexcept;
            static bool                         HelloAsync(const ITransmissionPtr& outbound) noexcept;
            static bool                         HelloAsync(const ITransmissionPtr& inbound, HelloAsyncCallback&& handler) noexcept;

        private:
            static bool                         PackPlaintextHeaders(Stream& stream, int channelId, int alignment) noexcept;
            static Int64                        UnpackPlaintextLength(const void* buffer, int offset, int length) noexcept;
            static bool                         HandshakeClient(const ITransmissionPtr& transmission, ConnectAsyncCallback&& handler) noexcept;
            static bool                         HandshakeServer(const ITransmissionPtr& transmission, int alignment, int channelId, AcceptAsyncCallback&& handler) noexcept;

        private:
            bool                                IsNone() noexcept;
            bool                                IsDisposed() noexcept;
            AsyncContextPtr                     GetContext() noexcept;

        public:
            static AsyncTcpSocketPtr            NewRemoteSocket(const AppConfigurationPtr& configuration, const AsyncContextPtr& context) noexcept;
            static AsyncTcpSocketPtr            NewRemoteSocket(const AppConfigurationPtr& configuration, const AsyncContextPtr& context, const boost::asio::ip::tcp::endpoint& remoteEP) noexcept;

        private:
            bool                                EstablishRemoteSocket() noexcept;
            bool                                KeepAlivedReadCycle(const ITransmissionPtr& transmission) noexcept;
            bool                                KeepAlivedSendCycle(const ITransmissionPtr& transmission) noexcept;
            bool                                ConnectRemoteSocket(boost::asio::ip::tcp::endpoint remoteEP) noexcept;

        private:
            bool                                RemoteSocketToOutboundSocket() noexcept;
            bool                                InboundSocketToRemoteSocket() noexcept;

        private:
            bool                                SendToRemoteSocket(const void* buffer, int length) noexcept;
            bool                                SendToOutboundSocket(const void* buffer, int length) noexcept;

        private:
            std::atomic<bool>                   disposed_;
            bool                                available_;
            ITransmissionPtr                    inbound_;
            ITransmissionPtr                    outbound_;
            AppConfigurationPtr                 configuration_;
            AsyncTcpSocketPtr                   remote_;
            AsyncResolverPtr                    resolver_;
            AsyncDeadlineTimerPtr               timeout_;
            std::shared_ptr<Byte>               buffers_;
        };
    }
}