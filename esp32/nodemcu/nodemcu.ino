#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// --- WiFi Credentials ---
const char* ssid = "Sakhi";
const char* password = "Sakhi@123";

ESP8266WebServer server(80);

// --- Global variables to track feature states ---
bool shakeHandActive = false;
bool burglerAlarmActive = false;
bool blindAssistActive = false;
bool safetyLockerActive = false;
bool chatbotActive = false;
bool restaurantWaiterActive = false;
bool joystickControlActive = false;
bool faceRecognitionActive = false;
bool tabletWaterActive = false;

// Background monitoring features (always active on face screen)
bool fireDetectionActive = true;
bool lightDetectionActive = true;

String phoneNumber = "";

// --- Define Sensor Pins ---
#define LED_PIN D1
#define RELAY_PIN D2
#define FLAME_SENSOR_PIN D0
#define MOTION_SENSOR_PIN D3
#define WATER_RELAY_PIN D5
#define TRIG_PIN D6
#define ECHO_PIN D7
#define SERVO_PIN D4
#define LDR_PIN A0
#define LDR_RELAY_PIN D8

// --- IFTTT Credentials ---
const char* iftttKey = "cFD9sxzKEY8bIPt8drJk__";

WiFiClient client;

// --- Fire Detector Mode Variables ---
bool fireNotified = false;
String fireMessage = "FIRE ALERT!!! Fire has been detected. Minor measures have been taken. Water Sprinkled. Please check right away.";
String fireCallMessage = "Fire Alert. Fire has been detected. Minor measures have been taken. Water Sprinkled. Please check right away.";

// --- Burglar Alert Mode Variables ---
const int motionDebounceCount = 2;
int motionCounter = 0;
unsigned long lastBurglarNotificationTime = 0;
const unsigned long notificationCooldown = 10000;
unsigned long previousBurglarMillis = 0;
const unsigned long burglarLoopInterval = 200;
String burglarMessage = "BURGLER ALERT!!! There might be an intruder. Please check the door right away to prevent robbery of your belongings.";
String burglarCallMessage = "Hi, Burgler Alert!!! There might be an intruder. Please check the door right away to prevent robbery of your belongings.";

// --- Tablet Dispenser Mode Variables ---
Servo tabletServo;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);
int tabletTargetHour, tabletTargetMinute;
unsigned long waterReminderIntervalMillis;
unsigned long lastWaterReminderTime = 0;
bool tabletTimeReachedForToday = false;

// --- LDR Mode Variables ---
int lightThreshold = 500;

// --- Function to Send Notification (Restored with full debugging) ---
int sendNotification(String alertMessage, String callMessage) {
  if (phoneNumber == "") {
    Serial.println("📞 Phone number not set, cannot send notifications.");
    return -1;
  }
  HTTPClient http;
  int ret = 0;

  // IFTTT Messaging Trigger
  String url1 = "http://maker.ifttt.com/trigger/firemsg/with/key/" + String(iftttKey) + "?value1=" + phoneNumber + "&value2=" + alertMessage;
  http.begin(client, url1);
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

  // IFTTT Calling Trigger
  String url2 = "http://maker.ifttt.com/trigger/Fire/with/key/" + String(iftttKey) + "?value1=" + callMessage;
  http.begin(client, url2);
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

// --- Helper for Ultrasonic Sensor ---
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.0343 / 2;
}

// --- Dispenser Logic ---
void dispense(String type) {
  if (type == "Tablet") {
    tabletServo.write(90);
    delay(2000);
    tabletServo.write(0);
  } else if (type == "Water") {
    digitalWrite(WATER_RELAY_PIN, LOW);
    delay(3000);
    digitalWrite(WATER_RELAY_PIN, HIGH);
  }
  Serial.println("✅ " + type + " dispensed.");
}

void manualDispense(String type) {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON - waiting for person to dispense " + type + "...");
  unsigned long entryTime = millis();
  const unsigned long dispenseTimeout = 30000;
  while (millis() - entryTime < dispenseTimeout) {
    float distance = getDistance();
    if (distance > 0 && distance < 30) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Person detected - dispensing " + type);
      dispense(type);
      return;
    }
    delay(100);
  }
  digitalWrite(LED_PIN, LOW);
  Serial.println("No person detected for " + type + " dispensing. Timeout.");
}

// --- Feature Methods ---
void shakeHandFeature() {
  // Continuous loop code for shake hand goes here
}

