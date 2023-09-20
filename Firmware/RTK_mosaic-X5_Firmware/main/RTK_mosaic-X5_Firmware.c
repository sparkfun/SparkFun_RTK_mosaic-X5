/* 

   SparkFun RTK mosaic-X5 Firmware

   The firmware has three modes:
   1 - WiFi STN with BLE Provisioning (sets X5 Ethernet to DHCP) (default)
   2 - WiFi AP Bridge (sets X5 Ethernet to DHCP)
   3 - mosaic-X5 UART (COM4) NMEA GGA display (does not modify the Ethernet settings)

   Mode 1 allows you to connect the mosaic-X5 to your WiFi network. The SSID and password
   are provided using the Espressif BLE Provisioning App.

   Mode 2 is used by SparkFun to test the RTK mosaic-X5. The ESP32 acts as a Ethernet and
   WiFi Access Point bridge, with DHCP server. This allows you to connect to the AP and
   view the mosaic-X5 web page.

   For modes 1 and 2, you need to link the mosaic-X5 and ESP32 Ethernet ports using a
   standard Ethernet patch cable.

   Mode 3 displays simple NMEA GGA information from the X5 COM4 UART on the OLED display.
   If you want to connect the mosaic-X5 directly to your Ethernet network, select Mode 3.
   This avoids the X5 Ethernet being set to DHCP, so you can apply your own settings.

   The mode can be changed via the ESP32 USB serial console. Connect to the Config ESP32
   port and open a terminal at 115200 baud to see the console. Type help for help.

   Written for and tested on ESP IDF v5.1.1

   Needs:
   idf.py add-dependency "espressif/qrcode^0.1.0"
   idf.py add-dependency "espressif/ssd1306^1.0.5"

   WiFi AP Bridge is based on Example 2 from:
   https://github.com/espressif/esp-idf/tree/master/examples/network/bridge
   
   Based on:

   mowi_wifi_client (WiFi to Ethernet packet forwarding with BLE based Provisioning)

   This example code is licensed under CC BY-SA 4.0 and OSHW Definition 1.0.

   For mor information about MOWI visit https://github.com/septentrio-gnss/mowi/.

   This software is based on following ESP-IDF / ESP-IOT-SOLUTION examples:
   https://github.com/espressif/esp-idf/tree/master/examples/ethernet/eth2ap
   https://github.com/espressif/esp-iot-solution/tree/release/v1.1/examples/eth2wifi
   https://github.com/espressif/esp-idf/tree/master/examples/provisioning 

   This software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>

#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_eth.h>
#include <esp_eth_driver.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_netif_ip_addr.h>
#include <nvs_flash.h>

#include <esp_private/wifi.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <hal/uart_hal.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "fnt_5x8.h"

#include <esp_timer.h>
#include <driver/i2c.h>

#include <lwip/inet.h>
#include <lwip/ip4_addr.h>

#include <esp_console.h>
#include <esp_vfs_dev.h>
#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>
#include <esp_vfs_fat.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "cmd_nvs.h"
#include "cmd_rtk.h"


static const char *VERSION = "Firmware v1.0.0";
static const char *TAG = "RTK_mosaic-X5_Firmware";
#define PROMPT_STR "RTK_X5"
static const char* prompt;

static esp_eth_handle_t s_eth_handle = NULL;
static esp_eth_mac_t *s_mac = NULL;
static esp_eth_phy_t *s_phy = NULL;
static QueueHandle_t flow_control_queue = NULL;

static uint8_t eth_mac[6];
static bool eth_mac_is_set = false;
static bool wifi_is_connected = false;
static bool provisioned = false;
static char sta_ip[16];

int* mode = NULL;
int* remember = NULL;
char* ap_ssid = NULL;
char* ap_password = NULL;
int* ap_channel = NULL;
int* ap_connections = NULL;
char* esp_log_level = NULL;

typedef struct {
    void *packet;
    uint16_t length;
} flow_control_msg_t;

/* qrcode generation resources */
#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"

/* These could be in Kconfig, but who will want to change them? */
#define CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM (1)
#define CONFIG_RTK_X5_MOSAIC_UART_BUF_SIZE (1024)
#define CONFIG_RTK_X5_WIFI_GPIO_PIN (12)
#define CONFIG_RTK_X5_BT_GPIO_PIN (13)
#define CONFIG_RTK_X5_UART_TX_GPIO_PIN (2)
#define CONFIG_RTK_X5_UART_RX_GPIO_PIN (4)
#define CONFIG_RTK_X5_ETHERNET_PHY_ADDR (1)
#define CONFIG_RTK_X5_ETHERNET_ERST_GPIO (5)
#define CONFIG_RTK_X5_ETHERNET_MDC_GPIO (23)
#define CONFIG_RTK_X5_ETHERNET_MDIO_GPIO (18)

