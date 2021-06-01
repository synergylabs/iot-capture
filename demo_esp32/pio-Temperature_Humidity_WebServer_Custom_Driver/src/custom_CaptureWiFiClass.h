#ifndef CUSTOME_CAPTURE_WIFI_CLASS_H
#define CUSTOME_CAPTURE_WIFI_CLASS_H

#include "CaptureWiFiClass.h"
#include "thserver_driver_message.h"

class CustomCaptureWiFiClass : public CaptureWiFiClass
{
private:

public:

    using CaptureWiFiClass::CaptureWiFiClass;
    int readFromTHServerDriver(capture_thserver_driver_message_t *read_data_msg);
    int writeToTHServerDriver(thserver_driver_msg_type_t type, float val);
};

#endif