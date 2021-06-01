#include "custom_CaptureWiFiClass.h"

int CustomCaptureWiFiClass::readFromWebServerDriver(capture_webserver_driver_message_t *read_data_msg)
{
    read_data_msg->read_metadata_from_driver(driver_connection, capture);
    return 0;
}

int CustomCaptureWiFiClass::writeToWebServerDriver(webserver_driver_msg_type_t type)
{
    capture_webserver_driver_message_t write_data_msg = capture_webserver_driver_message_t();
    write_data_msg.type = (webserver_driver_msg_type_t)htonl((uint32_t)type);
    LOGV("writeToCameraDriver about to write metadata\n");
    write_data_msg.write_metadata_to_driver(driver_connection, capture);

    return 0;
}