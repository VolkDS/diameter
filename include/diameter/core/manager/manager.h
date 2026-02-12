#ifndef DIAMETER_CORE_MANAGER_MANAGER_H
#define DIAMETER_CORE_MANAGER_MANAGER_H

#include <memory>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/asio/io_context.hpp>

#include <diameter/application/application.h>
#include <diameter/application/common/common.h>
#include <diameter/core/config/config.h>
#include <diameter/core/controller/incoming_controller.h>
#include <diameter/core/error.h>
#include <diameter/core/io/acceptor.h>
#include <diameter/core/io/connection.h>
#include <diameter/core/io/connector.h>
#include <diameter/core/peer/peer.h>
#include <diameter/log/log.h>
#include <diameter/message/message.h>

namespace diameter::core::manager {

class Manager
{
public:
    using AcceptorPtr = std::shared_ptr<io::Acceptor>;
    using ConnectionPtr = std::shared_ptr<io::Connection>;
    using ConnectorPtr = std::shared_ptr<io::Connector>;
    using PeerPtr = std::shared_ptr<peer::Peer>;
    using IPeerPtr = std::shared_ptr<peer::IPeer>;

    using OnRecvMessageCb = std::function<void(message::Message&&, const peer::IPeer&)>;
    using OnPeerStateCb = std::function<void(const peer::IPeer&)>;
    using PeerMap = std::unordered_map<std::string, PeerPtr>;
    using AcceptorMap = std::unordered_map<std::string, AcceptorPtr>;
    using ConnectorMap = std::unordered_map<std::string, ConnectorPtr>;

    Manager(boost::asio::io_context& ioc)
        : m_ioc(ioc),
          m_incoming_controller(ioc)
    {
        m_incoming_controller.set_on_remote_connection_CER_cb([this](auto&&... args) {
            this->on_remote_connection_CER_handler(std::forward<decltype(args)>(args)...);
        });
    }

    Manager(Manager const&) = delete;
    Manager& operator= (Manager const&) = delete;
    Manager(Manager&&) = default;
    Manager& operator= (Manager&&) = default;

