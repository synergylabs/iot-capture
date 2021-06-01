#ifndef CAPTURE_CORE_WEBSERVER_DRIVER_MESSAGE_H
#define CAPTURE_CORE_WEBSERVER_DRIVER_MESSAGE_H

#include <WiFiClient.h>
#include "libcapture.h"

enum class webserver_driver_msg_type_t : std::uint32_t
{
    NO_TYPE = 0,
    GPIO_26_OFF = 1,
    GPIO_26_ON = 2,
    GPIO_27_OFF = 3,
    GPIO_27_ON = 4,
};

class capture_webserver_driver_message_t
{
public:
    // 4
    webserver_driver_msg_type_t type; // We just need to communicate the type for this application

    capture_webserver_driver_message_t();

    ~capture_webserver_driver_message_t();

    size_t full_size();

    int write_metadata_to_driver(WiFiClient driver_connection, Capture &capture);

    int read_metadata_from_driver(WiFiClient driver_connection, Capture &capture);
};

#endif //CAPTURE_CORE_WEBSERVER_DRIVER_MESSAGE_H
