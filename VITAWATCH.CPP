/*
========================================================
              JEEVAN VITAWATCH
========================================================

PROJECT:
JEEVAN VitaWatch - ESP32 Health Monitoring Prototype

CONTROLLER:
ESP32 Dev Module

SENSORS:
1. MAX30102  -> Heart Rate + SpO2
2. MAX30205  -> Temperature
3. SSD1306 OLED -> Display

========================================================
                    CONNECTIONS
========================================================

                    ESP32
                 +-----------+
                 |           |
       3.3V -----|-----------|---- VCC
       GND  -----|-----------|---- GND
                 |           |
       GPIO 21 --|-----------|---- SDA
       GPIO 22 --|-----------|---- SCL
                 +-----------+

--------------------------------------------------------
OLED SSD1306
--------------------------------------------------------

OLED VCC  -> ESP32 3.3V
OLED GND  -> ESP32 GND
OLED SDA  -> ESP32 GPIO 21
OLED SCL  -> ESP32 GPIO 22

OLED I2C ADDRESS:
0x3C

--------------------------------------------------------
MAX30102
--------------------------------------------------------

MAX30102 VCC/VIN -> ESP32 3.3V
MAX30102 GND     -> ESP32 GND
MAX30102 SDA     -> ESP32 GPIO 21
MAX30102 SCL     -> ESP32 GPIO 22

MAX30102 I2C ADDRESS:
0x57

--------------------------------------------------------
MAX30205
--------------------------------------------------------

MAX30205 VCC -> ESP32 3.3V
MAX30205 GND -> ESP32 GND
MAX30205 SDA -> ESP32 GPIO 21
MAX30205 SCL -> ESP32 GPIO 22

MAX30205 I2C ADDRESS:
0x48

--------------------------------------------------------
IMPORTANT I2C NOTE
--------------------------------------------------------

All three devices share the same I2C bus.

SDA:
GPIO 21 -> OLED + MAX30102 + MAX30205

SCL:
GPIO 22 -> OLED + MAX30102 + MAX30205

This is NORMAL for I2C.

DO NOT give each sensor a separate SDA/SCL pin.

--------------------------------------------------------
ARDUINO IDE SETUP
--------------------------------------------------------

Board:
ESP32 Dev Module

Serial Monitor:
115200 baud

Required Libraries:

1. Adafruit SSD1306
2. Adafruit GFX Library
3. SparkFun MAX3010x Pulse and Proximity Sensor Library
4. Adafruit MAX30205 Library

========================================================
*/


#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "MAX30105.h"
#include "heartRate.h"

#include <Adafruit_MAX30205.h>


/*
========================================================
                    OLED SETTINGS
========================================================
*/

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1

#define OLED_ADDRESS 0x3C


Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


/*
========================================================
                    I2C PINS
========================================================
*/

#define SDA_PIN 21
#define SCL_PIN 22


/*
========================================================
                    SENSOR OBJECTS
========================================================
*/

MAX30105 particleSensor;

Adafruit_MAX30205 tempSensor;


/*
========================================================
                    HEART RATE
========================================================
*/

const byte RATE_SIZE = 4;

byte rates[RATE_SIZE];

byte rateSpot = 0;

long lastBeat = 0;

float beatsPerMinute;

int beatAvg = 0;


/*
========================================================
                       SPO2
========================================================
*/

float spo2 = 0;


/*
========================================================
                    TEMPERATURE
========================================================
*/

float temperature = 0;


/*
========================================================
                       SETUP
========================================================
*/

void setup()
{

  /*
  ------------------------------------------------------
  SERIAL COMMUNICATION
  ------------------------------------------------------
  */

  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("================================");
  Serial.println("       JEEVAN VITAWATCH");
  Serial.println("================================");


  /*
  ------------------------------------------------------
  START I2C
  ------------------------------------------------------
  */

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  Serial.println("I2C Started");


  /*
  ------------------------------------------------------
  START OLED
  ------------------------------------------------------
  */

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS))
  {

    Serial.println("OLED NOT FOUND!");

    while (1);
  }


  Serial.println("OLED FOUND");


  /*
  ------------------------------------------------------
  JEEVAN STARTUP SCREEN
  ------------------------------------------------------
  */

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  display.setTextSize(2);

  display.setCursor(
    10,
    10
  );

  display.println("JEEVAN");


  display.setTextSize(1);

  display.setCursor(
    30,
    35
  );

  display.println("VitaWatch");


  display.display();

  delay(2000);


  /*
  ------------------------------------------------------
  START MAX30102
  ------------------------------------------------------
  */

  if (!particleSensor.begin(
        Wire,
        I2C_SPEED_FAST))
  {

    Serial.println(
      "MAX30102 NOT FOUND!"
    );


    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(
      5,
      25
    );

    display.println(
      "MAX30102 NOT FOUND"
    );

    display.display();

  }
  else
  {

    Serial.println(
      "MAX30102 FOUND"
    );


    /*
    MAX30102 BASIC CONFIGURATION
    */

    particleSensor.setup();

    particleSensor.setPulseAmplitudeRed(
      0x0A
    );

    particleSensor.setPulseAmplitudeGreen(
      0
    );
  }


  /*
  ------------------------------------------------------
  START MAX30205
  ------------------------------------------------------
  */

  if (!tempSensor.begin(0x48))
  {

    Serial.println(
      "MAX30205 NOT FOUND!"
    );

  }
  else
  {

    Serial.println(
      "MAX30205 FOUND"
    );
  }


  /*
  ------------------------------------------------------
  SYSTEM READY
  ------------------------------------------------------
  */

  Serial.println();

  Serial.println(
    "JEEVAN VITAWATCH READY"
  );

  Serial.println();

}


