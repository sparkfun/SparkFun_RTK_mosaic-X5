/* 

   SparkFun RTK mosaic-X5 Firmware

   The firmware has two modes:
   1 - Ethernet (default)
   2 - WiFi

   In Mode 1, the RTK mosaic-X5 should be connected directly to an Ethernet network. Use a
   standard Ethernet patch cable to connect the MOSAIC ETHERNET port to your network.
   The RTK mosaic-X5 supports Power-over-Ethernet (PoE), allowing it to be powered by the
   network too.
   In Mode 1 the ESP32 requests NMEA GGA information from the X5 over COM4. It displays the
   GGA data (time and position) on the built-in OLED display, together with the X5's Ethernet
   IP address. Open a web browser and navigate to that address to view the X5's internal web page.
   In Mode 1 the ESP32 does not modify the X5's Ethernet settings, allowing you to change and
   save the settings without them being overwritten.

   Mode 2 allows you to connect the RTK mosaic-X5 to your WiFi network. Link the MOSAIC and
   ESP32 ETHERNET ports using a standard Ethernet patch cable. The ESP32 provides Network Address
   Port Translation between X5 Ethernet and WiFi. ESP32 is the DHCP server for mosaic-X5 Ethernet.
   The WiFi SSID and password are set using the CONFIG ESP32 USB serial console.
   Once the ESP32 is connected to WiFi, you can view the X5's internal web page at the IP address
   shown on the OLED display.
   In Mode 2 the ESP32 sets the X5 Ethernet interface to DHCP, so that the X5 can request an IP
   address from the ESP32.

   The mode can be changed via the CONFIG ESP32 USB serial console. Connect to the CONFIG ESP32
   USB port and open a terminal at 115200 baud to see the console. Type help for help.

   E.g.:

   help
   show
   set --mode=2 --ssid=SSID --password=PASSWORD
   restart

   ---

   Updates September 8th 2026 (v1.1.0):

   Major update - based on:
   https://github.com/espressif/esp-protocols/tree/master/examples/esp_netif/eth_gateway_wifi_sta

   Tested with ESP-IDF v6.1

   ---

   Updates September 7th 2026 (v1.0.6):

   ESP-IDF:
	 Bump to ESP-IDF v5.1.7
	 Add CONFIG_ETH_TRANSMIT_MUTEX=y
   WiFi mode improvements:
     Fixed an error where it was possible for the original ESP32 Ethernet MAC address to
      replace the desired spoofed mosaic-X5 MAC address
     The firmware ignores an all-zeros MAC Address in IPStatus (or an Ethernet packet)
        This was causing problems when starting in WiFi mode
     The IP_EVENT_STA_GOT_IP event IP address is copied to the OLED display
        This is received before the IPStatus update
        This ensures the IP address is displayed even if the IPStatus is missed
     The firmware no longer sends "seth,off" to turn Ethernet off before configuring DHCP
        It looks like this caused more problems than it solved
     Reduced the wait-for-IPStatus timeout from 5s to 3s
        It is received quicker than that
   General improvements:
     The minimal vTaskDelay has been increased from vTaskDelay(0) to vTaskDelay(1)
        This prevents unwanted Watchdog resets during during wait-for-command-response

   ---

   Updates April 22nd 2026 (v1.0.5):

   mosaic-X5 firmware 4.15.1 requires a mandatory user-defined username and password on IP interfaces.
   v1.0.5 adds the two nvm parameters x5_user (set -u) and x5_pass (set -x).
   If defined, these are passed to the X5 as "login,<x5_user>,<x5_pass>"
   
   Please note: the ESP32 does not _need_ to know the username and password _unless_ you also change
   the default access levels for the COM interfaces. Please see the docs for more details:
   https://docs.sparkfun.com/SparkFun_RTK_mosaic-X5/software_overview/#esp32-firmware-update

   Type help for help.

   E.g.:

   help
   show
   set --x5_user=USERNAME --x5_pass=PASSWORD
   restart

   ---

   Updates June 24th 2025 (v1.0.4):

   We've had one unit where the SBF and NMEA protocols became disabled on COM4, causing the ESP32
   firmware to hang. This version uses sdio to ensure SBF and NMEA are enabled on COM4 when starting.

   We've also had a unit where the NMEA prefix had been set to GN, instead of GP. This too
   caused the firmware to hang. This version is tolerant and can parse both GPGGA and GNGGA.

   A user requested that the position shown on the OLED was in DD MM SS.SSS format - same as
   the X5 internal web page. This version does that. The DDMM.MMMMM lat / lon in GPGGA is converted to
   DD MM SS.SSSS.

   Instead of using enoc and esoc to poll the NMEA GGA and SBF IPStatus messages, this version
   includes a parser which will extract:
   * lat, lon, etc. from periodic NMEA GGA messages on NMEA Stream10
   * the X5 Ethernet IP address from on-change IPStatus messages on SBF Stream10
   Sorry if you were using NMEA and SBF Stream10 for your own purposes!

   Because the GPGGA messages are periodic, not polled, the OLED is updated each time a GGA message is
   received - every second. The OLED displays UTC time from GGA, whereas the X5 web page displays GPS
   time. There is currently a difference of 18 seconds between the two.

   The altitude displayed on the OLED defaults to Orthometric height (height above MSL). This can be
   changed to include the geoid separation by changing the RTK_X5_DISPLAY_ALT_WITH_GEOID_SEPARATION
   configuration setting. The altitude displayed on the X5 web page includes the geoid separation.

   ---

   Written for and tested on ESP IDF v5.1.7

   Needs:
   idf.py add-dependency "espressif/ssd1306^1.0.5"

   Based on:

   mowi_wifi_client (WiFi to Ethernet packet forwarding with BLE based Provisioning)

   This example code is licensed under CC BY-SA 4.0 and OSHW Definition 1.0.

   For more information about MOWI visit https://github.com/septentrio-gnss/mowi/.

   This software is based on following ESP-IDF / ESP-IOT-SOLUTION examples:
   https://github.com/espressif/esp-idf/tree/master/examples/ethernet/eth2ap
   https://github.com/espressif/esp-iot-solution/tree/release/v1.1/examples/eth2wifi
   https://github.com/espressif/esp-idf/tree/master/examples/provisioning 

   This software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/event_groups.h>"
//#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_eth.h"
//#include "esp_eth_driver.h"
//#include "esp_mac.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "dhcpserver/dhcpserver.h"
#include "dhcpserver/dhcpserver_options.h"
#include "ethernet_init.h"

//#include <esp_private/wifi.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/uart_hal.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

//#include "ssd1306.h"
//#include "ssd1306_fonts.h"
#include "fnt_5x8.h"

#include "esp_timer.h"
#include "driver/i2c_master.h"

#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
//#include "nvs_flash.h"
#include "cmd_nvs.h"
#include "cmd_rtk.h"

//#include <lwip/inet.h>
//#include <lwip/ip4_addr.h>

/* The firmware version is updated by the Dockerfile */
static const char *VERSION = "Firmware v0.0.0";
static const char *TAG = "RTK_mosaic-X5_Firmware";
#define PROMPT_STR "RTK_X5"
static const char* prompt;

/** Event group bit: set when Wi-Fi station receives IPv4 (IP_EVENT_STA_GOT_IP). */
#define EXAMPLE_WIFI_STA_GOT_IP_BIT (1U << 0)

