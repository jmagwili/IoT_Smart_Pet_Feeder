/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
 *************************************************************
  Blynk.Edgent implements:
  - Blynk.Inject - Dynamic WiFi credentials provisioning
  - Blynk.Air    - Over The Air firmware updates
  - Device state indication using a physical LED
  - Credentials reset using a physical Button
 *************************************************************/

/* Fill in information from your Blynk Template here */
/* Read more: https://bit.ly/BlynkInject */
#define BLYNK_TEMPLATE_ID "TMPL6Jh2MMDUS"
#define BLYNK_TEMPLATE_NAME "Pet Feeder"

#define BLYNK_FIRMWARE_VERSION        "0.1.0"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#define APP_DEBUG

// Uncomment your board, or configure a custom board in Settings.h
#define USE_ESP32_DEV_MODULE
//#define USE_ESP32C3_DEV_MODULE
//#define USE_ESP32S2_DEV_KIT
//#define USE_WROVER_BOARD
//#define USE_TTGO_T7
//#define USE_TTGO_T_OI

#define BUTTON_PIN 34

#include "BlynkEdgent.h"
#include <ESP32Servo.h>
#include <time.h>

// Time zone and NTP server
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600; // GMT+8 for Philippine Time
const int daylightOffset_sec = 0;   // No DST in PH

// Servo setup
Servo servo;
const int servoPin = 21;    // GPIO pin for servo
const int openAngle = 140;   // Angle to open feeder
const int closeAngle = 100;   // Angle to close feeder

// Feeding schedule
String scheduledTime = "";

// Blynk Dropdown to set schedule
BLYNK_WRITE(V0) {
  String timeInput = param.asStr(); // Accepts time input as HH:MM
  if (timeInput.length() == 5 && timeInput.indexOf(':') == 2) {
    scheduledTime = timeInput;
    Serial.println("Updated schedule: " + scheduledTime);
  } else {
    Serial.println("Invalid time format");
  }
}


// Manual feed button
BLYNK_WRITE(V1) {
  int value = param.asInt();
  if (value) {
    Serial.println("Manual feed triggered");
    dispenseFood();
  }
}

void dispenseFood() {
  Serial.println("Dispensing food...");
  servo.write(openAngle);  // Open the feeder
  delay(700);             // Keep it open for 3 seconds
  servo.write(closeAngle); // Close the feeder
  delay(300);
  servo.write(openAngle);  // Open the feeder
  delay(700);             // Keep it open for 3 seconds
  servo.write(closeAngle);
  Serial.println("Feeding complete");
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  BlynkEdgent.begin();

  // NTP setup
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Servo setup
  servo.attach(servoPin);
  servo.write(closeAngle); // Start with feeder closed

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Setup complete");

}

void loop() {
  BlynkEdgent.run();

  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == HIGH) {
    dispenseFood();
    delay(300);  // Debounce delay
  }
  // Serial.println(buttonState);
  // delay(1000);

}

