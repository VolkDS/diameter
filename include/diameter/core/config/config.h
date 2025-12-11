#ifndef DIAMETER_CORE_CONFIG_CONFIG_H
#define DIAMETER_CORE_CONFIG_CONFIG_H

#include <chrono>
#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <unordered_map>

#include <diameter/core/peer/info.h>
#include <diameter/core/peer/ipeer.h>

namespace diameter::core::config {

// Each link has follow parameters:
// - local Node
// - local Realm
// - local Hostname
// - local Role (client/server)
// - local Protocol (tcp/sctp)
// - local IPs
// - local Port
// ------------
// - remote IPs
// - remote Port
// - remote Protocol (tcp/sctp) - same as local
// - remote Role (client/server)
// - remote Hostname
// - remote Realm
// - remote Node

enum class AddrType : uint32_t
{
    TCP,
    SCTP
};

struct Addr
{
    std::vector<std::string> host;
    uint16_t port = 0;
    AddrType type = AddrType::TCP;

    bool operator== (const Addr& rhs) const noexcept
    {
        return (host == rhs.host && port == rhs.port);
    }

    bool operator!= (const Addr& rhs) const noexcept
    {
        return !operator== (rhs);
    }

    bool empty() const noexcept
    {
        return host.empty();
    }
};

struct LocalPeerConfig
{
    peer::PeerInfo info;
};

struct PeerConfig
{
    peer::IPeer::Role role = peer::IPeer::Role::INITIATOR;

    // Local side
    std::string local_peer_name;
    Addr local_addr;

    // Remote side
    std::string remote_host;
    std::string remote_realm;
    Addr remote_addr;

    // Security
    bool use_tls = false;

    // Timers
    peer::IPeer::Duration reconnect_timeout = std::chrono::seconds {10};
    peer::IPeer::Duration watchdog_timeout = std::chrono::seconds {30};
    peer::IPeer::Duration request_timeout = std::chrono::seconds {1};
};

struct AcceptorConfig
{
    std::string name;

    // Local side
    std::string local_peer_name;
    Addr local_addr;

    // Security
    bool use_tls = false;

    // Timers
    peer::IPeer::Duration capability_timeout = std::chrono::seconds {5};

    // Options
    bool allow_dynamic_peers = false;
};

struct Config
{
    using Acceptors = std::unordered_map<std::string, AcceptorConfig>;
    using LocalPeers = std::unordered_map<std::string, LocalPeerConfig>;
    using Peers = std::unordered_map<std::string, PeerConfig>;

    bool empty() const noexcept
    {
        return (acceptors.empty() && local_peers.empty() && peers.empty());
    }

    Acceptors acceptors;
    LocalPeers local_peers;
    Peers peers;
};

} // namespace diameter::core::config

#endif
