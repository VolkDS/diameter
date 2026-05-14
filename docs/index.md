# Diameter

## Сlass diagram for Core

```plantuml
@startuml
Class PeerInfo {
  std::string origin_host
  std::string origin_realm
  uint32_t vendor_id
  std::string product_name

  inband_security

  std::set<uint32_t> supported_vendor_ids
  std::set<uint32_t> auth_application_ids
  std::set<uint32_t> acct_application_ids
  std::set<std::pair<uint32_t,uint32_t>> vendor_specific_application_ids
}

Class Peer {
  +std::string id
  +std::string name
  +State state

  Connection connect
  PeerInfo local_peer_info
  PeerInfo remote_peer_info

  +Peer()
  +void start(Connection&& conn)
  +void stop()
  +void r_conn_CER(Connection&& conn)
  +void send_message(MessagePtr msg)

  +void set_on_recv_message_cb(handler)
  +void set_on_open_state_cb(handler)
  +void set_on_closed_state_cb(handler)

  -void on_recv_message_from_connection_handler()
}

Class Connection {
  -boost::asio::io_context
  -boost::asio::ip::tcp::socket socket

  +Connection(boost::asio::ip::tcp::socket&&)
  +void send_message()
  +void set_on_recv_message_cb(handler)
  +void set_on_disconnect_cb(handler)
}

Class Acceptor {
  -boost::asio::io_context
  -boost::asio::ip::tcp::acceptor acceptor

  PeerInfo local_peer_info

  void start(handler)
  void stop()
  void set_on_new_connection_cb(handler)
}

Class Connector {
  +std::string id

  -boost::asio::io_context
  -boost::asio::ip::tcp::socket socket

  PeerInfo local_peer_info
  PeerInfo remote_peer_info

  std::string host
  uint16_t port

  void start(handler)
  void stop()
}

Class Manager {
  -Config config
  -AcceptorMap acceptors
  -ConnectorMap connectors
  -IncommingControler inc_controller
  -PeerMap peers

  +Manager(Config&&)

  +Peer* get_peer(std::string name)
  +void configure(Config config)
  +void set_on_recv_message_cb(handler)
  +void set_on_peer_open_state_cb(handler)
  +void set_on_peer_closed_state_cb(handler)

  -void on_new_connection_handler()
  -void on_recv_message_from_peer_handler()
}

Class IncomingController {
  -ConnectionMap connections

  +IncomingController(Config&&)

  +void add_new_connection(Connection&& conn, AcceptorConfig config)
  +void set_on_new_peer_cb(handler)
  +void set_on_disconnect_cb(handler)

  +void stop()

  -PeerInfo process_CER(MessagePtr msg)
  -void on_recv_message_from_connection_handler()
  -void on_disconnect_connection_handler()
}

Peer-->Connection
Manager-->Acceptor
Manager-->Connector
Manager-->Peer
Manager..>Connection
Manager-->IncomingController
Config<-Manager
IncomingController-->Connection
IncomingController..>PeerInfo
@enduml
```

```plantuml
@startuml
cloud INITIAL
cloud OKAY
cloud SUSPECT
cloud DOWN
cloud REOPEN

OKAY --> OKAY: recv_DWA
OKAY --> OKAY: recv_message
SUSPECT --> OKAY: recv_DWA
SUSPECT --> OKAY: recv_message
REOPEN --> OKAY: recv_DWA + numDWA(2)
REOPEN --> REOPEN: recv_DWA + numDWA(<2)
REOPEN --> REOPEN: recv_message
INITIAL --> INITIAL: recv_DWA
INITIAL --> INITIAL: recv_message
DOWN --> DOWN: recv_DWA
DOWN --> DOWN: recv_message
OKAY --> OKAY: timer_expires(pending)
OKAY --> SUSPECT: timer_expires
SUSPECT --> DOWN: timer_expires
DOWN --> DOWN: timer_expires
REOPEN --> REOPEN: timer_expires
REOPEN --> DOWN: timer_expires
INITIAL --> OKAY: connection_open
DOWN --> REOPEN: connection_open
OKAY --> DOWN: connection_close
SUSPECT --> DOWN: connection_close
REOPEN --> DOWN: connection_close
@enduml
```