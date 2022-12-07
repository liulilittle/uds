#include <uds/transmission/EncryptorTransmission.h>

namespace uds {
    namespace transmission {
        EncryptorTransmission::EncryptorTransmission(
            const std::shared_ptr<uds::threading::Hosting>&             hosting, 
            const std::shared_ptr<boost::asio::io_context>&             context, 
            const std::shared_ptr<boost::asio::ip::tcp::socket>&        socket,
            const std::string&                                          method,
            const std::string&                                          password,
            int                                                         alignment) noexcept
            : Transmission(hosting, context, socket, alignment)
            , encryptor_(true, method, password) {
            
        }

        bool EncryptorTransmission::WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept {
            if (!buffer || offset < 0 || length < 1) {
                return false;
            }

            int outlen;
            const std::shared_ptr<Byte> packet = encryptor_.Encrypt(buffer.get() + offset, length, outlen);
            if (!packet || outlen < 1) {
                return false;
            }

            const WriteAsyncCallback callback_ = BOOST_ASIO_MOVE_CAST(WriteAsyncCallback)(constantof(callback));
            return Transmission::WriteAsync(packet, 0, outlen,
                [callback_](bool success) noexcept {
                    if (callback_) {
                        callback_(success);
                    }
                });
        }
        
        bool EncryptorTransmission::ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept {
            if (!callback) {
                return false;
            }

            const ReadAsyncCallback callback_ = BOOST_ASIO_MOVE_CAST(ReadAsyncCallback)(constantof(callback));
            return Transmission::ReadAsync(
                [callback_, this](const std::shared_ptr<Byte>& buffer, int length) noexcept {
                    if (!buffer || length < 1) {
                        callback_(buffer, length);
                        return;
                    }

                    int outlen;
                    const std::shared_ptr<Byte> packet = encryptor_.Decrypt(buffer.get(), length, outlen);
                    if (!packet || outlen < 1) {
                        callback_(NULL, -1);
                    }
                    else {
                        callback_(packet, outlen);
                    }
                });
        }
    }
}