static EventGroupHandle_t event_group = NULL;

esp_netif_t *eth_netif = NULL;

//static bool wifi_is_connected = false;
static char ipAddress[25];

int* mode = NULL;
char* ssid = NULL;
char* password = NULL;
char* x5_user = NULL;
char* x5_pass = NULL;
char* esp_log_level = NULL;

const char *cidr2mask(uint8_t cidr) {
    switch (cidr) {
        default:
            return "Invalid";
            break;
        case 32:
            return "255.255.255.255";
            break;
        case 31:
            return "255.255.255.254";
            break;
        case 30:
            return "255.255.255.252";
            break;
        case 29:
            return "255.255.255.248";
            break;
        case 28:
            return "255.255.255.240";
            break;
        case 27:
            return "255.255.255.224";
            break;
        case 26:
            return "255.255.255.192";
            break;
        case 25:
            return "255.255.255.128";
            break;
        case 24:
            return "255.255.255.0";
            break;
        case 23:
            return "255.255.254.0";
            break;
        case 22:
            return "255.255.252.0";
            break;
        case 21:
            return "255.255.248.0";
            break;
        case 20:
            return "255.255.240.0";
            break;
        case 19:
            return "255.255.224.0";
            break;
        case 18:
            return "255.255.192.0";
            break;
        case 17:
            return "255.255.128.0";
            break;
        case 16:
            return "255.255.0.0";
            break;
        case 15:
            return "255.254.0.0";
            break;
        case 14:
            return "255.252.0.0";
            break;
        case 13:
            return "255.248.0.0";
            break;
        case 12:
            return "255.240.0.0";
            break;
        case 11:
            return "255.224.0.0";
            break;
        case 10:
            return "255.192.0.0";
            break;
        case 9:
            return "255.128.0.0";
            break;
        case 8:
            return "255.0.0.0";
            break;
        case 7:
            return "254.0.0.0";
            break;
        case 6:
            return "252.0.0.0";
            break;
        case 5:
            return "248.0.0.0";
            break;
        case 4:
            return "240.0.0.0";
            break;
        case 3:
            return "224.0.0.0";
            break;
        case 2:
            return "192.0.0.0";
            break;
        case 1:
            return "128.0.0.0";
            break;
        case 0:
            return "0.0.0.0";
            break;
    }
}

static volatile bool x5_uart_task_running = true;

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
// Unused IO UART pins
#define CONFIG_RTK_X5_IO_TX_GPIO_PIN (32)
#define CONFIG_RTK_X5_IO_RTS_GPIO_PIN (33)
#define CONFIG_RTK_X5_IO_RX_GPIO_PIN (34)
#define CONFIG_RTK_X5_IO_CTS_GPIO_PIN (35)


/* mosaic-X5 serial commands */
const char MOSAIC_CMD_ESCAPE[] = "SSSSSSSSSSSSSSSSSSSS\n\r";
const char MOSAIC_CMD_ESCAPE_RESPONSE[] = "COM4>";

const char MOSAIC_CMD_DATA_IN_OUT[] = "sdio,COM4,CMD,SBF+NMEA\n\r"; // Input: CMD; Output: SBF+NMEA
const char MOSAIC_CMD_DATA_IN_OUT_RESPONSE[] = "DataInOut";

const char MOSAIC_CMD_NMEA_STREAM10[] = "sno,Stream10,COM4,GGA,sec1\n\r"; // NMEA GGA every second
const char MOSAIC_CMD_NMEA_STREAM10_RESPONSE[] = "NMEAOutput";
const char MOSAIC_CMD_SBF_STREAM10[] = "sso,Stream10,COM4,IPStatus,OnChange\n\r"; // SBF IPStatus (4058) on change
const char MOSAIC_CMD_SBF_STREAM10_RESPONSE[] = "SBFOutput";

const char MOSAIC_CMD_ETHERNET_OFF[] = "seth,off\n\r";
const char MOSAIC_CMD_ETHERNET_OFF_RESPONSE[] = "EthernetMode";
const char MOSAIC_CMD_IP_DHCP[] = "sips,DHCP,,,,,,,1500\n\r";
const char MOSAIC_CMD_IP_DHCP_RESPONSE[] = "IPSettings";
const char MOSAIC_CMD_ETHERNET_ON[] = "seth,on\n\r";
const char MOSAIC_CMD_ETHERNET_ON_RESPONSE[] = "EthernetMode";

const char MOSAIC_CMD_EXE_IPSTATUS_ONCE[] = "esoc,COM4,IPStatus\n\r"; // Execute SBF IPStatus once

const char MOSAIC_CMD_SOFT_RESET[] = "erst,Soft,none\n\r"; // Execute soft reset
const char MOSAIC_CMD_SOFT_RESET_RESPONSE[] = "ResetReceiver";

bool send_command_check_response(const char *command, const char *response, int64_t timeoutMillis, int waitMillis, int tries); // Header

/* I2C OLED */
#define I2C_HOST  0
#define RTK_X5_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define RTK_X5_PIN_NUM_SDA           15
#define RTK_X5_PIN_NUM_SCL           14
#define RTK_X5_PIN_NUM_RST           -1
#define RTK_X5_OLED_HW_ADDR          0x3D

i2c_master_bus_handle_t bus_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;

void print_text(char *txt); // Header

const uint8_t oled_x_chars = 25; // 128 / 5
const uint8_t oled_y_chars = 8;  // 64 / 8
static char oled_text[25][8];
static bool oled_ready = false;
void x5_not_ready(void); // Header
void clear_oled_text(void); // Header
void print_oled(char *txt); // Header
void set_oled(char *txt); // Header
void update_oled(void); // Header
void display_IP(void); // Header

/* Console history - in NVS */
#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

//
//    SBF Message
//
//    |<-- Preamble --->|
//    |                 |
//    +--------+--------+---------+---------+---------+---------+
//    |  SYNC  |  SYNC  |   CRC   |   ID    | Length  | Payload |
//    | 8 bits | 8 bits | 16 bits | 2 bytes | 2 bytes | n bytes |
//    |  0x24  |  0x40  |         |         |         |         |
//    +--------+--------+---------+---------+---------+---------+
//                                |                             |
//                                |<-------- Checksum --------->|
//
//  The generator polynomial for the CRC is the so-called CRC-CCITT
//  polynomial: x16 +x12 +x5 +x0. The CRC is computed in the forward
//  direction using a seed of 0, no reverse and no final XOR.

static const uint16_t ccitt_crc_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

uint16_t ccitt_crc_update(uint16_t crc, const uint8_t data)
{
    uint8_t tbl_idx = ((crc >> 8) ^ data) & 0xff;
    
    crc = (ccitt_crc_table[tbl_idx]) ^ (crc << 8);

    return crc;
}

#define tokenValid ((*token != ',') && (*token != '*') && (*token != 0))
#define remainderValid ((*remainder != ',') && (*remainder != '*') && (*remainder != 0))

/* EVENT HANDLERS */

