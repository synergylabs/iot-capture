#include "CaptureWiFiClientSecure.h"
#include "libcapture_exception.h"

CaptureWiFiClientSecure::CaptureWiFiClientSecure()
{
    _connected = false;

    sslclient = new sslclient_context;
    ssl_init(sslclient);
    sslclient->socket = -1;
    sslclient->handshake_timeout = 120000;
    _CA_cert = NULL;
    _cert = NULL;
    _private_key = NULL;
    _pskIdent = NULL;
    _psKey = NULL;
    next = NULL;
}

CaptureWiFiClientSecure::CaptureWiFiClientSecure(int socket)
{
    throw NotYetImplementedException();
}

CaptureWiFiClientSecure::~CaptureWiFiClientSecure()
{
    stop();
    delete sslclient;
}

CaptureWiFiClientSecure &CaptureWiFiClientSecure::operator=(const CaptureWiFiClientSecure &other)
{
    stop();
    sslclient->socket = other.sslclient->socket;
    _connected = other._connected;
    return *this;
}

void CaptureWiFiClientSecure::stop()
{
    if (sslclient->socket >= 0)
    {
        close(sslclient->socket);
        sslclient->socket = -1;
        _connected = false;
        _peek = -1;
    }
    stop_ssl_socket(sslclient, _CA_cert, _cert, _private_key);
}

int CaptureWiFiClientSecure::connect(IPAddress ip, uint16_t port)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::connect(IPAddress ip, uint16_t port, int32_t timeout)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::connect(const char *host, uint16_t port)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::connect(const char *host, uint16_t port, int32_t timeout)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::connect(IPAddress ip, uint16_t port, const char *_CA_cert, const char *_cert, const char *_private_key)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::connect(const char *host, uint16_t port, const char *_CA_cert, const char *_cert, const char *_private_key)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::connect(IPAddress ip, uint16_t port, const char *pskIdent, const char *psKey)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::connect(const char *host, uint16_t port, const char *pskIdent, const char *psKey)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::peek()
{
    throw NotYetImplementedException();
}

size_t CaptureWiFiClientSecure::write(uint8_t data)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::read()
{
    throw NotYetImplementedException();
}

size_t CaptureWiFiClientSecure::write(const uint8_t *buf, size_t size)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::read(uint8_t *buf, size_t size)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::available()
{
    throw NotYetImplementedException();
}

uint8_t CaptureWiFiClientSecure::connected()
{
    throw NotYetImplementedException();
}

void CaptureWiFiClientSecure::setCACert(const char *rootCA)
{
    _CA_cert = rootCA;
}

void CaptureWiFiClientSecure::setCertificate(const char *client_ca)
{
    throw NotYetImplementedException();
}

void CaptureWiFiClientSecure::setPrivateKey(const char *private_key)
{
    throw NotYetImplementedException();
}

void CaptureWiFiClientSecure::setPreSharedKey(const char *pskIdent, const char *psKey)
{
    throw NotYetImplementedException();
}

bool CaptureWiFiClientSecure::verify(const char *fp, const char *domain_name)
{
    throw NotYetImplementedException();
}

char *CaptureWiFiClientSecure::_streamLoad(Stream &stream, size_t size)
{
    throw NotYetImplementedException();
}

bool CaptureWiFiClientSecure::loadCACert(Stream &stream, size_t size)
{
    throw NotYetImplementedException();
}

bool CaptureWiFiClientSecure::loadCertificate(Stream &stream, size_t size)
{
    throw NotYetImplementedException();
}

bool CaptureWiFiClientSecure::loadPrivateKey(Stream &stream, size_t size)
{
    throw NotYetImplementedException();
}

int CaptureWiFiClientSecure::lastError(char *buf, const size_t size)
{
    throw NotYetImplementedException();
}

void CaptureWiFiClientSecure::setHandshakeTimeout(unsigned long handshake_timeout)
{
    throw NotYetImplementedException();
}
