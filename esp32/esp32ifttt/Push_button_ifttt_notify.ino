#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Wifi_SSID";
const char* password = "Wifi_Password";

WiFiClient client;      // Required for HTTPClient
const int BUTTON_PIN = D1; // GPIO5, change if needed

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button connected to GND
  setupWifi();
}

void setupWifi() {
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);    

   Serial.print("Attempting to connect to SSID: ");
   Serial.println(ssid);  

   while (WiFi.status() != WL_CONNECTED) {
     delay(1000);
     Serial.print(".");
   }
   Serial.println("\nConnected to WiFi");
}

void loop() {
   if (WiFi.status() != WL_CONNECTED) {
     setupWifi();
   }

   // Check if button is pressed (LOW because of INPUT_PULLUP)
   if (digitalRead(BUTTON_PIN) == LOW) {
       Serial.println("Button pressed! Sending HTTP request...");

       // Send HTTP request once
       while (get_http() != 0); // retry until success

       // Wait until button is released to avoid multiple triggers
       while (digitalRead(BUTTON_PIN) == LOW) {
           delay(50);
       }
       Serial.println("Request sent. Waiting for next press...");
   }

   delay(50); // small debounce delay
}

int get_http() {
   HTTPClient http;
   int ret = 0;

   Serial.println("[HTTP] begin...");
   http.begin(client, "http://maker.ifttt.com/trigger/Fire Noti/with/key/cFD9sxzKEY8bIPt8drJk__"); 

   Serial.println("[HTTP] GET...");
   int httpCode = http.GET();

   if(httpCode > 0) {
       Serial.printf("[HTTP] GET code: %d\n", httpCode);
       if(httpCode == HTTP_CODE_OK) {
           String payload = http.getString();
           Serial.println(payload);
       }
   } else {
       ret = -1;
       Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
       delay(500); // wait half sec before retry
   }

   http.end();
   return ret;
}
