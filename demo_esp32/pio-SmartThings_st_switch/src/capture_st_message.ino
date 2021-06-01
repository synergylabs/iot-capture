#include "capture_st_message.h"

capture_st_driver_message_t::capture_st_driver_message_t()
{
    type = st_driver_msg_type_t::NO_TYPE;

    topic_len = 0; // Need to pay attention to endianess for this variable. Use ntohl() and htonl().
    message_len = 0;

    topic = nullptr;
    message = nullptr;
}

capture_st_driver_message_t::~capture_st_driver_message_t()
{
    if (topic != nullptr)
    {
        delete topic;
    }
    if (message != nullptr)
    {
        delete message;
    }
}

size_t capture_st_driver_message_t::metadata_size()
{
    return sizeof(type) + sizeof(topic_len) + sizeof(message_len);
}

size_t capture_st_driver_message_t::full_size()
{
    return metadata_size() + topic_len + message_len;
}

int capture_st_driver_message_t::write_metadata_to_driver(WiFiClient driver_connection, Capture &capture)
{
    char *buf = new char[metadata_size()]();

    memcpy(buf, &type, sizeof(type));
    memcpy(&(buf[4]), &topic_len, sizeof(topic_len));
    memcpy(&(buf[8]), &message_len, sizeof(message_len));

    LOGV("write metadata to driver...\n");
    capture.network_write(buf, metadata_size(), driver_connection);
    delete (buf);
    return 0;
}

int capture_st_driver_message_t::read_metadata_from_driver(WiFiClient driver_connection, Capture &capture)
{
    char *buf = new char[metadata_size()]();
    capture.network_read(buf, metadata_size(), driver_connection);
    memcpy((void *)&type, buf, sizeof(type));
    type = (st_driver_msg_type_t)ntohl((uint32_t)type);
    memcpy((void *)&topic_len, &(buf[4]), sizeof(topic_len));
    topic_len = ntohl(topic_len);
    memcpy((void *)&message_len, &(buf[8]), sizeof(message_len));
    message_len = ntohl(message_len);
    delete (buf);
    return 0;
}
