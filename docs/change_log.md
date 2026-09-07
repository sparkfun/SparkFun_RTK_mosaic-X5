Changes added at v1.0.6:

* ESP-IDF:
	* Bump to ESP-IDF v5.1.7
	* Add ```CONFIG_ETH_TRANSMIT_MUTEX=y```
* WiFi mode improvements:
	* Fixed an error where it was possible for the original ESP32 Ethernet MAC address to replace the desired spoofed mosaic-X5 MAC address
	* The firmware ignores the initial all-zeros MAC Address in IPStatus (or an Ethernet packet)
		* This was causing problems when starting in WiFi mode
	* The IP_EVENT_STA_GOT_IP event IP address is copied to the OLED display
		* This is received before the IPStatus update
		* This ensures the IP address is displayed even if the IPStatus is missed
	* The firmware no longer sends "seth,off" to turn Ethernet off before configuring DHCP
		* It looks like this caused more problems than it solved
	* Reduced the wait-for-IPStatus timeout from 5s to 3s
		* It is received quicker than that
* General improvements:
	* The minimal vTaskDelay has been increased from ```vTaskDelay(0)``` to ```vTaskDelay(1)```
		* This prevents unwanted Watchdog resets during wait-for-command-response

Changes added at v1.0.5:

* The firmware now supports a username and password for the X5 itself
	* This is to support mosaic-X5 firmware versions >= 4.15.1 where a username and password are mandatory *on IP interfaces* (webUI, Ethernet-over-USB, CLI over TCP/IP)
	* The username and password are *not* mandatory on COM and USB (virtual COM) ports, *unless* you set the `Default Access Level Per Interface` (`setDefaultAccessLevel` / `sdal`) to *none* for COM and/or USB ports
	* The ESP32 firmware only needs to know the username and password if the default access levels have been changed
	* For more information about the X5 Log-in procedure, please read [this Septentrio documentation](https://customersupport.septentrio.com/s/article/Cybersecurity-guidelines-Log-in-procedure)
	* If you upgrade the X5 firmware to 4.15.1:
		* Enter your user-defined username and password, using the Factory credentials *RxAdmin* and *S3pt3ntr10*, as described in the [Log-in procedure](https://customersupport.septentrio.com/s/article/Cybersecurity-guidelines-Log-in-procedure)
		* You will need to enter the user-defined username and password whenever you connect over an IP interface
		* Logging in via the ESP32 on a COM port does not remove the need to log in on IP interfaces. You still need to log in on each IP interface individually and separately
		* Upgrade the ESP32 Firmware to v1.0.5, following the procedure described below
		* If the ESP32 needs to know the username and password: in the ESP32 Serial Terminal / console, use option *set -u* to set the X5 username and option *set -x* to set the X5 password
		* Turn the RTK mosaic-X5 off and back on again

Changes added at v1.0.4:

* The firmware now includes an efficient SBF and NMEA parser
    * Previously the NMEA GPGGA and SBF IPStatus messages were polled, this resulted in the OLED being updated once every ~2 seconds
	* In v1.0.4: NMEA Stream10 carries the GPGGA message at 1Hz; SBF Stream10 carries the IPStatus message OnChange
	* The OLED is now updated on the arrival of the GPGGA message, at exactly 1Hz
* The firmware now ensures that the SBF and NMEA protocols are enabled for output on X5 COM4
    * Previously, the firmware would stall if either SBF or NMEA were disabled
* The firmware now supports the NMEA *GN* Talker ID, in addition to *GP*
    * Previously setting the Talker ID to *GN* would cause the firmware to stall
* The OLED now displays the Latitude and Longitude in *DD MM SS.SSSS* format, to match the format of the X5's internal web page
    * Previously the format was *DDMM.MMMMMM*, copied directly from the GPGGA message
* The firmware will perform a soft reset of the GNSS during startup - if needed
    * The X5 can go into a [Ready for SUF download](https://customersupport.septentrio.com/s/article/How-to-troubleshoot-receiver-reporting-Ready-for-SUF-download) state if it is reset four times with no antenna connected
	* This can be cleared with a soft reset

