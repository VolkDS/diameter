#ifndef DIAMETER_CORE_CONTROLLER_MESSAGE_CONTROLLER_H
#define DIAMETER_CORE_CONTROLLER_MESSAGE_CONTROLLER_H

#include <chrono>
#include <utility>

#include <boost/asio/io_context.hpp>

#include <diameter/core/error.h>
#include <diameter/message/header/end_to_end.h>
#include <diameter/message/header/hop_by_hop.h>
#include <diameter/message/message.h>

namespace diameter::core::controller {

class MessageController
{
public:
    using MessagePtr = std::shared_ptr<message::Message>;
    using MessageId = std::pair<
        message::header::HopByHopIdentifier,
        message::header::EndToEndIdentifier
    >;

    using Timer = boost::asio::steady_timer;
    using TimerPtr = std::shared_ptr<Timer>;
    using Clock = std::chrono::steady_clock;
    using Duration = typename Clock::duration;
    using TimePoint = typename Clock::time_point;

    using OnRequestTimeoutCb = std::function<void(const std::string&, MessagePtr&&)>;
    using OnResponseCb = std::function<void(const core::Error&, MessagePtr&&)>;

    struct MessageContext
    {
        MessagePtr message;
        TimerPtr timer;
        OnResponseCb handler;
    };

    struct MessageIdHash
    {
        size_t operator() (const MessageId& id) const
        {
            return std::hash<uint64_t> {}((static_cast<uint64_t>(id.first) << 32) | id.second);
        }
    };

    using MessageMap = std::unordered_map<MessageId, MessageContext, MessageIdHash>;
    using PeerMessageMap = std::unordered_map<std::string, MessageMap>;

    MessageController(boost::asio::io_context& ioc)
        : m_ioc(ioc)
    {
    }

    MessageController(MessageController const&) = delete;
    MessageController& operator= (MessageController const&) = delete;
    MessageController(MessageController&&) = delete;
    MessageController& operator= (MessageController&&) = delete;

    void set_on_request_timeout_cb(OnRequestTimeoutCb&& handler)
    {
        std::lock_guard lock(m_mutex);
        m_on_request_timeout_cb = std::move(handler);
    }

    bool push_request(const MessagePtr& request, const std::string& peer_name,
        const Duration& timeout)
    {
        return push_request(request, peer_name, timeout, [](const core::Error&, MessagePtr&&) {});
    }

    bool push_request(const MessagePtr& request, const std::string& peer_name,
        const Duration& timeout, OnResponseCb&& handler)
    {
        if (!request) {
            return false;
        }

        auto timer = std::make_shared<Timer>(m_ioc);
        auto ctx = MessageContext {request, std::move(timer), std::move(handler)};

        auto msg_id = make_message_id(request);
        std::lock_guard lock(m_mutex);
        auto& messages = m_peer_messages[peer_name];
        auto [it, inserted] = messages.emplace(msg_id, ctx);

        if (!inserted) {
            return false;
        }

        it->second.timer->expires_from_now(timeout);
        it->second.timer->async_wait([this, msg_id, peer_name](auto&&... args) {
            on_request_timeout(msg_id, peer_name, std::forward<decltype(args)>(args)...);
        });
        return true;
    }

    bool handle_response_with_cb(MessagePtr&& answer, const std::string& peer_name)
    {
        if (!answer) {
            return false;
        }

        auto msg_id = make_message_id(answer);
        OnResponseCb handler;

        {
            std::lock_guard lock(m_mutex);

            auto peer_it = m_peer_messages.find(peer_name);
            if (peer_it == m_peer_messages.end()) {
                return false;
            }

            auto& messages = peer_it->second;
            auto it = messages.find(msg_id);
            if (it == messages.end()) {
                return false;
            }

            it->second.timer->cancel();
            handler = std::move(it->second.handler);

            messages.erase(it);
            if (messages.empty()) {
                m_peer_messages.erase(peer_it);
            }
        }

        if (handler) {
            boost::asio::post(m_ioc,
                [handler = std::move(handler), answer = std::move(answer)]() mutable {
                    handler(core::Error::Success, std::move(answer));
                });
        }
        return true;
    }

    bool handle_response_without_cb(const MessagePtr& answer, const std::string& peer_name)
    {
        if (!answer) {
            return false;
        }

        auto msg_id = make_message_id(answer);

        std::lock_guard lock(m_mutex);

        auto peer_it = m_peer_messages.find(peer_name);
        if (peer_it == m_peer_messages.end()) {
            return false;
        }

        auto& messages = peer_it->second;
        auto it = messages.find(msg_id);
        if (it == messages.end()) {
            return false;
        }

        it->second.timer->cancel();

        messages.erase(it);
        if (messages.empty()) {
            m_peer_messages.erase(peer_it);
        }

        return true;
    }

    void handle_peer_disconnect(const std::string& peer_name)
    {
        std::lock_guard lock(m_mutex);

        auto peer_it = m_peer_messages.find(peer_name);
        if (peer_it == m_peer_messages.end()) {
            return;
        }

        auto& messages = peer_it->second;
        for (auto it = messages.begin(); it != messages.end();) {
            it->second.timer->cancel();
            if (it->second.handler) {
                boost::asio::post(m_ioc,
                    [handler = std::move(it->second.handler)]() mutable {
                        handler(core::Error::NetworkError, nullptr);
                    });
            }
            it = messages.erase(it);
        }
        m_peer_messages.erase(peer_it);
    }

private:
    MessageId make_message_id(const MessagePtr& message)
    {
        return std::make_pair(message->header.hop_by_hop, message->header.end_to_end);
    }

    void on_request_timeout(MessageId msg_id, const std::string& peer_name,
        const boost::system::error_code& error)
    {
        MessagePtr request;
        OnRequestTimeoutCb on_request_timeout_cb;
        OnResponseCb handler;

        {
            std::lock_guard lock(m_mutex);
            auto peer_it = m_peer_messages.find(peer_name);
            if (peer_it == m_peer_messages.end()) {
                return;
            }

            auto& messages = peer_it->second;
            auto it = messages.find(msg_id);
            if (it == messages.end()) {
                return;
            }

            request = std::move(it->second.message);
            handler = std::move(it->second.handler);

            messages.erase(it);
            if (messages.empty()) {
                m_peer_messages.erase(peer_it);
            }
            on_request_timeout_cb = m_on_request_timeout_cb;
        }

        if (handler) {
            boost::asio::post(m_ioc, [handler = std::move(handler)]() mutable {
                handler(core::Error::Timeout, nullptr);
            });
        }

        if (error != boost::asio::error::operation_aborted && on_request_timeout_cb) {
            on_request_timeout_cb(peer_name, std::move(request));
        }
    }

private:
    boost::asio::io_context& m_ioc;

    mutable std::mutex m_mutex;
    PeerMessageMap m_peer_messages;
    OnRequestTimeoutCb m_on_request_timeout_cb;
};

} // namespace diameter::core::controller

#endif
