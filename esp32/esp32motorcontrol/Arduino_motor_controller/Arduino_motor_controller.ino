// IMPORTANT: This code requires the Dabble Library (available on GitHub or the Arduino Library Manager).
// The Dabble app requires an HC-05 or HC-06 Bluetooth module, NOT the NRF24L01 module.

#include <Dabble.h>
#include <SoftwareSerial.h>

// --- MOTOR DRIVER PINS (L298N Configuration for 4WD) ---
// Left Motors (wired in parallel)
#define ENA_LEFT 9   // PWM pin for speed
#define IN1_LEFT 8   // Direction pin 1
#define IN2_LEFT 7   // Direction pin 2

// Right Motors (wired in parallel)
#define ENB_RIGHT 10 // PWM pin for speed
#define IN3_RIGHT 6  // Direction pin 3
#define IN4_RIGHT 5  // Direction pin 4

// --- PHYSICAL JOYSTICK PINS ---
#define JOYSTICK_X A0 // Analog input for X-axis (Steering)
#define JOYSTICK_Y A1 // Analog input for Y-axis (Speed)

// --- CONFIGURATION CONSTANTS ---
const int ANALOG_CENTER = 512; // Center value for 10-bit ADC (1023/2)
const int DEAD_ZONE = 100;     // Range around the center to treat as stop (512 +/- 100)
const int MAX_SPEED = 255;     // Maximum PWM speed
const bool DEBUG_MODE = true;  // Set to true to enable Serial Monitor debugging prints

// Function to control motor speeds and directions
void setMotorSpeeds(int leftSpeed, int rightSpeed) {
    // --- LEFT MOTOR CONTROL ---
    if (leftSpeed > 0) {
        // Forward
        digitalWrite(IN1_LEFT, HIGH);
        digitalWrite(IN2_LEFT, LOW);
        analogWrite(ENA_LEFT, leftSpeed);
    } else if (leftSpeed < 0) {
        // Reverse
        digitalWrite(IN1_LEFT, LOW);
        digitalWrite(IN2_LEFT, HIGH);
        analogWrite(ENA_LEFT, abs(leftSpeed));
    } else {
        // Stop
        digitalWrite(IN1_LEFT, LOW);
        digitalWrite(IN2_LEFT, LOW);
        analogWrite(ENA_LEFT, 0);
    }

    // --- RIGHT MOTOR CONTROL ---
    if (rightSpeed > 0) {
        // Forward
        digitalWrite(IN3_RIGHT, HIGH);
        digitalWrite(IN4_RIGHT, LOW);
        analogWrite(ENB_RIGHT, rightSpeed);
    } else if (rightSpeed < 0) {
        // Reverse
        digitalWrite(IN3_RIGHT, LOW);
        digitalWrite(IN4_RIGHT, HIGH);
        analogWrite(ENB_RIGHT, abs(rightSpeed));
    } else {
        // Stop
        digitalWrite(IN3_RIGHT, LOW);
        digitalWrite(IN4_RIGHT, LOW);
        analogWrite(ENB_RIGHT, 0);
    }
}

void setup() {
    Serial.begin(9600);
    Dabble.begin(9600); // Initialize Dabble communication at 9600 baud (standard for HC-05/06)
    
    // Print initialization message
    if (DEBUG_MODE) {
      Serial.println("Controller Initialized. Waiting for Dabble or Joystick input.");
    }

    // Set motor driver pins as OUTPUTs
    pinMode(ENA_LEFT, OUTPUT);
    pinMode(IN1_LEFT, OUTPUT);
    pinMode(IN2_LEFT, OUTPUT);
    pinMode(ENB_RIGHT, OUTPUT);
    pinMode(IN3_RIGHT, OUTPUT);
    pinMode(IN4_RIGHT, OUTPUT);

    // Ensure motors are stopped initially
    setMotorSpeeds(0, 0);
}

