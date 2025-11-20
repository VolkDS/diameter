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
    using ConnectionPtr = std::shared_ptr<io::Connection>;
    using ConnectorPtr = std::shared_ptr<io::Connector>;
    using MessagePtr = std::shared_ptr<message::Message>;

    using FsmUserDataType = std::variant<ConnectorPtr, ConnectionPtr, MessagePtr, std::nullptr_t>;

    using OnRecvMessageCb = std::function<void(message::Message&&)>;

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
        I_RCV_NON_CEA,
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

    //The stable states that a state machine may be in are Closed, I-Open, and R-Open
    using OnStableStateCb = std::function<void()>;

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
        m_fsm.process_event(Events::START, connector);
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

    void set_on_recv_message_cb()
    {

    }

    void set_on_open_state_cb(OnStableStateCb&& handler)
    {
        m_on_open_state_cb = std::move(handler);
    }

    void set_on_closed_state_cb(OnStableStateCb&& handler)
    {
        m_on_closed_state_cb = std::move(handler);
    }

    // void responder_recv_message();
    // void responder_recv_DWR();
    // void responder_recv_DWA();
    // void responder_recv_DPR();
    // void responder_recv_DPA();

    // void initiator_recv_message();
    // void initiator_recv_DWR();
    // void initiator_recv_DWA();
    // void initiator_recv_DPR();
    // void initiator_recv_DPA();

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

        // TODO: callback for generate CEA message
        auto CEA_message = std::make_shared<message::Message>();
        m_responder->send_message(CEA_message);
    }

    // The CER associated with the R_Conn_CER is processed.
    void process_CER(const MessagePtr& CER_message)
    {
        // TODO: Make some checks for CER and after send CEA

        // TODO: callback for generate CEA message
        auto CEA_message = std::make_shared<message::Message>();
        m_responder->send_message(CEA_message);
    }

    // A CER message is sent to the peer.
    void initiator_send_CER(FsmUserDataType&& ud)
    {
        m_initiator = std::get<ConnectionPtr>(std::move(ud));

        auto self = shared_from_this();
        m_initiator->set_on_disconnect_cb([self](const boost::system::error_code& error) {
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

        // TODO: callback for generate CER message
        auto CER_message = std::make_shared<message::Message>();
        m_initiator->send_message(CER_message);
    }

    // A CEA message is sent to the peer.
    void responder_send_CEA(FsmUserDataType&& ud);

    // If necessary, the connection is shut down, and any local resources are freed.
    // Events::I_RCV_CONN_NACK
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

    // A received CEA is processed.
    void process_CEA(FsmUserDataType&& ud);

    // An election occurs (see Section 5.6.4 for more information).
    void elect(FsmUserDataType&& ud);

    // The transport layer connection is disconnected, and local resources are freed.
    void initiator_disconnect(FsmUserDataType&& ud)
    {
        if (m_initiator) {
            m_initiator->stop();
            m_initiator.reset();
        }
    }
    void responder_disconnect(FsmUserDataType&& ud)
    {
        if (m_responder) {
            m_responder->stop();
            m_responder.reset();
        }
    }

    // The incoming connection associated with the R_Conn_CER is disconnected.
    void responder_reject(FsmUserDataType&& ud)
    {
        auto connect = std::get<ConnectionPtr>(std::move(ud));
        connect->stop();
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
    }

    // The DWR/DWA message is serviced.
    void process_DWR(FsmUserDataType&& ud)
    {
        // <DWR>  ::= < Diameter Header: 280, REQ >
        //            { Origin-Host }
        //            { Origin-Realm }
        //            [ Origin-State-Id ]
        //          * [ AVP ]
        auto DWR_message = std::get<MessagePtr>(std::move(ud));
    }
    void process_DWA(FsmUserDataType&& ud)
    {
        // <DWA>  ::= < Diameter Header: 280 >
        //            { Result-Code }
        //            { Origin-Host }
        //            { Origin-Realm }
        //            [ Error-Message ]
        //            [ Failed-AVP ]
        //            [ Origin-State-Id ]
        //          * [ AVP ]
        auto DWA_message = std::get<MessagePtr>(std::move(ud));
    }

    // A DWR/DWA message is sent.
    void initiator_send_DWR(FsmUserDataType&& ud);
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

    OnStableStateCb m_on_open_state_cb;
    OnStableStateCb m_on_closed_state_cb;
    OnRecvMessageCb m_on_recv_message_cb;
};

} // namespace diameter::core::peer

#endif
