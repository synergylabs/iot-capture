#ifndef LIBCAPTURE_MESSAGE_H
#define LIBCAPTURE_MESSAGE_H

enum monitor_msg_type_t : uint32_t
{
    MSG_NO_TYPE = 0,
    MSG_EAP_CREDS = 1,
    MSG_EAP_REQUEST = 2,
};

struct payload_eap_creds_t
{
    unsigned driver_port;
    unsigned username_len;
    unsigned password_len;
    char *username;
    char *password;
};

struct payload_eap_request_t
{
    unsigned driver_name_len;
    char *driver_name;
};

typedef union typedefPayload {
    struct payload_eap_request_t *eap_req;
    struct payload_eap_creds_t *eap_cred;
}eap_payload;

class capture_monitor_msg_t {
public:
    monitor_msg_type_t msg_type;
    eap_payload payload;

    void print();
    void generate_network_msg(char **buf, size_t *buf_len);
};

// TODO: Free all the dynamic memory allocated

int new_eap_request_msg(capture_monitor_msg_t *msg, const char *driver)
{
    struct payload_eap_request_t *payload_eap = new payload_eap_request_t(); 
    payload_eap->driver_name_len = strlen(driver);
    payload_eap->driver_name = new char[strlen(driver) + 1];
    strcpy(payload_eap->driver_name, driver);

    msg->msg_type = monitor_msg_type_t::MSG_EAP_REQUEST;
    msg->payload.eap_req = payload_eap;

    return 0;
}

#endif //LIBCAPTURE_MESSAGE_H
