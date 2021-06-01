#ifndef CAPTURE_CORE_CAPTURE_WIFI_CLIENT_H
#define CAPTURE_CORE_CAPTURE_WIFI_CLIENT_H

#include "BufferedRw.h"

class CaptureWiFiClient : public Client
{
private:
    bool is_connected = false;
    uint8_t client_id = 0;
    CaptureBufferedReader *bufReader = nullptr;
    CaptureBufferedWriter *bufWriter = nullptr;
    unsigned long long timestamp_connected = 0;

    uint8_t get_client_id() 
    {
        return client_id;
    }

    CaptureBufferedReader *get_read_buffer() 
    {
        return bufReader;
    }

    CaptureBufferedWriter *get_write_buffer()
    {
        return bufWriter;
    }

public:
    friend class CaptureWiFiClass;

    CaptureWiFiClient *next;
    CaptureWiFiClient();
    CaptureWiFiClient(uint8_t client_id);
    CaptureWiFiClient(const CaptureWiFiClient &ref);
    ~CaptureWiFiClient();

    // virtual functions from base class
    int connect(IPAddress ip, uint16_t port);
    int connect(IPAddress ip, uint16_t port, int32_t timeout);
    int connect(const char *host, uint16_t port);
    int connect(const char *host, uint16_t port, int32_t timeout);
    size_t write(uint8_t data);
    size_t write(const uint8_t *buf, size_t size);
    size_t write_P(PGM_P buf, size_t size);
    size_t write(Stream &stream);
    int available();
    int read();
    int read(uint8_t *buf, size_t size);
    int peek();
    void flush();
    void stop();
    uint8_t connected();

    operator bool()
    {
        return connected();
    }
    CaptureWiFiClient &operator=(const CaptureWiFiClient &other);
    bool operator==(const bool value)
    {
        return bool() == value;
    }
    bool operator!=(const bool value)
    {
        return bool() != value;
    }
    bool operator==(const CaptureWiFiClient &);
    bool operator!=(const CaptureWiFiClient &rhs)
    {
        return !this->operator==(rhs);
    };

    int fd() const;

    int setSocketOption(int option, char *value, size_t len);
    int setOption(int option, int *value);
    int getOption(int option, int *value);
    int setTimeout(uint32_t seconds);
    int setNoDelay(bool nodelay);
    bool getNoDelay();

    IPAddress remoteIP() const;
    IPAddress remoteIP(int fd) const;
    uint16_t remotePort() const;
    uint16_t remotePort(int fd) const;
    IPAddress localIP() const;
    IPAddress localIP(int fd) const;
    uint16_t localPort() const;
    uint16_t localPort(int fd) const;

    using Print::write;
};

#endif