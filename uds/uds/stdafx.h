#pragma once

#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN64
#define _WIN32 1
#endif

#ifndef R_OK
#define R_OK 4 /* Test for read permission. */
#endif

#ifndef W_OK
#define W_OK 2 /* Test for write permission. */
#endif

#ifndef X_OK
#define X_OK 1 /* Test for execute permission. */
#endif

#ifndef F_OK
#define F_OK 0 /* Test for existence. */
#endif

#ifndef elif
#define elif else if
#endif

#ifndef arraysizeof
#define arraysizeof(array_) (sizeof(array_) / sizeof(*array_))
#endif

#include <stdint.h>
#include <signal.h>
#include <limits.h>
#include <time.h>

#include <type_traits>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <unordered_map>

#ifndef BOOST_BEAST_VERSION_HPP
#define BOOST_BEAST_VERSION_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/config.hpp>

/*  BOOST_BEAST_VERSION

    Identifies the API version of Beast.

    This is a simple integer that is incremented by one every
    time a set of code changes is merged to the develop branch.
*/
#define BOOST_BEAST_VERSION 322
#define BOOST_BEAST_VERSION_STRING "uds"
#endif

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace stl {
    template<class T, T V>
    struct integral_constant {
        static constexpr T value = V;

        using value_type = T;
        using type = integral_constant;

        constexpr operator value_type() const noexcept {
            return value;
        }

        constexpr value_type operator()() const noexcept {
            return value;
        }
    };

    template<bool V>
    using bool_constant = integral_constant<bool, V>;

    using true_type = bool_constant<true>;
    using false_type = bool_constant<false>;

    template<class T, class U>
    struct is_same : false_type {};

    template<class T>
    struct is_same<T, T> : true_type {};

    template<class T>
    struct remove_pointer {
        typedef T type;
    };

    template<class T>
    struct remove_pointer<T*> {
        typedef T type;
    };

    template<class T>
    struct remove_pointer<T* const> {
        typedef T type;
    };

    template<class T>
    struct remove_pointer<T* volatile> {
        typedef T type;
    };

    template<class T>
    struct remove_pointer<T* const volatile> {
        typedef T type;
    };

    template<class T>
    struct remove_pointer<std::shared_ptr<T> > {
        typedef T type;
    };

    template<class T>
    struct remove_pointer<std::shared_ptr<T>&> {
        typedef T type;
    };

    template<class T>
    struct remove_pointer<std::shared_ptr<T> const&> {
        typedef T type;
    };

    template<class T>
    struct remove_reference {
        typedef T type;
    };

    template<class T>
    struct remove_reference<T&> {
        typedef T type;
    };

    template<class T>
    struct remove_reference<T&&> {
        typedef T type;
    };
}

#ifdef _WIN32
namespace boost { // boost::asio::posix::stream_descriptor
    namespace asio {
        namespace posix {
            typedef boost::asio::windows::stream_handle stream_descriptor;
        }
    }
}
#include <WinSock2.h>
#else
namespace boost {
    namespace asio {
        typedef io_service io_context;
    }
}
#endif

#ifdef JEMALLOC
#ifdef _Win32
#include <jemalloc/jemalloc.h>
#else
#ifdef __cplusplus 
extern "C" {
#endif
    void*                                                           je_malloc(size_t size);
    void                                                            je_free(void* size);
#ifdef __cplusplus 
}
#endif
#endif
#endif

#ifdef ANDROID
#include <android/log.h>

// 定义日志输出函数
#define LOG_TAG ("FRP")
#define LOG_INFO(...)   __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOG_WARN(...)   __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOG_DEBUG(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#ifdef _WIN32
#define LOG_TAG(TAG, FORMAT, ...) ::fprintf(stdout, "[%s][" ## TAG ## "](%s:%d): " ## FORMAT ## "\r\n", ((char*)::uds::GetCurrentTimeText<std::string>().data()), __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_TAG(TAG, FORMAT, ...) ::fprintf(stdout, "[%s][" TAG "](%s:%d): " FORMAT "\r\n", ((char*)::uds::GetCurrentTimeText<std::string>().data()), __FILE__, __LINE__, __VA_ARGS__)
#endif
#define LOG_INFO(FORMAT, ...)     LOG_TAG("INFO", FORMAT, __VA_ARGS__)
#define LOG_ERROR(FORMAT, ...)    LOG_TAG("ERROR", FORMAT, __VA_ARGS__)
#define LOG_WARN(FORMAT, ...)     LOG_TAG("WARN", FORMAT, __VA_ARGS__)
#define LOG_DEBUG(FORMAT, ...)    LOG_TAG("DEBUG", FORMAT, __VA_ARGS__)
#endif

