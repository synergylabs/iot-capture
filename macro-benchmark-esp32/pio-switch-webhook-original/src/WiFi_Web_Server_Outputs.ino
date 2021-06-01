/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Replace with your network credentials
const char *ssid = "WIFINAME";
const char *password = "WIFIPASSWORD";

String syn_bd_host = "<server-name>";
String syn_bd_uri = "/command/";
const int https_port = 443;

String local_controller_address = "192.168.5.250";
const int local_controller_port = 54321;

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;

void flipSwitch()
{
  if (output26State == "on")
  {
    output26State = "off";
    digitalWrite(output26, LOW);
  }
  else
  {
    output26State = "on";
    digitalWrite(output26, HIGH);
  }
}

void getCommand(WiFiClientSecure *client)
{
  int64_t xLastWakeTime = esp_timer_get_time();
  Serial.printf("[Start] Current tick count: %d\n", xLastWakeTime);

  if (!client->connect(syn_bd_host.c_str(), https_port))
  {
    Serial.println("Connection to syn-bd failed.");
    return;
  }

  String getRequest =
      "GET " + syn_bd_uri + " HTTP/1.1\r\n" +
      "Host: " + syn_bd_host + "\r\n" +
      "Connection: close\r\n" +
      "\r\n";
  client->print(getRequest);

  String res;
  while (client->connected())
  {
    String line = client->readStringUntil('\n');
    if (line == "\r")
    {
      break;
    }
  }
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client->available())
  {
    char c = client->read();
    res = res + c;
  }

  client->stop();

  int result = atoi(res.c_str());
  if (result > 0)
  {
    // Send notification to the benchmark controller
    flipSwitch();
    sendNotification();
  }

  xLastWakeTime = esp_timer_get_time();
  Serial.printf("[End] Current tick count: %d\n", xLastWakeTime);
}

void sendNotification() 
{
  WiFiClient notify_server;
  notify_server.connect(local_controller_address.c_str(), local_controller_port);

  notify_server.write("7");
}

void setup()
{
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

const char *root_ca_syn_bd =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n"
    "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
    "DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n"
    "PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n"
    "Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
    "AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n"
    "rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n"
    "OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n"
    "xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n"
    "7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n"
    "aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n"
    "HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n"
    "SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n"
    "ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n"
    "AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n"
    "R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n"
    "JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n"
    "Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n"
    "-----END CERTIFICATE-----\n";

void loop()
{
  // Set up HTTPS connection
  WiFiClientSecure syn_bd_conn;

  syn_bd_conn.setCACert(root_ca_syn_bd);

  getCommand(&syn_bd_conn);

  // delay(100);
}
