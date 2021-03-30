#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Pinout
const int gasSensor = A0;
const int button = 16;

// Network config
const String WIFI_SSID = "vodafoneAA2FWX";
const String WIFI_PASSWORD = "nLEn4rc7rCexG3tx";
const uint8_t MAX_ATTEMPTS = 20;

// Endpoint
const String HOST = "ptsv2.com";
const String ENDPOINT = "/t/3ixsq-1616803416/post";

// Vars
int reading;
uint8_t buttonPrev;
String dataJSON;
String requestMessage;

void setup()
{
  Serial.begin(115200);
  connectWiFi();
  pinMode(button, INPUT_PULLUP);
  buttonPrev = digitalRead(button);
}

void loop()
{
  if (digitalRead(button) == HIGH and buttonPrev == LOW)
  {
    sendReading();
  }
  buttonPrev = digitalRead(button);
}

void connectWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  byte i = 0;
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED and i < MAX_ATTEMPTS)
  {
    Serial.print(".");
    delay(500);
  }
  if (i < MAX_ATTEMPTS)
  {
    Serial.println("\nConnection established!");
  }
  else
  {
    Serial.println("\nConnection failed.");
  }
}

void sendReading()
{
  WiFiClientSecure client;
  client.setInsecure();
  const int httpPort = 443;

  if (!client.connect(HOST, httpPort))
  {
    Serial.println("connection failed");
  }

  reading = analogRead(gasSensor);
  dataJSON = "{\"gas\": " + String(reading) + "}";

  // Request body
  requestMessage = "POST " + ENDPOINT + " HTTP/1.1\r\n" +
                   "Host: " + HOST + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + String(dataJSON.length()) + "\r\n\r\n" +
                   dataJSON;

  // Send POST
  client.print(requestMessage);
  Serial.println("POST " + dataJSON + " sent to: " + HOST + ENDPOINT);
}