#ifndef strcasecmp
#define strcasecmp strcasecmp_
#endif

#ifndef strncasecmp
#define strncasecmp strncasecmp_
#endif

#ifndef BOOST_ASIO_MOVE_CAST
#define BOOST_ASIO_MOVE_CAST(type) static_cast<type&&>
#endif

#ifndef BOOST_ASIO_MOVE_ARG
#define BOOST_ASIO_MOVE_ARG(type) type&&
#endif

inline int                                                                  strncasecmp_(const void* x, const void* y, size_t length) noexcept {
    if (x == y || length == 0) {
        return 0;
    }

    char* px = (char*)x;
    char* py = (char*)y;

    for (size_t i = 0; i < length; i++) {
        int xch = toupper(*px++);
        int ych = toupper(*py++);

        if (xch != ych) {
            return xch > ych ? 1 : -1;
        }
    }
    return 0;
}

inline int                                                                  strcasecmp_(const void* x, const void* y) noexcept {
    if (x == y) {
        return 0;
    }

    char* px = (char*)x;
    char* py = (char*)y;

    size_t xlen = strlen(px);
    size_t ylen = strlen(py);

    if (xlen != ylen) {
        return xlen > ylen ? 1 : -1;
    }
    return strncasecmp(x, y, xlen);
}

namespace uds {
    template <typename T>
    constexpr inline T                                                      Malign(const T size, int alignment) noexcept {
        return (T)(((uint64_t)size + alignment - 1) & ~(alignment - 1));
    }

    inline void*                                                            Malloc(size_t size) noexcept {
        if (!size) {
            return NULL;
        }

        size = Malign(size, 16);
#ifdef JEMALLOC
        return (void*)::je_malloc(size);
#else
        return (void*)::malloc(size);
#endif
    }

    inline void                                                             Mfree(const void* p) noexcept {
        if (p) {
#ifdef JEMALLOC
            ::je_free((void*)p);
#else
            ::free((void*)p);
#endif
        }
    }

    typedef unsigned char                                                   Byte;
    typedef signed char                                                     SByte;
    typedef signed short int                                                Int16;
    typedef signed int                                                      Int32;
    typedef signed long long                                                Int64;
    typedef unsigned short int                                              UInt16;
    typedef unsigned int                                                    UInt32;
    typedef unsigned long long                                              UInt64;
    typedef double                                                          Double;
    typedef float                                                           Single;
    typedef bool                                                            Boolean;
    typedef signed char                                                     Char;

    template<typename _Ty>
    inline int                                                              Tokenize(const _Ty& str, std::vector<_Ty>& tokens, const _Ty& delimiters) noexcept {
        if (str.empty()) {
            return 0;
        }
        elif(delimiters.empty()) {
            tokens.push_back(str);
            return 1;
        }

        char* deli_ptr = (char*)delimiters.data();
        char* deli_endptr = deli_ptr + delimiters.size();
        char* data_ptr = (char*)str.data();
        char* data_endptr = data_ptr + str.size();
        char* last_ptr = NULL;

        int length = 0;
        int seg = 0;
        while (data_ptr < data_endptr) {
            int ch = *data_ptr;
            int b = 0;
            for (char* p = deli_ptr; p < deli_endptr; p++) {
                if (*p == ch) {
                    b = 1;
                    break;
                }
            }
            if (b) {
                if (seg) {
                    int sz = data_ptr - last_ptr;
                    if (sz > 0) {
                        length++;
                        tokens.push_back(_Ty(last_ptr, sz));
                    }
                    seg = 0;
                }
            }
            elif(!seg) {
                seg = 1;
                last_ptr = data_ptr;
            }
            data_ptr++;
        }
        if ((seg && last_ptr) && last_ptr < data_ptr) {
            length++;
            tokens.push_back(_Ty(last_ptr, data_ptr - last_ptr));
        }
        return length;
    }

    template<typename _Ty> /* 65279u */
    inline _Ty                                                              ZTrim(const _Ty& s) {
        std::size_t length = s.size();
        if (length == 0) {
            return _Ty();
        }

        char* r = (char*)Malloc(length);
        char* p = (char*)s.data();

        std::size_t l = 0;
        for (std::size_t i = 0; i < length;) {
            std::size_t c0 = (unsigned char)p[i];
            std::size_t c1 = c0;
            std::size_t c2 = c0;

            std::size_t n = i + 1;
            if (n < length) {
                c1 = c0 | (unsigned char)p[n] << 8; // LE
                c2 = c0 << 8 | (unsigned char)p[n]; // BE
            }

            if (c1 == 65279u || c2 == 65279u) {
                i += 2;
            }
            else {
                i++;
                r[l++] = (signed char)c0;
            }
        }

        _Ty result(r, l);
        Mfree(r);
        return result;
    }

