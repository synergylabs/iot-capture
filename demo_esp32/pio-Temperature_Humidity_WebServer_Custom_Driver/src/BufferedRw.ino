#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include "libcapture_mutex.h"
#include "BufferedRw.h"

CaptureBufferedReader::CaptureBufferedReader()
{
    buf = new uint8_t[READ_BUFFER_SIZE]();
    capacity = READ_BUFFER_SIZE;
    client_id = 0;
    buf_len = 0;
}

CaptureBufferedReader::CaptureBufferedReader(const CaptureBufferedReader &ref) 
{
    LOGV("Buffered reader copy constructor called!!\n");
}

CaptureBufferedReader::CaptureBufferedReader(uint8_t client) : CaptureBufferedReader()
{
    client_id = client;
}

CaptureBufferedReader::~CaptureBufferedReader()
{
    if (buf != nullptr) 
    {
        delete buf;
    }
}

bool CaptureBufferedReader::checkCapacity(size_t incoming)
{
    return ((buf_len + incoming) <= capacity);
}

int CaptureBufferedReader::readn(uint8_t *data, size_t size)
{
    memcpy(data, buf, size);
    memcpy(buf, buf + size, buf_len - size);
    buf_len -= size;
    capacity += size;
    return size;
}

int CaptureBufferedReader::bufferedRead(uint8_t *data, size_t size)
{
    uint8_t *temp_buf;
    size_t temp_buf_len = 0;
    if (buf_len >= size)
    {
        // Data already in buffer, return it
        memcpy(data, buf, size);
        memcpy(buf, buf + size, buf_len - size);
        buf_len = buf_len - size;
        capacity = capacity + size;
        return 0;
    }
    // Data not in buffer, handle it by requesting more from driver
    temp_buf = new uint8_t[size]();
    memcpy(temp_buf, buf, buf_len);
    size -= buf_len;
    temp_buf_len = buf_len;
    capacity = capacity + buf_len;
    buf_len = 0;
    // Two different cases:

    if (!checkCapacity(size))
    {
        return -1;
        // Incoming data cannot fit in the buffer
        // size_t to_request = size / capacity;
        // to_request *= capacity;
        // CaptureWiFi.readFromDriver(client_id, temp_buf+temp_buf_len, to_request);
        // temp_buf_len += to_request;
        // to_request = size - to_request;
        // // Remaining data can fit in buffer
        // readn(temp_buf+temp_buf_len, to_request);
        // temp_buf_len += to_request;
    }

    while (buf_len < size)
    {
        // Incoming data can fit in the buffer
        int bytes_read = CaptureWiFi.readFromDriver(client_id, buf+buf_len, capacity);
        buf_len += bytes_read;
        if (buf_len >= size)
        {
            capacity -= buf_len;
            readn(temp_buf + temp_buf_len, size);
            temp_buf_len += size;
            break;
        }
    }
    memcpy(data, temp_buf, temp_buf_len);
    delete temp_buf;
    return size;
}

CaptureBufferedWriter::CaptureBufferedWriter()
{
    mutex = xSemaphoreCreateMutex();
    buf = new uint8_t[READ_BUFFER_SIZE]();
    client_id = 0;
    capacity = READ_BUFFER_SIZE;
    buf_len = 0;
    client_id = 0;
    std::future<void> futureObj = exitSignal.get_future();
    poller_thread = new std::thread(&CaptureBufferedWriter::poller, this, std::move(futureObj));
    // poller_thread->detach();
}

CaptureBufferedWriter::CaptureBufferedWriter(uint8_t client) : CaptureBufferedWriter()
{
    client_id = client;
}

CaptureBufferedWriter::~CaptureBufferedWriter()
{
    if (buf != nullptr) 
    {
        delete buf;
    }
    exitSignal.set_value();
    poller_thread->join();
}

int CaptureBufferedWriter::flush()
{
    lock(mutex);
    if (buf == nullptr)
    {
        return 0;
    }
    CaptureWiFi.writeToDriver(client_id, buf, buf_len);
    buf_len = 0;
    capacity += buf_len;
    unlock(mutex);
    return 0;
}

void CaptureBufferedWriter::poller(std::future<void> futureObj)
{
    while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
    {
        usleep(POLL_INTERVAL);
        if (buf_len)
        {
            flush();
        }
    }
}

bool CaptureBufferedWriter::checkCapacity(size_t outgoing)
{
    return ((buf_len + outgoing) <= capacity);
}

bool CaptureBufferedWriter::isEmpty()
{
    return (buf_len == 0);
}

int CaptureBufferedWriter::bufferedWrite(const uint8_t *data, size_t size)
{
    lock(mutex);
    if (checkCapacity(size))
    {
        // Data can fit in buffer
        memcpy(buf + buf_len, data, size);
        buf_len += size;
        capacity -= size;
    }
    else
    {
        // Outgoing data cannot fit in buffer, flush buffer and write
        // directly to the driver.
        flush();
        CaptureWiFi.writeToDriver(client_id, data, size);
    }
    unlock(mutex);
    return size;
}