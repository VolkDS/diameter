#include <diameter/core/peer/peer.h>

namespace diameter::core::peer {

const Peer::FsmActionTableType Peer::m_fsm_enter_table{
    {States::IOPEN,  &Peer::enter_open_state  },
    {States::ROPEN,  &Peer::enter_open_state  },
    {States::CLOSED, &Peer::enter_closed_state},
};

// clang-format off
const Peer::FsmTransitionTableType Peer::m_fsm_transition_table {
    {{States::CLOSED, Events::START},      {States::WAIT_CONN_ACK, &Peer::initiator_start_connection}},
    {{States::CLOSED, Events::R_CONN_CER}, {States::ROPEN,         &Peer::responder_accept}},

    {{States::WAIT_CONN_ACK, Events::I_RCV_CONN_ACK},  {States::WAIT_CEA, &Peer::initiator_apply}},
    {{States::WAIT_CONN_ACK, Events::I_RCV_CONN_NACK}, {States::CLOSED,   &Peer::cleanup}},
    {{States::WAIT_CONN_ACK, Events::R_CONN_CER},      {States::ELECT,    &Peer::responder_accept}},
    {{States::WAIT_CONN_ACK, Events::TIMEOUT},         {States::CLOSED,   &Peer::error}},

    {{States::WAIT_CEA, Events::I_RCV_CEA},     {States::IOPEN,        &Peer::process_CEA}},
    {{States::WAIT_CEA, Events::R_CONN_CER},    {States::WAIT_RETURNS, &Peer::responder_accept}},
    {{States::WAIT_CEA, Events::I_PEER_DISC},   {States::CLOSED,       &Peer::initiator_disconnect}},
    {{States::WAIT_CEA, Events::I_RCV_DPA},     {States::CLOSED,       &Peer::error}},
    {{States::WAIT_CEA, Events::I_RCV_DPR},     {States::CLOSED,       &Peer::error}},
    {{States::WAIT_CEA, Events::I_RCV_DWA},     {States::CLOSED,       &Peer::error}},
    {{States::WAIT_CEA, Events::I_RCV_DWR},     {States::CLOSED,       &Peer::error}},
    {{States::WAIT_CEA, Events::I_RCV_MESSAGE}, {States::CLOSED,       &Peer::error}},
    {{States::WAIT_CEA, Events::TIMEOUT},       {States::CLOSED,       &Peer::error}},

    {{States::ELECT, Events::I_RCV_CONN_ACK},  {States::WAIT_RETURNS,  &Peer::initiator_apply}},
    {{States::ELECT, Events::I_RCV_CONN_NACK}, {States::ROPEN,         &Peer::responder_send_CEA}},
    {{States::ELECT, Events::R_PEER_DISC},     {States::WAIT_CONN_ACK, &Peer::responder_disconnect}},
    {{States::ELECT, Events::R_CONN_CER},      {States::ELECT,         &Peer::responder_reject}},
    {{States::ELECT, Events::TIMEOUT},         {States::CLOSED,        &Peer::error}},

    {{States::WAIT_RETURNS, Events::WIN_ELECTION}, {States::ROPEN,        &Peer::initiator_disconnect}},
    {{States::WAIT_RETURNS, Events::I_PEER_DISC},  {States::ROPEN,        &Peer::initiator_disconnect}},
    {{States::WAIT_RETURNS, Events::I_RCV_CEA},    {States::IOPEN,        &Peer::responder_disconnect}},
    {{States::WAIT_RETURNS, Events::R_PEER_DISC},  {States::WAIT_CEA,     &Peer::responder_disconnect}},
    {{States::WAIT_RETURNS, Events::R_CONN_CER},   {States::WAIT_RETURNS, &Peer::responder_reject}},
    {{States::WAIT_RETURNS, Events::TIMEOUT},      {States::CLOSED,       &Peer::error}},

    {{States::ROPEN, Events::SEND_MESSAGE},  {States::ROPEN,   &Peer::responder_send_message}},
    {{States::ROPEN, Events::R_RCV_MESSAGE}, {States::ROPEN,   &Peer::process_message}},
    {{States::ROPEN, Events::R_RCV_DWR},     {States::ROPEN,   &Peer::process_DWR}},
    {{States::ROPEN, Events::R_RCV_DWA},     {States::ROPEN,   &Peer::process_DWA}},
    {{States::ROPEN, Events::R_CONN_CER},    {States::ROPEN,   &Peer::responder_reject}},
    {{States::ROPEN, Events::STOP},          {States::CLOSING, &Peer::responder_send_DPR}},
    {{States::ROPEN, Events::R_RCV_DPR},     {States::CLOSING, &Peer::process_DPR}},
    {{States::ROPEN, Events::R_PEER_DISC},   {States::CLOSED,  &Peer::responder_disconnect}},

    {{States::IOPEN, Events::SEND_MESSAGE},  {States::IOPEN,   &Peer::initiator_send_message}},
    {{States::IOPEN, Events::I_RCV_MESSAGE}, {States::IOPEN,   &Peer::process_message}},
    {{States::IOPEN, Events::I_RCV_DWR},     {States::IOPEN,   &Peer::process_DWR}},
    {{States::IOPEN, Events::I_RCV_DWA},     {States::IOPEN,   &Peer::process_DWA}},
    {{States::IOPEN, Events::R_CONN_CER},    {States::IOPEN,   &Peer::responder_reject}},
    {{States::IOPEN, Events::STOP},          {States::CLOSING, &Peer::initiator_send_DPR}},
    {{States::IOPEN, Events::I_RCV_DPR},     {States::CLOSING, &Peer::process_DPR}},
    {{States::IOPEN, Events::I_PEER_DISC},   {States::CLOSED,  &Peer::initiator_disconnect}},

    {{States::CLOSING, Events::I_RCV_DPA},   {States::CLOSED,  &Peer::initiator_disconnect}},
    {{States::CLOSING, Events::R_RCV_DPA},   {States::CLOSED,  &Peer::responder_disconnect}},
    {{States::CLOSING, Events::TIMEOUT},     {States::CLOSED,  &Peer::error}},
    {{States::CLOSING, Events::I_PEER_DISC}, {States::CLOSED,  &Peer::initiator_disconnect}},
    {{States::CLOSING, Events::R_PEER_DISC}, {States::CLOSED,  &Peer::responder_disconnect}},
};
// clang-format on
}
