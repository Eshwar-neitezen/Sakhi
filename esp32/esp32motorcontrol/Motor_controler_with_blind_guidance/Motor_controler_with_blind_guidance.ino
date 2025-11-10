#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

// ------------------ L298N Motor Driver Pins ------------------
#define ENA_LEFT_SPEED 13
#define IN1_LEFT_DIR_1 12
#define IN2_LEFT_DIR_2 14

#define ENB_RIGHT_SPEED 27
#define IN3_RIGHT_DIR_1 26
#define IN4_RIGHT_DIR_2 25

// ------------------ Joystick Pins ------------------
#define JOYSTICK_X_PIN 34
#define JOYSTICK_Y_PIN 35
#define JOYSTICK_SW_PIN 23

// ------------------ Ultrasonic Sensor Pins ------------------
#define ULTRASONIC_LEFT_TRIG 5
#define ULTRASONIC_LEFT_ECHO 4
#define ULTRASONIC_RIGHT_TRIG 18
#define ULTRASONIC_RIGHT_ECHO 19

// ------------------ Indicators ------------------
#define MODE_LED_PIN 2   // Onboard LED (GPIO 2)
#define BUZZER_PIN 15    // Active buzzer pin

// ------------------ Constants ------------------
const int baseSpeed = 150;
bool blindMode = false;
bool lastSwitchState = HIGH;

// ------------------ Function Declarations ------------------
void drive(int leftSpeed, int rightSpeed);
void stopMotors();
long readDistance(int trigPin, int echoPin);
void blindGuidanceMode();
void beep(int duration);

