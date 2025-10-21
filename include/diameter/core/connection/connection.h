#ifndef DIAMETER_CORE_CONNECTION_CONNECTION_H
#define DIAMETER_CORE_CONNECTION_CONNECTION_H

#include <functional>

#include <diameter/message/message.h>

namespace diameter::core::connection {

class Connection
{
public:
    virtual ~Connection() = default;

    using OnRecvMessageCb = std::function<void(message::Message&&)>;
    using OnDisconnectCb = std::function<void()>;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void send_message(message::Message&&) = 0;

    virtual void set_on_recv_message_cb(OnRecvMessageCb&& callback) = 0;
    virtual void set_on_disconnect_cb(OnDisconnectCb&& callback) = 0;
};

} // namespace diameter::core::connection

#endif
