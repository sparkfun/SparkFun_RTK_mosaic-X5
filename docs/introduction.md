---
icon: material/book-open-page-variant
---

# Introduction
<div class="grid cards desc" markdown>

-   <a href="https://www.sparkfun.com/products/23748">
	**RTK mosaic-X5**<br>
	**SKU:** GPS-23748
	
	---
	
	<figure markdown>
	![Product Thumbnail]()
	</figure></a>


-	Designed and manufactured in Boulder, Colorado, USA, the SparkFun RTK mosaic-X5 is the perfect solution for your high-precision positioning and navigation needs. Based around the multi-constellation, multi-frequency, L5-ready mosaic-X5 from Septentrio, this is our most advanced RTK product to date. It supports GNSS signals from GPS (USA), GLONASS (Russia), Beidou (China), Galileo (Europe), Navic (India) plus special additional satellites (e.g. SBAS and QZSS). The mosaic-X5 also has built-in on-module support for other L-band correction services.

</div>

The RTK mosaic-X5 can be configured as a Real Time Kinematic (RTK) Base, where it feeds corrections to other RTK Rovers, or as an RTK Rover where it can use corrections to achieve a horizontal positioning accuracy of 6 millimeters (0.6cm) (plus 0.5 PPM). For applications like robotics and autonomous systems, the mosaic-X5 can deliver position updates at 100Hz (100 times per second). The mosaic-X5 is a _very_ sophisticated chip running a full internal web page server; the position can be monitored and the module fully configured through that web page using a standard browser.

Under the hood, the RTK mosaic-X5 is based on the mosaic-X5 GNSS module from Septentrio, plus the Espressif ESP32-WROVER processor (16MB flash, 8MB PSRAM). The mosaic-X5 has USB-C connectivity (with Ethernet-over-USB), multiple UARTs and supports full Ethernet connectivity. The only interface it doesn't offer is WiFi and that's why we've included the ESP32 with its own Ethernet connection. You can connect the mosaic-X5 directly to your Ethernet network - our product supports Power-over-Ethernet too. Or you can link the mosaic-X5 Ethernet port to the ESP32 Ethernet port and have the ESP32 provide WiFi connectivity. In that mode, the ESP32 becomes an Ethernet to WiFi Bridge, seamlessly passing WiFi traffic to and from the mosaic-X5 via Ethernet.

<center>
![QR code to product page](./assets/img/qr_code/product-low.png)
</center>