// Event handler for Ethernet
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    esp_netif_t *eth_netif = (esp_netif_t *)arg;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr));
        ESP_LOGI(TAG, "Ethernet MAC address is currently: %02X:%02X:%02X:%02X:%02X:%02X", 
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        {
            esp_err_t err = esp_netif_dhcps_start(eth_netif);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "DHCP server start: %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "DHCP server started on %s", esp_netif_get_desc(eth_netif));
            }
        }
        break;

    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGE(TAG, "Ethernet Link Down. Restarting...");
        print_oled("Ethernet Link Down");
        print_oled("Restarting...");
        vTaskDelay(pdMS_TO_TICKS(2000));
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
    if (event_base == WIFI_EVENT) {
        ESP_LOGI(TAG, "Wi-Fi Event: base=%s, id=%ld", event_base, event_id);
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Wi-Fi STA started. Connecting...");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "Wi-Fi STA stopped");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Wi-Fi STA connected. Waiting for IP...");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Wi-Fi STA disconnected, retrying...");
            //wifi_is_connected = false;
            gpio_set_level(CONFIG_RTK_X5_WIFI_GPIO_PIN, false);
            esp_wifi_connect();
            break;
        default:
            ESP_LOGW(TAG, "Unhandled Wi-Fi event: id=%ld", event_id);
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        const esp_netif_ip_info_t *ip_info = &event->ip_info;
        esp_netif_t *netif = event->esp_netif;
        esp_netif_dns_info_t dns_info;

        ESP_LOGI(TAG, "Wi-Fi STA Got IP Address");
        ESP_LOGI(TAG, "Event: base=%s, id=%ld", event_base, event_id);
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "STAIP:" IPSTR, IP2STR(&ip_info->ip));
        ESP_LOGI(TAG, "STAMASK:" IPSTR, IP2STR(&ip_info->netmask));
        ESP_LOGI(TAG, "STAGW:" IPSTR, IP2STR(&ip_info->gw));

        // Print DHCP DNS information
        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
            ESP_LOGI(TAG, "DHCP_DNS_MAIN:" IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        }
        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info) == ESP_OK) {
            ESP_LOGI(TAG, "DHCP_DNS_BACKUP:" IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        }

        ESP_LOGI(TAG, "~~~~~~~~~~~");

        xEventGroupSetBits(event_group, EXAMPLE_WIFI_STA_GOT_IP_BIT);

        uint8_t *ptr = (uint8_t *)&ip_info->ip;
        snprintf(ipAddress, sizeof(ipAddress), "IP:   %d.%d.%d.%d", *(ptr + 0), *(ptr + 1), *(ptr + 2), *(ptr + 3));
        //wifi_is_connected = true;
        gpio_set_level(CONFIG_RTK_X5_WIFI_GPIO_PIN, true);
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    esp_netif_t *netif = event->esp_netif;
    esp_netif_dns_info_t dns_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "Event: base=%s, id=%ld", event_base, event_id);
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));

    // Print DNS information
    if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
        ESP_LOGI(TAG, "DHCP_DNS_MAIN:" IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    }
    if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info) == ESP_OK) {
        ESP_LOGI(TAG, "DHCP_DNS_BACKUP:" IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    }

    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

static wifi_auth_mode_t example_wifi_sta_authmode_threshold(void)
{
#if CONFIG_EXAMPLE_ESP_WIFI_STA_AUTHMODE_THRESHOLD_OPEN
    return WIFI_AUTH_OPEN;
#elif CONFIG_EXAMPLE_ESP_WIFI_STA_AUTHMODE_THRESHOLD_WEP
    return WIFI_AUTH_WEP;
#elif CONFIG_EXAMPLE_ESP_WIFI_STA_AUTHMODE_THRESHOLD_WPA_PSK
    return WIFI_AUTH_WPA_PSK;
#elif CONFIG_EXAMPLE_ESP_WIFI_STA_AUTHMODE_THRESHOLD_WPA2_PSK
    return WIFI_AUTH_WPA2_PSK;
#elif CONFIG_EXAMPLE_ESP_WIFI_STA_AUTHMODE_THRESHOLD_WPA_WPA2_PSK
    return WIFI_AUTH_WPA_WPA2_PSK;
#elif CONFIG_EXAMPLE_ESP_WIFI_STA_AUTHMODE_THRESHOLD_WPA3_PSK
    return WIFI_AUTH_WPA3_PSK;
#else
    return WIFI_AUTH_WPA2_PSK;
#endif
}

/** Initialize WiFi STA */
static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = example_wifi_sta_authmode_threshold(),
        },
    };
    strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi STA initialized. SSID: %s", ssid);
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
    // Create a buffer for the incoming data
    // NMEA GPGGA could be close to 100 bytes, depending on the position precision
    // IPStatus (SBF 4058) Rev1 is 88 bytes
    // IPStatus is less frequent than GPGGA
    // We decode GGA from the end of the FIFO
    // We can use GPGGA to 'push' IPStatus to the start of the FIFO
    // We decode SBF from the start of the FIFO
    size_t buf_size = 128 * sizeof(uint8_t);
    static uint8_t *uart_buf = NULL;
    if (uart_buf == NULL) {
        uart_buf = (uint8_t *) malloc(buf_size);
        memset(uart_buf, 0, buf_size);
    }
    static uint8_t *fifo = NULL;
    if (fifo == NULL) {
        fifo = (uint8_t *) malloc(buf_size);
        memset(fifo, 0, buf_size);
    }

    while (true) {
        if (x5_uart_task_running)
        {
            int length = uart_read_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, uart_buf, buf_size, 10/portTICK_PERIOD_MS);
            if (length > 0)
            {
                for (size_t x = 0; x < (size_t)length; x++) // For each byte received
                {
                    uint8_t *ptr1 = fifo;
                    uint8_t *ptr2 = fifo;
                    ptr2++;
                    for (size_t i = 0; i < (buf_size - 1); i++) // Shuffle FIFO along by 1 byte
                        *ptr1++ = *ptr2++;
                    // ptr1 is pointing at fifo[buf_size - 1]
                    ptr2 = uart_buf;
                    ptr2 += x; // Point at the received byte
                    *ptr1 = *ptr2; // Store the byte at the end of the FIFO

                    // Check for a valid SBF message - at the start of the fifo
                    
                    ptr1 = fifo;
                    if (*ptr1 == 0x24) // $
                    {
                        if (*(ptr1 + 1) == 0x40) // @
                        {
                            uint16_t crc = *(ptr1 + 2);
                            crc |= ((uint16_t)*(ptr1 + 3)) << 8;
                            uint16_t id = *(ptr1 + 4);
                            id |= ((uint16_t)*(ptr1 + 5)) << 8;
                            uint16_t len = *(ptr1 + 6);
                            len |= ((uint16_t)*(ptr1 + 7)) << 8;

                            if (len < (buf_size - 1))
                            {
                                uint16_t actualCrc = 0;
                                for (size_t i = 4; i < len; i ++)
                                {
                                    actualCrc = ccitt_crc_update(actualCrc, *(ptr1 + i));
                                }
                                if (actualCrc == crc)
                                {
#if CONFIG_EXAMPLE_RTK_X5_VERBOSE_LOG
                                    ESP_LOGI(TAG, "Valid SBF Message: ID %d", (id & 0x1FFF));
#endif
                                    if ((id & 0x1FFF) == 4058)
                                    {
                                        // MACAddress (6 bytes) is in bytes 14-19
                                        // IPAddress (4 bytes) is in bytes 32-35
                                        snprintf(ipAddress, sizeof(ipAddress), "IP:   %d.%d.%d.%d", *(ptr1 + 32), *(ptr1 + 33), *(ptr1 + 34), *(ptr1 + 35));
                                        ESP_LOGI(TAG, "IPStatus MAC: %02X:%02X:%02X:%02X:%02X:%02X IPAddress: %d.%d.%d.%d Gateway: %d.%d.%d.%d Subnet Mask: %s", 
                                                 *(ptr1 + 14), *(ptr1 + 15), *(ptr1 + 16), *(ptr1 + 17), *(ptr1 + 18), *(ptr1 + 19),
                                                 *(ptr1 + 32), *(ptr1 + 33), *(ptr1 + 34), *(ptr1 + 35),
                                                 *(ptr1 + 48), *(ptr1 + 49), *(ptr1 + 50), *(ptr1 + 51),
                                                 cidr2mask(*(ptr1 + 52)));

                                    }
                                }
                                else
                                    ESP_LOGW(TAG, "Invalid SBF Message: ID %d", (id & 0x1FFF));
                            }
                        }
                    }

                    // Check for a valid NMEA message at the end of the fifo

                    // Count bytes since the the previous dollar
                    static size_t lastDollar = 0;
                    if (*(ptr1 + buf_size - 1) == '$')
                        lastDollar = 0;
                    else
                        lastDollar++;

                    // Check for the end of a NMEA message (*, CR, LF) at the end of the fifo
                    if ((lastDollar < (buf_size - 1)) && (*(ptr1 + buf_size - 5) == '*') && (*(ptr1 + buf_size - 2) == '\r') && (*(ptr1 + buf_size - 1) == '\n'))
                    {
                        size_t msgStart = buf_size - (lastDollar + 1);
                        if (*(ptr1 + msgStart) == '$')
                        {
                            // Check the checksum
                            size_t i = msgStart + 1;
                            uint8_t csum = 0;
                            while (i < (buf_size - 5))
                            {
                                csum = csum ^ *(ptr1 + i);
                                i++;
                            }
                            uint8_t csum1 = 0;
                            csum1 = *(ptr1 + buf_size - 4);
                            if ((csum1 >= '0') && (csum1 <= '9'))
                                csum1 = csum1 - '0';
                            else if ((csum1 >= 'A') && (csum1 <= 'F'))
                                csum1 = 10 + csum1 - 'A';
                            else if ((csum1 >= 'a') && (csum1 <= 'f'))
                                csum1 = 10 + csum1 - 'a';
                            else
                                csum1 = 0;
                            uint8_t csum2 = 0;
                            csum2 = *(ptr1 + buf_size - 3);
                            if ((csum2 >= '0') && (csum2 <= '9'))
                                csum2 = csum2 - '0';
                            else if ((csum2 >= 'A') && (csum2 <= 'F'))
                                csum2 = 10 + csum2 - 'A';
                            else if ((csum2 >= 'a') && (csum2 <= 'f'))
                                csum2 = 10 + csum2 - 'a';
                            else
                                csum2 = 0;
                            csum1 <<= 4;
                            csum1 |= csum2;
                            if (csum == csum1)
                            {
                                if  ((*(ptr1 + msgStart + 1) == 'G') && (*(ptr1 + msgStart + 3) == 'G') && (*(ptr1 + msgStart + 4) == 'G') && (*(ptr1 + msgStart + 5) == 'A'))
                                {
                                    do {
                                        char *remainder = (char *)(ptr1 + msgStart);
                                        char *token = strtok_r(remainder, ",", &remainder); // $GPGGA
                                        if (!remainderValid) break;
                                        char line[25];
                                        token = strtok_r(remainder, ",", &remainder); // Time
                                        if (!remainderValid) break;
                                        char theTime[11];
                                        snprintf(theTime, sizeof(theTime), "%c%c:%c%c:%c%c.%c", *(token), *(token + 1), *(token + 2), *(token + 3), *(token + 4), *(token + 5), *(token + 7));
                                        snprintf(line, sizeof(line), "Time: %s", tokenValid ? theTime : "?");
                                        set_oled(line);
                                        
                                        token = strtok_r(remainder, ",", &remainder); // Latitude
                                        if (!remainderValid) break;
                                        char theLat[14];
                                        float secs = 0;
                                        float multiplier = 0.1;
                                        int places = 1;
                                        while ((*(token + 4 + places) != ',') && (places < 8))
                                        {
                                            secs += ((float)(*(token + 4 + places) - '0')) * multiplier;
                                            multiplier /= 10.0;
                                            places++;
                                        }
                                        secs *= 60.0;
                                        snprintf(theLat, sizeof(theLat), "%c%c %c%c %02.4f", *(token), *(token + 1), *(token + 2), *(token + 3), secs);
                                        snprintf(line, sizeof(line), "Lat:   %s %c", tokenValid ? theLat : "?", remainderValid ? *remainder : '?');
                                        set_oled(line);
                                        
                                        token = strtok_r(remainder, ",", &remainder); // N/S
                                        if (!remainderValid) break;
                                        token = strtok_r(remainder, ",", &remainder); // Longitude
                                        if (!remainderValid) break;
                                        char theLon[15];
                                        secs = 0;
                                        multiplier = 0.1;
                                        places = 1;
                                        while ((*(token + 5 + places) != ',') && (places < 8))
                                        {
                                            secs += ((float)(*(token + 5 + places) - '0')) * multiplier;
                                            multiplier /= 10.0;
                                            places++;
                                        }
                                        secs *= 60.0;
                                        snprintf(theLon, sizeof(theLon), "%c%c%c %c%c %02.4f", *(token), *(token + 1), *(token + 2), *(token + 3), *(token + 4), secs);
                                        snprintf(line, sizeof(line), "Long: %s %c", tokenValid ? theLon : "?", remainderValid ? *remainder : '?');
                                        set_oled(line);
                                        
                                        token = strtok_r(remainder, ",", &remainder); // E/W
                                        if (!remainderValid) break;
                                        token = strtok_r(remainder, ",", &remainder); // Fix
                                        if (!remainderValid) break;
                                        char fixType[17] = { 0 };
                                        if (tokenValid) {
                                            switch (*token) {
                                                default:
                                                    snprintf(fixType, sizeof(fixType), "Unknown");
                                                    break;
                                                case '0':
                                                    snprintf(fixType, sizeof(fixType), "Invalid");
                                                    break;
                                                case '1':
                                                    snprintf(fixType, sizeof(fixType), "Autonomous");
                                                    break;
                                                case '2':
                                                    snprintf(fixType, sizeof(fixType), "Differential");
                                                    break;
                                                case '3':
                                                    snprintf(fixType, sizeof(fixType), "PPS");
                                                    break;
                                                case '4':
                                                    snprintf(fixType, sizeof(fixType), "RTK Fixed");
                                                    break;
                                                case '5':
                                                    snprintf(fixType, sizeof(fixType), "RTK Float");
                                                    break;
                                                case '6':
                                                    snprintf(fixType, sizeof(fixType), "Dead Reckoning");
                                                    break;
                                                case '7':
                                                    snprintf(fixType, sizeof(fixType), "Manual");
                                                    break;
                                                case '8':
                                                    snprintf(fixType, sizeof(fixType), "Simulation");
                                                    break;
                                                case '9':
                                                    snprintf(fixType, sizeof(fixType), "WAAS");
                                                    break;
                                            }
                                        }
                                        snprintf(line, sizeof(line), "Fix:  %s %s", tokenValid ? token : "?", fixType);
                                        set_oled(line);
                                        
                                        token = strtok_r(remainder, ",", &remainder); // Num Sat
                                        if (!remainderValid) break;
                                        snprintf(line, sizeof(line), "Sat:  %s", tokenValid ? token : "?");
                                        set_oled(line);
                                        
                                        token = strtok_r(remainder, ",", &remainder); // HDOP
                                        if (!remainderValid) break;
                                        snprintf(line, sizeof(line), "HDOP: %s", tokenValid ? token : "?");
                                        set_oled(line);
                                        
                                        token = strtok_r(remainder, ",", &remainder); // Alt (Elev)
                                        if (!remainderValid) break;
                                        float alt = 0.0;
                                        int digits = 0;
                                        int neg = 0;
                                        if (*token == '-')
                                            neg = 1;
                                        while ((*(token + neg + digits) != '.') && (digits < 7))
                                        {
                                            alt *= 10.0;
                                            alt += ((float)(*(token + neg + digits) - '0'));
                                            digits++;
                                        }
                                        if (digits == 7) break; // Something has gone horribly wrong...
                                        multiplier = 0.1;
                                        places = 1;
                                        while ((*(token + neg + digits + places) != ',') && (places < 8))
                                        {
                                            alt += ((float)(*(token + neg + digits + places) - '0')) * multiplier;
                                            multiplier /= 10.0;
                                            places++;
                                        }
                                        if (neg == 1)
                                            alt *= -1.0;
                                        
                                        token = strtok_r(remainder, ",", &remainder); // M
                                        if (!remainderValid) break;
                                        
                                        token = strtok_r(remainder, ",", &remainder); // Geoid
                                        if (!remainderValid) break;
                                        float geoid = 0.0;
                                        digits = 0;
                                        neg = 0;
                                        if (*token == '-')
                                            neg = 1;
                                        while ((*(token + neg + digits) != '.') && (digits < 7))
                                        {
                                            geoid *= 10.0;
                                            geoid += ((float)(*(token + neg + digits) - '0'));
                                            digits++;
                                        }
                                        if (digits == 7) break; // Something has gone horribly wrong...
                                        multiplier = 0.1;
                                        places = 1;
                                        while ((*(token + neg + digits + places) != ',') && (places < 8))
                                        {
                                            geoid += ((float)(*(token + neg + digits + places) - '0')) * multiplier;
                                            multiplier /= 10.0;
                                            places++;
                                        }
                                        if (neg == 1)
                                            geoid *= -1.0;
#if CONFIG_EXAMPLE_RTK_X5_DISPLAY_ALT_WITH_GEOID_SEPARATION
                                        alt += geoid;
#endif
                                        char theAlt[15];
                                        snprintf(theAlt, sizeof(theAlt), "%0.3f", alt);
                                        snprintf(line, sizeof(line), "Alt:  %s %c", tokenValid ? theAlt : "?", remainderValid ? *remainder : '?');
                                        set_oled(line);

                                        // token = strtok_r(remainder, ",", &remainder); // M
                                        // if (!remainderValid) break;
                                        
                                        // token = strtok_r(remainder, ",", &remainder); // Age
                                        // if (!remainderValid) break;
                                        
                                        // snprintf(line, sizeof(line), "Age:  %s", tokenValid ? token : "?");
                                        // set_oled(line);
                                        
                                        set_oled(ipAddress); // Show the stored IP address
                                        update_oled();
                                    } while (0); // This is just a trick to execute the do loop once - and allow the code to break out early
                                }
                            }
                        }
                    }
                }
            }
        }

        vTaskDelay(1); // Yield - to allow watchdog to be reset

    }

    free(uart_buf);
    uart_buf = NULL;
    free(fifo);
    fifo = NULL;
    vTaskDelete(NULL);
}

