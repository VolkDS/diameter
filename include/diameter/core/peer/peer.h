#ifndef DIAMETER_CORE_PEER_PEER_H
#define DIAMETER_CORE_PEER_PEER_H

#include <memory>
#include <string>
#include <variant>

#include <diameter/application/base/command.h>
#include <diameter/core/io/connection.h>
#include <diameter/core/io/connector.h>
#include <diameter/core/fsm.h>
#include <diameter/core/peer/info.h>
#include <diameter/core/peer/ipeer.h>
#include <diameter/message/message.h>

namespace diameter::core::peer {

class Peer
    : public std::enable_shared_from_this<Peer>
{
public:
    using SelfPtr = std::shared_ptr<Peer>;
    using SelfWPtr = std::weak_ptr<Peer>;
    using ConnectionPtr = std::shared_ptr<io::Connection>;
    using ConnectorPtr = std::shared_ptr<io::Connector>;
    using MessagePtr = std::shared_ptr<message::Message>;

    using FsmUserDataType = std::variant<ConnectorPtr, ConnectionPtr, MessagePtr, std::nullptr_t>;

    using OnRecvMessageCb = std::function<void(MessagePtr&&)>;
    //The stable states that a state machine may be in are Closed, I-Open, and R-Open
    using OnStableStateCb = std::function<void()>;
    using OnRecvCommonMessageCb = std::function<MessagePtr(MessagePtr&&)>;

    enum class States : uint32_t
    {
        CLOSED,
        WAIT_CONN_ACK,
        WAIT_CEA,
        ELECT,
        WAIT_RETURNS,
        ROPEN,
        IOPEN,
        CLOSING
    };

    enum class Events : uint32_t
    {
        START,
        R_CONN_CER,
        I_RCV_CONN_ACK,
        I_RCV_CONN_NACK,
        TIMEOUT,
        I_RCV_CEA,
        I_PEER_DISC,
        R_PEER_DISC,
        WIN_ELECTION,
        SEND_MESSAGE,
        R_RCV_MESSAGE,
        R_RCV_DWR,
        R_RCV_DWA,
        STOP,
        R_RCV_DPR,
        I_RCV_MESSAGE,
        I_RCV_DWR,
        I_RCV_DWA,
        I_RCV_DPR,
        I_RCV_DPA,
        R_RCV_DPA
    };

    using FsmType = diameter::core::StateMachine<States, Events, Peer, FsmUserDataType&&>;
    using FsmTransitionTableType = FsmType::transition_table_t;

    using IdentityType = std::string;

    using on_send_message_error_t = std::function<void(message::Message&&)>;

    template<typename... Args>
    static SelfPtr create(Args&&... args)
    {
        return SelfPtr(new Peer(std::forward<Args>(args)...));
    }

    Peer(Peer const&) = delete;
    Peer& operator= (Peer const&) = delete;
    Peer(Peer&&) = delete;
    Peer& operator= (Peer&&) = delete;

    void start(const ConnectorPtr& connector)
    {
        m_fsm.process_event(Events::START,  std::move(connector));
    }

    void stop()
    {
        m_fsm.process_event(Events::STOP);
    }

    void responder_connection_CER(const ConnectionPtr& connect)
    {
        m_fsm.process_event(Events::R_CONN_CER, std::move(connect));
    }

    // void initiator_recv_connection_ack();
    // void initiator_recv_connection_nack();
    void timeout();
    // void initiator_recv_CEA();

    // void initiator_recv_non_CEA();
    void win_election();

    void send_message(const MessagePtr& message)
    {
        bool processed = m_fsm.process_event(Events::SEND_MESSAGE, std::move(message));
        if (!processed) {
        }
    }

    void set_on_recv_message_cb(OnRecvMessageCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_on_recv_message_cb = std::move(handler);
    }

    void set_on_open_state_cb(OnStableStateCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_on_open_state_cb = std::move(handler);
    }

    void set_on_closed_state_cb(OnStableStateCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_on_closed_state_cb = std::move(handler);
    }

private:
    Peer(const IdentityType& local_host, const IdentityType& local_realm,
        const IdentityType& remote_host, const IdentityType& remote_realm)
        : m_local_host(local_host),
          m_local_realm(local_realm),
          m_remote_host(remote_host),
          m_remote_realm(remote_realm),
          m_fsm(this, States::CLOSED, &m_fsm_transition_table)
    {
    }

    static const FsmTransitionTableType m_fsm_transition_table;

    // A transport connection is initiated with the peer.
    void initiator_start_connection(FsmUserDataType&& ud)
    {
        m_connector = std::get<ConnectorPtr>(std::move(ud));
        auto self = shared_from_this();
        m_connector->set_on_connect_cb([self](const boost::system::error_code& error,
                                       io::Connector::SocketType&& socket) {
            if (error) {
                self->m_fsm.process_event(Events::I_RCV_CONN_NACK);
                return;
            }
            auto connection = io::Connection::create(std::move(socket));
            self->m_fsm.process_event(Events::I_RCV_CONN_ACK, std::move(connection));
        });
        m_connector->run();
    }

    // The incoming connection associated with the R_Conn_CER is accepted as the responder
    // connection.
    void responder_accept(FsmUserDataType&& ud)
    {
        m_responder = std::get<ConnectionPtr>(std::move(ud));
        auto CER_message = std::make_shared<message::Message>(); //TODO Get CER from IncomingController

        auto self = shared_from_this();
        m_responder->set_on_disconnect_cb([self](const boost::system::error_code& error) {
            self->m_fsm.process_event(Events::R_PEER_DISC);
        });
        m_responder->set_on_recv_message_cb([self](MessagePtr&& message) {
            if (message->header.application_id == message::header::ApplicationV::Common) {
                if (message->header.command_flags[message::header::CommandFlag::Request]) {
                    switch (message->header.command_code)
                    {
                        case application::base::CommandV::DeviceWatchdog:
                            self->m_fsm.process_event(Events::R_RCV_DWR, std::move(message));
                            break;
                        case application::base::CommandV::DisconnectPeer:
                            self->m_fsm.process_event(Events::R_RCV_DPR, std::move(message));
                            break;
                        default:
                            self->m_fsm.process_event(Events::R_RCV_MESSAGE, std::move(message));
                            break;
                    }
                }
                else {
                    switch (message->header.command_code)
                    {
                        case application::base::CommandV::DeviceWatchdog:
                            self->m_fsm.process_event(Events::R_RCV_DWR, std::move(message));
                            break;
                        case application::base::CommandV::DisconnectPeer:
                            self->m_fsm.process_event(Events::R_RCV_DPR, std::move(message));
                            break;
                        default:
                            self->m_fsm.process_event(Events::R_RCV_MESSAGE, std::move(message));
                            break;
                    }
                }
            }
            self->m_fsm.process_event(Events::R_RCV_MESSAGE, std::move(message));
        });

        process_CER(CER_message);
    }

    // The incoming connection associated with the R_Conn_CER is disconnected.
    void responder_reject(FsmUserDataType&& ud)
    {
        auto responder = std::get<ConnectionPtr>(std::move(ud));
        responder->stop();
    }

    // A CER message is sent to the peer.
    // Events::I_RCV_CONN_ACK
    void initiator_apply(FsmUserDataType&& ud)
    {
        m_initiator = std::get<ConnectionPtr>(std::move(ud));

        auto self = shared_from_this();
        m_initiator->set_on_disconnect_cb([self](const boost::system::error_code& /*error*/) {
            self->m_fsm.process_event(Events::I_PEER_DISC);
        });
        m_initiator->set_on_recv_message_cb([self](MessagePtr&& message) {
            if (message->header.application_id == message::header::ApplicationV::Common) {
                if (message->header.command_flags[message::header::CommandFlag::Request]) {
                    switch (message->header.command_code)
                    {
                        case application::base::CommandV::DeviceWatchdog:
                            self->m_fsm.process_event(Events::I_RCV_DWR, std::move(message));
                            break;
                        case application::base::CommandV::DisconnectPeer:
                            self->m_fsm.process_event(Events::I_RCV_DPR, std::move(message));
                            break;
                        default:
                            self->m_fsm.process_event(Events::I_RCV_MESSAGE, std::move(message));
                            break;
                    }
                }
                else {
                    switch (message->header.command_code)
                    {
                        case application::base::CommandV::CapabilitiesExchange:
                            self->m_fsm.process_event(Events::I_RCV_CEA, std::move(message));
                            break;
                        case application::base::CommandV::DeviceWatchdog:
                            self->m_fsm.process_event(Events::I_RCV_DWR, std::move(message));
                            break;
                        case application::base::CommandV::DisconnectPeer:
                            self->m_fsm.process_event(Events::I_RCV_DPR, std::move(message));
                            break;
                        default:
                            self->m_fsm.process_event(Events::I_RCV_MESSAGE, std::move(message));
                            break;
                    }
                }
            }
            self->m_fsm.process_event(Events::I_RCV_MESSAGE, std::move(message));
        });
        m_initiator->run();

        initiator_send_CER(nullptr);
    }

    void initiator_send_CER(FsmUserDataType&& ud)
    {
        // TODO: callback for generate CER message
        auto CER_message = std::make_shared<message::Message>();
        m_initiator->send_message(CER_message);

        if (m_fsm.state() == States::ELECT) {
            elect(m_local_host, m_remote_host);
        }
    }

    // A CEA message is sent to the peer.
    void responder_send_CEA(FsmUserDataType&& ud)
    {
        // TODO: callback for generate CEA message
        auto CEA_message = std::make_shared<message::Message>();
        m_responder->send_message(CEA_message);
    }

    // If necessary, the connection is shut down, and any local resources are freed.
    // Only when States::WAIT_CONN_ACK recv Events::I_RCV_CONN_NACK
    void cleanup(FsmUserDataType&& /*ud*/)
    {
        if (m_connector) {
            m_connector->stop();
            m_connector.reset();
        }
    }

    // The transport layer connection is disconnected, either politely or abortively, in response
    // to an error condition. Local resources are freed.
    void error(FsmUserDataType&& /*ud*/)
    {
        if (m_initiator) {
            m_initiator->stop();
            m_initiator.reset();
        }
        if (m_responder) {
            m_responder->stop();
            m_responder.reset();
        }
    }

    // An election occurs (see Section 5.6.4 for more information).
    //
    // The responder compares the Origin-Host received in the CER
    // with its own Origin-Host as two streams of octets.
    // If the local Origin-Host lexicographically succeeds the received Origin-Host,
    // a Win-Election event is issued locally.
    void elect(const std::string& local_origin_host, const std::string& received_origin_host)
    {
        //true if the Received Origin Host range is lexicographically less than the Local Origin Host, otherwise false.
        bool is_win = std::lexicographical_compare(
            received_origin_host.begin(), received_origin_host.end(),
            local_origin_host.begin(), local_origin_host.end(),
            [](unsigned char a, unsigned char b) {
                return std::tolower(a) < std::tolower(b);
            }
        );

        if (is_win) {
            // TODO: Important! This call should be after the end off prev action
            m_fsm.process_event(Events::WIN_ELECTION);
        }
    }

    // The transport layer connection is disconnected, and local resources are freed.
    void initiator_disconnect(FsmUserDataType&& ud)
    {
        if (m_initiator) {
            m_initiator->stop();
            m_initiator.reset();
        }
        if (m_fsm.state() == States::WAIT_RETURNS) {
            responder_send_CEA(nullptr);
        }
    }
    void responder_disconnect(FsmUserDataType&& ud)
    {
        if (m_responder) {
            m_responder->stop();
            m_responder.reset();
        }
    }

    // A message is to be sent.
    void initiator_send_message(FsmUserDataType&& ud)
    {
        auto message = std::get<MessagePtr>(std::move(ud));
        m_initiator->send_message(message);
    }
    void responder_send_message(FsmUserDataType&& ud)
    {
        auto message = std::get<MessagePtr>(std::move(ud));
        m_responder->send_message(message);
    }

    // A message is serviced.
    void process_message(FsmUserDataType&& ud)
    {
        auto message = std::get<MessagePtr>(std::move(ud));

        std::shared_lock lock(m_callback_mutex);
        if (m_on_recv_message_cb) {
            m_on_recv_message_cb(std::move(message));
        }
    }

    // The CER associated with the R_Conn_CER is processed.
    void process_CER(FsmUserDataType&& ud)
    {
        auto CER_message = std::get<MessagePtr>(std::move(ud));

        // TODO: Make some checks for CER and after send CEA
        PeerInfo peer_info = make_peer_info(CER_message);

        if (m_fsm.state() == States::CLOSED) {
            responder_send_CEA(nullptr);
        }
        else if (m_fsm.state() == States::WAIT_CEA) {
            elect(m_local_host, m_remote_host);
        }
    }

    // A received CEA is processed.
    void process_CEA(FsmUserDataType&& ud)
    {
        auto CEA_message = std::get<MessagePtr>(std::move(ud));

        PeerInfo peer_info = make_peer_info(CEA_message);
    }

    // The DWR/DWA message is serviced.
    void process_DWR(FsmUserDataType&& ud)
    {
        auto DWR_message = std::get<MessagePtr>(std::move(ud));

        std::shared_lock lock(m_callback_mutex);
        MessagePtr DWA_message;
        if (m_on_recv_DWR_cb) {
            DWA_message = m_on_recv_DWR_cb(std::move(DWR_message));
        }

        if (m_fsm.state() == States::IOPEN) {
            initiator_send_DWA(DWA_message);
        }
        // States::ROPEN
        else {
            responder_send_DWA(DWA_message);
        }
    }

    void process_DWA(FsmUserDataType&& ud)
    {
        auto DWA_message = std::get<MessagePtr>(std::move(ud));

        std::shared_lock lock(m_callback_mutex);
        if (m_on_recv_DWA_cb) {
            m_on_recv_DWA_cb(std::move(DWA_message));
        }
    }

    // A DWR/DWA message is sent.
    void initiator_send_DWR(FsmUserDataType&& ud)
    {
    }

    void initiator_send_DWA(FsmUserDataType&& ud);
    void responder_send_DWR(FsmUserDataType&& ud);
    void responder_send_DWA(FsmUserDataType&& ud);

    // A DPR/DPA message is sent to the peer.
    void initiator_send_DPR(FsmUserDataType&& ud);
    void initiator_send_DPA(FsmUserDataType&& ud);
    void responder_send_DPR(FsmUserDataType&& ud);
    void responder_send_DPA(FsmUserDataType&& ud);

    IdentityType m_local_host;
    IdentityType m_local_realm;
    IdentityType m_remote_host;
    IdentityType m_remote_realm;

    FsmType m_fsm;

    on_send_message_error_t on_send_message_error;

    ConnectorPtr m_connector;
    ConnectionPtr m_initiator;
    ConnectionPtr m_responder;

    std::shared_mutex m_callback_mutex;
    OnStableStateCb m_on_open_state_cb;
    OnStableStateCb m_on_closed_state_cb;
    OnRecvMessageCb m_on_recv_message_cb;

    //m_on_generate_CER_cb;
    //m_on_generate_DWR_cb;
    //m_on_generate_DPR_cb;

    OnRecvCommonMessageCb m_on_recv_CER_cb;
    OnRecvCommonMessageCb m_on_recv_CEA_cb;
    OnRecvCommonMessageCb m_on_recv_DWR_cb;
    OnRecvCommonMessageCb m_on_recv_DWA_cb;
    OnRecvCommonMessageCb m_on_recv_DPR_cb;
    OnRecvCommonMessageCb m_on_recv_DPA_cb;
};

} // namespace diameter::core::peer

#endif
