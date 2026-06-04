#ifndef DIAMETER_CORE_PEER_PEER_H
#define DIAMETER_CORE_PEER_PEER_H

#include <memory>
#include <mutex>
#include <string>
#include <variant>

#include <diameter/application/common/common.h>
#include <diameter/core/fsm.h>
#include <diameter/core/io/connection.h>
#include <diameter/core/io/connector.h>
#include <diameter/core/peer/info.h>
#include <diameter/core/peer/ipeer.h>
#include <diameter/log/log.h>
#include <diameter/message/message.h>

namespace diameter::core::peer {

namespace detail {

inline std::string make_full_peer_identity(const std::string& local_host,
    const std::string& local_realm, const std::string& remote_host, const std::string& remote_realm)
{
    return local_host + "_" + local_realm + "_" + remote_host + "_" + remote_realm;
}

inline std::string make_full_peer_identity(const PeerInfo& local_peer_info,
    const PeerInfo& remote_peer_info)
{
    return make_full_peer_identity(local_peer_info.origin_host, local_peer_info.origin_realm,
        remote_peer_info.origin_host, remote_peer_info.origin_realm);
}

} // namespace detail

class Peer : public std::enable_shared_from_this<Peer>
{
public:
    using SelfPtr = std::shared_ptr<Peer>;
    using SelfWPtr = std::weak_ptr<Peer>;
    using ConnectionPtr = std::shared_ptr<io::Connection>;
    using ConnectorPtr = std::shared_ptr<io::Connector>;
    using MessagePtr = std::shared_ptr<message::Message>;

    struct IncomingData
    {
        ConnectionPtr connection;
        MessagePtr CER_message;
        PeerInfo remote_peer_info;
    };

    struct Callbacks
    {
        using OnRecvMessageCb = std::function<void(MessagePtr&&)>;
        using OnRecvCommonRequestCb = std::function<MessagePtr(const MessagePtr&)>;
        using OnRecvCommonAnswerCb = std::function<bool(const MessagePtr&)>;
        using OnGenerateMessageCb = std::function<MessagePtr()>;
        // The stable states that a state machine may be in are Closed, I-Open, and R-Open
        using OnStableStateCb = std::function<void()>;

        OnStableStateCb on_open_state_cb;
        OnStableStateCb on_closed_state_cb;
        OnRecvMessageCb on_recv_message_cb;
        OnGenerateMessageCb on_generate_CER_cb;
        OnGenerateMessageCb on_generate_DWR_cb;
        OnGenerateMessageCb on_generate_DPR_cb;
        OnRecvCommonRequestCb on_recv_CER_cb;
        OnRecvCommonAnswerCb on_recv_CEA_cb;
        OnRecvCommonRequestCb on_recv_DWR_cb;
        OnRecvCommonAnswerCb on_recv_DWA_cb;
        OnRecvCommonRequestCb on_recv_DPR_cb;
        OnRecvCommonAnswerCb on_recv_DPA_cb;
    };

    using FsmUserDataType
        = std::variant<ConnectorPtr, ConnectionPtr, IncomingData, MessagePtr, std::nullptr_t>;

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

    friend std::ostream& operator<< (std::ostream& os, const States& state)
    {
        switch (state) {
            case States::CLOSED:
                return os << "CLOSED";
            case States::WAIT_CONN_ACK:
                return os << "WAIT_CONN_ACK";
            case States::WAIT_CEA:
                return os << "WAIT_CEA";
            case States::ELECT:
                return os << "ELECT";
            case States::WAIT_RETURNS:
                return os << "WAIT_RETURNS";
            case States::ROPEN:
                return os << "ROPEN";
            case States::IOPEN:
                return os << "IOPEN";
            case States::CLOSING:
                return os << "CLOSING";
            default:
                return os << "UNKNOWN_STATE(" << static_cast<uint32_t>(state) << ")";
        }
    }

