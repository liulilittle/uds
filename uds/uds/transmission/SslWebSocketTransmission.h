#pragma once

#include <uds/transmission/Transmission.h>

namespace uds {
    namespace transmission {
        class SslWebSocketTransmission : public Transmission {
            typedef boost::asio::ip::tcp::socket                            AsioTcpSocket;
            typedef boost::asio::ssl::stream<AsioTcpSocket>                 SslvTcpSocket;
            typedef boost::beast::websocket::stream<SslvTcpSocket>          SslvWebSocket;

        private:
            SslWebSocketTransmission(
                const std::shared_ptr<uds::threading::Hosting>&             hosting,
                const std::shared_ptr<boost::asio::io_context>&             context,
                const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
                bool                                                        verify_peer,
                const std::string&                                          host,
                const std::string&                                          path,
                const std::string&                                          certificate_file,
                const std::string&                                          certificate_key_file,
                const std::string&                                          certificate_chain_file,
                const std::string&                                          certificate_key_password,
                const std::string&                                          ciphersuites,
                int                                                         alignment) noexcept;

        public:
            SslWebSocketTransmission(
                const std::shared_ptr<uds::threading::Hosting>&             hosting, 
                const std::shared_ptr<boost::asio::io_context>&             context, 
                const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
                bool                                                        verify_peer,
                const std::string&                                          host,
                const std::string&                                          path,
                const std::string&                                          ciphersuites,
                int                                                         alignment) noexcept;

            SslWebSocketTransmission(
                const std::shared_ptr<uds::threading::Hosting>&             hosting, 
                const std::shared_ptr<boost::asio::io_context>&             context, 
                const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
                const std::string&                                          host,
                const std::string&                                          path,
                const std::string&                                          certificate_file,
                const std::string&                                          certificate_key_file,
                const std::string&                                          certificate_chain_file,
                const std::string&                                          certificate_key_password,
                const std::string&                                          ciphersuites,
                int                                                         alignment) noexcept;

        public:
            virtual void                                                    Dispose() noexcept override;
            virtual bool                                                    HandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept override;
            virtual bool                                                    WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept override;
            virtual bool                                                    ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept override;

        protected:
            virtual bool                                                    OnWriteAsync(const BOOST_ASIO_MOVE_ARG(pmessage) message) noexcept override;

        private:
            std::atomic<bool>                                               disposed_;
            std::shared_ptr<boost::asio::ssl::context>                      ssl_context_;
            std::shared_ptr<SslvWebSocket>                                  ssl_websocket_;
            bool                                                            verify_peer_;
            std::string                                                     host_;
            std::string                                                     path_;
            std::string                                                     certificate_file_;
            std::string                                                     certificate_key_file_;
            std::string                                                     certificate_chain_file_;
            std::string                                                     certificate_key_password_;
            std::string                                                     ciphersuites_;
        };
    }
}