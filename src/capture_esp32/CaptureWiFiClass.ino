#include "CaptureWiFiClass.h"
#include "libcapture_driver_message.h"
#include "libcapture_mutex.h"
#include "libcapture_exception.h"
#include "libcapture_globals.h"

using std::string;

CaptureWiFiClass::CaptureWiFiClass() : CaptureWiFiClass(USE_DEFAULT_DRIVER)
{
}

CaptureWiFiClass::CaptureWiFiClass(const char *driver_name)
{
    mutex_wifi_class = xSemaphoreCreateMutex();
    state = WL_DISCONNECTED;
    driver = new char[strlen(driver_name) + 1];
    strcpy(driver, driver_name);
}

CaptureWiFiClass::~CaptureWiFiClass()
{
    delete[] driver;
}

void CaptureWiFiClass::begin(const char *ssid, const char *password)
{
    capture.connect_psk(ssid, password);
    capture.discover_monitor();
    capture.connect_monitor();
    capture.receive_credentials(driver);
    capture.connect_eap();
    lock_mutex(mutex_wifi_class);
    state = WL_CONNECTED;
    unlock_mutex(mutex_wifi_class);

    Serial.println("Driver IP is...");
    Serial.println(capture.driver_ip());
    Serial.println("Driver port is...");
    Serial.println(capture.driver_port());

    // connect to the device driver
    while (!driver_connection.connect(capture.driver_ip(), capture.driver_port()))
    {
        Serial.println("Connection failed.");
        Serial.println("Waiting 2 seconds before retrying...");
        delay(2000);
    }
    Serial.println("Connected to device driver.");
}

int CaptureWiFiClass::status()
{
    lock_mutex(mutex_wifi_class);
    int res = state;
    unlock_mutex(mutex_wifi_class);
    return res;
}

// This should return remote driver's publicly accessible IP,
// because people will connect to the webserver with *this* IP address.
char *CaptureWiFiClass::localIP()
{
    throw NotYetImplementedException();
}

CaptureWiFiClient *CaptureWiFiClass::getNewClient()
{
    LOGV("CaptureWiFiClass::getNewClient()\n");
    LOGV("1\n");
    capture_driver_message_t get_client_id_msg = capture_driver_message_t();
    get_client_id_msg.set_type(driver_msg_type_t::GET_NEW_CLIENT);
    get_client_id_msg.write_message_to_driver(driver_connection, capture);
    LOGV("2\n");

    capture_driver_message_t reply = capture_driver_message_t();
    reply.read_message_from_driver(driver_connection, capture);
    LOGV("3\n");

    CaptureWiFiClient *client = nullptr;
    LOGV("4\n");

    // TODO: process piggyback message explilcitly.
    if (reply.get_client_id() > 0)
    {
        LOGV("5\n");
        client = new CaptureWiFiClient(reply.get_client_id());
        LOGV("6\n");
        // Valid client. Process piggyback message
        reply.process_piggyback_message(client->get_read_buffer());
        LOGV("7\n");
    }

    return client;
}

uint8_t CaptureWiFiClass::checkClientConnected(CaptureWiFiClient *client)
{
    LOGV("CaptureWiFiClass::checkClientConnected()");
    capture_driver_message_t get_client_status_msg = capture_driver_message_t(client->get_client_id());
    get_client_status_msg.set_type(driver_msg_type_t::CHECK_STATUS);
    get_client_status_msg.write_message_to_driver(driver_connection, capture);

    capture_driver_message_t reply = capture_driver_message_t();
    reply.read_message_from_driver(driver_connection, capture);

    uint32_t buf_len = reply.get_payload_len();
    if (buf_len < 1)
    {
        // error??
        LOGV("error: receive 0 payload\n");
        return 0;
    }

    uint8_t status = 0;
    uint8_t *buf = new uint8_t[buf_len];
    reply.get_payload(buf, buf_len);
    status = buf[0];
    delete[] buf;

    return status;
}

uint8_t CaptureWiFiClass::closeClientConnection(CaptureWiFiClient *client)
{
    // flush out pending messages
    if (client->bufWriter != nullptr)
    {
        lock_mutex(client->bufWriter->flush_mutex);
    }

    if (client->bufWriter != nullptr && !(client->bufWriter->is_empty()))
    {
        client->bufWriter->flush(true);
    }
    else
    {
        capture_driver_message_t close_client_msg = capture_driver_message_t(client->get_client_id());
        close_client_msg.set_type(driver_msg_type_t::CLOSE_CLIENT);
        close_client_msg.write_message_to_driver(driver_connection, capture);
    }

    if (client->bufWriter != nullptr)
    {
        unlock_mutex(client->bufWriter->flush_mutex);
    }
    return 0;
}

int CaptureWiFiClass::readFromDriver(CaptureWiFiClient *client, uint8_t *buf, size_t size)
{
    // LOGV("Reading %u bytes from driver, either buffer or direct\n", size);
    if (client->bufReader != nullptr)
    {
        size_t load_size = client->bufReader->read_from_buffer(buf, size);
        if (load_size > 0)
        {
            return load_size;
        }

        load_size = client->bufReader->load_buffer_from_network_driver(driver_connection, capture);
        if (load_size >= size)
        {
            load_size = client->bufReader->read_from_buffer(buf, size);
        }
        else
        {
            load_size = 0;
        }
        return load_size;
    }
    else
    {
        capture_driver_message_t read_data_msg = capture_driver_message_t(client->get_client_id());
        read_data_msg.set_type(driver_msg_type_t::INGRESS);
        read_data_msg.set_payload_len(size);
        read_data_msg.write_message_to_driver(driver_connection, capture);

        capture_driver_message_t reply = capture_driver_message_t();
        reply.read_message_from_driver(driver_connection, capture);

        size = reply.get_payload_len();
        LOGV("CaptureWiFiClass::readFromDriver reply payload size: %u\n", size);
        return size;
    }
}

int CaptureWiFiClass::writeToDriver(CaptureWiFiClient *client, const uint8_t *buf, size_t size)
{
    if (client->bufWriter != nullptr)
    {
        return client->bufWriter->buffered_write(&driver_connection, &capture, buf, size);
    }
    else
    {
        LOGV("Writing to driver... payload size is %d\n", size);
        capture_driver_message_t write_data_msg = capture_driver_message_t(client->get_client_id());
        write_data_msg.set_type(driver_msg_type_t::EGRESS);
        write_data_msg.set_payload(buf, size);
        write_data_msg.write_message_to_driver(driver_connection, capture);
        LOGV("Finish writing to driver\n");
        return 0;
    }
}
