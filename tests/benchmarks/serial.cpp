#include <benchmark/benchmark.h>

#include <netpacker/netpacker.h>

#include <diameter/message/message.h>
#include <diameter/serial/serial.h>

using namespace diameter::message;

static void BM_MessageSerial(benchmark::State& state) {
    for (auto _ : state) {
        Message msg {
            {
                header::ProtocolVersion{header::ProtocolVersionV::V01},
                header::MessageLength{0},
                header::CommandFlags{0x80},
                header::CommandCode{257},
                header::ApplicationId{0},
                header::HopByHopIdentifier{0x6ad1d314},
                header::EndToEndIdentifier{0x77287404}
            },
            std::list<avp::AVP>{
                avp::AVP{264, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::DiameterIdentity("testhost.epc.mnc000.mcc000.3gppnetwork.org")},
                avp::AVP{296, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::DiameterIdentity("epc.mnc000.mcc000.3gppnetwork.org")},
                avp::AVP{257, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Address("127.0.0.1")},
                avp::AVP{266, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Unsigned32(uint32_t{10415})},
                avp::AVP{269, avp::Flags{}, std::nullopt, avp::UTF8String("ExampleProduct")},
                avp::AVP{258, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Unsigned32(uint32_t{4})},
                avp::AVP{265, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Unsigned32(uint32_t{10415})}
            }
        };

        auto data = std::vector<uint8_t>(msg.size());
        benchmark::DoNotOptimize(netpacker::put(data.begin(), data.end(), msg));
    }
}
// Register the function as a benchmark
BENCHMARK(BM_MessageSerial);

BENCHMARK_MAIN();
