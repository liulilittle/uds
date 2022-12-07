#pragma once

#include <uds/Reference.h>

namespace uds {
    class IDisposable : public Reference {
    public:
        virtual                                 ~IDisposable() noexcept = default;

    public:
        virtual void                            Dispose() = 0;
    };
}