    template<typename _Ty> 
    inline _Ty                                                              LTrim(const _Ty& s) noexcept {
        _Ty str = s;
        if (str.empty()) {
            return str;
        }

        int64_t pos = -1;
        for (size_t i = 0, l = str.size(); i < l; ++i) {
            char ch = str[i];
            if (ch == ' ' ||
                ch == '\0' ||
                ch == '\n' ||
                ch == '\r' ||
                ch == '\t') {
                pos = i + 1;
            }
            else {
                break;
            }
        }
        if (pos >= 0) {
            if (pos >= (int64_t)str.size()) {
                return "";
            }
            str = str.substr(pos);
        }
        return str;
    }

    template<typename _Ty>
    inline _Ty                                                              RTrim(const _Ty& s) noexcept {
        _Ty str = s;
        if (str.empty()) {
            return str;
        }

        int64_t pos = -1;
        int64_t i = str.size();
        i--;
        for (; i >= 0u; --i) {
            char ch = str[i];
            if (ch == ' ' ||
                ch == '\0' ||
                ch == '\n' ||
                ch == '\r' ||
                ch == '\t') {
                pos = i;
            }
            else {
                break;
            }
        }
        if (pos >= 0) {
            if (0 >= pos) {
                return "";
            }
            str = str.substr(0, pos);
        }
        return str;
    }

    template<typename _Ty>
    inline _Ty                                                              ToUpper(const _Ty& s) noexcept {
        _Ty r = s;
        if (!r.empty()) {
            std::transform(s.begin(), s.end(), r.begin(), toupper);
        }
        return r;
    }

    template<typename _Ty>
    inline _Ty                                                              ToLower(const _Ty& s) noexcept {
        _Ty r = s;
        if (!r.empty()) {
            std::transform(s.begin(), s.end(), r.begin(), tolower);
        }
        return r;
    }

    template<typename _Ty>
    inline _Ty                                                              Replace(const _Ty& s, const _Ty& old_value, const _Ty& new_value) noexcept {
        _Ty r = s;
        if (r.empty()) {
            return r;
        }
        do {
            typename _Ty::size_type pos = r.find(old_value);
            if (pos != _Ty::npos) {
                r.replace(pos, old_value.length(), new_value);
            }
            else {
                break;
            }
        } while (1);
        return r;
    }

    template<typename _Ty>
    inline int                                                              Split(const _Ty& str, std::vector<_Ty>& tokens, const _Ty& delimiters) noexcept {
        if (str.empty()) {
            return 0;
        }
        elif(delimiters.empty()) {
            tokens.push_back(str);
            return 1;
        }
        size_t last_pos = 0;
        size_t curr_cnt = 0;
        while (1) {
            size_t pos = str.find(delimiters, last_pos);
            if (pos == _Ty::npos) {
                pos = str.size();
            }

            size_t len = pos - last_pos;
            if (len != 0) {
                curr_cnt++;
                tokens.push_back(str.substr(last_pos, len));
            }

            if (pos == str.size()) {
                break;
            }
            last_pos = pos + delimiters.size();
        }
        return curr_cnt;
    }

    inline uint64_t                                                         GetTickCount(bool microseconds) noexcept {
#ifdef _WIN32
        static LARGE_INTEGER ticksPerSecond; // (unsigned long long)GetTickCount64();
        LARGE_INTEGER ticks;
        if (!ticksPerSecond.QuadPart) {
            QueryPerformanceFrequency(&ticksPerSecond);
        }

        QueryPerformanceCounter(&ticks);
        if (microseconds) {
            double cpufreq = (double)(ticksPerSecond.QuadPart / 1000000);
            unsigned long long nowtick = (unsigned long long)(ticks.QuadPart / cpufreq);
            return nowtick;
        }
        else {
            unsigned long long seconds = ticks.QuadPart / ticksPerSecond.QuadPart;
            unsigned long long milliseconds = 1000 * (ticks.QuadPart - (ticksPerSecond.QuadPart * seconds)) / ticksPerSecond.QuadPart;
            unsigned long long nowtick = seconds * 1000 + milliseconds;
            return (unsigned long long)nowtick;
        }
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);

        if (microseconds) {
            unsigned long long nowtick = (unsigned long long)tv.tv_sec * 1000000;
            nowtick += tv.tv_usec;
            return nowtick;
        }

