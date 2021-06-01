#ifndef CAPTURE_BUFFERED_READ_WRITE_H
#define CAPTURE_BUFFERED_READ_WRITE_H

#include <thread>
#include <future>

const uint16_t DEFAULT_BUFFER_SIZE = 4096;
const uint16_t POLL_INTERVAL = 2000; // periodically flush write buffer, in microseconds

class BaseBuffer {
private:
    SemaphoreHandle_t mutex;
    uint8_t *buf = nullptr;
    size_t buf_len = 0;
    uint8_t client_id = 0;
    size_t capacity = 0;

    void resize_buffer(size_t capacity_in);
    void resize_buffer_no_lock(size_t capacity_in);
    void flush_buffer();

    size_t push_data_to_buffer_no_lock(const uint8_t *src, size_t size);

    void lock();
    void unlock();

    bool check_free_capacity_no_lock(size_t incoming);
    bool is_empty_no_lock();

public:
    BaseBuffer();
    BaseBuffer(uint8_t client_id_in);
    ~BaseBuffer();

    bool check_free_capacity(size_t incoming);
    bool check_load_capacity(size_t request);
    bool is_empty();

    /**
     * Return number of bytes successfully load/unloaded.
     * */
    size_t write_to_buffer(const uint8_t *src, size_t size);
    size_t read_from_buffer(uint8_t *dest, size_t size);
    size_t load_buffer_from_network_driver(WiFiClient& driver_connection, Capture& capture);
    size_t unload_buffer_to_network_driver(WiFiClient& driver_connection, Capture& capture, bool explicit_close = false);

    size_t get_buf_len();
    uint8_t get_client_id();
};

class CaptureBufferedReader : public BaseBuffer {
public:
    CaptureBufferedReader();
    CaptureBufferedReader(const CaptureBufferedReader &ref);
    CaptureBufferedReader(uint8_t client_id);
    ~CaptureBufferedReader();
};

class CaptureBufferedWriter : public BaseBuffer {
private:
    std::thread * poller_thread = nullptr;
    std::promise<void> exitSignal;

    WiFiClient *driver_connection_ptr = nullptr;
    Capture *capture_ptr = nullptr;

public:
    SemaphoreHandle_t flush_mutex;

    CaptureBufferedWriter();
    CaptureBufferedWriter(uint8_t client_id);
    ~CaptureBufferedWriter();

    int flush(bool explicit_close = false);
    size_t buffered_write(WiFiClient *driver_connection_in, Capture *capture_in, const uint8_t *data, size_t size);
    void poller(std::future<void> futureObj);
};

#endif