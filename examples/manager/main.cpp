#include <boost/asio.hpp>

#include <diameter/diameter.h>

int main()
{
    boost::asio::io_context ioc;
    auto manager = diameter::core::manager::Manager(ioc);

    auto config = diameter::core::config::Config{};

    config.acceptors = {
        {"test", {
            "test",
            "local_peer",
            {{"192.168.13.71"}, 3868}
        }}
    };
    config.local_peers = {
        {"local_peer", {{
            "my.origin.host",
            "my.origin.realm",
            1000,
            "PRODUCT"
        }}}
    };
    config.peers = {
        {"BEST_PEER", {
            diameter::core::peer::IPeer::Role::RESPONDER,
            "local_peer",
            {{"192.168.13.71"}, 3868},
            "mme01.mme.epc.mnc000.mcc250.3gppnetwork.org",
            "epc.mnc000.mcc250.3gppnetwork.org",
            {{"192.168.17.67"}, 3880}
        }}
    };

    manager.configure(std::move(config));
    ioc.run();
    return 0;
}