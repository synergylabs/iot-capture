#ifndef CAPTURE_CORE_UTILS_TPP
#define CAPTURE_CORE_UTILS_TPP

#include <stdexcept>
#include <memory>

#include "logger.hpp"

/*
 * Templates have to be defined in header files
 */
template<typename ... Args>
std::string string_format(const std::string &format, Args ... args) {
    auto size = static_cast<size_t>(snprintf(nullptr, 0, format.c_str(), args ...) + 1); // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

template<typename ... Args>
pid_t execute_cmd(const std::string &fmt_cmd, Args ... args) {
    const char TAG[] = "execute_cmd";
    std::string str = string_format(fmt_cmd, args...);
    LOGD("executing cmd: %s", str.c_str());
    return system2(str.c_str());
}

#endif //CAPTURE_CORE_UTILS_TPP
