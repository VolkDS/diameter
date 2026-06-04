#ifndef DIAMETER_CORE_IO_CONNECTOR_H
#define DIAMETER_CORE_IO_CONNECTOR_H

#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <diameter/core/config/config.h>
#include <diameter/log/log.h>

namespace diameter::core::io {

class Connector : public std::enable_shared_from_this<Connector>
{
public:
    using ProtocolType = boost::asio::ip::tcp; // move to template
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

        if (!m_src_address.empty()) {
            start_local_bind();
        }
        else {
            start_connect();
        }
    }

    void stop()
    {
        if (!m_running.exchange(false)) {
            return;
        }

        if (m_stopped.exchange(true)) {
            return;
        }
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
          m_dst_address(dst_address),
          m_src_address(src_address),
          m_running(false),
          m_stopped(false)
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

    void start_local_bind()
    {
        DIAMETER_LOG_DEBUG("start_local_bind()");
        ResolverQueryType query(m_src_address.host[0],
            std::to_string(static_cast<unsigned int>(m_src_address.port)));

        auto self = shared_from_this();
        m_resolver.async_resolve(query, [self](auto&&... args) {
            self->on_src_address_resolve_handler(std::forward<decltype(args)>(args)...);
        });
    }

    void on_src_address_resolve_handler(const boost::system::error_code& error,
        ResolverIterType iterator)
    {
        DIAMETER_LOG_DEBUG("on_src_address_resolve_handler()");
        if (is_stopped()) {
            return;
        }

        if (error) {
            start_timer();
            return;
        }

        if (iterator == ResolverType::iterator()) {
            start_timer();
            return;
        }

        for (auto it = iterator; it != ResolverType::iterator(); ++it) {
            EndpointType endpoint = it->endpoint();
            try {
                m_socket.open(endpoint.protocol());
                m_socket.set_option(boost::asio::socket_base::reuse_address(true));
                m_socket.bind(endpoint);

                // Success - start connection and break
                start_connect();
                return;
            }
            catch (const boost::system::system_error& ex) {
                DIAMETER_LOG_ERROR("Bind failed for " << endpoint.address().to_string() << ": "
                                                      << ex.what());
                continue;
            }
        }

        // All endpoints failed
        start_timer();
    }

    void start_connect()
    {
        DIAMETER_LOG_DEBUG("start_connect()");
        ResolverQueryType query(m_dst_address.host[0],
            std::to_string(static_cast<unsigned int>(m_dst_address.port)));

        auto self = shared_from_this();
        m_resolver.async_resolve(query, [self](auto&&... args) {
            self->on_dst_address_resolve_handler(std::forward<decltype(args)>(args)...);
        });
    }

    void on_dst_address_resolve_handler(const boost::system::error_code& error,
        ResolverIterType iterator)
    {
        DIAMETER_LOG_DEBUG("on_dst_address_resolve_handler()");
        if (is_stopped()) {
            return;
        }

        if (error) {
            start_timer();
            return;
        }

        async_connect(iterator);
    }

    void async_connect(ResolverIterType iterator)
    {
        DIAMETER_LOG_DEBUG("async_connect()");
        if (iterator == ResolverType::iterator()) {
            start_timer();
            return;
        }

        EndpointType endpoint = iterator->endpoint();
        auto self = shared_from_this();
        m_socket.async_connect(endpoint, [self, iterator](auto&&... args) {
            self->on_async_connect_handler(iterator, std::forward<decltype(args)>(args)...);
        });
    }

    void on_async_connect_handler(ResolverIterType iterator, const boost::system::error_code& error)
    {
        DIAMETER_LOG_DEBUG("on_async_connect_handler()");
        if (is_stopped()) {
            return;
        }

        if (error) {
            DIAMETER_LOG_ERROR("on_async_connect(): " << error.message());
            async_connect(++iterator);
            return;
        }

        call_on_connect_cb(error);
    }

    void start_timer()
    {
        if (m_socket.is_open()) {
            m_socket.close();
        }

        // TODO: config
        DIAMETER_LOG_DEBUG("start_timer()");
        auto self = shared_from_this();
        m_timer.expires_from_now(std::chrono::seconds(1));
        m_timer.async_wait([self](auto&&... args) {
            self->on_timer(std::forward<decltype(args)>(args)...);
        });
    }

    void on_timer(const boost::system::error_code& error)
    {
        if (is_stopped() || error == boost::asio::error::operation_aborted) {
            return;
        }

        if (!m_src_address.empty()) {
            start_local_bind();
        }
        else {
            start_connect();
        }
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
    std::atomic_bool m_stopped;

    std::mutex m_callback_mutex;
    OnConnectCb m_on_connect_cb;
    OnStopCb m_on_stop_cb;
};

} // namespace diameter::core::io

#endif
