// Included libraries
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
// Included sketch files
#include "OTAPageHtml.h"
#include "MainCode.h"

WebServer server(80);

TaskHandle_t webServerTaskHandle;

void setup() {

  Serial.begin(115200);
  while(!Serial) yield();
  Serial.println();
  Serial.print("mainTask running on core ");
  Serial.println(xPortGetCoreID());

  // Handle web server requests on separate core
  // Main code runs on core 1, so web server runs on core 0
  xTaskCreatePinnedToCore(
    webServerTask,         /* Task function. */
    "webServer",           /* name of task. */
    16384,                 /* Stack size of task */
    NULL,                  /* parameter of the task */
    1,                     /* priority of the task */
    &webServerTaskHandle,  /* Task handle to keep track of task */
    0                      /* pin task to core 0 */ 
  );
  delay(500);

  // Execute setup from Code.h
  setup1();

}

void returnSuccess(String msg) {
  server.send(200, "text/plain", msg);
}

void returnFailure(String msg) {
  server.send(500, "text/plain", msg);
}

uint8_t connectToWiFi() {
  // Delete old config
  WiFi.disconnect(true);
  // Try connecting WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("ESP32");
  bool isConnected = false;
  for (int i = 0; i < NETWORKS_COUNT; i++) {
    Serial.print("Connecting to ");
    Serial.print(SSIDs[i]);
    WiFi.begin(SSIDs[i], passwords[i]);
    for (int j = 0; j < 10; j++) {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
      } else {
        isConnected = true;
        break;
      }
    }
    Serial.println("");
    if (isConnected == true) {
      Serial.print("Connected to ");
      Serial.println(SSIDs[i]);
      Serial.print("ESP32 hostname: ");
      Serial.println(WiFi.getHostname());
      Serial.print("Ready! Access at: http://");
      Serial.println(WiFi.localIP());

      connectedToWiFiCallback();

      break;
    }
  }
  // If not connected, block device
  if (isConnected == false) {
    Serial.println("Unable to connect to WiFi");
  }
  return WiFi.status();
}

void webServerTask(void * pvParameters) {
  Serial.print("webServerTask running on core ");
  Serial.println(xPortGetCoreID());

  // Must establish WiFi connection
  while (connectToWiFi() != WL_CONNECTED) {
    Serial.println("Trying again...");
  }

  // OTA upgrade page (protected by basic auth) 
  server.on("/", HTTP_GET, 
    []() {
      if (!server.authenticate(authUser, authPass)) {
        return server.requestAuthentication();
      }
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", otaPageHtml);
    }
  );
  // Discovery endpoint
  server.on("/esp32_present", HTTP_GET, []() {returnSuccess("Present");});
  // Firmware version endpoint
  server.on("/fw_version", HTTP_GET, []() {returnSuccess(FW_VERSION);});
  // firmware upload endpoint
  server.on("/upgrade", HTTP_POST, 
    []() {
      returnSuccess(Update.hasError() ? "FAIL" : "OK");
      delay(1000);
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.println("Disabling WDT on core 0, so device doesn't reboot mid update...");
        disableCore0WDT();
        Serial.printf("Upgrade started, FW file: %s\r\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Upgrade completed, transfered: %u bytes\r\nRebooting device...\r\n", upload.totalSize);
          enableCore0WDT();
        } else {
          Update.printError(Serial);
        }
      }
    }
  );
  // LED commands endpoint
  server.on("/process_command", HTTP_POST, 
    []() {
      if (server.args() == 0) {
        returnFailure("No command sent");
      } else {
        String cmd = server.arg("command");
        if (cmd.length() == 0) {
          returnFailure("Empty command");
        } else {
          returnSuccess("Success");
          processCommand(cmd);
        }
      }
    }
  );
  // Start web server
  server.begin();

  while(true) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost, reconnecting...");
      // Must establish WiFi connection
      while (connectToWiFi() != WL_CONNECTED) {
        Serial.println("Trying again...");
      }
    }
    server.handleClient();
    delay(10); // MUST HAVE THIS DELAY
  } 
}

void loop() {
  // Execute loop from Code.h
  loop1();
}
