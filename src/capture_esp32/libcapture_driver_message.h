#ifndef LIBCAPTURE_DRIVER_MESSAGE_H
#define LIBCAPTURE_DRIVER_MESSAGE_H

#include <WiFiClient.h>
#include "libcapture.h"
#include "BufferedRw.h"

enum class driver_msg_type_t : std::uint8_t
{
    NO_TYPE = 0,
    GET_NEW_CLIENT = 1,
    INGRESS = 2,
    EGRESS = 3,
    CHECK_STATUS = 4,
    CLOSE_CLIENT = 5,
};

class capture_driver_message_t
{
private:
    // metadata (1st part) size: 1 + 1 + 1 + 1 = 4 bytes
    driver_msg_type_t type;
    uint8_t client_id;
    uint8_t piggyback = 0; // If piggyback is on, the semantic of payload_len changes, aka prefetching.
    uint8_t explicit_close = 0; // Flag for explicit closing a client.

    // metadata (2nd part) payload length size: 4 bytes
    uint32_t payload_len; // Need to pay attention to endianess for this variable. Use ntohl() and htonl() before sending it out.
    uint8_t *payload = nullptr;

    void release_old_payload();

public:
    capture_driver_message_t(uint8_t new_client_id = 0);
    ~capture_driver_message_t();
   
    // metadata_size will be present in every message.
    size_t metadata_size();
    size_t full_size();

    driver_msg_type_t get_type();
    void set_type(driver_msg_type_t type_in);
   
    uint8_t get_client_id();
    void set_client_id(uint8_t client_id_in);
   
    uint8_t get_piggyback_opt();
    void set_piggyback_opt(uint8_t opt);

    uint8_t get_explicit_close()
    {
        return explicit_close;
    }
    void set_explicit_close(bool explicit_close_in) 
    {
        explicit_close = (explicit_close_in ? 1 : 0);
    }

    uint32_t get_payload_len();
    void set_payload_len(uint32_t len_in);

    size_t get_payload(uint8_t *buf, size_t buf_size);
    void set_payload(const uint8_t *payload_in, size_t size);
    void set_payload_from_network_read(WiFiClient& driver_connection, Capture& capture, size_t size);

    // read and write methods returns int, indicating the status (0: success, -1: error)
    int write_message_to_driver(WiFiClient& driver_connection, Capture& capture);
    int read_message_from_driver(WiFiClient& driver_connection, Capture& capture);

    void process_piggyback_message(CaptureBufferedReader *bufReader);    
};

#endif // LIBCAPTURE_DRIVER_MESSAGE_H
