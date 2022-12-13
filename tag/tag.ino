#include <Arduino_GFX_Library.h>

#include <WiFi.h>
// #include <WiFiMulti.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char *rootCACertificate =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
  "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
  "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
  "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
  "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
  "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
  "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
  "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
  "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
  "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
  "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
  "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
  "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
  "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
  "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
  "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
  "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
  "rqXRfboQnoZsG4q5WTP468SQvvG5"
  "-----END CERTIFICATE-----\n";

struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  uint32_t pressedTime;
  bool pressed;
};

struct Keel {
  char tagid;
  char patientname;
  char room;
  uint32_t lastChecked;
  char needs;
};

struct Screen {
  uint32_t x;
  uint32_t y;
};

Screen s = { 320, 240 };

#define TFT_SCK 18
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_CS 22
#define TFT_DC 21
#define TFT_RESET 17
#define BUTTON 16

const char *ssid = "Midipatch";
const char *password = "midihaus";

Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);

unsigned long previousMillis = 0;
unsigned long interval = 30000;

HTTPClient http;  // Declare object of class HTTPClient
// WiFiMulti WiFiMulti;

String payload = "query { allPatient(input: {}) { edges { node { 	id name room { name	} age	} } } }";


const char *host = "https://staging--2022-keel-hackmo-n4WKU0.keelapps.xyz";
const char *fingerprint = "123456789";

Button b = { BUTTON, 0, false };
unsigned long debounceDelay = 250;  // the debounce time; increase if the output flickers


// Button interrept
void IRAM_ATTR ISR() {
  if ((millis() - b.pressedTime) > debounceDelay) {
    b.numberKeyPresses++;
    b.pressed = true;
    b.pressedTime = millis();
  }
}

// Set clock
void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}



// Basic Wifi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  writeBasic("Hello, Keel!");
  checkInternet();
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void checkInternet() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    writeBasic("Connecting to Wifi");

    bool success = Ping.ping("https://google.com", 3);

    if (!success) {
      writeBasic("No internet");
      delay(5000);
      checkInternet();
    }
    setClock();
    writeBasic("Internet");
  }
}

void maintainConnection() {
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    Serial.print(millis());
    writeBasic("Retrying Wifi");

    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}

// Sketch Setup
void setup(void) {
  display.begin();
  Serial.begin(115200);

  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(BUTTON, ISR, FALLING);

  initWiFi();
}

// Basic text writing
void writeBasic(String content) {

  display.setRotation(3);
  display.fillScreen(BLACK);
  display.setCursor(.1 * s.x, .1 * s.y);
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.print(content);

  // Print IP
  display.setCursor(.1 * s.x, .9 * s.y);
  display.setTextSize(2);
  display.print(WiFi.localIP());
}

void loop() {

  if (b.pressed) {
    writeBasic(String(b.numberKeyPresses));
    b.pressed = false;
    updateKeelStruct();
  }

  maintainConnection();
}






void updateKeelStruct() {
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client->setCACert(rootCACertificate);

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, host, 443, "/Web", true)) {  // HTTPS
        Serial.print("[HTTPS] post...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/json");

        int httpCode = https.POST(payload);  // post w/ payload

        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] post... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] post... failed, error: %s %s\n", String(httpCode), https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }

    delete client;
  } else {
    Serial.println("Unable to create client");
  }
}