    friend std::ostream& operator<< (std::ostream& os, const Events& event)
    {
        switch (event) {
            case Events::START:
                return os << "START";
            case Events::R_CONN_CER:
                return os << "R_CONN_CER";
            case Events::I_RCV_CONN_ACK:
                return os << "I_RCV_CONN_ACK";
            case Events::I_RCV_CONN_NACK:
                return os << "I_RCV_CONN_NACK";
            case Events::TIMEOUT:
                return os << "TIMEOUT";
            case Events::I_RCV_CEA:
                return os << "I_RCV_CEA";
            case Events::I_PEER_DISC:
                return os << "I_PEER_DISC";
            case Events::R_PEER_DISC:
                return os << "R_PEER_DISC";
            case Events::WIN_ELECTION:
                return os << "WIN_ELECTION";
            case Events::SEND_MESSAGE:
                return os << "SEND_MESSAGE";
            case Events::R_RCV_MESSAGE:
                return os << "R_RCV_MESSAGE";
            case Events::R_RCV_DWR:
                return os << "R_RCV_DWR";
            case Events::R_RCV_DWA:
                return os << "R_RCV_DWA";
            case Events::STOP:
                return os << "STOP";
            case Events::R_RCV_DPR:
                return os << "R_RCV_DPR";
            case Events::I_RCV_MESSAGE:
                return os << "I_RCV_MESSAGE";
            case Events::I_RCV_DWR:
                return os << "I_RCV_DWR";
            case Events::I_RCV_DWA:
                return os << "I_RCV_DWA";
            case Events::I_RCV_DPR:
                return os << "I_RCV_DPR";
            case Events::I_RCV_DPA:
                return os << "I_RCV_DPA";
            case Events::R_RCV_DPA:
                return os << "R_RCV_DPA";
            default:
                return os << "UNKNOWN_EVENT(" << static_cast<uint32_t>(event) << ")";
        }
    }

    using FsmType = diameter::core::StateMachine<States, Events, Peer, FsmUserDataType&&>;
    using FsmTransitionTableType = FsmType::transition_table_t;
    using FsmActionTableType = FsmType::action_table_t;

    using IdentityType = std::string;

    template<typename... Args>
    static SelfPtr create(Args&&... args)
    {
        return SelfPtr(new Peer(std::forward<Args>(args)...));
    }

    Peer(Peer const&) = delete;
    Peer& operator= (Peer const&) = delete;
    Peer(Peer&&) = delete;
    Peer& operator= (Peer&&) = delete;

    IdentityType full_id() const
    {
        return m_full_id;
    }

    std::string name() const
    {
        return m_name;
    }

    std::vector<std::string> initiator_local_address() const
    {
        return m_initiator->local_address();
    }

    uint16_t initiator_local_port() const
    {
        return m_initiator->local_port();
    }

    std::vector<std::string> responder_local_address() const
    {
        return m_responder->local_address();
    }

    uint16_t responder_local_port() const
    {
        return m_responder->local_port();
    }

    void start(const ConnectorPtr& connector)
    {
        process_fsm_event(Events::START, std::move(connector));
    }

    void stop()
    {
        process_fsm_event(Events::STOP);
    }

    void responder_connection_CER(IncomingData&& incoming_data)
    {
        process_fsm_event(Events::R_CONN_CER, std::move(incoming_data));
    }

    void timeout()
    {
        process_fsm_event(Events::TIMEOUT);
    }

    void send_message(const MessagePtr& message)
    {
        bool processed = process_fsm_event(Events::SEND_MESSAGE, std::move(message));
        if (!processed) {
            // TODO !processed
        }
    }

    void set_on_recv_message_cb(Callbacks::OnRecvMessageCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_recv_message_cb = std::move(handler);
    }

    void set_on_recv_CER_cb(Callbacks::OnRecvCommonRequestCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_recv_CER_cb = std::move(handler);
    }

    void set_on_recv_CEA_cb(Callbacks::OnRecvCommonAnswerCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_recv_CEA_cb = std::move(handler);
    }

