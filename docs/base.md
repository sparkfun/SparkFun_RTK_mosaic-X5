---
icon: material/cog
---

In Real Time Kinematic (RTK) positioning, a Rover uses corrections from a local Base station to accurately augment its position to centimeter or millimeter level. This is done by using carrier-phase measurements of the received GNSS signals and differencing techniques. The Base station antenna location is needed to calculate an accurate and reliable position.

In this section, we discuss two different ways of establishing the Base station antenna position: temporary and fixed.

## Temporary Base

The mosaic-X5 can be set to "Static" mode and can determine the antenna location (reference position) *automatically*. This is equivalent to "Survey-In" on u-blox GNSS modules. The module will refine its estimate of the antenna position and use that to generate correction data for Rovers.

<figure markdown>
[![Board Dimensions](./assets/img/auto_reference_position.jpg){ width="400" }](./assets/img/auto_reference_position.jpg "Click to enlarge")
<figcaption markdown>
[Selecting the auto reference position (JPG)](./assets/img/auto_reference_position.jpg).
</figcaption>
</figure>

!!! info
    In this mode the module refines its estimate of the antenna position; a new antenna position will be calculated each time the mosaic-X5 is restarted. For this reason, the auto function should only be used to generate a temporary base antenna location.

## Fixed Base

The best way to determine the base antenna location is to: log raw GNSS signal data for typically 24 hours, convert it to RINEX format and then submit it to a Precise Point Positioning (PPP) post-processing service such as:
* [NRCAN](https://webapp.csrs-scrs.nrcan-rncan.gc.ca/geod/tools-outils/ppp.php)
* [OPUS](https://www.ngs.noaa.gov/OPUS/)
* [APPS](https://pppx.gdgps.net/)

There are some great articles written about PPP. Here we are just covering the essentials. For more information check out:
* Gary Miller’s great [PPP HOWTO](https://gpsd.gitlab.io/gpsd/ppp-howto.html)
* Suelynn Choy, [GNSS PPP](https://www.unoosa.org/documents/pdf/icg/2018/ait-gnss/16_PPP.pdf)

Once the precise antenna position is known, it can be programmed into the module. The corrections the module generates will then be based on that precise, fixed antenna position.

First, let's check what Datum the module is using. It defaults to WGS84/ITRS. In North America, it might be better to select NAD83 but we'll go with the default.

<figure markdown>
[![Board Dimensions](./assets/img/RINEX0.png){ width="400" }](./assets/img/RINEX0.png "Click to enlarge")
<figcaption markdown>
[Check the datum (PNG)](./assets/img/RINEX0.png).
</figcaption>
</figure>

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

Use the **Enable Logging** radio button to start logging data, or press the **LOG** button. The red LOG LED will blink while data is being logged.

The **IGS24H** Naming Type is useful. When selected, the mosaic-X5 will log data in intervals of 24 hours, opening a new file at UTC midnight.

Use the **Disk Contents** tab to download the SBF data to your computer. Click the green arrow to download an individual file. Or - if the file is large - dismount the disk, eject it and use your computer to copy the files manually.

<figure markdown>
[![Board Dimensions](./assets/img/RINEX4.png){ width="400" }](./assets/img/RINEX4.png "Click to enlarge")
<figcaption markdown>
[Downloading the logging data (PNG)](./assets/img/RINEX4.png).
</figcaption>
</figure>

Use the **RxTools** \ **SBF Converter** utility to convert the data to RINEX format.

<figure markdown>
[![Board Dimensions](./assets/img/RINEX5.png){ width="400" }](./assets/img/RINEX5.png "Click to enlarge")
<figcaption markdown>
[Convert to RINEX (PNG)](./assets/img/RINEX5.png).
</figcaption>
</figure>

Upload the RINEX data to your chosen PPP post-process service. We have found [NRCAN](https://webapp.csrs-scrs.nrcan-rncan.gc.ca/geod/tools-outils/ppp.php) is very easy to use and produces excellent results. Both OPUS and APPS have file size limits, you may need to log data much slower than 1Hz if you want to use OPUS or APPS.

We select the **ITRF** tab because we are using the WGS84/ITRS datum. (If you are using NAD83, select that tab instead.)

<figure markdown>
[![Board Dimensions](./assets/img/CSRS-PPP.png){ width="400" }](./assets/img/CSRS-PPP.png "Click to enlarge")
<figcaption markdown>
[Select the datum for post-processing (PNG)](./assets/img/CSRS-PPP.png).
</figcaption>
</figure>

<figure markdown>
[![Board Dimensions](./assets/img/RINEX6.png){ width="400" }](./assets/img/RINEX6.png "Click to enlarge")
<figcaption markdown>
[Precise antenna position (PNG)](./assets/img/RINEX6.png).
</figcaption>
</figure>

We can now store those coordinates in the mosaic-X5 module memory, either as **Geodetic** (Latitude, Longitude, Alttude) or **Cartesian** (ECEF X/Y/Z) coordinates. The mosaic-X5 allows you to store 5 of each.

<figure markdown>
[![Board Dimensions](./assets/img/RINEX7.png){ width="400" }](./assets/img/RINEX7.png "Click to enlarge")
<figcaption markdown>
[Setting the fixed position (PNG)](./assets/img/RINEX7.png).
</figcaption>
</figure>

<figure markdown>
[![Board Dimensions](./assets/img/RINEX8.png){ width="400" }](./assets/img/RINEX8.png "Click to enlarge")
<figcaption markdown>
[Setting the fixed position (PNG)](./assets/img/RINEX8.png).
</figcaption>
</figure>

We can now generate corrections using that static / fixed antenna position.

<figure markdown>
[![Board Dimensions](./assets/img/RINEX9.png){ width="400" }](./assets/img/RINEX9.png "Click to enlarge")
<figcaption markdown>
[Setting the fixed position (PNG)](./assets/img/RINEX9.png).
</figcaption>
</figure>