static void production_test_task(void *args)
{
    while (true) {
        gpio_set_level(CONFIG_RTK_X5_IO_TX_GPIO_PIN, gpio_get_level(CONFIG_RTK_X5_IO_RX_GPIO_PIN));
        gpio_set_level(CONFIG_RTK_X5_IO_RTS_GPIO_PIN, gpio_get_level(CONFIG_RTK_X5_IO_CTS_GPIO_PIN));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}


/* INITIALIZATION */

void x5_not_ready(void)
{
    ESP_LOGE(TAG, "mosaic-X5 not ready. Restarting...");
    print_oled("mosaic-X5 not ready");
    print_oled("Restarting...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

void initialize_leds(void)
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

void initialize_uart(void)
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

bool send_command_check_response(const char *command, const char *response, int64_t timeoutMillis, int waitMillis, int tries)
{
    // Create a FIFO for incoming UART bytes
    size_t buf_size = strlen(response) * sizeof(uint8_t);
    uint8_t *uart_buf = (uint8_t *)malloc(buf_size);
    memset(uart_buf, 0, buf_size);
    uint8_t *fifo = (uint8_t *)malloc(buf_size);
    memset(fifo, 0, buf_size);

    // Create a large buffer to hold all bytes received from the X5 for ESP_LOGI diagnostics
    const size_t large_buf_size = 512;
    uint8_t *large_buf = (uint8_t *)malloc(large_buf_size);
    memset(large_buf, 0, large_buf_size);
    uint8_t *large_ptr = large_buf;
    
    int64_t timeMicros = esp_timer_get_time();
    int try = 0;
    bool keepGoing = true;

    while (keepGoing && (try < tries)) {
        ESP_LOGI(TAG, "Sending: %s", command);

        ESP_ERROR_CHECK(uart_flush_input(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM));
        uart_write_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, (const char*)command, strlen(command));

        while (keepGoing && (esp_timer_get_time() < (timeMicros + (timeoutMillis * 1000))))
        {
            int length = uart_read_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, uart_buf, buf_size, 10/portTICK_PERIOD_MS);
            if (length > 0)
            {
                for (size_t x = 0; x < (size_t)length; x++) // For each byte received
                {
                    uint8_t *ptr1 = fifo;
                    uint8_t *ptr2 = fifo;
                    ptr2++;
                    for (size_t i = 0; i < (buf_size - 1); i++) // Shuffle FIFO along by 1 byte
                        *ptr1++ = *ptr2++;
                    // ptr1 is pointing at fifo[buf_size - 1]
                    ptr2 = uart_buf;
                    ptr2 += x; // Point at the received byte
                    *ptr1 = *ptr2; // Store the byte at the end of the FIFO

                    if ((large_ptr - large_buf) < large_buf_size)
                    {
                        *large_ptr = *ptr2;
                        large_ptr++;
                    }

                    if (memcmp(fifo, (uint8_t *)response, strlen(response)) == 0) // Compare the FIFO to the response
                        keepGoing = false; // Match found
                }
            }

            vTaskDelay(1); // Yield - to allow watchdog to be reset
        }

        try++;
        timeMicros = esp_timer_get_time(); // Update the timer

        while (esp_timer_get_time() < (timeMicros + (waitMillis * 1000))) // Wait for the remaining bytes to arrive
        {
            int length = uart_read_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, uart_buf, buf_size, 10/portTICK_PERIOD_MS);
            if (length > 0)
            {
                for (size_t x = 0; x < (size_t)length; x++) // For each byte received
                {
                    uint8_t *ptr2 = uart_buf;
                    ptr2 += x; // Point at the received byte

                    if ((large_ptr - large_buf) < large_buf_size)
                    {
                        *large_ptr = *ptr2;
                        large_ptr++;
                    }
                }
            }

            vTaskDelay(1); // Yield - to allow watchdog to be reset
        }

        if (large_ptr > large_buf)
        {
            ESP_LOGI(TAG, "Received: %s", large_buf);
            memset(large_buf, 0, large_buf_size);
            large_ptr = large_buf;
        }
    }

    free(large_buf);
    free(uart_buf);
    free(fifo);

    return (keepGoing == false);
}

