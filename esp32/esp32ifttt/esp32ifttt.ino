#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// --- 1. FILL IN YOUR CREDENTIALS HERE ---
#define WIFI_SSID       "Add_wifi_ssid"
#define WIFI_PASS       "add_wifi_password"
#define AIO_USERNAME    "add_adafruit_io_username"
#define AIO_KEY         "add_adafruit_io_key_here"
// -----------------------------------------

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define LED_PIN         2 // The GPIO pin the LED is connected to

// --- Adafruit MQTT Client Setup ---
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// --- Set up the feeds you want to listen to ---
Adafruit_MQTT_Subscribe lightSwitchFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/light-switch");
Adafruit_MQTT_Subscribe blindModeFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/blind");
Adafruit_MQTT_Subscribe alzhiemerModeFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/alzhiemer");
Adafruit_MQTT_Subscribe waiterModeFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/waiter");


void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.print("\nConnecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" Connected!");

    // Subscribe to all four Adafruit IO feeds
    mqtt.subscribe(&lightSwitchFeed);
    mqtt.subscribe(&blindModeFeed);
    mqtt.subscribe(&alzhiemerModeFeed);
    mqtt.subscribe(&waiterModeFeed);
}

void loop() {
    MQTT_connect();

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(5000))) {
        // Check for light-switch messages
        if (subscription == &lightSwitchFeed) {
            String command = (char *)lightSwitchFeed.lastread;
            Serial.print("Received from light-switch: ");
            Serial.println(command);
            if (command == "ON") {
                digitalWrite(LED_PIN, HIGH);
            } else if (command == "OFF") {
                digitalWrite(LED_PIN, LOW);
            }
        }
        
        // Check for blind-mode messages
        if (subscription == &blindModeFeed) {
            String command = (char *)blindModeFeed.lastread;
            Serial.print("Received from blind-mode: ");
            Serial.println(command);
            if (command == "blind mode") {
                Serial.println("ACTION: Blind mode logic would run here.");
            }
        }

        // Check for alzheimer-mode messages
        if (subscription == &alzhiemerModeFeed) {
            String command = (char *)alzhiemerModeFeed.lastread;
            Serial.print("Received from alzheimer-mode: ");
            Serial.println(command);
            if (command == "alzheimer mode") {
                Serial.println("ACTION: Alzheimer mode logic would run here.");
            }
        }
        
        // Check for waiter-mode messages
        if (subscription == &waiterModeFeed) {
            String command = (char *)waiterModeFeed.lastread;
            Serial.print("Received from waiter-mode: ");
            Serial.println(command);

            if (command == "waiter mode activated") {
                Serial.println("ACTION: At your service. Waiter mode logic is now ACTIVE.");
                // A unique blink pattern for activation
                for (int i=0; i<2; i++) {
                  digitalWrite(LED_PIN, HIGH); delay(50);
                  digitalWrite(LED_PIN, LOW); delay(50);
                }
            } 
            // --- NEW BLOCK TO HANDLE DEACTIVATION ---
            else if (command == "waiter mode deactivated") {
                Serial.println("ACTION: Waiter mode is now DEACTIVATED.");
                // You could add a different LED pattern here to confirm deactivation
            }
            // --- END OF NEW BLOCK ---
        }
    }
}

// Function to connect and reconnect to the MQTT broker
void MQTT_connect() {
    int8_t ret;
    if (mqtt.connected()) {
        return;
    }
    Serial.print("Connecting to MQTT... ");
    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0) {
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Retrying MQTT connection in 10 seconds...");
        mqtt.disconnect();
        delay(10000);
        retries--;
        if (retries == 0) {
            while (1);
        }
    }
    Serial.println("MQTT Connected!");
}