/* mosaic-X5 serial commands */
const char MOSAIC_CMD_ETHERNET_OFF[] = "seth,off\n\r";
const char MOSAIC_CMD_ETHERNET_OFF_RESPONSE_START[] = "$R: seth";
const char MOSAIC_CMD_IP_DHCP[] = "setIPSettings,DHCP,,,,,,,1500\n\r";
const char MOSAIC_CMD_IP_DHCP_RESPONSE_START[] = "$R: setIPSettings";
const char MOSAIC_CMD_ETHERNET_ON[] = "seth,on\n\r";
const char MOSAIC_CMD_ETHERNET_ON_RESPONSE_START[] = "$R: seth";
const char MOSAIC_CMD_OUTPUT_GGA_ONCE[] = "enoc,COM4,GGA\n\r";
const char MOSAIC_CMD_OUTPUT_GGA_ONCE_RESPONSE_START[] = "$GPGGA,";

/* I2C OLED */
#define I2C_HOST  0
#define RTK_X5_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define RTK_X5_PIN_NUM_SDA           15
#define RTK_X5_PIN_NUM_SCL           14
#define RTK_X5_PIN_NUM_RST           -1
#define RTK_X5_OLED_HW_ADDR          0x3C

void print_text(char *txt); // Header
void display_qr(esp_qrcode_handle_t qrcode); // Header
static ssd1306_handle_t disp;

const uint8_t oled_x_chars = 25; // 128 / 5
const uint8_t oled_y_chars = 8;  // 64 / 8
static char oled_text[25][8];
void clear_oled_text(void); // Header
void print_oled(char *txt); // Header
void display_IP(void); // Header

/* Extra SSD1306 commands - if needed */
#define RTK_SSD1306_CMD_SET_MEMORY_ADDR_MODE  0x20
#define RTK_SSD1306_CMD_SET_COLUMN_RANGE      0x21
#define RTK_SSD1306_CMD_SET_PAGE_RANGE        0x22
#define RTK_SSD1306_CMD_SET_START_LINE        0x40
#define RTK_SSD1306_CMD_SET_CONTRAST_BANK0    0x81
#define RTK_SSD1306_CMD_SET_CHARGE_PUMP       0x8D
#define RTK_SSD1306_CMD_MIRROR_X_OFF          0xA0
#define RTK_SSD1306_CMD_MIRROR_X_ON           0xA1
#define RTK_SSD1306_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define RTK_SSD1306_CMD_INVERT_OFF            0xA6
#define RTK_SSD1306_CMD_INVERT_ON             0xA7
#define RTK_SSD1306_CMD_MULTIPLEX_RATIO       0xA8
#define RTK_SSD1306_CMD_DISP_OFF              0xAE
#define RTK_SSD1306_CMD_DISP_ON               0xAF
#define RTK_SSD1306_CMD_MIRROR_Y_OFF          0xC0
#define RTK_SSD1306_CMD_MIRROR_Y_ON           0xC8
#define RTK_SSD1306_CMD_DISPLAY_OFFSET        0xD3
#define RTK_SSD1306_CMD_CLOCK_DIVIDER         0xD5
#define RTK_SSD1306_CMD_PRE_CHARGE            0xD9
#define RTK_SSD1306_CMD_COM_PINS              0xDA
#define RTK_SSD1306_CMD_SET_VCOMH_DESELECT    0xDB

/* Console history - in NVS */
#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"


/* PACKETS / DATA FORWARDING */

// Forward packets from Wi-Fi to Ethernet
static esp_err_t pkt_wifi2eth(void *buffer, uint16_t len, void *eb)
{
    if (esp_eth_transmit(s_eth_handle, buffer, len) != ESP_OK) {
        ESP_LOGE(TAG, "Ethernet send packet failed");
    }
#if CONFIG_RTK_X5_VERBOSE_LOG
    else {
        uint8_t *ptr = (uint8_t *)buffer;
        ESP_LOGI(TAG, "Sent Ethernet packet L:%d D:%02X:%02X:%02X:%02X:%02X:%02X S:%02X:%02X:%02X:%02X:%02X:%02X IPS:%d.%d.%d.%d IPD:%d.%d.%d.%d",
            len, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11],
            ptr[26], ptr[27], ptr[28], ptr[29], ptr[30], ptr[31], ptr[32], ptr[33]);
    }
#endif

    esp_wifi_internal_free_rx_buffer(eb);
    return ESP_OK;
}