    void set_on_recv_DWR_cb(Callbacks::OnRecvCommonRequestCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_recv_DWR_cb = std::move(handler);
    }

    void set_on_recv_DWA_cb(Callbacks::OnRecvCommonAnswerCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_recv_DWA_cb = std::move(handler);
    }

    void set_on_recv_DPR_cb(Callbacks::OnRecvCommonRequestCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_recv_DPR_cb = std::move(handler);
    }

    void set_on_recv_DPA_cb(Callbacks::OnRecvCommonAnswerCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_recv_DPA_cb = std::move(handler);
    }

    void set_on_open_state_cb(Callbacks::OnStableStateCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_open_state_cb = std::move(handler);
    }

    void set_on_closed_state_cb(Callbacks::OnStableStateCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_closed_state_cb = std::move(handler);
    }

    void set_on_generate_CER_cb(Callbacks::OnGenerateMessageCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_generate_CER_cb = std::move(handler);
    }

    void set_on_generate_DWR_cb(Callbacks::OnGenerateMessageCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_generate_DWR_cb = std::move(handler);
    }

    void set_on_generate_DPR_cb(Callbacks::OnGenerateMessageCb&& handler)
    {
        std::unique_lock lock(m_callback_mutex);
        m_callbacks.on_generate_DPR_cb = std::move(handler);
    }

private:
    Peer(const std::string& name, const IdentityType& local_host, const IdentityType& local_realm,
        const IdentityType& remote_host, const IdentityType& remote_realm)
        : m_name(name),
          m_local_host(local_host),
          m_local_realm(local_realm),
          m_remote_host(remote_host),
          m_remote_realm(remote_realm),
          m_fsm(this, States::CLOSED, &m_fsm_transition_table, &m_fsm_enter_table)
    {
        m_full_id = detail::make_full_peer_identity(m_local_host, m_local_realm, m_remote_host,
            m_remote_realm);
    }

    bool process_fsm_event(const Events& event)
    {
        return process_fsm_event(event, nullptr);
    }

    bool process_fsm_event(const Events& event, FsmUserDataType&& ud)
    {
        // Only one event at one moment
        // If it will be slow...
        std::lock_guard<std::mutex> lock(m_fsm_mutex);
        DIAMETER_LOG_DEBUG("[peer=" << m_name << "] Event " << event << " in state "
                                    << m_fsm.state());
        bool processed = m_fsm.process_event(event, std::forward<FsmUserDataType>(ud));
        DIAMETER_LOG_DEBUG("[peer=" << m_name << "] New state " << m_fsm.state());
        return processed;
    }

    // A transport connection is initiated with the peer.
    void initiator_start_connection(FsmUserDataType&& ud)
    {
        m_connector = std::get<ConnectorPtr>(std::move(ud));
        auto self = shared_from_this();
        m_connector->set_on_connect_cb([self](const boost::system::error_code& error,
                                           io::Connector::SocketType&& socket) {
            if (error) {
                self->process_fsm_event(Events::I_RCV_CONN_NACK);
                return;
            }
            auto connection = io::Connection::create(std::move(socket));
            self->process_fsm_event(Events::I_RCV_CONN_ACK, std::move(connection));
        });
        m_connector->run();
    }

