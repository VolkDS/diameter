#ifndef DIAMETER_LOG_LOGGER_IMPL_H
#define DIAMETER_LOG_LOGGER_IMPL_H

#include <iostream>

namespace diameter::log {

} // namespace diameter::log

#define DIAMETER_LOG_ACTION(LEVEL, PARAMS)                    \
  do {                                                        \
    std::cout << "[" << LEVEL << "] " << PARAMS << std::endl; \
  } while (0)

#define DIAMETER_LOG_FATAL(PARAMS) DIAMETER_LOG_ACTION("FATAL", PARAMS)
#define DIAMETER_LOG_ERROR(PARAMS) DIAMETER_LOG_ACTION("ERROR", PARAMS)
#define DIAMETER_LOG_WARNING(PARAMS) DIAMETER_LOG_ACTION("WARNING", PARAMS)
#define DIAMETER_LOG_INFO(PARAMS) DIAMETER_LOG_ACTION("INFO", PARAMS)
#define DIAMETER_LOG_DEBUG(PARAMS) DIAMETER_LOG_ACTION("DEBUG", PARAMS)
#define DIAMETER_LOG_TRACE(PARAMS) DIAMETER_LOG_ACTION("TRACE", PARAMS)

#endif