<center>
[Purchase from SparkFun :fontawesome-solid-cart-plus:{ .heart }](https://www.sparkfun.com/products/23748){ .md-button .md-button--primary }
</center>

## :fontawesome-solid-list-check:&nbsp;Required Materials
The RTK mosaic-X5 comes with everything you need to get up and running. Our kit includes:

* Cased GNSS Receiver
* L1/L2/L5 GNSS Surveying Antenna
* Reinforced RG58 TNC-SMA Cable (10m)
* SMA WiFi / Bluetooth Antenna
* 32GB microSD Card (Class 10)
* USB-C Power Supply (5V 1A wall adapter)
* USB-C Cable (A to C, 2m)
* Ethernet Cable (CAT-6, 1m)

Your RTK mosaic-X5 is equally at home on your desk, lab bench or in a server rack. But you're still going to want to put the GNSS antenna outdoors, so it will have the best view of the sky. Some extra SMA extension cables may be useful and we have good quality low-loss RG58 cables available in the store. The GNSS SMA antenna connection is standard polarity. If you want to extend the ESP32 WiFi / BT antenna connection too, you need a Reverse Polarity (RP) cable for that.

<div class="grid cards col-4" markdown>

-   <a href="https://www.sparkfun.com/products/21281">
	<figure markdown>
	![Interface Cable - SMA Female to SMA Male (10m, RG58)](https://cdn.sparkfun.com//assets/parts/2/1/0/6/5/21281-_CAB-_01.jpg)
	</figure>

	---

	**Interface Cable - SMA Female to SMA Male (10m, RG58)**<br>
	CAB-21281</a>

-   <a href="https://www.sparkfun.com/products/22038">
	<figure markdown>
	![Interface Cable - RP-SMA Male to RP-SMA Female (10M, RG58)](https://cdn.sparkfun.com//assets/parts/2/1/9/0/5/22038-_CAB-_01.jpg)
	</figure>

	---

	**Interface Cable - RP-SMA Male to RP-SMA Female (10M, RG58)**<br>
	CAB-22038</a>

</div>

If you are permanently mounting your antenna up on your roof, you may want some extra hardware to help:

<div class="grid cards col-4" markdown>

-   <a href="https://www.sparkfun.com/products/22197">
	<figure markdown>
	![GNSS Antenna Mounting Hardware Kit](https://cdn.sparkfun.com//assets/parts/2/2/0/9/7/22197-_01.jpg)
	</figure>

	---

	**GNSS Antenna Mounting Hardware Kit**<br>
	KIT-22197</a>

-   <a href="https://www.sparkfun.com/products/21257">
	<figure markdown>
	![GNSS Magnetic Antenna Mount - 5/8" 11-TPI](https://cdn.sparkfun.com//assets/parts/2/1/0/2/7/SparkFun-GNSS-Antenna-Magnetic-Mount-21257-1.jpg)
	</figure>

	---

	**GNSS Magnetic Antenna Mount - 5/8" 11-TPI**<br>
	PRT-21257</a>

</div>

## :material-weather-pouring:&nbsp;Selecting An Enclosure

The RTK mosaic-X5 comes in a beautiful custom extruded aluminium enclosure, with machined end panels and matching stickers. The slotted flanges make it easy to install and secure the enclosure in many locations. But the enclosure only provides limited protection against the ingress of dust and water; it is IP42. So, if you are going to permanently install it up on the roof too, you're going to need a suitable weatherproof box. We found a good one - the [Orbit 57095](https://www.orbitonline.com/products/gray-outdoor-timer-cabinet) - also available from [Amazon](https://www.amazon.com/Orbit-57095-Weather-Resistant-Outdoor-Mounted-Controller/dp/B000VYGMF2) - back when we put together our very first [DIY GNSS Reference Station](https://learn.sparkfun.com/tutorials/how-to-build-a-diy-gnss-reference-station#mini-computer-setup-option-1).

-   <a href="https://learn.sparkfun.com/tutorials/1363">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/1/3/6/3/Roof_Enclosure.jpg)
	</figure>

	---
	
	**How to Build a DIY GNSS Reference Station**</a>

!!! info "AC Not Required!"
    The Orbit enclosure comes with a built-in power outlet, but you don't actually need it! The RTK mosaic-X5 can be powered by Power-over-Ethernet (PoE), meaning all you really need to run up to the roof is a standard 8-core CAT-6 Ethernet cable. Choose a PoE Ethernet Switch that meets your needs. We have had good experiences with the [TP-Link TL-SG1005P](https://www.tp-link.com/us/business-networking/poe-switch/tl-sg1005p/) - available from many retailers including [Amazon](https://www.amazon.com/TP-Link-Compliant-Shielded-Optimization-TL-SG1005P/dp/B076HZFY3F).


## :material-bookshelf:&nbsp;Suggested Reading

As a more sophisticated product, we will skip over the more fundamental tutorials (i.e. [**Ohm's Law**](https://learn.sparkfun.com/tutorials/voltage-current-resistance-and-ohms-law) and [**What is Electricity?**](https://learn.sparkfun.com/tutorials/what-is-electricity)). However, below are a few tutorials that may help users familiarize themselves with various aspects of the board.


<div class="grid cards col-4" markdown align="center">

-   <a href="https://learn.sparkfun.com/tutorials/813">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/8/1/3/Location-Wandering-GPS-combined.jpg)
	</figure>

	---
	
	**What is GPS RTK?**</a>

-   <a href="https://learn.sparkfun.com/tutorials/1362">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/1/3/6/2/GNSS_RTK_DIY_Surveying_Tutorial.jpg)
	</figure>

	---
	
	**Setting up a Rover Base RTK System**</a>

-   <a href="https://learn.sparkfun.com/tutorials/1363">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/178-100/assets/learn_tutorials/1/3/6/3/Roof_Enclosure.jpg)
	</figure>

	---
	
	**How to Build a DIY GNSS Reference Station**</a>

-   <a href="https://learn.sparkfun.com/tutorials/908">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/264-148/assets/learn_tutorials/9/0/8/USB-to-serial_converter_CH340-closeup.jpg)
	</figure>

	---
	
	**How to Install CH340 Drivers**</a>

-   <a href="https://learn.sparkfun.com/tutorials/62">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/264-148/assets/learn_tutorials/6/2/Input_Output_Logic_Level_Tolerances_tutorial_tile.png)
	</figure>

	---
	
	**Logic Levels**</a>

-   <a href="https://learn.sparkfun.com/tutorials/82">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/264-148/assets/learn_tutorials/8/2/I2C-Block-Diagram.jpg)
	</figure>

	---
	
	**I2C**</a>

-   <a href="https://learn.sparkfun.com/tutorials/8">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/264-148/assets/7/d/f/9/9/50d24be7ce395f1f6c000000.jpg)
	</figure>

	---
	
	**Serial Communication**</a>

-   <a href="https://learn.sparkfun.com/tutorials/5">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/264-148/assets/e/3/9/9/4/51d9fbe1ce395f7a2a000000.jpg)
	</figure>

	---
	
	**How to Solder: Through-Hole Soldering**</a>

-   <a href="https://learn.sparkfun.com/tutorials/664">
	<figure markdown>
	![Tutorial Thumbnail](https://cdn.sparkfun.com/c/264-148/assets/learn_tutorials/6/6/4/PCB_TraceCutLumenati.jpg)
	</figure>

	---
	
	**How to Work with Jumper Pads and PCB Traces**</a>

</div>

