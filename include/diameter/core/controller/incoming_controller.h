#ifndef DIAMETER_CORE_CONTROLLER_INCOMING_CONTROLLER_H
#define DIAMETER_CORE_CONTROLLER_INCOMING_CONTROLLER_H

#include <chrono>
#include <functional>
#include <memory>
#include <limits>
#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <diameter/core/io/connection.h>
#include <diameter/application/base/command.h>
#include <diameter/message/message.h>

namespace diameter::core::controller {

class IncomingController
{
public:
    using ConnectionId = uint64_t;
    using ConnectionPtr = std::shared_ptr<io::Connection>;
    using MessagePtr = std::shared_ptr<message::Message>;
    using TimerPtr = std::shared_ptr<boost::asio::steady_timer>;

private:
    struct WaitingData
    {
        ConnectionPtr connect;
        TimerPtr timer;
    };

    using WaitingDataMap = std::unordered_map<ConnectionId, WaitingData>;

public:
    IncomingController(boost::asio::io_context& ioc)
        : m_ioc(ioc)
    {
    }

    IncomingController(IncomingController const&) = delete;
    IncomingController& operator= (IncomingController const&) = delete;
    IncomingController(IncomingController&&) = delete;
    IncomingController& operator= (IncomingController&&) = delete;

    void add_new_connection(const ConnectionPtr& connect, const std::chrono::steady_clock::duration& timeout)
    {
        auto timer = std::make_shared<boost::asio::steady_timer>(m_ioc);
        auto data = WaitingData{connect, timer};

        std::lock_guard lock(m_mutex);
        auto conn_id = generate_connection_id();
        auto [it, inserted] = m_waiting_connections.emplace(conn_id, data);

        it->second.connect->set_on_disconnect_cb([this, conn_id](auto&&... args) {
            on_disconnect(conn_id, std::forward<decltype(args)>(args)...);
        });
        it->second.connect->set_on_recv_message_cb([this, conn_id](auto&&... args) {
            on_message(conn_id, std::forward<decltype(args)>(args)...);
        });
        it->second.timer->expires_from_now(timeout);

        // Run async operations
        it->second.connect->run();
        it->second.timer->async_wait([this, conn_id](auto&&... args) {
            on_timeout(conn_id, std::forward<decltype(args)>(args)...);
        });
    }

    void stop()
    {
        std::lock_guard lock(m_mutex);
        for (auto& [id, data] : m_waiting_connections) {
            data.timer->cancel();
            data.connect->stop();
        }
        m_waiting_connections.clear();
    }

private:
    void on_timeout(ConnectionId conn_id, const boost::system::error_code& error)
    {
        ConnectionPtr connect;
        {
            std::lock_guard lock(m_mutex);
            auto it = m_waiting_connections.find(conn_id);
            if (it == m_waiting_connections.end()) {
                return;
            }
            connect = it->second.connect;
            m_waiting_connections.erase(it);
        }

        if (error != boost::asio::error::operation_aborted) {
            connect->stop();
        }      
    }

    void on_disconnect(ConnectionId conn_id, const boost::system::error_code& error)
    {
        std::lock_guard lock(m_mutex);
        auto it = m_waiting_connections.find(conn_id);
        if (it != m_waiting_connections.end()) {
            it->second.timer->cancel();
        }
    }

    void on_message(ConnectionId conn_id, MessagePtr&& message)
    {
        ConnectionPtr connect;
        {
            std::lock_guard lock(m_mutex);
            auto it = m_waiting_connections.find(conn_id);
            if (it == m_waiting_connections.end()) {
                return;
            }
            it->second.timer->cancel();
            connect = it->second.connect;
        }           

        auto msg = std::move(message);
        // 5.6.1. Incoming Connections
        // The logic that handles incoming connections SHOULD close and discard
        // the connection if any message other than a CER arrives
        if (!is_CER_message(msg)) {
            connect->stop();
            return;
        }

        // TODO: call on R_Conn_CER
    }

    bool is_CER_message(const MessagePtr& message) const noexcept
    {
        if (message->header.application_id == message::header::ApplicationV::Common) {
            if (message->header.command_flags[message::header::CommandFlag::Request]) {
                if (message->header.command_code == application::base::CommandV::CapabilitiesExchange) {
                    return true;
                }
            }
        }
        return false;
    }

    ConnectionId generate_connection_id() const
    {
        static ConnectionId last_id = 0;
        return 1 + (++last_id % std::numeric_limits<ConnectionId>::max());
    }

private:
    boost::asio::io_context& m_ioc;

    mutable std::mutex m_mutex;
    std::unordered_map<ConnectionId, WaitingData> m_waiting_connections;
};

}
#endif