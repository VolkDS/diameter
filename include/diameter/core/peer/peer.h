#ifndef DIAMETER_CORE_PEER_PEER_H
#define DIAMETER_CORE_PEER_PEER_H

#include <string>
#include <variant>

#include <diameter/core/connection/iconnection.h>
#include <diameter/core/fsm.h>
#include <diameter/core/peer/info.h>
#include <diameter/core/peer/ipeer.h>

namespace diameter::core::peer {

class Peer
{
public:
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

    enum class Events: uint32_t
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

    using identity_t = std::string;
    using connection_t = diameter::core::connection::IConnection;
    using connection_ptr = std::shared_ptr<connection_t>;
    using fsm_user_data_t = std::variant<connection_ptr, std::nullptr_t>;
    using fsm_t = diameter::core::StateMachine<States, Events, Peer, fsm_user_data_t&&>;
    using fsm_transition_table_t = fsm_t::transition_table_t;

    using message_t = diameter::message::Message;

    using on_send_message_error_t = std::function<void(message::Message&&)>;

    explicit Peer(const identity_t& local_host, const identity_t& local_realm,
        const identity_t& remove_host, const identity_t& remove_realm)
        : m_local_host(local_host),
          m_local_realm(local_realm),
          m_remote_host(remove_host),
          m_remote_realm(remove_realm),
          m_fsm(this, States::CLOSED, &m_fsm_transition_table)
    {
    }

    Peer(Peer const&) = default;
    Peer& operator= (Peer const&) = default;
    Peer(Peer&&) = default;
    Peer& operator= (Peer&&) = default;

    void start(connection_ptr&& connect)
    {
        m_fsm.process_event(Events::START, std::move(connect));
    }

    void stop()
    {
        m_fsm.process_event(Events::STOP);
    }

    void responder_connection_CER(connection_ptr&& connect)
    {
        m_fsm.process_event(Events::R_CONN_CER, std::move(connect));
    }

    void initiator_recv_connection_ack();
    void initiator_recv_connection_nack();
    void timeout();
    void initiator_recv_CEA();

    void initiator_recv_non_CEA();
    void win_election();
  
    void send_message(message_t&& message)
    {
        bool processed = m_fsm.process_event(Events::SEND_MESSAGE);
        if (!processed) {

        }
    }

    void responder_recv_message();
    void responder_recv_DWR();
    void responder_recv_DWA();
    void responder_recv_DPR();
    void responder_recv_DPA();
    
    void initiator_recv_message();
    void initiator_recv_DWR();
    void initiator_recv_DWA();
    void initiator_recv_DPR();
    void initiator_recv_DPA();

private:
    static const fsm_transition_table_t m_fsm_transition_table;

    // A transport connection is initiated with the peer.
    void initiator_start_connection(fsm_user_data_t&& ud)
    {
        m_initiator = std::get<connection_ptr>(std::move(ud));
        m_initiator->start();
    }

    // The incoming connection associated with the R_Conn_CER is accepted as the responder connection.
    void responder_accept(fsm_user_data_t&& ud)
    {
        m_responder = std::get<connection_ptr>(std::move(ud));
    }

    // The CER associated with the R_Conn_CER is processed.
    void process_CER(fsm_user_data_t&& ud);

    // A CER message is sent to the peer.
    void initiator_send_CER(fsm_user_data_t&& ud);

    // A CEA message is sent to the peer.
    void responder_send_CEA(fsm_user_data_t&& ud);

    // If necessary, the connection is shut down, and any local resources are freed.
    void cleanup(fsm_user_data_t&& ud);

    // The transport layer connection is disconnected, either politely or abortively, in response
    // to an error condition. Local resources are freed.
    void error(fsm_user_data_t&& ud);

    // A received CEA is processed.
    void process_CEA(fsm_user_data_t&& ud);

    // An election occurs (see Section 5.6.4 for more information).
    void elect(fsm_user_data_t&& ud);

    // The transport layer connection is disconnected, and local resources are freed.
    void initiator_disconnect(fsm_user_data_t&& ud);
    void responder_disconnect(fsm_user_data_t&& ud);

    // The incoming connection associated with the R_Conn_CER is disconnected.
    void responder_reject(fsm_user_data_t&& ud)
    {
        auto connect = std::get<connection_ptr>(std::move(ud));
        connect->stop();
    }

    // A message is to be sent.
    void initiator_send_message(fsm_user_data_t&& ud);
    void responder_send_message(fsm_user_data_t&& ud);

    // A message is serviced.
    void process_message(fsm_user_data_t&& ud);

    // The DWR/DWA message is serviced.
    void process_DWR(fsm_user_data_t&& ud);
    void process_DWA(fsm_user_data_t&& ud);

    // A DWR/DWA message is sent.
    void initiator_send_DWR(fsm_user_data_t&& ud);
    void initiator_send_DWA(fsm_user_data_t&& ud);
    void responder_send_DWR(fsm_user_data_t&& ud);
    void responder_send_DWA(fsm_user_data_t&& ud);

    // A DPR/DPA message is sent to the peer.
    void initiator_send_DPR(fsm_user_data_t&& ud);
    void initiator_send_DPA(fsm_user_data_t&& ud);
    void responder_send_DPR(fsm_user_data_t&& ud);
    void responder_send_DPA(fsm_user_data_t&& ud);

    identity_t m_local_host;
    identity_t m_local_realm;
    identity_t m_remote_host;
    identity_t m_remote_realm;

    fsm_t m_fsm;

    on_send_message_error_t on_send_message_error;

    connection_ptr m_initiator;
    connection_ptr m_responder;
};

} // namespace diameter::core::peer

#endif