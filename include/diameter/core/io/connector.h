#ifndef DIAMETER_CORE_IO_CONNECTOR_H
#define DIAMETER_CORE_IO_CONNECTOR_H

#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <diameter/core/config/config.h>

namespace diameter::core::io {

class Connector
    : public std::enable_shared_from_this<Connector>
{
public:
    using ProtocolType = boost::asio::ip::tcp; //move to template
    using SelfPtr = std::shared_ptr<Connector>;
    using SocketType = ProtocolType::socket;
    using EndpointType = ProtocolType::endpoint;
    using ResolverType = ProtocolType::resolver;
    using ResolverIterType = ResolverType::iterator;
    using ResolverQueryType = ResolverType::query;
    using TimerType = boost::asio::steady_timer;

    using OnConnectCb = std::function<void(const boost::system::error_code&, SocketType&&)>;
    using OnStopCb = std::function<void()>;
    using AddrType = config::Addr;

    Connector(Connector const&) = delete;
    Connector& operator= (Connector const&) = delete;
    Connector(Connector&&) = delete;
    Connector& operator= (Connector&&) = delete;

    ~Connector() = default;

    template<typename... Args>
    static SelfPtr create(Args&&... args)
    {
        return SelfPtr(new Connector(std::forward<Args>(args)...));
    }

    void run()
    {
        if (m_running.exchange(true)) {
            return;
        }
    }

    void stop()
    {
    }

    void set_on_connect_cb(OnConnectCb&& handler)
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_on_connect_cb = std::move(handler);
    }

    void set_on_stop_cb(OnStopCb&& handler)
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_on_stop_cb = std::move(handler);
    }

private:
    Connector(boost::asio::io_context& ioc, const AddrType& dst_address,
        const AddrType& src_address)
        : m_socket(ioc),
          m_resolver(ioc),
          m_timer(ioc),
          m_src_address(src_address),
          m_running(false)
    {
    }

    void call_on_connect_cb(const boost::system::error_code& error)
    {
        OnConnectCb connect_cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            connect_cb = std::move(m_on_connect_cb);
        }

        if (connect_cb) {
            boost::asio::post(m_socket.get_executor(), [connect_cb = std::move(connect_cb), error,
                                                           socket = std::move(m_socket)]() mutable {
                connect_cb(error, std::move(socket));
            });
        }
    }

    SocketType m_socket;
    ResolverType m_resolver;
    TimerType m_timer;
    AddrType m_dst_address;
    AddrType m_src_address;
    std::atomic_bool m_running;

    std::mutex m_callback_mutex;
    OnConnectCb m_on_connect_cb;
    OnStopCb m_on_stop_cb;
};

} // namespace diameter::core::io

#endif
