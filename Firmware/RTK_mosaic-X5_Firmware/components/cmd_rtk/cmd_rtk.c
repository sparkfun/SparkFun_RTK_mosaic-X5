/* The CLI commands of the RTK mosaic-X5

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

#include "cmd_rtk.h"

#define PARAM_NAMESPACE "esp32_rtk"
static const char *TAG = "cmd_rtk";

static void register_set_rtk(void);
static void register_show(void);
static void register_restart(void);
static void register_log_level(void);

void param_set_value_str(char **param, const char* val) {
    if (*param != NULL)
        free(*param);
    *param = (char *)malloc(strlen(val)+1);
    strcpy(*param, val);
}

void param_set_value_int(int **param, const int val) {
    if (*param != NULL)
        free(*param);
    *param = (int *)malloc(sizeof(int));
    **param = val;
}

void preprocess_string(char* str)
{
    if (str == NULL)
        return;

    char *p, *q;

    for (p = q = str; *p != 0; p++)
    {
        if (*(p) == '%' && *(p + 1) != 0 && *(p + 2) != 0)
        {
            // quoted hex
            uint8_t a;
            p++;
            if (*p <= '9')
                a = *p - '0';
            else
                a = toupper((unsigned char)*p) - 'A' + 10;
            a <<= 4;
            p++;
            if (*p <= '9')
                a += *p - '0';
            else
                a += toupper((unsigned char)*p) - 'A' + 10;
            *q++ = a;
        }
        else if (*(p) == '+') {
            *q++ = ' ';
        } else {
            *q++ = *p;
        }
    }
    *q = '\0';
}

esp_err_t get_config_param_str(char* name, char** param)
{
    nvs_handle_t nvs;

    esp_err_t err = nvs_open(PARAM_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        size_t len;
        if ((err = nvs_get_str(nvs, name, NULL, &len)) == ESP_OK) {
            if (*param != NULL)
                free(*param);
            *param = (char *)malloc(len + 1);
            err = nvs_get_str(nvs, name, *param, &len);
            ESP_LOGI(TAG, "get_config_param_str %s: %s", name, *param);
        } else {
            ESP_LOGW(TAG, "get_config_param_str str %s not found in nvs - it may not have been set", name); // nvs may be empty
        }
        nvs_close(nvs);
    } else {
        ESP_LOGW(TAG, "get_config_param_str could not open nvs - it may not have been initialized"); // nvs may not have been initialized
    }
    return err;
}

esp_err_t get_config_param_int(char* name, int** param)
{
    nvs_handle_t nvs;

    esp_err_t err = nvs_open(PARAM_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        int32_t i32;
        if ((err = nvs_get_i32(nvs, name, &i32)) == ESP_OK) {
            if (*param != NULL)
                free(*param);
            *param = (int *)malloc(sizeof(int));
            **param = (int)i32;
            ESP_LOGI(TAG, "get_config_param_int %s: %d", name, **param);
        } else {
            ESP_LOGW(TAG, "get_config_param_int i32 %s not found in nvs - it may not have been set", name); // nvs may be empty
        }
        nvs_close(nvs);
    } else {
        ESP_LOGW(TAG, "get_config_param_int could not open nvs - it may not have been initialized"); // nvs may not have been initialized
    }
    return err;
}

esp_err_t get_config_param_blob(char* name, uint8_t* blob,  size_t blob_len)
{
    nvs_handle_t nvs;

    esp_err_t err = nvs_open(PARAM_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        size_t len;
        if ((err = nvs_get_blob(nvs, name, NULL, &len)) == ESP_OK) {
            if (len != blob_len) {
                return ESP_ERR_NVS_INVALID_LENGTH;
            }
            err = nvs_get_blob(nvs, name, blob, &len);
            ESP_LOGI(TAG, "get_config_param_blob %s: %d", name, len);
        } else {
            ESP_LOGW(TAG, "get_config_param_blob blob %s not found in nvs - it may not have been set", name); // nvs may be empty
            return err;
        }
        nvs_close(nvs);
    } else {
        ESP_LOGW(TAG, "get_config_param_blob could not open nvs - it may not have been initialized"); // nvs may not have been initialized
        return err;
    }
    return ESP_OK;
}

void register_rtk(void)
{
    register_set_rtk();
    register_show();
    register_restart();
    register_log_level();
}

/** Arguments used by 'set' function */
static struct {
    struct arg_int* mode;
    struct arg_str* ssid;
    struct arg_str* password;
    struct arg_end* end;
} set_rtk_arg;

