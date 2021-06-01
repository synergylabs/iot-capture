#include "custom_CaptureWiFiClass.h"

int CustomCaptureWiFiClass::readFromCameraDriver(capture_st_driver_message_t *read_data_msg)
{
    readFromCameraDriver(read_data_msg, driver_connection);
}

int CustomCaptureWiFiClass::readFromCameraDriver(capture_st_driver_message_t *read_data_msg, WiFiClient &connection)
{
    LOGV("Debug checkpoint1\n");
    read_data_msg->read_metadata_from_driver(connection, capture);

    LOGV("Next try to read data for topic length: %u\n", read_data_msg->topic_len);

    if (read_data_msg->topic_len > 0)
    {
        read_data_msg->topic = new char[read_data_msg->topic_len+1]();
        capture.network_read(read_data_msg->topic, read_data_msg->topic_len, connection);
        LOGV("Finish reading data for length: %u\n", read_data_msg->topic_len);
    }
    if (read_data_msg->message_len > 0)
    {
        read_data_msg->message = new char[read_data_msg->message_len+1]();
        capture.network_read(read_data_msg->message, read_data_msg->message_len, connection);
        LOGV("Finish reading data for length: %u\n", read_data_msg->message_len);
    }
    return 0;
}

int CustomCaptureWiFiClass::writeToCameraDriver(st_driver_msg_type_t type, uint32_t topic_len,
                                                const char *topic, uint32_t message_len, char *message)
{
    writeToCameraDriver(type, topic_len, topic, message_len, message, driver_connection);
}

int CustomCaptureWiFiClass::writeToCameraDriver(st_driver_msg_type_t type, uint32_t topic_len,
                                                const char *topic, uint32_t message_len, char *message,
                                                WiFiClient &connection)
{
    capture_st_driver_message_t write_data_msg = capture_st_driver_message_t();
    write_data_msg.type = (st_driver_msg_type_t)htonl((uint32_t)type);
    write_data_msg.topic_len = htonl(topic_len);
    write_data_msg.message_len = htonl(message_len);
    LOGV("writeToCameraDriver about to write metadata\n");
    write_data_msg.write_metadata_to_driver(connection, capture);

    if (topic_len > 0)
    {
        LOGV("writeToCameraDriver about to write payload topic\n");
        capture.network_write(topic, topic_len, connection);
    }
    if (message_len > 0)
    {
        LOGV("writeToCameraDriver about to write payload message\n");
        capture.network_write(message, message_len, connection);
    }

    return 0;
}

int CustomCaptureWiFiClass::sendSubscribeMessage(const char *topic, uint32_t topic_len)
{
    writeToCameraDriver(st_driver_msg_type_t::SUBSCRIBE, topic_len, topic, 0, nullptr);
}
int CustomCaptureWiFiClass::sendPublishMessage(const char *topic, uint32_t topic_len, char *message, uint32_t message_len)
{
    writeToCameraDriver(st_driver_msg_type_t::PUBLISH, topic_len, topic, message_len, message);
}