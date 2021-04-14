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

// AQI
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
      handler(subscribeQuality(), "SUBSCRIPTION", "REQUEST");
      while (!isSubscribed)
        server.handleClient();
      handler(postLocation(), "POSITION", "UPDATE");
      // Create QUALITY container and subscribe
    }
  }
}

void loop()
{
  server.handleClient();
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

void printLCD(String fl, String sl = "")
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(fl);
  if (sl.length() > 0)
    ;
  {
    lcd.setCursor(0, 1);
    lcd.print(sl);
  }
}

void handleAQI()
{ //Handler for the body path
  Serial.println("== HANDLER HANDLER HANDLER ==");
  if (server.hasArg("plain") == false)
  { //Check if body received
    server.send(500, "text/plain", "Body not received");
    return;
  }
  String reqBody = server.arg("plain");
  Serial.println("HOT FROM THE CSEEEEEEEEEEEEEEEEEEEEEEEEE");
  Serial.println(reqBody);
  StaticJsonDocument<128> req;
  DeserializationError error = deserializeJson(req, reqBody);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  if (req["m2m:sgn"]["vrq"])
  {
    server.sendHeader("X-M2M-RSC", "2000");
    server.send(200, "text/plain", "verified");
    isSubscribed = true;
    return;
  }
  aqi = req["m2m:sgn"]["nev"]["rep"]["m2m:cin"]["con"];
  Serial.println(aqi);
  printLCD("AQI", String(aqi));
  server.send(200, "application/json", "{}");
}

void ledIndicator()
{
  digitalWrite(LED_NORMAL, LOW);
  digitalWrite(LED_WARNING, LOW);
  digitalWrite(LED_DANGER, LOW);
  if (aqi != 0 && aqi < 4)
  {
    digitalWrite(LED_DANGER, HIGH);
  }
  else if (aqi < 7)
  {
    digitalWrite(LED_WARNING, HIGH);
  }
  else
  {
    digitalWrite(LED_NORMAL, HIGH);
  }
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
int16_t handler(int16_t responseCode, String resource, String action)
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

uint16_t registerCnt(String endpoint, String rn)
{
  DynamicJsonDocument payload(1024);
  payload["m2m:cnt"]["mni"] = 1;
  payload["m2m:cnt"]["rn"] = rn;
  String payloadString;
  serializeJson(payload, payloadString);
  return postToCse(endpoint, payloadString, 3);
}

uint16_t postLocation()
{
  DynamicJsonDocument payload(96);
  payload["m2m:cin"]["cnf"] = "application/json:0";
  DynamicJsonDocument loc(48);
  loc["lat"] = LATITUDE;
  loc["lon"] = LONGITUDE;
  String locString;
  serializeJson(loc, locString);
  Serial.println("Location string: " + locString);
  payload["m2m:cin"]["con"] = locString;
  String payloadString;
  Serial.println("Payload string: " + payloadString);
  serializeJson(payload, payloadString);
  return postToCse("/" + String(RESOURCE_NAME) + "/LOCATION", payloadString, 4);
}

uint16_t subscribeQuality()
{
  String rn = "subQuality" + String(RESOURCE_NAME);
  DynamicJsonDocument payload(1024);
  JsonArray net = payload["m2m:sub"]["enc"].createNestedArray("net");
  net.add(3);
  net.add(4);
  JsonArray nu = payload["m2m:sub"].createNestedArray("nu");
  nu.add("http://" + WiFi.localIP().toString() + ":" + String(SERVER_PORT));
  payload["m2m:sub"]["rn"] = rn;
  payload["m2m:sub"]["nct"] = 1;
  String payloadString;
  serializeJson(payload, payloadString);
  return postToCse("/" + String(RESOURCE_NAME) + "/QUALITY/la", payloadString, 23);
}

uint32_t postToCse(String endpoint, String payload, uint8_t ty)
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
  Serial.print(payload);
  Serial.print("\n\nResponse: ");
  DynamicJsonDocument res(1024);
  deserializeJson(res, client.getString());
  serializeJsonPretty(res, Serial);
  Serial.println();
  Serial.println();
  client.end();
  return statusCode;
}
