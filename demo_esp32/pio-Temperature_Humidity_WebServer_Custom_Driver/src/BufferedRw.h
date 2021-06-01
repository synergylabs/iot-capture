#ifndef CAPTURE_BUFFERED_READ_WRITE_H
#define CAPTURE_BUFFERED_READ_WRITE_H

#include <thread>
#include <future>

const uint16_t READ_BUFFER_SIZE = 4096;
const uint16_t WRITE_BUFFER_SIZE = 4096;
const uint16_t POLL_INTERVAL = 2000; // 2000 microseconds

class CaptureBufferedReader {
private:
    uint8_t *buf = nullptr;
    size_t buf_len = 0;
    uint8_t client_id = 0;
    size_t capacity = READ_BUFFER_SIZE;
public:
    CaptureBufferedReader();
    CaptureBufferedReader(const CaptureBufferedReader &ref);
    CaptureBufferedReader(uint8_t client_id);
    ~CaptureBufferedReader();
    bool checkCapacity(size_t incoming);
    int readn(uint8_t *data, size_t size);
    int bufferedRead(uint8_t *data, size_t size);
};

class CaptureBufferedWriter {
private:
    SemaphoreHandle_t mutex;
    uint8_t *buf = nullptr;
    uint8_t client_id = 0;
    size_t buf_len = 0;
    size_t capacity = WRITE_BUFFER_SIZE;
    std::thread * poller_thread = nullptr;
    std::promise<void> exitSignal;
public:
    CaptureBufferedWriter();
    CaptureBufferedWriter(uint8_t client_id);
    ~CaptureBufferedWriter();
    bool isEmpty();
    bool checkCapacity(size_t outgoing);
    int flush();
    int bufferedWrite(const uint8_t *data, size_t size);
    void poller(std::future<void> futureObj);
};

#endif