// Forward packets from Ethernet to Wi-Fi
// Note that, Ethernet works faster than Wi-Fi on ESP32,
// so we need to add an extra queue to balance their speed difference.
static esp_err_t pkt_eth2wifi(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t len, void *priv)
{
    esp_err_t ret = ESP_OK;
    flow_control_msg_t msg = {
        .packet = buffer,
        .length = len
    };
    if (xQueueSend(flow_control_queue, &msg, pdMS_TO_TICKS(CONFIG_RTK_X5_FLOW_CONTROL_QUEUE_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGE(TAG, "Send flow control message failed or timeout");
        free(buffer);
        ret = ESP_FAIL;
    }
#if CONFIG_RTK_X5_VERBOSE_LOG
    else {
        ESP_LOGI(TAG, "Queued WiFi packet of length %d", (int)len);
    }
#endif
    return ret;
}

// This task will fetch the packet from the queue, and then send out through Wi-Fi.
// Wi-Fi handles packets slower than Ethernet, we might add some delay between each transmitting.
static void eth2wifi_flow_control_task(void *args)
{
    flow_control_msg_t msg;
    int res = 0;
    uint32_t timeout = 0;
    while (true) {
        if (xQueueReceive(flow_control_queue, &msg, pdMS_TO_TICKS(CONFIG_RTK_X5_FLOW_CONTROL_QUEUE_TIMEOUT_MS)) == pdTRUE) {
            timeout = 0;
            if (msg.length) {
                do {
                    if(!eth_mac_is_set) {
                        memcpy(eth_mac, (uint8_t*)msg.packet + 6, sizeof(eth_mac));
                        eth_mac_is_set = true;
                        ESP_LOGI(TAG, "Extracted MAC address from packet: %02X:%02X:%02X:%02X:%02X:%02X", 
                            eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
                    }

                    vTaskDelay(pdMS_TO_TICKS(timeout));
                    timeout += 2;
                    if(wifi_is_connected) {
                        res = esp_wifi_internal_tx(ESP_IF_WIFI_STA, msg.packet, msg.length);
                    }
                } while (res && timeout < CONFIG_RTK_X5_FLOW_CONTROL_WIFI_SEND_TIMEOUT_MS);
                if (res != ESP_OK) {
                    ESP_LOGE(TAG, "WiFi send packet failed: %d", res);
                }
#if CONFIG_RTK_X5_VERBOSE_LOG
                else {
                    ESP_LOGI(TAG, "Sent WiFi packet of length %d", msg.length);
                }
#endif
            }
            free(msg.packet);
        }
    }
    vTaskDelete(NULL);
}


/* EVENT HANDLERS */

// Event handler for Ethernet
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_G_MAC_ADDR, &eth_mac));
        ESP_LOGI(TAG, "Got MAC address: %02X:%02X:%02X:%02X:%02X:%02X", 
            eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
        break;

    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGE(TAG, "Ethernet Link Down");
        esp_restart();
        break;

    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;

    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;

    default:
        break;
    }
}

// Event handler for Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{ 
    static uint8_t s_con_cnt = 0; // Access Point connection count

    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;

            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received WiFi credentials"
                            "\n\tSSID     : %s\n\tPassword : %s",
                            (const char *) wifi_sta_cfg->ssid,
                            (const char *) wifi_sta_cfg->password);
                break;
            }

            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed\n\tReason : %s",
                            (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                            "WiFi station authentication failed" : "WiFi access-point not found");
                wifi_prov_mgr_reset_sm_state_on_failure();

                // Wait for ESP to inform BLE app
                vTaskDelay(500);

                // Reboot ESP
                esp_restart();
                break;
            }

            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;

            case WIFI_PROV_END:
                ESP_LOGI(TAG, "Provisioning ended. Manager deinit");
                // De-initialize manager
                wifi_prov_mgr_deinit();
                break;
                
            default:
                break;
        }
    }
    
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (provisioned) {
            ESP_LOGI(TAG, "WiFi started. Connecting...");
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
        else {
            ESP_LOGI(TAG, "Not yet provisioned. Ignoring WIFI_EVENT_STA_START");
        }
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected");
        wifi_is_connected = false;
        gpio_set_level(CONFIG_RTK_X5_WIFI_GPIO_PIN, false);

        // Stop forwarding WiFi and Ethernet data
        ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, NULL));
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi AP got a station connected");
        if (!s_con_cnt) {
            wifi_is_connected = true;
            ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_AP, &pkt_wifi2eth));
        }
        s_con_cnt++;
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi AP got a station disconnected");
        s_con_cnt--;
        if (!s_con_cnt) {
            wifi_is_connected = false;
            ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_AP, NULL));
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{ 
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_is_connected = true;
        gpio_set_level(CONFIG_RTK_X5_WIFI_GPIO_PIN, true);

        snprintf(sta_ip, sizeof(sta_ip), "%s", inet_ntoa(event->ip_info.ip));
        display_IP();

        // Start forwarding WiFi and Ethernet data
        ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, &pkt_wifi2eth));
    }
}

esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf) {
        ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; // +1 for NULL terminating byte

    return ESP_OK;
}

void check_provisioned(void)
{
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
}

/* Tasks */