void initialize_ethernet(void)
{
    ESP_LOGI(TAG, "Initializing Ethernet");
    print_oled("Initializing Ethernet");

    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles = NULL;
    ESP_ERROR_CHECK(ethernet_init_all(&eth_handles, &eth_port_cnt));

    if (eth_port_cnt == 0) {
        ESP_LOGE(TAG, "No Ethernet interface initialized");
        return;
    }
    if (eth_port_cnt > 1) {
        ESP_LOGW(TAG, "%u Ethernet interfaces initialized; this example only uses the first one", eth_port_cnt);
    }

    // Static IP configuration for Ethernet gateway
    static esp_netif_ip_info_t s_eth_gw_ip_info;
    memset(&s_eth_gw_ip_info, 0, sizeof(esp_netif_ip_info_t));

    if (esp_netif_str_to_ip4(CONFIG_EXAMPLE_ETH_GATEWAY_IP_ADDR, &s_eth_gw_ip_info.ip) != ESP_OK) {
        ESP_LOGE(TAG, "Invalid IP address: %s", CONFIG_EXAMPLE_ETH_GATEWAY_IP_ADDR);
        return;
    }
    if (esp_netif_str_to_ip4(CONFIG_EXAMPLE_ETH_GATEWAY_NETMASK, &s_eth_gw_ip_info.netmask) != ESP_OK) {
        ESP_LOGE(TAG, "Invalid netmask: %s", CONFIG_EXAMPLE_ETH_GATEWAY_NETMASK);
        return;
    }
    if (esp_netif_str_to_ip4(CONFIG_EXAMPLE_ETH_GATEWAY_GW, &s_eth_gw_ip_info.gw) != ESP_OK) {
        ESP_LOGE(TAG, "Invalid gateway: %s", CONFIG_EXAMPLE_ETH_GATEWAY_GW);
        return;
    }

    static esp_netif_inherent_config_t inherent_config = {
        .flags = ESP_NETIF_DHCP_SERVER,
        ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac)
        .ip_info = &s_eth_gw_ip_info,
        .get_ip_event = 0,
        .lost_ip_event = 0,
        .if_key = "ETH_GW",
        .if_desc = "eth_gw",
        .route_prio = 50,
        .bridge_info = NULL
    };

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    cfg.base = &inherent_config;

    if (eth_netif == NULL)
        eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(eth_netif != NULL ? ESP_OK : ESP_FAIL);

    /* Configure DHCP server options */
    // Set lease time
    uint32_t lease_time = CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_LEASE_TIME;
    esp_err_t err = esp_netif_dhcps_option(eth_netif, ESP_NETIF_OP_SET, IP_ADDRESS_LEASE_TIME, &lease_time, sizeof(lease_time));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set DHCP lease time: %s", esp_err_to_name(err));
    }

    // Configure IP address pool
    dhcps_lease_t dhcp_lease;
    memset(&dhcp_lease, 0, sizeof(dhcps_lease_t));
    dhcp_lease.enable = true;
    if (esp_netif_str_to_ip4(CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_START_ADDR, (esp_ip4_addr_t *)&dhcp_lease.start_ip) != ESP_OK) {
        ESP_LOGW(TAG, "Invalid DHCP start address: %s, using default pool", CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_START_ADDR);
    } else if (esp_netif_str_to_ip4(CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_END_ADDR, (esp_ip4_addr_t *)&dhcp_lease.end_ip) != ESP_OK) {
        ESP_LOGW(TAG, "Invalid DHCP end address: %s, using default pool", CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_END_ADDR);
    } else {
        err = esp_netif_dhcps_option(eth_netif, ESP_NETIF_OP_SET, REQUESTED_IP_ADDRESS, &dhcp_lease, sizeof(dhcps_lease_t));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to set DHCP IP pool: %s", esp_err_to_name(err));
        }
    }

    // Configure DNS servers if enabled
