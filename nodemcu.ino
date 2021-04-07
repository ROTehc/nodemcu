#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "secrets.h"
#include "config.h"


// Pinout
#define BUTTON 16

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// AE
const String AE_ENDPOINT = String(CSE_ENDPOINT) + "/" + String(RESOURCE_NAME);

// Vars
uint32_t delayStart = 0;
uint8_t buttonPrev;
uint8_t successfulSetup = 0;

void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  pinMode(BUTTON, INPUT);

  if (connectWiFi()) {
    if (handler(registerAE(), "AE", "REGISTRATION")) {
      delay(5000);
      if (handler(registerCnt("DESCRIPTOR"), "DESCRIPTOR", "CREATION")) {
        delay(5000);
        if (handler(registerCnt("DATA"), "DATA", "CREATION")) {
          delay(5000);

          String positionData = "[" + String(LONGITUDE, 6) + "," + String(LATITUDE, 6) + "]";
          if (handler(
                postData(AE_ENDPOINT + "/DESCRIPTOR", positionData),
                "POSITION", "UPDATE"
              )) {
            successfulSetup = 1;
            pinMode(BUTTON, INPUT_PULLUP);
            buttonPrev = digitalRead(BUTTON);
          }
        }
      }
    }
  }
}

void loop()
{
  if (successfulSetup && ((delayStart == 0) || ((millis() - delayStart > REQUEST_PERIOD)))) {
    delayStart = millis();
    handler(
      postData(AE_ENDPOINT + "/DATA", readData()),
      "DATA", "UPDATE"
    );
  }
}

void printLCD(String fl, String sl = "")
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(fl);
  if (sl.length() > 0);
  {
    lcd.setCursor(0, 1);
    lcd.print(sl);
  }
}

String readData() {
  String data;
  data += "{co2:" + String(random(100, 2500)) + ",";
  data += "o3:" + String(random(5, 125)) + ",";
  data += "no2:" + String(random(5, 76)) + ",";
  data += "so2:" + String(random(10, 250)) + "}";
  return data;
}

// Setup Functions
uint8_t connectWiFi()
{
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  uint8_t i = 0;
  Serial.print("Connecting to: ");
  Serial.println(SECRET_SSID);

  printLCD("CONNECTING");

  while (WiFi.status() != WL_CONNECTED and i < MAX_ATTEMPTS)
  {
    Serial.print(".");
    delay(500);
    lcd.setCursor(i, 1);
    lcd.print("*");
    i++;
  }
  if (i < MAX_ATTEMPTS)
  {
    Serial.println("\nConnection established!");
    printLCD("CONNECTED TO", SECRET_SSID);
    return 1;
  }
  else
  {
    Serial.println("\nConnection to WiFi failed.");
    printLCD("ERROR");
    return 0;
  }
}

// CSE Functions
int16_t handler(int16_t responseCode, String resource, String action) {
  if (responseCode == 201)
  {
    Serial.println(resource + " " + action + " SUCCESS\n");
    printLCD(resource + " " + action, "SUCCESS");
    return 1;
  } else {
    Serial.print(resource + " " + action);
    Serial.printf(" ERROR: CODE %d\n\n", responseCode);
    printLCD(resource + " " + action, "ERROR");
    return 0;
  }
}

uint16_t registerAE()
{
  String payload = "{\"m2m:ae\":{\"rn\":\"" +
                   String(RESOURCE_NAME) +
                   String("\",\"api\":\"N.ROTehc.com.") +
                   String(RESOURCE_NAME) +
                   String("\",\"srv\":[\"3\"],\"rr\":false}}");

  return postToCse(CSE_ENDPOINT, payload, 2);
}

uint16_t registerCnt(String rn) {
  String payload = "{\"m2m:cnt\":{\"mbs\":10000,\"mni\":10,\"rn\":\"" + rn + "\"}}";
  return postToCse(AE_ENDPOINT, payload, 3);
}

uint16_t postData(String endpoint, String data) {
  String payload = "{\"m2m:cin\":{\"cnf\":\"application/json\",\"con\":\"" + data + "\"}}";
  return postToCse(endpoint, payload, 4);
}

uint32_t postToCse(String endpoint, String payload, uint8_t ty)
{
  HTTPClient client;
  String url = String(SECRET_HOST) + ":" + String(SECRET_PORT) + endpoint;
  if (!client.begin(url)) {
    Serial.println("Connection to host failed");
    return 0;
  }
  client.addHeader("X-M2M-Origin", CSE_ORIGINATOR);
  client.addHeader("X-M2M-RVI", String(CSE_RELEASE));
  client.addHeader("X-M2M-RI", "123456");
  client.addHeader("Content-Type", "application/json;ty=" + String(ty));
  client.addHeader("Content-Length", String(payload.length()));
  client.addHeader("Accept", "application/json");

  // Send POST
  int16_t statusCode = client.POST(payload);
  Serial.println("POST sent to: " + String(SECRET_HOST) + ":" + String(SECRET_PORT) + endpoint + " with status " + statusCode);
  Serial.print(payload);
  Serial.print("\n\nResponse: ");
  Serial.println(client.getString());
  Serial.println();
  client.end();
  return statusCode;
}
