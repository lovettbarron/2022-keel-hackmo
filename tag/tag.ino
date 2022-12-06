#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <ESP32Ping.h>

 
const char* ssid = "The House";
const char* password = "welcome!";
 
 
#define TFT_SCK    18
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_CS     22
#define TFT_DC     21
#define TFT_RESET  17
#define BUTTON     16

Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);


unsigned long previousMillis = 0;
unsigned long interval = 30000;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  writeBasic("Hello, Keel!");   
  delay(500);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    writeBasic("Connecting to Wifi");

    bool success = Ping.ping("www.google.com", 3);
 
  if(!success){
    writeBasic("No internet");
        delay(500);

    return;
  }
 
  Serial.println("Internet!");

  }

  delay(2000);
  Serial.println(WiFi.localIP());
}

void retryWifi() {
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    writeBasic("Retrying Wifi");

    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
}
 
void setup(void)
{
  display.begin();
  Serial.begin(115200);

  pinMode(BUTTON, INPUT_PULLUP);

    initWiFi();


}
 
void CheckInTag() {
  
}


 void writeBasic(String content)  {

  display.setRotation(3);
  display.fillScreen(BLACK);
  display.setCursor(20, 20);
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.print(content);
  display.setCursor(20,100);
  display.print(WiFi.localIP());
}


void loop() {
  int buttonState = digitalRead(BUTTON);
  if(buttonState) {
    writeBasic("Inactive");
  } else {
    writeBasic("Active");    
  }
  retryWifi();
  delay(500);
}
