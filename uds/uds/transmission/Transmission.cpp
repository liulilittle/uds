#include <uds/transmission/Transmission.h>
#include <uds/net/Socket.h>

namespace uds {
    namespace transmission {
        Transmission::Transmission(const std::shared_ptr<uds::threading::Hosting>& hosting, const std::shared_ptr<boost::asio::io_context>& context, const std::shared_ptr<boost::asio::ip::tcp::socket>& socket, int alignment) noexcept
            : disposed_(false)
            , hosting_(hosting)
            , context_(context)
            , socket_(socket)
            , writing_(false) {
            typedef uds::net::IPEndPoint IPEndPoint;

            /* Format address enpoint. */
            boost::system::error_code ec;
            localEP_ = IPEndPoint::V6ToV4(IPEndPoint::ToEndPoint(socket->local_endpoint(ec)));
            remoteEP_ = IPEndPoint::V6ToV4(IPEndPoint::ToEndPoint(socket->remote_endpoint(ec)));

            /* Alloc transmission receive buffers. */
            if (alignment >= (UINT8_MAX << 1) && alignment <= ETRANSMISSION_MSS) {
                constantof(ETRANSMISSION_MSS) = alignment;
            }
            buffer_ = make_shared_alloc<Byte>(ETRANSMISSION_MSS);
        }

        uds::net::IPEndPoint Transmission::GetLocalEndPoint() noexcept {
            return localEP_;
        }

        uds::net::IPEndPoint Transmission::GetRemoteEndPoint() noexcept {
            return remoteEP_;
        }

        void Transmission::SetLocalEndPoint(const uds::net::IPEndPoint& value) noexcept {
            localEP_ = value;
        }

        void Transmission::SetRemoteEndPoint(const uds::net::IPEndPoint& value) noexcept {
            remoteEP_ = value;
        }

        std::shared_ptr<boost::asio::io_context> Transmission::GetContext() noexcept {
            return context_;
        }

        bool Transmission::HandshakeAsync(HandshakeType type, const BOOST_ASIO_MOVE_ARG(HandshakeAsyncCallback) callback) noexcept {
            if (!context_ || !socket_ || !callback) {
                return false;
            }

            const bool available = socket_->is_open();
            if (!available) {
                return false;
            }

            const HandshakeAsyncCallback callback_ = BOOST_ASIO_MOVE_CAST(HandshakeAsyncCallback)(constantof(callback));
            const std::shared_ptr<Reference> reference_ = GetReference();

            boost::asio::post(*context_,
                [reference_, this, callback_]() noexcept {
                    callback_(socket_->is_open());
                });
            return true;
        }

        bool Transmission::ReadAsync(const BOOST_ASIO_MOVE_ARG(ReadAsyncCallback) callback) noexcept {
            if (!socket_->is_open()) {
                return false;
            }

            return Transmission::Unpack(*socket_, forward0f(callback));
        }

        bool Transmission::WriteAsync(const std::shared_ptr<Byte>& buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept {
            if (!socket_->is_open()) {
                return false;
            }

            pmessage messages = Pack(buffer.get(), offset, length, forward0f(callback));
            if (!messages) {
                return false;
            }

            OnAddWriteAsync(forward0f(messages));
            return true;
        }

        void Transmission::OnAddWriteAsync(const BOOST_ASIO_MOVE_ARG(pmessage) message) noexcept {
            const std::shared_ptr<Reference> reference = GetReference();
            const pmessage messages = BOOST_ASIO_MOVE_CAST(pmessage)(constantof(message));

            messages_.push_back(messages);
            OnAsyncWrite(false);
        }

        void Transmission::Dispose() noexcept {
            if (!disposed_.exchange(true)) {
                messages_.clear();
                uds::net::Socket::Closesocket(socket_);
            }
        }

        bool Transmission::OnWriteAsync(const BOOST_ASIO_MOVE_ARG(pmessage) message) noexcept {
            const std::shared_ptr<Reference> reference = GetReference();
            const pmessage messages = BOOST_ASIO_MOVE_CAST(pmessage)(constantof(message));

            boost::asio::async_write(*socket_, boost::asio::buffer(messages->packet.get(), messages->packet_size),
                [reference, this, messages](const boost::system::error_code& ec, size_t sz) noexcept {
                    bool success = ec ? false : true;
                    if (!success) {
                        Close();
                    }

                    const WriteAsyncCallback& callback = messages->callback;
                    if (callback) {
                        callback(success);
                    }
                    OnAsyncWrite(true);
                });
            return true;
        }

        bool Transmission::OnAsyncWrite(bool internall) noexcept {
            if (!internall) {
                if (writing_.exchange(true)) { // 正在队列写数据且不是内部调用则返回真
                    return true;
                }
            }

            message_queue::iterator tail = messages_.begin();
            message_queue::iterator endl = messages_.end();
            if (tail == endl) { // 当前消息队列是空得
                writing_.exchange(false);
                return false;
            }

            std::shared_ptr<Reference> reference = GetReference();
            pmessage message = std::move(*tail);

            messages_.erase(tail); // 从消息队列中删除这条消息
            return OnWriteAsync(BOOST_ASIO_MOVE_CAST(pmessage)(message));
        }

        Transmission::pmessage Transmission::Pack(const void* buffer, int offset, int length, const BOOST_ASIO_MOVE_ARG(WriteAsyncCallback) callback) noexcept {
            if (!buffer || offset < 0 || length < 1 || length > ETRANSMISSION_MSS) {
                return NULL;
            }

            int packet_size_ = ETRANSMISSION_TSS + length;
            std::shared_ptr<Byte> packet_ = make_shared_alloc<Byte>(packet_size_);

            Byte* p_ = packet_.get();
            p_[0] = (Byte)(length >> 8);
            p_[1] = (Byte)(length);
            memcpy(p_ + ETRANSMISSION_TSS, ((Byte*)buffer) + offset, length);

            pmessage messages = make_shared_object<message>();
            messages->packet = packet_;
            messages->packet_size = packet_size_;
            messages->callback = BOOST_ASIO_MOVE_CAST(WriteAsyncCallback)(constantof(callback));
            return messages;
        }
    }
}