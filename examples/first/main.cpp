//g++ tests/main.cpp -I include/ -I /home/volkov/work/projects/C++/netpacker/include -Wall -Wextra

#include <iostream>
#include <iomanip>
#include <vector>

#include <netpacker/netpacker.h>

#include <diameter/application/base/command.h>
#include <diameter/application/base/id.h>
#include <diameter/message/message.h>
#include <diameter/serial/serial.h>

namespace dm = diameter::message;
namespace dmh = diameter::message::header;
namespace dma = diameter::message::avp;
namespace da = diameter::application;

int main()
{
    dm::Message msg;

    msg.header.version = dmh::ProtocolVersionV::V01;
    msg.header.command_flags.set(dmh::CommandFlag::Request);
    msg.header.command_flags.set(dmh::CommandFlag::Proxiable);
    msg.header.command_code = da::base::CommandV::DeviceWatchdog;
    msg.header.application_id = dmh::ApplicationV::Relay;
    msg.header.hop_by_hop = 1;
    msg.header.end_to_end = 1;

    dma::AVP avp;
    avp.code = 0xaffffffa;
    avp.flags.set(dma::Flag::Mandatory);
    avp.flags.set(dma::Flag::VendorSpecific);
    avp.vendor_id = 25;
    //avp.value = sdma::Value(sdma::Value::RawData{0x01, 0x02});

    msg.avps.emplace_back(avp);

    std::cout << msg.size() << std::endl;
    std::vector<uint8_t> data(msg.size());

    netpacker::put(data.begin(), data.end(), msg);

    for (unsigned int i = 0; i < data.size(); i++) {
        if ((i % 4 == 0) && (i != 0)) {
            std::cout << std::endl;
        }
        std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;

    return 0;
}
