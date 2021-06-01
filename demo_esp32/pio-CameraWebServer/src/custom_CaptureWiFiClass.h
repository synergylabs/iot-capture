#ifndef CUSTOME_CAPTURE_WIFI_CLASS_H
#define CUSTOME_CAPTURE_WIFI_CLASS_H

#include "CaptureWiFiClass.h"
#include "camera_driver_message.h"

class CustomCaptureWiFiClass : public CaptureWiFiClass
{
private:

public:

    using CaptureWiFiClass::CaptureWiFiClass;
    int readFromCameraDriver(capture_camera_driver_message_t *read_data_msg);
        int readFromCameraDriver(capture_camera_driver_message_t *read_data_msg, WiFiClient connection);
    int writeToCameraDriver(const char *buf, camera_driver_msg_type_t type, uint32_t len);
    int writeToCameraDriver(const char *buf, camera_driver_msg_type_t type, uint32_t len, WiFiClient connection);
};

#endif
