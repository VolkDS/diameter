#include <diameter/core/peer/info.h>

namespace diameter::core::peer {

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

PeerInfo make_peer_info(const std::shared_ptr<message::Message>& message)
{
    if (message->header.application_id != message::header::ApplicationV::Common) {
        return PeerInfo{};
    }
    if (message->header.command_code != application::base::CommandV::CapabilitiesExchange) {
        return PeerInfo{};
    }

    auto peer_info = PeerInfo{};
    for (const auto& avp : message->avps) {
        // Origin-Host
        if (avp.code == 264) {
            auto value = serial::avp::value_as<message::avp::DiameterIdentity>(avp.value);
            if (!peer_info.origin_host.empty()) {
                throw AvpOccursTooManyTimes("Duplicate Origin-Host");
            }
            peer_info.origin_host = value->value();
        }

        // Origin-Realm
        if (avp.code == 296) {
            auto value = serial::avp::value_as<message::avp::DiameterIdentity>(avp.value);
            if (!peer_info.origin_realm.empty()) {
                throw AvpOccursTooManyTimes("Duplicate Origin-Realm");
            }
            peer_info.origin_realm = value->value();
        }

        // Vendor-Id
        if (avp.code == 266) {
            auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
            if (!peer_info.origin_realm.empty()) {
                throw AvpOccursTooManyTimes("Duplicate Vendor-Id");
            }
            peer_info.vendor_id = *value;
        }

        // Product-Name
        if (avp.code == 269) {
            auto value = serial::avp::value_as<message::avp::UTF8String>(avp.value);
            if (!peer_info.product_name.empty()) {
                throw AvpOccursTooManyTimes("Duplicate Product-Name");
            }
            peer_info.product_name = *value;
        }

        // Supported-Vendor-Id
        if (avp.code == 265) {
            auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
            peer_info.supported_vendor_ids.insert(*value);
        }

        // Auth-Application-Id
        if (avp.code == 258) {
            auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
            peer_info.auth_application_ids.insert(*value);
        }

        // Inband-Security-Id
        // AVP is of type Unsigned32 and used in order to advertise support of the security portion of the application.
        // The use of this AVP in CER and CEA messages is NOT RECOMMENDED.
        // NO_INBAND_SECURITY = 0
        // TLS = 1
        if (avp.code == 299) {
            auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
            if (*value == 1) {
                peer_info.inband_security_supported = true;
            }
            else {
                peer_info.inband_security_supported = false;
            }
        }

        // Acct-Application-Id
        if (avp.code == 259) {
            auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
            peer_info.acct_application_ids.insert(*value);
        }

        // Vendor-Specific-Application-Id
        // <Vendor-Specific-Application-Id> ::= < AVP Header: 260 >
        //                                      { Vendor-Id }
        //                                      [ Auth-Application-Id ]
        //                                      [ Acct-Application-Id ]
        if (avp.code == 260) {
            auto value = serial::avp::value_as<message::avp::Grouped>(avp.value);

            message::avp::VendorId vendor_id = 0;
            message::header::ApplicationId auth_application_id = 0;
            message::header::ApplicationId acct_application_id = 0;
            for (const auto& a : *value) {
                // Vendor-Id
                if (a.code == 266) {
                    auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
                    vendor_id = *value;
                }

                // Auth-Application-Id
                if (a.code == 258) {
                    auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
                    auth_application_id = *value;
                }

                // Acct-Application-Id
                if (a.code == 259) {
                    auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
                    acct_application_id = *value;
                }
            }

            if (vendor_id == 0) {
                throw MissingAvp("Not found Vendor-Id in Vendor-Specific-Application-Id");
            }

            if (auth_application_id == 0 && acct_application_id == 0) {
                throw MissingAvp("Not found Auth-Application-Id or Acct-Application-Id in Vendor-Specific-Application-Id");
            }

            if (auth_application_id != 0) {
                peer_info.vendor_specific_application_ids.insert({vendor_id, auth_application_id});
            }
            else {
                peer_info.vendor_specific_application_ids.insert({vendor_id, acct_application_id});
            }
        }
    }
    return peer_info;
}

}