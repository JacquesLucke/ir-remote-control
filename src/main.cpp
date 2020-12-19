#include <Arduino.h>
#include <ESP8266WiFi.h>

/* Has to define WIFI_NAME and WIFI_PASSWORD. */
#include "private_data.h"

static void setupWiFiConnection() {
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);
  setupWiFiConnection();
}

void loop() {
}