        return ((unsigned long long)tv.tv_usec / 1000) + ((unsigned long long)tv.tv_sec * 1000);
#endif
    }

    inline uint64_t                                                         GetTickCount() {
        return GetTickCount(false);
    }

    template<typename _Ty>
    inline _Ty                                                              PaddingLeft(const _Ty& s, int count, char padding_char) noexcept {
        char buf[1500];
        if (count < 1 || count <= (int)s.size()) {
            return s;
        }
        _Ty r = s;
        int len = count - (int)s.size();
        while (len > 0) {
            int rd = len;
            if (rd >= (int)sizeof(buf)) {
                rd = sizeof(buf);
            }
            memset(buf, padding_char, rd);
            len -= rd;
            r = _Ty(buf, rd) + r;
        }
        return r;
    }

    template<typename _Ty>
    inline _Ty                                                              PaddingRight(const _Ty& s, int count, char padding_char) noexcept {
        char buf[1500];
        if (count < 1 || count <= (int)s.size()) {
            return s;
        }
        _Ty r = s;
        int len = count - (int)s.size();
        while (len > 0) {
            int rd = len;
            if (rd >= (int)sizeof(buf)) {
                rd = sizeof(buf);
            }
            memset(buf, padding_char, rd);
            len -= rd;
            r = r + _Ty(buf, rd);
        }
        return r;
    }

    template<typename _Ty>
    inline _Ty                                                              GetCurrentTimeText() noexcept {
        time_t rawtime;
        struct tm* ptminfo;

        time(&rawtime);
        ptminfo = localtime(&rawtime);

        auto fmt = [](int source, char* dest) noexcept {
            if (source < 10) {
                char temp[3];
                strcpy(dest, "0");
                sprintf(temp, "%d", source);
                strcat(dest, temp);
            }
            else {
                sprintf(dest, "%d", source);
            }
        };

        char yyyy[5], MM[3], dd[3], hh[3], mm[3], ss[3];
        sprintf(yyyy, "%d", (ptminfo->tm_year + 1900));

        fmt(ptminfo->tm_mon + 1, MM);
        fmt(ptminfo->tm_mday, dd);
        fmt(ptminfo->tm_hour, hh);
        fmt(ptminfo->tm_min, mm);
        fmt(ptminfo->tm_sec, ss);

        _Ty sb;
        sb.append(yyyy).
            append("-").
            append(MM).
            append("-").
            append(dd).
            append(" ").
            append(hh).
            append(":").
            append(mm).
            append(":").
            append(ss);
        return sb;
    }

    int                                                                     GetCurrentProcessId() noexcept;

    int                                                                     GetProcesserCount() noexcept;

    bool                                                                    ToBoolean(const char* s) noexcept;

    std::string                                                             StrFormatByteSize(Int64 size) noexcept;

    Char                                                                    RandomAscii() noexcept;

    int                                                                     RandomNext() noexcept;

    int                                                                     RandomNext(int minValue, int maxValue) noexcept;

    double                                                                  RandomNextDouble() noexcept;

    int                                                                     GetHashCode(const char* s, int len) noexcept;

    void                                                                    SetThreadPriorityToMaxLevel() noexcept;

    void                                                                    SetProcessPriorityToMaxLevel() noexcept;
  
    bool                                                                    IsInputHelpCommand(int argc, const char* argv[]) noexcept;

    bool                                                                    GetCommandArgument(const char* name, int argc, const char** argv, bool defaultValue) noexcept;
 
    std::string                                                             GetCommandArgument(const char* name, int argc, const char** argv) noexcept;

    std::string                                                             GetCommandArgument(const char* name, int argc, const char** argv, const char* defaultValue) noexcept;

    std::string                                                             GetCommandArgument(const char* name, int argc, const char** argv, const std::string& defaultValue) noexcept;

    std::string                                                             GetFullExecutionFilePath() noexcept;

    std::string                                                             GetApplicationStartupPath() noexcept;

    std::string                                                             GetExecutionFileName() noexcept;

    std::string                                                             GetCurrentDirectoryPath() noexcept;

    const char*                                                             GetPlatformCode() noexcept;

    boost::uuids::uuid                                                      GuidGenerate() noexcept;

    boost::uuids::uuid                                                      LexicalCast(const void* guid, int length) noexcept;

    boost::uuids::uuid                                                      StringToGuid(const std::string& guid) noexcept;

    std::string                                                             GuidToString(const boost::uuids::uuid& uuid) noexcept;

    std::string                                                             LexicalCast(const boost::uuids::uuid& uuid) noexcept;

    template<typename T>
    inline int                                                              FindIndexOf(int* next, T* src, int src_len, T* sub, int sub_len) noexcept {
        auto FindNextOf = [](int* next, T* sub, int sub_len) noexcept {
            int l = sub_len - 1;
            int i = 0;
            int j = -1;
            next[0] = -1;
            while (i < l) {
                if (j == -1 || sub[i] == sub[j]) {
                    j++;
                    i++;
                    if (sub[i] == sub[j]) {
                        next[i] = next[j];
                    }
                    else {
                        next[i] = j;
                    }
                }
                else {
                    j = next[j];
                }
            }
        };
        int i = 0;
        int j = 0;
        FindNextOf(next, sub, sub_len);
        while (i < src_len && j < sub_len) {
            if (j == -1 || src[i] == sub[j]) {
                i++;
                j++;
            }
            else {
                j = next[j];
            }
        }
        if (j >= sub_len) {
            return i - sub_len;
        }
        else {
            return -1;
        }
    }

    template<typename T>
    inline constexpr T*                                                     addressof(const T& v) noexcept {
        return (T*)&reinterpret_cast<const char&>(v);
    }

    template<typename T>
    inline constexpr T*                                                     addressof(const T* v) noexcept {
        return const_cast<T*>(v);
    }

    template<typename T>
    inline constexpr T&                                                     constantof(const T& v) noexcept {
        return const_cast<T&>(v);
    }

    template<typename T>                                                    
    inline constexpr T*                                                     constantof(const T* v) noexcept {
        return const_cast<T*>(v);
    }

    template<typename T>
    inline constexpr T&&                                                    constant0f(const T&& v) noexcept {
        return const_cast<T&&>(v);
    }

    template<typename T>
    inline constexpr T&&                                                    forward0f(const T& v) noexcept {
        return std::forward<T>(constantof(v));
    }

    template<typename T>
    inline std::shared_ptr<T>                                               make_shared_alloc(int length) noexcept {
        static_assert(sizeof(T) > 0, "can't make pointer to incomplete type");

        // https://pkg.go.dev/github.com/google/agi/core/os/device
        // ARM64v8a: __ALIGN(8)
        // ARMv7a  : __ALIGN(4)
        // X86_64  : __ALIGN(8)
        // X64     : __ALIGN(4)
        if (length < 1) {
            return NULL;
        }

        T* p = (T*)Malloc(length * sizeof(T));
        return std::shared_ptr<T>(p, Mfree);
    }

    template<typename T, typename... A>
    inline std::shared_ptr<T>                                               make_shared_object(A&&... args) noexcept {
        static_assert(sizeof(T) > 0, "can't make pointer to incomplete type");

        T* p = (T*)Malloc(sizeof(T));
        return std::shared_ptr<T>(new (p) T(std::forward<A&&>(args)...),
            [](T* p) noexcept {
                p->~T();
                Mfree(p);
            });
    }
}

