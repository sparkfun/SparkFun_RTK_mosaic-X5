---
icon: material/tools
---

## USB-C Ports
The USB ports are utilized to configure the mosaic-X5 module and ESP32 WiFi settings. Additionally, the USB ports can also be used as a power source for the RTK mosaic-X5.

<!-- Reworded
The USB port for the mosaic-X5 is used for serial communication to stream the GNSS data and to access the SD card as a mass storage device. With the default firmware, the USB port for the ESP32 is used for serial communication to configure the network settings of the Ethernet-to-WiFi bridge.
-->

<div class="grid" markdown>

<div markdown>

The USB port to the mosaic-X5 can be used to configure the module through an IP port, serial communication to stream the GNSS data, and access the SD card as a mass storage device. To connect to the mosaic-X5, users only need to plug a USB-C cable into the `CONFIG MOSAIC` USB port and their computer.

<figure markdown>
[![mosaic-X5 USB connection](./assets/img/hookup_guide/assembly-usb-mosaic.jpg){ width="400" }](./assets/img/hookup_guide/assembly-usb-mosaic.jpg "Click to enlarge")
<figcaption markdown>The RTK mosaic-X5 with USB-C cable being attached.</figcaption>
</figure>

</div>

<div markdown>

With the default firmware, the USB port for the ESP32 is used for serial communication to configure the network settings of the Ethernet-to-WiFi bridge. To configure the network settings for the ESP32, users only need to plug a USB-C cable into the `CONFIG ESP32` USB port and their computer.

<figure markdown>
[![ESP32 USB connection](./assets/img/hookup_guide/assembly-usb-esp32.jpg){ width="400" }](./assets/img/hookup_guide/assembly-usb-esp32.jpg "Click to enlarge")
<figcaption markdown>The RTK mosaic-X5 with USB-C cable being attached.</figcaption>
</figure>

</div>

</div>

!!! tip "Software Requirements"
	Depending on their computer's operating system, users may need to install USB drivers to interface with the mosaic-X5 and/or the ESP32. Users may also need to install a terminal emulator for serial communication with the mosaic-X5 and the ESP32.


## Antennas

<div class="grid" markdown>

<div markdown>

