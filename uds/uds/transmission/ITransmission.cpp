#include <uds/transmission/ITransmission.h>

namespace uds {
    namespace transmission {
        ITransmission::ITransmission() noexcept = default;

        ITransmission::~ITransmission() noexcept = default;

        void ITransmission::Close() noexcept {
            Dispose();
        }

        std::shared_ptr<ITransmission> ITransmission::Constructor(const std::shared_ptr<ITransmission>& reference) {
            ITransmission* transmission = this;
            if (!reference) {
                throw std::invalid_argument("Null references are not allowed to construct generic transport layer objects.");
            }

            if (reference.get() != transmission) {
                throw std::invalid_argument("It is not allowed to pass address identifiers that are different from this transport layer reference.");
            }

            reference_ = reference;
            return transmission->GetReference();
        }

        std::shared_ptr<ITransmission> ITransmission::GetReference() noexcept {
            std::weak_ptr<ITransmission> weak = reference_;
            return weak.lock();
        }
    }
}