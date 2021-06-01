#ifndef LIBCAPTURE_GLOBALS_H
#define LIBCAPTURE_GLOBALS_H

#include <string>

/**
   Global Variables
*/
std::string MONITOR_ADDR = "192.168.11.1";
uint16_t MONITOR_PORT = 8080;
const int MAX_BUFFER_LEN = 1024;
uint16_t TEST_DRIVER_PORT=8188;
const char *USE_DEFAULT_DRIVER = "default_driver";
const char *USE_DEFAULT_CAMERA_DRIVER = "default_camera_driver";

#endif // LIBCAPTURE_GLOBALS_H