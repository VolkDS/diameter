#ifndef DIAMETER_CORE_CONTROLLER_WATCHDOG_CONTROLLER_H
#define DIAMETER_CORE_CONTROLLER_WATCHDOG_CONTROLLER_H

#include <memory>
#include <shared_mutex>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <diameter/core/fsm.h>
#include <diameter/log/log.h>
#include <diameter/message/message.h>

namespace diameter::core::controller {

// RFC 3539 3.4.1. Algorithm Overview
// The watchdog behavior is controlled by an algorithm defined in this section.
// This algorithm is appropriate for use either within primary/secondary or
// load balancing configurations.
class WatchdogController
{
public:
    struct Callbacks
    {
        using OnActionCb = std::function<void()>;

        std::shared_mutex m_mutex;

        OnActionCb on_failover_cb;
        OnActionCb on_failback_cb;
        OnActionCb on_send_DWR_cb;
    };

    enum class States : uint32_t
    {
        INITIAL,
        OKAY,
        SUSPECT,
        DOWN,
        REOPEN
    };

    enum class Events : uint32_t
    {
        RCV_DWA,
        RCV_MESSAGE,
        TIMER_EXPIRES,
        CONNECTION_UP,
        CONNECTION_DOWN,
    };

    using FsmUserDataType = std::nullptr_t;
    using FsmType = diameter::core::StateMachine<States, Events, WatchdogController, FsmUserDataType&&>;
    using FsmTransitionTableType = FsmType::transition_table_t;
    using FsmActionTableType = FsmType::action_table_t;

    WatchdogController(boost::asio::io_context& ioc)
        : m_ioc(ioc),
          m_timer(ioc),
          m_fsm(this, States::INITIAL, &m_fsm_transition_table)
    {
    }

    void set_on_failover_cb();
    void set_on_failback_cb();
    void set_on_send_DWR_cb();

    void set_timer(const std::chrono::steady_clock::duration& timeout)
    {
        m_timer.expires_from_now(timeout);
        m_timer.async_wait([this](auto&&... args) {
            on_timer_cb(std::forward<decltype(args)>(args)...);
        });
    }

    void recv_message(const std::shared_ptr<message::Message>& message)
    {

    }

    void on_connection_open()
    {

    }

    void on_connection_close()
    {
        
    }

private:

    bool process_fsm_event(const Events& event)
    {
        return process_fsm_event(event, nullptr);
    }

    bool process_fsm_event(const Events& event, FsmUserDataType&& ud)
    {
        //DIAMETER_LOG_DEBUG("Event " << event << " in state " << m_fsm.state());
        bool processed = m_fsm.process_event(event, std::forward<FsmUserDataType>(ud));
        //DIAMETER_LOG_DEBUG("New state " << m_fsm.state());
        return processed;
    }

    void on_timer_cb(const boost::system::error_code& error)
    {
        if (error == boost::asio::error::operation_aborted) {
            return;
        }
    }

    void throwaway(FsmUserDataType&& ud)
    {

    }

    void open(FsmUserDataType&& ud)
    {

    }

private:
    boost::asio::io_context& m_ioc;
    boost::asio::steady_timer m_timer;

    static const FsmTransitionTableType m_fsm_transition_table;
    FsmType m_fsm;
};

}

#endif