    // The incoming connection associated with the R_Conn_CER is accepted as the responder
    // connection.
    void responder_accept(FsmUserDataType&& ud)
    {
        auto incoming_data = std::get<IncomingData>(ud);
        m_responder = std::move(incoming_data.connection);
        m_responder_CER_message = std::move(incoming_data.CER_message);
        m_responder_peer_info = std::move(incoming_data.remote_peer_info);

        auto self = shared_from_this();
        m_responder->set_on_disconnect_cb([self](const boost::system::error_code& /*error*/) {
            // DIAMETER_LOG_ERROR("[peer="<< self->m_name <<"] responder on_disconnect: " << error
            // << " (" << error.message() << ")");
            self->process_fsm_event(Events::R_PEER_DISC);
        });
        m_responder->set_on_recv_message_cb([self](MessagePtr&& message) {
            if (message->header.application_id == message::header::ApplicationV::Common) {
                if (message->header.command_flags[message::header::CommandFlag::Request]) {
                    switch (message->header.command_code) {
                        case application::common::CommandV::DeviceWatchdog:
                            self->process_fsm_event(Events::R_RCV_DWR, std::move(message));
                            break;
                        case application::common::CommandV::DisconnectPeer:
                            self->process_fsm_event(Events::R_RCV_DPR, std::move(message));
                            break;
                        default:
                            break;
                    }
                }
                else {
                    switch (message->header.command_code) {
                        case application::common::CommandV::DeviceWatchdog:
                            {
                                std::shared_lock lock(self->m_callback_mutex);
                                if (self->m_callbacks.on_recv_DWA_cb) {
                                    if (self->m_callbacks.on_recv_DWA_cb(message)) {
                                        self->process_fsm_event(Events::R_RCV_DWA, std::move(message));
                                    }
                                }
                                break;
                            }
                        case application::common::CommandV::DisconnectPeer:
                            {
                                std::shared_lock lock(self->m_callback_mutex);
                                if (self->m_callbacks.on_recv_DPA_cb) {
                                    if (self->m_callbacks.on_recv_DPA_cb(message)) {
                                        self->process_fsm_event(Events::R_RCV_DPA, std::move(message));
                                    }
                                }
                                break;
                            }
                        default:
                            break;
                    }
                }
                return;
            }
            self->process_fsm_event(Events::R_RCV_MESSAGE, std::move(message));
        });

        process_CER();
    }

    // The incoming connection associated with the R_Conn_CER is disconnected.
    void responder_reject(FsmUserDataType&& ud)
    {
        auto incoming_data = std::get<IncomingData>(std::move(ud));
        incoming_data.connection->stop();
    }

    // A CER message is sent to the peer.
    // Events::I_RCV_CONN_ACK
    void initiator_apply(FsmUserDataType&& ud)
    {
        m_initiator = std::get<ConnectionPtr>(std::move(ud));

        auto self = shared_from_this();
        m_initiator->set_on_disconnect_cb([self](const boost::system::error_code& /*error*/) {
            // DIAMETER_LOG_ERROR("[peer="<< self->m_name <<"] initiator on_disconnect: " << error
            // << " (" << error.message() << ")");
            self->process_fsm_event(Events::I_PEER_DISC);
        });
        m_initiator->set_on_recv_message_cb([self](MessagePtr&& message) {
            if (message->header.application_id == message::header::ApplicationV::Common) {
                if (message->header.command_flags[message::header::CommandFlag::Request]) {
                    switch (message->header.command_code) {
                        case application::common::CommandV::DeviceWatchdog:
                            self->process_fsm_event(Events::I_RCV_DWR, std::move(message));
                            break;
                        case application::common::CommandV::DisconnectPeer:
                            self->process_fsm_event(Events::I_RCV_DPR, std::move(message));
                            break;
                        default:
                            break;
                    }
                }
                else {
                    switch (message->header.command_code) {
                        case application::common::CommandV::CapabilitiesExchange:
                            {
                                std::shared_lock lock(self->m_callback_mutex);
                                if (self->m_callbacks.on_recv_CEA_cb) {
                                    if (self->m_callbacks.on_recv_CEA_cb(message)) {
                                        self->process_fsm_event(Events::I_RCV_CEA, std::move(message));
                                    }
                                }
                                break;
                            }
                        case application::common::CommandV::DeviceWatchdog:
                            {
                                std::shared_lock lock(self->m_callback_mutex);
                                if (self->m_callbacks.on_recv_DWA_cb) {
                                    if (self->m_callbacks.on_recv_DWA_cb(message)) {
                                        self->process_fsm_event(Events::I_RCV_DWA, std::move(message));
                                    }
                                }
                                break;
                            }
                        case application::common::CommandV::DisconnectPeer:
                            {
                                std::shared_lock lock(self->m_callback_mutex);
                                if (self->m_callbacks.on_recv_DWA_cb) {
                                    if (self->m_callbacks.on_recv_DWA_cb(message)) {
                                        self->process_fsm_event(Events::I_RCV_DPA, std::move(message));
                                    }
                                }
                                break;
                            }
                        default:
                            break;
                    }
                }
                return;
            }
            self->process_fsm_event(Events::I_RCV_MESSAGE, std::move(message));
        });
        m_initiator->run();

        initiator_send_CER(nullptr);
    }

