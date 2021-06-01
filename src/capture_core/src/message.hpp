//
// Created by Han Zhang on 8/28/19.
//

#ifndef CAPTURE_CORE_MESSAGE_HPP
#define CAPTURE_CORE_MESSAGE_HPP

#include <memory>

enum msg_type_t : uint32_t {
    CAPTURE_MSG_NO_TYPE = 0,
    CAPTURE_MSG_EAP_CREDS = 1,
    CAPTURE_MSG_EAP_REQUEST = 2,
};

class payload_base_t {
public:
    virtual ~payload_base_t() = default;
};

class payload_eap_creds_t : public payload_base_t {
public:
    unsigned driver_port;
    unsigned username_len;
    unsigned password_len;
    char *username;
    char *password;

    ~payload_eap_creds_t() override = default;
};

class payload_eap_request_t : public payload_base_t {
public:
    static const std::string USE_DEFAULT_DRIVER;
    static const std::string USE_DEFAULT_CAMERA_DRIVER;

    unsigned driver_name_len;
    char *driver_name;

    ~payload_eap_request_t() override = default;
};

struct message_t {
    // TODO: move to protocol buffer -- ON HOLD
    msg_type_t msg_type;
    payload_base_t *payload;

    // TODO:
    // FIXME: memory leak due to dynamic memory allocation of all these pointers
};

std::unique_ptr<message_t> new_empty_msg();

int new_eap_creds_msg(std::unique_ptr<message_t> &msg, unsigned driver_port,
                      char const *username, char const *password);

int read_eap_request_msg(std::unique_ptr<message_t> &msg, int sock_fd);

#endif //CAPTURE_CORE_MESSAGE_HPP
