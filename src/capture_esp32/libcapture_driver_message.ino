#include "libcapture_driver_message.h"

#include <cstdint>
#include <stdio.h>

capture_driver_message_t::capture_driver_message_t(uint8_t new_client_id)
{
    type = driver_msg_type_t::NO_TYPE;
    client_id = new_client_id;
    explicit_close = 0;
    payload_len = 0;
    payload = nullptr;
}

capture_driver_message_t::~capture_driver_message_t()
{
    LOGV("Message descructor! Message type: %d\n", type);
    release_old_payload();
    // LOGV("Finish self destruction.\n");
}

size_t capture_driver_message_t::metadata_size()
{
    return (sizeof(type) + sizeof(client_id) + sizeof(piggyback) + sizeof(explicit_close) + sizeof(payload_len));
}

size_t capture_driver_message_t::full_size()
{
    return metadata_size() + payload_len;
}

driver_msg_type_t capture_driver_message_t::get_type()
{
    return type;
}

void capture_driver_message_t::set_type(driver_msg_type_t type_in)
{
    this->type = type_in;
}

uint8_t capture_driver_message_t::get_client_id()
{
    return client_id;
}

void capture_driver_message_t::set_client_id(uint8_t client_id_in)
{
    this->client_id = client_id_in;
}

uint8_t capture_driver_message_t::get_piggyback_opt()
{
    return piggyback;
}

void capture_driver_message_t::set_piggyback_opt(uint8_t opt)
{
    piggyback = opt;
}

uint32_t capture_driver_message_t::get_payload_len()
{
    return payload_len;
}

size_t capture_driver_message_t::get_payload(uint8_t *buf, size_t buf_size)
{
    assert(buf_size >= payload_len);
    memcpy(buf, payload, payload_len);
    return payload_len;
}

void capture_driver_message_t::set_payload(const uint8_t *payload_in, size_t size)
{
    release_old_payload();
    payload = new uint8_t[size]();
    if (payload_in != nullptr)
    {
        memcpy(payload, payload_in, (size_t)size);
    }
    payload_len = size;
}

void capture_driver_message_t::set_payload_from_network_read(WiFiClient &driver_connection, Capture &capture, size_t size)
{
    LOGV("Begin capture_driver_message_t::set_payload_from_network_read, waiting for %u bytes\n", size);
    set_payload(nullptr, size);
    LOGV("ck1\n");
    capture.network_read(payload, size, driver_connection);
    LOGV("ck2\n");
}

void capture_driver_message_t::set_payload_len(uint32_t len_in)
{
    // We only allow INGRESS message to directly change payload_len,
    // for other messages, payload and payload_len should be changed together.
    assert(type == driver_msg_type_t::INGRESS);
    release_old_payload();
    payload_len = len_in;
}

void capture_driver_message_t::release_old_payload()
{
    if (payload != nullptr)
    {
        delete[] payload;
        payload = nullptr;
        payload_len = 0;
    }
}

int capture_driver_message_t::write_message_to_driver(WiFiClient &driver_connection, Capture &capture)
{
    uint32_t actual_payload_len = 0;
    if (type == driver_msg_type_t::INGRESS)
    {
        actual_payload_len = 0;
    }
    else
    {
        actual_payload_len = get_payload_len();
    }

    uint32_t total_size = metadata_size() + actual_payload_len;
    LOGV("Total size: %u\n", total_size);

    uint8_t *buf = new uint8_t[total_size];
    uint32_t payload_len_network_order = htonl(get_payload_len());

    buf[0] = (char)get_type();
    buf[1] = (char)get_client_id();
    buf[2] = get_piggyback_opt();
    buf[3] = get_explicit_close();
    memcpy(&(buf[4]), &payload_len_network_order, sizeof(payload_len_network_order));

    LOGV("Sending message type: %d, client_id: %u, payload_size: %u, len (after conversion): %u\n", get_type(), get_client_id(), (get_payload_len()), payload_len_network_order);

    if (actual_payload_len > 0)
    {
        assert(actual_payload_len == get_payload_len());
        get_payload(&buf[4 + sizeof(payload_len_network_order)], actual_payload_len);
    }

    LOGV("About to send buf to network_write\n");
    capture.network_write((char *)buf, total_size, driver_connection);
    LOGV("Done writing.\n");

    delete[] buf;
    LOGV("Done. Deleted buf.\n");

    return 0;
}

int capture_driver_message_t::read_message_from_driver(WiFiClient &driver_connection, Capture &capture)
{
    size_t bytes_read = 0;
    uint8_t *meta_buf = new uint8_t[metadata_size()];

    LOGV("Reading into meta_buf...\n");
    bytes_read = capture.network_read(meta_buf, metadata_size(), driver_connection);
    LOGV("meta_buf read bytes: %u\n", bytes_read);

    LOGV("Message conversion...\n");
    set_type((driver_msg_type_t)meta_buf[0]);
    set_client_id((uint8_t)meta_buf[1]);
    set_piggyback_opt(meta_buf[2]);
    set_explicit_close(meta_buf[3]);
    uint32_t payload_len_network_order = 0;
    memcpy((void *)&payload_len_network_order, &(meta_buf[4]), sizeof(payload_len_network_order));
    uint32_t payload_len_in = ntohl(payload_len_network_order);

    delete[] meta_buf;

    if (payload_len_in > 0)
    {
        // Read payload content
        set_payload_from_network_read(driver_connection, capture, payload_len_in);
    }
    LOGV("Finish capture_driver_message_t::read_message_from_driver\n");
    return 0;
}

void capture_driver_message_t::process_piggyback_message(CaptureBufferedReader *bufReader)
{
    if (piggyback == 0) {
        LOGV("No piggy back message\n");
        return;
    }

    LOGV("Pushing piggyback message into read buffer (len %u)...\n", get_payload_len());
    if (!bufReader->check_free_capacity(get_payload_len()))
    {
        LOGV("Piggyback message too long! Abort\n");
        return;
    }

    bufReader->write_to_buffer(payload, payload_len);
}
