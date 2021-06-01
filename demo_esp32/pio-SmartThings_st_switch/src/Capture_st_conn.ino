#include "Capture_st_dev.h"
#include "capture_st_message.h"

#include "Capture_iot_main.h"
#include "Capture_iot_error.h"

struct iot_context *global_ctx = nullptr;
CustomCaptureWiFiClass CaptureWiFi;

void process_mqtt_message_from_driver(void *arg);
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

void callback(char *topic, char *message)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp = message;
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
                if (strcmp(cur_cmd_set->cmd_type, messageTemp.c_str()) == 0)
                {
                    LOGV("Found matching messageTemp %s\n", messageTemp.c_str());

                    iot_cap_cmd_data_t *cmd_data = (iot_cap_cmd_data_t *)malloc(sizeof(iot_cap_cmd_data_t));
                    cmd_data->num_args = 0;

                    cur_cmd_set->cmd_cb((IOT_CAP_HANDLE *)cur_handle, cmd_data, cur_cmd_set->usr_data);
                    free(cmd_data);
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
    LOGV("Try to connect WiFi...");
    CaptureWiFi.begin("TEST-DO-NOT-USE-PSK", "WiFiPassword");

    global_ctx = ctx;

    xTaskCreate(process_mqtt_message_from_driver, "process_mqtt_message_from_driver", 4096, nullptr, 20, NULL);

    // Initialize all MQTT capabilities
    _st_initialize_all_capabilities();
    _st_conn_subscribe_all_topics();

    return IOT_ERROR_NONE;
}

void process_mqtt_message_from_driver(void *arg)
{
    // Start a forever while loop to maintain connection to the driver
    while (true) {
        if (CaptureWiFi.driver_connection.available())
        {
            LOGV("Blocking on read from MQTT broker\n");
            capture_st_driver_message_t *msg = new capture_st_driver_message_t();
            if (CaptureWiFi.readFromCameraDriver(msg) == 0) {
                LOGV("Processing new message just read\n");
                if (msg->type == st_driver_msg_type_t::INCOMING_EVENT) {
                    callback(msg->topic, msg->message);
                } else {
                    LOGV("Unknown type %u\n", msg->type);
                }
            }
            LOGV("Done with message in read\n");
            delete msg;
        }
        sleep(2);
    }
}

void _st_initialize_all_capabilities()
{
    LOGV("Preprare to initialize all capabilities...\n");
    struct iot_cap_handle_list *cur_handle_list = global_ctx->cap_handle_list;
    while (cur_handle_list != nullptr)
    {
        struct iot_cap_handle *cur_handle = cur_handle_list->handle;

        if (cur_handle->init_cb)
            cur_handle->init_cb((IOT_CAP_HANDLE *)cur_handle, cur_handle->init_usr_data);

        cur_handle_list = cur_handle_list->next;
    }
    LOGV("Finish initializing capabilities...\n");
}

void _st_conn_subscribe_all_topics()
{
    struct iot_cap_handle_list *cur_handle_list = global_ctx->cap_handle_list;
    while (cur_handle_list != nullptr)
    {
        struct iot_cap_handle *cur_handle = cur_handle_list->handle;

        // TODO
        // client.subscribe(cur_handle->capability);
        CaptureWiFi.sendSubscribeMessage(cur_handle->capability, strlen(cur_handle->capability));

        cur_handle_list = cur_handle_list->next;
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
