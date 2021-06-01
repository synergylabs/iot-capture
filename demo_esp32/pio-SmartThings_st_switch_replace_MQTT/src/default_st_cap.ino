#include <iostream>
#include <sstream>
#include <string.h>
#include "default_st_dev.h"
#include "default_iot_error.h"
#include "default_iot_capability.h"

#define MAX_SQNUM 0x7FFFFFFF

using std::string;

static int32_t sqnum = 0;

static void _iot_free_val(iot_cap_val_t *val);
static void _iot_free_unit(iot_cap_unit_t *unit);
static void _iot_free_cmd_data(iot_cap_cmd_data_t *cmd_data);
static void _iot_free_evt_data(iot_cap_evt_data_t *evt_data);
char *_st_cap_construct_payload(uint8_t evt_num, iot_cap_evt_data_t **evt_data);

template <typename T>
std::string to_string(T t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

IOT_EVENT *st_cap_attr_create_int(char *attribute, int integer, char *unit)
{
    iot_cap_evt_data_t *evt_data;

    if (!attribute)
    {
        IOT_ERROR("attribute is NULL");
        return NULL;
    }

    evt_data = (iot_cap_evt_data_t *)malloc(sizeof(iot_cap_evt_data_t));
    if (!evt_data)
    {
        IOT_ERROR("failed to malloc for evt_data");
        return NULL;
    }

    memset(evt_data, 0, sizeof(iot_cap_evt_data_t));
    evt_data->evt_type = strdup(attribute);
    evt_data->evt_value.type = IOT_CAP_VAL_TYPE_INTEGER;
    evt_data->evt_value.integer = integer;

    if (unit != NULL)
    {
        evt_data->evt_unit.type = IOT_CAP_UNIT_TYPE_STRING;
        evt_data->evt_unit.string = strdup(unit);
    }
    else
    {
        evt_data->evt_unit.type = IOT_CAP_UNIT_TYPE_UNUSED;
    }

    return (IOT_EVENT *)evt_data;
}

IOT_EVENT *st_cap_attr_create_number(char *attribute, double number, char *unit)
{
    iot_cap_evt_data_t *evt_data;

    if (!attribute)
    {
        IOT_ERROR("attribute is NULL");
        return NULL;
    }

    evt_data = (iot_cap_evt_data_t *)malloc(sizeof(iot_cap_evt_data_t));
    if (!evt_data)
    {
        IOT_ERROR("failed to malloc for evt_data");
        return NULL;
    }

    memset(evt_data, 0, sizeof(iot_cap_evt_data_t));
    evt_data->evt_type = strdup(attribute);
    evt_data->evt_value.type = IOT_CAP_VAL_TYPE_NUMBER;
    evt_data->evt_value.number = number;

    if (unit != NULL)
    {
        evt_data->evt_unit.type = IOT_CAP_UNIT_TYPE_STRING;
        evt_data->evt_unit.string = strdup(unit);
    }
    else
    {
        evt_data->evt_unit.type = IOT_CAP_UNIT_TYPE_UNUSED;
    }

    return (IOT_EVENT *)evt_data;
}

IOT_EVENT *st_cap_attr_create_string(char *attribute, char *string, char *unit)
{
    iot_cap_evt_data_t *evt_data;

    if (!attribute || !string)
    {
        IOT_ERROR("attribute or string is NULL");
        return NULL;
    }

    evt_data = (iot_cap_evt_data_t *)malloc(sizeof(iot_cap_evt_data_t));
    if (!evt_data)
    {
        IOT_ERROR("failed to malloc for evt_data");
        return NULL;
    }

    memset(evt_data, 0, sizeof(iot_cap_evt_data_t));
    evt_data->evt_type = strdup(attribute);
    evt_data->evt_value.type = IOT_CAP_VAL_TYPE_STRING;
    evt_data->evt_value.string = strdup(string);

    if (unit != NULL)
    {
        evt_data->evt_unit.type = IOT_CAP_UNIT_TYPE_STRING;
        evt_data->evt_unit.string = strdup(unit);
    }
    else
    {
        evt_data->evt_unit.type = IOT_CAP_UNIT_TYPE_UNUSED;
    }

    return (IOT_EVENT *)evt_data;
}

IOT_EVENT *st_cap_attr_create_string_array(char *attribute,
                                           uint8_t str_num, char *string_array[], char *unit) {}

void st_cap_attr_free(IOT_EVENT *event)
{
    iot_cap_evt_data_t *evt_data = (iot_cap_evt_data_t *)event;

    if (evt_data)
    {
        _iot_free_evt_data(evt_data);
        free(evt_data);
    }
}

int st_cap_attr_send(IOT_CAP_HANDLE *cap_handle,
                     uint8_t evt_num, IOT_EVENT *event[])
{
    iot_cap_evt_data_t **evt_data = (iot_cap_evt_data_t **)event;
    int ret;
    struct iot_context *ctx;
    struct iot_cap_handle *handle = (struct iot_cap_handle *)cap_handle;

    LOGV("Prepare to send...\n");

    if (!handle || !evt_data || !evt_num)
    {
        IOT_ERROR("There is no handle or evt_data");
        return IOT_ERROR_INVALID_ARGS;
    }

    ctx = handle->ctx;

    sqnum = (sqnum + 1) & MAX_SQNUM; // Use only positive number

    /* Make event data format & enqueue data */
    char *payload = _st_cap_construct_payload(evt_num, evt_data);

    LOGV("Payload is: %s\n", payload);

    client.publish(evt_data[0]->evt_type, payload);
    free(payload);
    return sqnum;
}

char *_st_cap_construct_payload(uint8_t evt_num, iot_cap_evt_data_t **evt_data)
{
    string buffer;

    for (int idx = 0; idx < evt_num; idx++)
    {
        if (idx > 0)
        {
            buffer += ";";
        }

        switch (evt_data[idx]->evt_value.type)
        {
        case IOT_CAP_VAL_TYPE_STRING:
            buffer += evt_data[idx]->evt_value.string;
            break;
        case IOT_CAP_VAL_TYPE_NUMBER:
            buffer += to_string(evt_data[idx]->evt_value.number);
            break;
        case IOT_CAP_VAL_TYPE_INTEGER:
            buffer += to_string(evt_data[idx]->evt_value.integer);
            break;
        default:
            IOT_ERROR("Not supported for event type payload.");
            break;
        }
    }
    LOGV("Buffer is: %s\n", buffer.c_str());

    size_t size = buffer.length() + 1;
    char *output = (char *) malloc(size);
    strncpy(output, buffer.c_str(), size);

    return output;
}

IOT_CAP_HANDLE *st_cap_handle_init(IOT_CTX *iot_ctx, const char *component,
                                   const char *capability, st_cap_init_cb init_cb, void *init_usr_data)
{
    struct iot_cap_handle *handle = nullptr;
    struct iot_cap_handle_list *cur_list;
    struct iot_cap_handle_list *new_list;
    struct iot_context *ctx = (struct iot_context *)iot_ctx;

    handle = (iot_cap_handle *)malloc(sizeof(struct iot_cap_handle));
    if (!handle)
    {
        IOT_ERROR("failed to malloc for iot_cap_handle");
        return nullptr;
    }

    memset(handle, 0, sizeof(struct iot_cap_handle));

    if (component)
    {
        handle->component = strdup(component);
    }
    else
    {
        handle->component = strdup("main");
    }
    if (!handle->component)
    {
        IOT_ERROR("failed to malloc for component");
        free(handle);
        return nullptr;
    }

    handle->capability = strdup(capability);
    if (!handle->capability)
    {
        IOT_ERROR("failed to malloc for capability");
        free((void *)handle->component);
        free(handle);
        return nullptr;
    }

    handle->cmd_list = nullptr;

    new_list = (iot_cap_handle_list_t *)malloc(sizeof(iot_cap_handle_list_t));
    if (!new_list)
    {
        IOT_ERROR("failed to malloc for handle list");
        free((void *)handle->component);
        free((void *)handle->capability);
        free(handle);
        return nullptr;
    }

    if (ctx->cap_handle_list == nullptr)
    {
        ctx->cap_handle_list = new_list;
        cur_list = ctx->cap_handle_list;
    }
    else
    {
        cur_list = ctx->cap_handle_list;
        while (cur_list->next != nullptr)
            cur_list = cur_list->next;
        cur_list->next = new_list;
        cur_list = cur_list->next;
    }
    cur_list->next = nullptr;
    cur_list->handle = handle;

    if (init_cb)
        handle->init_cb = init_cb;

    if (init_usr_data)
        handle->init_usr_data = init_usr_data;

    handle->ctx = ctx;
    return (IOT_CAP_HANDLE *)handle;
}

int st_cap_cmd_set_cb(IOT_CAP_HANDLE *cap_handle, const char *cmd_type,
                      st_cap_cmd_cb cmd_cb, void *usr_data)
{
    struct iot_cap_handle *handle = (struct iot_cap_handle *)cap_handle;
    struct iot_cap_cmd_set *command;
    struct iot_cap_cmd_set_list *cur_list;
    struct iot_cap_cmd_set_list *new_list;
    const char *needle_str, *cmd_str;
    size_t str_len;

    if (!handle || !cmd_type || !cmd_cb)
    {
        IOT_ERROR("There is no handle or cb data");
        return IOT_ERROR_INVALID_ARGS;
    }

    needle_str = cmd_type;
    str_len = strlen(needle_str);

    new_list = (iot_cap_cmd_set_list_t *)malloc(sizeof(iot_cap_cmd_set_list_t));
    if (!new_list)
    {
        IOT_ERROR("failed to malloc for cmd set list");
        return IOT_ERROR_MEM_ALLOC;
    }

    cur_list = handle->cmd_list;

    if (cur_list == NULL)
    {
        handle->cmd_list = new_list;
        cur_list = handle->cmd_list;
    }
    else
    {
        while (cur_list->next != NULL)
        {
            cmd_str = cur_list->command->cmd_type;
            if (cmd_str && !strncmp(cmd_str, needle_str, str_len))
            {
                IOT_ERROR("There is already same handle for : %s",
                          needle_str);
                return IOT_ERROR_INVALID_ARGS;
            }
            cur_list = cur_list->next;
        }

        cur_list->next = new_list;
        cur_list = cur_list->next;
    }

    command = (iot_cap_cmd_set_t *)malloc(sizeof(iot_cap_cmd_set_t));
    if (!command)
    {
        IOT_ERROR("failed to malloc for cmd set");
        return IOT_ERROR_MEM_ALLOC;
    }

    command->cmd_type = strdup(needle_str);
    command->cmd_cb = cmd_cb;
    command->usr_data = usr_data;

    cur_list->command = command;
    cur_list->next = NULL;

    return IOT_ERROR_NONE;
}

static void _iot_free_val(iot_cap_val_t *val)
{
    if (val == NULL)
    {
        return;
    }

    if (val->type == IOT_CAP_VAL_TYPE_STRING && val->string != NULL)
    {
        free(val->string);
    }
    else if (val->type == IOT_CAP_VAL_TYPE_STR_ARRAY && val->strings != NULL)
    {
        for (int i = 0; i < val->str_num; i++)
        {
            if (val->strings[i] != NULL)
            {
                free(val->strings[i]);
            }
        }
        free(val->strings);
    }
}

static void _iot_free_unit(iot_cap_unit_t *unit)
{
    if (unit == NULL)
    {
        return;
    }

    if (unit->type == IOT_CAP_UNIT_TYPE_STRING && unit->string != NULL)
    {
        free(unit->string);
    }
}

static void _iot_free_cmd_data(iot_cap_cmd_data_t *cmd_data)
{
    if (cmd_data == NULL)
    {
        return;
    }

    for (int i = 0; i < cmd_data->num_args; i++)
    {
        if (cmd_data->args_str[i] != NULL)
        {
            free(cmd_data->args_str[i]);
        }
        _iot_free_val(&cmd_data->cmd_data[i]);
    }
}

static void _iot_free_evt_data(iot_cap_evt_data_t *evt_data)
{
    if (evt_data == NULL)
    {
        return;
    }

    if (evt_data->evt_type != NULL)
    {
        free((void *)evt_data->evt_type);
    }
    _iot_free_val(&evt_data->evt_value);
    _iot_free_unit(&evt_data->evt_unit);
}
