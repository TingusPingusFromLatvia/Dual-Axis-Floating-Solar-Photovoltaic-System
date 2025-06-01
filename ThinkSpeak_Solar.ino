#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// --- WiFi credentials ---
const char* ssid = "TingusPingus";
const char* password = "bigchungus";

// --- ThingSpeak settings ---
const char* server = "http://api.thingspeak.com/update";
const String apiKey = "M0LW9DOB7QSTIZ8R";

// --- Timing for data upload ---
unsigned long lastUpload = 0;
const unsigned long uploadInterval = 15000; // 15 seconds minimum for free tier

// --- Servo setup ---
Servo servohori;
Servo servoverti;
int servoh = 90;
int servov = 90;
int servohTopLimit = 160, servohBottomLimit = 20;
int servovLimitHigh = 160, servovLimitLow = 20;
const int servoPinH = 18;
const int servoPinV = 19;

// --- LDR and Solar Sensor pins ---
const int ldrTopLeft  = 32;
const int ldrTopRight = 33;
const int ldrBotLeft  = 34;
const int ldrBotRight = 35;
const int solarPin    = 32;  // Output from panel

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Setup servos
  servohori.attach(servoPinH);
  servoverti.attach(servoPinV);
  servohori.write(servoh);
  servoverti.write(servov);
  delay(500); // Let the system stabilize
}

void loop() {
  // Read LDRs
  int topl = analogRead(ldrTopLeft);
  int topr = analogRead(ldrTopRight);
  int botl = analogRead(ldrBotLeft);
  int botr = analogRead(ldrBotRight);
  int solout = analogRead(solarPin); // Value to log

  // Calculate averages
  int avgTop = (topl + topr) / 2;
  int avgBot = (botl + botr) / 2;
  int avgLeft = (topl + botl) / 2;
  int avgRight = (topr + botr) / 2;

  // Vertical Movement
  if (avgTop < avgBot) {
    servov = constrain(servov + 1, servovLimitLow, servovLimitHigh);
    servoverti.write(servov);
    delay(10);
  } else if (avgBot < avgTop) {
    servov = constrain(servov - 1, servovLimitLow, servovLimitHigh);
    servoverti.write(servov);
    delay(10);
  }

  // Horizontal Movement
  if (avgLeft > avgRight) {
    servoh = constrain(servoh + 1, servohBottomLimit, servohTopLimit);
    servohori.write(servoh);
    delay(10);
  } else if (avgRight > avgLeft) {
    servoh = constrain(servoh - 1, servohBottomLimit, servohTopLimit);
    servohori.write(servoh);
    delay(10);
  }

  // Upload to ThingSpeak periodically
  if (millis() - lastUpload > uploadInterval) {
    sendToThingSpeak(solout);
    lastUpload = millis();
  }

  delay(100);
}

void sendToThingSpeak(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey + "&field1=" + String(value);
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println("Data sent to ThingSpeak: " + String(value));
    } else {
      Serial.print("Error sending data: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.begin(ssid, password);
  }
}
