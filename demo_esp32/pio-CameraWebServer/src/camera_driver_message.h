#ifndef CAPTURE_CORE_CAMERA_DRIVER_MESSAGE_H
#define CAPTURE_CORE_CAMERA_DRIVER_MESSAGE_H

#include <WiFiClient.h>
#include "libcapture.h"

enum class camera_driver_msg_type_t : std::uint32_t
{
    NO_TYPE = 0,
    INDEX = 1,
    CONTROL = 2,
    STATUS = 3,
    CAPTURE = 4,
    STREAM_START = 5,
    STREAM_CONTENT_1 = 6,
    STREAM_CONTENT_2 = 7,
    STREAM_CONTENT_3 = 8,
    STREAM_STOP = 9,
};

class capture_camera_driver_message_t
{
public:
    // 4
    camera_driver_msg_type_t type;

    // 4
    uint32_t len; // Need to pay attention to endianess for this variable. Use ntohl() and htonl().

    char *payload;

    capture_camera_driver_message_t();

    ~capture_camera_driver_message_t();

    size_t metadata_size();

    size_t full_size();

    int write_metadata_to_driver(WiFiClient driver_connection, Capture &capture);

    int read_metadata_from_driver(WiFiClient driver_connection, Capture &capture);
};

#endif //CAPTURE_CORE_CAMERA_DRIVER_MESSAGE_H
