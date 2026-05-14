#ifndef DIAMETER_LOG_LOGGER_H
#define DIAMETER_LOG_LOGGER_H

#include <diameter/log/logger_impl.h>

#ifndef DIAMETER_LOG_ERROR
#  error "<diameter/log/logger_impl.h> must define DIAMETER_LOG_ERROR macro"
#endif

#ifndef DIAMETER_LOG_WARNING
#  error "<diameter/log/logger_impl.h> must define DIAMETER_LOG_WARNING macro"
#endif

#ifndef DIAMETER_LOG_INFO
#  error "<diameter/log/logger_impl.h> must define DIAMETER_LOG_INFO macro"
#endif

#ifndef DIAMETER_LOG_DEBUG
#  error "<diameter/log/logger_impl.h> must define DIAMETER_LOG_DEBUG macro"
#endif

#ifndef DIAMETER_LOG_TRACE
#  error "<diameter/log/logger_impl.h> must define DIAMETER_LOG_TRACE macro"
#endif

#endif