#if CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_ENABLE_DNS
    dhcps_offer_t dhcps_dns_value = OFFER_DNS;
    err = esp_netif_dhcps_option(eth_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to enable DNS in DHCP offers: %s", esp_err_to_name(err));
    } else {
        // Set DNS server addresses (will be updated from WiFi STA DNS if available)
        esp_netif_dns_info_t dns_info = {};
        dns_info.ip.type = ESP_IPADDR_TYPE_V4;

        if (esp_netif_str_to_ip4(CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_DNS_MAIN, &dns_info.ip.u_addr.ip4) == ESP_OK) {
            err = esp_netif_set_dns_info(eth_netif, ESP_NETIF_DNS_MAIN, &dns_info);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to set primary DNS: %s", esp_err_to_name(err));
            }
        }

        if (esp_netif_str_to_ip4(CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_DNS_BACKUP, &dns_info.ip.u_addr.ip4) == ESP_OK) {
            err = esp_netif_set_dns_info(eth_netif, ESP_NETIF_DNS_BACKUP, &dns_info);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to set backup DNS: %s", esp_err_to_name(err));
            }
        }
    }
#endif // CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_ENABLE_DNS

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, eth_netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));
    ESP_ERROR_CHECK(esp_eth_start(eth_handles[0]));

    ESP_LOGI(TAG, "Ethernet gateway ready (static IP: " IPSTR ", netmask: " IPSTR "). DHCP starts when link is up.",
             IP2STR(&s_eth_gw_ip_info.ip), IP2STR(&s_eth_gw_ip_info.netmask));



    ESP_LOGI(TAG, "Configuring Mosaic Ethernet DHCP");
    print_oled("Configure Ethernet DHCP");

    // Disable Mosaic Ethernet - iterate to initialize connection
    // if (!send_command_check_response(MOSAIC_CMD_ETHERNET_OFF, MOSAIC_CMD_ETHERNET_OFF_RESPONSE, 2000, 50, 5))
    // {
    //     ESP_LOGE(TAG, "Disable Mosaic Ethernet response failed");
    //     x5_not_ready();
    // }

    // Set Mosaic Ethernet DHCP with MTU
    if (!send_command_check_response(MOSAIC_CMD_IP_DHCP, MOSAIC_CMD_IP_DHCP_RESPONSE, 2000, 50, 5))
    {
        ESP_LOGE(TAG, "Set Mosaic Ethernet DHCP response failed");
        x5_not_ready();
    }

    // Enable Mosaic Ethernet
    if (!send_command_check_response(MOSAIC_CMD_ETHERNET_ON, MOSAIC_CMD_ETHERNET_ON_RESPONSE, 2000, 50, 5))
    {
        ESP_LOGE(TAG, "Enable Mosaic Ethernet response failed");
        x5_not_ready();
    }

    // Ethernet configured
    ESP_LOGI(TAG, "Mosaic Ethernet DHCP configured");
    print_oled("DHCP Configured");
}

