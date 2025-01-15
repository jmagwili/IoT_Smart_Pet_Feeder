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
#define LED_PIN 2 

#include "BlynkEdgent.h"
#include <ESP32Servo.h>
#include <time.h>
#include <Ultrasonic.h>
#include <set>
#include <vector>

// Time zone and NTP server
const char* ntpServer = "time.google.com";
const long gmtOffset_sec = 8 * 3600; // GMT+8 for Philippine Time
const int daylightOffset_sec = 0;   // No DST in PH

// Servo setup
Servo servo;
const int servoPin = 21;    // GPIO pin for servo
const int openAngle = 140;   // Angle to open feeder
const int closeAngle = 100;   // Angle to close feeder

// Feeding schedule
String morningSched[] = {"",""};
String afternoonSched[] = {"",""};
String eveningSched[] = {"",""};

bool hasDispensedMorning = false;
bool hasDispensedAfternoon = false;
bool hasDispensedEvening = false;

// Ultrasonic Sensor Setup
Ultrasonic ultrasonic(4, 16);
int distance;

// WiFi status LED control
bool isWiFiConnected = false;
unsigned long previousMillis = 0;
const long blinkInterval = 500; // Interval for LED blinking (ms)

void updateWiFiStatusLED() {
  if (Blynk.connected()) {
    if (!isWiFiConnected) {
      isWiFiConnected = true;
      digitalWrite(LED_PIN, HIGH); // Turn on LED
    }
  } else {
    isWiFiConnected = false;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
    }
  }
}

// Morning Schedule
BLYNK_WRITE(V2) {
  int time = param.asInt();
  String days = param[3].asStr(); // Accepts time input as HH:MM
  Serial.println(time);

  int seconds = time; // Example value in seconds
  int hours = seconds / 3600; // Calculate hours
  int minutes = (seconds % 3600) / 60; // Calculate remaining minutes

  // Format into HH:MM
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);

  morningSched[0] = timeStr;
  morningSched[1] = days;

  Serial.println("Updated schedule: " + morningSched[0] + morningSched[1]);
}

// Afternoon Schedule
BLYNK_WRITE(V3) {
  int time = param.asInt();
  String days = param[3].asStr(); // Accepts time input as HH:MM
  Serial.println(time);

  int seconds = time; // Example value in seconds
  int hours = seconds / 3600; // Calculate hours
  int minutes = (seconds % 3600) / 60; // Calculate remaining minutes

  // Format into HH:MM
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);

  afternoonSched[0] = timeStr;
  afternoonSched[1] = days;

  Serial.println("Updated schedule: " + afternoonSched[0] + afternoonSched[1]);
}

// Evening Schedule
BLYNK_WRITE(V4) {
  int time = param.asInt();
  String days = param[3].asStr(); // Accepts time input as HH:MM
  Serial.println(time);

  int seconds = time; // Example value in seconds
  int hours = seconds / 3600; // Calculate hours
  int minutes = (seconds % 3600) / 60; // Calculate remaining minutes

  // Format into HH:MM
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);

  eveningSched[0] = timeStr;
  eveningSched[1] = days;

  Serial.println("Updated schedule: " + eveningSched[0] + eveningSched[1]);
}

// Manual feed button
BLYNK_WRITE(V1) {
  int value = param.asInt();
  if (value) {
    Serial.println("Manual feed triggered");
    if(!isContainerEmpty()){
      dispenseFood();
    }else{
      Blynk.logEvent("food_failed_to_dispense");
      Serial.println("failed to dispense. Empty container. notif sent");
    }
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

bool isContainerEmpty(){
  distance = ultrasonic.read();

  if(distance == 9){
    return true;
  }else{
    return false;
  }
}

std::vector<int> toArray(String input) {
  std::vector<int> result;
  int startIndex = 0;
  int commaIndex = input.indexOf(',');

  while (commaIndex != -1) {
    String number = input.substring(startIndex, commaIndex); // Extract the number
    result.push_back(number.toInt());                       // Convert to int and store in vector
    startIndex = commaIndex + 1;                            // Move past the comma
    commaIndex = input.indexOf(',', startIndex);            // Find the next comma
  }

  // Handle the last (or only) number
  if (startIndex < input.length()) {
    String number = input.substring(startIndex);
    result.push_back(number.toInt());
  }

  return result;
}

int convertDay(int dayOfWeek) {
    // Ensure the input is between 1 and 7
    if (dayOfWeek < 1 || dayOfWeek > 7) {
        throw std::invalid_argument("Invalid day of the week. Must be between 1 and 7.");
    }
    // Convert the day format
    return (dayOfWeek % 7);
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
  pinMode(LED_PIN, OUTPUT); // Set LED pin as output

  Serial.println("Setup complete");

}

void loop() {
  BlynkEdgent.run();

  int buttonState = digitalRead(BUTTON_PIN);

  // Manual dispense button
  if (buttonState == HIGH) {
    if(!isContainerEmpty()){
      dispenseFood();
      delay(300);
    }else{
      Serial.println("Food failed to dispense. Empty container");
    }
  }

  // Send notification if container is empty
  if(isContainerEmpty()){
    Blynk.logEvent("food_container_empty");
    Serial.println("notification sent");
  }

    // Get the current time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Format current time as HH:MM
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  String currentTime = String(timeStr);

  // Serial.println("Current time: " + currentTime);

  // Get the day of the week
  const char* daysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  String currentDay = daysOfWeek[timeinfo.tm_wday];

  Serial.println("Current time: " + currentTime + " (" + currentDay + ")");
  

  for(int i=0; i<toArray(morningSched[1]).size();i++){
    if(!hasDispensedMorning){
      if(convertDay(toArray(morningSched[1])[i]) == timeinfo.tm_wday){
        Serial.println("Scheduled time: " + morningSched[0]);
        if(currentTime == morningSched[0]){
          dispenseFood();
          hasDispensedMorning = true;
        }
      }
    }
  }

  // Reset the flag at midnight
  if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0) {
    hasDispensedMorning = false;
    hasDispensedAfternoon = false;
    hasDispensedEvening = false;
  }

  delay(1000);
  
  updateWiFiStatusLED();

  // Serial.print("Distance in CM: ");
  // Serial.println(distance);
  // delay(1000);
}

