#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

// ------------------ Motor Driver Pins ------------------
#define ENA_LEFT_SPEED 13
#define IN1_LEFT_DIR_1 12
#define IN2_LEFT_DIR_2 14

#define ENB_RIGHT_SPEED 27
#define IN3_RIGHT_DIR_1 26
#define IN4_RIGHT_DIR_2 25

// ------------------ Joystick ------------------
#define JOYSTICK_X_PIN 34
#define JOYSTICK_Y_PIN 35
#define JOYSTICK_SW_PIN 23

// ------------------ Ultrasonic Sensors ------------------
#define ULTRASONIC_LEFT_TRIG 5
#define ULTRASONIC_LEFT_ECHO 4
#define ULTRASONIC_RIGHT_TRIG 18
#define ULTRASONIC_RIGHT_ECHO 19

// ------------------ Line Follower IR Sensors ------------------
#define LEFT_IR_PIN 32
#define RIGHT_IR_PIN 33

// ------------------ Indicators ------------------
#define MODE_LED_PIN 2
#define BUZZER_PIN 15

// ------------------ Constants ------------------
const int baseSpeed = 150;
int mode = 0; // 0 = Manual, 1 = Blind, 2 = Line Follower
bool lastSwitchState = HIGH;

// ------------------ Function Declarations ------------------
void drive(int leftSpeed, int rightSpeed);
void stopMotors();
long readDistance(int trigPin, int echoPin);
void blindGuidanceMode();
void lineFollowerMode();
void beep(int duration);

void setup() {
  Serial.begin(115200);
  Dabble.begin("ESP32_BOT");

  // Motor setup
  pinMode(IN1_LEFT_DIR_1, OUTPUT);
  pinMode(IN2_LEFT_DIR_2, OUTPUT);
  pinMode(IN3_RIGHT_DIR_1, OUTPUT);
  pinMode(IN4_RIGHT_DIR_2, OUTPUT);

  // Joystick + sensors
  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
  pinMode(ULTRASONIC_LEFT_TRIG, OUTPUT);
  pinMode(ULTRASONIC_LEFT_ECHO, INPUT);
  pinMode(ULTRASONIC_RIGHT_TRIG, OUTPUT);
  pinMode(ULTRASONIC_RIGHT_ECHO, INPUT);
  pinMode(LEFT_IR_PIN, INPUT);
  pinMode(RIGHT_IR_PIN, INPUT);

  // Indicators
  pinMode(MODE_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(MODE_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("=== ESP32 Multi-Mode Bot Initialised ===");
  Serial.println("Mode: 🕹 Manual Control");
}

void loop() {
  Dabble.processInput();

  // ---------- Mode Toggle ----------
  bool swState = digitalRead(JOYSTICK_SW_PIN);
  if ((swState == LOW && lastSwitchState == HIGH) || GamePad.isSelectPressed()) {
    mode = (mode + 1) % 3;  // Cycle through 3 modes
    beep(150);
    digitalWrite(MODE_LED_PIN, HIGH);
    delay(100);
    digitalWrite(MODE_LED_PIN, LOW);

    switch (mode) {
      case 0: Serial.println("\n🕹 Mode: Manual Control"); break;
      case 1: Serial.println("\n🤖 Mode: Blind Guidance"); break;
      case 2: Serial.println("\n⚫ Mode: Line Follower"); break;
    }
    delay(300);
  }
  lastSwitchState = swState;

  // ---------- Mode Handling ----------
  if (mode == 0) {
    // Manual Control
    int joyX = analogRead(JOYSTICK_X_PIN);
    int joyY = analogRead(JOYSTICK_Y_PIN);
    int mappedX = map(joyX, 0, 4095, -255, 255);
    int mappedY = map(joyY, 0, 4095, -255, 255);
    bool dabbleActive = false;

    if (GamePad.isUpPressed()) { drive(baseSpeed, baseSpeed); dabbleActive = true; }
    else if (GamePad.isDownPressed()) { drive(-baseSpeed, -baseSpeed); dabbleActive = true; }
    else if (GamePad.isLeftPressed()) { drive(-baseSpeed/1.2, baseSpeed/1.2); dabbleActive = true; }
    else if (GamePad.isRightPressed()) { drive(baseSpeed/1.2, -baseSpeed/1.2); dabbleActive = true; }
    else if (GamePad.isStartPressed()) { stopMotors(); dabbleActive = true; }

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

  else if (mode == 1) {
    blindGuidanceMode();
  }

  else if (mode == 2) {
    lineFollowerMode();
  }

  delay(20);
}

// ------------------ Motor Functions ------------------
void drive(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // Left motor
  digitalWrite(IN1_LEFT_DIR_1, leftSpeed >= 0);
  digitalWrite(IN2_LEFT_DIR_2, leftSpeed < 0);

  // Right motor
  digitalWrite(IN3_RIGHT_DIR_1, rightSpeed >= 0);
  digitalWrite(IN4_RIGHT_DIR_2, rightSpeed < 0);

  analogWrite(ENA_LEFT_SPEED, abs(leftSpeed));
  analogWrite(ENB_RIGHT_SPEED, abs(rightSpeed));
}

void stopMotors() {
  analogWrite(ENA_LEFT_SPEED, 0);
  analogWrite(ENB_RIGHT_SPEED, 0);
}

// ------------------ Ultrasonic ------------------
long readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance > 0 ? distance : 999;
}

// ------------------ Blind Guidance Mode ------------------
void blindGuidanceMode() {
  long leftDist = readDistance(ULTRASONIC_LEFT_TRIG, ULTRASONIC_LEFT_ECHO);
  long rightDist = readDistance(ULTRASONIC_RIGHT_TRIG, ULTRASONIC_RIGHT_ECHO);
  long threshold = 20;

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
    beep(60);
    delay(200);
  }
  else if (rightDist < threshold) {
    Serial.println("↩ Object Right — Turning Left");
    drive(-baseSpeed, baseSpeed);
    beep(60);
    delay(200);
  }
  else {
    drive(baseSpeed, baseSpeed);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ------------------ Line Follower Mode ------------------
void lineFollowerMode() {
  int leftIR = digitalRead(LEFT_IR_PIN);
  int rightIR = digitalRead(RIGHT_IR_PIN);

  // Assuming: 0 = Black Line, 1 = White Surface
  if (leftIR == 0 && rightIR == 0) {
    Serial.println("⬆ On Track — Forward");
    drive(baseSpeed, baseSpeed);
  }
  else if (leftIR == 0 && rightIR == 1) {
    Serial.println("↩ Adjust Left");
    drive(baseSpeed / 2, baseSpeed);
  }
  else if (leftIR == 1 && rightIR == 0) {
    Serial.println("↪ Adjust Right");
    drive(baseSpeed, baseSpeed / 2);
  }
  else {
    Serial.println("❌ Lost Line — Stop");
    stopMotors();
  }
}

// ------------------ Buzzer ------------------
void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}
