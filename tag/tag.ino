#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <WiFi.h>
// #include <WiFiMulti.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

enum View
{
  TAG,
  STAFF,
  CHECKIN,
  OFFLINE,
  DEBUG
};

struct Button
{
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  uint32_t pressedTime;
  bool pressed;
  bool longPress;
  bool complete;
};

struct Keel
{
  char id[32];
  char patientname[32];
  char room[32];
  uint32_t lastChecked;
  char needs[32];
};

struct Staff
{
  unsigned char id;
  unsigned char name;
  bool selected;
  bool active;
};

Staff staff_array[6];
int staff_select = -1;

struct Screen
{
  View state;
  uint32_t x;
  uint32_t y;
  bool rerender;
};

Screen s = {OFFLINE, 320, 240, true};

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

// wifi recheck
unsigned long previousMillis = 0;
unsigned long interval = 30000;

// api recheck
unsigned long previousMillisApi = -120000;
unsigned long intervalApi = 120000;

HTTPClient http; // Declare object of class HTTPClient

// WiFiMulti WiFiMulti;

const char *patientQuery = "";
const char *CheckinQuery = "";
const char *AvailableStaff = "{\"query\":\"query {\\n  listAllStaff(input: {}) {\\n\\t\\tedges {\\n\\t\\t\\tnode {\\n\\t\\t\\t\\tid\\n\\t\\t\\t\\tname\\n\\t\\t\\t}\\n\\t\\t}\\n  }\\n}\"}";

const char *host = "staging--2022-keel-hackmo-n4WKU0.keelapps.xyz";
const char *fingerprint = "C5:16:8F:D4:3F:7D:CC:75:40:30:38:99:EA:F6:D4:34:70:02:8F:F4";

Button b = {BUTTON, 0, false, false, false};
unsigned long debounceDelay = 50;      // the debounce time; increase if the output flickers
unsigned long longPressDuration = 500; // long vs short press diff

// Button interrupt
void IRAM_ATTR ISR()
{
  if ((millis() - b.pressedTime) > debounceDelay)
  {
    if (digitalRead(BUTTON) == HIGH) // Has peen released
    {

      if (b.pressed)
      {
        if ((millis() - b.pressedTime) > longPressDuration)
        {

          b.longPress = true;
          b.complete = true;
        }
        else
        {
          b.complete = true;
        }
      }
    }
    else
    {
      b.pressed = true;

      if (!b.complete)
      {
        b.numberKeyPresses++;
      }
      b.pressedTime = millis();
      b.longPress = false;
      b.complete = false;
    }
  }
}

void resetButton()
{
  b.pressedTime = millis();
  b.pressed = false;
  b.complete = false;
  b.longPress = false;
}

void timeoutButton()
{
  if (digitalRead(BUTTON) == HIGH) // Has peen released
  {
    if ((millis() - b.pressedTime) > debounceDelay + 2000)
    {
      resetButton();
    }
  }
}

// Set clock
void setClock()
{
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2)
  {
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
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  writeBasic("Hello, Keel!");
  checkInternet();
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void checkInternet()
{
  while (WiFi.status() != WL_CONNECTED)
  {

    delay(500);
    writeBasic("Connecting to Wifi");

    bool success = Ping.ping("https://google.com", 3);

    if (!success)
    {
      writeBasic("No internet");
      delay(2000);
      checkInternet();
      s.state = OFFLINE;
    }
    setClock();
    writeBasic("Internet");
    s.state = TAG;
  }
}

void maintainConnection()
{
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval))
  {
    Serial.print(millis());
    writeBasic("Retrying Wifi");

    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}

// Sketch Setup
void setup(void)
{
  display.begin();
  Serial.begin(115200);

  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(BUTTON, ISR, CHANGE);

  initWiFi();
  // Serial.print(rootCACertificate);
}

