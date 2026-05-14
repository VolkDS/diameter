#ifndef DIAMETER_APPLICATION_COMMON_RESULT_CODE_H
#define DIAMETER_APPLICATION_COMMON_RESULT_CODE_H

#include <diameter/message/avp/avp.h>

namespace diameter::application::common {

struct ResultCodeV
{
    enum Value : message::avp::Unsigned32::value_type
    {
        // 1xxx (Informational)
        MULTI_ROUND_AUTH = 1001,

        // 2xxx (Success)
        SUCCESS = 2001,
        LIMITED_SUCCESS = 2002,

        // 3xxx (Protocol Errors)
        COMMAND_UNSUPPORTED = 3001,
        UNABLE_TO_DELIVER = 3002,
        REALM_NOT_SERVED = 3003,
        TOO_BUSY = 3004,
        LOOP_DETECTED = 3005,
        REDIRECT_INDICATION = 3006,
        APPLICATION_UNSUPPORTED = 3007,
        INVALID_HDR_BITS = 3008,
        INVALID_AVP_BITS = 3009,
        UNKNOWN_PEER = 3010,

        // 4xxx (Transient Failures)
        AUTHENTICATION_REJECTED = 4001,
        OUT_OF_SPACE = 4002,
        ELECTION_LOST = 4003,

        // 5xxx (Permanent Failure)
        AVP_UNSUPPORTED = 5001,
        UNKNOWN_SESSION_ID = 5002,
        AUTHORIZATION_REJECTED = 5003,
        INVALID_AVP_VALUE = 5004,
        MISSING_AVP = 5005,
        RESOURCES_EXCEEDED = 5006,
        CONTRADICTING_AVPS = 5007,
        AVP_NOT_ALLOWED = 5008,
        AVP_OCCURS_TOO_MANY_TIMES = 5009,
        NO_COMMON_APPLICATION = 5010,
        UNSUPPORTED_VERSION = 5011,
        UNABLE_TO_COMPLY = 5012,
        INVALID_BIT_IN_HEADER = 5013,
        INVALID_AVP_LENGTH = 5014,
        INVALID_MESSAGE_LENGTH = 5015,
        INVALID_AVP_BIT_COMBO = 5016,
        NO_COMMON_SECURITY = 5017
    };
};
} // namespace diameter::application::common

#endif
