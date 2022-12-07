#include <uds/transmission/ITransmission.h>

namespace uds {
    namespace transmission {
        ITransmission::~ITransmission() noexcept = default;

        void ITransmission::Close() noexcept {
            Dispose();
        }
    }
}