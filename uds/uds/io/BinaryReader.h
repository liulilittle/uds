#pragma once

#include <uds/stdafx.h>
#include <uds/io/Stream.h>

namespace uds {
    namespace io {
        class BinaryReader {
        public:
            inline BinaryReader(Stream& stream) noexcept
                : _stream(stream) {

            }

        public:
            inline int                                      Read(const void* buffer, int offset, int length) noexcept {
                return _stream.Read(buffer, offset, length);
            }
            template<typename TValueType>   
            inline std::shared_ptr<TValueType>              ReadValues(int counts) noexcept {
                if (counts < 1) {
                    return NULL;
                }

                std::shared_ptr<TValueType> buf = make_shared_alloc<TValueType>(counts);
                if (NULL == buf) {
                    return NULL;
                }

                int size = counts * sizeof(TValueType);
                int len = _stream.Read(buf.get(), 0, size);
                if (len < 0 || len != size) {
                    return NULL;
                }
                return buf;
            }
            inline std::shared_ptr<Byte>                    ReadBytes(int counts) noexcept {
                return ReadValues<Byte>(counts);
            }
            template<typename TValueType>   
            inline bool                                     TryReadValue(TValueType& out) noexcept {
                TValueType* p = (TValueType*)&reinterpret_cast<const char&>(out);
                int len = _stream.Read(p, 0, sizeof(TValueType));
                return (size_t)len == sizeof(TValueType);
            }
            template<typename TValueType>           
            inline TValueType                               ReadValue() {
                TValueType out;
                if (!TryReadValue(out)) {
                    throw std::runtime_error("Unable to read from stream to TValueType size values");
                }
                return out;
            }
            inline Stream&                                  GetStream() noexcept { return _stream; }

        public:
            inline Int16                                    ReadInt16() noexcept { return ReadValue<Int16>(); }
            inline Int32                                    ReadInt32() noexcept { return ReadValue<Int32>(); }
            inline Int64                                    ReadInt64() noexcept { return ReadValue<Int64>(); }
            inline UInt16                                   ReadUInt16() noexcept { return ReadValue<UInt16>(); }
            inline UInt32                                   ReadUInt32() noexcept { return ReadValue<UInt32>(); }
            inline UInt64                                   ReadUInt64() noexcept { return ReadValue<UInt64>(); }
            inline SByte                                    ReadSByte() noexcept { return ReadValue<SByte>(); }
            inline Byte                                     ReadByte() noexcept { return ReadValue<Byte>(); }
            inline Single                                   ReadSingle() noexcept { return ReadValue<Single>(); }
            inline Double                                   ReadDouble() noexcept { return ReadValue<Double>(); }
            inline bool                                     ReadBoolean() noexcept { return ReadValue<bool>(); }
            inline Char                                     ReadChar() noexcept { return ReadValue<Char>(); }

        public:     
            inline bool                                     TryReadInt16(Int16& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadInt32(Int32& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadInt64(Int64& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadUInt16(UInt16& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadUInt32(UInt32& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadUInt64(UInt64& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadSByte(SByte& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadByte(Byte& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadSingle(Single& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadDouble(bool& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadBoolean(bool& out) noexcept { return TryReadValue(out); }
            inline bool                                     TryReadChar(Char& out) noexcept { return TryReadValue(out); }

        private:            
            Stream&                                         _stream;
        };
    }
}