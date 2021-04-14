#include "config.h"
#include "secrets.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#define LED_NORMAL D4
#define LED_WARNING D5
#define LED_DANGER D6

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// HTTP Server
ESP8266WebServer server(SERVER_PORT);
String publicIP;

// CSE
const String CSE_URL = String(CSE_SCHEMA) + "://" + String(CSE_ADDRESS) + ":" + String(CSE_PORT) + "/";

// CNT
const String ACTUATORS_ENDPOINT = CSE_URL + "cse-in/SmartApp/Actuators";
bool isSubscribed = false;

// Device
float aqi = 0;

void setup()
{
  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_WARNING, OUTPUT);
  pinMode(LED_DANGER, OUTPUT);
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  if (connectWiFi())
  {
    getPublicIP();
    server.begin();
    server.on("/", handleAQI);

    if (handler(registerCnt("", String(RESOURCE_NAME)), "ACTUATOR CNT", "CREATION"))
    {
      // Create LOCATION and instantiate container
      handler(registerCnt("/" + String(RESOURCE_NAME), "LOCATION"), "LOCATION CNT", "CREATION");
      handler(registerCnt("/" + String(RESOURCE_NAME), "QUALITY"), "QUALITY CNT", "CREATION");
      subscribeQuality();
      while (!isSubscribed)
        server.handleClient();
      handler(postLocation(), "POSITION", "UPDATE");
      // Create QUALITY container and subscribe
    }
  }
  Serial.println("SETUP DONE");
}

void ledIndicator()
{
  digitalWrite(LED_NORMAL, LOW);
  digitalWrite(LED_WARNING, LOW);
  digitalWrite(LED_DANGER, LOW);
  Serial.print("With AQI=" + String(aqi) + " the danger is ");
  if (aqi != 0.0 && aqi < 4.0)
  {
    Serial.println("HIGH");
    digitalWrite(LED_DANGER, HIGH);
  }
  else if (aqi < 7.0)
  {
    Serial.println("MODERATE");
    digitalWrite(LED_WARNING, HIGH);
  }
  else
  {
    Serial.println("LOW");
    digitalWrite(LED_NORMAL, HIGH);
  }
}

void printLCD(const String &fl, const String &sl = "")
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(fl);
  if (sl.length() > 0)
  {
    lcd.setCursor(0, 1);
    lcd.print(sl);
  }
}

void loop()
{
  server.handleClient();
  ledIndicator();
  printLCD("AQI", String(aqi));
  delay(250);
}

void getPublicIP()
{
  HTTPClient client;
  if (!client.begin("api.ipify.org", 80))
  {
    Serial.println("Failed to connect with 'api.ipify.org' !");
  }
  else
  {
    client.GET();
    publicIP = client.getString();
    client.end();
    Serial.print("Public IP: ");
    Serial.println(publicIP);
  }
}

void handleAQI()
{ //Handler for the body path
  if (server.hasArg("plain") == false)
  { //Check if body received
    server.send(500, "text/plain", "Body not received");
    return;
  }
  StaticJsonDocument<192> req;
  deserializeJson(req, server.arg("plain"));
  // serializeJsonPretty(req, Serial);
  if (req["m2m:sgn"]["vrq"])
  {
    server.sendHeader("X-M2M-RSC", "2000");
    server.send(200, "text/plain", "SUBSCRIPTION VALIDATED\r\n");
    Serial.println(F("Subscription validated"));
    isSubscribed = true;
    return;
  }
  aqi = req["m2m:sgn"]["nev"]["rep"]["m2m:cin"]["con"];
  server.send(200, "text/plain", "AQI Received\r\n");
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
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    printLCD("CONNECTED");
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
int16_t handler(int16_t responseCode, const String &resource, const String &action)
{
  if (responseCode == 201)
  {
    Serial.println(resource + " " + action + " SUCCESS\n");
    printLCD(resource + " " + action, "SUCCESS");
    return 1;
  }
  else
  {
    Serial.print(resource + " " + action);
    Serial.printf(" ERROR: CODE %d\n\n", responseCode);
    printLCD(resource + " " + action, "ERROR");
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
  StaticJsonDocument<96> payload;
  payload["m2m:cin"]["cnf"] = "application/json:0";
  StaticJsonDocument<64> loc;
  loc["lat"] = String(LATITUDE, 6);
  loc["lon"] = String(LONGITUDE, 6);
  String locString;
  serializeJson(loc, locString);
  payload["m2m:cin"]["con"] = locString;
  String payloadString;
  serializeJson(payload, payloadString);
  return postToCse("/" + String(RESOURCE_NAME) + "/LOCATION", payloadString, 4);
}

uint16_t subscribeQuality()
{
  StaticJsonDocument<256> payload;
  JsonArray net = payload["m2m:sub"]["enc"].createNestedArray("net");
  net.add(3);

  JsonArray nu = payload["m2m:sub"].createNestedArray("nu");
  nu.add("http://" + publicIP + ":" + String(SERVER_PORT));

  JsonArray lbl = payload["m2m:sub"].createNestedArray("lbl");
  lbl.add("QUALITY");

  payload["m2m:sub"]["rn"] = "subQuality" + String(RESOURCE_NAME);
  payload["m2m:sub"]["nct"] = 1;
  String payloadString;
  serializeJson(payload, payloadString);
  return postToCse("/" + String(RESOURCE_NAME) + "/QUALITY", payloadString, 23);
}

uint32_t postToCse(const String &endpoint, const String &payload, uint8_t ty)
{
  HTTPClient client;
  String url = ACTUATORS_ENDPOINT + endpoint;
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
  /*
  StaticJsonDocument<1024> postResponse;
  deserializeJson(postResponse, client.getString());
  serializeJsonPretty(postResponse, Serial);
  Serial.println("\n");
  */
  client.end();
  return statusCode;
}
