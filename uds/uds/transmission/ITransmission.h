#pragma once

#include <uds/IDisposable.h>
#include <uds/net/IPEndPoint.h>

namespace uds {
    namespace transmission {
        class ITransmission : public IDisposable {
        public:
            typedef std::function<void(const std::shared_ptr<Byte>&, int)>      ReadAsyncCallback;
            typedef std::function<void(bool)>                                   WriteAsyncCallback;
            typedef WriteAsyncCallback                                          HandshakeAsyncCallback;
            typedef enum {
                HandshakeType_Server,
                HandshakeType_Client,
            }                                                                   HandshakeType;

        public:
            virtual                                                             ~ITransmission() noexcept override;

        public:
            void                                                                Close() noexcept;

        public:
            virtual bool                                                        HandshakeAsync(
                HandshakeType                                                   type,
                const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback)               callback) = 0;
            virtual bool                                                        ReadAsync(
                const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback)                    callback) = 0;
            virtual bool                                                        WriteAsync(
                const std::shared_ptr<Byte>&                                    buffer,
                int                                                             offset, 
                int                                                             length, 
                const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback)                   callback) = 0;

        public:
            virtual std::shared_ptr<boost::asio::io_context>                    GetContext() = 0;
            virtual uds::net::IPEndPoint                                        GetLocalEndPoint() = 0;
            virtual uds::net::IPEndPoint                                        GetRemoteEndPoint() = 0;
        };
    }
}