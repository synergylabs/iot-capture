#include "libcapture_monitor_message.h"

void capture_monitor_msg_t::print()
{
    LOGV("Message type: %u\n", msg_type);
}

void capture_monitor_msg_t::generate_network_msg(char **buf, size_t *buf_size)
{
    *buf_size = sizeof(msg_type) + sizeof(payload.eap_req->driver_name_len) + strlen(payload.eap_req->driver_name);
    *buf = new char[*buf_size];
    memset(*buf, 0, *buf_size);

    unsigned tmp;

    tmp = (htonl(msg_type));
    memcpy(*buf, (char *)&tmp, sizeof(msg_type));

    tmp = (htonl(payload.eap_req->driver_name_len));
    memcpy(*buf + sizeof(msg_type),
           (char *)&tmp,
           sizeof(payload.eap_req->driver_name_len));
    memcpy(*buf + sizeof(msg_type) + sizeof(payload.eap_req->driver_name_len),
           payload.eap_req->driver_name,
           strlen(payload.eap_req->driver_name));
}
