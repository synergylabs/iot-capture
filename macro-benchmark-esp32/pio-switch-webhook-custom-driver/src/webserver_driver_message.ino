#include <cstdint>
#include <stdio.h>

#include "webserver_driver_message.h"

capture_webserver_driver_message_t::capture_webserver_driver_message_t()
{
    type = webserver_driver_msg_type_t::NO_TYPE;
}

capture_webserver_driver_message_t::~capture_webserver_driver_message_t()
{
    LOGV("webserver driver message destruct\n");
}

size_t capture_webserver_driver_message_t::full_size()
{
    return sizeof(type);
}

int capture_webserver_driver_message_t::write_metadata_to_driver(WiFiClient driver_connection, Capture &capture)
{
    char *buf = new char[full_size()]();

    memcpy(buf, &type, sizeof(type));

    capture.network_write(buf, full_size(), driver_connection);
    delete (buf);
    return 0;
}

int capture_webserver_driver_message_t::read_metadata_from_driver(WiFiClient driver_connection, Capture &capture)
{
    // char buf[metadata_size()] = {0};
    char *buf = new char[full_size()]();

    capture.network_read(buf, full_size(), driver_connection);
    memcpy((void *)&type, buf, sizeof(type));
    type = (webserver_driver_msg_type_t)ntohl((uint32_t)type);

    LOGV("After conversion, msg_type: %u\n", (uint32_t)type);
    delete (buf);
    return 0;
}