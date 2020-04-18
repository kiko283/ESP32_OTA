#include <Arduino.h>

// ##### VARIABLES NEEDED FOR OTA #####

const int NETWORKS_COUNT = 1;

// Network(s) setup
// You can add more than one if you plan
// to use device in multiple networks
const char* SSIDs[NETWORKS_COUNT] = {
  "SSID1"
  // "SSID2"
  // "SSID3"
};
const char* passwords[NETWORKS_COUNT] = {
  "password1",
  // "password2"
  // "password3"
};

// OTA upgrade page basic authorization credentials
const char* authUser = "admin";
const char* authPass = "Password123";

// /fw_version endpoint returns this value, you can
// use this to keep track of which version is in device
const char* FW_VERSION = "1.0.0";

// ##### OTHER NECESSARY VARIABLES #####

const int ONBOARD_LED_PIN = 2;
const int ONBOARD_LED_ON  = HIGH;
const int ONBOARD_LED_OFF = LOW;
uint8_t state = LOW;
bool stateOverride = false;
// ##### ##### ##### ##### #####

// Custom setup function, executed from main setup()
void setup1 () {

  // Serial.begin(115200) already executed in main setup()

  pinMode(ONBOARD_LED_PIN, OUTPUT);
}

// Custom loop function, continuously executed from main loop()
void loop1() {
  state = !state;
  if(!stateOverride) digitalWrite(ONBOARD_LED_PIN, state);
  delay(1000);
}

// Callback function when device connects to WiFi
void connectedToWiFiCallback() {
  digitalWrite(ONBOARD_LED_PIN, ONBOARD_LED_ON);
  delay(1000);
  digitalWrite(ONBOARD_LED_PIN, ONBOARD_LED_OFF);
}

// Function used to process commands sent through wifi
// * Device will print it's IP address on serial line
// * To send command to device:
//    - Make a POST request to http://<device_IP>/process_command
//    - POST body:  application/x-www-form-urlencoded
//    - POST field: command=<command_to_send>
void processCommand(String command) {
  if (command == "LED_ON") {
    stateOverride = true;
    digitalWrite(ONBOARD_LED_PIN, ONBOARD_LED_ON);
  } else if (command == "LED_OFF") {
    stateOverride = true;
    digitalWrite(ONBOARD_LED_PIN, ONBOARD_LED_OFF);
  } else if (command == "LED_RELEASE") {
    stateOverride = false;
  }
}
