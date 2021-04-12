#include "config.h"
#include "secrets.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

#define LED_WARNING D4
#define LED_NORMAL D5
#define LED_DANGER D6

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// HTTP Server
ESP8266WebServer server(SERVER_PORT);
String publicIP = "";

// CSE
const String address = String(CSE_SCHEMA) + "://" + String(CSE_ADDRESS) + ":" + String(CSE_PORT);

// AE
const String AE_ENDPOINT = String(CSE_ENDPOINT) + "/" + String(ACTUATORS_GROUP);
const String ORIGINATOR = String(CSE_ORIGINATOR);

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

    if (handler(registerCnt(AE_ENDPOINT, RESOURCE_NAME, 1), "ACTUATOR CNT", "CREATION"))
    {
      // Create LOCATION and instantiate container
      handler(registerCnt(AE_ENDPOINT + "/" + RESOURCE_NAME, "LOCATION", 1), "LOCATION CNT", "CREATION");
      // TODO: send JSON location without errors
      //String positionData = "{\"lon\":" + String(LONGITUDE, 6) + ",\"lat\":" + String(LATITUDE, 6) + "}";
      String positionData = "[mock]";
      handler(postData(AE_ENDPOINT + "/" + RESOURCE_NAME + "/LOCATION", positionData), "POSITION", "UPDATE");

      // Create QUALITY container and subscribe
      handler(registerCnt(AE_ENDPOINT + "/" + RESOURCE_NAME, "QUALITY", 1), "QUALITY CNT", "CREATION");
      handler(subscribeQuality(AE_ENDPOINT + "/" + RESOURCE_NAME + "/QUALITY"), "SUBSCRIPTION", "REQUEST");
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
    String publicIP = client.getString();
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

  if (server.hasArg("plain") == false)
  { //Check if body received

    server.send(200, "text/plain", "Body not received");
    return;
  }

  String message = server.arg("plain");

  StaticJsonDocument<80> filter;
  filter["m2m:sgn"]["nev"]["rep"]["m2m:cin"]["con"] = true;

  StaticJsonDocument<192> doc;

  DeserializationError error = deserializeJson(doc, message, DeserializationOption::Filter(filter));

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  const char *notification = doc["m2m:sgn"]["nev"]["rep"]["m2m:cin"]["con"];
  Serial.println(notification);
  printLCD("New value", String(notification));

  server.sendHeader("X-M2M-RSC", "2000");
  server.send(200, "text/plain", message);
}

void ledIndicator(int level)
{
  digitalWrite(LED_NORMAL, LOW);
  digitalWrite(LED_WARNING, LOW);
  digitalWrite(LED_DANGER, LOW);
  if (level < 500)
  {
    digitalWrite(LED_NORMAL, HIGH);
  }
  else if (level < 1000)
  {
    digitalWrite(LED_WARNING, HIGH);
  }
  else
  {
    digitalWrite(LED_DANGER, HIGH);
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

uint16_t registerCnt(String endpoint, String rn, String mni)
{
  String payload = "{\"m2m:cnt\":{\"mni\":" + mni + ",\"rn\":\"" + rn + "\"}}";
  return postToCse(endpoint, payload, 3);
}

uint16_t postData(String endpoint, String data)
{
  String payload =
      "{\"m2m:cin\":{\"cnf\":\"application/json\",\"con\":\"" + data + "\"}}";
  return postToCse(endpoint, payload, 4);
}

uint16_t subscribeQuality(String endpoint)
{
  String payload =
      String("{\"m2m:sub\":{\"enc\" : {\"net\" : [3]},\"nu\" : [\"http://") + "192.168.0.93" + ":" + String(SERVER_PORT) + "/\"], \"rn\" : \"DataSubscriber\"}, \"nct\": 0}";
  return postToCse(endpoint, payload, 23);
}

uint32_t
postToCse(String endpoint, String payload, uint8_t ty)
{
  HTTPClient client;
  String url = address + endpoint;
  if (!client.begin(url))
  {
    Serial.println("Connection to host failed");
    return 0;
  }
  client.addHeader("X-M2M-Origin", ORIGINATOR);
  client.addHeader("X-M2M-RVI", String(CSE_RELEASE));
  client.addHeader("X-M2M-RI", "123456");
  client.addHeader("Content-Type",
                   "application/vnd.onem2m-res+json; ty=" + String(ty));
  client.addHeader("Content-Length", String(payload.length()));
  client.addHeader("Accept", "application/json");

  // Send POST
  int16_t statusCode = client.POST(payload);
  Serial.println("POST sent to: " + address + endpoint + " with status " + statusCode);
  Serial.print(payload);
  Serial.print("\n\nResponse: ");
  Serial.println(client.getString());
  Serial.println();
  client.end();
  return statusCode;
}
