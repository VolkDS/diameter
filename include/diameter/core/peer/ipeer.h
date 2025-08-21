#ifndef DIAMETER_CORE_PEER_PEER_INTERFACE_H
#define DIAMETER_CORE_PEER_PEER_INTERFACE_H

#include <memory>

#include <diameter/message/message.h>

namespace diameter::core::peer {

class IPeer
{
public:
    using MessagePtr = std::shared_ptr<diameter::message::Message>;
    using OnRecvMessageCb = std::function<void(message::Message&&)>;
    using OnOpenStateCb = std::function<void()>;
    using OnClosedStateCb = std::function<void()>;

    using Clock = std::chrono::steady_clock;
    using Duration = typename Clock::duration;
    using TimePoint = typename Clock::time_point;

    enum class Role : uint32_t
    {
        INITIATOR,
        RESPONDER
    };

    enum class State : uint32_t
    {
        CLOSED,
        WAIT_CONN_ACK,
        WAIT_CEA,
        ELECT,
        WAIT_RETURNS,
        ROPEN,
        IOPEN,
        CLOSING
    };

    virtual ~IPeer() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void r_conn_CER() = 0;

    virtual void send_message(const MessagePtr& message) = 0;
    virtual void set_on_open_state_cb(OnOpenStateCb&& callback) = 0;
    virtual void set_on_closed_state_cb(OnOpenStateCb&& callback) = 0;

    virtual std::string id() = 0;
    virtual std::string name() = 0;
    virtual State state() = 0;
};

}

#endif