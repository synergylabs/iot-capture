#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string>

#include "logger.hpp"

using std::string;

static std::string log_level_name[__MAX_LEVELS] = {
        "ERROR",
        "WARN",
        "INFO",
        "DEBUG",
        "VERBOSE",
};

char *strip(char *time) {
    time[strlen(time) - 1] = '\0'; // null the '\n' char at the end
    return time;
}

void _log(const string tag, log_level level, const char *fmt, ...) {
    va_list args;
    time_t t;
    if (level <= LOG_LEVEL) {
        t = time(NULL); // now
        // print log prefix
        fprintf(STREAM, "%s %s | %s: ", strip(asctime(localtime(&t))), tag.c_str(), log_level_name[level].c_str());
        // print message
        va_start(args, fmt);
        vfprintf(STREAM, fmt, args);
        // newline at the end
        fprintf(STREAM, "\n");
    }
}
