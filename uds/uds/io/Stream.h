#pragma once

#include <uds/stdafx.h>
#include <uds/io/SeekOrigin.h>

namespace uds {
    namespace io {
        class Stream {
        public:
            virtual bool                        CanSeek() = 0;
            virtual bool                        CanRead() = 0;
            virtual bool                        CanWrite() = 0;

        public:
            virtual int                         GetPosition() = 0;
            virtual int                         GetLength() = 0;
            virtual bool                        Seek(int offset, SeekOrigin loc) = 0;
            virtual bool                        SetPosition(int position)  = 0;
            virtual bool                        SetLength(int value) = 0;

        public:
            virtual bool                        WriteByte(Byte value) = 0;
            virtual bool                        Write(const void* buffer, int offset, int count) = 0;

        public:
            virtual int                         ReadByte() = 0;
            virtual int                         Read(const void* buffer, int offset, int count) = 0;

        public:
            inline void                         Close() noexcept { this->Dispose(); }
            virtual void                        Dispose() = 0;
        };
    }
}