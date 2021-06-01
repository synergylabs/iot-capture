//
// Created by Han Zhang on 9/3/19.
//

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <memory>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>


#include "message.hpp"
#include "logger.hpp"

using std::unique_ptr;
using std::string;

const string payload_eap_request_t::USE_DEFAULT_DRIVER = "default_driver";
const string payload_eap_request_t::USE_DEFAULT_CAMERA_DRIVER = "default_camera_driver";

int new_eap_creds_msg(unique_ptr<message_t> &msg, unsigned driver_port, char const *username, char const *password) {
    assert(msg != nullptr);

    auto *payload_eap = new payload_eap_creds_t;
    payload_eap->driver_port = driver_port;
    payload_eap->username_len = strlen(username);
    payload_eap->username = (char *) malloc(payload_eap->username_len + 1);
    strncpy(payload_eap->username, username, payload_eap->username_len);
    payload_eap->password_len = strlen(password);
    payload_eap->password = (char *) malloc(payload_eap->password_len + 1);
    strncpy(payload_eap->password, password, payload_eap->password_len);

    msg->msg_type = CAPTURE_MSG_EAP_CREDS;
    msg->payload = payload_eap;

    return 0;
}

std::unique_ptr<message_t> new_empty_msg() {
    std::unique_ptr<message_t> msg(new message_t);
    msg->msg_type = CAPTURE_MSG_NO_TYPE;
    msg->payload = nullptr;

    return msg;
}

int read_eap_request_msg(std::unique_ptr<message_t> &msg, int sock_fd) {
    int val_read = 0;
    val_read = read(sock_fd, &(msg->msg_type), sizeof(msg->msg_type));

    if (val_read < 0) {
        return -1;
    }

    msg->msg_type = static_cast<msg_type_t>(ntohl(msg->msg_type));

    const char TAG[] = "message: read EAP request";
    if (msg->msg_type != CAPTURE_MSG_EAP_REQUEST) {
        LOGE("Received wrong type");
    }

    auto eap_request_payload = new payload_eap_request_t;
    val_read = read(sock_fd, &(eap_request_payload->driver_name_len), sizeof(eap_request_payload->driver_name_len));
    if (val_read < 0) {
        return -1;
    }
    eap_request_payload->driver_name_len = ntohl(eap_request_payload->driver_name_len);
    eap_request_payload->driver_name = new char[eap_request_payload->driver_name_len];
    val_read = read(sock_fd, eap_request_payload->driver_name, eap_request_payload->driver_name_len);
    if (val_read < 0) {
        return -1;
    }

    msg->payload = eap_request_payload;

    return 0;
}

