### Configure main project targets ###

# Configure library
add_library(capture_core_logger SHARED STATIC logger.cpp logger.hpp)
install(TARGETS capture_core_logger DESTINATION ${CAPTURE_CORE_INSTALL_LIB_DIR})
install(FILES logger.hpp DESTINATION ${CAPTURE_CORE_INSTALL_INCLUDE_DIR})

add_library(capture_core_crypto SHARED STATIC crypto.cpp crypto.hpp)
target_link_libraries(capture_core_crypto crypto)
install(TARGETS capture_core_crypto DESTINATION ${CAPTURE_CORE_INSTALL_LIB_DIR})
install(FILES crypto.hpp DESTINATION ${CAPTURE_CORE_INSTALL_INCLUDE_DIR})

add_library(capture_core_message SHARED STATIC message.cpp message.hpp)
install(TARGETS capture_core_message DESTINATION ${CAPTURE_CORE_INSTALL_LIB_DIR})
install(FILES message.hpp DESTINATION ${CAPTURE_CORE_INSTALL_INCLUDE_DIR})

add_library(capture_core_utils SHARED STATIC utils.cpp utils.hpp utils.tpp)
install(TARGETS capture_core_utils DESTINATION ${CAPTURE_CORE_INSTALL_LIB_DIR})
install(FILES utils.hpp DESTINATION ${CAPTURE_CORE_INSTALL_INCLUDE_DIR})

# Configure common linking library
set(CAPTURE_LIBS capture_core_logger capture_core_crypto capture_core_message
        capture_core_utils pthread config++)

### Configure executable targets

### Monitor Build Configuration ###
add_executable(monitor_main
        monitor/monitor_main.cpp monitor/monitor_main.hpp
        monitor/device_client.cpp monitor/device_client.hpp)
target_link_libraries(monitor_main ${CAPTURE_LIBS})

#### Local Device Build Configuration ###
#add_executable(local_device_main local_device/local_device_main.c local_device/local_device_main.h)
#target_link_libraries(local_device_main ${CAPTURE_LIBS})
#
#### Driver Build Configuration ###
#add_executable(driver_main driver/driver_main.c driver/driver_main.h)
#target_link_libraries(driver_main ${CAPTURE_LIBS})
