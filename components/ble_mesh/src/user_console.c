/**
 * @file user_console.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "user_console.h"
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "esp_console.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"

#include "ble_mesh_process.h"
#include "ble_mesh.h"

#define USER_TAG "user_controller"

void user_console_register_command(void);

static void register_free(void);
static void ble_mesh_command(void);

static void example_ble_mesh_send_gen_onoff_set(esp_ble_mesh_model_t *model, uint16_t app_idx, uint16_t net_idx, uint16_t addr, uint8_t status, uint8_t transaction_id)
{
    ESP_LOGI(USER_TAG, "example_ble_mesh_send_gen_onoff_set net index = 0x%04x --- app index = 0x%04x --- addr = 0x%04x", net_idx, app_idx, addr);

    esp_ble_mesh_generic_client_set_state_t set = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_err_t err = ESP_OK;

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET;
    common.model = model;
    common.ctx.net_idx = net_idx;
    common.ctx.app_idx = app_idx;
    common.ctx.addr = addr; /* to all nodes */
    common.ctx.send_ttl = 3;
    common.ctx.send_rel = false;
    common.msg_timeout = 0;             /* 0 indicates that timeout value from menuconfig will be used */
    common.msg_role = ROLE_PROVISIONER; // ROLE_NODE;

    set.onoff_set.op_en = false;
    set.onoff_set.onoff = status;
    set.onoff_set.tid = transaction_id;

    err = esp_ble_mesh_generic_client_set_state(&common, &set);
    if (err)
    {
        ESP_LOGE(USER_TAG, "Send Generic OnOff Set Unack failed");
        return;
    }
}

/**
 * @brief
 *
 */
void user_console_init(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ble_mesh_console", ESP_LOG_INFO);

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    // init console REPL environment
    repl_config.max_history_len = 1;
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    // register console
    user_console_register_command();
    printf("!!!ready!!!\n");
    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

/** 'free' command prints available heap memory */

static int free_mem(int argc, char **argv)
{
    printf("freeheap:%d\n", esp_get_free_heap_size());
    return 0;
}

static int scan(int argc, char **argv)
{
    printf("scan provision enable\r\n");
    int err = esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK)
    {
        printf("ERR Failed to enable mesh provisioner (err %d)\r\n", err);
        return err;
    }

    return 0;
}

static int stop(int argc, char **argv)
{
    printf("stop provision disable\r\n");

    int err = esp_ble_mesh_provisioner_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK)
    {
        printf("ERR Failed to enable mesh provisioner (err %d)\r\n", err);
        return err;
    }

    return 0;
}

typedef struct
{
    struct arg_int *unicast_addr;
    struct arg_int *status;
    struct arg_end *end;
} be_mesh_messange_t;

typedef struct
{
    struct arg_int *index;
    struct arg_end *end;
} be_mesh_prov_t;

typedef struct
{
    struct arg_int *appkey_index;
    struct arg_int *element_address;
    struct arg_int *network_index;
    struct arg_int *mod_id;
    struct arg_int *cid;
    struct arg_end *end;
} ble_mesh_provisioner_bind_model_t;

ble_mesh_provisioner_bind_model_t provisioner_local_bind;

be_mesh_messange_t ble_message_message;
be_mesh_prov_t be_mesh_prov;
be_mesh_prov_t be_mesh_read;

// test control function on/off
extern int ble_mesh_send_gen_onoff_set(uint8_t state, uint16_t addr);
/**
 * @brief command ble_send -u 0x19 -s 1
 *
 * @param argc
 * @param argv
 * @return int
 */
static int ble_control_on_off(int argc, char **argv)
{
    printf("send message \r\n");
    uint16_t element_addr = 0;
    uint16_t status = 0;
    int nerrors = arg_parse(argc, argv, (void **)&ble_message_message);
    if (nerrors != 0)
    {
        printf("ERR Invalid\r\n");
        // arg_print_errors(stderr, ble_message_message.end, argv[0]);
        return 1;
    }

    arg_int_to_value(ble_message_message.unicast_addr, element_addr, "element set status");
    arg_int_to_value(ble_message_message.status, status, "element address");
    printf("send message to address: %x with status: %d\r\n", element_addr, status);
    ble_mesh_send_gen_onoff_set(status, element_addr);
    return 0;
}

/**
 * @brief command ble_prov -u 0
 * ble_read -u 3
 *
 * @param argc
 * @param argv
 * @return int
 */
