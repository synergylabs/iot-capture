#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include "libcapture_mutex.h"
#include "BufferedRw.h"
#include "assert.h"
#include "libcapture_driver_message.h"

/**
 * MARK: BaseBuffer
 * */

BaseBuffer::BaseBuffer()
{
    mutex = xSemaphoreCreateMutex();
    buf = nullptr;
    buf_len = 0;
    client_id = 0;
    capacity = 0;

    resize_buffer(DEFAULT_BUFFER_SIZE);
}

BaseBuffer::BaseBuffer(uint8_t client_id_in) : BaseBuffer()
{
    client_id = client_id_in;
}

BaseBuffer::~BaseBuffer()
{
    resize_buffer(0);
}

void BaseBuffer::resize_buffer(size_t capacity_in)
{
    lock();
    resize_buffer_no_lock(capacity_in);
    unlock();
}

void BaseBuffer::resize_buffer_no_lock(size_t capacity_in)
{
    if (buf != nullptr)
    {
        delete[] buf;
        buf = nullptr;
    }
    capacity = capacity_in;
    if (capacity > 0)
    {
        buf = new uint8_t[capacity]();
    }
    buf_len = 0;
}

void BaseBuffer::flush_buffer()
{
    read_from_buffer(nullptr, 0);
}

size_t BaseBuffer::push_data_to_buffer_no_lock(const uint8_t *src, size_t size)
{
    assert(check_free_capacity_no_lock(size));
    memcpy(&buf[buf_len], src, size);
    buf_len += size;
    return size;
}

void BaseBuffer::lock()
{
    lock_mutex(mutex);
}

void BaseBuffer::unlock()
{
    unlock_mutex(mutex);
}

bool BaseBuffer::check_free_capacity_no_lock(size_t incoming)
{
    return ((buf_len + incoming) <= capacity);
}

bool BaseBuffer::check_free_capacity(size_t incoming)
{
    lock();
    bool res = check_free_capacity_no_lock(incoming);
    unlock();
    return res;
}

bool BaseBuffer::check_load_capacity(size_t request)
{
    lock();
    bool res = (buf_len >= request);
    unlock();
    return res;
}

bool BaseBuffer::is_empty_no_lock()
{
    return (buf_len == 0);
}

bool BaseBuffer::is_empty()
{
    lock();
    bool res = is_empty_no_lock();
    unlock();
    return res;
}

size_t BaseBuffer::write_to_buffer(const uint8_t *src, size_t size)
{
    size_t load_size = 0;
    lock();
    if (check_free_capacity_no_lock(size))
    {
        load_size = push_data_to_buffer_no_lock(src, size);
    }
    unlock();
    return load_size;
}

size_t BaseBuffer::load_buffer_from_network_driver(WiFiClient &driver_connection, Capture &capture)
{
    size_t space_availiable = capacity - buf_len;
    size_t data_len = 0, load_size = 0;

    if (data_len < space_availiable)
    {
        capture_driver_message_t read_data_msg = capture_driver_message_t(client_id);
        read_data_msg.set_type(driver_msg_type_t::INGRESS);
        read_data_msg.set_payload_len(space_availiable);
        read_data_msg.write_message_to_driver(driver_connection, capture);

        capture_driver_message_t reply = capture_driver_message_t();
        reply.read_message_from_driver(driver_connection, capture);
        
        data_len = reply.get_payload_len();
        uint8_t *data = new uint8_t[data_len]();
        reply.get_payload(data, data_len);

        load_size = write_to_buffer(data, data_len);
    }
    return load_size;
}

size_t BaseBuffer::read_from_buffer(uint8_t *dest, size_t size)
{
    if (dest != nullptr && size == 0)
    {
        LOGV("Receive 0-length unload request.\n");
        return 0;
    }

    lock();
    if (size > buf_len)
    {
        unlock();
        return 0;
    }

    if (dest == nullptr)
    {
        // Just flush the buffer
        memset(buf, 0, capacity);
        buf_len = 0;
        unlock();
        return 0;
    }

    size_t load_size = size;
    memcpy(dest, buf, load_size);
    buf_len -= load_size;

    uint8_t *tmp = new uint8_t[buf_len]();
    memcpy(tmp, &buf[load_size], buf_len);
    memset(buf, 0, capacity);
    memcpy(buf, tmp, buf_len);
    unlock();

    delete[] tmp;
    return load_size;
}

