#ifndef DIAMETER_CORE_IO_CONNECTION_H
#define DIAMETER_CORE_IO_CONNECTION_H

#include <atomic>
#include <functional>
#include <memory>
#include <queue>
#include <shared_mutex>
#include <vector>

#include <boost/asio.hpp>

#include <netpacker/netpacker.h>

#include <diameter/core/config/config.h>
#include <diameter/serial/serial.h>

namespace diameter::core::io {

class Connection
    : public std::enable_shared_from_this<Connection>
{
public:
    using ProtocolType = boost::asio::ip::tcp; //move to template
    using SelfPtr = std::shared_ptr<Connection>;
    using SocketType = ProtocolType::socket;
    using EndpointType = ProtocolType::endpoint;
    using RawDataType = std::vector<uint8_t>;
    using MessagePtr = std::shared_ptr<message::Message>;

    using OnRecvMessageCb = std::function<void(MessagePtr&&)>;
    using OnDisconnectCb = std::function<void(const boost::system::error_code&)>;

    Connection(Connection const&) = delete;
    Connection& operator= (Connection const&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator= (Connection&&) = delete;

    ~Connection()
    {
        stop();
    }

    template<typename... Args>
    static SelfPtr create(Args&&... args)
    {
        return SelfPtr(new Connection(std::forward<Args>(args)...));
    }

    void run()
    {
        if (m_running.exchange(true)) {
            return;
        }
        start_recv_header();
    }

    void stop()
    {
        do_shutdown(boost::asio::error::operation_aborted);
    }

    void send_message(const MessagePtr& msg)
    {
        if (is_stopped()) {
            return;
        }

        try {
            auto data = RawDataType(msg->size());
            netpacker::put(data.begin(), data.end(), *msg);

            {
                std::lock_guard<std::mutex> lock(m_send_queue_mutex);
                m_send_queue.push(std::move(data));
            }

            start_send();
        }
        catch (const std::exception& e) {
            do_shutdown(boost::system::error_code(boost::system::errc::protocol_error,
                boost::system::system_category()));
        }
    }

    void set_on_recv_message_cb(OnRecvMessageCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_on_recv_message_cb = std::move(handler);
    }

    void set_on_disconnect_cb(OnDisconnectCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_on_disconnect_cb = std::move(handler);
    }

private:
    Connection(SocketType&& socket)
        : m_socket(std::move(socket)),
          m_running(false),
          m_stopped(false),
          m_sending(false)
    {
    }

    bool is_running() const noexcept
    {
        return m_running.load() && !m_stopped.load();
    }

    bool is_stopped() const noexcept
    {
        return m_stopped.load();
    }
    
    bool is_sending() const noexcept
    {
        return m_sending.load();
    }

    void do_shutdown(const boost::system::error_code& error = {})
    {     
        do_close(error, true);
    }

    void do_close(const boost::system::error_code& error = {}, bool with_shutdown = false)
    {
        if (m_stopped.exchange(true)) {
            return;
        }

        boost::system::error_code ec;
        if (with_shutdown) {
            m_socket.shutdown(SocketType::shutdown_both, ec);
        }
        m_socket.close(ec);

        if (!is_sending()) {
            clear_sending_queue();
        }

        call_on_disconnect_cb(error);
    }

    void clear_sending_queue()
    {
        std::unique_lock lock(m_send_queue_mutex);
        std::queue<RawDataType> empty;
        std::swap(m_send_queue, empty);
    }

    void start_recv_header()
    {
        if (is_stopped()) {
            return;
        }

        m_header_buffer.resize(message::header::Header::size());

        auto self = shared_from_this();
        boost::asio::async_read(m_socket, boost::asio::buffer(m_header_buffer),
            [self](auto&&... args) {
                self->on_read_header(std::forward<decltype(args)>(args)...);
            }
        );
    }

    void on_read_header(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (is_stopped()) {
            return;
        }

        if (error) {
            do_close(error);
            return;
        }

        try {
            auto pos = m_header_buffer.begin();
            auto header = netpacker::get<message::header::Header>(pos, m_header_buffer.end());

            if (header.length > header.size()) {
                size_t need_bytes_size = header.length - header.size();
                m_message_buffer.resize(header.length);

                std::copy(m_header_buffer.begin(), m_header_buffer.end(), m_message_buffer.begin());

                auto self = shared_from_this();
                boost::asio::async_read(m_socket,
                    boost::asio::buffer(m_message_buffer.data() + header.size(), need_bytes_size),
                    [self](auto&&... args) {
                        self->on_read_message(std::forward<decltype(args)>(args)...);
                    }
                );
                return;
            }

            auto message = std::make_shared<message::Message>();
            message->header = std::move(header);

            call_on_recv_message_cb(std::move(message));
            start_recv_header();
        }
        catch (const std::exception& e) {
            do_shutdown(boost::system::error_code(boost::system::errc::protocol_error,
                boost::system::system_category()));
        }
    }

    void on_read_message(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (is_stopped()) {
            return;
        }

        if (error) {
            do_close(error);
            return;
        }

        try {
            auto pos = m_message_buffer.begin();
            auto message = netpacker::get<message::Message>(pos, m_message_buffer.end());

            auto ptr = std::make_shared<message::Message>(std::move(message));
            call_on_recv_message_cb(std::move(ptr));
            start_recv_header();
        }
        catch (const std::exception& e) {
            do_shutdown(boost::system::error_code(boost::system::errc::protocol_error,
                boost::system::system_category())
            );
        }
    }

    void start_send()
    {
        if (is_stopped()) {
            clear_sending_queue();
            return;
        }

        if (m_sending.exchange(true)) {
            //already in send process
            return;
        }

        RawDataType* current_buffer = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_send_queue_mutex);
            if (m_send_queue.empty()) {
                m_sending = false;
                return;
            }
            current_buffer = &m_send_queue.front();
        }

        auto self = shared_from_this();
        boost::asio::async_write(m_socket, boost::asio::buffer(*current_buffer),
            [self](auto&&... args) {
                self->on_write_complete(std::forward<decltype(args)>(args)...);
            }
        );
    }

