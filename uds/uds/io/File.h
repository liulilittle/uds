#pragma once

#include <uds/stdafx.h>

namespace uds {
    namespace io {
        enum FileAccess {
            Read                                = 1,
            Write                               = 2,
            ReadWrite                           = 3,
        };

        class File final {
        public:
            static bool                         CanAccess(const char* path, FileAccess access_) noexcept;
            static int                          GetLength(const char* path) noexcept;
            static bool                         Exists(const char* path) noexcept;
            static int                          GetEncoding(const void* p, int length, int& offset) noexcept;
            static std::shared_ptr<Byte>        ReadAllBytes(const char* path, int& length) noexcept;
            static bool                         WriteAllBytes(const char* path, const void* data, int length) noexcept;
        };
    }
}