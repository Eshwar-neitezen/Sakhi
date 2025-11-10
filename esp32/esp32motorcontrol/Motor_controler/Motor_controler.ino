#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

// --- Bluetooth Setup ---
#define BLUETOOTH_NAME "Sakhi"

// --- L298N Pin Definitions ---
#define ENA_LEFT_SPEED 13
#define IN1_LEFT_DIR_1 12
#define IN2_LEFT_DIR_2 14

#define ENB_RIGHT_SPEED 27
#define IN3_RIGHT_DIR_1 26
#define IN4_RIGHT_DIR_2 25

// --- Joystick ---
#define JOYSTICK_X_PIN 34
#define JOYSTICK_Y_PIN 35

// --- Speed Limits ---
int baseSpeed = 180; // Max PWM 255 for ESP32

// --- Function Declarations ---
void drive(int leftSpeed, int rightSpeed);
void stopMotors();

void setup() {
  Serial.begin(115200);
  Dabble.begin(BLUETOOTH_NAME); // BLE name

  pinMode(IN1_LEFT_DIR_1, OUTPUT);
  pinMode(IN2_LEFT_DIR_2, OUTPUT);
  pinMode(IN3_RIGHT_DIR_1, OUTPUT);
  pinMode(IN4_RIGHT_DIR_2, OUTPUT);

  pinMode(ENA_LEFT_SPEED, OUTPUT);
  pinMode(ENB_RIGHT_SPEED, OUTPUT);

  Serial.println("=== ESP32 4WD Controller Initialized ===");
  Serial.println("Waiting for Dabble or Joystick input...");
}

void loop() {
  Dabble.processInput();

  // --- Read Joystick ---
  int joyX = analogRead(JOYSTICK_X_PIN);
  int joyY = analogRead(JOYSTICK_Y_PIN);

  int mappedX = map(joyX, 0, 4095, -255, 255);
  int mappedY = map(joyY, 0, 4095, -255, 255);

  bool dabbleActive = false;

  // --- Dabble Control ---
  if (GamePad.isUpPressed()) {
    Serial.println("Dabble: FORWARD");
    drive(baseSpeed, baseSpeed);
    dabbleActive = true;
  }
  else if (GamePad.isDownPressed()) {
    Serial.println("Dabble: BACKWARD");
    drive(-baseSpeed, -baseSpeed);
    dabbleActive = true;
  }
  else if (GamePad.isLeftPressed()) {
    Serial.println("Dabble: LEFT");
    drive(-baseSpeed / 1.2, baseSpeed / 1.2);
    dabbleActive = true;
  }
  else if (GamePad.isRightPressed()) {
    Serial.println("Dabble: RIGHT");
    drive(baseSpeed / 1.2, -baseSpeed / 1.2);
    dabbleActive = true;
  }
  else if (GamePad.isStartPressed()) {
    Serial.println("Dabble: STOP");
    stopMotors();
    dabbleActive = true;
  }

  // --- Joystick Control ---
  if (!dabbleActive) {
    int threshold = 100; // Dead zone

    if (abs(mappedY) > threshold || abs(mappedX) > threshold) {
      int leftSpeed = mappedY + mappedX;
      int rightSpeed = mappedY - mappedX;

      leftSpeed = constrain(leftSpeed, -255, 255);
      rightSpeed = constrain(rightSpeed, -255, 255);

      Serial.print("Joystick - L: ");
      Serial.print(leftSpeed);
      Serial.print("  R: ");
      Serial.println(rightSpeed);

      drive(leftSpeed, rightSpeed);
    } else {
      stopMotors();
    }
  }

  delay(20); // Smooth response
}

// --- Drive Function ---
void drive(int leftSpeed, int rightSpeed) {
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

// --- Stop Function ---
void stopMotors() {
  analogWrite(ENA_LEFT_SPEED, 0);
  analogWrite(ENB_RIGHT_SPEED, 0);
  digitalWrite(IN1_LEFT_DIR_1, LOW);
  digitalWrite(IN2_LEFT_DIR_2, LOW);
  digitalWrite(IN3_RIGHT_DIR_1, LOW);
  digitalWrite(IN4_RIGHT_DIR_2, LOW);
}