static void console_task(void *args)
{
    while (true) {
        /* Get a line using linenoise.
        * The line is returned when ENTER is pressed.
        */
        char* line = linenoise(prompt);

        if (line != NULL) {
            /* Add the command to the history if not empty*/
            if (strlen(line) > 0) {
                linenoiseHistoryAdd(line);
                /* Save command history to filesystem */
                linenoiseHistorySave(HISTORY_PATH);
            }

            /* Try to run the command */
            int ret;
            esp_err_t err = esp_console_run(line, &ret);
            if (err == ESP_ERR_NOT_FOUND) {
                printf("Unrecognized command\n");
            } else if (err == ESP_ERR_INVALID_ARG) {
                // command was empty
            } else if (err == ESP_OK && ret != ESP_OK) {
                printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
            } else if (err != ESP_OK) {
                printf("Internal error: %s\n", esp_err_to_name(err));
            }
            /* linenoise allocates line buffer on the heap, so need to free it */
            linenoiseFree(line);
        }
    }
    vTaskDelete(NULL);
}

static void x5_uart_task(void *args)
{
    // Create a temporary buffer for the incoming data
    size_t buf_size = CONFIG_RTK_X5_MOSAIC_UART_BUF_SIZE * sizeof(uint8_t);
    uint8_t *uart_buf = (uint8_t *) malloc(buf_size);
    int len = 0;

    while (true) {
        // Request NMEA GGA. The response is $GPGGA,,,,,*CS\r\n$R: enoc,COM4,GGA\r\n  NMEAOnce, COM4, GGA\r\nCOM4>
        ESP_ERROR_CHECK(uart_flush_input(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM));
        uart_write_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, (char*)MOSAIC_CMD_OUTPUT_GGA_ONCE, strlen(MOSAIC_CMD_OUTPUT_GGA_ONCE));
        len = uart_read_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, uart_buf, CONFIG_RTK_X5_MOSAIC_UART_BUF_SIZE, pdMS_TO_TICKS(700)); // Force a timeout
        if(len <= 0 || strncmp((char *)uart_buf, MOSAIC_CMD_OUTPUT_GGA_ONCE_RESPONSE_START, strlen(MOSAIC_CMD_OUTPUT_GGA_ONCE_RESPONSE_START)) != 0) {
            ESP_LOGE(TAG, "Output NMEA GGA once failed");
        }
        else {
            char *remainder = (char *)uart_buf;
            char *token = strtok_r(remainder, ",", &remainder); // $GPGGA
            char line[25];
            token = strtok_r(remainder, ",", &remainder); // Time
            snprintf(line, sizeof(line), "Time: %s", token);
            print_oled(line);
            token = strtok_r(remainder, ",", &remainder); // Latitude
            snprintf(line, sizeof(line), "Lat:  %s %c", token, *remainder);
            print_oled(line);
            token = strtok_r(remainder, ",", &remainder); // N/S
            token = strtok_r(remainder, ",", &remainder); // Longitude
            snprintf(line, sizeof(line), "Long: %s %c", token, *remainder);
            print_oled(line);
            token = strtok_r(remainder, ",", &remainder); // E/W
            token = strtok_r(remainder, ",", &remainder); // Fix
            snprintf(line, sizeof(line), "Fix:  %s", token);
            print_oled(line);
            token = strtok_r(remainder, ",", &remainder); // Num Sat
            snprintf(line, sizeof(line), "Sat:  %s", token);
            print_oled(line);
            token = strtok_r(remainder, ",", &remainder); // HDOP
            snprintf(line, sizeof(line), "HDOP: %s", token);
            print_oled(line);
            token = strtok_r(remainder, ",", &remainder); // Alt (Elev)
            snprintf(line, sizeof(line), "Alt:  %s %c", token, *remainder);
            print_oled(line);
            token = strtok_r(remainder, ",", &remainder); // M
            token = strtok_r(remainder, ",", &remainder); // Geoid
            token = strtok_r(remainder, ",", &remainder); // M
            token = strtok_r(remainder, ",", &remainder); // Age
            snprintf(line, sizeof(line), "Age:  %s", token);
            print_oled(line);
        }
    }

    free(uart_buf);
    vTaskDelete(NULL);
}


/* Generate QR code for BLE provisioning */

static void prov_print_qr(const char *name, const char *pop, const char *transport)
{
    if (!name || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }

    char payload[150] = {0};
    if (pop) {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, pop, transport);
    }
    
    else {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, transport);
    }

    // Show qrcode
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    cfg.display_func = &display_qr; // Display it on the OLED instead of the console
    esp_qrcode_generate(&cfg, payload);
}


/* INITIALIZATION */

static void initialize_leds(void)
{
    // Configure WiFi LED GPIO
    gpio_config_t io_conf;

    // Disable interrupt, set as output mode, disable pull modes
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << CONFIG_RTK_X5_WIFI_GPIO_PIN;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = false;
    gpio_config(&io_conf);

    // Turn WiFi LED off
    gpio_set_level(CONFIG_RTK_X5_WIFI_GPIO_PIN, false);

    // Disable interrupt, set as output mode, disable pull modes
    io_conf.pin_bit_mask = 1ULL << CONFIG_RTK_X5_BT_GPIO_PIN;
    gpio_config(&io_conf);

    // Turn BT LED off
    gpio_set_level(CONFIG_RTK_X5_BT_GPIO_PIN, false);
}

