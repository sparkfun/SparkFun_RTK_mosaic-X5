/* 

   SparkFun RTK mosaic-X5 WiFi Station example

   This example shows how to set the SSID and password using Bluetooth
   provisioning via the Espressif ESP BLE Provisioning App.

   Written for and tested on ESP IDF v5.1.1

   The I2C OLED code is taken from:
   https://github.com/espressif/esp-idf/tree/master/examples/peripherals/lcd/i2c_oled
   Needs:
   idf.py add-dependency "lvgl/lvgl^8.3.9"
   idf.py add-dependency "espressif/esp_lvgl_port^1.3.0"
   idf.py add-dependency "espressif/qrcode^0.1.0"

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
#include <nvs_flash.h>

#include <esp_private/wifi.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <hal/uart_hal.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h"

#include <esp_timer.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/i2c.h>
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_vendor.h"

#include <lwip/inet.h>
#include <lwip/ip4_addr.h>

static const char *TAG = "RTK_mosaic-X5_WiFi_STN_BT_Prov";

static esp_eth_handle_t s_eth_handle = NULL;
static esp_eth_mac_t *s_mac = NULL;
static esp_eth_phy_t *s_phy = NULL;
static QueueHandle_t flow_control_queue = NULL;

static uint8_t eth_mac[6];
static bool eth_mac_is_set = false;
static bool wifi_is_connected = false;
static bool provisioned = false;
static char sta_ip[16];

typedef struct {
    void *packet;
    uint16_t length;
} flow_control_msg_t;

// qrcode generation resources
#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"

/* Kconfig */
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

/* I2C OLED */
#define I2C_HOST  0
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (100 * 1000)
#define EXAMPLE_PIN_NUM_SDA           15
#define EXAMPLE_PIN_NUM_SCL           14
#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_I2C_HW_ADDR           0x3D
#define EXAMPLE_LCD_H_RES             128
#define EXAMPLE_LCD_V_RES             64
#define EXAMPLE_LCD_CMD_BITS          8
#define EXAMPLE_LCD_PARAM_BITS        8

void lvgl_scroll_text(char *txt); // Header
void lvgl_display_qr(esp_qrcode_handle_t qrcode); // Header
lv_disp_t * disp = NULL;


/* PACKETS / DATA FORWARDING */

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
    cfg.display_func = &lvgl_display_qr;
    esp_qrcode_generate(&cfg, payload);
}

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

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{ 
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
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{ 
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_is_connected = true;
        gpio_set_level(CONFIG_RTK_X5_WIFI_GPIO_PIN, true);

        // Start forwarding WiFi and Ethernet data
        ESP_ERROR_CHECK(esp_wifi_internal_reg_rxcb(ESP_IF_WIFI_STA, &pkt_wifi2eth));

        snprintf(sta_ip, sizeof(sta_ip), "%s", inet_ntoa(event->ip_info.ip));
        lvgl_scroll_text(sta_ip);
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

    /*
    It is recommended to fully initialize the Ethernet driver and network interface before registering
    the user’s Ethernet/IP event handlers, i.e., register the event handlers as the last thing prior to
    starting the Ethernet driver. Such an approach ensures that Ethernet/IP events get executed first by
    the Ethernet driver or network interface so the system is in the expected state when executing the
    user’s handlers.
    */
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));


    ESP_LOGI(TAG, "Configuring Mosaic Ethernet DHCP");

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
    free(uart_buf);

    // Wait for Mosaic to report ist MAC address
    ESP_LOGI(TAG, "Waiting for Mosaic Ethernet MAC address");
    while(!eth_mac_is_set) {
        vTaskDelay(10);
    }

    // Mask ESP MAC address with Mosaic one
    ESP_ERROR_CHECK(esp_base_mac_addr_set(eth_mac));
    ESP_LOGI(TAG, "ESP base MAC address set as: %02X:%02X:%02X:%02X:%02X:%02X", 
        eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
}

static void initialize_wifi(void)
{
    // Initialize TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());

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

#ifdef CONFIG_MOWI_PROV_MGR_REMEMBER
    check_provisioned();
#endif

    // Start provisioning service if not yet provisioned
    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");

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

        // Release provisioning resources
        wifi_prov_mgr_deinit();

        // Start WiFi station
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
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

static esp_err_t initialize_i2c(void)
{
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_HOST, &i2c_conf);
    ESP_ERROR_CHECK(ret);
    ret = i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0);
    ESP_ERROR_CHECK(ret);
    return ret;
}

static esp_err_t initialize_oled(void)
{
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,                 // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,     // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS, // According to SSD1306 datasheet
        .dc_bit_offset = 6,                       // According to SSD1306 datasheet
    };
    esp_err_t ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }
    };
    disp = lvgl_port_add_disp(&disp_cfg);

    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROT_180);

    return ret;
}

void lvgl_scroll_text(char *txt)
{
    ESP_LOGI(TAG, "LVGL Scroll Text called: %s", txt);

    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    lv_label_set_text(label, txt);
    lv_obj_set_width(label, disp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void lvgl_display_qr(esp_qrcode_handle_t qrcode)
{
    ESP_LOGI(TAG, "LVGL Display QR called");

    static lv_style_t style;
    lv_style_init(&style);

    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t * obj = lv_img_create(scr);
    lv_obj_add_style(obj, &style, 0);

    lv_img_set_src(obj, qrcode);

    lv_obj_center(obj);
}

/* APPLICATION MAIN */

void app_main(void)
{
    // Initialize NVS partition
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize auxiliary peripherals
    initialize_leds();
    initialize_uart();
    initialize_i2c();
    initialize_oled();

    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(initialize_flow_control());

    // Initialize main peripherals
    initialize_ethernet();
    initialize_wifi();
}
