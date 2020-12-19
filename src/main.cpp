#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

#include "circular_buffer.h"

// Has to define WIFI_NAME and WIFI_PASSWORD.
#include "private_data.h"

constexpr int web_server_port = 80;
ESP8266WebServer web_server(web_server_port);

constexpr int ir_receiver_pin = D1;
IRrecv ir_receiver(ir_receiver_pin);

constexpr int ir_send_pin = D2;
IRsend ir_sender(ir_send_pin);

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

static String makeSendButtonNED(const char *name, const char *hex_code) {
  return "<button type='button' "
         "onclick=\"fetch('./send_ir?hex_code=" +
         String(hex_code) + "')\">" + name + "</button>";
}

static void setupWebServer() {
  web_server.begin();
  Serial.printf("Started web server on port %d.\n", web_server_port);

  web_server.on("/", []() {
    String response;
    response += R"STR(
      <!DOCTYPE>
      <html lang='en'>
      <head>
        <meta charset='UTF-8'>
        <meta name='viewport' content='width=device-width, initial-scale=1.0' />
        <title>IR Remote</title>
        <link rel='stylesheet' href='./style.css'>
      </head>
      <body>
    )STR";
    response += "<h1>Remote Control</h1>";

    response += makeSendButtonNED("Reduce Volume", "0x1EA0CF3");
    response += makeSendButtonNED("Increase Volume", "0x1EA0DF2");

    response += "<table style=\"border-spacing: 0.5em;\">";
    response += "<tr><th>Protocol</th><th>Hex Code</th></tr>";
    for (int i = 0; i < last_ir_messages.size(); i++) {
      const IRDecodeResults &ir_result =
          last_ir_messages.get_least_recently_added(i);
      const String hex_code = resultToHexidecimal(&ir_result);
      response += "<tr>";
      response += "<td>" + typeToString(ir_result.decode_type) + "<th>";
      response += "<td>" + hex_code + "<th>";
      response += "</tr>";
    }
    response += "</table>";
    response += "</body></html>";
    web_server.send(200, "text/html", response.c_str());
  });

  web_server.on("/send_ir", []() {
    String hex_code = web_server.arg("hex_code");
    Serial.println(hex_code);

    uint32_t code = 0;
    sscanf(hex_code.c_str() + 2, "%x", &code);
    Serial.println(code);
    Serial.println(uint64ToString(code, 16));

    ir_receiver.disableIRIn();
    ir_sender.sendNEC(code);
    ir_receiver.enableIRIn();

    web_server.send(200, "text/html", "code send");
  });

  web_server.on("/style.css", []() {
    web_server.send(200, "text/css", R"STR(
      * {
        color: blue;
      }
    )STR");
  });
}

void setup() {
  Serial.begin(9600);
  setupWiFiConnection();
  setupWebServer();
  ir_receiver.enableIRIn();
  ir_sender.begin();
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