void initialize_wifi(void)
{
    ESP_LOGI(TAG, "Starting WiFi STA. Connecting to %s", ssid);
    print_oled("Starting WiFi STA");
    print_oled("Connecting to:");
    print_oled(ssid);

    // Initialize WiFi STA
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(sta_netif != NULL ? ESP_OK : ESP_FAIL);
    wifi_init_sta();

    // Wait for WiFi STA to get IP address
    xEventGroupWaitBits(event_group, EXAMPLE_WIFI_STA_GOT_IP_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    // Update Ethernet gateway DNS with Wi-Fi STA DNS if enabled
#if CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_ENABLE_DNS
    ESP_ERROR_CHECK(eth_netif != NULL ? ESP_OK : ESP_FAIL);
    esp_netif_dns_info_t sta_dns_info;
    if (esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &sta_dns_info) == ESP_OK) {
        esp_netif_dns_info_t eth_dns_info = {};
        eth_dns_info.ip.type = ESP_IPADDR_TYPE_V4;
        eth_dns_info.ip.u_addr.ip4 = sta_dns_info.ip.u_addr.ip4;
        esp_err_t err = esp_netif_set_dns_info(eth_netif, ESP_NETIF_DNS_MAIN, &eth_dns_info);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Updated Ethernet gateway DNS with Wi-Fi STA DNS: " IPSTR, IP2STR(&sta_dns_info.ip.u_addr.ip4));
            print_oled("Ethernet DNS updated");
        }
    }
#endif // CONFIG_EXAMPLE_ETH_GATEWAY_DHCP_ENABLE_DNS

#if CONFIG_LWIP_IPV4_NAPT
    // Setup NAPT (Network Address Port Translation) if enabled
    ESP_ERROR_CHECK(esp_netif_napt_enable(eth_netif));
    ESP_LOGI(TAG, "NAPT enabled on Ethernet gateway");
    print_oled("NAPT enabled");
#endif
}

void initialize_i2c(void)
{
    ESP_LOGI(TAG, "Initialize I2C bus");

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = RTK_X5_PIN_NUM_SCL,
        .sda_io_num = RTK_X5_PIN_NUM_SDA,
        .glitch_ignore_cnt = 4,
        .intr_priority = 0,
        .trans_queue_depth = 0, // no tx queue, transmit using blocking mode
        .flags = {
            .enable_internal_pullup = true,
            .allow_pd = false,
        }
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
}

void initialize_oled(void)
{
    ESP_LOGI(TAG, "OLED initialization");

    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = RTK_X5_OLED_HW_ADDR,
        .scl_speed_hz = RTK_X5_LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1, // According to SSD1306 datasheet
        .dc_bit_offset = 6,       // According to SSD1306 datasheet
        .lcd_cmd_bits = 8,        // According to SSD1306 datasheet
        .lcd_param_bits = 8,      // According to SSD1306 datasheet
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .flags = {
            .dc_low_on_data = false, // According to SSD1306 datasheet, DC=0 means command, DC=1 means data
            .disable_control_phase = false, // Control phase is used
        },
        .transaction_timeout_ms = 0, // 0 keeps the legacy infinite wait behavior
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR, // SSD1306 is monochrome, so RGB order doesn't matter
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 1, // SSD1306 is monochrome, so 1 bit per pixel
        .reset_gpio_num = GPIO_NUM_NC,
        .vendor_config = NULL,
        .flags = {
            .reset_active_high = false, // SSD1306 reset is active low
        }
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // turn on display
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    oled_ready = true;

    clear_oled_text();
    print_oled("RTK mosaic-X5 starting");
    print_oled((char *)VERSION);
}


void initialize_filesystem(void)
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

void initialize_nvs(void)
{
    ESP_LOGI(TAG, "Initializing NVS");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void initialize_console(void)
{
    ESP_LOGI(TAG, "Initializing console");
    print_oled("Initializing console");

    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    uart_vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    uart_vfs_dev_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

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
    uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
            .hint_color = atoi(LOG_COLOR_I)
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

void start_console(void)
{
    ESP_LOGI(TAG, "Starting console");
    print_oled("Starting console");

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;

    printf("\nRTK mosaic-X5 ESP-IDF console\n");
    printf(VERSION);
    printf("\n\nType 'help' to get the list of commands.\n"
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

    BaseType_t ret = xTaskCreate(console_task, "console_task", 4096, NULL, (tskIDLE_PRIORITY + 1), NULL);
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Create console task failed");
    }
}

void initialize_x5_uart_task(void)
{
    ESP_LOGI(TAG, "Initializing mosaic-X5 UART task");

    BaseType_t ret = xTaskCreatePinnedToCore(x5_uart_task, "x5_uart_task", 4096, NULL, (tskIDLE_PRIORITY + 2), NULL, 1);
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Create mosaic-x5 UART task failed");
    }

    ESP_LOGI(TAG, "mosaic-X5 UART task initialized");
}

void initialize_X5(void)
{
    // Send the escape sequence
    ESP_LOGI(TAG, "Sending escape sequence to mosaic-X5");
    print_oled("Sending escape sequence");
    if (!send_command_check_response(MOSAIC_CMD_ESCAPE, MOSAIC_CMD_ESCAPE_RESPONSE, 2000, 50, 30))
    {
        ESP_LOGE(TAG, "Escape sequence not acknowledged");
        print_oled("Escape sequence fail");
        ESP_LOGE(TAG, "Sending soft reset");
        print_oled("Sending soft reset");
        
        send_command_check_response(MOSAIC_CMD_SOFT_RESET, MOSAIC_CMD_SOFT_RESET_RESPONSE, 2000, 50, 1);

        ESP_LOGI(TAG, "Sending escape sequence to mosaic-X5");
        print_oled("Sending escape sequence");
        if (!send_command_check_response(MOSAIC_CMD_ESCAPE, MOSAIC_CMD_ESCAPE_RESPONSE, 2000, 50, 30))
        {
            ESP_LOGE(TAG, "Escape sequence not acknowledged");
            print_oled("Escape sequence fail");
            x5_not_ready();
        }
    }

    // Attempt X5 login using username and password
    if ((strlen(x5_user) > 0) && (strlen(x5_pass) > 0))
    {
        ESP_LOGI(TAG, "Attempting mosaic-X5 login");
        print_oled("Attempting login");
        char commandStr[100];
        sprintf(commandStr, "login,%s,%s\n\r", x5_user, x5_pass);
        if (!send_command_check_response((const char *)&commandStr[0], "LogIn", 2000, 50, 5))
        {
            ESP_LOGE(TAG, "LogIn not acknowledged");
            print_oled("LogIn not ackd");
            ESP_LOGE(TAG, "Sending soft reset");
            print_oled("Sending soft reset");
            
            send_command_check_response(MOSAIC_CMD_SOFT_RESET, MOSAIC_CMD_SOFT_RESET_RESPONSE, 2000, 50, 1);

            ESP_LOGI(TAG, "Sending escape sequence to mosaic-X5");
            print_oled("Sending escape sequence");
            if (!send_command_check_response(MOSAIC_CMD_ESCAPE, MOSAIC_CMD_ESCAPE_RESPONSE, 2000, 50, 30))
            {
                ESP_LOGE(TAG, "Escape sequence not acknowledged");
                print_oled("Escape sequence fail");
                // Don't call x5_not_ready();
            }

            ESP_LOGI(TAG, "Attempting mosaic-X5 login");
            print_oled("Attempting login");
            if (!send_command_check_response((const char *)&commandStr[0], "LogIn", 2000, 50, 5))
            {
                ESP_LOGE(TAG, "LogIn not acknowledged");
                print_oled("LogIn not ackd");
                // Don't call x5_not_ready();
            }
        }
    }

    // Ensure SBF and NMEA are enabled
    ESP_LOGI(TAG, "Configuring mosaic-X5 DataInOut");
    print_oled("Configuring DataInOut");
    if (!send_command_check_response(MOSAIC_CMD_DATA_IN_OUT, MOSAIC_CMD_DATA_IN_OUT_RESPONSE, 2000, 50, 5))
    {
        ESP_LOGE(TAG, "DataInOut not acknowledged");
        print_oled("DataInOut not ackd");
        ESP_LOGE(TAG, "Sending soft reset");
        print_oled("Sending soft reset");
        
        send_command_check_response(MOSAIC_CMD_SOFT_RESET, MOSAIC_CMD_SOFT_RESET_RESPONSE, 2000, 50, 1);

        ESP_LOGI(TAG, "Sending escape sequence to mosaic-X5");
        print_oled("Sending escape sequence");
        if (!send_command_check_response(MOSAIC_CMD_ESCAPE, MOSAIC_CMD_ESCAPE_RESPONSE, 2000, 50, 30))
        {
            ESP_LOGE(TAG, "Escape sequence not acknowledged");
            print_oled("Escape sequence fail");
            x5_not_ready();
        }

        ESP_LOGI(TAG, "Configuring mosaic-X5 DataInOut");
        print_oled("Configuring DataInOut");
        if (!send_command_check_response(MOSAIC_CMD_DATA_IN_OUT, MOSAIC_CMD_DATA_IN_OUT_RESPONSE, 2000, 50, 5))
        {
            ESP_LOGE(TAG, "DataInOut not acknowledged");
            print_oled("DataInOut not ackd");
            x5_not_ready();
        }
    }

    // Ensure GPGGA is enabled
    ESP_LOGI(TAG, "Configuring mosaic-X5 GPGGA (NMEA Stream10)");
    print_oled("Configuring GPGGA");
    if (!send_command_check_response(MOSAIC_CMD_NMEA_STREAM10, MOSAIC_CMD_NMEA_STREAM10_RESPONSE, 2000, 50, 5))
    {
        ESP_LOGE(TAG, "NMEA Stream10 not acknowledged");
        x5_not_ready();
    }

    // Ensure IPStatus is enabled
    ESP_LOGI(TAG, "Configuring mosaic-X5 IPStatus (SBF Stream10)");
    print_oled("Configuring IPStatus");
    if (!send_command_check_response(MOSAIC_CMD_SBF_STREAM10, MOSAIC_CMD_SBF_STREAM10_RESPONSE, 2000, 50, 5))
    {
        ESP_LOGE(TAG, "SBF Stream10 not acknowledged");
        x5_not_ready();
    }    

    // Initialize the displayed IP address. Will be updated by the arrival of SBF IPStatus or STA Connect
    snprintf(ipAddress, sizeof(ipAddress), "IP:   0.0.0.0");
}

// Test the unused TX/RX/RTS/CTS pins: TX follows RX, RTS follows CTS
void production_test(void)
{

    // Set unused pins to inputs and outputs
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << CONFIG_RTK_X5_IO_TX_GPIO_PIN;
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = false;
    gpio_config(&io_conf);
    io_conf.pin_bit_mask = 1ULL << CONFIG_RTK_X5_IO_RTS_GPIO_PIN;
    gpio_config(&io_conf);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << CONFIG_RTK_X5_IO_RX_GPIO_PIN;
    gpio_config(&io_conf);
    io_conf.pin_bit_mask = 1ULL << CONFIG_RTK_X5_IO_CTS_GPIO_PIN;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Initializing production test task");

    BaseType_t ret = xTaskCreate(production_test_task, "production_test_task", 2048, NULL, (tskIDLE_PRIORITY + 1), NULL);
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Create production test task failed");
    }
}