static void initialize_uart(void)
{
    // Configure UART parameters
    uart_config_t uart_param = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver, set Tx FIFO to 0 to send data immediately
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, CONFIG_RTK_X5_MOSAIC_UART_BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, &uart_param));
    ESP_ERROR_CHECK(uart_set_pin(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, CONFIG_RTK_X5_UART_TX_GPIO_PIN, CONFIG_RTK_X5_UART_RX_GPIO_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Redundant parameters setting - bug in ESP-IDF workaround 
    uart_set_baudrate(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, 115200);
    uart_set_word_length(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, UART_DATA_8_BITS);
    uart_set_parity(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, UART_PARITY_DISABLE); 
    uart_set_stop_bits(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, UART_STOP_BITS_1);
}

static void initialize_ethernet(void)
{
    ESP_LOGI(TAG, "Initializing Ethernet");
    print_oled("Initializing Ethernet");


    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());

/*
    // DHCP example from https://github.com/espressif/esp-idf/issues/4086#issuecomment-559530841

    #define IP4TOADDR(a,b,c,d) esp_netif_htonl(ESP_IP4TOUINT32(a, b, c, d))
    const esp_netif_ip_info_t my_ap_ip = {
            .ip = { .addr = IP4TOADDR( 192, 168, 1, 1) },
            .gw = { .addr = IP4TOADDR( 192, 168, 1, 1) },
            .netmask = { .addr = IP4TOADDR( 255, 255, 255, 0) },

    };

    const esp_netif_inherent_config_t eth_behav_cfg = {
            .get_ip_event = IP_EVENT_ETH_GOT_IP,
            .lost_ip_event = 0,
            .flags = ESP_NETIF_DHCP_SERVER | ESP_NETIF_FLAG_AUTOUP,
            .ip_info = (esp_netif_ip_info_t*)& my_ap_ip,
            .if_key = "ETH_DHCPS",
            .if_desc = "eth",
            .route_prio = 50
    };

    esp_netif_config_t eth_as_dhcps_cfg = {
        .base = &eth_behav_cfg,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };

    esp_netif_t *eth_netif = esp_netif_new(&eth_as_dhcps_cfg);
*/


    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = CONFIG_RTK_X5_ETHERNET_PHY_ADDR;
    phy_config.reset_gpio_num = CONFIG_RTK_X5_ETHERNET_ERST_GPIO;

    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp32_emac_config.smi_mdc_gpio_num = CONFIG_RTK_X5_ETHERNET_MDC_GPIO;
    esp32_emac_config.smi_mdio_gpio_num = CONFIG_RTK_X5_ETHERNET_MDIO_GPIO;
    
    s_mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);

    s_phy = esp_eth_phy_new_ksz80xx(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(s_mac, s_phy);

    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &s_eth_handle));

    ESP_ERROR_CHECK(esp_eth_update_input_path(s_eth_handle, &pkt_eth2wifi, NULL));

    bool eth_promiscuous = true;
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_PROMISCUOUS, &eth_promiscuous));

    bool auto_negociate = true;
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_AUTONEGO, &auto_negociate));


    /* attach Ethernet driver to TCP/IP stack */
    //ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(s_eth_handle)));

    /*
    It is recommended to fully initialize the Ethernet driver and network interface before registering
    the user’s Ethernet/IP event handlers, i.e., register the event handlers as the last thing prior to
    starting the Ethernet driver. Such an approach ensures that Ethernet/IP events get executed first by
    the Ethernet driver or network interface so the system is in the expected state when executing the
    user’s handlers.
    */
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));


    ESP_LOGI(TAG, "Configuring Mosaic Ethernet DHCP");
    print_oled("Configure Ethernet DHCP");

    // Create a temporary buffer for the incoming data
    uint8_t *uart_buf = (uint8_t *) calloc(CONFIG_RTK_X5_MOSAIC_UART_BUF_SIZE, sizeof(uint8_t));
    int len = 0;

    // Disable Mosaic Ethernet - iterate to initialize connection
    ESP_ERROR_CHECK(uart_flush_input(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM));
    uart_write_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, (char*)MOSAIC_CMD_ETHERNET_OFF, strlen(MOSAIC_CMD_ETHERNET_OFF));
    len = uart_read_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, uart_buf, strlen(MOSAIC_CMD_ETHERNET_OFF_RESPONSE_START), 500);
    if(len <= 0 || strncmp((char *)uart_buf, MOSAIC_CMD_ETHERNET_OFF_RESPONSE_START, strlen(MOSAIC_CMD_ETHERNET_OFF_RESPONSE_START)) != 0) {
        ESP_LOGE(TAG, "Disable Mosaic Ethernet response failed");
        esp_restart();
    }

    // Set Mosaic Ethernet DHCP with MTU
    ESP_ERROR_CHECK(uart_flush_input(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM));
    uart_write_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, (char*)MOSAIC_CMD_IP_DHCP, strlen(MOSAIC_CMD_IP_DHCP));
    len = uart_read_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, uart_buf, strlen(MOSAIC_CMD_IP_DHCP_RESPONSE_START), 500);
    if(len <= 0 || strncmp((char *)uart_buf, MOSAIC_CMD_IP_DHCP_RESPONSE_START, strlen(MOSAIC_CMD_IP_DHCP_RESPONSE_START)) != 0) {
        ESP_LOGE(TAG, "Set Mosaic Ethernet DHCP response failed");
        esp_restart();
    }

    // Start ESP ethernet
    esp_eth_start(s_eth_handle);

    // Enable Mosaic Ethernet
    ESP_ERROR_CHECK(uart_flush_input(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM));
    uart_write_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, (char*)MOSAIC_CMD_ETHERNET_ON, strlen(MOSAIC_CMD_ETHERNET_ON));
    len = uart_read_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, uart_buf, strlen(MOSAIC_CMD_ETHERNET_ON_RESPONSE_START), 500);
    if(len <= 0 || strncmp((char *)uart_buf, MOSAIC_CMD_ETHERNET_ON_RESPONSE_START, strlen(MOSAIC_CMD_ETHERNET_ON_RESPONSE_START)) != 0) {
        ESP_LOGE(TAG, "Enable Mosaic Ethernet response failed");
        esp_restart();
    }

    // Ethernet configured, temporary buffer no longer needed
    ESP_LOGI(TAG, "Mosaic Ethernet DHCP configured");
    print_oled("DHCP Configured");
    free(uart_buf);

    // Wait for Mosaic to report ist MAC address
    ESP_LOGI(TAG, "Waiting for Mosaic Ethernet MAC address");
    print_oled("Waiting for MAC address");
    while(!eth_mac_is_set) {
        vTaskDelay(10);
    }

    // Mask ESP MAC address with Mosaic one
    ESP_ERROR_CHECK(esp_base_mac_addr_set(eth_mac));
    ESP_LOGI(TAG, "ESP base MAC address set as: %02X:%02X:%02X:%02X:%02X:%02X", 
        eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    char mac_str[25];
    snprintf(mac_str, sizeof(mac_str), "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
        eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    print_oled(mac_str);
}

static void initialize_wifi(void)
{
    ESP_LOGI(TAG, "Initializing WiFi");
    print_oled("Initializing WiFi");

    // Check if we are in Mode 1 (WiFi STN plus BLE Provisioning) or Mode 2 (WiFi AP)

    if (*mode == 1)
    {

        // Register event handler for WiFi and IP events
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

        // Initialize WiFi including netif with default config
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // Configure provisioning manager
        wifi_prov_mgr_config_t config = {
            .scheme = wifi_prov_scheme_ble,
            .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
        };

        // Initialize provisioning manager
        ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

        if (*remember == 1) {
            check_provisioned();
        }

        // Start provisioning service if not yet provisioned
        if (!provisioned) {
            ESP_LOGI(TAG, "Starting provisioning");
            print_oled("Starting provisioning");

            // Set device name
            char service_name[30];
            snprintf(service_name, sizeof(service_name), "PROV_RTK_X5_%02X:%02X:%02X:%02X:%02X:%02X", 
                eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);

            // Set security level
            wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
            const char *pop = "abcd1234";
            wifi_prov_security1_params_t *sec_params = pop;
            const char *service_key = NULL;

            uint8_t custom_service_uuid[] = {
                0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
            };
            ESP_ERROR_CHECK(wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid));
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create("custom-data"));

            // Start provisioning service
            ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, service_name, service_key));
            ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL));

            gpio_set_level(CONFIG_RTK_X5_BT_GPIO_PIN, true);

            // Display QR code for provisioning
            prov_print_qr(service_name, pop, PROV_TRANSPORT_BLE);
        }
        
        else {
            ESP_LOGI(TAG, "Already provisioned, starting WiFi STA");
            print_oled("Already provisioned");
            print_oled("Starting WiFi STA");

            // Release provisioning resources
            wifi_prov_mgr_deinit();

            // Start WiFi station
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());
        }
    }

    else
    {
        // Mode 2 - WiFi AP
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        wifi_config_t wifi_config = {
            .ap = {
                .ssid_len = strlen(ap_ssid),
                .max_connection = *ap_connections,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                .channel = *ap_channel,
            },
        };
        strlcpy((char*)wifi_config.ap.ssid, ap_ssid, strlen(ap_ssid));
        if (strlen(ap_password) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }
        else {
            strlcpy((char*)wifi_config.ap.password, ap_password, strlen(ap_password));
        }
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
}

