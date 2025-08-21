#ifndef DIAMETER_CORE_MANAGER_MANAGER_H
#define DIAMETER_CORE_MANAGER_MANAGER_H

#include <string>
#include <utility>

#include <diameter/core/config/config.h>
#include <diameter/core/peer/ipeer.h>

namespace diameter::core::manager {

class Manager {
public:
    using OnRecvMessageCb = std::function<void(message::Message&&, const peer::IPeer&)>;
    using OnPeerStateCb = std::function<void(const peer::IPeer&)>;
    using PeerMapType = std::map<std::string, peer::IPeer>;

    explicit Manager(config::Config&& conf)
        : m_config(std::move(conf))
    {}

    Manager(Manager const&) = delete;
    Manager& operator= (Manager const&) = delete;
    Manager(Manager&&) = default;
    Manager& operator= (Manager&&) = default;

    void configure(config::Config&& conf)
    {

    }

    peer::IPeer* get_peer(const std::string& peer_name)
    {

    }

    void set_on_recv_message_cb(OnRecvMessageCb&& callback)
    {

    }

    void set_on_peer_open_state_cb(OnPeerStateCb&& callback)
    {

    }

    void set_on_peer_closed_state_cb(OnPeerStateCb&& callback)
    {

    }

private:
    config::Config m_config;
    PeerMapType m_peers;

    OnRecvMessageCb m_on_recv_message_callback;
    OnPeerStateCb m_on_peer_open_state_callback;
    OnPeerStateCb m_on_peer_closed_state_callback;
};

} // namespace diameter::core::manager

#endif