size_t BaseBuffer::unload_buffer_to_network_driver(WiFiClient &driver_connection, Capture &capture, bool explicit_close)
{
    lock();
    if (is_empty_no_lock())
    {
        unlock();
        return 0;
    }
    size_t load_size = buf_len;

    LOGV("Writing to driver... payload size is %d\n", load_size);
    capture_driver_message_t write_data_msg = capture_driver_message_t(client_id);
    write_data_msg.set_type(driver_msg_type_t::EGRESS);
    write_data_msg.set_payload(buf, load_size);
    write_data_msg.set_explicit_close(explicit_close);
    write_data_msg.write_message_to_driver(driver_connection, capture);
    LOGV("Finish writing to driver\n");

    unlock();

    flush_buffer();

    return load_size;
}

size_t BaseBuffer::get_buf_len()
{
    lock();
    size_t res = buf_len;
    unlock();
    return res;
}

uint8_t BaseBuffer::get_client_id()
{
    return client_id;
}

/**
 * MARK: CaptureBufferedReader
 * */

CaptureBufferedReader::CaptureBufferedReader() : BaseBuffer()
{
}

CaptureBufferedReader::CaptureBufferedReader(const CaptureBufferedReader &ref) : CaptureBufferedReader()
{
    LOGV("Buffered reader copy constructor called!!\n");
}

CaptureBufferedReader::CaptureBufferedReader(uint8_t client) : BaseBuffer(client)
{
}

CaptureBufferedReader::~CaptureBufferedReader()
{
}

/**
 * MARK: CaptureBufferedWriter
 */

CaptureBufferedWriter::CaptureBufferedWriter() : CaptureBufferedWriter(0)
{
}

CaptureBufferedWriter::CaptureBufferedWriter(uint8_t client) : BaseBuffer(client)
{
    flush_mutex = xSemaphoreCreateMutex();
    std::future<void> futureObj = exitSignal.get_future();
    poller_thread = new std::thread(&CaptureBufferedWriter::poller, this, std::move(futureObj));

    driver_connection_ptr = nullptr;
    capture_ptr = nullptr;
}

CaptureBufferedWriter::~CaptureBufferedWriter()
{
    exitSignal.set_value();
    poller_thread->join();

    lock_mutex(flush_mutex);
    flush(true);
    unlock_mutex(flush_mutex);
}

int CaptureBufferedWriter::flush(bool explicit_close)
{
    LOGV("Buffered Writer flush, current buf_len: %d, explicit_close: %d\n", BaseBuffer::get_buf_len(), explicit_close);
    if (BaseBuffer::is_empty())
    {
        return 0;
    }
    BaseBuffer::unload_buffer_to_network_driver(*driver_connection_ptr, *capture_ptr, explicit_close);
    return 0;
}

void CaptureBufferedWriter::poller(std::future<void> futureObj)
{
    while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
    {
        usleep(POLL_INTERVAL);
        lock_mutex(flush_mutex);
        if (!BaseBuffer::is_empty())
        {
            flush();
        }
        unlock_mutex(flush_mutex);
    }
}

size_t CaptureBufferedWriter::buffered_write(WiFiClient *driver_connection_in, Capture *capture_in, const uint8_t *data, size_t size)
{
    lock_mutex(flush_mutex);
    if (driver_connection_ptr == nullptr || capture_ptr == nullptr)
    {
        driver_connection_ptr = driver_connection_in;
        capture_ptr = capture_in;
    }

    if (!BaseBuffer::check_free_capacity(size))
    {
        flush();
    }

    if (BaseBuffer::check_free_capacity(size))
    {
        // Data can fit in buffer
        size = BaseBuffer::write_to_buffer(data, size);
        unlock_mutex(flush_mutex);
        return size;
    }
    else
    {
        unlock_mutex(flush_mutex);
        return -1;
    }
}