static esp_err_t initialize_flow_control(void)
{
    flow_control_queue = xQueueCreate(CONFIG_RTK_X5_FLOW_CONTROL_QUEUE_LENGTH, sizeof(flow_control_msg_t));
    if (!flow_control_queue) {
        ESP_LOGE(TAG, "Create flow control queue failed");
        return ESP_FAIL;
    }

    BaseType_t ret = xTaskCreate(eth2wifi_flow_control_task, "flow_ctl", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Create flow control task failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void initialize_i2c(void)
{
    ESP_LOGI(TAG, "Initialize I2C bus");

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = RTK_X5_PIN_NUM_SDA,
        .scl_io_num = RTK_X5_PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = RTK_X5_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));
}

static void initialize_oled(void)
{
    ESP_LOGI(TAG, "OLED initialization");

    disp = ssd1306_create(I2C_HOST, RTK_X5_OLED_HW_ADDR);
    if (!disp)
    {
        ESP_LOGE(TAG, "SSD1306 create returned error");
        return;
    }

    ssd1306_refresh_gram(disp);

    clear_oled_text();
    print_oled("RTK mosaic-X5 starting");
    print_oled((char *)VERSION);
}


static void initialize_filesystem(void)
{
    ESP_LOGI(TAG, "Initializing file system");

    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}

