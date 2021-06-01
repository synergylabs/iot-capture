#ifndef CUSTOME_CAPTURE_WIFI_CLASS_H
#define CUSTOME_CAPTURE_WIFI_CLASS_H

#include "CaptureWiFiClass.h"
#include "webserver_driver_message.h"

class CustomCaptureWiFiClass : public CaptureWiFiClass
{
private:

public:

    using CaptureWiFiClass::CaptureWiFiClass;
    int readFromWebServerDriver(capture_webserver_driver_message_t *read_data_msg);
    int writeToWebServerDriver(webserver_driver_msg_type_t type);
};

#endif