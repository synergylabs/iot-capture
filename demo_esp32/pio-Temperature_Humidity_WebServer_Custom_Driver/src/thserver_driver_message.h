#ifndef CAPTURE_CORE_WEBSERVER_DRIVER_MESSAGE_H
#define CAPTURE_CORE_WEBSERVER_DRIVER_MESSAGE_H

#include <WiFiClient.h>
#include "libcapture.h"

enum class thserver_driver_msg_type_t : std::uint32_t
{
    NO_TYPE = 0,
    GET_TEMP = 1,
    TEMP_VAL = 2,
    GET_HUM = 3,
    HUM_VAL = 4,
};

class capture_thserver_driver_message_t
{
public:
    // 4
    thserver_driver_msg_type_t type; // We just need to communicate the type for this application
    float value;

    capture_thserver_driver_message_t();

    ~capture_thserver_driver_message_t();

    size_t full_size();

    int write_metadata_to_driver(WiFiClient driver_connection, Capture &capture);

    int read_metadata_from_driver(WiFiClient driver_connection, Capture &capture);
};

#endif //CAPTURE_CORE_WEBSERVER_DRIVER_MESSAGE_H
