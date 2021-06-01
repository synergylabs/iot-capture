#include "custom_CaptureWiFiClass.h"

int CustomCaptureWiFiClass::readFromCameraDriver(capture_camera_driver_message_t *read_data_msg)
{
    readFromCameraDriver(read_data_msg, driver_connection);
}

int CustomCaptureWiFiClass::readFromCameraDriver(capture_camera_driver_message_t *read_data_msg, WiFiClient connection)
{
    LOGV("Debug checkpoint1\n");
    read_data_msg->read_metadata_from_driver(connection, capture);

    LOGV("Next try to read data for length: %u\n", read_data_msg->len);

    if (read_data_msg->len > 0)
    {
        read_data_msg->payload = new char[read_data_msg->len];
        capture.network_read(read_data_msg->payload, read_data_msg->len, connection);
        LOGV("Finish reading data for length: %u\n", read_data_msg->len);
    }
    return 0;
}

int CustomCaptureWiFiClass::writeToCameraDriver(const char *buf, camera_driver_msg_type_t type, uint32_t len)
{
    writeToCameraDriver(buf, type, len, driver_connection);
}

int CustomCaptureWiFiClass::writeToCameraDriver(const char *buf, camera_driver_msg_type_t type, uint32_t len, WiFiClient connection)
{
    LOGV("inside buf is: %s\n", buf);
    capture_camera_driver_message_t write_data_msg = capture_camera_driver_message_t();
    write_data_msg.type = (camera_driver_msg_type_t)htonl((uint32_t)type);
    LOGV("writeToCameraDriver len: %u\n", len);
    write_data_msg.len = htonl(len);
    LOGV("writeToCameraDriver about to write metadata\n");
    write_data_msg.write_metadata_to_driver(connection, capture);

    if (len > 0)
    {
        LOGV("writeToCameraDriver about to write payload\n");
        capture.network_write(buf, len, connection);
    }

    return 0;
}