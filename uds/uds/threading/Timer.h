#pragma once

#include <uds/threading/Hosting.h>

namespace uds {
    namespace threading {
        template<typename TimeoutHandler>
        inline std::shared_ptr<boost::asio::deadline_timer>     SetTimeout(
            const std::shared_ptr<boost::asio::io_context>&     context,
            const BOOST_ASIO_MOVE_ARG(TimeoutHandler)           handler,
            int                                                 timeout) noexcept {
            class TimeoutContext final {
            public:
                std::shared_ptr<boost::asio::deadline_timer> timeout;
            };
            std::shared_ptr<TimeoutContext> TC_ = make_shared_object<TimeoutContext>();
            TimeoutHandler TH_ = BOOST_ASIO_MOVE_CAST(TimeoutHandler)(constantof(handler));
            TC_->timeout = uds::threading::Hosting::Timeout(context,
                [TC_, TH_]() noexcept {
                    boost::asio::deadline_timer* key_ = TC_->timeout.get();
                    uds::threading::Hosting::Cancel(TC_->timeout);
                    TC_->timeout.reset();
                    TH_(key_);
                }, timeout);
            return TC_->timeout;
        }

        template<typename TimeoutHandler>
        inline std::shared_ptr<boost::asio::deadline_timer>     SetTimeout(
            const std::shared_ptr<uds::threading::Hosting>&     hosting,
            const BOOST_ASIO_MOVE_ARG(TimeoutHandler)           handler,
            int                                                 timeout) noexcept {
            if (!hosting) {
                return NULL;
            }

            std::shared_ptr<boost::asio::io_context> context = hosting->GetContext();
            if (!context) {
                return NULL;
            }
            return uds::threading::SetTimeout(context, forward0f(handler), timeout);
        }

        inline bool                                             ClearTimeout(
            std::shared_ptr<boost::asio::deadline_timer>&       timeout) noexcept {
            std::shared_ptr<boost::asio::deadline_timer> deadline_timer = std::move(timeout);
            if (!deadline_timer) {
                return false;
            }
            else {
                uds::threading::Hosting::Cancel(deadline_timer);
                timeout.reset();
                deadline_timer.reset();
                return true;
            }
        }
    }
}