    void configure(config::Config&& conf)
    {
        std::unique_lock lock(m_mutex);
        m_config = std::move(conf);

        for (auto it = m_acceptors.begin(); it != m_acceptors.end();) {
            auto& acceptor_name = it->first;
            auto& acceptor = it->second;
            auto is_removed = (m_config.acceptors.find(acceptor_name) == m_config.acceptors.end());

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
            auto& acceptor_name = pair.first;
            auto& acceptor_config = pair.second;
            if (m_acceptors.find(acceptor_name) == m_acceptors.end()) {
                create_acceptor(acceptor_name, acceptor_config);
            }
        }

        for (auto it = m_connectors.begin(); it != m_connectors.end();) {
            auto peer_name = it->first;
            auto connector = it->second;

            // Try to find peer in config with connector name
            auto peer_it = m_config.peers.find(peer_name);
            auto is_removed = (peer_it == m_config.peers.end());

            if (is_removed) {
                it = m_connectors.erase(it);
                connector->stop();
            }
            else if (peer_it->second.role != peer::IPeer::Role::INITIATOR) {
                it = m_connectors.erase(it);
                connector->stop();
            }
            else {
                // TODO: Need configure connector
                ++it;
            }
        }

        for (auto it = m_peers.begin(); it != m_peers.end();) {
            auto peer_name = it->first;
            auto peer = it->second;

            auto peer_it = m_config.peers.find(peer_name);
            auto is_removed = (peer_it == m_config.peers.end());
            if (is_removed) {
                it = m_peers.erase(it);
                peer->stop();
            }
            else {
                // TODO: apply config for peer;
                ++it;
            }
        }

        for (const auto& pair : m_config.peers) {
            auto& peer_name = pair.first;
            auto& peer_config = pair.second;

            if (m_peers.find(peer_name) == m_peers.end()) {
                auto& local_peer_conf = m_config.local_peers.at(
                    peer_config.local_peer_name); // TODO: need validate it early
                create_peer(peer_name, local_peer_conf, peer_config);
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
        DIAMETER_LOG_DEBUG("Create acceptor ["<< name <<"]");
        // TODO SCTP type
        if (acceptor_config.local_addr.type != config::AddrType::TCP) {
            throw std::runtime_error("Unsupported address type");
        }

        auto acceptor = io::Acceptor::create(m_ioc, acceptor_config.local_addr);
        acceptor->set_on_accept_cb([this, acceptor_name = name](auto&&... args) {
            this->on_accept_handler(acceptor_name, std::forward<decltype(args)>(args)...);
        });
        acceptor->run();
        m_acceptors.insert({name, acceptor});
    }

    void create_peer(const std::string& name, const config::LocalPeerConfig& local_peer_config,
        const config::PeerConfig& peer_config)
    {
        auto peer = peer::Peer::create(name, local_peer_config.info.origin_host, local_peer_config.info.origin_realm,
            peer_config.remote_host, peer_config.remote_realm);
        m_peers.insert({name, peer});

        peer::Peer::SelfWPtr wpeer(peer);
        peer->set_on_open_state_cb([wpeer](){
            // TODO: Run DWR timer
        });

        peer->set_on_closed_state_cb([wpeer](){
            // TODO: Run reconnect timer
        });

        peer->set_on_recv_message_cb([wpeer](peer::Peer::MessagePtr&& message){

        });

        peer->set_on_generate_CEA_cb([this](auto&&... args){
            return on_generate_CEA_handler(std::forward<decltype(args)>(args)...);
        });

        // peer->set_on_generate_CER_cb([](){
        //     return peer::Peer::MessagePtr{};
        // };

        if (peer_config.role == peer::IPeer::Role::INITIATOR) {
            if (peer_config.remote_addr.type != config::AddrType::TCP) {
                throw std::runtime_error("Unsupported address type");
            }
            auto connector = io::Connector::create(m_ioc, peer_config.remote_addr, peer_config.local_addr);
            peer->start(connector);
        }
    }

private:
    void on_accept_handler(const std::string& acceptor_name, const boost::system::error_code& error,
        io::Acceptor::SocketType&& socket)
    {
        if (error) {
            return;
        }

        boost::system::error_code ignore_error;
        std::string src_host = socket.local_endpoint(ignore_error).address().to_string();
        uint16_t src_port = socket.local_endpoint(ignore_error).port();
        std::string dst_host = socket.remote_endpoint(ignore_error).address().to_string();
        uint16_t dst_port = socket.remote_endpoint(ignore_error).port();

        DIAMETER_LOG_DEBUG("Incomming connect "
            << src_host << ":" << src_port
            << " <- "
            << dst_host << ":" << dst_port
        );

        auto connection = io::Connection::create(std::move(socket));

        std::shared_lock lock(m_mutex);
        auto it = m_config.acceptors.find(acceptor_name);
        if (it == m_config.acceptors.end()) {
            // Not found acceptor
            connection->stop();
            return;
        }
        auto& acceptor_config = it->second;

        m_incoming_controller.add_new_connection(std::move(connection), acceptor_name, acceptor_config.capability_timeout);
    }

    void on_remote_connection_CER_handler(ConnectionPtr&& connection, const std::string& acceptor_name,
        std::shared_ptr<message::Message>&& CER_message)
    {
        std::shared_lock lock(m_mutex);
        auto it = m_config.acceptors.find(acceptor_name);
        if (it == m_config.acceptors.end()) {
            // Stop connection without CEA because local peer unknown for answer
            connection->stop();
            return;
        }
        auto& acceptor_config = it->second;
        auto& local_peer_conf = m_config.local_peers.at(acceptor_config.local_peer_name);

        peer::PeerInfo remote_peer_info;
        try {
            remote_peer_info = peer::make_peer_info(CER_message);
        }
        catch (const core::Exception& ex) {
            //TODO: send CEA with Error AVP
            connection->stop();
            return;
        }

        auto full_name = peer::detail::make_full_peer_identity(local_peer_conf.info.origin_host, local_peer_conf.info.origin_realm,
            remote_peer_info.origin_host, remote_peer_info.origin_realm);

        for (auto it = m_peers.begin(); it != m_peers.end();) {
            auto peer_name = it->first;
            auto peer = it->second;

            if (peer->full_id() == full_name) {
                auto incoming_data = peer::Peer::IncomingData {std::move(connection), std::move(CER_message), std::move(remote_peer_info)};
                peer->responder_connection_CER(std::move(incoming_data));
                return;
            }
        }
        DIAMETER_LOG_ERROR("Unknown peer [" << full_name << "]");
        //TODO: send CEA: UNKNOWN_PEER
    }

    // <CEA> ::= < Diameter Header: 257 >
    //           { Result-Code }
    //           { Origin-Host }
    //           { Origin-Realm }
    //        1* { Host-IP-Address }
    //           { Vendor-Id }
    //           { Product-Name }
    //           [ Origin-State-Id ]
    //           [ Error-Message ]
    //           [ Failed-AVP ]
    //         * [ Supported-Vendor-Id ]
    //         * [ Auth-Application-Id ]
    //         * [ Inband-Security-Id ]
    //         * [ Acct-Application-Id ]
    //         * [ Vendor-Specific-Application-Id ]
    //           [ Firmware-Revision ]
    //         * [ AVP ]
    peer::Peer::MessagePtr on_generate_CEA_handler(const peer::Peer::MessagePtr& CER_message, const peer::PeerInfo& remote_peer_info)
    {
        auto builder = application::common::CapabilitiesExchangeBuilder();
        auto CEA_message = builder
            .set_hop_by_hop(CER_message->header.hop_by_hop)
            .set_end_to_end(CER_message->header.end_to_end)
            .add_result_code(application::common::ResultCodeV::SUCCESS)
            .add_origin_host("volkov.mnc000.mcc250.3gppnetwork.org")
            .add_origin_realm("epc.mnc000.mcc250.3gppnetwork.org")
            .add_host_ip_address("192.168.13.71")
            .add_vendor_id(1000)
            .add_product_name("TEST")
            .add_supported_vendor_id(10415)
            .add_auth_application_id(16777238)
            .add_inband_security_id(0)
            .build();
        return CEA_message;
    }

private:
    boost::asio::io_context& m_ioc;

    config::Config m_config;
    PeerMap m_peers;
    AcceptorMap m_acceptors;
    ConnectorMap m_connectors;

    controller::IncomingController m_incoming_controller;

    std::shared_mutex m_mutex;

    OnRecvMessageCb m_on_recv_message_callback;
    OnPeerStateCb m_on_peer_open_state_callback;
    OnPeerStateCb m_on_peer_closed_state_callback;
};

} // namespace diameter::core::manager

#endif
