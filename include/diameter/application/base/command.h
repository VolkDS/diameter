#ifndef DIAMETER_APPLICATION_BASE_COMMAND_H
#define DIAMETER_APPLICATION_BASE_COMMAND_H

#include <diameter/message/header/command_code.h>

namespace diameter::application::base {

struct CommandV
{
    enum Value : message::header::CommandCode
    {
        AbortSession         = 274, // ASR 8.5.1
        AccountingRequest    = 271, // ACR 9.7.1
        CapabilitiesExchange = 257, // CER 5.3.1
        DeviceWatchdog       = 280, // DWR 5.5.1
        DisconnectPeer       = 282, // DPR 5.4.1
        ReAuth               = 258, // RAR 8.3.1
        SessionTermination   = 275  // STR 8.4.1
    };
};

}

#endif
