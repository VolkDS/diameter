#ifndef DIAMETER_APPLICATION_COMMON_MESSAGE_BUILDER_H
#define DIAMETER_APPLICATION_COMMON_MESSAGE_BUILDER_H

#include <memory>

#include <diameter/application/common/avp.h>
#include <diameter/application/common/command.h>
#include <diameter/message/message.h>

namespace diameter::application::common {

//  <answer-message> ::= < Diameter Header: code, ERR [, PXY] >
//                    0*1< Session-Id >
//                       { Origin-Host }
//                       { Origin-Realm }
//                       { Result-Code }
//                       [ Origin-State-Id ]
//                       [ Error-Message ]
//                       [ Error-Reporting-Host ]
//                       [ Failed-AVP ]
//                       [ Experimental-Result ]
//                     * [ Proxy-Info ]
//                     * [ AVP ]

template<class T>
class MessageBuilder
{
public:
    MessageBuilder()
        : message(std::make_shared<message::Message>())
    {
        message->header.version = message::header::ProtocolVersionV::V01;
        message->header.application_id = message::header::ApplicationV::Common;
        message->header.command_flags.set(message::header::CommandFlag::Request);
    }

    T& set_hop_by_hop(message::header::HopByHopIdentifier hop_by_hop)
    {
        message->header.hop_by_hop = hop_by_hop;
        return static_cast<T&>(*this);
    }

    T& set_end_to_end(message::header::EndToEndIdentifier end_to_end)
    {
        message->header.end_to_end = end_to_end;
        return static_cast<T&>(*this);
    }

    T& add_result_code(uint32_t value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::ResultCode},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Unsigned32(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        message->header.command_flags.reset(message::header::CommandFlag::Request);
        return static_cast<T&>(*this);
    }

    T& add_origin_host(const std::string& value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::OriginHost},
            message::avp::Flags{},
            std::nullopt,
            message::avp::DiameterIdentity(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return static_cast<T&>(*this);
    }

    T& add_origin_realm(const std::string& value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::OriginRealm},
            message::avp::Flags{},
            std::nullopt,
            message::avp::DiameterIdentity(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return static_cast<T&>(*this);
    }

    std::shared_ptr<message::Message> build()
    {
        return std::move(message);
    }

protected:
    std::shared_ptr<message::Message> message;
};

// <CER> ::= < Diameter Header: 257, REQ >
//           { Origin-Host }
//           { Origin-Realm }
//        1* { Host-IP-Address }
//           { Vendor-Id }
//           { Product-Name }
//           [ Origin-State-Id ]
//         * [ Supported-Vendor-Id ]
//         * [ Auth-Application-Id ]
//         * [ Inband-Security-Id ]
//         * [ Acct-Application-Id ]
//         * [ Vendor-Specific-Application-Id ]
//           [ Firmware-Revision ]
//         * [ AVP ]

// <CEA> ::= < Diameter Header: 257 >
//           { Result-Code }
//           { Origin-Host }
//           { Origin-Realm }
//        1* { Host-IP-Address }
//           { Vendor-Id }
//           { Product-Name }
//           [ Origin-State-Id ]
//           [ Error-Message ]
//           [ Failed-AVP ]
//         * [ Supported-Vendor-Id ]
//         * [ Auth-Application-Id ]
//         * [ Inband-Security-Id ]
//         * [ Acct-Application-Id ]
//         * [ Vendor-Specific-Application-Id ]
//           [ Firmware-Revision ]
//         * [ AVP ]

class CapabilitiesExchangeBuilder : public MessageBuilder<CapabilitiesExchangeBuilder>
{
public:
    CapabilitiesExchangeBuilder()
        : MessageBuilder()
    {
        message->header.command_code = application::common::CommandV::CapabilitiesExchange;
    }

