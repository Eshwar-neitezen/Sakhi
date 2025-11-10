#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Wifi_SSID";
const char* password = "Wifi_Password";

WiFiClient client;  // Required for the new HTTPClient API

void setup() {
  Serial.begin(115200);
  setupWifi();
}

void setupWifi() {
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);    

   Serial.print("Attempting to connect to SSID: ");
   Serial.println(ssid);  

   // Wait for connection
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

   // Send HTTP request 5 times
   Serial.println("Sending HTTP request 5 times...");
   for (int i = 0; i < 5; i++) {
       while (get_http() != 0); // retry until success
       delay(500); // small delay between requests
   }

   delay(10000); // wait 10s before sending next batch (adjust as needed)
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