void burglerAlarmFeature() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousBurglarMillis >= burglarLoopInterval) {
    previousBurglarMillis = currentMillis;

    int motionDetected = digitalRead(MOTION_SENSOR_PIN);
    Serial.println("DETECTING...");

    if (motionDetected == HIGH) {
      motionCounter++;
    } else {
      motionCounter = 0;
    }

    if (motionCounter >= motionDebounceCount) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("🚨 Motion detected!");

      if (millis() - lastBurglarNotificationTime > notificationCooldown && phoneNumber != "") {
        sendNotification(burglarMessage, burglarCallMessage);
        lastBurglarNotificationTime = millis();
      }
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void blindAssistFeature() {
  // Continuous loop code for blind assist goes here
}

void safetyLockerFeature() {
  // Continuous loop code for safety locker goes here
}

void chatbotFeature() {
  // Continuous loop code for chatbot goes here
}

void restaurantWaiterFeature() {
  // Continuous loop code for restaurant waiter goes here
}

void joystickControlFeature() {
  // Continuous loop code for joystick control goes here
}

void faceRecognitionFeature() {
  // Continuous loop code for face recognition goes here
}

void tabletWaterFeature() {
  if (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  if (currentHour == tabletTargetHour && currentMinute == tabletTargetMinute && !tabletTimeReachedForToday) {
    Serial.println("💊 It's tablet time!");
    manualDispense("Tablet");
    tabletTimeReachedForToday = true;
  } else if (currentHour != tabletTargetHour || currentMinute != tabletTargetMinute) {
     if (tabletTimeReachedForToday && (currentHour > tabletTargetHour || (currentHour == tabletTargetHour && currentMinute > tabletTargetMinute))) {
        tabletTimeReachedForToday = false;
    }
  }

  if (millis() - lastWaterReminderTime >= waterReminderIntervalMillis) {
    Serial.println("💧 Time for water!");
    manualDispense("Water");
    lastWaterReminderTime = millis();
  }
}

void fireDetectionFeature() {
  static unsigned long lastFireCheck = 0;
  if (millis() - lastFireCheck > 1000) {
    int flameDetected = digitalRead(FLAME_SENSOR_PIN);
    if (flameDetected == LOW) {
      Serial.println("🔥 Flame detected! Take action immediately!");
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(RELAY_PIN, LOW);
      if (!fireNotified && phoneNumber != "") {
        sendNotification(fireMessage, fireCallMessage);
        fireNotified = true;
      }
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(RELAY_PIN, HIGH);
      fireNotified = false;
    }
    lastFireCheck = millis();
  }
}

void lightDetectionFeature() {
  static unsigned long lastLightCheck = 0;
  if (millis() - lastLightCheck > 1000) {
    int ldrValue = analogRead(LDR_PIN);
    if (ldrValue > lightThreshold) {
      Serial.println("Low Light!!! Turning ON lights");
      digitalWrite(LDR_RELAY_PIN, LOW);
    } else {
      digitalWrite(LDR_RELAY_PIN, HIGH);
    }
    lastLightCheck = millis();
  }
}

void checkSerialCommands() {
  // Check for data on the main hardware serial port (from the ESP32)
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    // Blink the onboard LED for visual confirmation that a command was received
    pinMode(D4, OUTPUT);
    digitalWrite(D4, LOW); delay(100); digitalWrite(D4, HIGH);

    // Check for the specific command for Alzheimer mode
    if (command == "start:alzheimer") {
      // This line calls the same function as your UI button
      showModal("alzheimer");
    }
    // You can add more 'else if' statements for other commands later
  }
}

const char mainPage[] PROGMEM = R"MAIN(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sakhi - Smart Assistance</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Poppins', 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #0f2027 0%, #203a43 50%, #2c5364 100%);
            min-height: 100vh;
            color: #fff;
            overflow-x: hidden;
        }
        .face-screen {
            display: flex;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            width: 100vw;
            position: fixed;
            top: 0;
            left: 0;
            background: linear-gradient(135deg, #E3F2FD 0%, #BBDEFB 50%, #90CAF9 100%);
            animation: fadeIn 0.6s ease;
            cursor: pointer;
            overflow: hidden;
            padding: 0;
            margin: 0;
        }
        .robot-face {
            width: 100vw;
            height: 100vh;
            position: relative;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.3s ease;
        }
        .robot-face:hover .face-content {
            transform: scale(1.01);
        }
        @keyframes float {
            0%, 100% { transform: translateY(0px); }
            50% { transform: translateY(-15px); }
        }
        .face-content {
            position: relative;
            width: 100%;
            height: 100%;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            transition: all 0.3s ease;
        }
        .eyes-container {
            display: flex;
            gap: 180px;
            margin-bottom: 120px;
            animation: float 3s ease-in-out infinite;
        }
        .eye {
            width: 200px;
            height: 200px;
            background: white;
            border-radius: 50%;
            position: relative;
            box-shadow: 0 15px 40px rgba(33, 150, 243, 0.3);
            display: flex;
            align-items: center;
            justify-content: center;
            border: 8px solid #2196F3;
        }
        .pupil {
            width: 80px;
            height: 80px;
            background: linear-gradient(135deg, #1976D2, #2196F3);
            border-radius: 50%;
            position: relative;
            animation: blink 4s ease-in-out infinite;
            box-shadow: 0 6px 15px rgba(25, 118, 210, 0.4);
        }
        .pupil::after {
            content: '';
            width: 35px;
            height: 35px;
            background: white;
            border-radius: 50%;
            position: absolute;
            top: 15px;
            left: 15px;
            animation: sparkle 2s ease-in-out infinite;
        }
        @keyframes sparkle {
            0%, 100% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.7; transform: scale(0.9); }
        }
        @keyframes blink {
            0%, 90%, 100% { 
                transform: scaleY(1);
                opacity: 1;
            }
            93%, 97% { 
                transform: scaleY(0.1);
                opacity: 0.5;
            }
        }
        .mouth {
            width: 200px;
            height: 100px;
            border: 10px solid #2196F3;
            border-top: none;
            border-radius: 0 0 200px 200px;
            position: relative;
            background: linear-gradient(to bottom, rgba(144, 202, 249, 0.3) 0%, rgba(100, 181, 246, 0.5) 100%);
            box-shadow: inset 0 -10px 20px rgba(33, 150, 243, 0.2);
        }
        .cheeks {
            position: absolute;
            width: 80px;
            height: 80px;
            background: radial-gradient(circle, rgba(100, 181, 246, 0.8), transparent);
            border-radius: 50%;
            top: 50%;
            transform: translateY(-50%);
            animation: blush 3s ease-in-out infinite;
        }
        .cheek-left {
            left: 15%;
        }
        .cheek-right {
            right: 15%;
        }
        @keyframes blush {
            0%, 100% { opacity: 0.6; }
            50% { opacity: 0.9; }
        }
        @keyframes pulse {
            0%, 100% { opacity: 0.9; }
            50% { opacity: 1; }
        }
        @keyframes fadeIn {
            from { opacity: 0; transform: scale(0.9); }
            to { opacity: 1; transform: scale(1); }
        }
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
            display: flex; 
            flex-direction: column; 
            align-items: center;
            width: 100%;
            padding: 30px 20px;
        }
        .header { 
            text-align: center; 
            margin-bottom: 80px; 
            animation: fadeInDown 0.8s ease;
            width: 100%;
        }
        .logo { 
            font-size: 4em; 
            margin-bottom: 15px; 
            display: inline-block; 
            animation: float 3s ease-in-out infinite; 
        }
        @keyframes fadeInDown { 
            from { opacity: 0; transform: translateY(-30px); } 
            to { opacity: 1; transform: translateY(0); } 
        }
        @keyframes fadeInUp { 
            from { opacity: 0; transform: translateY(30px); } 
            to { opacity: 1; transform: translateY(0); } 
        }
        h1 {
            font-size: 4.5em;
            font-weight: 900;
            background: linear-gradient(135deg, #00d4ff, #0099ff, #00d4ff);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            margin-bottom: 20px;
            letter-spacing: -2px;
            text-shadow: 0 10px 30px rgba(0, 212, 255, 0.2);
        }
        .tagline { 
            font-size: 1.3em; 
            color: rgba(255, 255, 255, 0.9);
            font-weight: 500;
            background: linear-gradient(135deg, #ffb347, #ff6b6b);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            letter-spacing: 1px;
        }
        .button-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); 
            gap: 25px; 
            margin-bottom: 60px; 
            animation: fadeInUp 0.8s ease 0.2s both;
            width: 100%;
        }
        .btn {
            padding: 40px 25px;
            border: none;
            border-radius: 25px;
            font-size: 16px;
            font-weight: 700;
            cursor: pointer;
            transition: all 0.4s cubic-bezier(0.23, 1, 0.320, 1);
            position: relative;
            overflow: hidden;
            backdrop-filter: blur(15px);
            border: 2px solid rgba(255, 255, 255, 0.3);
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            gap: 15px;
            color: white;
            box-shadow: 0 8px 32px rgba(0, 212, 255, 0.1);
            min-height: 200px;
        }
        .btn .emoji { 
            font-size: 3em; 
            filter: drop-shadow(0 4px 8px rgba(0, 0, 0, 0.2)); 
        }
        .btn .text { 
            font-size: 1em; 
            letter-spacing: 0.5px; 
        }
        .btn:nth-child(1) { background: linear-gradient(135deg, rgba(255, 107, 107, 0.3), rgba(255, 107, 107, 0.1)); border-color: rgba(255, 107, 107, 0.4); }
        .btn:nth-child(2) { background: linear-gradient(135deg, rgba(106, 17, 203, 0.3), rgba(106, 17, 203, 0.1)); border-color: rgba(106, 17, 203, 0.4); }
        .btn:nth-child(3) { background: linear-gradient(135deg, rgba(255, 154, 0, 0.3), rgba(255, 154, 0, 0.1)); border-color: rgba(255, 154, 0, 0.4); }
        .btn:nth-child(4) { background: linear-gradient(135deg, rgba(0, 200, 255, 0.3), rgba(0, 200, 255, 0.1)); border-color: rgba(0, 200, 255, 0.4); }
        .btn:nth-child(5) { background: linear-gradient(135deg, rgba(76, 175, 80, 0.3), rgba(76, 175, 80, 0.1)); border-color: rgba(76, 175, 80, 0.4); }
        .btn:nth-child(6) { background: linear-gradient(135deg, rgba(255, 193, 7, 0.3), rgba(255, 193, 7, 0.1)); border-color: rgba(255, 193, 7, 0.4); }
        .btn:nth-child(7) { background: linear-gradient(135deg, rgba(63, 81, 181, 0.3), rgba(63, 81, 181, 0.1)); border-color: rgba(63, 81, 181, 0.4); }
        .btn:nth-child(8) { background: linear-gradient(135deg, rgba(233, 30, 99, 0.3), rgba(233, 30, 99, 0.1)); border-color: rgba(233, 30, 99, 0.4); }
        .btn::after { 
            content: ''; 
            position: absolute; 
            top: 50%; 
            left: 50%; 
            width: 0; 
            height: 0; 
            border-radius: 50%; 
            background: rgba(255, 255, 255, 0.1); 
            transform: translate(-50%, -50%); 
            transition: width 0.6s, height 0.6s; 
        }
        .btn:hover::after { 
            width: 300px; 
            height: 300px; 
        }
        .btn:hover { 
            transform: translateY(-15px) scale(1.05); 
            box-shadow: 0 30px 60px rgba(0, 212, 255, 0.3), 0 0 30px rgba(0, 212, 255, 0.2), inset 0 0 20px rgba(255, 255, 255, 0.1); 
            border-color: rgba(255, 255, 255, 0.8); 
        }
        .dashboard-screen {
            display: none;
            width: 100%;
        }
        .phone-section {
            text-align: center;
            margin-bottom: 50px;
            animation: fadeInUp 0.8s ease 0.3s both;
            width: 100%;
            max-width: 600px;
        }
        .phone-container {
            background: linear-gradient(135deg, rgba(33, 150, 243, 0.2), rgba(33, 150, 243, 0.05));
            backdrop-filter: blur(15px);
            padding: 40px 35px;
            border-radius: 25px;
            border: 2px solid rgba(33, 150, 243, 0.3);
            box-shadow: 0 15px 40px rgba(0, 212, 255, 0.2);
        }
        .phone-container h2 {
            font-size: 1.8em;
            margin-bottom: 10px;
            background: linear-gradient(135deg, #00d4ff, #2196F3);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        .phone-container p {
            color: rgba(255, 255, 255, 0.8);
            margin-bottom: 25px;
            font-size: 0.95em;
        }
        .phone-input-group {
            display: flex;
            gap: 15px;
            align-items: stretch;
        }
        .phone-input {
            flex: 1;
            padding: 16px 20px;
            border: 2px solid rgba(33, 150, 243, 0.4);
            border-radius: 15px;
            font-size: 1.1em;
            background: rgba(255, 255, 255, 0.1);
            color: white;
            transition: all 0.3s ease;
            font-family: 'Poppins', sans-serif;
        }
        .phone-input:focus {
            outline: none;
            border-color: #2196F3;
            background: rgba(255, 255, 255, 0.15);
            box-shadow: 0 0 20px rgba(33, 150, 243, 0.3);
        }
        .phone-input::placeholder {
            color: rgba(255, 255, 255, 0.5);
        }
        .phone-submit-btn {
            background: linear-gradient(135deg, #2196F3, #1976D2);
            color: white;
            padding: 16px 35px;
            border: none;
            border-radius: 15px;
            font-size: 1.1em;
            font-weight: 700;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 8px 25px rgba(33, 150, 243, 0.4);
            white-space: nowrap;
        }
        .phone-submit-btn:hover {
            transform: translateY(-3px);
            box-shadow: 0 12px 35px rgba(33, 150, 243, 0.6);
            background: linear-gradient(135deg, #1976D2, #1565C0);
        }
        .video-section { 
            text-align: center; 
            margin-top: 60px; 
            animation: fadeInUp 0.8s ease 0.4s both;
            width: 100%;
        }
        .video-btn { 
            background: linear-gradient(135deg, #00d4ff 0%, #0099ff 100%); 
            color: white; 
            padding: 18px 50px; 
            border: none; 
            border-radius: 50px; 
            font-size: 1.1em; 
            font-weight: 700; 
            cursor: pointer; 
            transition: all 0.4s ease; 
            box-shadow: 0 15px 40px rgba(0, 212, 255, 0.4); 
            display: inline-block; 
            letter-spacing: 1px; 
            margin: 10px;
        }
        .video-btn:hover { 
            transform: translateY(-8px) scale(1.08); 
            box-shadow: 0 25px 60px rgba(0, 212, 255, 0.6); 
        }
        .exit-section {
            margin-top: 40px;
            text-align: center;
            animation: fadeInUp 0.8s ease 0.6s both;
        }
        .exit-btn {
            background: linear-gradient(135deg, #ff6b6b 0%, #ff4444 100%);
            color: white;
            padding: 16px 50px;
            border: none;
            border-radius: 50px;
            font-size: 1.1em;
            font-weight: 700;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 10px 30px rgba(255, 107, 107, 0.3);
            letter-spacing: 1px;
        }
        .exit-btn:hover {
            transform: translateY(-5px) scale(1.05);
            box-shadow: 0 15px 40px rgba(255, 107, 107, 0.5);
        }
        .modal { 
            display: none; 
            position: fixed; 
            z-index: 1000; 
            left: 0; 
            top: 0; 
            width: 100%; 
            height: 100%; 
            background: rgba(0, 0, 0, 0.7); 
            backdrop-filter: blur(5px); 
            animation: fadeIn 0.3s ease; 
        }
        .modal-content {
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            margin: 5% auto;
            padding: 50px 40px;
            border-radius: 25px;
            width: 90%;
            max-width: 600px;
            color: white;
            box-shadow: 0 25px 60px rgba(0, 0, 0, 0.5);
            animation: slideUp 0.4s cubic-bezier(0.34, 1.56, 0.64, 1);
            border: 1px solid rgba(0, 212, 255, 0.2);
        }
        @keyframes slideUp { 
            from { transform: translateY(100px); opacity: 0; } 
            to { transform: translateY(0); opacity: 1; } 
        }
        .modal-header { 
            font-size: 2.2em; 
            margin-bottom: 25px; 
            border-bottom: 2px solid #00d4ff; 
            padding-bottom: 20px; 
            display: flex; 
            align-items: center; 
            gap: 15px; 
        }
        .modal-header-emoji { font-size: 2em; }
        .modal-body { 
            font-size: 1.05em; 
            line-height: 1.8; 
            margin-bottom: 30px; 
            color: rgba(255, 255, 255, 0.85); 
        }
        .visual { 
            text-align: center; 
            font-size: 5em; 
            margin: 40px 0; 
            animation: pulse 2s ease-in-out infinite; 
        }
        .close-btn { 
            background: linear-gradient(135deg, #00d4ff, #0099ff); 
            color: white; 
            padding: 14px 35px; 
            border: none; 
            border-radius: 25px; 
            cursor: pointer; 
            font-size: 1em; 
            font-weight: 600; 
            transition: all 0.3s ease; 
            margin: 8px; 
            box-shadow: 0 5px 15px rgba(0, 212, 255, 0.2); 
        }
        .close-btn:hover { 
            background: linear-gradient(135deg, #0099ff, #0066cc); 
            transform: translateY(-3px); 
            box-shadow: 0 10px 25px rgba(0, 212, 255, 0.4); 
        }
        .stop-btn {
            background: linear-gradient(135deg, #ff6b6b, #ff4444);
        }
        .stop-btn:hover {
            background: linear-gradient(135deg, #ff4444, #cc0000);
        }
        .sub-buttons { 
            display: grid; 
            grid-template-columns: 1fr 1fr; 
            gap: 15px; 
            margin-top: 30px; 
        }
        .sub-buttons .close-btn { 
            width: 100%; 
            margin: 0; 
        }
        .full-width-btn { 
            width: 100% !important; 
            margin-top: 20px; 
        }
    </style>
</head>
<body>
    <div class="face-screen" id="faceScreen" onclick="openDashboard()">
        <div class="robot-face">
            <div class="face-content">
                <div class="cheeks cheek-left"></div>
                <div class="cheeks cheek-right"></div>
                <div class="eyes-container">
                    <div class="eye">
                        <div class="pupil"></div>
                    </div>
                    <div class="eye">
                        <div class="pupil"></div>
                    </div>
                </div>
                <div class="mouth"></div>
            </div>
        </div>
    </div>

    <div class="dashboard-screen" id="dashboardScreen">
        <div class="container">
            <div class="header">
                <div class="logo">🤖</div>
                <h1>Sakhi</h1>
                <p class="tagline">Sakhi Layegi Sukhi 💕</p>
            </div>
            
            <div class="phone-section">
                <div class="phone-container">
                    <h2>📱 Contact Information</h2>
                    <p>Enter your mobile number for emergency notifications</p>
                    <div class="phone-input-group">
                        <input type="tel" id="phoneInput" class="phone-input" placeholder="Enter 10-digit mobile number" maxlength="10" pattern="[0-9]{10}">
                        <button class="phone-submit-btn" onclick="submitPhone()">Submit</button>
                    </div>
                </div>
            </div>

            <div class="button-grid">
                <button class="btn" onclick="showModal('shakehand')"><span class="emoji">👋</span><span class="text">Shake Hand</span></button>
                <button class="btn" onclick="showModal('alzheimer')"><span class="emoji">🧠</span><span class="text">Alzheimer's</span></button>
                <button class="btn" onclick="showModal('burgler')"><span class="emoji">🚨</span><span class="text">Burgler Alarm</span></button>
                <button class="btn" onclick="showModal('blind')"><span class="emoji">👁️</span><span class="text">Blind Assist</span></button>
                <button class="btn" onclick="showModal('locker')"><span class="emoji">🔐</span><span class="text">Safety Locker</span></button>
                <button class="btn" onclick="showModal('chatbot')"><span class="emoji">💬</span><span class="text">Chatbot</span></button>
                <button class="btn" onclick="showModal('server')"><span class="emoji">🍽️</span><span class="text">Restaurant Waiter</span></button>
                <button class="btn" onclick="showModal('joystick')"><span class="emoji">🕹️</span><span class="text">Joystick Control</span></button>
            </div>
            <div class="video-section">
                <button class="video-btn" onclick="window.open('https://youtu.be/9qRvcFxtc6I?si=_jwGv_cEP9RtRQdo', '_blank')">🎥 Watch Full Video</button>
            </div>
            <div class="exit-section">
                <button class="exit-btn" onclick="exitDashboard()">← EXIT</button>
            </div>
        </div>
    </div>

    <div id="shakehandModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">👋</span><span>Shake Hand</span></div>
            <div class="visual">🤝</div>
            <div class="modal-body">Gesture-based control through hand shake detection. Perfect for contactless interaction and accessibility.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('shakehand')">Stop & Close</button>
        </div>
    </div>

    <div id="alzheimerModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">🧠</span><span>Alzheimer's Assistance</span></div>
            <div class="modal-body">Comprehensive care system with advanced features for daily health and safety.</div>
            <div class="sub-buttons">
                <button class="close-btn" onclick="showSubModal('facerecognition')">👤 Face Recognition</button>
                <button class="close-btn" onclick="showSubModal('tabletwater')">💊 Tablet/Water</button>
            </div>
            <button class="close-btn full-width-btn" onclick="closeModal('alzheimerModal')">Close</button>
        </div>
    </div>

    <div id="facerecognitionSubModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">👤</span><span>Face Recognition</span></div>
            <div class="visual">🔍👤</div>
            <div class="modal-body">Advanced facial recognition. Identifies family members and caregivers for personalized greetings and security.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('facerecognition')">Stop & Close</button>
        </div>
    </div>

    <div id="tabletwaterSubModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">💊</span><span>Tablet/Water Reminder</span></div>
            <div class="modal-body">
                <div style="margin-bottom: 25px;">
                    <label style="display: block; margin-bottom: 10px; font-size: 1.1em; color: #00d4ff;">
                        💊 Tablet Time (24-hour format):
                    </label>
                    <input type="time" id="tabletTimeInput" style="width: 100%; padding: 12px; border-radius: 10px; border: 2px solid #00d4ff; background: rgba(255, 255, 255, 0.1); color: white; font-size: 1.1em;">
                </div>
                <div style="margin-bottom: 25px;">
                    <label style="display: block; margin-bottom: 10px; font-size: 1.1em; color: #00d4ff;">
                        💧 Water Interval (minutes):
                    </label>
                    <input type="number" id="waterIntervalInput" placeholder="e.g., 30" min="1" style="width: 100%; padding: 12px; border-radius: 10px; border: 2px solid #00d4ff; background: rgba(255, 255, 255, 0.1); color: white; font-size: 1.1em;">
                </div>
                <button class="close-btn" style="width: 100%; margin-top: 10px; padding: 16px;" onclick="activateTabletWater()">✓ Activate</button>
                <button class="close-btn stop-btn" style="width: 100%;" onclick="stopFeature('tabletwater')">✕ Stop & Close</button>
            </div>
        </div>
    </div>

    <div id="burglerModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">🚨</span><span>Burgler Alarm</span></div>
            <div class="visual">🔔</div>
            <div class="modal-body">Motion and intrusion detection. Monitors your space 24/7 and triggers alerts on unauthorized activity.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('burgler')">Stop & Close</button>
        </div>
    </div>

    <div id="blindModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">👁️</span><span>Blind Assist</span></div>
            <div class="visual">🦯</div>
            <div class="modal-body">AI-powered navigation and obstacle detection. Real-time audio feedback for safe navigation.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('blind')">Stop & Close</button>
        </div>
    </div>

    <div id="lockerModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">🔐</span><span>Safety Locker</span></div>
            <div class="visual">🔒</div>
            <div class="modal-body">Secure storage with smart locking. Biometric and PIN access control for valuables.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('locker')">Stop & Close</button>
        </div>
    </div>

    <div id="chatbotModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">💬</span><span>Chatbot</span></div>
            <div class="visual">🤖</div>
            <div class="modal-body">Conversational AI assistant. Ask questions and control devices through natural language.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('chatbot')">Stop & Close</button>
        </div>
    </div>

    <div id="serverModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">🍽️</span><span>Restaurant Waiter</span></div>
            <div class="visual">👨‍🍳</div>
            <div class="modal-body">Intelligent restaurant service robot. Takes orders, serves food, and provides exceptional dining experience with voice interaction and autonomous navigation.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('server')">Stop & Close</button>
        </div>
    </div>

    <div id="joystickModal" class="modal">
        <div class="modal-content">
            <div class="modal-header"><span class="modal-header-emoji">🕹️</span><span>Joystick Control</span></div>
            <div class="visual">🎮</div>
            <div class="modal-body">Real-time device control with analog joystick. Smooth, responsive control for robotics.</div>
            <button class="close-btn stop-btn" onclick="stopFeature('joystick')">Stop & Close</button>
        </div>
    </div>

    <script>
        let tabletWaterInterval = null;
        
        function openDashboard() {
            document.getElementById('faceScreen').style.display = 'none';
            document.getElementById('dashboardScreen').style.display = 'block';
            fetch('/dashboard/open');
        }
        
        function exitDashboard() {
            document.getElementById('dashboardScreen').style.display = 'none';
            document.getElementById('faceScreen').style.display = 'flex';
            fetch('/dashboard/close');
        }
        
        function submitPhone() {
            const phoneInput = document.getElementById('phoneInput');
            const phone = phoneInput.value.trim();
            
            const phoneRegex = /^[0-9]{10}$/;
            if (!phoneRegex.test(phone)) {
                alert('Please enter a valid 10-digit mobile number!');
                return;
            }
            
            fetch('/submitphone?number=' + encodeURIComponent(phone))
                .then(response => response.text())
                .then(data => {
                    alert('Mobile number saved successfully: ' + phone);
                    phoneInput.value = '';
                })
                .catch(error => {
                    alert('Error saving mobile number');
                });
        }
        
        function showModal(feature) {
            const modalId = feature + 'Modal';
            document.getElementById(modalId).style.display = 'block';
            fetch('/start/' + feature);
        }
        
        function showSubModal(feature) {
            const subModalId = feature + 'SubModal';
            document.getElementById(subModalId).style.display = 'block';
            if (feature !== 'tabletwater') {
                fetch('/start/' + feature);
            }
        }
        
        function closeModal(modalId) {
            document.getElementById(modalId).style.display = 'none';
        }
        
        function stopFeature(feature) {
            fetch('/stop/' + feature)
                .then(response => response.text())
                .then(data => {
                    const modalId = feature + 'Modal';
                    const subModalId = feature + 'SubModal';
                    
                    if (document.getElementById(modalId)) {
                        document.getElementById(modalId).style.display = 'none';
                    }
                    if (document.getElementById(subModalId)) {
                        document.getElementById(subModalId).style.display = 'none';
                    }
                    
                    if (feature === 'tabletwater' && tabletWaterInterval) {
                        clearInterval(tabletWaterInterval);
                        tabletWaterInterval = null;
                    }
                });
        }
        
        function activateTabletWater() {
            const tabletTime = document.getElementById('tabletTimeInput').value;
            const waterInterval = document.getElementById('waterIntervalInput').value;
            
            if (!tabletTime || !waterInterval) {
                alert('Please enter both tablet time and water interval!');
                return;
            }
            
            fetch('/start/tabletwater?tablet_time=' + encodeURIComponent(tabletTime) + '&water_interval=' + encodeURIComponent(waterInterval))
                .then(response => response.text())
                .then(data => {
                    alert('Activated! Tablet: ' + tabletTime + ', Water: every ' + waterInterval + ' min');
                    startTabletWaterMonitoring(tabletTime, waterInterval);
                });
        }
        
        function startTabletWaterMonitoring(tabletTime, waterInterval) {
            if (tabletWaterInterval) {
                clearInterval(tabletWaterInterval);
            }
            
            tabletWaterInterval = setInterval(() => {
                fetch('/ping/tabletwater?tablet_time=' + encodeURIComponent(tabletTime) + '&water_interval=' + encodeURIComponent(waterInterval));
            }, 5000);
        }
        
        window.onclick = function(event) {
            if (event.target.classList.contains('modal')) {
                event.target.style.display = 'none';
            }
        }
    </script>
</body>
</html>
)MAIN";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("========================================");

  // Initialize background monitoring sensors
  pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); 
  pinMode(LDR_PIN, INPUT);
  pinMode(LDR_RELAY_PIN, OUTPUT);
  digitalWrite(LDR_RELAY_PIN, HIGH); 
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 

  Serial.println("🔥 FIRE DETECTION: ACTIVE (Background)");
  Serial.println("💡 LIGHT DETECTION: ACTIVE (Background)");
  Serial.println("========================================");

  server.on("/", []() {
    server.send_P(200, "text/html", mainPage);
  });

  server.on("/dashboard/open", []() {
    fireDetectionActive = false;
    lightDetectionActive = false;
    Serial.println("\n========================================");
    Serial.println("📊 DASHBOARD OPENED");
    Serial.println("🔥 Fire Detection: PAUSED");
    Serial.println("💡 Light Detection: PAUSED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Dashboard opened");
  });

  server.on("/dashboard/close", []() {
    fireDetectionActive = true;
    lightDetectionActive = true;
    // Also stop any actively running features on dashboard close
    shakeHandActive = false;
    burglerAlarmActive = false;
    blindAssistActive = false;
    safetyLockerActive = false;
    chatbotActive = false;
    restaurantWaiterActive = false;
    joystickControlActive = false;
    faceRecognitionActive = false;
    tabletWaterActive = false;
    // Reset any associated states
    motionCounter = 0;
    digitalWrite(LED_PIN, LOW);
    timeClient.end();
    if (tabletServo.attached()) tabletServo.detach();
    digitalWrite(WATER_RELAY_PIN, HIGH);


    Serial.println("\n========================================");
    Serial.println("📊 DASHBOARD CLOSED - FACE MODE ACTIVE");
    Serial.println("🔥 Fire Detection: RESUMED");
    Serial.println("💡 Light Detection: RESUMED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Dashboard closed");
  });

  server.on("/submitphone", []() {
    if (server.hasArg("number")) {
      phoneNumber = server.arg("number");
      Serial.println("\n===================================");
      Serial.println("📱 MOBILE NUMBER RECEIVED");
      Serial.print("Phone Number: ");
      Serial.println(phoneNumber);
      Serial.println("===================================");
      server.send(200, "text/plain", "Phone number saved: " + phoneNumber);
    } else {
      server.send(400, "text/plain", "No phone number provided");
    }
  });

  server.on("/start/shakehand", []() {
    shakeHandActive = true;
    Serial.println("\n========================================");
    Serial.println("👋 SHAKE HAND FEATURE STARTED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Shake Hand Started");
  });

  server.on("/stop/shakehand", []() {
    shakeHandActive = false;
    Serial.println("\n========================================");
    Serial.println("👋 SHAKE HAND FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Shake Hand Stopped");
  });

  server.on("/start/burgler", []() {
    burglerAlarmActive = true;
    Serial.println("\n========================================");
    Serial.println("🚨 BURGLER ALARM FEATURE STARTED");
    pinMode(MOTION_SENSOR_PIN, INPUT);
    motionCounter = 0;
    lastBurglarNotificationTime = 0;
    previousBurglarMillis = millis(); 
    Serial.println("========================================");
    server.send(200, "text/plain", "Burgler Alarm Started");
  });

  server.on("/stop/burgler", []() {
    burglerAlarmActive = false;
    motionCounter = 0;
    digitalWrite(LED_PIN, LOW); 
    Serial.println("\n========================================");
    Serial.println("🚨 BURGLER ALARM FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Burgler Alarm Stopped");
  });

  server.on("/start/blind", []() {
    blindAssistActive = true;
    Serial.println("\n========================================");
    Serial.println("👁️ BLIND ASSIST FEATURE STARTED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Blind Assist Started");
  });

  server.on("/stop/blind", []() {
    blindAssistActive = false;
    Serial.println("\n========================================");
    Serial.println("👁️ BLIND ASSIST FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Blind Assist Stopped");
  });

  server.on("/start/locker", []() {
    safetyLockerActive = true;
    Serial.println("\n========================================");
    Serial.println("🔐 SAFETY LOCKER FEATURE STARTED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Safety Locker Started");
  });

  server.on("/stop/locker", []() {
    safetyLockerActive = false;
    Serial.println("\n========================================");
    Serial.println("🔐 SAFETY LOCKER FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Safety Locker Stopped");
  });

  server.on("/start/chatbot", []() {
    chatbotActive = true;
    Serial.println("\n========================================");
    Serial.println("💬 CHATBOT FEATURE STARTED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Chatbot Started");
  });

  server.on("/stop/chatbot", []() {
    chatbotActive = false;
    Serial.println("\n========================================");
    Serial.println("💬 CHATBOT FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Chatbot Stopped");
  });

  server.on("/start/server", []() {
    restaurantWaiterActive = true;
    Serial.println("\n========================================");
    Serial.println("🍽️ RESTAURANT WAITER FEATURE STARTED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Restaurant Waiter Started");
  });

  server.on("/stop/server", []() {
    restaurantWaiterActive = false;
    Serial.println("\n========================================");
    Serial.println("🍽️ RESTAURANT WAITER FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Restaurant Waiter Stopped");
  });

  server.on("/start/joystick", []() {
    joystickControlActive = true;
    Serial.println("\n========================================");
    Serial.println("🕹️ JOYSTICK CONTROL FEATURE STARTED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Joystick Control Started");
  });

  server.on("/stop/joystick", []() {
    joystickControlActive = false;
    Serial.println("\n========================================");
    Serial.println("🕹️ JOYSTICK CONTROL FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Joystick Control Stopped");
  });

  server.on("/start/facerecognition", []() {
    faceRecognitionActive = true;
    Serial.println("\n========================================");
    Serial.println("👤 FACE RECOGNITION FEATURE STARTED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Face Recognition Started");
  });

  server.on("/stop/facerecognition", []() {
    faceRecognitionActive = false;
    Serial.println("\n========================================");
    Serial.println("👤 FACE RECOGNITION FEATURE STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Face Recognition Stopped");
  });

  server.on("/start/tabletwater", []() {
    if (server.hasArg("tablet_time") && server.hasArg("water_interval")) {
      String tablet_time_str = server.arg("tablet_time");
      String water_interval_str = server.arg("water_interval");
      int colonIndex = tablet_time_str.indexOf(':');
      if (colonIndex != -1) {
        tabletTargetHour = tablet_time_str.substring(0, colonIndex).toInt();
        tabletTargetMinute = tablet_time_str.substring(colonIndex + 1).toInt();
      } else {
        Serial.println("Error parsing tablet time format.");
        server.send(400, "text/plain", "Invalid tablet time format");
        return;
      }
      int waterMinutes = water_interval_str.toInt();
      waterReminderIntervalMillis = (unsigned long)waterMinutes * 60 * 1000;
      tabletWaterActive = true;
      tabletTimeReachedForToday = false;
      timeClient.begin();
      timeClient.update();
      lastWaterReminderTime = millis();
      pinMode(WATER_RELAY_PIN, OUTPUT);
      pinMode(TRIG_PIN, OUTPUT);
      pinMode(ECHO_PIN, INPUT);
      digitalWrite(WATER_RELAY_PIN, HIGH);
      digitalWrite(TRIG_PIN, LOW);
      tabletServo.attach(SERVO_PIN);
      tabletServo.write(0);
      Serial.println("\n========================================");
      Serial.println("💊 TABLET/WATER REMINDER STARTED");
      Serial.print("Tablet Time: "); Serial.print(tabletTargetHour); Serial.print(":"); Serial.println(tabletTargetMinute);
      Serial.print("Water Interval: "); Serial.print(waterMinutes); Serial.println(" minutes");
      Serial.println("========================================");
      server.send(200, "text/plain", "Tablet/Water Reminder Started");
    } else {
      server.send(400, "text/plain", "Missing parameters for tabletwater");
    }
  });

  server.on("/ping/tabletwater", []() {
    if (tabletWaterActive) {
      server.send(200, "text/plain", "Monitoring");
    } else {
      server.send(200, "text/plain", "Not active");
    }
  });

  server.on("/stop/tabletwater", []() {
    tabletWaterActive = false;
    timeClient.end();
    if (tabletServo.attached()) tabletServo.detach(); 
    digitalWrite(WATER_RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    Serial.println("\n========================================");
    Serial.println("💊 TABLET/WATER REMINDER STOPPED");
    Serial.println("========================================");
    server.send(200, "text/plain", "Tablet/Water Reminder Stopped");
  });

  server.begin();
  Serial.println("\n✅ HTTP SERVER STARTED SUCCESSFULLY");
  Serial.println("========================================");
}

void loop() {
  server.handleClient();
  
  if (fireDetectionActive) {
    fireDetectionFeature();
  }
  
  if (lightDetectionActive) {
    lightDetectionFeature();
  }
  
  if (shakeHandActive) {
    shakeHandFeature();
  }
  
  if (burglerAlarmActive) {
    burglerAlarmFeature();
  }
  
  if (blindAssistActive) {
    blindAssistFeature();
  }
  
  if (safetyLockerActive) {
    safetyLockerFeature();
  }
  
  if (chatbotActive) {
    chatbotFeature();
  }
  
  if (restaurantWaiterActive) {
    restaurantWaiterFeature();
  }
  
  if (joystickControlActive) {
    joystickControlFeature();
  }
  
  if (faceRecognitionActive) {
    faceRecognitionFeature();
  }
  
  if (tabletWaterActive) {
    tabletWaterFeature();
  }
}