    CapabilitiesExchangeBuilder& add_host_ip_address(const std::string& value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::HostIPAddress},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Address(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_vendor_id(uint32_t value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::VendorId},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Unsigned32(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_product_name(const std::string& value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::ProductName},
            message::avp::Flags{},
            std::nullopt,
            message::avp::UTF8String(value)
        };
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_supported_vendor_id(uint32_t value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::SupportedVendorId},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Unsigned32(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_auth_application_id(uint32_t value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::AuthApplicationId},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Unsigned32(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_acct_application_id(uint32_t value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::AcctApplicationId},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Unsigned32(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_inband_security_id(uint32_t value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::InbandSecurityId},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Unsigned32(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_vendor_specific_auth_application_id(uint32_t vendor_id,
        uint32_t application_id)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::VendorSpecificApplicationId},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Grouped(message::avp::Grouped::value_type{
                message::avp::AVP{
                    message::avp::Code{application::common::AvpCodeV::VendorId},
                    message::avp::Flags{message::avp::Flag::Mandatory},
                    std::nullopt,
                    message::avp::Unsigned32(vendor_id)
                },
                message::avp::AVP{
                    message::avp::Code{application::common::AvpCodeV::AuthApplicationId},
                    message::avp::Flags{message::avp::Flag::Mandatory},
                    std::nullopt,
                    message::avp::Unsigned32(application_id)
                }
            })
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }

    CapabilitiesExchangeBuilder& add_vendor_specific_acct_application_id(uint32_t vendor_id,
        uint32_t application_id)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::VendorSpecificApplicationId},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Grouped(message::avp::Grouped::value_type{
                message::avp::AVP{
                    message::avp::Code{application::common::AvpCodeV::VendorId},
                    message::avp::Flags{message::avp::Flag::Mandatory},
                    std::nullopt,
                    message::avp::Unsigned32(vendor_id)
                },
                message::avp::AVP{
                    message::avp::Code{application::common::AvpCodeV::AcctApplicationId},
                    message::avp::Flags{message::avp::Flag::Mandatory},
                    std::nullopt,
                    message::avp::Unsigned32(application_id)
                }
            })
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }
};

// <DWR>  ::= < Diameter Header: 280, REQ >
//            { Origin-Host }
//            { Origin-Realm }
//            [ Origin-State-Id ]
//          * [ AVP ]

// <DWA>  ::= < Diameter Header: 280 >
//            { Result-Code }
//            { Origin-Host }
//            { Origin-Realm }
//            [ Error-Message ]
//            [ Failed-AVP ]
//            [ Origin-State-Id ]
//          * [ AVP ]

class DeviceWatchdogBuilder : public MessageBuilder<DeviceWatchdogBuilder>
{
public:
    DeviceWatchdogBuilder()
        : MessageBuilder()
    {
        message->header.command_code = application::common::CommandV::DeviceWatchdog;
    }
};

// <DPR>  ::= < Diameter Header: 282, REQ >
//            { Origin-Host }
//            { Origin-Realm }
//            { Disconnect-Cause }
//          * [ AVP ]

// <DPA>  ::= < Diameter Header: 282 >
//            { Result-Code }
//            { Origin-Host }
//            { Origin-Realm }
//            [ Error-Message ]
//            [ Failed-AVP ]
//          * [ AVP ]

class DisconnectPeerBuilder : public MessageBuilder<DisconnectPeerBuilder>
{
public:
    DisconnectPeerBuilder()
        : MessageBuilder()
    {
        message->header.command_code = application::common::CommandV::DisconnectPeer;
    }

    DisconnectPeerBuilder& add_disconnect_cause(
        message::avp::Enumerated::value_type::value_type value)
    {
        auto avp = message::avp::AVP {
            message::avp::Code{application::common::AvpCodeV::DisconnectCause},
            message::avp::Flags{},
            std::nullopt,
            message::avp::Enumerated(value)
        };
        avp.flags.set(message::avp::Flag::Mandatory);
        message->avps.push_back(std::move(avp));
        return *this;
    }
};

} // namespace diameter::application::common

#endif
