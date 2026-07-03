#ifndef DIAMETER_CORE_IO_ACCEPTOR_H
#define DIAMETER_CORE_IO_ACCEPTOR_H

#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <diameter/core/config/config.h>
#include <diameter/log/logger.h>

namespace diameter::core::io {

class Acceptor final : public std::enable_shared_from_this<Acceptor>
{
public:
    using ProtocolType = boost::asio::ip::tcp; // move to template
    using SelfPtr = std::shared_ptr<Acceptor>;
    using AcceptorType = ProtocolType::acceptor;
    using SocketType = ProtocolType::socket;
    using EndpointType = ProtocolType::endpoint;
    using ResolverType = ProtocolType::resolver;
    using ResolverIterType = ResolverType::iterator;
    using ResolverQueryType = ResolverType::query;
    using TimerType = boost::asio::steady_timer;

    using OnAcceptCb = std::function<void(const boost::system::error_code&, SocketType&&)>;
    using OnStartCb = std::function<void(const boost::system::error_code&, const EndpointType&)>;
    using OnStopCb = std::function<void()>;
    using AddrType = config::Addr;

    Acceptor(Acceptor const&) = delete;
    Acceptor& operator= (Acceptor const&) = delete;
    Acceptor(Acceptor&&) = delete;
    Acceptor& operator= (Acceptor&&) = delete;

    ~Acceptor()
    {
        stop();
    }

    template<typename... Args>
    static SelfPtr create(Args&&... args)
    {
        return SelfPtr(new Acceptor(std::forward<Args>(args)...));
    }

    void set_on_accept_cb(OnAcceptCb&& handler)
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_on_accept_cb = std::move(handler);
    }

    void set_on_start_cb(OnStartCb&& handler)
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_on_start_cb = std::move(handler);
    }

    void set_on_stop_cb(OnStopCb&& handler)
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_on_stop_cb = std::move(handler);
    }

    void run()
    {
        if (m_running.exchange(true)) {
            call_on_start_cb(boost::asio::error::already_started, EndpointType{});
            return;
        }
        start_resolve();
    }

    void stop()
    {
        if (!m_running.exchange(false)) {
            return;
        }

        OnStartCb start_cb;
        OnAcceptCb accept_cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            start_cb = std::move(m_on_start_cb);
            accept_cb = std::move(m_on_accept_cb);
        }

        boost::system::error_code ec;
        m_acceptor.cancel(ec);
        m_acceptor.close(ec);

        call_on_stop_cb();
    }

private:
    Acceptor(boost::asio::io_context& ioc, const AddrType& src_address)
        : m_acceptor(ioc),
          m_resolver(ioc),
          m_timer(ioc),
          m_src_address(src_address),
          m_running(false)
    {
    }

    bool is_running()
    {
        return m_running.load();
    }

    void start_resolve()
    {
        ResolverQueryType query(m_src_address.host[0],
            std::to_string(static_cast<unsigned int>(m_src_address.port)));

        DIAMETER_LOG_DEBUG("Resolving " << m_src_address.host[0] << ":" << m_src_address.port);
        m_resolver.async_resolve(query, [self(shared_from_this())](auto&&... args) {
            self->on_resolve_handler(std::forward<decltype(args)>(args)...);
        });
    }

    void on_resolve_handler(const boost::system::error_code& error, ResolverIterType iterator)
    {
        if (!is_running()) {
            return;
        }

        if (error) {
            DIAMETER_LOG_ERROR("Resolve failed: " << error.message());
            call_on_start_cb(error, EndpointType{});
            stop();
            return;
        }

        if (iterator == ResolverType::iterator()) {
            DIAMETER_LOG_ERROR("Resolve failed: No data");
            call_on_start_cb(boost::asio::error::no_data, EndpointType{});
            stop();
            return;
        }

        const EndpointType& src_endpoint = iterator->endpoint();
        DIAMETER_LOG_DEBUG("Resolved " << src_endpoint);

        try {
            m_acceptor.open(src_endpoint.protocol());
            m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            m_acceptor.bind(src_endpoint);
            m_acceptor.listen(boost::asio::socket_base::max_listen_connections);
        }
        catch (const boost::system::system_error& ex) {
            call_on_start_cb(ex.code(), src_endpoint);
            stop();
            return;
        }

        boost::system::error_code ignore;
        auto bind_ep = m_acceptor.local_endpoint(ignore);
        call_on_start_cb(boost::system::error_code{}, bind_ep);
        start_accept();
    }

    void start_accept()
    {
        if (!is_running()) {
            return;
        }

        m_acceptor.async_accept([self(shared_from_this())](auto&&... args) {
            self->on_accept_handler(std::forward<decltype(args)>(args)...);
        });
    }

    void on_accept_handler(const boost::system::error_code& error, SocketType socket)
    {
        if (!is_running() || error == boost::asio::error::operation_aborted) {
            return;
        }

        call_on_accept_cb(error, std::move(socket));

        if (!error) {
            start_accept();
            return;
        }
        DIAMETER_LOG_ERROR("Accept failed: " << error.message());
        start_timer();
    }

    void start_timer()
    {
        m_timer.expires_from_now(std::chrono::seconds(1));
        m_timer.async_wait([self(shared_from_this())](auto&&... args) {
            self->on_timer(std::forward<decltype(args)>(args)...);
        });
    }

    void on_timer(const boost::system::error_code& error)
    {
        if (!is_running() || error == boost::asio::error::operation_aborted) {
            return;
        }
        start_accept();
    }

    void call_on_accept_cb(const boost::system::error_code& error, SocketType&& socket)
    {
        OnAcceptCb accept_cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            accept_cb = m_on_accept_cb;
        }

        if (accept_cb) {
            boost::asio::post(m_acceptor.get_executor(),
                [accept_cb = std::move(accept_cb), error, socket = std::move(socket)]() mutable {
                    accept_cb(error, std::move(socket));
                });
        }
    }

    void call_on_start_cb(const boost::system::error_code& error, const EndpointType& bind_ep)
    {
        OnStartCb start_cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            start_cb = std::move(m_on_start_cb);
        }

        if (start_cb) {
            boost::asio::post(m_acceptor.get_executor(),
                [start_cb = std::move(start_cb), error, bind_ep]() mutable {
                    start_cb(error, bind_ep);
                });
        }
    }

    void call_on_stop_cb()
    {
        OnStopCb stop_cb;
        {
            std::lock_guard<std::mutex> lock(m_callback_mutex);
            stop_cb = std::move(m_on_stop_cb);
        }

        if (stop_cb) {
            boost::asio::post(m_acceptor.get_executor(), [stop_cb = std::move(stop_cb)]() mutable {
                stop_cb();
            });
        }
    }

    AcceptorType m_acceptor;
    ResolverType m_resolver;
    TimerType m_timer;
    AddrType m_src_address;
    std::atomic_bool m_running;

    std::mutex m_callback_mutex;
    OnAcceptCb m_on_accept_cb;
    OnStartCb m_on_start_cb;
    OnStopCb m_on_stop_cb;
};

} // namespace diameter::core::io

#endif