static void initialize_nvs(void)
{
    ESP_LOGI(TAG, "Initializing NVS");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void initialize_console(void)
{
    ESP_LOGI(TAG, "Initializing console");
    print_oled("Initializing console");

    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
            .hint_color = atoi(LOG_COLOR_CYAN)
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(10);

    /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    /* Do return empty lines */
    linenoiseAllowEmpty(true);

    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
}

static void start_console(void)
{
    ESP_LOGI(TAG, "Starting console");
    print_oled("Starting console");

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;

    printf("\n"
           "RTK mosaic-X5 ESP-IDF console.\n"
           "\nType 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n\n");
        linenoiseSetDumbMode(1);
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = PROMPT_STR "> ";
    }

    BaseType_t ret = xTaskCreate(console_task, "console_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Create console task failed");
    }
}

static void initialize_x5_uart_task(void)
{
    ESP_LOGI(TAG, "Initializing mosaic-X5 UART task");

    BaseType_t ret = xTaskCreate(x5_uart_task, "x5_uart_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Create mosaic-x5 UART task failed");
    }
}

/* Display the QR code on the OLED */

void display_qr(esp_qrcode_handle_t qrcode)
{
    ESP_LOGI(TAG, "Display QR called");

    ssd1306_clear_screen(disp, 0);
    ssd1306_refresh_gram(disp);

    uint8_t size = esp_qrcode_get_size(qrcode);

    uint8_t magnify = SSD1306_HEIGHT / size;

    if (magnify == 0)
    {
        ESP_LOGE(TAG, "QR code is too big for display: size %d", size);
        return;
    }

    const uint8_t x_border = (SSD1306_WIDTH - size) / 2;
    const uint8_t y_border = (SSD1306_HEIGHT - size) / 2;

    for (uint8_t y = 0; y < size; y++) {
        for (uint8_t x = 0; x < size; x++) {
            if (esp_qrcode_get_module(qrcode, x, y)) {
                for (uint8_t mx = 0; mx < magnify; mx++) {
                    for (uint8_t my = 0; my < magnify; my++) {
                        ssd1306_fill_point(disp, x_border + (x * magnify) + mx, y_border + (y * magnify) + my, 1);
                    }
                }
            }
        }
    }

    ssd1306_refresh_gram(disp);
}

/* Very simple 8-line scrolling text console on the OLED */

void clear_oled_text(void) {
    for (uint8_t y = 0; y < oled_y_chars; y++)
        for (uint8_t x = 0; x < oled_x_chars; x++)
            oled_text[x][y] = ' ';
}

void ssd1306_draw_58char(ssd1306_handle_t dev, uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
    uint8_t i, j;
    uint8_t chTemp = 0, chYpos0 = chYpos, chMode = 0;

    for (i = 0; i < FONT_5X7_WIDTH; i++) {
        chTemp = font5x7_data[(uint16_t)chChar * FONT_5X7_WIDTH + i];
        for (j = 0; j < FONT_5X7_HEIGHT; j++) {
            chMode = chTemp & 0x01 ? 1 : 0;
            ssd1306_fill_point(dev, chXpos, chYpos, chMode);
            chTemp >>= 1;
            chYpos++;
            }
        chYpos = chYpos0;
        chXpos++;
    }
}

void print_oled(char *txt) {
    // Scroll text up by one line
    for (uint8_t y = 0; y < (oled_y_chars - 1); y++)
        for (uint8_t x = 0; x < oled_x_chars; x++)
            oled_text[x][y] = oled_text[x][y + 1];
    uint8_t x;
    // Add new text - wrapping not currently supported
    char *ptr = txt;
    for (x = 0; (x < strlen(txt)) && (x < oled_x_chars); x++) {
        oled_text[x][oled_y_chars - 1] = *ptr;
        ptr++;
    }
    // Wipe to end of line
    for (; x < oled_x_chars; x++)
        oled_text[x][oled_y_chars - 1] = ' ';
    // Print the characters
    ssd1306_clear_screen(disp, 0);
    uint8_t ypos = 0;
    for (uint8_t y = 0; y < oled_y_chars; y++) {
        uint8_t xpos = 0;
        for (uint8_t x = 0; x < oled_x_chars; x++) {
            ssd1306_draw_58char(disp, xpos, ypos, oled_text[x][y]);
            xpos += 5;
        }
        ypos += 8;
    }
    ssd1306_refresh_gram(disp);
}

