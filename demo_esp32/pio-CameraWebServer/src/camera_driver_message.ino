#include <cstdint>
#include <stdio.h>

#include "camera_driver_message.h"

capture_camera_driver_message_t::capture_camera_driver_message_t()
{
    type = camera_driver_msg_type_t::NO_TYPE;
    len = 0;
    payload = nullptr;
}

capture_camera_driver_message_t::~capture_camera_driver_message_t()
{
    LOGV("camera driver message destruct\n");
    if (payload != nullptr)
    {
        delete payload;
        payload = nullptr;
    }
}

size_t capture_camera_driver_message_t::metadata_size()
{
    return (sizeof(type) + sizeof(len));
}

size_t capture_camera_driver_message_t::full_size()
{
    return metadata_size() + ntohl(len);
}

int capture_camera_driver_message_t::write_metadata_to_driver(WiFiClient driver_connection, Capture &capture)
{
    char *buf = new char[metadata_size()]();

    memcpy(buf, &type, sizeof(type));
    memcpy(&(buf[4]), &len, sizeof(len));

    capture.network_write(buf, metadata_size(), driver_connection);
    delete (buf);
    return 0;
}

int capture_camera_driver_message_t::read_metadata_from_driver(WiFiClient driver_connection, Capture &capture)
{
    LOGV("Debug checkpoint2\n");
    // char buf[metadata_size()] = {0};
    char *buf = new char[metadata_size()]();

    capture.network_read(buf, metadata_size(), driver_connection);
    LOGV("Debug checkpoint3\n");
    memcpy((void *)&type, buf, sizeof(type));
     LOGV("Debug checkpoint3a\n");
   type = (camera_driver_msg_type_t)ntohl((uint32_t)type);
    LOGV("Debug checkpoint3aa\n");

    memcpy((void *)&len, &(buf[sizeof(type)]), sizeof(len));
      LOGV("Debug checkpoint3aaa\n");
  len = ntohl(len);

    LOGV("After conversion, msg_type: %u, len: %u\n", type, len);
    delete (buf);
    return 0;
}