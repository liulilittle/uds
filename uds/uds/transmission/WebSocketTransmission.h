#pragma once

#include <uds/transmission/Transmission.h>

namespace uds {
    namespace transmission {
        class WebSocketTransmission : public Transmission {
            typedef boost::asio::ip::tcp::socket                        AsioTcpSocket;
            typedef boost::beast::websocket::stream<AsioTcpSocket>      AsioWebSocket;

        public:
            WebSocketTransmission(
                const std::shared_ptr<uds::threading::Hosting>&         hosting, 
                const std::shared_ptr<boost::asio::io_context>&         context, 
                const std::shared_ptr<boost::asio::ip::tcp::socket>&    socket,
                const std::string&                                      host,
                const std::string&                                      path,
                int                                                     alignment) noexcept;
        
        public:
            virtual void                                                Dispose() noexcept override;
            virtual bool                                                HandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept override;
            virtual bool                                                WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept override;
            virtual bool                                                ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept override;

        public:
            static std::string                                          WSSOF(
                const std::string&                                      url,
                std::string&                                            host,
                std::string&                                            addr,
                std::string&                                            path,
                int&                                                    port,
                bool&                                                   tlsv) noexcept;

        protected:
            virtual bool                                                OnWriteAsync(const BOOST_ASIO_MOVE_ARG(pmessage) message) noexcept override;

        private:
            std::atomic<bool>                                           disposed_;
            std::string                                                 host_;
            std::string                                                 path_;
            AsioWebSocket                                               websocket_;
        };
    }
}