/* Display the WiFi STN IP Address using the 1608 font */

void ssd1306_draw_1608char(ssd1306_handle_t dev, uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
    uint8_t i, j;
    uint8_t chTemp = 0, chYpos0 = chYpos, chMode = 0;

    for (i = 0; i < 16; i++) {
        chTemp = c_chFont1608[chChar - 0x20][i];
        for (j = 0; j < 8; j++) {
            chMode = chTemp & 0x80 ? 1 : 0;
            ssd1306_fill_point(dev, chXpos, chYpos, chMode);
            chTemp <<= 1;
            chYpos++;
            if ((chYpos - chYpos0) == 16) {
                chYpos = chYpos0;
                chXpos++;
                break;
            }
        }
    }
}

void display_IP(void)
{
    ssd1306_clear_screen(disp, 0);

    uint8_t ypos = 8;
    char my_IP[] = "IP Address:";
    uint8_t xpos = (SSD1306_WIDTH - (strlen(my_IP) * 8)) / 2;
    char *ptr = my_IP;
    for (uint8_t x = 0; x < strlen(my_IP); x++) {
        ssd1306_draw_1608char(disp, xpos, ypos, *ptr++);
        xpos += 8;
    }

    ypos = 40;
    xpos = (SSD1306_WIDTH - (strlen(sta_ip) * 8)) / 2;
    ptr = sta_ip;
    for (uint8_t x = 0; x < strlen(sta_ip); x++) {
        ssd1306_draw_1608char(disp, xpos, ypos, *ptr++);
        xpos += 8;
    }
    
    ssd1306_refresh_gram(disp);
}


/* APPLICATION MAIN */

void app_main(void)
{
    // Initialize NVS partition and file system
    initialize_nvs();
    initialize_filesystem();

    // Initialize auxiliary peripherals
    initialize_leds();
    initialize_uart();
    initialize_i2c();
    initialize_oled();

    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Default settings */
    get_config_param_int("mode", &mode);
    if (mode == NULL) {
        param_set_value_int(&mode, 1); // Default to WiFi STN with BLE Provisioning
    }
    get_config_param_int("remember", &remember);
    if (remember == NULL) {
        param_set_value_int(&remember, 1); // Remember the BLE Provisioning
    }
    get_config_param_str("ap_ssid", &ap_ssid);
    if (ap_ssid == NULL) {
        param_set_value_str(&ap_ssid, "RTK_mosaic-X5_AP");
    }
    get_config_param_str("ap_password", &ap_password);
    if (ap_password == NULL) {
        param_set_value_str(&ap_password, ""); // Default: no password
    }
    get_config_param_int("ap_channel", &ap_channel);
    if (ap_channel == NULL) {
        param_set_value_int(&ap_channel, 1);
    }
    get_config_param_int("ap_connections", &ap_connections);
    if (ap_connections == NULL) {
        param_set_value_int(&ap_connections, 1);
    }
    get_config_param_str("log_level", &esp_log_level);
    if (esp_log_level == NULL) {
        param_set_value_str(&esp_log_level, "warn");
    }
    set_log_level_by_str((const char *)esp_log_level);


    // Initialize console
    initialize_console();

    /* Register commands */
    esp_console_register_help_command();
    //register_nvs(); // We need nvs but don't need to register it. It just clutters up the help
    register_rtk();

    // Start the console
    start_console();


    // Check the mode
    if (*mode == 1) {
        ESP_LOGI(TAG, "Firmware is in mode 1: WiFi STN with BLE Provisioning");
        print_oled("Firmware is in mode 1");
        print_oled(" WiFi STN with BLE Prov");
    }
    else if (*mode == 2) {
        ESP_LOGI(TAG, "Firmware is in mode 2: WiFi AP");
        print_oled("Firmware is in mode 2");
        print_oled(" WiFi AP");
        char txt[25];
        snprintf(txt, sizeof(txt), "SSID: %s", ap_ssid);
        print_oled(txt);
        snprintf(txt, sizeof(txt), "Pass: %s", ap_password);
        print_oled(txt);
    }
    else
    {
        ESP_LOGI(TAG, "Firmware is in mode 3: mosaic-X5 UART terminal");
        print_oled("Firmware is in mode 3");
        print_oled(" mosaic-X5 COM4 UART GGA");
    }

    if (*mode == 3) {
        initialize_x5_uart_task(); // In mode 3, run the simple uart-oled-terminal task
    }
    else {
        // Initialize flow control and main peripherals - but only in modes 1 and 2
        ESP_ERROR_CHECK(initialize_flow_control());
        initialize_ethernet();
        initialize_wifi();
    }
}