void loop() {
    Dabble.processInput(); // Must be called in every loop iteration

    int finalLeftSpeed = 0;
    int finalRightSpeed = 0;
    
    // GamePad.getYaxisData() returns -100 to 100 (Forward/Reverse)
    int yValue = GamePad.getYaxisData();
    // GamePad.getXaxisData() returns -100 to 100 (Steering)
    int xValue = GamePad.getXaxisData();


    // --- 1. DABBLE APP CONTROL (Priority Input) ---
    // Check if the Dabble joystick is actively sending a non-zero command. 
    // This replaces the deprecated GamePad.isDataReceived().
    if (xValue != 0 || yValue != 0) {
        // Use the Joystick widget (Axis 1 and 2) in the Dabble GamePad screen
        
        if (DEBUG_MODE) {
          Serial.print("[DABBLE] X:");
          Serial.print(xValue);
          Serial.print(", Y:");
          Serial.print(yValue);
        }

        // Map the -100 to 100 range to motor speed/direction (max speed 255)
        int power = map(abs(yValue), 0, 100, 0, MAX_SPEED);

        // Simple tank drive mixing logic:
        if (yValue != 0) {
            // Moving Forward/Backward
            if (xValue > 0) {
                // Turning Right (Reduce Right Motor Speed)
                finalLeftSpeed = yValue > 0 ? power : -power;
                finalRightSpeed = (yValue > 0 ? power : -power) - map(xValue, 0, 100, 0, power);
            } else if (xValue < 0) {
                // Turning Left (Reduce Left Motor Speed)
                finalLeftSpeed = (yValue > 0 ? power : -power) + map(abs(xValue), 0, 100, 0, power);
                finalRightSpeed = yValue > 0 ? power : -power;
            } else {
                // Straight
                finalLeftSpeed = yValue > 0 ? power : -power;
                finalRightSpeed = yValue > 0 ? power : -power;
            }
        } else {
            // Static rotation (if Y is zero, X controls spin)
            int spinPower = map(abs(xValue), 0, 100, 0, MAX_SPEED);
            if (xValue > 0) {
                // Spin Right
                finalLeftSpeed = spinPower;
                finalRightSpeed = -spinPower;
            } else if (xValue < 0) {
                // Spin Left
                finalLeftSpeed = -spinPower;
                finalRightSpeed = spinPower;
            } else {
                // Stop
                finalLeftSpeed = 0;
                finalRightSpeed = 0;
            }
        }
        
        if (DEBUG_MODE) {
          Serial.print(" -> L:");
          Serial.print(finalLeftSpeed);
          Serial.print(", R:");
          Serial.println(finalRightSpeed);
        }

    }

    // --- 2. PHYSICAL JOYSTICK CONTROL (Fallback Input) ---
    // Only execute if Dabble is not actively sending movement commands
    else {
        // Read physical joystick values
        int joyY = analogRead(JOYSTICK_Y);
        int joyX = analogRead(JOYSTICK_X);
        
        if (DEBUG_MODE) {
          Serial.print("[PHYSICAL] Raw X:");
          Serial.print(joyX);
          Serial.print(", Raw Y:");
          Serial.print(joyY);
        }

        // Check if Y axis is outside the dead zone
        if (abs(joyY - ANALOG_CENTER) > DEAD_ZONE) {
            // Map Y axis (0-1023) to motor power (-MAX_SPEED to +MAX_SPEED)
            // Center is 512. >512 is reverse (negative speed), <512 is forward (positive speed)
            int power = map(joyY, 0, 1023, -MAX_SPEED, MAX_SPEED);

            // Invert the speed to match typical joystick direction (push forward = forward)
            // If your joystick is inverted, remove the '-'
            power = -power; 

            // Check if X axis is outside the dead zone (Steering)
            if (abs(joyX - ANALOG_CENTER) > DEAD_ZONE) {
                // Map X axis (0-1023) to a steering offset (-1 to 1)
                // Center is 512.
                float steer = map(joyX, 0, 1023, -100, 100) / 100.0; // steer will be -1.0 to 1.0
                
                if (DEBUG_MODE) {
                    Serial.print(", Power:");
                    Serial.print(power);
                    Serial.print(", Steer:");
                    Serial.print(steer);
                }

                // Apply steering logic:
                if (steer > 0) { // Turning Right
                    finalLeftSpeed = power;
                    finalRightSpeed = power * (1.0 - steer); // Reduce right motor speed
                } else { // Turning Left
                    finalRightSpeed = power;
                    finalLeftSpeed = power * (1.0 + steer); // Reduce left motor speed (steer is negative)
                }
            } else {
                // Moving straight
                finalLeftSpeed = power;
                finalRightSpeed = power;
            }
        } else if (abs(joyX - ANALOG_CENTER) > DEAD_ZONE) {
             // Static rotation (Y is zero, X controls spin)
            int spinPower = map(abs(joyX - ANALOG_CENTER), DEAD_ZONE, ANALOG_CENTER, 0, MAX_SPEED);
            if (joyX > ANALOG_CENTER) { // Spin Right (Right motor reverse, Left motor forward)
                finalLeftSpeed = spinPower;
                finalRightSpeed = -spinPower;
            } else { // Spin Left (Left motor reverse, Right motor forward)
                finalLeftSpeed = -spinPower;
                finalRightSpeed = spinPower;
            }
        } else {
            // Joystick is centered, stop the motors
            finalLeftSpeed = 0;
            finalRightSpeed = 0;
        }
        
        if (DEBUG_MODE) {
            // Only print final speeds if motion is detected, otherwise print stop message.
            if (finalLeftSpeed != 0 || finalRightSpeed != 0) {
              Serial.print(" -> L:");
              Serial.print(finalLeftSpeed);
              Serial.print(", R:");
              Serial.println(finalRightSpeed);
            } else if (abs(joyX - ANALOG_CENTER) < DEAD_ZONE && abs(joyY - ANALOG_CENTER) < DEAD_ZONE) {
              Serial.println(" -> Stopped (Centered)");
            }
        }
    }

    // Execute the final determined motor speeds
    setMotorSpeeds(finalLeftSpeed, finalRightSpeed);
}
