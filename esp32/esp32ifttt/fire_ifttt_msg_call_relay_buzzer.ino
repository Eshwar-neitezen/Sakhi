#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Wifi_SSID";
const char* password = "Wifi_Password";

WiFiClient client;

int LED = D2;
int RELAY = D4;
int Flame_sensor = D1;
int Flame_detected;

bool fireNotified = false;  // prevent multiple IFTTT triggers
String phoneNumber = "";    // +916360120183

// ---------- Setup WiFi ----------
void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi!");
}

// ---------- Send Notification (IFTTT Notification + Call) ----------
int sendNotification(String number) {
  HTTPClient http;
  int ret = 0;

  // ----------- 1️⃣ IFTTT Notification Applet -----------
  Serial.println("[HTTP] Sending IFTTT Notification...");
  http.begin(client, "http://maker.ifttt.com/trigger/firemsg/with/key/cFD9sxzKEY8bIPt8drJk__?value1=" + phoneNumber);

  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[HTTP] Notification GET code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    ret = -1;
    Serial.printf("[HTTP] GET failed (notification), error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

  // ----------- 2️⃣ IFTTT Call Applet -----------
  Serial.println("[HTTP] Triggering IFTTT Call Applet with phone number...");
  String url = "http://maker.ifttt.com/trigger/Fire/with/key/cFD9sxzKEY8bIPt8drJk__";
  http.begin(client, url);

  httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[HTTP] Call GET code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    ret = -1;
    Serial.printf("[HTTP] GET failed (call), error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();

  return ret;
}

// ---------- Detect Fire ----------
void detectFire() {
  Flame_detected = digitalRead(Flame_sensor);

  if (Flame_detected == LOW) { // Flame detected (LOW signal)
    Serial.println("🔥 Flame detected! Take action immediately!");
    digitalWrite(LED, HIGH);
    digitalWrite(RELAY, LOW);

    if (!fireNotified && phoneNumber != "") {
      sendNotification(phoneNumber);
      fireNotified = true;
    }
  } else {
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, HIGH);
    fireNotified = false; // reset notification flag when fire is gone
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
  pinMode(Flame_sensor, INPUT_PULLUP);

  setupWifi();

  delay(2000);

  // Ask for phone number
  Serial.println("\n📱 Enter the phone number to send alert to:");
  while (phoneNumber == "") {
    if (Serial.available() > 0) {
      phoneNumber = Serial.readStringUntil('\n');
      phoneNumber.trim();
      if (phoneNumber.length() > 0) {
        Serial.print("✅ Phone number set to: ");
        Serial.println(phoneNumber);
      } else {
        Serial.println("⚠️ Invalid entry. Please enter a valid phone number:");
      }
    }
  }

  Serial.println("\n🚨 Fire detection system is now active!");
}

// ---------- Main Loop ----------
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }

  detectFire();
  delay(1000); // check every second
}
