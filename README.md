SparkFun RTK mosaic-X5
========================================

[![SparkFun RTK mosaic-X5](https://cdn.sparkfun.com/r/455-455/assets/parts/2/4/0/7/2/23748-RTK-Mosaic-X5-Kit-All-Feature-NOAA.jpg)](https://www.sparkfun.com/products/23748)

[*SparkFun RTK mosaic-X5 (GPS-23748)*](https://www.sparkfun.com/products/23748)

Designed and manufactured in Boulder, Colorado, USA, the SparkFun RTK mosaic-X5 is the perfect solution for your high-precision positioning and navigation needs. Based around the multi-constellation, multi-frequency, L5-ready mosaic-X5 from Septentrio, this is our most advanced RTK product to date. It supports GNSS signals from GPS (USA), GLONASS (Russia), Beidou (China), Galileo (Europe), Navic (India) plus special additional satellites (e.g. SBAS and QZSS). The mosaic-X5 also has built-in on-module support for other L-band correction services.

The RTK mosaic-X5 can be configured as a Real Time Kinematic (RTK) Base, where it feeds corrections to other RTK Rovers, or as an RTK Rover where it can use corrections to achieve a horizontal positioning accuracy of 6 millimeters (0.6cm) (plus 0.5 PPM). For applications like robotics and autonomous systems, the mosaic-X5 can deliver position updates at 100Hz (100 times per second). The mosaic-X5 is a _very_ sophisticated chip running a full internal web page server; the position can be monitored and the module fully configured through that web page using a standard browser.

Under the hood, the RTK mosaic-X5 is based on the mosaic-X5 GNSS module from Septentrio, plus the Espressif ESP32-WROVER processor (16MB flash, 8MB PSRAM). The mosaic-X5 has USB-C connectivity (with Ethernet-over-USB), multiple UARTs and supports full Ethernet connectivity. The only interface it doesn't offer is WiFi and that's why we've included the ESP32 with its own Ethernet connection. You can connect the mosaic-X5 directly to your Ethernet network - our product supports Power-over-Ethernet too. Or you can link the mosaic-X5 Ethernet port to the ESP32 Ethernet port and have the ESP32 provide WiFi connectivity. In that mode, the ESP32 becomes an Ethernet to WiFi Bridge, seamlessly passing WiFi traffic to and from the mosaic-X5 via Ethernet.

Documentation
--------------

* **[Product Manual](http://docs.sparkfun.com/SparkFun_RTK_mosaic-X5/)** - Product manual for the RTK mosaic-X5 hosted by GitHub pages.<br>
  [![Built with Material for MkDocs](https://img.shields.io/badge/Material_for_MkDocs-526CFE?logo=MaterialForMkDocs&logoColor=white)](https://squidfunk.github.io/mkdocs-material/) [![GitHub Pages Deploy](https://github.com/sparkfun/SparkFun_RTK_mosaic-X5/actions/workflows/mkdocs.yml/badge.svg)](https://github.com/sparkfun/SparkFun_RTK_mosaic-X5/actions/workflows/mkdocs.yml)


*Need to download or print our hookup guide?*

* [Print *(Print to PDF)* from Single-Page View](http://docs.sparkfun.com/SparkFun_RTK_mosaic-X5/print_view)

Repository Contents
-------------------

* **[/docs](/docs/)** - Online documentation files
    * [assets](/docs/assets/) - Assets files
        * [board_files](/docs/assets/board_files/) - Files for the product design
            * [Schematic](/docs/assets/board_files/schematic.pdf) (.pdf)
            * [Dimensions](/docs/assets/board_files/dimensions.png) (.png)
            * [Eagle files](/docs/assets/board_files/eagle_files.zip) (.zip)
        * [img/hookup_guide/](/docs/assets/img/hookup_guide/) - Images for hookup guide documentation
* **[/Hardware](/Hardware/)** - Eagle design files (.brd, .sch)
* **[/Production](/Production/)** - PCB panel production files
* **[/Front_Panel](/Front_Panel/)** - Eagle design files (.brd, .sch) for the prototype (PCB) enclosure panel used to validate the dimensions
* **[/Rear_Panel](/Rear_Panel/)** - Eagle design files (.brd, .sch) for the prototype (PCB) enclosure panel used to validate the dimensions
* **[/Front_Sticker](/Front_Sticker/)** - DXF and PDF files for the front sticker
* **[/Rear_Sticker](/Rear_Sticker/)** - DXF and PDF files for the rear sticker
* **[/Firmware](/Firmware/)** - ESP IDF source code and binaries for the firmware which runs on the ESP32-WROVER
* **[/Test_Sketches](/Test_Sketches/)** - Additional code used to validate and test the RTK mosaic-X5
* **[/Documents](/Documents/)** - Component datasheets etc.

Product Variants
----------------

* [GPS-23748](https://www.sparkfun.com/products/23748)- v1.0, Initial Release

Version History
---------------

* [v10](https://github.com/sparkfun/SparkFun_RTK_mosaic-X5/releases/tag/v10) - Initial Release


License Information
-------------------

This product is ***open source***!

Please review the [LICENSE.md](./LICENSE.md) file for license information.

If you have any questions or concerns about licensing, please contact technical support on our [SparkFun forums](https://forum.sparkfun.com/viewforum.php?f=152).

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