In order to receive [GNSS](https://en.wikipedia.org/wiki/Satellite_navigation "Global Navigation Satellite System") signals, users will need a compatible antenna. With the parts included in this kit, connect the L1/L2/L5 (tri-band) GNSS antenna to the RTK mosaic-X5 using the TNC-to-SMA cable.

<figure markdown>
[![GNSS antenna connected to the cable](./assets/img/hookup_guide/assembly-gnss-tnc.jpg){ width="400" }](./assets/img/hookup_guide/assembly-gnss-tnc.jpg "Click to enlarge")
<figcaption markdown>Attaching a tri-band GPS antenna to the TNC-SMA cable.</figcaption>
</figure>

<figure markdown>
[![GNSS antenna connected to the RTK mosaic-X5](./assets/img/hookup_guide/assembly-gnss_antenna.jpg){ width="400" }](./assets/img/hookup_guide/assembly-gnss_antenna.jpg "Click to enlarge")
<figcaption markdown>Attaching a TNC-SMA cable to the SMA connector on the RTK mosaic-X5.</figcaption>
</figure>

</div>

<div markdown>

For the WiFi connection, users will need a compatible antenna. Connect the WiFi antenna, included in this kit, to the RTK mosaic-X5.

<figure markdown>
[![WiFi antenna connected to the RTK mosaic-X5](./assets/img/hookup_guide/assembly-wifi_antenna.jpg){ width="400" }](./assets/img/hookup_guide/assembly-wifi_antenna.jpg "Click to enlarge")
<figcaption markdown>Attaching a 2.4GHz WiFi antenna to the RP-SMA connector on the RTK mosaic-X5.</figcaption>
</figure>

!!! warning "WiFi Network Compatibility"
	The ESP32 is only compatible with 2.4GHz bands and cannot access the 5GHz band.
	The ESP32 firmware we provide is only compatible with basic `SSID` and `Password` WiFi authentication.

	* The firmware is not compatible with networks that implement other provisioning methods such as a [captive portal](https://en.wikipedia.org/wiki/Captive_portal), a QR code, or [Wi-Fi protected setup](https://en.wikipedia.org/wiki/Wi-Fi_Protected_Setup "WPS").

</div>

</div>

??? tip "Mounting Location"
	Users should mount their GNSS antenna outside, where it will have a clear, unobstructed view of the sky. Avoid areas with nearby buildings, EMF structures (i.e. radio towers or power lines), and vegetation (i.e. trees). These objects can increase errors due to signal muti-path, interference, and elevated noise plane.

	<figure markdown>
	[![GNSS antenna connected to the RTK mosaic-X5](./assets/img/hookup_guide/assembly-gnss-mount_location.jpg){ width="400" }](./assets/img/hookup_guide/assembly-gnss-mount_location.jpg "Click to enlarge")
	<figcaption markdown>The tri-band GPS antenna, mounted outside with an unobstructed view of the sky.</figcaption>
	</figure>

??? tip "Connector Polarity"
	When selecting antennas and/or cables for the RTK mosaic-X5, double-check the polarity of the connection.


## Ethernet Jacks
There are two ethernet jacks on the RTK mosaic-X5, which can be used to provide network access to the mosaic-X5 module. In addition, one of the ethernet jacks supports [power over ethernet (PoE)](https://en.wikipedia.org/wiki/Power_over_Ethernet "PoE") to power the device.

=== "Basic Network"
	The jack to the mosaic-X5 allows users to provide internet access and power; it supports [PoE](https://en.wikipedia.org/wiki/Power_over_Ethernet "Power over Ethernet"). To provide network access, users should connect the RTK mosaic-X5 from the `MOSAIC ETHERNET (PoE)` jack to their local network with the (CAT-6) ethernet cable provided in the kit.

	* To power the device, a PoE network switch or PoE injector should be installed in between the network connection to the RTK mosaic-X5.
	<!-- * For a WiFi connection, connect the provided (CAT-6) ethernet cable between the `MOSAIC ETHERNET (PoE)` and `ESP32 ETHERNET` jacks on the RTK mosaic-X5. -->

	<figure markdown>
	[![mosaic-X5 PoE connection](./assets/img/hookup_guide/assembly-ethernet-mosaic.jpg){ width="400" }](./assets/img/hookup_guide/assembly-ethernet-mosaic.jpg "Click to enlarge")
	<figcaption markdown>The RTK mosaic-X5 with ethernet cable being attached to the `MOSAIC ETHERNET (PoE)` jack.</figcaption>
	</figure>


=== "WiFi Bridge"
	The jack to the ESP32 allows users to provide WiFi access to the mosaic-X5, by utilizing the ESP32 as a WiFi network bridge.

	<figure markdown>
	[![ESP32 ethernet connection](./assets/img/hookup_guide/assembly-ethernet-esp32.jpg){ width="400" }](./assets/img/hookup_guide/assembly-ethernet-esp32.jpg 	"Click to enlarge")
	<figcaption markdown>The RTK mosaic-X5 with an ethernet cable being attached to the `ESP32 ETHERNET` jack.</figcaption>
	</figure>

	To setup the WiFi bridge, connect the provided (CAT-6) ethernet cable between the `MOSAIC ETHERNET (PoE)` and `ESP32 ETHERNET` jacks on the RTK mosaic-X5.

	<figure markdown>
	[![ESP32 ethernet connection](./assets/img/hookup_guide/assembly-wifi_bridge.jpg){ width="400" }](./assets/img/hookup_guide/assembly-wifi_bridge.jpg "Click to enlarge")
	<figcaption markdown>The RTK mosaic-X5 with an ethernet cable connected between the `MOSAIC ETHERNET (PoE)` and `ESP32 ETHERNET` jacks.</figcaption>
	</figure>


	??? tip "Configure WiFi Connection"
		Users will need to configure the WiFi connection for the ESP32, through the `CONFIG ESP32` USB-C port.

	??? warning "Cable Management"
		Users should avoid wrapping the ethernet cable around the WiFi antenna; doing so will result in the loss of data packets and cause the web page to freeze. If users must coil the wiring, we recommend that the coil be placed at least 2-3" away from the WiFi antenna.

		<figure markdown>
		[![Bad ESP32 ethernet connection](./assets/img/hookup_guide/assembly-bad_ethernet.jpg){ width="400" }](./assets/img/hookup_guide/assembly-bad_ethernet.jpg "Click to enlarge")
		<figcaption markdown>Do not wrap/coil the ethernet cable around the WiFi antenna.</figcaption>
		</figure>


??? tip "Configuration: mosaic-X5 Settings"
	Users can configure the mosaic-X5 module through the network connection (i.e. ethernet or WiFi).


## SD Card Slot
An &micro;SD card slot is available for users to log and store data, locally on the board. Users will need to insert a compatible SD card and configure the mosaic-X5 module for data logging.

<figure markdown>
[![RTK mosaic-X5 SD card slot](./assets/img/hookup_guide/assembly-sd_card.jpg){ width="400" }](./assets/img/hookup_guide/assembly-sd_card.jpg "Click to enlarge")
<figcaption markdown>Inserting an SD card into the RTK mosaic-X5.</figcaption>
</figure>

!!! info "SD Card Compatibility"
	The mosaic-X5 supports &micro;SD cards with a **FAT32** file system *(i.e. only cards **up to 32GB** in size)*.

??? warning "Initial Configuration"
	Before logging can take place, it is necessary to define a "logging stream" using the **Logging** page or **RxTools**. Streams can contain NMEA or SBF (Septentrio Binary Format) data; SBF can contain RTCM and/or RINEX.

	<figure markdown>
	[![Logging stream configuration](./assets/img/hookup_guide/Logging.png){ width="400" }](./assets/img/hookup_guide/Logging.png "Click to enlarge")
	<figcaption markdown>microSD logging stream configuration.</figcaption>
	</figure>

	!!! tip "Instructional Video"
		:material-youtube: [How to log data to the SD card of the Septentrio mosaic receiver module](https://youtu.be/Y9tvOebnoxk)

??? tip "Button Operation"
	There are multiple ways to configure and enable data logging to an SD card. However, the simplest method is with the ++"LOG"++ button. Once the stream is defined,

	* Pressing the ++"LOG"++ button *(< 5s)* toggles data logging to the SD card on and off.
	* Holding the ++"LOG"++ button for more than 5 seconds *(> 5s)* and then releasing it, will force the board to:
		* Unmount the SD card if it was mounted
		* Mount the SD card if it was unmounted

	For more information, please reference the [SD Card Slot](../hardware_overview/#sd-card-slot) section.## SD Card


## IO Terminals
Users can easily attach accessories to the RTK mosaic-X5 by wiring them into the terminal blocks on the back fo the enclosure.

<figure markdown>
[![Terminal Block](./assets/img/hookup_guide/assembly-terminal_block2.jpg){ width="400" }](./assets/img/hookup_guide/assembly-terminal_block2.jpg "Click to enlarge")
<figcaption markdown>Connecting a wire to the terminal block.</figcaption>
</figure>


??? tip "Multiple Connections"
	For multiple connections or wiring harnesses, users can disconnect the terminal blocks from their sockets on the RTK mosaic-X5.

	<figure markdown>
	[![Disassembled Terminal Block](https://cdn.sparkfun.com/c/600-400/assets/parts/2/2/5/2/5/22461-_PRT_10-Way_Terminal___Socket-_01.jpg){ width="400" }](https://cdn.sparkfun.com/assets/parts/2/2/5/2/5/22461-_PRT_10-Way_Terminal___Socket-_01.jpg "Click to enlarge")
	<figcaption markdown>Components of the terminal block.</figcaption>
	</figure>

	Users can wiggle or use a soft/rigid object to carefully pry the terminal block off from its connector. In the picture below, a plastic name tag (~1.5mm thick) is used to carefully pry the terminal block up. We have also found the edge of a [PCB ruler](https://www.sparkfun.com/products/15295) works great too.

	<figure markdown>
	[![Prying the Terminal Block](./assets/img/hookup_guide/disassembly-terminal_block.jpg){ width="400" }](./assets/img/hookup_guide/disassembly-terminal_block.jpg "Click to enlarge")
	<figcaption markdown>Using a soft/rigid object to carefully pry the terminal block free from its socket.</figcaption>
	</figure>

	Once wired up, users can simply push the terminal block back into its socket.

	<div class="grid" markdown>

	<div markdown>

	<figure markdown>
	[![Terminal Block](./assets/img/hookup_guide/assembly-terminal_block.jpg){ width="400" }](./assets/img/hookup_guide/assembly-terminal_block.jpg "Click to enlarge")
	<figcaption markdown>Connecting a wire to the terminal block.</figcaption>
	</figure>

	</div>

	<div markdown>

	<figure markdown>
	[![Terminal Block](./assets/img/hookup_guide/assembly-terminal_block-attach.jpg){ width="400" }](./assets/img/hookup_guide/assembly-terminal_block-attach.jpg "Click to enlarge")
	<figcaption markdown>Attaching the terminal block to its socket on the RTK mosaic-X5.</figcaption>
	</figure>

	</div>

	</div>

	!!! warning
		To avoid shorts or damaging the RTK mosaic-X5, verify the wiring with the labels on the back of the enclosure.
