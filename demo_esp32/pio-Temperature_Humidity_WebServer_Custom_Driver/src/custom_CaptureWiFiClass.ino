#include "custom_CaptureWiFiClass.h"

int CustomCaptureWiFiClass::readFromTHServerDriver(capture_thserver_driver_message_t *read_data_msg)
{
    read_data_msg->read_metadata_from_driver(driver_connection, capture);
    return 0;
}

int CustomCaptureWiFiClass::writeToTHServerDriver(thserver_driver_msg_type_t type, float val)
{
    capture_thserver_driver_message_t write_data_msg = capture_thserver_driver_message_t();
    write_data_msg.type = (thserver_driver_msg_type_t)htonl((uint32_t)type);
    write_data_msg.value = val;
    LOGV("writeToCameraDriver about to write metadata\n");
    write_data_msg.write_metadata_to_driver(driver_connection, capture);

    return 0;
}