namespace network {
    namespace asio {
        template<typename AsyncWriteStream, typename MutableBufferSequence>
        inline bool                                                         async_read(AsyncWriteStream& stream, const MutableBufferSequence& buffers, const boost::asio::yield_context& y) noexcept {
            if (!buffers.data() || !buffers.size()) {
                return false;
            }

            boost::system::error_code ec;
            try {
                std::size_t bytes_transferred = boost::asio::async_read(stream, constantof(buffers), y[ec]);
                if (ec) {
                    return false;
                }
                return bytes_transferred == buffers.size();
            }
            catch (std::exception&) {
                return false;
            }
        }

        template<typename AsyncWriteStream, typename ConstBufferSequence>
        inline bool                                                         async_write(AsyncWriteStream& stream, const ConstBufferSequence& buffers, const boost::asio::yield_context& y) noexcept {
            if (!buffers.data() || !buffers.size()) {
                return false;
            }

            boost::system::error_code ec;
            try {
                std::size_t bytes_transferred = boost::asio::async_write(stream, constantof(buffers), y[ec]);
                if (ec) {
                    return false;
                }
                return bytes_transferred == buffers.size();
            }
            catch (std::exception&) {
                return false;
            }
        }

        inline int                                                          async_connect(boost::asio::ip::tcp::socket& socket, const boost::asio::ip::tcp::endpoint& remoteEP, const boost::asio::yield_context& y) noexcept {
            boost::asio::ip::address address = remoteEP.address();
            if (address.is_unspecified() || address.is_multicast()) {
                return false;
            }

            int port = remoteEP.port();
            if (port < 1 || port > 65535) {
                return false;
            }

            boost::system::error_code ec;
            try {
                socket.async_connect(remoteEP, y[ec]);
                return ec.value();
            }
            catch (std::exception&) {
                return -1;
            }
        }
    }
}