/* 'set' command */
int set_rtk(int argc, char **argv)
{
    esp_err_t err;
    nvs_handle_t nvs;

    int nerrors = arg_parse(argc, argv, (void **) &set_rtk_arg);
    if (nerrors != 0) {
        arg_print_errors(stderr, set_rtk_arg.end, argv[0]);
        return 1;
    }

    if (set_rtk_arg.ssid->count > 0)
        preprocess_string((char*)set_rtk_arg.ssid->sval[0]);
    if (set_rtk_arg.password->count > 0)
        preprocess_string((char*)set_rtk_arg.password->sval[0]);

    err = nvs_open(PARAM_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        return err;
    }

    if (set_rtk_arg.mode->count > 0) {
        if ((set_rtk_arg.mode->ival[0] < 1) || (set_rtk_arg.mode->ival[0] > 2)) {
            printf("mode must be 1 or 2: 1 = Ethernet; 2 = WiFi\n");
        }
        else {
            err = nvs_set_i32(nvs, "mode", set_rtk_arg.mode->ival[0]);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "mode stored: %d", set_rtk_arg.mode->ival[0]);
                // Don't update the global in RAM - it could cause badness... The change will happen at the next restart.
                //param_set_value_int(&mode, set_rtk_arg.mode->ival[0]);
            }
        }
    }

    if (set_rtk_arg.ssid->count > 0) {
        err = nvs_set_str(nvs, "ssid", set_rtk_arg.ssid->sval[0]);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "ssid stored: %s", set_rtk_arg.ssid->sval[0]);
            param_set_value_str(&ssid, (const char *)set_rtk_arg.ssid->sval[0]); // Update the global in RAM
        }
    }

    if (set_rtk_arg.password->count > 0) {
        err = nvs_set_str(nvs, "password", set_rtk_arg.password->sval[0]);
        if (err == ESP_OK) {            
            ESP_LOGI(TAG, "password stored: %s", set_rtk_arg.password->sval[0]);
            param_set_value_str(&password, (const char *)set_rtk_arg.password->sval[0]); // Update the global in RAM
        }
    }

    nvs_close(nvs);
    return err;
}

static void register_set_rtk(void)
{
    set_rtk_arg.mode = arg_int0("m", "mode", NULL, "\n\tmode: 1 = Ethernet (default)"
        "\n\t      2 = WiFi");
    set_rtk_arg.ssid = arg_str0("s", "ssid", NULL, "WiFi SSID");
    set_rtk_arg.password = arg_str0("p", "password", NULL, "WiFi Password"
        "\n\tTo set a NULL password, use: --password=%00");
    set_rtk_arg.end = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "set",
        .help = "Set the RT mosaic-X5 operating mode plus the WiFi SSID and password",
        .hint = NULL,
        .func = &set_rtk,
        .argtable = &set_rtk_arg
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int show(int argc, char **argv)
{
    int *new_mode = NULL;
    get_config_param_int("mode", &new_mode);
    if (new_mode != NULL) // Use the (updated) value from nvs if available
        printf("mode:      %d (%s)\n", *new_mode, *new_mode == 1 ? "Ethernet" : "WiFi");
    else if (mode != NULL)
        printf("mode:      %d (%s)\n", *mode, *mode == 1 ? "Ethernet" : "WiFi");
    else
        printf("mode:      <not defined>");
    printf("ssid:      %s\n", ssid != NULL ? ssid : "<not defined>");
    printf("password:  %s\n", password != NULL ? password : "<not defined>");
    printf("log_level: %s\n", esp_log_level != NULL ? esp_log_level : "<not defined>");

    return 0;
}

static void register_show(void)
{
    const esp_console_cmd_t cmd = {
        .command = "show",
        .help = "Show the current configuration",
        .hint = NULL,
        .func = &show,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int restart(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

static void register_restart(void)
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Software reset of the ESP32",
        .hint = NULL,
        .func = &restart,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/** log_level command changes log level via esp_log_level_set */

static struct {
    struct arg_str *level;
    struct arg_end *end;
} log_level_args;

static const char* s_log_level_names[] = {
    "none",
    "error",
    "warn",
    "info",
    "debug",
    "verbose"
};

int set_log_level_by_str(const char *level_str)
{
    esp_log_level_t level;
    size_t level_len = strlen(level_str);
    for (level = ESP_LOG_NONE; level <= ESP_LOG_VERBOSE; level++) {
        if (memcmp(level_str, s_log_level_names[level], level_len) == 0) {
            break;
        }
    }
    if (level > ESP_LOG_VERBOSE) {
        printf("Invalid log level '%s', choose from none|error|warn|info|debug|verbose\n", level_str);
        return 1;
    }
    if (level > CONFIG_LOG_MAXIMUM_LEVEL) {
        printf("Can't set log level to %s, max level limited in menuconfig to %s.\n"
               "Please increase CONFIG_LOG_MAXIMUM_LEVEL in menuconfig.\n",
               s_log_level_names[level], s_log_level_names[CONFIG_LOG_MAXIMUM_LEVEL]);
        return 1;
    }
    esp_log_level_set("*", level);

    return 0;
}

static int set_log_level(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &log_level_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, log_level_args.end, argv[0]);
        return 1;
    }
    assert(log_level_args.level->count == 1);
    const char* level_str = log_level_args.level->sval[0];
    
    int err = set_log_level_by_str(level_str);
    if (err != ESP_OK) {
        return err;
    }

    nvs_handle_t nvs;
    err = nvs_open(PARAM_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs, "log_level", level_str);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "log_level stored: %s", level_str);
        param_set_value_str(&esp_log_level, (const char *)level_str); // Update the global in RAM
    }

    nvs_close(nvs);

    return 0;
}

static void register_log_level(void)
{
    log_level_args.level = arg_str1(NULL, NULL, "<none|error|warn|info|debug|verbose>", "Log level to set. Abbreviated words are accepted.");
    log_level_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "log_level",
        .help = "Set log level.",
        .hint = NULL,
        .func = &set_log_level,
        .argtable = &log_level_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
