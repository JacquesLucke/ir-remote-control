#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "circular_buffer.h"

// Has to define WIFI_NAME and WIFI_PASSWORD.
#include "private_data.h"

constexpr int web_server_port = 80;
ESP8266WebServer web_server(web_server_port);

constexpr int ir_receiver_pin = D1;
IRrecv ir_receiver(ir_receiver_pin);

// Use a name with more information for this type.
using IRDecodeResults = decode_results;

// Queue that keeps track of the last few ir messages.
CircularBuffer<IRDecodeResults, 5> last_ir_messages;

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

  web_server.on("/", []() {
    String response = "<h1>Remote Control</h1>";
    response += "<table style=\"border-spacing: 0.5em;\">";
    response += "<tr><th>Protocol</th><th>Hex Code</th></tr>";
    for (int i = 0; i < last_ir_messages.size(); i++) {
      const IRDecodeResults &ir_result =
          last_ir_messages.get_least_recently_added(i);
      response += "<tr>";
      response += "<td>" + typeToString(ir_result.decode_type) + "<th>";
      response += "<td>" + resultToHexidecimal(&ir_result) + "<th>";
      response += "</tr>";
    }
    response += "</table>";
    web_server.send(200, "text/html", response.c_str());
  });
}

void setup() {
  Serial.begin(9600);
  setupWiFiConnection();
  setupWebServer();
  ir_receiver.enableIRIn();
}

static void handleIRReceiver() {
  IRDecodeResults ir_result;
  if (ir_receiver.decode(&ir_result)) {
    if (!ir_result.repeat) {
      last_ir_messages.push(ir_result);
    }
    Serial.println(resultToHumanReadableBasic(&ir_result));
    ir_receiver.resume();
  }
}

void loop() {
  handleIRReceiver();
  web_server.handleClient();
}