    void initiator_send_CER(FsmUserDataType&& /*ud*/)
    {
        MessagePtr CER_message;
        {
            std::shared_lock lock(m_callback_mutex);
            if (m_callbacks.on_generate_CER_cb) {
                CER_message = m_callbacks.on_generate_CER_cb();
            }
        }
        m_initiator->send_message(CER_message);

        if (m_fsm.state() == States::ELECT) {
            elect(m_local_host, m_remote_host);
        }
    }

    // A CEA message is sent to the peer.
    void responder_send_CEA(FsmUserDataType&& /*ud*/)
    {
        MessagePtr CEA_message;
        {
            std::shared_lock lock(m_callback_mutex);
            if (m_callbacks.on_recv_CER_cb) {
                CEA_message = m_callbacks.on_recv_CER_cb(m_responder_CER_message);
            }
        }
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
        // true if the Received Origin Host range is lexicographically less than the Local Origin
        // Host, otherwise false.
        bool is_win = std::lexicographical_compare(received_origin_host.begin(),
            received_origin_host.end(), local_origin_host.begin(), local_origin_host.end(),
            [](unsigned char a, unsigned char b) {
                return std::tolower(a) < std::tolower(b);
            });

        if (is_win) {
            // TODO: Important! This call should be after the end off prev action
            process_fsm_event(Events::WIN_ELECTION);
        }
    }

    // The transport layer connection is disconnected, and local resources are freed.
    void initiator_disconnect(FsmUserDataType&& /*ud*/)
    {
        if (m_initiator) {
            m_initiator->stop();
            m_initiator.reset();
        }
        if (m_fsm.state() == States::WAIT_RETURNS) {
            responder_send_CEA(nullptr);
        }
    }
    void responder_disconnect(FsmUserDataType&& /*ud*/)
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
        if (m_callbacks.on_recv_message_cb) {
            m_callbacks.on_recv_message_cb(std::move(message));
        }
    }

    // The CER associated with the R_Conn_CER is processed.
    void process_CER()
    {
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

        // TODO: Maybe save it?
        PeerInfo peer_info = make_peer_info(CEA_message);
    }

