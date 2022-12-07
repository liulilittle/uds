#include <uds/io/File.h>
#include <uds/io/MemoryStream.h>
#include <uds/text/Encoding.h>

namespace uds {
    namespace io {
        int File::GetLength(const char* path) noexcept {
            if (NULL == path) {
                return ~0;
            }

            FILE* stream = fopen(path, "rb");
            if (NULL == stream) {
                return ~0;
            }

            fseek(stream, 0, SEEK_END);
            long length = ftell(stream);
            fclose(stream);

            return length;
        }

        bool File::Exists(const char* path) noexcept {
            if (NULL == path) {
                return false;
            }

            return access(path, F_OK) == 0;
        }

        bool File::WriteAllBytes(const char* path, const void* data, int length) noexcept {
            if (NULL == path || length < 0) {
                return false;
            }

            if (NULL == data && length != 0) {
                return false;
            }

            FILE* f = fopen(path, "wb+");
            if (NULL == f) {
                return false;
            }

            if (length > 0) {
                fwrite((char*)data, length, 1, f);
            }

            fflush(f);
            fclose(f);
            return true;
        }

        bool File::CanAccess(const char* path, FileAccess access_) noexcept {
#ifdef _WIN32
            if (NULL == path) {
                return false;
            }

            int flags = 0;
            if ((access_ & FileAccess::ReadWrite) == FileAccess::ReadWrite) {
                flags |= R_OK | W_OK;
            }
            else {
                if (access_ & FileAccess::Read) {
                    flags |= R_OK;
                }
                if (access_ & FileAccess::Write) {
                    flags |= W_OK;
                }
            }
            return access(path, flags) == 0;
#else
            int flags = 0;
            if ((access_ & FileAccess::ReadWrite) == FileAccess::ReadWrite) {
                flags |= O_RDWR;
            }
            else {
                if (access_ & FileAccess::Read) {
                    flags |= O_RDONLY;
                }
                if (access_ & FileAccess::Write) {
                    flags |= O_WRONLY;
                }
            }

            int fd = open(path, flags);
            if (fd == -1) {
                return false;
            }
            else {
                close(fd);
                return true;
            }
#endif
        }

        int File::GetEncoding(const void* p, int length, int& offset) noexcept {
            offset = 0;
            if (NULL == p || length < 1) {
                return uds::text::Encoding::ASCII;
            }
            // byte[] Unicode = new byte[] { 0xFF, 0xFE, 0x41 };
            // byte[] UnicodeBIG = new byte[] { 0xFE, 0xFF, 0x00 };
            // byte[] UTF8 = new byte[] { 0xEF, 0xBB, 0xBF }; // BOM
            const Byte* s = (Byte*)p;
            if (s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
                offset += 3;
                return uds::text::Encoding::UTF8;
            }
            elif(s[0] == 0xFE && s[1] == 0xFF && s[2] == 0x00) {
                offset += 3;
                return uds::text::Encoding::BigEndianUnicode;
            }
            elif(s[0] == 0xFF && s[1] == 0xFE && s[2] == 0x41) {
                offset += 3;
                return uds::text::Encoding::Unicode;
            }
            return uds::text::Encoding::ASCII;
        }

        std::shared_ptr<Byte> File::ReadAllBytes(const char* path, int& length) noexcept {
            length = ~0;
            if (NULL == path) {
                return NULL;
            }

            FILE* file_ = fopen(path, "rb"); // Oracle Cloud Shells Compatibility...
            if (!file_) {
                return NULL;
            }

            MemoryStream stream_;
            char buff_[1400];
            for (;;) {
                size_t count_ = fread(buff_, 1, sizeof(buff_), file_);
                if (count_ == 0) {
                    break;
                }
                stream_.Write(buff_, 0, count_);
            }

            fclose(file_);
            length = stream_.GetPosition();
            return stream_.GetBuffer();
        }
    }
}
