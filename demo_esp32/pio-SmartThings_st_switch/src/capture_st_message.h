#ifndef CAPTURE_CORE_ST_DRIVER_MESSAGE_H
#define CAPTURE_CORE_ST_DRIVER_MESSAGE_H

#include <WiFiClient.h>
#include "libcapture.h"

enum class st_driver_msg_type_t : std::uint32_t
{
    NO_TYPE = 0,
    SUBSCRIBE = 1,
    INCOMING_EVENT = 2,
    PUBLISH = 3
};

class capture_st_driver_message_t
{
public:
    // 4
    st_driver_msg_type_t type;

    // 4
    uint32_t topic_len; // Need to pay attention to endianess for this variable. Use ntohl() and htonl().
    uint32_t message_len;

    char *topic;
    char *message;

    capture_st_driver_message_t();

    ~capture_st_driver_message_t();

    size_t metadata_size();

    size_t full_size();

    int write_metadata_to_driver(WiFiClient driver_connection, Capture &capture);

    int read_metadata_from_driver(WiFiClient driver_connection, Capture &capture);
};

#endif //CAPTURE_CORE_CAMERA_DRIVER_MESSAGE_H
