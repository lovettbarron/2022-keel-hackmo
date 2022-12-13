#include <Arduino_GFX_Library.h>

#include <WiFi.h>
// #include <WiFiMulti.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char *rootCACertificate = //"C5:16:8F:D4:3F:7D:CC:75:40:30:38:99:EA:F6:D4:34:70:02:8F:F4";
    "-----BEGIN CERTIFICATE-----\n"
    "MIIF1DCCBLygAwIBAgIQDPUXjEzsDcfAT+ZjzUJmizANBgkqhkiG9w0BAQsFADBG\n"
    "MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRUwEwYDVQQLEwxTZXJ2ZXIg\n"
    "Q0EgMUIxDzANBgNVBAMTBkFtYXpvbjAeFw0yMjEwMDcwMDAwMDBaFw0yMzExMDUy\n"
    "MzU5NTlaMBkxFzAVBgNVBAMMDioua2VlbGFwcHMueHl6MIIBIjANBgkqhkiG9w0B\n"
    "AQEFAAOCAQ8AMIIBCgKCAQEAliPsxphaJMaDtQSkFenbVPt471arSNQVCnDT7naH\n"
    "xNsw+9IuyVUDkPOK3m0Tx32o871YhdqukuhL2rm/KSGu/JXbAgCxor1tlAyxEXCs\n"
    "asR/zdYxITYPpJTplQ7puw8IG1vZgd99YsOFdOfW040RJrE+wzgbehgvCJDVdncl\n"
    "HvYEmD1vjqUbaj4ZPlbgiWruivD1aeFSrPGfpIHbLA6WPPlxewUyTNyPlB34j8+D\n"
    "OpzOcqXRgG8TB/K010gFXQWK6dzUejYn135rpuPb3b4K5ZfJy30wIWUL86/+8xbA\n"
    "2Y6WRCf9os19dAjIcdzzdyCMlKpujwnVeK5OuPLyiYi/rwIDAQABo4IC6TCCAuUw\n"
    "HwYDVR0jBBgwFoAUWaRmBlKge5WSPKOUByeWdFv5PdAwHQYDVR0OBBYEFJwlEqpc\n"
    "TFoAIG7Z7m2NMSOEkoo4MBkGA1UdEQQSMBCCDioua2VlbGFwcHMueHl6MA4GA1Ud\n"
    "DwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwPQYDVR0f\n"
    "BDYwNDAyoDCgLoYsaHR0cDovL2NybC5zY2ExYi5hbWF6b250cnVzdC5jb20vc2Nh\n"
    "MWItMS5jcmwwEwYDVR0gBAwwCjAIBgZngQwBAgEwdQYIKwYBBQUHAQEEaTBnMC0G\n"
    "CCsGAQUFBzABhiFodHRwOi8vb2NzcC5zY2ExYi5hbWF6b250cnVzdC5jb20wNgYI\n"
    "KwYBBQUHMAKGKmh0dHA6Ly9jcnQuc2NhMWIuYW1hem9udHJ1c3QuY29tL3NjYTFi\n"
    "LmNydDAMBgNVHRMBAf8EAjAAMIIBfgYKKwYBBAHWeQIEAgSCAW4EggFqAWgAdgDo\n"
    "PtDaPvUGNTLnVyi8iWvJA9PL0RFr7Otp4Xd9bQa9bgAAAYOyYIqyAAAEAwBHMEUC\n"
    "IExMQd19+/BclZMtPHHIcVDheOIrrXDO7NomhpiuDi6tAiEAhV5EBY/A78qEoONn\n"
    "tRJ7g3h2Pzwtf8s3U4zDlydmLTEAdgCzc3cH4YRQ+GOG1gWp3BEJSnktsWcMC4fc\n"
    "8AMOeTalmgAAAYOyYIsXAAAEAwBHMEUCIQDOV6KbwE6YhtqN3DM5WWzx/Ub2R4Z3\n"
    "jxZ11KMF5MJJMAIgTDziw65J/mp5zNWCTGbFFLoz5bZT94JkIOEvKhGuN2kAdgC3\n"
    "Pvsk35xNunXyOcW6WPRsXfxCz3qfNcSeHQmBJe20mQAAAYOyYIrqAAAEAwBHMEUC\n"
    "IQCvdTtgatbPcBlRjDX9nskYigBOOY79fBxrlQ+Ge7URBQIgL6qnvgWvz7WOjHA6\n"
    "a7fNMpufpIg+md3DmQtw7gwNeXQwDQYJKoZIhvcNAQELBQADggEBACa6bZvTxJck\n"
    "vOBsgX6pHrPqpucNQcFcbLn5GoP7gvtCH/AgBSRt5f0Di+iMY4bPW2JNa2/h0XQX\n"
    "atInjf521461zC+fskNV1fBNQh6gwuPhHm8lloZKAY4Ahu9BoUwvZVCbPJPkH2L1\n"
    "/0ztNg2PvhmyAxWHFoSLBxuK8LARLJeCYu91Wu4SfbYbtiHInDJfyU/wQ7OUcBtc\n"
    "s/9OQyKMaIOGkgI5XLOTBkkHrp6uzQcu0S1ABo7Nrv/gxbogHjyo4BKSt6zxiCjQ\n"
    "lXwy/taMjmouIzFo7k/VUU/MucMxjIhH86dq2YFk2D7kAj5AhBgYWByQysTy7iBp\n"
    "dyUrvjp/uak=\n"
    "-----END CERTIFICATE-----\n";

