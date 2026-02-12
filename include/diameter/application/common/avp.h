#ifndef DIAMETER_APPLICATION_COMMON_AVP_H
#define DIAMETER_APPLICATION_COMMON_AVP_H

#include <diameter/message/avp/code.h>

namespace diameter::application::common {

// 4.5. Diameter Common Protocol AVPs
struct AvpCodeV
{
    enum Value : message::avp::Code
    {
        AcctInterimInterval = 85,
        AccountingRealtimeRequired = 483,
        AcctMultiSessionId = 50,
        AccountingRecordNumber = 485,
        AccountingRecordType = 480,
        AcctSessionId = 44,
        AccountingSubSessionId = 287,
        AcctApplicationId = 259,
        AuthApplicationId = 258,
        AuthRequestType = 274,
        AuthorizationLifetime = 291,
        AuthGracePeriod = 276,
        AuthSessionState = 277,
        ReAuthRequestType = 285,
        Class = 25,
        DestinationHost = 293,
        DestinationRealm = 283,
        DisconnectCause = 273,
        ErrorMessage = 281,
        ErrorReportingHost = 294,
        EventTimestamp = 55,
        ExperimentalResult = 297,
        ExperimentalResultCode = 298,
        FailedAVP = 279,
        FirmwareRevision = 267,
        HostIPAddress = 257,
        InbandSecurityId = 299,
        MultiRoundTimeOut = 272,
        OriginHost = 264,
        OriginRealm = 296,
        OriginStateId = 278,
        ProductName = 269,
        ProxyHost = 280,
        ProxyInfo = 284,
        ProxyState = 33,
        RedirectHost = 292,
        RedirectHostUsage = 261,
        RedirectMaxCacheTime = 262,
        ResultCode = 268,
        RouteRecord = 282,
        SessionId = 263,
        SessionTimeout = 27,
        SessionBinding = 270,
        SessionServerFailover = 271,
        SupportedVendorId = 265,
        TerminationCause = 295,
        UserName = 1,
        VendorId = 266,
        VendorSpecificApplicationId = 260
    };
};

}

#endif