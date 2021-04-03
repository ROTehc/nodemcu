#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Pinout
const int CO2_SENSOR = A0;
const int BUTTON = 16;

// Network config
const String WIFI_SSID = "vodafoneAA2FWX";
const String WIFI_PASSWORD = "nLEn4rc7rCexG3tx";
const uint8_t MAX_ATTEMPTS = 20;

// Endpoint
const String HOST = "192.168.0.222";
const int HTTP_PORT = 8080;

// AE
const String AE_ORIGINATOR = "Cae_deviceTesting";
const String RESOURCE_NAME = "sensorGAS1";

// Vars
int reading;
uint8_t buttonPrev;

void setup()
{
	lcd.begin(16, 2);
	Serial.begin(115200);
	connectWiFi();
	registerAE();
	pinMode(BUTTON, INPUT_PULLUP);
	buttonPrev = digitalRead(BUTTON);
}

void loop()
{
	if (digitalRead(BUTTON) == HIGH and buttonPrev == LOW)
	{
		sendReading();
	}
	buttonPrev = digitalRead(BUTTON);
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

void connectWiFi()
{
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	int i = 0;
	Serial.print("Connecting to: ");
	Serial.println(WIFI_SSID);

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
		printLCD("CONNECTED TO", WIFI_SSID);
	}
	else
	{
		Serial.println("\nConnection failed.");
		printLCD("ERROR");
	}
}

String sendCSE(String endpoint, String payload, int ty)
{
	WiFiClient client;

	if (!client.connect(HOST, HTTP_PORT))
	{
		Serial.println("connection failed");
	}

	// Request body
	requestMessage = "POST " + endpoint + " HTTP/1.1\r\n" +
					 "Host: " + HOST + " \r\n" +
					 "X-M2M-Origin: " + AE_ORIGINATOR + "\r\n" +
					 "X-M2M-RVI: 3\r\n" +
					 "X-M2M-RI: 123456\r\n" +
					 "Content-Length: " + String(payload.length()) + "\r\n" +
					 "Content-Type: application/json;ty=" + ty + "\r\n" +
					 "Connection: close\r\n\r\n" +
					 payload;

	// Send POST
	client.print(requestMessage);
	Serial.println("POST " + payload + " sent to: " + HOST + endpoint + ":" + HTTP_PORT);

	unsigned long timeout = millis();
	while (client.available() == 0)
	{
		if (millis() - timeout > 5000)
		{
			Serial.println(">>> Client Timeout !");
			client.stop();
			delay(60000);
			return "error";
		}
	}

	// Read the HTTP response
	String res = "";
	if (client.available())
	{
		res = client.readStringUntil('\r');
		Serial.print(res);
	}

	return res;
}

void registerAE()
{
	String payload;
	payload = String("{\"m2m:ae\":{") +
			  "\"rn\":\"" + String(RESOURCE_NAME) + "\",\r\n" +
			  "\"api\":\"N." + String(RESOURCE_NAME) + ".ROTehc.com\",\r\n" +
			  "\"srv\": [\"3\"],\r\n" +
			  "\"rr\":false}}";

	String res = sendCSE("/cse-in", payload, 2);

	if (res == "HTTP/1.1 201 CREATED")
	{
		printLCD("AE REGISTERED", "SUCCESSFULLY");
		delay(1000);
		payload = String("{\"m2m:cnt\":{\"rn\":\"gas_ppm\"}}");
		String endpoint = String("/cse-in/") + RESOURCE_NAME;
		res = sendCSE(endpoint, payload, 3);
		if (res == "HTTP/1.1 201 CREATED")
		{
			printLCD("CNT CREATED", "SUCCESSFULLY");
			delay(1000);
		}
	}
}

void sendReading()
{
	reading = analogRead(CO2_SENSOR);
	String payload = "{\"m2m:cin\": {\"con\": \"" + String(reading) + "\"}}";
	sendCSE("/cse-in/" + RESOURCE_NAME + "/gas_ppm", payload, 4);
	printLCD("DATA SENT TO CSE", "READING: " + String(reading));
}
