#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <WiFi.h>
// #include <WiFiMulti.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  uint32_t pressedTime;
  bool pressed;
  bool longPress;
  bool complete;
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

const char *payload = "{\"query\":\"query {\\n  allPatient(input: {}) {\\n\\t\\tedges {\\n\\t\\t\\tnode {\\n\\t\\t\\t\\tid\\n\\t\\t\\t\\tname\\n\\t\\t\\t\\troom {\\n\\t\\t\\t\\t\\tname\\n\\t\\t\\t\\t}\\n\\t\\t\\t\\tage\\n\\t\\t\\t}\\n\\t\\t}\\n  }\\n}\"}";

const char *patientQuery = "";
const char *CheckinQuery = "";
const char *AvailableNurse = "";

const char *host = "staging--2022-keel-hackmo-n4WKU0.keelapps.xyz";
const char *fingerprint = "C5:16:8F:D4:3F:7D:CC:75:40:30:38:99:EA:F6:D4:34:70:02:8F:F4";

Button b = { BUTTON, 0, false, false, false };
unsigned long debounceDelay = 50;       // the debounce time; increase if the output flickers
unsigned long longPressDuration = 500;  // long vs short press diff

// Button interrupt
void IRAM_ATTR ISR() {
  if ((millis() - b.pressedTime) > debounceDelay) {
    if (digitalRead(BUTTON) == HIGH)  // Has peen released
    {

      if (b.pressed) {
        if ((millis() - b.pressedTime) > longPressDuration) {

          b.longPress = true;
          b.complete = true;
        } else {
          b.complete = true;
        }
      }
    } else {
      b.pressed = true;

      if (!b.complete) {
        b.numberKeyPresses++;
      }
      b.pressedTime = millis();
      b.longPress = false;
      b.complete = false;
    }
  }
}

void resetButton() {
  b.pressedTime = millis();
  b.pressed = false;
  b.complete = false;
  b.longPress = false;
  Serial.println("ResetButton");
}

void timeoutButton() {
  if (digitalRead(BUTTON) == HIGH)  // Has peen released
  {
    if ((millis() - b.pressedTime) > debounceDelay + 2000) {
      resetButton();
    }
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
      delay(2000);
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
  attachInterrupt(BUTTON, ISR, CHANGE);

  initWiFi();
  // Serial.print(rootCACertificate);
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

  if (b.complete) {
    if (b.longPress) {
      writeBasic(String(String(b.numberKeyPresses) + " Long"));
    } else {
      writeBasic(String(String(b.numberKeyPresses) + " Short"));
    }
    // updateKeelStruct();
    resetButton();
  }
  timeoutButton();
  maintainConnection();
}

void updateKeelStruct() {
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    client->setInsecure();
    // client->setCACert(rootCACertificate);
    client->setTimeout(15);

    if (!client->connect(host, 443)) {
      Serial.println("Connection failed!");
    } else {
      Serial.println("Connected to server!");

      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, host, 443, "/Web", true)) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        // https.addHeader("Content-Type", "application/json");
        https.addHeader("Content-Type", "application/graphql");
        int httpCode = https.POST(payload);

        // httpCode will be negative on error
        if (httpCode > 0) {

          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String pay = https.getString();
            Serial.println(pay);
            // DynamicJsonDocument doc(1024);
            // deserializeJson(doc, pay);
          } else {
            String pay = https.getString();
            Serial.println(pay);
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      client->stop();
    }
  } else {
    Serial.println("Unable to create client");
  }
}