void setup() {
  Serial.begin(115200);
  Dabble.begin("Sakhi");

  // Motor setup
  pinMode(IN1_LEFT_DIR_1, OUTPUT);
  pinMode(IN2_LEFT_DIR_2, OUTPUT);
  pinMode(IN3_RIGHT_DIR_1, OUTPUT);
  pinMode(IN4_RIGHT_DIR_2, OUTPUT);

  // Joystick button
  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);

  // Ultrasonic sensors
  pinMode(ULTRASONIC_LEFT_TRIG, OUTPUT);
  pinMode(ULTRASONIC_LEFT_ECHO, INPUT);
  pinMode(ULTRASONIC_RIGHT_TRIG, OUTPUT);
  pinMode(ULTRASONIC_RIGHT_ECHO, INPUT);

  // LED + Buzzer
  pinMode(MODE_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(MODE_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\n=== System Initialised ===");
  Serial.println("Mode: 🕹 Manual Control");
}

void loop() {
  Dabble.processInput();

  // ---------- Mode Toggle via Joystick Button ----------
  bool swState = digitalRead(JOYSTICK_SW_PIN);
  if (swState == LOW && lastSwitchState == HIGH) {
    blindMode = !blindMode;
    digitalWrite(MODE_LED_PIN, blindMode ? HIGH : LOW);
    beep(200);
    if (blindMode) {
      Serial.println("\n=== 🤖 Blind Guidance Mode ACTIVATED ===");
    } else {
      Serial.println("\n=== 🕹 Manual Mode ACTIVATED ==="); 
      stopMotors();
    }
    delay(300);
  }
  lastSwitchState = swState;

  // ---------- Mode Toggle via Dabble ----------
  if (GamePad.isSelectPressed()) {
    blindMode = !blindMode;
    digitalWrite(MODE_LED_PIN, blindMode ? HIGH : LOW);
    beep(200);
    if (blindMode) {
      Serial.println("\n=== 🤖 Blind Guidance Mode ACTIVATED (via Dabble) ===");
    } else {
      Serial.println("\n=== 🕹 Manual Mode ACTIVATED (via Dabble) ===");
      stopMotors();
    }
    delay(300);
  }

  // ---------- Mode Handling ----------
  if (blindMode) {
    blindGuidanceMode();
  } else {
    // Manual Control Section
    int joyX = analogRead(JOYSTICK_X_PIN);
    int joyY = analogRead(JOYSTICK_Y_PIN);
    int mappedX = map(joyX, 0, 4095, -255, 255);
    int mappedY = map(joyY, 0, 4095, -255, 255);
    bool dabbleActive = false;

    // Dabble manual control
    if (GamePad.isUpPressed()) { drive(baseSpeed, baseSpeed); dabbleActive = true; }
    else if (GamePad.isDownPressed()) { drive(-baseSpeed, -baseSpeed); dabbleActive = true; }
    else if (GamePad.isLeftPressed()) { drive(-baseSpeed / 1.2, baseSpeed / 1.2); dabbleActive = true; }
    else if (GamePad.isRightPressed()) { drive(baseSpeed / 1.2, -baseSpeed / 1.2); dabbleActive = true; }
    else if (GamePad.isStartPressed()) { stopMotors(); dabbleActive = true; }

    // Analog joystick control
    if (!dabbleActive) {
      int threshold = 100;
      if (abs(mappedY) > threshold || abs(mappedX) > threshold) {
        int leftSpeed = constrain(mappedY + mappedX, -255, 255);
        int rightSpeed = constrain(mappedY - mappedX, -255, 255);
        drive(leftSpeed, rightSpeed);
      } else {
        stopMotors();
      }
    }
  }

  delay(20);
}

// ------------------ Motor Functions ------------------
void drive(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Left Motor
  if (leftSpeed >= 0) {
    digitalWrite(IN1_LEFT_DIR_1, HIGH);
    digitalWrite(IN2_LEFT_DIR_2, LOW);
  } else {
    digitalWrite(IN1_LEFT_DIR_1, LOW);
    digitalWrite(IN2_LEFT_DIR_2, HIGH);
  }

  // Right Motor
  if (rightSpeed >= 0) {
    digitalWrite(IN3_RIGHT_DIR_1, HIGH);
    digitalWrite(IN4_RIGHT_DIR_2, LOW);
  } else {
    digitalWrite(IN3_RIGHT_DIR_1, LOW);
    digitalWrite(IN4_RIGHT_DIR_2, HIGH);
  }

  analogWrite(ENA_LEFT_SPEED, abs(leftSpeed));
  analogWrite(ENB_RIGHT_SPEED, abs(rightSpeed));
}

void stopMotors() {
  analogWrite(ENA_LEFT_SPEED, 0);
  analogWrite(ENB_RIGHT_SPEED, 0);
}

// ------------------ Ultrasonic Functions ------------------
long readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance > 0 ? distance : 999;
}

// ------------------ Blind Guidance Mode ------------------
void blindGuidanceMode() {
  long leftDist = readDistance(ULTRASONIC_LEFT_TRIG, ULTRASONIC_LEFT_ECHO);
  long rightDist = readDistance(ULTRASONIC_RIGHT_TRIG, ULTRASONIC_RIGHT_ECHO);
  long threshold = 20; // cm

  Serial.print("Left: "); Serial.print(leftDist);
  Serial.print(" cm | Right: "); Serial.print(rightDist); Serial.println(" cm");

  // LED blink for activity
  digitalWrite(MODE_LED_PIN, millis() / 500 % 2);

  if (leftDist < threshold && rightDist < threshold) {
    Serial.println("⚠️ Object Ahead — Reversing");
    drive(-baseSpeed, -baseSpeed);
    beep(100);
    delay(300);
    stopMotors();
  }
  else if (leftDist < threshold) {
    Serial.println("↪ Object Left — Turning Right");
    drive(baseSpeed, -baseSpeed);
    beep(80);
    delay(200);
  }
  else if (rightDist < threshold) {
    Serial.println("↩ Object Right — Turning Left");
    drive(-baseSpeed, baseSpeed);
    beep(80);
    delay(200);
  }
  else {
    drive(baseSpeed, baseSpeed);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ------------------ Buzzer Helper ------------------
void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}
