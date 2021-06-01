#include "CaptureWiFiClient.h"
#include "libcapture_exception.h"
#include "esp_timer.h"

CaptureWiFiClient::CaptureWiFiClient() : CaptureWiFiClient(0)
{
}

CaptureWiFiClient::CaptureWiFiClient(const CaptureWiFiClient &ref) : CaptureWiFiClient(ref.client_id)
{
    LOGV("Copy constructor called!!\n\n");
}


CaptureWiFiClient::CaptureWiFiClient(uint8_t client_id)
{
    this->client_id = client_id;
    bufReader = nullptr;
    bufWriter = nullptr;
    if (client_id > 0 && USE_BUFFERED_RW) {
        bufReader = new CaptureBufferedReader(client_id);
        bufWriter = new CaptureBufferedWriter(client_id);
    }
    // If this is a valid new client, we set the client_connected cache to be valid.
    if (client_id > 0) {
        timestamp_connected = esp_timer_get_time();
        is_connected = true;
    }
}

CaptureWiFiClient::~CaptureWiFiClient()
{
    LOGV("\n\n\n\nCaptureWiFiClient::~CaptureWiFiClient() destructor is called!!\n\n\n\n");
    if (bufReader != nullptr)
    {
        delete bufReader;
    }
    if (bufWriter != nullptr)
    {
        delete bufWriter;
    }
}

int CaptureWiFiClient::connect(IPAddress ip, uint16_t port)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClient::connect(IPAddress ip, uint16_t port, int32_t timeout)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClient::connect(const char *host, uint16_t port)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClient::connect(const char *host, uint16_t port, int32_t timeout)
{
    throw NotYetImplementedException();
}

size_t CaptureWiFiClient::write(uint8_t data)
{
    return write(&data, 1);
}
size_t CaptureWiFiClient::write(const uint8_t *buf, size_t size)
{
    // Main network write routine.
    return CaptureWiFi.writeToDriver(this, buf, size);
}
size_t CaptureWiFiClient::write_P(PGM_P buf, size_t size)
{
    return write(buf, size);
}
size_t CaptureWiFiClient::write(Stream &stream)
{
    throw NotYetImplementedException();
}

// Return number of bytes can be read from this client
int CaptureWiFiClient::available()
{
    // TODO: communicate to driver to receive number of bytes to read
    return 1;
}
int CaptureWiFiClient::read()
{
    uint8_t data = 0;
    int res = read(&data, 1);
    if (res <= 0)
    {
        return res;
    }
    return data;
}
int CaptureWiFiClient::read(uint8_t *buf, size_t size)
{
     return CaptureWiFi.readFromDriver(this, buf, size);
}
int CaptureWiFiClient::peek()
{
    throw NotYetImplementedException();
}
void CaptureWiFiClient::flush()
{
    throw NotYetImplementedException();
}
void CaptureWiFiClient::stop()
{
    is_connected = 0;

    CaptureWiFi.closeClientConnection(this);
}
uint8_t CaptureWiFiClient::connected()
{
    // LOGV("CaptureWiFiClient::connected()\n");
    if (client_id == 0)
        return 0;
    
    if (!timestamp_connected) {
        timestamp_connected = esp_timer_get_time();
        is_connected = CaptureWiFi.checkClientConnected(this);
        return is_connected;
    }

    unsigned long long current_time = esp_timer_get_time();
    unsigned long long time_elapsed = (current_time - timestamp_connected) / (1000 * 1000);
    
    if (time_elapsed >= 2) {
        timestamp_connected = esp_timer_get_time();
        is_connected = CaptureWiFi.checkClientConnected(this);
    }

    return is_connected;
}

int CaptureWiFiClient::fd() const
{
    throw NotYetImplementedException();
}

int CaptureWiFiClient::setSocketOption(int option, char *value, size_t len)
{
    throw NotYetImplementedException();
}
int CaptureWiFiClient::setOption(int option, int *value)
{
    throw NotYetImplementedException();
}
int CaptureWiFiClient::getOption(int option, int *value)
{
    throw NotYetImplementedException();
}
int CaptureWiFiClient::setTimeout(uint32_t seconds)
{
    throw NotYetImplementedException();
}
int CaptureWiFiClient::setNoDelay(bool nodelay)
{
    throw NotYetImplementedException();
}
bool CaptureWiFiClient::getNoDelay()
{
    throw NotYetImplementedException();
}

IPAddress CaptureWiFiClient::remoteIP() const
{
    throw NotYetImplementedException();
}
IPAddress CaptureWiFiClient::remoteIP(int fd) const
{
    throw NotYetImplementedException();
}
uint16_t CaptureWiFiClient::remotePort() const
{
    throw NotYetImplementedException();
}
uint16_t CaptureWiFiClient::remotePort(int fd) const
{
    throw NotYetImplementedException();
}
IPAddress CaptureWiFiClient::localIP() const
{
    throw NotYetImplementedException();
}
IPAddress CaptureWiFiClient::localIP(int fd) const
{
    throw NotYetImplementedException();
}
uint16_t CaptureWiFiClient::localPort() const
{
    throw NotYetImplementedException();
}
uint16_t CaptureWiFiClient::localPort(int fd) const
{
    throw NotYetImplementedException();
}
