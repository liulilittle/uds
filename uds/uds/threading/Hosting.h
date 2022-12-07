#pragma once

#include <uds/Reference.h>

namespace uds {
    namespace threading {
        class Hosting final : public Reference {
            typedef std::shared_ptr<boost::asio::io_context>            ContextPtr;
            typedef std::mutex                                          Mutex;
            typedef std::lock_guard<Mutex>                              MutexScope;
            typedef std::list<ContextPtr>                               ContextList;
            typedef std::shared_ptr<Byte>                               ChunkPtr;
            typedef std::unordered_map<void*, ChunkPtr>                 ChunkTable;

        public:
            static const int                                            BufferSize = 65535;

        public:
            inline const std::shared_ptr<boost::asio::io_context>&      GetContext() noexcept {
                return context_;
            }
            inline const std::shared_ptr<Byte>&                         GetBuffer() noexcept {
                return buffer_;
            }
            bool                                                        OpenTimeout() noexcept;
            bool                                                        Run(std::function<void()> entryPoint) noexcept;

        public:
            inline uint64_t                                             CurrentMillisec() noexcept {
                return now_;
            }
            inline static int                                           GetMaxConcurrency() noexcept {
                int concurrent = std::thread::hardware_concurrency();
                if (concurrent < 1) {
                    concurrent = 1;
                }
                return concurrent;
            }

        public:
            template<typename TimeoutHandler>
            inline std::shared_ptr<boost::asio::deadline_timer>         Timeout(const BOOST_ASIO_MOVE_ARG(TimeoutHandler) handler, int timeout) noexcept {
                return Timeout<TimeoutHandler>(GetContext(), forward0f(handler), timeout);
            }
            
            template<typename TimeoutHandler>
            inline static std::shared_ptr<boost::asio::deadline_timer>  Timeout(const std::shared_ptr<boost::asio::io_context>& context_, const BOOST_ASIO_MOVE_ARG(TimeoutHandler) handler, int timeout) noexcept {
                if (timeout < 1) {
                    handler();
                    return NULL;
                }

                if (!context_) {
                    return NULL;
                }

                class AsyncWaitTimeoutHandler final {
                public:
                    std::shared_ptr<boost::asio::deadline_timer>        timeout_;
                    TimeoutHandler                                      handler_;

                public:
                    inline AsyncWaitTimeoutHandler(const AsyncWaitTimeoutHandler&& other) noexcept
                        : handler_(std::move(constantof(other.handler_)))
                        , timeout_(std::move(constantof(other.timeout_))) {

                    }
                    inline AsyncWaitTimeoutHandler(const std::shared_ptr<boost::asio::deadline_timer>& timeout, TimeoutHandler&& handler) noexcept
                        : timeout_(timeout)
                        , handler_(std::move(handler)) {

                    }

                public:
                    inline void operator()(const boost::system::error_code& ec) noexcept {
                        if (!ec) {
                            handler_();
                        }
                    }
                };

                std::shared_ptr<boost::asio::deadline_timer> timeout_ = make_shared_object<boost::asio::deadline_timer>(*context_);
                if (!timeout_) {
                    return NULL;
                }

                AsyncWaitTimeoutHandler completion_(timeout_, BOOST_ASIO_MOVE_CAST(TimeoutHandler)(constantof(handler)));
                timeout_->expires_from_now(boost::posix_time::milliseconds(timeout));
                timeout_->async_wait(BOOST_ASIO_MOVE_CAST(AsyncWaitTimeoutHandler)(completion_));
                return timeout_;
            }
            
            static void                                                 Cancel(const boost::asio::deadline_timer& timeout) noexcept;
            static void                                                 Cancel(const std::shared_ptr<boost::asio::deadline_timer>& timeout) noexcept;

        private:
            bool                                                        AwaitTimeoutAsync() noexcept;

        private:
            uint64_t                                                    now_;
            std::shared_ptr<Byte>                                       buffer_;
            std::shared_ptr<boost::asio::io_context>                    context_;
            std::shared_ptr<boost::asio::deadline_timer>                timeout_;
        };
    }
}