struct Button
{
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  uint32_t pressedTime;
  bool pressed;
};

struct Keel
{
  char tagid;
  char patientname;
  char room;
  uint32_t lastChecked;
  char needs;
};

struct Screen
{
  uint32_t x;
  uint32_t y;
};

Screen s = {320, 240};

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

HTTPClient http; // Declare object of class HTTPClient
// WiFiMulti WiFiMulti;

const chat *payload = "query {}";

const char *host = "staging--2022-keel-hackmo-n4WKU0.keelapps.xyz";
const char *fingerprint = "C5:16:8F:D4:3F:7D:CC:75:40:30:38:99:EA:F6:D4:34:70:02:8F:F4";

Button b = {BUTTON, 0, false};
unsigned long debounceDelay = 250; // the debounce time; increase if the output flickers

// Button interrept
void IRAM_ATTR ISR()
{
  if ((millis() - b.pressedTime) > debounceDelay)
  {
    b.numberKeyPresses++;
    b.pressed = true;
    b.pressedTime = millis();
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
      delay(5000);
      checkInternet();
    }
    setClock();
    writeBasic("Internet");
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
  attachInterrupt(BUTTON, ISR, FALLING);

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

void loop()
{

  if (b.pressed)
  {
    writeBasic(String(b.numberKeyPresses));
    b.pressed = false;
    updateKeelStruct();
  }

  maintainConnection();
}

void updateKeelStruct()
{
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setInsecure();
    client->setCACert(rootCACertificate);
    client->setTimeout(15);

    if (!client->connect(host, 443))
    {
      Serial.println("Connection failed!");
    }
    else
    {
      Serial.println("Connected to server!");

      client->println("GET https://staging--2022-keel-hackmo-n4WKU0.keelapps.xyz/Web HTTP/1.0");
      client->println("Host: staging--2022-keel-hackmo-n4WKU0.keelapps.xyz");
      client->println("Connection: close");
      client->println();

      // while (client->connected())
      // {
      //   String line = client->readStringUntil('\n');
      //   if (line == "\r")
      //   {
      //     Serial.println("headers received");
      //     break;
      //   }
      // }
      // // if there are incoming bytes available
      // // from the server, read them and print them:

      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, host, 443, "/Web", true))
      { // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        // https.addHeader("Content-Type", "application/json");
        https.addHeader("Content-Type", "application/graphql");
        int httpCode = https.POST(payload);
        // while (client->available())
        // {
        //   char c = client->read();
        //   Serial.write(c);
        // }

        // httpCode will be negative on error
        if (httpCode > 0)
        {

          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
          String pay = https.getString();
          Serial.println(pay);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
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
