idf_component_register(SRCS "RTK_mosaic-X5_Firmware.c" ethernet_iface.c
                       INCLUDE_DIRS "."
                       REQUIRES ethernet_init esp_netif esp_eth esp_wifi nvs_flash esp_driver_gpio esp_driver_uart esp_lcd esp_timer console fatfs cmd_nvs cmd_rtk)
