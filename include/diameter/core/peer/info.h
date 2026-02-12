#ifndef DIAMETER_CORE_PEER_INFO_H
#define DIAMETER_CORE_PEER_INFO_H

#include <memory>
#include <set>
#include <string>

#include <diameter/application/common/common.h>
#include <diameter/core/error.h>
#include <diameter/message/avp/vendor_id.h>
#include <diameter/message/header/application_id.h>
#include <diameter/message/message.h>
#include <diameter/serial/serial.h>

namespace diameter::core::peer {

struct PeerInfo
{
    using application_ids_type = std::set<message::header::ApplicationId>;
    using vendor_specific_application_ids_type
        = std::set< std::pair<message::avp::VendorId, message::header::ApplicationId>>;

    std::string origin_host;
    std::string origin_realm;
    message::avp::VendorId vendor_id;
    std::string product_name;

    bool inband_security_supported = false;
    application_ids_type supported_vendor_ids;
    application_ids_type auth_application_ids;
    application_ids_type acct_application_ids;
    vendor_specific_application_ids_type vendor_specific_application_ids;
};

PeerInfo make_peer_info(const std::shared_ptr<message::Message>& message);

}

#endif
