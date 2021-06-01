#ifndef LOGGER_H
#define LOGGER_H

#include <string>

// convenience macro to prepend TAG macro argument
#define LOG(args...) _log(TAG,args)
// convenience macros to prepend TAG and log level
//#define LOGE(args...) _log(TAG,ERROR,args)
#define LOGE(args...) {\
    _log(TAG,ERROR,args);\
    exit(EXIT_FAILURE);\
    }
#define LOGW(args...) _log(TAG,WARN,args)
#define LOGI(args...) _log(TAG,INFO,args)
#define LOGD(args...) _log(TAG,DEBUG,args)
#define LOGV(args...) _log(TAG,VERBOSE,args)

typedef enum {
    ERROR,  // program cannot continue
    WARN,   // program did something strange, but can continue
    INFO,   // program did something noteworthy
    DEBUG,  // used for development debugging
    VERBOSE, // status messages
    __MAX_LEVELS
} log_level;


char *strip(char *time);

void _log(std::string tag, log_level level, const char *fmt, ...);

// GLOBAL VARS
#define LOG_LEVEL VERBOSE
#define STREAM stderr

#endif // LOGGER_H
