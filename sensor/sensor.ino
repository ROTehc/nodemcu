#include "config.h"
#include "secrets.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// Pinout
#define SETUP_LED 2
#define SEND_LED 16

// CSE
const String CSE_URL = String(CSE_SCHEMA) + "://" + String(CSE_ADDRESS) + ":" + String(CSE_PORT) + "/";

// CNT
const String SENSORS_ENDPOINT = CSE_URL + "cse-in/SmartApp/Sensors";
bool isSubscribed = false;

void setup()
{
  Serial.begin(115200);
  pinMode(SETUP_LED, OUTPUT);
  pinMode(SEND_LED, OUTPUT);
  pinMode(A0, INPUT);

  if (connectWiFi())
  {
    if (handler(registerCnt("", String(RESOURCE_NAME)), "SENSOR CNT", "CREATION"))
    {
      // Create LOCATION and instantiate container
      handler(registerCnt("/" + String(RESOURCE_NAME), "LOCATION"), "LOCATION CNT", "CREATION");
      handler(registerCnt("/" + String(RESOURCE_NAME), "DATA"), "DATA CNT", "CREATION");
      handler(postLocation(), "POSITION", "UPDATE");
    }
  }
  digitalWrite(SETUP_LED, LOW);
  Serial.println("SETUP DONE");
}

void loop()
{
  digitalWrite(SEND_LED, LOW);
  handler(postReading(), "DATA", "UPDATE");
  digitalWrite(SEND_LED, HIGH);
  delay(REPORT_FREQ);
}

// Setup Functions
uint8_t connectWiFi()
{
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  uint8_t i = 0;
  Serial.print("Connecting to: ");
  Serial.println(SECRET_SSID);

  while (WiFi.status() != WL_CONNECTED and i < MAX_ATTEMPTS)
  {
    Serial.print(".");
    delay(500);
    i++;
  }
  if (i < MAX_ATTEMPTS)
  {
    Serial.println("\nConnection established!");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    return 1;
  }
  else
  {
    Serial.println("\nConnection to WiFi failed.");
    return 0;
  }
}

// CSE Functions
int16_t handler(int16_t responseCode, const String &resource, const String &action)
{
  if (responseCode == 201)
  {
    Serial.println(resource + " " + action + " SUCCESS\n");
    return 1;
  }
  else
  {
    Serial.print(resource + " " + action);
    Serial.printf(" ERROR: CODE %d\n\n", responseCode);
    return 0;
  }
}

uint16_t registerCnt(const String &endpoint, const String &rn)
{
  StaticJsonDocument<128> payload;
  payload["m2m:cnt"]["mni"] = 1;
  payload["m2m:cnt"]["rn"] = rn;
  String payloadString;
  serializeJson(payload, payloadString);
  return postToCse(endpoint, payloadString, 3);
}

uint16_t postLocation()
{
  StaticJsonDocument<192> payload;
  payload["m2m:cin"]["cnf"] = "application/json:0";
  StaticJsonDocument<92> loc;
  loc["lat"] = String(LATITUDE, 6);
  loc["lon"] = String(LONGITUDE, 6);
  String locString;
  serializeJson(loc, locString);
  payload["m2m:cin"]["con"] = locString;
  JsonArray lbl = payload["m2m:cin"].createNestedArray("lbl");
  lbl.add("LOCATION");
  String payloadString;
  serializeJson(payload, payloadString);
  return postToCse("/" + String(RESOURCE_NAME) + "/LOCATION", payloadString, 4);
}

uint16_t postReading()
{
  int co2 = 1500 / 1024 * analogRead(A0) + 200;
  Serial.println("CO2: " + co2);
  StaticJsonDocument<192> payload;
  payload["m2m:cin"]["cnf"] = "application/json:0";
  StaticJsonDocument<92> con;
  con["co2"] = co2;
  con["o3"] = random(20, 65);
  con["no2"] = random(15, 50);
  con["so2"] = random(15, 50);
  String conString;
  serializeJson(con, conString);
  payload["m2m:cin"]["con"] = conString;
  JsonArray lbl = payload["m2m:cin"].createNestedArray("lbl");
  lbl.add("DATA");
  String payloadString;
  serializeJson(payload, payloadString);
  return postToCse("/" + String(RESOURCE_NAME) + "/DATA", payloadString, 4);
}

uint32_t postToCse(const String &endpoint, const String &payload, uint8_t ty)
{
  HTTPClient client;
  String url = SENSORS_ENDPOINT + endpoint;
  if (!client.begin(url))
  {
    Serial.println("Connection to host failed");
    return 0;
  }
  client.addHeader("X-M2M-Origin", "SmartApp");
  client.addHeader("X-M2M-RVI", String(CSE_RELEASE));
  client.addHeader("X-M2M-RI", String(random(10000, 99999)));
  client.addHeader("Content-Type",
                   "application/vnd.onem2m-res+json; ty=" + String(ty));
  client.addHeader("Content-Length", String(payload.length()));
  client.addHeader("Accept", "application/json");

  // Send POST
  int16_t statusCode = client.POST(payload);
  Serial.println("POST sent to: " + url + " with status " + statusCode);
  Serial.println(payload);
  Serial.println("Response:");
  Serial.println(client.getString());
  client.end();
  return statusCode;
}
