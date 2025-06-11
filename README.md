# Diameter Protocol Library

Library is an implementation of the Diameter Base Protocol [RFC 6733](http://tools.ietf.org/html/rfc6733) for C++ language

## Requirements

You need to install or clone [Netpacker Library](https://github.com/VolkDS/netpacker) for the project compilation
```
$ mkdir -p externals
$ git clone https://github.com/VolkDS/netpacker externals/netpacker
```

## Examples

Encode the Message
```c++
    #include <diameter/diameter.h>
    #include <netpacker/netpacker.h>

    using namespace diameter::message;

    // Create Diameter Message
    Message m {
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

    // And serialize it
    auto data = std::vector<uint8_t>(m.size());
    netpacker::put(data.begin(), data.end(), m);
```

Decode the message
```c++
    // Receive some data 
    std::string hexed = \
        "010000C080000101000000006AD1D314" \
        "77287404000001084000003274657374" \
        "686F73742E6570632E6D6E633030302E" \
        "6D63633030302E336770706E6574776F" \
        "726B2E6F726700000000012840000029" \
        "6570632E6D6E633030302E6D63633030" \
        "302E336770706E6574776F726B2E6F72" \
        "67000000000001014000000E00017F00" \
        "000100000000010A4000000C000028AF" \
        "0000010D000000164578616D706C6550" \
        "726F647563740000000001024000000C" \
        "00000004000001094000000C000028AF";

    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    // And put it to decoder
    auto pos = data.begin();
    auto msg = netpacker::get<Message>(pos, data.end());
```

### Usage with CMake

If using CMake, you can use ```add_subdirectory``` for incorporate the library
directly in to one's CMake project.
```cmake
add_subdirectory(diameter)
```
And link to the library as follows.
```cmake
target_link_libraries(MyTarget diameter::diameter)
```
