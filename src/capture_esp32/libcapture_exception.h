#ifndef LIBCAPTURE_EXCEPTION_H
#define LIBCAPTURE_EXCEPTION_H

#include <stdexcept>

class NotYetImplementedException : public std::logic_error
{
public:
    NotYetImplementedException() : std::logic_error{"Function not yet implemented"} {}
};

#endif