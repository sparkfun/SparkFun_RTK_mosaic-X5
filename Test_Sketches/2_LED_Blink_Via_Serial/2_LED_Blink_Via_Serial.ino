/*
  SparkFun RTK mosaic-X5 Test Sketch

  Link TX_O to RX_I and RTS_O to CTS_I to test the serial pins and the LEDs

  License: MIT. Please see LICENSE.md for more details

  ESP32-WROVER-IE Pin Allocations:
  D0  : Boot + Boot Button + Ethernet REFCLK
  D1  : Serial TX (CH340 RX)
  D2  : Serial TX (mosaic-X5 COM4 RX)
  D3  : Serial RX (CH340 TX)
  D4  : Serial RX (mosaic-X5 COM4 TX)
  D5  : Ethernet ERST
  D12 : LED WiFi
  D13 : LED BT
  D14 : I2C SCL
  D15 : I2C SDA
  D16 : N/C
  D17 : N/C
  D18 : Ethernet MDIO
  D19 : Ethernet TXD0
  D21 : Ethernet TXEN
  D22 : Ethernet TXD1
  D23 : Ethernet MDC
  D25 : Ethernet RXD0
  D26 : Ethernet RXD1
  D27 : Ethernet CRSDV
  D32 : Serial TX (IO header)
  D33 : Serial RTS (IO header)
  A34 : Serial RX (IO header)
  A35 : Serial CTS (IO header)
  A36 : N/C
  A39 : N/C
*/

const int LED_WIFI = 12;
const int LED_BT = 13;
const int SCL_2 = 14;
const int SDA_2 = 15;

const int TX_O = 32;
const int RX_I = 34;
const int RTS_O = 33;
const int CTS_I = 35;

void setup()
{
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_BT, OUTPUT);

  pinMode(TX_O, OUTPUT);
  pinMode(RX_I, INPUT);
  pinMode(RTS_O, OUTPUT);
  pinMode(CTS_I, INPUT);
}

void loop()
{
  digitalWrite(TX_O, HIGH);
  digitalWrite(RTS_O, LOW);
  digitalWrite(LED_WIFI, digitalRead(RX_I));
  digitalWrite(LED_BT, digitalRead(CTS_I));
  delay(500); // At 1Hz
  digitalWrite(TX_O, LOW);
  digitalWrite(RTS_O, HIGH);
  digitalWrite(LED_WIFI, digitalRead(RX_I));
  digitalWrite(LED_BT, digitalRead(CTS_I));
  delay(500); // At 1Hz
}
