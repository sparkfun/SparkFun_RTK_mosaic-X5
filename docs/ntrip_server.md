---
icon: material/cog
---

Now we know the Base's antenna position accurately, we can generate and share RTK corrections in a number of ways. The mosaic-X5 is a very clever module and is able to share corrections through an NTRIP Caster, or direct using Serial, USB, or an IP connection. For example, here are the output options available for RTCMv3:

<figure markdown>
[![Board Dimensions](./assets/img/RTCM_output.png){ width="400" }](./assets/img/RTCM_output.png "Click to enlarge")
<figcaption markdown>
[Selecting the RTCMv3 Output Type (PNG)](./assets/img/RTCM_output.png).
</figcaption>
</figure>

In the Advanced Settings, you can also define which messages to output and at what internal:

<figure markdown>
[![Board Dimensions](./assets/img/RTCM_output_2.png){ width="400" }](./assets/img/RTCM_output_2.png "Click to enlarge")
<figcaption markdown>
[Advanced settings for RTCMv3 Output (PNG)](./assets/img/RTCM_output_2.png).
</figcaption>
</figure>

In this section, we are going to concentrate on sharing the corrections using an NTRIP Caster. Think of an NTRIP Caster as an intermediary, a way of sharing RTCM corrections between Bases and Rovers. There are several good free-to-use NTRIP Caster services available, including:
* [RTK2go](http://rtk2go.com/)
* [Emlid Caster](https://emlid.com/ntrip-caster/)

Here we are going to use RTK2go.

## Create a New Base Station Reservation

Creating a New Reservation for your base station is really easy. Simply follow [the instructions](http://rtk2go.com/sample-page/new-reservation/)...

At SparkFun, we use **MountPt Name**s like **bldr_dwntwn2**, **bldr_dwntwn3**, **bldr_SparkFun1** - bldr is short for Boulder. Choose a name which is short, easy to remember and - we suggest - includes clues about who you are or where your server is. You can see the full list of all active bases at [rtk2go.com:2101](http://www.rtk2go.com:2101/).

The mosaic-X5 is a high end receiver so be sure to select **NTRIP Rev2** as the protocol and provide a login user name. IBAN - the banking network - provide a handy list of [three-letter country codes](https://www.iban.com/country-codes).

The registration process typically takes under 8 hours to complete, somewhat longer on weekends. Once your mount point has been set up, RTK2go wil send you a nice email confirming the mount point (**mountPt**) name, password and other details (Country, Country Code etc..). Make a note of those details, as we need to enter them into the mosaic-X5 NTRIP Server configuration.

<figure markdown>
[![Board Dimensions](./assets/img/NTRIP_server_0.png){ width="400" }](./assets/img/NTRIP_server_0.png "Click to enlarge")
<figcaption markdown>
[RTK 2 Go Confirmation Email (PNG)](./assets/img/NTRIP_server_0.png).
</figcaption>
</figure>

## Configure the NTRIP Server

In the **Corrections** tab, select **New NTRIP server**:

<figure markdown>
[![Board Dimensions](./assets/img/NTRIP_server_1.png){ width="400" }](./assets/img/NTRIP_server_1.png "Click to enlarge")
<figcaption markdown>
[Configuring the NTRIP server (PNG)](./assets/img/NTRIP_server_1.png).
</figcaption>
</figure>

Enter the details from the confirmation email:

<figure markdown>
[![Board Dimensions](./assets/img/NTRIP_server_2.png){ width="400" }](./assets/img/NTRIP_server_2.png "Click to enlarge")
<figcaption markdown>
[Configuring the NTRIP server (PNG)](./assets/img/NTRIP_server_2.png).
</figcaption>
</figure>

Remember to press **OK** after each change - and save the configuration.

Check that the **Position Mode** is set to **Static**, using either an *automatic* **Reference Position** or the precise position from your RINEX post-process data.

<figure markdown>
[![Board Dimensions](./assets/img/NTRIP_server_3.png){ width="400" }](./assets/img/NTRIP_server_3.png "Click to enlarge")
<figcaption markdown>
[Configuring the Reference Position (PNG)](./assets/img/NTRIP_server_3.png).
</figcaption>
</figure>

All being well, you should see a green line for NTRIP showing that your corrections are being forwarded to the caster:

<figure markdown>
[![Board Dimensions](./assets/img/NTRIP_server_4.png){ width="400" }](./assets/img/NTRIP_server_4.png "Click to enlarge")
<figcaption markdown>
[NTRIP connection (PNG)](./assets/img/NTRIP_server_4.png).
</figcaption>
</figure>

We've mentioned before that the mosaic-X5 is a very clever GNSS module. We can configure it to send corrections to *multiple* NTRIP Casters if desired, simply by adding extra NTRIP Caster Server connections.
