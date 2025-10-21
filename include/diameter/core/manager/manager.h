#ifndef DIAMETER_CORE_MANAGER_MANAGER_H
#define DIAMETER_CORE_MANAGER_MANAGER_H

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/asio/io_context.hpp>

#include <diameter/core/config/config.h>
#include <diameter/core/io/acceptor.h>
#include <diameter/core/io/connection.h>
#include <diameter/core/io/connector.h>
#include <diameter/core/peer/ipeer.h>

namespace diameter::core::manager {

class Manager
{
public:
    using AcceptorPtr = std::shared_ptr<io::Acceptor>;
    using ConnectorPtr = std::shared_ptr<io::Connector>;
    using PeerPtr = std::shared_ptr<peer::IPeer>;

    using OnRecvMessageCb = std::function<void(message::Message&&, const peer::IPeer&)>;
    using OnPeerStateCb = std::function<void(const peer::IPeer&)>;
    using PeerMap = std::unordered_map<std::string, PeerPtr>;
    using AcceptorMap = std::unordered_map<std::string, AcceptorPtr>;
    using ConnectorMap = std::unordered_map<std::string, ConnectorPtr>;

    Manager(boost::asio::io_context& ioc)
        : m_ioc(ioc)
    {
    }

    Manager(Manager const&) = delete;
    Manager& operator= (Manager const&) = delete;
    Manager(Manager&&) = default;
    Manager& operator= (Manager&&) = default;

    void configure(config::Config&& conf)
    {
        auto lk = std::lock_guard(m_mutex);
        m_config = std::move(conf);

        for (auto it = m_acceptors.begin(); it != m_acceptors.end();) {
            auto& name = it->first;
            auto& acceptor = it->second;
            auto is_removed = (m_config.acceptors.find(name) == m_config.acceptors.end());

            if (is_removed) {
                acceptor->stop();
                it = m_acceptors.erase(it);
            }
            else {
                // TODO: Need configure acceptor
                ++it;
            }
        }

        for (const auto& pair : m_config.acceptors) {
            if (m_acceptors.find(pair.first) == m_acceptors.end()) {
                auto& acceptor_conf = pair.second;
                create_acceptor(pair.first, acceptor_conf);
            }
        }

        for (auto it = m_connectors.begin(); it != m_connectors.end();) {
            auto& name = it->first;
            auto& connector = it->second;

            // Try to find peer in config with connector name
            auto peer_it = m_config.peers.find(name);
            auto is_removed = (peer_it == m_config.peers.end());

            if (is_removed) {
                connector->stop();
                it = m_connectors.erase(it);
            }
            else if (peer_it->second.role != peer::IPeer::Role::INITIATOR) {
                connector->stop();
                it = m_connectors.erase(it);
            }
            else {
                // TODO: Need configure connector
                ++it;
            }
        }

        for (auto it = m_peers.begin(); it != m_peers.end();) {
            auto& name = it->first;
            auto& peer = it->second;

            auto peer_it = m_config.peers.find(name);
            auto is_removed = (peer_it == m_config.peers.end());
            if (is_removed) {
                peer->stop();
                it = m_peers.erase(it);
            }
            else {
                // TODO: apply config for peer;
                ++it;
            }
        }

        for (const auto& pair : m_config.peers) {
            if (m_peers.find(pair.first) == m_peers.end()) {
                auto& peer_conf = pair.second;
                auto& local_peer_conf = m_config.local_peers.at(
                    peer_conf.local_peer_name); // TODO: need validate it early
                create_peer(pair.first, local_peer_conf, peer_conf);
            }
        }
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
    void create_acceptor(const std::string& name, const config::AcceptorConfig& acceptor_config)
    {
        // TODO SCTP type
        if (acceptor_config.local_addr.type != config::AddrType::TCP) {
            throw std::runtime_error("Unsupported address type");
        }

        auto acceptor = io::Acceptor::create(m_ioc, acceptor_config.local_addr);
        acceptor->set_on_accept_cb([](const boost::system::error_code& error,
                                       io::Acceptor::SocketType&& socket) {
            if (error) {
                return;
            }
        });
        acceptor->run();
        m_acceptors.insert({name, acceptor});
    }

    void create_peer(const std::string& name, const config::LocalPeerConfig& local_peer_config,
        const config::PeerConfig& peer_config)
    {
    }

    void create_connector(const std::string& name, const config::LocalPeerConfig& local_peer_config,
        const config::PeerConfig& peer_config)
    {
    }

private:
    boost::asio::io_context& m_ioc;

    config::Config m_config;
    PeerMap m_peers;
    AcceptorMap m_acceptors;
    ConnectorMap m_connectors;

    std::mutex m_mutex;

    OnRecvMessageCb m_on_recv_message_callback;
    OnPeerStateCb m_on_peer_open_state_callback;
    OnPeerStateCb m_on_peer_closed_state_callback;
};

} // namespace diameter::core::manager

#endif
