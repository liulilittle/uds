#pragma once

#include <uds/transmission/Transmission.h>
#include <uds/cryptography/Encryptor.h>

namespace uds {
    namespace transmission {
        class EncryptorTransmission : public Transmission {
        public:
            EncryptorTransmission(
                const std::shared_ptr<uds::threading::Hosting>&             hosting, 
                const std::shared_ptr<boost::asio::io_context>&             context, 
                const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
                const std::string&                                          method,
                const std::string&                                          password, 
                int                                                         alignment) noexcept;

        public:
            virtual bool                                                    WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept override;
            virtual bool                                                    ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept override;

        private:
            std::atomic<bool>                                               disposed_;
            uds::cryptography::Encryptor                                    encryptor_;
        };
    }
}