    void on_write_complete(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (is_stopped()) {
            clear_sending_queue();
            return;
        }

        if (error) {
            m_sending = false;
            do_close(error);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_send_queue_mutex);
            if (!m_send_queue.empty()) {
                m_send_queue.pop();
            }
        }

        m_sending = false;
        start_send();
    }

    void call_on_recv_message_cb(MessagePtr&& message)
    {
        OnRecvMessageCb recv_message_cb;
        {
            std::shared_lock lock(m_callback_mutex);
            recv_message_cb = m_on_recv_message_cb;
        }

        if (recv_message_cb) {
            boost::asio::post(m_socket.get_executor(),
                [recv_message_cb = std::move(recv_message_cb),
                    message = std::move(message)]() mutable {
                    recv_message_cb(std::move(message));
                }
            );
        }
    }

    void call_on_disconnect_cb(const boost::system::error_code& error)
    {
        OnRecvMessageCb recv_message_cb;
        OnDisconnectCb disconnect_cb;
        {
            std::unique_lock lock(m_callback_mutex);
            disconnect_cb = std::move(m_on_disconnect_cb);
            m_on_recv_message_cb = nullptr;
        }

        if (disconnect_cb) {
            boost::asio::post(m_socket.get_executor(),
                [disconnect_cb = std::move(disconnect_cb), error]() mutable {
                    disconnect_cb(error);
                }
            );
        }
    }

    SocketType m_socket;

    RawDataType m_header_buffer;
    RawDataType m_message_buffer;

    std::mutex m_send_queue_mutex;
    std::queue<RawDataType> m_send_queue;

    std::atomic_bool m_running;
    std::atomic_bool m_stopped;
    std::atomic_bool m_sending;

    std::shared_mutex m_callback_mutex;
    OnRecvMessageCb m_on_recv_message_cb;
    OnDisconnectCb m_on_disconnect_cb;
};

} // namespace diameter::core::io

#endif