static int ble_prov(int argc, char **argv)
{

    uint8_t index = 0;
    int nerrors = arg_parse(argc, argv, (void **)&be_mesh_prov);
    if (nerrors != 0)
    {
        printf("ERR Invalid\r\n");
        // arg_print_errors(stderr, ble_message_message.end, argv[0]);
        return 1;
    }

    arg_int_to_value(be_mesh_prov.index, index, "element set status");
    printf("provion with index %d \r\n", index);
    ble_mesh_provision_device_index(index);
    return 0;
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
static int ble_prov_enable(int argc, char **argv)
{
    uint8_t index = 0;
    int nerrors = arg_parse(argc, argv, (void **)&be_mesh_prov);
    if (nerrors != 0)
    {
        printf("ERR Invalid\r\n");
        // arg_print_errors(stderr, ble_message_message.end, argv[0]);
        return 1;
    }

    arg_int_to_value(be_mesh_prov.index, index, "element set status");
    printf("ble prov enable %d \r\n", index);
    ble_mesh_provisioner_prov_enable(index);
    return 0;
}
/**
 * @brief command ble_prov -u 0
 * ble_read -u 3
 *
 * @param argc
 * @param argv
 * @return int
 */
static int ble_show_proved_number(void)
{
    // printf("number in list unprov: %d \r\n", ble_mesh_provision_device_get_num_devices());
    ble_mesh_provision_device_show_devices();
    return 0;
}

/**
 * @brief command ble_prov -u 0
 * ble_read -u 3
 *
 * @param argc
 * @param argv
 * @return int
 */
static int ble_show_unprov_number(void)
{
    printf("number in list unprov: %d \r\n", ble_mesh_provision_device_get_num_devices());
    return 0;
}

extern void ble_mesh_send_sensor_message(uint32_t opcode, uint16_t addr);


static int ble_read(int argc, char **argv)
{
    printf("send request read \r\n");
    uint8_t value = 0;

    int nerrors = arg_parse(argc, argv, (void **)&be_mesh_prov);
    if (nerrors != 0)
    {
        printf("ERR Invalid\r\n");
        // arg_print_errors(stderr, ble_message_message.end, argv[0]);
        return 1;
    }

    arg_int_to_value(be_mesh_prov.index, value, "element set status");
    printf("read with value %d \r\n", value);

    ble_mesh_send_sensor_message(ESP_BLE_MESH_MODEL_OP_SENSOR_GET, value);
    return 0;
}


static int ble_binding(int argc, char **argv)
{
    esp_err_t err;
    uint16_t element_addr = 0;
    uint16_t app_idx = 0;
    uint16_t model_id = 0;
    uint16_t company_id = 0xFFFF;

    printf(" enter %s\n", __func__);

    int nerrors = arg_parse(argc, argv, (void **)&provisioner_local_bind);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, provisioner_local_bind.end, argv[0]);
        return 1;
    }

    arg_int_to_value(provisioner_local_bind.element_address, element_addr, "element address");
    arg_int_to_value(provisioner_local_bind.appkey_index, app_idx, "appkey index");
    arg_int_to_value(provisioner_local_bind.mod_id, model_id, "model id");
    arg_int_to_value(provisioner_local_bind.cid, company_id, "company id");
    err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(element_addr, app_idx, model_id, company_id);

    if (err != ESP_OK)
    {
        printf("Provisioner:BindModel,Fail,%d\n", err);
    }
    else
    {
        printf("Provisioner:BindModel,OK\n");
    }
    printf("exit %s\n", __func__);
    return err;
}
void user_console_register_command(void)
{
    register_free();
    ble_mesh_command();
}

static void register_free(void)
{
    const esp_console_cmd_t cmd = {
        .command = "free",
        .help = "Get the total size of heap memory available",
        .hint = NULL,
        .func = &free_mem,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void ble_mesh_command(void)
{
    const esp_console_cmd_t cmd_scan = {
        .command = "ble_scan",
        .help = "start scanning ble mesh",
        .hint = NULL,
        .func = &scan,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_scan));

    const esp_console_cmd_t cmd_stop = {
        .command = "ble_stop",
        .help = "stop scanning ble mesh",
        .hint = NULL,
        .func = &stop,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_stop));

    ble_message_message.unicast_addr = arg_int1("u", NULL, "<unicast address>", "unicast address");
    ble_message_message.status = arg_int1("s", NULL, "<status>", "mesage status");

    ble_message_message.end = arg_end(1);

    const esp_console_cmd_t cmd_send = {
        .command = "ble_control",
        .help = "control message to ble mesh",
        .hint = NULL,
        .func = &ble_control_on_off,
        .argtable = &ble_message_message,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_send));

    be_mesh_read.index = arg_int1("u", NULL, "<value>", "value");
    be_mesh_read.end = arg_end(1);

    const esp_console_cmd_t cmd_request = {
        .command = "ble_read",
        .help = "read message to ble mesh",
        .hint = NULL,
        .func = &ble_read,
        .argtable = &be_mesh_read,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_request));

    be_mesh_prov.index = arg_int1("u", NULL, "<index>", "index");
    be_mesh_prov.end = arg_end(1);

    const esp_console_cmd_t cmd_prov = {
        .command = "ble_prov",
        .help = "provision device with index",
        .hint = NULL,
        .func = &ble_prov,
        .argtable = &be_mesh_prov,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_prov));

    const esp_console_cmd_t cmd_show_number_unprov = {
        .command = "ble_unprov_number",
        .help = "show the number of unprov devices",
        .hint = NULL,
        .func = &ble_show_unprov_number,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_show_number_unprov));

    const esp_console_cmd_t cmd_show_number_prov = {
        .command = "ble_prov_number",
        .help = "show the number of devices provisioned by the provider",
        .hint = NULL,
        .func = &ble_show_proved_number,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_show_number_prov));

    be_mesh_prov.index = arg_int1("u", NULL, "<index>", "index");
    be_mesh_prov.end = arg_end(1);

    const esp_console_cmd_t cmd_prov_enable = {
        .command = "ble_prov_enable",
        .help = "enable provisioning",
        .hint = NULL,
        .func = &ble_prov_enable,
        .argtable = &be_mesh_prov,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_prov_enable));
}
