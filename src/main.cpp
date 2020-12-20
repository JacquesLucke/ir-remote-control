#include <Adafruit_SSD1306.h>
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

TwoWire display_wire;
Adafruit_SSD1306 display{128, 32, &display_wire};

constexpr int ir_receiver_pin = D1;
IRrecv ir_receiver(ir_receiver_pin);

constexpr int ir_send_pin = D2;
IRsend ir_sender(ir_send_pin);

// Use a name with more information for this type.
using IRDecodeResults = decode_results;

// Queue that keeps track of the last few ir messages.
CircularBuffer<IRDecodeResults, 5> last_ir_messages;

static void updateDisplayText(const char *text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(text);
  display.display();
}

static void setupWiFiConnection() {
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  Serial.print("Connecting");
  updateDisplayText("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  String display_text = WiFi.localIP().toString();
  updateDisplayText(display_text.c_str());
}

static String makeQuickSendButton(const char *name, const char *hex_code) {
  return "<div class='quick-send-button' "
         "onclick=\"fetch('./send_ir?hex_code=" +
         String(hex_code) + "')\">" + name + "</div>";
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

    response += "<div class='quick-buttons-container'>";
    response += makeQuickSendButton("Decrease Volume", "0x1EA0CF3");
    response += makeQuickSendButton("Increase Volume", "0x1EA0DF2");
    response += makeQuickSendButton("Play/Pause", "0x1EAFE01");
    response += makeQuickSendButton("Show Display", "0x1EA1DE2");
    response += makeQuickSendButton("Use Bluetooth", "0x1EA22DD");
    response += makeQuickSendButton("Use TV", "0x1EAA25D");
    response += makeQuickSendButton("Speaker On/Off", "0x1EA12ED");
    response += makeQuickSendButton("TV On/Off", "0x20DF10EF");
    response += makeQuickSendButton("Netflix", "0x20DF6A95");
    response += makeQuickSendButton("Prime", "0x20DF3AC5");
    response += "</div>";

    if (last_ir_messages.size() > 0) {
      response += "<h3>Last Received Signals</h3>";
      response += "<table>";
      response += "<tr><th>Protocol</th><th>Hex Code</th><th>Send</th></tr>";
      for (int i = 0; i < last_ir_messages.size(); i++) {
        const IRDecodeResults &ir_result =
            last_ir_messages.get_least_recently_added(i);
        const String hex_code = resultToHexidecimal(&ir_result);
        response += "<tr>";
        response += "<td>" + typeToString(ir_result.decode_type) + "</td>";
        response += "<td>" + hex_code + "</td>";
        response +=
            "<td><button type='button' onclick='fetch(\"./send_ir?hex_code=" +
            hex_code + "\")'>Send</button></td>";
        response += "</tr>";
      }
      response += "</table>";
    }
    response += "</body></html>";
    web_server.send(200, "text/html", response.c_str());
  });

  web_server.on("/send_ir", []() {
    String hex_code = web_server.arg("hex_code");
    Serial.println(hex_code);

    uint32_t code = 0;
    sscanf(hex_code.c_str() + 2, "%x", &code);

    ir_receiver.disableIRIn();
    ir_sender.sendNEC(code);
    ir_receiver.enableIRIn();

    web_server.send(200, "text/html", "code send");
  });

  web_server.on("/style.css", []() {
    web_server.send(200, "text/css", R"STR(
      body {
        margin: 0;
      }

      .quick-buttons-container {
        display: grid;
        width: 100%;
        grid-template-columns: repeat(auto-fit, minmax(10rem, 1fr));
      }

      .quick-send-button {
        background-color: #CCCCCC;
        text-align: center;
        margin: 0.3rem;
        padding-top: 2rem;
        padding-bottom: 2rem;
        user-select: none;
      }

      .quick-send-button:hover {
        background-color: #AAAAAA;
      }

      .quick-send-button:active {
        background-color: #888888;
      }

      th, td {
        padding: 0.5em;
        text-align: left;
        border-bottom: 1px solid #ddd;
      }
    )STR");
  });
}

static void setupDisplay() {
  display_wire.begin(/* sda */ D5, /* scl */ D6);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  Serial.println("Setup Display.");
}

void setup() {
  Serial.begin(9600);
  setupDisplay();
  setupWiFiConnection();
  setupWebServer();
  ir_receiver.enableIRIn();
  ir_sender.begin();
}

static void handleIRReceiver() {
  IRDecodeResults ir_result;
  if (ir_receiver.decode(&ir_result)) {
    if (!ir_result.repeat && ir_result.decode_type == NEC) {
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