    // The DWR/DWA message is serviced.
    void process_DWR(FsmUserDataType&& ud)
    {
        auto DWR_message = std::get<MessagePtr>(std::move(ud));

        MessagePtr DWA_message;
        {
            std::shared_lock lock(m_callback_mutex);
            if (m_callbacks.on_recv_DWR_cb) {
                DWA_message = m_callbacks.on_recv_DWR_cb(std::move(DWR_message));
            }
            else {
                auto DWA_builder = application::common::DeviceWatchdogBuilder();
                DWA_message = DWA_builder
                    .set_hop_by_hop(DWR_message->header.hop_by_hop)
                    .set_end_to_end(DWR_message->header.end_to_end)
                    .add_result_code(application::common::ResultCodeV::SUCCESS)
                    .add_origin_host(m_local_host)
                    .add_origin_realm(m_local_realm)
                    .build();
            }
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
        if (m_callbacks.on_recv_DWA_cb) {
            m_callbacks.on_recv_DWA_cb(std::move(DWA_message));
        }
    }

    void process_DPR(FsmUserDataType&& ud)
    {
        auto DPR_message = std::get<MessagePtr>(std::move(ud));

        MessagePtr DPA_message;
        {
            std::shared_lock lock(m_callback_mutex);
            if (m_callbacks.on_recv_DWR_cb) {
                DPA_message = m_callbacks.on_recv_DWR_cb(std::move(DPR_message));
            }
            else {
                auto DPA_builder = application::common::DisconnectPeerBuilder();
                DPA_message = DPA_builder
                    .set_hop_by_hop(DPR_message->header.hop_by_hop)
                    .set_end_to_end(DPR_message->header.end_to_end)
                    .add_result_code(application::common::ResultCodeV::SUCCESS)
                    .add_origin_host(m_local_host)
                    .add_origin_realm(m_local_realm)
                    .build();
            }
        }

        if (m_fsm.state() == States::IOPEN) {
            initiator_send_DPA(DPA_message);
        }
        // States::ROPEN
        else {
            responder_send_DPA(DPA_message);
        }
    }

    // A DWR/DWA message is sent.
    void initiator_send_DWR(FsmUserDataType&& ud)
    {
        initiator_send_message(std::forward<FsmUserDataType>(ud));
    }

    void initiator_send_DWA(FsmUserDataType&& ud)
    {
        initiator_send_message(std::forward<FsmUserDataType>(ud));
    }

    void responder_send_DWR(FsmUserDataType&& ud)
    {
        responder_send_message(std::forward<FsmUserDataType>(ud));
    }
    void responder_send_DWA(FsmUserDataType&& ud)
    {
        responder_send_message(std::forward<FsmUserDataType>(ud));
    }

    // A DPR/DPA message is sent to the peer.
    void initiator_send_DPR(FsmUserDataType&& /*ud*/)
    {
        MessagePtr DPR_message;
        {
            std::shared_lock lock(m_callback_mutex);
            if (m_callbacks.on_generate_DPR_cb) {
                DPR_message = m_callbacks.on_generate_DPR_cb();
            }
        }
        m_initiator->send_message(DPR_message);
    }
    void initiator_send_DPA(FsmUserDataType&& ud)
    {
        initiator_send_message(std::forward<FsmUserDataType>(ud));
    }

    void responder_send_DPR(FsmUserDataType&& /*ud*/)
    {
        MessagePtr DPR_message;
        {
            std::shared_lock lock(m_callback_mutex);
            if (m_callbacks.on_generate_CER_cb) {
                DPR_message = m_callbacks.on_generate_CER_cb();
            }
        }
        m_responder->send_message(DPR_message);
    }
    void responder_send_DPA(FsmUserDataType&& ud)
    {
        responder_send_message(std::forward<FsmUserDataType>(ud));
    }

    void enter_open_state(FsmUserDataType&& /*ud*/)
    {
        std::shared_lock lock(m_callback_mutex);
        if (m_callbacks.on_open_state_cb) {
            m_callbacks.on_open_state_cb();
        }
    }

    void enter_closed_state(FsmUserDataType&& /*ud*/)
    {
        std::shared_lock lock(m_callback_mutex);
        if (m_callbacks.on_closed_state_cb) {
            m_callbacks.on_closed_state_cb();
        }
    }

    static const FsmTransitionTableType m_fsm_transition_table;
    static const FsmActionTableType m_fsm_enter_table;

    std::string m_name;
    IdentityType m_local_host;
    IdentityType m_local_realm;
    IdentityType m_remote_host;
    IdentityType m_remote_realm;
    IdentityType m_full_id;

    std::mutex m_fsm_mutex;
    FsmType m_fsm;

    ConnectorPtr m_connector;
    ConnectionPtr m_initiator;
    ConnectionPtr m_responder;

    MessagePtr m_responder_CER_message;
    PeerInfo m_responder_peer_info;

    std::shared_mutex m_callback_mutex;
    Callbacks m_callbacks;
};

} // namespace diameter::core::peer

#endif