/* Very simple 8-line scrolling text console on the OLED */

void clear_oled_text(void) {
    for (uint8_t y = 0; y < oled_y_chars; y++)
        for (uint8_t x = 0; x < oled_x_chars; x++)
            oled_text[x][y] = ' ';
}

void ssd1306_draw_58char(uint8_t chXpos, uint8_t chYpos, uint8_t chChar)
{
    uint8_t i;
    uint8_t chTemp;

    for (i = 0; i < FONT_5X7_WIDTH; i++) {
        chTemp = font5x7_data[(uint16_t)chChar * FONT_5X7_WIDTH + i];
        esp_lcd_panel_draw_bitmap(panel_handle, chXpos, chYpos, chXpos + 1, chYpos + FONT_5X7_HEIGHT, (const void *)&chTemp);
    }
}

void print_oled(char *txt) {
    set_oled(txt);
    update_oled();
}

void set_oled(char *txt) {
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
}

void update_oled(void) {
    if (oled_ready) {
        // Print the characters
        uint8_t ypos = 0;
        for (uint8_t y = 0; y < oled_y_chars; y++) {
            uint8_t xpos = 0;
            for (uint8_t x = 0; x < oled_x_chars; x++) {
                ssd1306_draw_58char(xpos, ypos, oled_text[x][y]);
                xpos += 5;
            }
            ypos += 8;
        }
    }
    else {
        ESP_LOGE(TAG, "OLED not ready");
    }
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
        // Default to Ethernet - leave X5 Ethernet configuration unchanged
        param_set_value_int(&mode, 1);
    }
    get_config_param_str("ssid", &ssid);
    if (ssid == NULL) {
        param_set_value_str(&ssid, "sparkfun-guest"); // Default SSID
    }
    get_config_param_str("password", &password);
    if (password == NULL) {
        param_set_value_str(&password, ""); // Default password
    }
    get_config_param_str("x5_user", &x5_user);
    if (x5_user == NULL) {
        param_set_value_str(&x5_user, ""); // Default X5 username
    }
    get_config_param_str("x5_pass", &x5_pass);
    if (x5_pass == NULL) {
        param_set_value_str(&x5_pass, ""); // Default X5 password
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

    // SparkFun production test for the unused TX/RX/RTS/CTS pins
    production_test();

    // Check the X5 is communicating and that the port is configured correctly
    initialize_X5();

    // Execute SBF IPStatus once - to force update of ipAddress
    // The reply will be handled by the x5_uart_task
    uart_write_bytes(CONFIG_RTK_X5_MOSAIC_UART_PORT_NUM, MOSAIC_CMD_EXE_IPSTATUS_ONCE, strlen(MOSAIC_CMD_EXE_IPSTATUS_ONCE));

    x5_uart_task_running = true; // Ensure the uart task is unpaused

    initialize_x5_uart_task(); // Start the uart task - parse incoming GPGGA and IPStatus messages

    // Check the mode
    if (*mode == 1) {
        ESP_LOGI(TAG, "Firmware is in mode 1: Ethernet");
        print_oled("Mode 1: Ethernet");
    }
    else
    {
        ESP_LOGI(TAG, "Firmware is in mode 2: WiFi");
        print_oled("Mode 2: WiFi");

        x5_uart_task_running = false; // Pause the uart task so send_command_check_response can receive the response

        // Initialize main peripherals
        initialize_ethernet();
        initialize_wifi();

        x5_uart_task_running = true; // Unpause the uart task
    }

    print_oled("Waiting for signal");
}
