
#include <Servo.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ======== Servo Setup ========
Servo servoHandshake;   // Handshake servo
Servo servoLeftArm;     // Left arm servo
Servo servoSafe;        // Safe lock servo

const int servoHandshakePin = 3;
const int servoLeftArmPin   = 5;
const int servoSafePin      = 6;

// ======== Buttons ========
const int buttonHandshakePin = 7; // Button 1: handshake
const int buttonArmPin       = 8; // Button 2: raise/lower arms

bool armRaised = false;
bool lastButtonArmState = HIGH;

// ======== Keypad Setup ========
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 10, 11, 12};
byte colPins[COLS] = {A0, A1, A2, A3};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String password = "1234";
String input = "";

// ======== LCD Setup ========
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change address if needed

void setup() {
  servoHandshake.attach(servoHandshakePin);
  servoLeftArm.attach(servoLeftArmPin);
  servoSafe.attach(servoSafePin);

  pinMode(buttonHandshakePin, INPUT_PULLUP);
  pinMode(buttonArmPin, INPUT_PULLUP);

  // Initialize servos
  servoHandshake.write(0);
  servoLeftArm.write(0);
  servoSafe.write(0);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Ready");
}

void loop() {
  // ----- Handshake button -----
  if (digitalRead(buttonHandshakePin) == LOW) {
    handshakeMotion();
    delay(500); // debounce
  }

  // ----- Arm toggle button -----
  bool currentButtonArmState = digitalRead(buttonArmPin);
  if (lastButtonArmState == HIGH && currentButtonArmState == LOW) {
    armRaised = !armRaised;
    if (armRaised) {
      servoHandshake.write(90);
      servoLeftArm.write(0);
      lcd.setCursor(0,1);
      lcd.print("Arms Raised    ");
    } else {
      servoHandshake.write(0);
      servoLeftArm.write(90);
      lcd.setCursor(0,1);
      lcd.print("Arms Lowered  ");
    }
    delay(300);
  }
  lastButtonArmState = currentButtonArmState;

  // ----- Keypad password -----
  char key = keypad.getKey();
  if (key) {
    if (key == '#') { // Enter key
      lcd.setCursor(0,1);
      if (input == password) {
        servoSafe.write(90); // unlock
        lcd.print("Safe Unlocked ");
      } else {
        servoSafe.write(0);  // lock
        lcd.print("Access Denied ");
      }
      input = ""; // reset input
    } 
    else if (key == '*') { // Clear
      input = "";
      lcd.setCursor(0,1);
      lcd.print("Entry Cleared ");
      servoSafe.write(90);
    } 
    else { // Add digit
      if (input.length() < 10) { // limit input
        input += key;
        lcd.setCursor(0,1);
        lcd.print("Enter: ");
        for (int i = 0; i < input.length(); i++) lcd.print("*");
        lcd.print("   "); // clear remaining chars
      }
    }
  }
}

// ----- Handshake motion -----
void handshakeMotion() {
  servoHandshake.write(90);
  delay(300);

  servoHandshake.write(110);
  delay(200);
  servoHandshake.write(70);
  delay(200);
  servoHandshake.write(100);
  delay(200);
  servoHandshake.write(80);
  delay(200);

  servoHandshake.write(0);
  delay(300);
}
