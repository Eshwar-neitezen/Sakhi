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
// Using a single sensor at the front, reusing the previous "left" pins.
#define ULTRASONIC_FRONT_TRIG 5
#define ULTRASONIC_FRONT_ECHO 4
// Removed the right ultrasonic sensor definitions.

// ------------------ Indicators ------------------
#define MODE_LED_PIN 2   // Onboard LED (GPIO 2)
#define BUZZER_PIN 15    // Active buzzer pin

// ------------------ Constants and State ------------------
const int baseSpeed = 120;
bool blindMode = false; // false = Joystick_mode (Manual), true = Blind_mode (Auto Avoidance)
bool lastSwitchState = HIGH;

// ------------------ Function Declarations ------------------
void drive(int leftSpeed, int rightSpeed);
void stopMotors();
long readDistance(int trigPin, int echoPin);
void blindGuidanceMode();
void beep(int duration);
void setMode(bool newMode, const char* source);

void setup() {
  // Serial communication is essential for the RX/TX state change
  Serial.begin(115200);
  Dabble.begin("Sakhi");

  // Motor setup
  pinMode(IN1_LEFT_DIR_1, OUTPUT);
  pinMode(IN2_LEFT_DIR_2, OUTPUT);
  pinMode(IN3_RIGHT_DIR_1, OUTPUT);
  pinMode(IN4_RIGHT_DIR_2, OUTPUT);

  // Joystick button
  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);

  // Ultrasonic sensor (Front only)
  pinMode(ULTRASONIC_FRONT_TRIG, OUTPUT);
  pinMode(ULTRASONIC_FRONT_ECHO, INPUT);
  
  // LED + Buzzer
  pinMode(MODE_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(MODE_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\n=== System Initialised ===");
  Serial.println("Send 'Blind_mode' or 'Joystick_mode' via Serial to switch modes.");
  setMode(blindMode, "Initialization");
}

void loop() {
  Dabble.processInput();

  // ---------- Mode Toggle via Serial RX (NEW LOGIC) ----------
  // Checks for incoming data on the RX line
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Read incoming serial command until newline
    command.trim(); // Remove leading/trailing whitespace

    if (command.equalsIgnoreCase("Blind_mode")) {
      setMode(true, "Serial RX");
    } else if (command.equalsIgnoreCase("Joystick_mode")) {
      setMode(false, "Serial RX");
    }
  }

  // ---------- Mode Toggle via Joystick Button (TOGGLE FUNCTION) ----------
  bool swState = digitalRead(JOYSTICK_SW_PIN);
  if (swState == LOW && lastSwitchState == HIGH) {
    setMode(!blindMode, "Joystick Button");
    delay(300); // Debounce delay
  }
  lastSwitchState = swState;

  // ---------- Mode Toggle via Dabble (TOGGLE FUNCTION) ----------
  if (GamePad.isSelectPressed()) {
    setMode(!blindMode, "Dabble Select");
    delay(300); // Debounce delay
  }

  // ---------- Mode Handling ----------
  if (blindMode) {
    blindGuidanceMode();
  } else {
    // Manual Control Section (Joystick_mode)
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

// ------------------ State Change Function ------------------
// Helper function to handle mode changes cleanly
void setMode(bool newMode, const char* source) {
    if (blindMode != newMode) {
        blindMode = newMode;
        digitalWrite(MODE_LED_PIN, blindMode ? HIGH : LOW);
        beep(200);

        if (blindMode) {
            Serial.print("\n=== 🤖 Blind Guidance Mode ACTIVATED (via ");
            Serial.print(source);
            Serial.println(") ===");
        } else {
            Serial.print("\n=== 🕹 Joystick Mode ACTIVATED (via ");
            Serial.print(source);
            Serial.println(") ===");
            stopMotors();
        }
    }
}

// ------------------ Motor Functions (Unchanged) ------------------
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

// ------------------ Ultrasonic Functions (Unchanged) ------------------
long readDistance(int trigPin, int echoPin) {
  // Clears the trig pin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trig pin high for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echo pin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout after 30ms
  // Calculating the distance (speed of sound = 0.034 cm/us)
  long distance = duration * 0.034 / 2;
  // Return a large number if reading failed (0)
  return distance > 0 ? distance : 999;
}

// ------------------ Blind Guidance Mode (Single Sensor Logic) ------------------
void blindGuidanceMode() {
  // Read distance from the single front sensor
  long frontDist = readDistance(ULTRASONIC_FRONT_TRIG, ULTRASONIC_FRONT_ECHO);
  long threshold = 20; // Stop/Avoidance threshold in cm

  Serial.print("Front Distance: "); Serial.print(frontDist); Serial.println(" cm");

  // LED blink for activity
  digitalWrite(MODE_LED_PIN, millis() / 500 % 2);

  if (frontDist < threshold) {
    Serial.println("⚠️ Obstacle Detected! Stopping and Reversing.");
    // Single sensor avoidance: reverse, then try to turn/stop if necessary
    drive(-baseSpeed, -baseSpeed); 
    beep(100);
    delay(300);
    stopMotors(); 
    delay(1000);
    // After reversing, a human operator would typically take over or the robot 
    // could implement a simple turn-in-place logic (e.g., turn right) for 500ms
    // drive(baseSpeed, -baseSpeed); 
    // delay(500); 
    // stopMotors();

  }
  else {
    // No obstacles, move straight
    drive(baseSpeed, -baseSpeed);
    digitalWrite(BUZZER_PIN, LOW); // Silence buzzer if clear
  }
}

// ------------------ Buzzer Helper (Unchanged) ------------------
void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}
