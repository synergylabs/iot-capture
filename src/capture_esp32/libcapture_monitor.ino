// Establish connection to central Capture monitor hub
#include "libcapture.h"
#include "libcapture_globals.h"
#include "libcapture_monitor_message.h"

/*
   Send out discovery message to find monitor platform in the LAN
*/
int Capture::discover_monitor()
{
  // TODO
  monitor_addr.assign(MONITOR_ADDR);
  monitor_port = MONITOR_PORT;

  return 0;
}

/*
   Establish connection to monitor platform
   Return 0 on success
*/
int Capture::connect_monitor()
{
  if (!wifi_client.connect(monitor_addr.c_str(), monitor_port))
  {
    LOGV("Connection to monitor failed\n");
    return -1;
  }
  LOGV("Successfully connected to monitor hub\n");
  return 0;
}

/*
   Receive credentials for WPA-EAP network
*/
int Capture::receive_credentials(const char *driver)
{
  capture_monitor_msg_t *msg = new capture_monitor_msg_t();
  new_eap_request_msg(msg, driver);
  char *buf = nullptr;
  size_t buf_len = 0;

  msg->generate_network_msg(&buf, &buf_len);
  LOGV("buf_len: %u\n", buf_len);
  network_write(buf, buf_len);

  if (eap_username != nullptr || eap_password != nullptr)
  {
    LOGV("EAP network credentials already received. Exit");
    return 0;
  }

  eap_username = new char[MAX_BUFFER_LEN];
  memset(eap_username, 0, MAX_BUFFER_LEN);
  eap_password = new char[MAX_BUFFER_LEN];
  memset(eap_password, 0, MAX_BUFFER_LEN);

  msg = new capture_monitor_msg_t();
  LOGV("size: %d, sizeof MSG_EAP_CREDS: %d\n", sizeof(msg->msg_type), sizeof(MSG_EAP_CREDS));

  LOGV("Waiting for new credentials\n");

  network_read((uint8_t *)&(msg->msg_type), sizeof(msg->msg_type));
  LOGV("before casting msg_type: %d, expected: %d\n", msg->msg_type, MSG_EAP_CREDS);

  msg->msg_type = (monitor_msg_type_t)ntohl(msg->msg_type);

  LOGV("received msg_type: %d, expected: %d\n", msg->msg_type, MSG_EAP_CREDS);
  if (msg->msg_type != MSG_EAP_CREDS) {
    return -1;
  }
    
  payload_eap_creds_t *payload_eap = new payload_eap_creds_t();

  network_read((uint8_t *)&payload_eap->driver_port, sizeof(payload_eap->driver_port));
  LOGV("before conversion driver port number as ... %u\n", payload_eap->driver_port);
  payload_eap->driver_port = ntohl(payload_eap->driver_port);
  LOGV("READING driver port number as ... %u\n", payload_eap->driver_port);
  eap_driver_port = payload_eap->driver_port;

  network_read((uint8_t *)&payload_eap->username_len, sizeof(payload_eap->username_len));
  payload_eap->username_len = ntohl(payload_eap->username_len);
  network_read((uint8_t *)&payload_eap->password_len, sizeof(payload_eap->password_len));
  payload_eap->password_len = ntohl(payload_eap->password_len);

  assert(payload_eap->username_len < MAX_BUFFER_LEN && payload_eap->password_len < MAX_BUFFER_LEN);

  network_read((uint8_t *)eap_username, payload_eap->username_len);
  network_read((uint8_t *)eap_password, payload_eap->password_len);

  msg->payload.eap_cred = payload_eap;

  LOGV("New credentials are %s %s\n", eap_username, eap_password);

  return 0;
}
