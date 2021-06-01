#include <WiFi.h> // ESP32 WiFi include

#include "default_iot_main.h"
#include "default_iot_error.h"

const char *mqtt_server = "192.168.11.1";
struct iot_context *global_ctx = nullptr;
WiFiClient espClient;
PubSubClient client(espClient);

static int _connect_psk(const char *ssid, const char *password);
void _st_conn_subscribe_all_topics();
void _st_initialize_all_capabilities();

IOT_CTX *st_conn_init(unsigned char *onboarding_config, unsigned int onboarding_config_len,
                      unsigned char *device_info, unsigned int device_info_len)
{
    struct iot_context *ctx = (iot_context *)malloc(sizeof(struct iot_context));

    /* Initialize all values */
    memset(ctx, 0, sizeof(struct iot_context));

    return (IOT_CTX *)ctx;
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    struct iot_cap_handle_list *cur_handle_list = global_ctx->cap_handle_list;
    while (cur_handle_list != nullptr)
    {
        struct iot_cap_handle *cur_handle = cur_handle_list->handle;

        if (strcmp(cur_handle->capability, topic) == 0)
        {
            LOGV("Found matching topic!\n");

            struct iot_cap_cmd_set_list *cur_cmd_list = cur_handle->cmd_list;
            while (cur_cmd_list != nullptr)
            {
                struct iot_cap_cmd_set *cur_cmd_set = cur_cmd_list->command;
                if (strcmp(cur_cmd_set->cmd_type, messageTemp.c_str()) == 0) {
                    LOGV("Found matching messageTemp %s\n", messageTemp.c_str());

                    iot_cap_cmd_data_t *cmd_data = (iot_cap_cmd_data_t *)malloc(sizeof(iot_cap_cmd_data_t));
                    cmd_data->num_args = 0;

                    cur_cmd_set->cmd_cb((IOT_CAP_HANDLE *) cur_handle, cmd_data, cur_cmd_set->usr_data);
                    break;
                }

                cur_cmd_list = cur_cmd_list->next;
            }

            break;
        }

        cur_handle_list = cur_handle_list->next;
    }
}

int st_conn_start(IOT_CTX *iot_ctx, st_status_cb status_cb,
                  iot_status_t maps, void *usr_data, iot_pin_t *pin_num)
{
    // struct iot_state_data state_data;
    iot_error_t iot_err;
    struct iot_context *ctx = (struct iot_context *)iot_ctx;

    if (!ctx)
        return IOT_ERROR_BAD_REQ;

    // Connect WiFi
    _connect_psk("TEST-DO-NOT-USE-PSK", "WiFiPassword");

    global_ctx = ctx;

    // Connect MQTT broker
    client.setServer(mqtt_server, 1883);
    // Set up MQTT callback function
    client.setCallback(callback);

    // Initialize all MQTT capabilities
    st_conn_mqtt_reconnect();
    _st_initialize_all_capabilities();

    return IOT_ERROR_NONE;
}

void _st_initialize_all_capabilities()
{
    struct iot_cap_handle_list *cur_handle_list = global_ctx->cap_handle_list;
    while (cur_handle_list != nullptr)
    {
        struct iot_cap_handle *cur_handle = cur_handle_list->handle;

        if (cur_handle->init_cb)
            cur_handle->init_cb((IOT_CAP_HANDLE *)cur_handle, cur_handle->init_usr_data);

        cur_handle_list = cur_handle_list->next;
    }
}

void _st_conn_subscribe_all_topics()
{
    struct iot_cap_handle_list *cur_handle_list = global_ctx->cap_handle_list;
    while (cur_handle_list != nullptr)
    {
        struct iot_cap_handle *cur_handle = cur_handle_list->handle;

        client.subscribe(cur_handle->capability);

        cur_handle_list = cur_handle_list->next;
    }
}

void st_conn_mqtt_reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client"))
        {
            Serial.println("connected");
            // Subscribe to MQTT topics
            // TODO
            client.subscribe("test");
            _st_conn_subscribe_all_topics();
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

int st_conn_cleanup(IOT_CTX *iot_ctx, bool reboot)
{
    return IOT_ERROR_NONE;
}

void st_conn_ownership_confirm(IOT_CTX *iot_ctx, bool confirm)
{
    return;
}

int st_conn_set_noti_cb(IOT_CTX *iot_ctx,
                        st_cap_noti_cb noti_cb, void *noti_usr_data)
{
    struct iot_context *ctx = (struct iot_context *)iot_ctx;

    if (!ctx || !noti_cb)
    {
        IOT_ERROR("There is no ctx or cb !!!");
        return IOT_ERROR_INVALID_ARGS;
    }

    ctx->noti_cb = noti_cb;

    if (noti_usr_data)
        ctx->noti_usr_data = noti_usr_data;

    return IOT_ERROR_NONE;
}

/*
   Connect this device to PSK network to obtain internet access
*/
static int _connect_psk(const char *ssid, const char *password)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to ");
    Serial.println(ssid);
    Serial.print("Password:");
    Serial.println(password);

    uint8_t i = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);

        if ((++i % 16) == 0)
        {
            Serial.println(F(" still trying to connect"));
        }
    }

    Serial.print(F("Connected. My IP address is: "));
    Serial.println(WiFi.localIP());
    return 0;
}
