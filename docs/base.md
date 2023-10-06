---
icon: material/cog
---

In Real Time Kinematic (RTK) positioning, a Rover uses corrections from a local Base station to accurately augment its position to centimeter or millimeter level. This is done by using carrier-phase measurements of the received GNSS signals and differencing techniques. The Base station antenna location is needed to calculate an accurate and reliable position.

## Using the auto function

The mosaic-X5 can be set to "Static" mode and can determine the antenna location (reference position) *automatically*. This is equivalent to "Survey-In" on u-blox GNSS modules. The module will refine its estimate of the antenna position and use that to generate correction data for Rovers.

<figure markdown>
[![Board Dimensions](./assets/img/auto_reference_position.jpg){ width="400" }](./assets/img/auto_reference_position.jpg "Click to enlarge")
<figcaption markdown>
[Selecting the auto reference position (JPG)](./assets/img/auto_reference_position.jpg).
</figcaption>
</figure>

!!! info
    A new antenna position will be calculated each time the mosaic-X5 is restarted. For this reason, the auto function should only be used to generate a temporary base antenna location.

## Using an online PPP post-processing service

The best way to determine the base antenna location is to log raw GNSS signal data for typically 24 hours, convert it to RINEX format and then submit it data to a post-processing service such as:
* [NRCAN](https://webapp.csrs-scrs.nrcan-rncan.gc.ca/geod/tools-outils/ppp.php)
* [OPUS](https://www.ngs.noaa.gov/OPUS/)
* [APPS](https://pppx.gdgps.net/)

Set up an SBF logging stream to log **PostProcess**, **Rinex**, **Rinex (meas3)** with an interval of 1 sec:

<figure markdown>
[![Board Dimensions](./assets/img/RINEX1.png){ width="400" }](./assets/img/RINEX1.png "Click to enlarge")
<figcaption markdown>
[Selecting the logging stream (PNG)](./assets/img/RINEX1.png).
</figcaption>
</figure>

<figure markdown>
[![Board Dimensions](./assets/img/RINEX2.png){ width="400" }](./assets/img/RINEX2.png "Click to enlarge")
<figcaption markdown>
[Selecting the logging stream (PNG)](./assets/img/RINEX2.png).
</figcaption>
</figure>

<figure markdown>
[![Board Dimensions](./assets/img/RINEX3.png){ width="400" }](./assets/img/RINEX3.png "Click to enlarge")
<figcaption markdown>
[Selecting the logging stream (PNG)](./assets/img/RINEX3.png).
</figcaption>
</figure>

The **IGS24H** Naming Type may be useful. Use the **Enable Logging** radio button to start logging data, or press the **LOG** button. The red LOG LED will blink while data is being logged. Log data for 24 hours...

Use the **Disk Contents** tab to download the SBF data to your computer.

Use the **RxTools** \ **SBF Converter** utility to convert the data to RINEX format.