/*
========================================================
                       MAIN LOOP
========================================================
*/

void loop()
{

  /*
  ======================================================
                  READ MAX30102
  ======================================================
  */

  long irValue =
    particleSensor.getIR();

  long redValue =
    particleSensor.getRed();


  /*
  ======================================================
                    HEART RATE
  ======================================================
  */

  if (checkForBeat(irValue))
  {

    long delta =
      millis() - lastBeat;


    lastBeat =
      millis();


    beatsPerMinute =
      60.0 / (delta / 1000.0);


    if (
      beatsPerMinute > 20 &&
      beatsPerMinute < 255
    )
    {

      rates[rateSpot++] =
        (byte)beatsPerMinute;


      rateSpot %= RATE_SIZE;


      beatAvg = 0;


      for (
        byte x = 0;
        x < RATE_SIZE;
        x++
      )
      {

        beatAvg +=
          rates[x];
      }


      beatAvg /=
        RATE_SIZE;
    }
  }


  /*
  ======================================================
                       SPO2
  ======================================================

  NOTE:
  This is only a prototype estimation.
  It is NOT medically validated.
  */

  if (irValue > 50000)
  {

    float ratio =
      (float)redValue /
      (float)irValue;


    spo2 =
      110.0 -
      (25.0 * ratio);


    if (spo2 > 100)
      spo2 = 100;


    if (spo2 < 70)
      spo2 = 70;

  }
  else
  {

    spo2 = 0;
  }


  /*
  ======================================================
                    TEMPERATURE
  ======================================================
  */

  temperature =
    tempSensor.readTemperature();


  /*
  ------------------------------------------------------
  CHECK TEMPERATURE READING
  ------------------------------------------------------
  */

  if (
    temperature < -40 ||
    temperature > 100
  )
  {

    temperature = 0;
  }


  /*
  ======================================================
                    SERIAL MONITOR
  ======================================================
  */

  Serial.print("IR: ");

  Serial.print(irValue);


  Serial.print(" | BPM: ");

  Serial.print(beatAvg);


  Serial.print(" | SpO2: ");

  Serial.print(spo2, 1);

  Serial.print("%");


  Serial.print(" | Temperature: ");

  Serial.print(
    temperature,
    2
  );

  Serial.println(" C");


  /*
  ======================================================
                       OLED DISPLAY
  ======================================================
  */

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  /*
  ------------------------------------------------------
  HEADER
  ------------------------------------------------------
  */

  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );

  display.println(
    "JEEVAN VITAWATCH"
  );


  display.drawLine(
    0,
    10,
    127,
    10,
    SSD1306_WHITE
  );


  /*
  ------------------------------------------------------
                    HEART RATE
  ------------------------------------------------------
  */

  display.setTextSize(1);

  display.setCursor(
    0,
    16
  );

  display.print(
    "HEART RATE"
  );


  display.setTextSize(2);

  display.setCursor(
    75,
    14
  );


  if (beatAvg > 0)
  {

    display.print(
      beatAvg
    );

  }
  else
  {

    display.print(
      "--"
    );
  }


  display.setTextSize(1);

  display.setCursor(
    108,
    20
  );

  display.print(
    "BPM"
  );


  /*
  ------------------------------------------------------
                         SPO2
  ------------------------------------------------------
  */

  display.setCursor(
    0,
    35
  );

  display.print(
    "SpO2"
  );


  display.setTextSize(2);

  display.setCursor(
    45,
    31
  );


  if (spo2 > 0)
  {

    display.print(
      spo2,
      0
    );

    display.print(
      "%"
    );

  }
  else
  {

    display.print(
      "--"
    );
  }


  /*
  ------------------------------------------------------
                      TEMPERATURE
  ------------------------------------------------------
  */

  display.setTextSize(1);

  display.setCursor(
    0,
    52
  );

  display.print(
    "TEMP:"
  );


  display.setCursor(
    38,
    52
  );


  if (temperature > 0)
  {

    display.print(
      temperature,
      1
    );

    display.print(
      " C"
    );

  }
  else
  {

    display.print(
      "--"
    );
  }


  /*
  ------------------------------------------------------
  UPDATE OLED
  ------------------------------------------------------
  */

  display.display();


  /*
  ======================================================
                     LOOP DELAY
  ======================================================
  */

  delay(50);

}


/*
========================================================
                    JEEVAN NOTES
========================================================

I2C DEVICES EXPECTED:

OLED:
0x3C

MAX30102:
0x57

MAX30205:
0x48


EXPECTED I2C SCANNER:

Device found: 0x3C
Device found: 0x48
Device found: 0x57


If 0x48 does NOT appear:
Check MAX30205 VCC, GND, SDA and SCL.

If 0x57 does NOT appear:
Check MAX30102 VCC, GND, SDA and SCL.

If 0x3C does NOT appear:
Check OLED VCC, GND, SDA and SCL.


========================================================
                    END OF CODE
========================================================
*/