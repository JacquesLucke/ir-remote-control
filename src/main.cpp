#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

/* Has to define WIFI_NAME and WIFI_PASSWORD. */
#include "private_data.h"

constexpr int web_server_port = 80;
ESP8266WebServer web_server(web_server_port);

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

static void setupWebServer() {
  web_server.begin();
  Serial.printf("Started web server on port %d.\n", web_server_port);

  web_server.on("/",
                []() { web_server.send(200, "text/html", "Remote Control"); });
}

void setup() {
  Serial.begin(9600);
  setupWiFiConnection();
  setupWebServer();
}

void loop() {
  web_server.handleClient();
}
