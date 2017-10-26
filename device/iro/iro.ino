#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include "./configuration.h"
#include "./iroModesManager.h"
#include "./setupIroMode.h"
#include "./ringIroMode.h"
#include "./gaugeIroMode.h"
#include "./smileyIroMode.h"

const char* ssid = "";
const char* password = "";

ESP8266WebServer server(80);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXELS_PIN, NEO_GRB + NEO_KHZ800);

IroModesManager *iroModesManager = new IroModesManager(&server, &pixels);
RingIroMode *ringMode = new RingIroMode(iroModesManager);
GaugeIroMode *gaugeIroMode = new GaugeIroMode(iroModesManager);
SetupIroMode *setupIroMode = new SetupIroMode(iroModesManager);
SmileyIroMode *smileyIroMode = new SmileyIroMode(iroModesManager);

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

uint8_t chunk[1024];
bool fileLoaderToggle = false;
bool handleFileRead(String path) {
  iroModesManager->switchOffAllModes();
  fileLoaderToggle = !fileLoaderToggle;
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    File file = SPIFFS.open(path, "r");
    int fileSize = file.size();
    int fileSlice = fileSize / NUMPIXELS;
    while (fileSize > 0) {
      size_t chunkLength = std::min((int)(sizeof(chunk) - 1), fileSize);
      file.read((uint8_t *)chunk, chunkLength);
      server.client().write((const char*)chunk, chunkLength);
      fileSize -= chunkLength;
      for (int i = 0; i < NUMPIXELS - (fileSize / fileSlice) - 1; i++) {
        if (fileLoaderToggle) {
          pixels.setPixelColor(i, pixels.Color(0, 0, 20));
        } else {
          pixels.setPixelColor(i, pixels.Color(20, 5, 0));
        }
      }
      for (int i = NUMPIXELS - (fileSize / fileSlice); i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
      if (fileLoaderToggle) {
        pixels.setPixelColor(NUMPIXELS - 1, pixels.Color(0, 0, 20));
      } else {
        pixels.setPixelColor(NUMPIXELS - 1, pixels.Color(20, 5, 0));
      }
      pixels.show();
    }
    pixels.show();
    file.close();
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;
}

void handleNotFound() {
  if (!handleFileRead(server.uri())) {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  }
}

void showIp() {
  int deviceIp = WiFi.localIP()[3];
  int unit = deviceIp / 100;
  int remainder = deviceIp - (100 * unit);

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  for (int i = 0; i < unit; i++) {
    pixels.setPixelColor(i, pixels.Color(12, 0, 0));
  }
  unit = remainder / 10;
  remainder -= (10 * unit);
  for (int i = 2; i < unit + 2; i++) {
    pixels.setPixelColor(i, pixels.Color(9, 3, 0));
  }
  for (int i = 12; i < remainder + 12; i++) {
    pixels.setPixelColor(i, pixels.Color(6, 6, 0));
  }

  pixels.show();
}

bool waitingToggleStatus = false;
void waitingToggle() {
  if (waitingToggleStatus) {
    pixels.setPixelColor(0, pixels.Color(0, 1, 4));
    pixels.setPixelColor(6, pixels.Color(0, 0, 0));
    pixels.setPixelColor(12, pixels.Color(0, 1, 4));
    pixels.setPixelColor(18, pixels.Color(0, 0, 0));
  } else {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.setPixelColor(6, pixels.Color(4, 1, 0));
    pixels.setPixelColor(12, pixels.Color(0, 0, 0));
    pixels.setPixelColor(18, pixels.Color(4, 1, 0));
  }
  waitingToggleStatus = !waitingToggleStatus;
  pixels.show();
}

void setup(void) {
  pixels.begin();
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");
  SPIFFS.begin();
  int wifiStatus = WiFi.status();
  while (wifiStatus != WL_CONNECTED && wifiStatus != WL_CONNECT_FAILED && wifiStatus != WL_NO_SSID_AVAIL) {
    delay(500);
    waitingToggle();
    switch (wifiStatus) {
      case WL_CONNECTION_LOST:
        Serial.println("Connection lost");
        break;
      case WL_DISCONNECTED:
        Serial.println("Disconnected");
        break;
      case WL_IDLE_STATUS:
        Serial.println("Iddling");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("Connection failed");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("No SSID available");
        break;
      default:
        Serial.println(WiFi.status());
    }
    wifiStatus = WiFi.status();
  }

  if (wifiStatus == WL_CONNECTED) {
    Serial.println("Connection successful");
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    showIp();
  } else {
    Serial.println("Configuration mode on 192.168.1.1");
    iroModesManager->switchToMode(setupIroMode);
    IPAddress configurationIp(192, 168, 1, 1);
    IPAddress configurationNetworkMask(255, 255, 255, 0);
    WiFi.softAPConfig(configurationIp, configurationIp, configurationNetworkMask);
    WiFi.softAP(CONFIGURATION_AP, CONFIGURATION_PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(configurationIp);
  }

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/api/networks", []() {
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      server.send(200, "application/json", "[]");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      String apList = String("");
      for (int i = 0; i < n; ++i)
      {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
        apList += String("\"") + String(WiFi.SSID(i)) + String("\"");
        if (i < n - 1) {
          apList += String(",");
        }
        delay(10);
      }
      server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
      server.send(200, "text/plain", String("[") + apList + String("]"));
    }
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

}

void loop(void) {
  iroModesManager->animateModes();
  server.handleClient();
}
