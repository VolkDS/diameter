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
#include <diameter/core/controller/message_controller.h>
#include <diameter/core/controller/watchdog_controller.h>
#include <diameter/core/error.h>
#include <diameter/core/ids_generator.h>
#include <diameter/core/io/acceptor.h>
#include <diameter/core/io/connection.h>
#include <diameter/core/io/connector.h>
#include <diameter/core/peer/peer.h>
#include <diameter/log/log.h>
#include <diameter/message/message.h>

namespace diameter::core::manager {

namespace detail {

inline int64_t make_connection_id()
{
    // Make non-zero ConnectionId
    static std::atomic<int64_t> last_id = -1; // to start from zero
    return 1 + (++last_id % std::numeric_limits<int64_t>::max());
}

}

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
          m_incoming_controller(ioc),
          m_message_controller(ioc)
    {
        m_incoming_controller.set_on_remote_connection_CER_cb([this](auto&&... args) {
            return this->on_remote_connection_CER_handler(std::forward<decltype(args)>(args)...);
        });
        m_message_controller.set_on_request_timeout_cb([this](auto&&... args) {
            return this->on_request_timeout_handler(std::forward<decltype(args)>(args)...);
        });
    }

    Manager(Manager const&) = delete;
    Manager& operator= (Manager const&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator= (Manager&&) = delete;

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

    // peer::IPeer* get_peer(const std::string& peer_name)
    // {
    // }

    void set_on_recv_message_cb(OnRecvMessageCb&& handler)
    {
        std::unique_lock lock(m_mutex);
        m_on_recv_message_cb = std::move(handler);
    }

    void set_on_peer_open_state_cb(OnPeerStateCb&& handler)
    {
        std::unique_lock lock(m_mutex);
        m_on_peer_open_state_cb = std::move(handler);
    }

    void set_on_peer_closed_state_cb(OnPeerStateCb&& handler)
    {
        std::unique_lock lock(m_mutex);
        m_on_peer_closed_state_cb = std::move(handler);
    }

private:
    void create_acceptor(const std::string& name, const config::AcceptorConfig& acceptor_config)
    {
        DIAMETER_LOG_DEBUG("Create acceptor [" << name << "]");
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
        auto peer = peer::Peer::create(name, local_peer_config.info.origin_host,
            local_peer_config.info.origin_realm, peer_config.remote_host, peer_config.remote_realm);
        m_peers.insert({name, peer});

        peer::Peer::SelfWPtr wpeer(peer);
        peer->set_on_open_state_cb([this, wpeer]() {
            return on_open_state_handler(wpeer);
        });

        peer->set_on_closed_state_cb([this, wpeer]() {
            return on_closed_state_handler(wpeer);
        });

        peer->set_on_recv_message_cb([this, wpeer](auto&&... args) {
            return on_recv_message_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        peer->set_on_recv_CER_cb([this, wpeer](auto&&... args) {
            return on_recv_CER_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        peer->set_on_recv_CEA_cb([this, wpeer](auto&&... args) {
            return on_recv_CEA_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        peer->set_on_recv_DWA_cb([this, wpeer](auto&&... args) {
            return on_recv_DWA_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        peer->set_on_recv_DPA_cb([this, wpeer](auto&&... args) {
            return on_recv_DPA_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        peer->set_on_generate_CER_cb([this, wpeer](auto&&... args) {
            return on_generate_CER_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        peer->set_on_generate_DWR_cb([this, wpeer](auto&&... args) {
            return on_generate_DWR_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        peer->set_on_generate_DPR_cb([this, wpeer](auto&&... args) {
            return on_generate_DPR_handler(wpeer, std::forward<decltype(args)>(args)...);
        });

        if (peer_config.role == peer::IPeer::Role::INITIATOR) {
            if (peer_config.remote_addr.type != config::AddrType::TCP) {
                throw std::runtime_error("Unsupported address type");
            }
            auto connector
                = io::Connector::create(m_ioc, peer_config.remote_addr, peer_config.local_addr);
            peer->start(connector);
        }
    }

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

        m_incoming_controller.add_new_connection(std::move(connection), acceptor_name,
            acceptor_config.capability_timeout);
    }

    void on_remote_connection_CER_handler(ConnectionPtr&& connection,
        const std::string& acceptor_name, peer::Peer::MessagePtr&& CER_message)
    {
        std::shared_lock lock(m_mutex);
        auto it = m_config.acceptors.find(acceptor_name);
        if (it == m_config.acceptors.end()) {
            DIAMETER_LOG_ERROR("Stop connection without CEA because local peer unknown for answer");
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
            DIAMETER_LOG_ERROR("TODO: send CEA with Error AVP");
            connection->stop();
            return;
        }

        auto full_name
            = peer::detail::make_full_peer_identity(local_peer_conf.info, remote_peer_info);

        for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
            auto peer_name = it->first;
            auto peer = it->second;

            if (peer->full_id() == full_name) {
                auto incoming_data = peer::Peer::IncomingData {
                    std::move(connection), std::move(CER_message), std::move(remote_peer_info)};
                peer->responder_connection_CER(std::move(incoming_data));
                return;
            }
        }
        DIAMETER_LOG_ERROR("TODO: send CEA Unknown peer [" << full_name << "]");
        connection->stop();
    }

    void on_request_timeout_handler(const std::string& peer_name, peer::Peer::MessagePtr&& request)
    {
        if (request->header.application_id != message::header::ApplicationV::Common) {
            return;
        }

        switch (request->header.command_code) {
            case application::common::CommandV::CapabilitiesExchange:
            case application::common::CommandV::DeviceWatchdog:
            case application::common::CommandV::DisconnectPeer:
                break;
            default:
                return;
        }

        DIAMETER_LOG_DEBUG("[peer=" << peer_name << "] on_request_timeout_handler()");
        peer::Peer::SelfPtr peer_ptr;
        {
            std::shared_lock lock(m_mutex);
            auto it = m_peers.find(peer_name);
            if (it == m_peers.end()) {
                DIAMETER_LOG_WARNING("[peer=" << peer_name << "] Peer not found");
                return;
            }
            peer_ptr = it->second;
        }
        peer_ptr->timeout();
    }

    void on_recv_message_handler(peer::Peer::SelfWPtr peer_wptr, peer::Peer::MessagePtr&& message)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return;
    }

    void on_open_state_handler(peer::Peer::SelfWPtr peer_wptr)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return;

        DIAMETER_LOG_DEBUG("[peer=" << peer_ptr->name() << "] OPEN");
        // TODO: Run DWR timer
    }

    void on_closed_state_handler(peer::Peer::SelfWPtr peer_wptr)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return;

        DIAMETER_LOG_DEBUG("[peer=" << peer_ptr->name() << "] CLOSED");
        m_message_controller.handle_peer_disconnect(peer_ptr->name());
        // TODO: Run reconnect timer
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
    peer::Peer::MessagePtr on_recv_CER_handler(peer::Peer::SelfWPtr peer_wptr,
        const peer::Peer::MessagePtr& CER_message)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return nullptr;

        std::shared_lock lock(m_mutex);
        auto local_peer_opt = m_config.get_local_peer_config_by_peer(peer_ptr->name());
        lock.unlock();

        if (!local_peer_opt.has_value())
            return nullptr;
        auto& local_peer_config = local_peer_opt.value();

        // TODO check common applications

        auto builder = application::common::CapabilitiesExchangeBuilder();
        builder.set_hop_by_hop(CER_message->header.hop_by_hop)
            .set_end_to_end(CER_message->header.end_to_end)
            .add_result_code(application::common::ResultCodeV::SUCCESS)
            .add_origin_host(local_peer_config.info.origin_host)
            .add_origin_realm(local_peer_config.info.origin_realm);

        for (auto& ip_address : peer_ptr->responder_local_address()) {
            builder.add_host_ip_address(ip_address);
        }

        builder.add_vendor_id(local_peer_config.info.vendor_id)
            .add_product_name(local_peer_config.info.product_name);

        for (auto& supported_vendor_id : local_peer_config.info.supported_vendor_ids) {
            builder.add_supported_vendor_id(supported_vendor_id);
        }

        for (auto& auth_app_id : local_peer_config.info.auth_application_ids) {
            builder.add_auth_application_id(auth_app_id);
        }

        for (auto& acct_app_id : local_peer_config.info.acct_application_ids) {
            builder.add_acct_application_id(acct_app_id);
        }

        for (auto& vsa_id : local_peer_config.info.vendor_specific_application_ids) {
            // TODO different types
            builder.add_vendor_specific_auth_application_id(vsa_id.first, vsa_id.second);
        }

        if (local_peer_config.info.inband_security_supported) {
            builder.add_inband_security_id(1);
        }
        else {
            builder.add_inband_security_id(0);
        }

        return builder.build();
    }

    bool on_recv_CEA_handler(peer::Peer::SelfWPtr peer_wptr, const peer::Peer::MessagePtr& CEA_message)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return false;

        return m_message_controller.handle_response_without_cb(CEA_message, peer_ptr->name());
    }

    bool on_recv_DWA_handler(peer::Peer::SelfWPtr peer_wptr, const peer::Peer::MessagePtr& DWA_message)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return false;

        return m_message_controller.handle_response_without_cb(DWA_message, peer_ptr->name());
    }

    bool on_recv_DPA_handler(peer::Peer::SelfWPtr peer_wptr, const peer::Peer::MessagePtr& DPA_message)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return false;

        return m_message_controller.handle_response_without_cb(DPA_message, peer_ptr->name());
    }

    peer::Peer::MessagePtr on_generate_CER_handler(peer::Peer::SelfWPtr peer_wptr)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return nullptr;

        std::shared_lock lock(m_mutex);
        auto peer_config_opt = m_config.get_peer_config(peer_ptr->name());
        auto local_peer_opt = m_config.get_local_peer_config_by_peer(peer_ptr->name());
        lock.unlock();

        if (!local_peer_opt.has_value() || !peer_config_opt.has_value())
            return nullptr;
        auto& local_peer_config = local_peer_opt.value();
        auto& peer_config = peer_config_opt.value();

        auto builder = application::common::CapabilitiesExchangeBuilder();
        builder.set_hop_by_hop(m_ids_generator.next_hop_by_hop())
            .set_end_to_end(m_ids_generator.next_end_to_end())
            .add_origin_host(local_peer_config.info.origin_host)
            .add_origin_realm(local_peer_config.info.origin_realm);

        for (auto& ip_address : peer_ptr->initiator_local_address()) {
            builder.add_host_ip_address(ip_address);
        }

        builder.add_vendor_id(local_peer_config.info.vendor_id)
            .add_product_name(local_peer_config.info.product_name);

        for (auto& supported_vendor_id : local_peer_config.info.supported_vendor_ids) {
            builder.add_supported_vendor_id(supported_vendor_id);
        }

        for (auto& auth_app_id : local_peer_config.info.auth_application_ids) {
            builder.add_auth_application_id(auth_app_id);
        }

        for (auto& acct_app_id : local_peer_config.info.acct_application_ids) {
            builder.add_acct_application_id(acct_app_id);
        }

        for (auto& vsa_id : local_peer_config.info.vendor_specific_application_ids) {
            // TODO different types
            builder.add_vendor_specific_auth_application_id(vsa_id.first, vsa_id.second);
        }

        if (local_peer_config.info.inband_security_supported) {
            builder.add_inband_security_id(1);
        }
        else {
            builder.add_inband_security_id(0);
        }

        peer::Peer::MessagePtr CER_message = builder.build();
        m_message_controller.push_request(CER_message, peer_ptr->name(),
            peer_config.request_timeout);
        return CER_message;
    }

    peer::Peer::MessagePtr on_generate_DWR_handler(peer::Peer::SelfWPtr peer_wptr)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return nullptr;

        std::shared_lock lock(m_mutex);
        auto peer_config_opt = m_config.get_peer_config(peer_ptr->name());
        auto local_peer_opt = m_config.get_local_peer_config_by_peer(peer_ptr->name());
        lock.unlock();

        if (!local_peer_opt.has_value() || !peer_config_opt.has_value())
            return nullptr;
        auto& local_peer_config = local_peer_opt.value();
        auto& peer_config = peer_config_opt.value();

        auto builder = application::common::DeviceWatchdogBuilder();
        builder.set_hop_by_hop(m_ids_generator.next_hop_by_hop())
            .set_end_to_end(m_ids_generator.next_end_to_end())
            .add_origin_host(local_peer_config.info.origin_host)
            .add_origin_realm(local_peer_config.info.origin_realm);

        peer::Peer::MessagePtr DWR_message = builder.build();
        m_message_controller.push_request(DWR_message, peer_ptr->name(),
            peer_config.request_timeout);
        return DWR_message;
    }

    peer::Peer::MessagePtr on_generate_DPR_handler(peer::Peer::SelfWPtr peer_wptr)
    {
        auto peer_ptr = peer_wptr.lock();
        if (!peer_ptr)
            return nullptr;

        std::shared_lock lock(m_mutex);
        auto peer_config_opt = m_config.get_peer_config(peer_ptr->name());
        auto local_peer_opt = m_config.get_local_peer_config_by_peer(peer_ptr->name());
        lock.unlock();

        if (!local_peer_opt.has_value() || !peer_config_opt.has_value())
            return nullptr;
        auto& local_peer_config = local_peer_opt.value();
        auto& peer_config = peer_config_opt.value();

        auto builder = application::common::DisconnectPeerBuilder();
        builder.set_hop_by_hop(m_ids_generator.next_hop_by_hop())
            .set_end_to_end(m_ids_generator.next_end_to_end())
            .add_origin_host(local_peer_config.info.origin_host)
            .add_origin_realm(local_peer_config.info.origin_realm)
            .add_disconnect_cause(0); // REBOOTING

        peer::Peer::MessagePtr DPR_message = builder.build();
        m_message_controller.push_request(DPR_message, peer_ptr->name(),
            peer_config.request_timeout);
        return DPR_message;
    }

private:
    boost::asio::io_context& m_ioc;

    config::Config m_config;
    PeerMap m_peers;
    AcceptorMap m_acceptors;
    ConnectorMap m_connectors;

    IdsGenerator m_ids_generator;

    controller::IncomingController m_incoming_controller;
    controller::MessageController m_message_controller;

    std::shared_mutex m_mutex;

    OnRecvMessageCb m_on_recv_message_cb;
    OnPeerStateCb m_on_peer_open_state_cb;
    OnPeerStateCb m_on_peer_closed_state_cb;
};

} // namespace diameter::core::manager

#endif
