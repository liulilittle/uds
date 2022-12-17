#pragma once

#include <uds/stdafx.h>

namespace uds {
    namespace cryptography {
        class Encryptor final {
        public:
            Encryptor(const std::string& method, const std::string& password) noexcept;

        public:
            static void                                         Initialize() noexcept;
            static bool                                         Support(const std::string& method) noexcept;

        public:
            std::shared_ptr<Byte>                               Encrypt(Byte* data, int datalen, int& outlen) noexcept;
            std::shared_ptr<Byte>                               Decrypt(Byte* data, int datalen, int& outlen) noexcept;

        private:
            bool                                                initCipher(std::shared_ptr<EVP_CIPHER_CTX>& context, int enc, int raise);
            void                                                initKey(const std::string& method, const std::string password);

        private:
            const EVP_CIPHER*                                   _cipher;
            std::shared_ptr<Byte>                               _key; // _cipher->key_len
            std::shared_ptr<Byte>                               _iv;
            std::string                                         _method;
            std::string                                         _password;
            std::shared_ptr<EVP_CIPHER_CTX>                     _encryptCTX;
            std::shared_ptr<EVP_CIPHER_CTX>                     _decryptCTX;
        };
    }
}