#ifndef DIAMETER_MESSAGE_HEADER_APPLICATION_ID_H
#define DIAMETER_MESSAGE_HEADER_APPLICATION_ID_H

#include <cstdint>

namespace diameter::message::header {

// see RFC6733 section 3, Application-ID
using ApplicationId = uint32_t;

struct ApplicationV
{
    // http://www.iana.org/assignments/aaa-parameters/aaa-parameters.xhtml
    // Application IDs
    enum Value : ApplicationId
    {
        Common = 0,              // RFC6733 section 2.4
        NASREQ = 1,              // RFC7155
        MobileIPv4 = 2,          // RFC4004
        Accounting = 3,          // RFC6733 section 2.4
        CreditControl = 4,       // RFC4006
        EAP = 5,                 // RFC4072
        SIP = 6,                 // RFC4740
        MobileIPv4IKE = 7,       // RFC5778
        MobileIPv4Auth = 8,      // RFC5778
        QoS = 9,                 // RFC5866
        CapabilitiesUpdate = 10, // RFC6737
        IKESK = 11,              // RFC6738
        NATControl = 12,         // RFC6736
        ERP = 13,                // RFC6942

        // 14 - 16777215(0x00ffffff) Unassigned

        TGPP_Cx = 16777216,      // 3GPP TS 29.228 3GPP TS 29.229
        TGPP_Sh = 16777217,      // 3GPP TS 29.328 3GPP TS 29.329
        TGPP_Re = 16777218,      // 3GPP TS 32.296
        TGPP_Wx = 16777219,      // 3GPP TS 29.234
        TGPP_Zn = 16777220,      // 3GPP TS 29.109
        TGPP_Zh = 16777221,      // 3GPP TS 29.109
        TGPP_Gq = 16777222,      // 3GPP TS 29.209
        TGPP_Gmb = 16777223,     // 3GPP TS 29.061
        TGPP_Gx = 16777224,      // 3GPP TS 29.210
        TGPP_Gx_Gy = 16777225,   // 3GPP TS 29.210
        TGPP_MM10 = 16777226,    // 3GPP TS 29.140
        EricssonMSI = 16777227,  //
        EricssonZx = 16777228,   //
        TGPP_Rx = 16777229,      // 3GPP TS 29.211
        TGPP_Pr = 16777230,      // 3GPP TS 29.234
        ETSIe4 = 16777231,       // ETSI ES 283 034
        EricssonCIP = 16777232,  //
        EricssonMm = 16777233,   //
        VodafoneGx = 16777234,   //
        ITU_T_Rs = 16777235,     // ITU-T Recommendation Q.3301.1
        TGPP_Rx2 = 16777236,     // 3GPP TS 29.214
        TGPP_Ty = 16777237,      //
        TGPP_Gx2 = 16777238,     // 3GPP TS 29.212

        // 16777364-4294967294(0xfffffffe) Unassigned

        Relay = 0xffffffff       // RFC6733 section 2.4
    };
};

} // namespace diameter::message::header

#endif