// Basic text writing
void writeBasic(String content)
{

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

// Basic text writing
void writeTag()
{

  display.setRotation(3);
  if (s.rerender)
  {
    display.fillScreen(BLACK);
    display.setCursor(.05 * s.x, .05 * s.y);
    display.setTextSize(4);
    display.setTextColor(WHITE);
    display.print("Room 33");
    display.setCursor(.05 * s.x, .2 * s.y);
    display.print("Patient Name");

    display.setTextSize(2);

    display.setCursor(.05 * s.x, .5 * s.y);
    display.print("Needs");

    s.rerender = false;
  }
  display.setTextSize(2);

  display.setCursor(.1 * s.x, .7 * s.y);
  display.print("Time since checked");

  // Print IP
  display.setCursor(.1 * s.x, .9 * s.y);
  display.setTextSize(2);
  display.print(WiFi.localIP());
}

void writeCheckin()
{

  display.setRotation(3);
  display.fillScreen(RED);
  display.setCursor(.0 * s.x, .3 * s.y);
  display.setTextSize(6);
  display.setTextColor(WHITE);
  display.print("Good Job!");

  // Print IP
  display.setCursor(.1 * s.x, .9 * s.y);
  display.setTextSize(2);
  display.print(WiFi.localIP());
}

// Basic text writing
void writeStaff()
{

  display.setRotation(3);
  if (s.rerender)
  {
    display.fillScreen(WHITE);
    s.rerender = false;
  }

  int index = 0;

  for (auto obj : staff_array)
  {
    Serial.println(staff_array[index].name);
    display.setCursor(.1 * s.x, (index + 1) * .1 * s.y);
    display.setTextSize(3);
    if (index == staff_select)
    {
      display.setTextColor(RED);
    }
    else
    {
      display.setTextColor(BLACK);
    }
    display.print(staff_array[index].name);
    index++;
  }

  // Print IP
  display.setCursor(.1 * s.x, .9 * s.y);
  display.setTextSize(2);
  display.print(WiFi.localIP());
}

void checkStateChangeNeede()
{
  switch (s.state)
  {
  case TAG:
    if (b.complete)
    {

      s.state = STAFF;
      s.rerender = true;
      writeStaff();

      resetButton();
    }
    writeTag();

    break;

  case STAFF:
    if (b.complete)
    {
      if (b.longPress)
      {
        staff_array[staff_select].active = true;
        s.state = CHECKIN;
      }
      else
      {
        staff_select = (staff_select + 1) % 6;
      }
      writeStaff();
      resetButton();
    }

    break;
  case CHECKIN:
    writeCheckin();
    s.rerender = true;

    delay(2000);
    s.state = TAG;

  default:
    // statements
    break;
  }
}

void loop()
{

  // refreshes data periodically
  unsigned long currentMillisApi = millis();
  if (!b.complete && (currentMillisApi - previousMillisApi >= intervalApi))
  {
    updateKeelStruct();
    previousMillisApi = currentMillisApi;
  }

  checkStateChangeNeede();

  // Basic reset stuff
  timeoutButton();
  maintainConnection();
}

void updateKeelStruct()
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setInsecure();
    // client->setCACert(rootCACertificate);
    client->setTimeout(5);

    if (!client->connect(host, 443))
    {
      Serial.println("\nConnection failed!");
    }
    else
    {
      Serial.println("\nConnected to server!");

      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, host, 443, "/Web", true))
      { // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        // https.addHeader("Content-Type", "application/json");
        https.addHeader("Content-Type", "application/graphql");
        int httpCode = https.POST(AvailableStaff);

        // httpCode will be negative on error
        if (httpCode > 0)
        {

          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            String pay = https.getString();
            Serial.println(pay);
            parseStaff(&pay);
          }
          else
          {
            String pay = https.getString();
            Serial.println(pay);
          }
        }
        else
        {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      }
      else
      {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      client->stop();
    }
  }
  else
  {
    Serial.println("Unable to create client");
  }
}

void parseStaff(String *payload)
{

  char p[payload->length() + 1];
  payload->toCharArray(p, payload->length() + 1);

  StaticJsonDocument<200> filter;
  filter["data"]["listAllStaff"]["edges"][0]["node"]["id"] = true;
  filter["data"]["listAllStaff"]["edges"][0]["node"]["name"] = true;

  StaticJsonDocument<400> doc;
  deserializeJson(doc, p, DeserializationOption::Filter(filter));
  serializeJsonPretty(doc, Serial);

  JsonArray a = doc["data"]["listAllStaff"]["edges"];
  unsigned int index = 0;
  for (auto obj : a)
  {
    // serializeJsonPretty(obj["node"]["name"], Serial);
    unsigned char id = obj["node"]["id"].as<unsigned char>();
    unsigned char name = obj["node"]["name"].as<unsigned char>();

    staff_array[index] = {id, name, false, false};

    Serial.println(staff_array[index].name);

    index++;
  }
}