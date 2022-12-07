#include <uds/threading/Hosting.h>

namespace uds {
    namespace threading {
        bool Hosting::Run(std::function<void()> entryPoint) noexcept {
            if (context_ || buffer_) {
                return false;
            }

            context_ = make_shared_object<boost::asio::io_context>();
            if (!context_) {
                return false;
            }

            buffer_ = make_shared_alloc<Byte>(BufferSize);
            if (!buffer_) {
                return false;
            }

            if (!OpenTimeout()) {
                return false;
            }

            if (entryPoint) {
                context_->post(std::move(entryPoint));
            }

            boost::system::error_code ec_;
            boost::asio::io_context::work work_(*context_);
            context_->run(ec_);
            return true;
        }

        bool Hosting::OpenTimeout() noexcept {
            if (timeout_ || !context_) {
                return true;
            }

            timeout_ = make_shared_object<boost::asio::deadline_timer>(*context_);
            if (!timeout_) {
                return false;
            }

            return AwaitTimeoutAsync();
        }

        bool Hosting::AwaitTimeoutAsync() noexcept {
            const std::shared_ptr<Reference> reference = GetReference();
            const std::shared_ptr<boost::asio::deadline_timer> timeout = timeout_;
            if (!timeout) {
                return false;
            }

            const static uint64_t ANY_WAIT_TICK_TIMEOUT = 10;
            timeout->expires_from_now(boost::posix_time::milliseconds(ANY_WAIT_TICK_TIMEOUT));
            timeout->async_wait(
                [this, reference, timeout](const boost::system::error_code& ec) noexcept {
                    now_ += ANY_WAIT_TICK_TIMEOUT;
                    AwaitTimeoutAsync();
                });
            return true;
        }

        void Hosting::Cancel(const boost::asio::deadline_timer& timeout) noexcept {
            boost::system::error_code ec;
            try {
                boost::asio::deadline_timer& t = const_cast<boost::asio::deadline_timer&>(timeout);
                t.cancel(ec);
            }
            catch (std::exception&) {}
        }

        void Hosting::Cancel(const std::shared_ptr<boost::asio::deadline_timer>& timeout) noexcept {
            if (timeout) {
                Cancel(*timeout);
            }
        }
    }
}