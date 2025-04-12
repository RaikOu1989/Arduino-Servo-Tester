//Calibrate 18 servos, one at a time
//Use 3 buttons:
//NEXT → Switch to the next servo
//SET MIN → Store current pulse as the minimum for selected servo
//SET MAX → Store current pulse as the maximum
//Use a potentiometer to control the pulse width (500–2500µs)
//Display servo number, current pulse, and saved min/max
//Save values in EEPROM for persistence
//Component |	Pin
//Potentiometer |	A0
//Servo |	9
//LCD	| 12,11,5,4,3,2
//Button: NEXT |	6
//Button: SET MIN	| 7
//Button: SET MAX |	8

#include <LiquidCrystal.h>
#include <Servo.h>
#include <EEPROM.h>

// Constants
const unsigned long exportHoldTime = 3000; // 3 seconds
const int servoCount = 18;
const int minPulseDefault = 1000;
const int maxPulseDefault = 2000;

// Pins
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
const int servoPin = 9;
const int potPin = A0;
const int nextBtnPin = 6;
const int setMinPin = 7;
const int setMaxPin = 8;

// Objects
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
Servo myServo;

// Variables
int currentServo = 0;
int pulseWidth = 1500;
int lastServo = -1;

unsigned long exportTriggerTime = 0;
bool exportingNow = false;

// EEPROM addressing
int getMinAddress(int index) { return index * 4; }
int getMaxAddress(int index) { return index * 4 + 2; }

void setup() {
  Serial.begin(9600);
  myServo.attach(servoPin);
  lcd.begin(16, 2);

  pinMode(nextBtnPin, INPUT_PULLUP);
  pinMode(setMinPin, INPUT_PULLUP);
  pinMode(setMaxPin, INPUT_PULLUP);

  lcd.print("Servo Calibrator");
  delay(1000);
  lcd.clear();
}

void loop() {
  bool nextPressed = digitalRead(nextBtnPin) == LOW;
  bool minPressed = digitalRead(setMinPin) == LOW;

  // Combo hold for export
  if (nextPressed && minPressed) {
    if (exportTriggerTime == 0) {
      exportTriggerTime = millis();
    } else if (millis() - exportTriggerTime >= exportHoldTime && !exportingNow) {
      exportingNow = true;
      exportToSerial();
      exportingNow = false;
      exportTriggerTime = 0;
      lastServo = -1; // force screen redraw
      return; // prevent LCD flicker
    }

    // Show hold status
    lcd.setCursor(0, 0);
    lcd.print("Hold to Export   ");
    lcd.setCursor(0, 1);
    int dots = ((millis() - exportTriggerTime) / 500) % 4;
    lcd.print("Hold");
    for (int i = 0; i < dots; i++) lcd.print(".");
    for (int i = dots; i < 3; i++) lcd.print(" ");
    delay(100);
    return;
  } else {
    exportTriggerTime = 0; // reset if buttons released
  }

  // Potentiometer & servo
  int potVal = analogRead(potPin);
  pulseWidth = map(potVal, 0, 1023, 500, 2500);
  myServo.writeMicroseconds(pulseWidth);

  // Button actions
  if (buttonPressed(nextBtnPin)) {
    currentServo = (currentServo + 1) % servoCount;
    lastServo = -1;
  }

  if (buttonPressed(setMinPin)) {
    savePulse(getMinAddress(currentServo), pulseWidth);
    lcd.setCursor(0, 1);
    lcd.print("Min saved       ");
    delay(500);
  }

  if (buttonPressed(setMaxPin)) {
    savePulse(getMaxAddress(currentServo), pulseWidth);
    lcd.setCursor(0, 1);
    lcd.print("Max saved       ");
    delay(500);
  }

  // LCD display update
  if (lastServo != currentServo) {
    lcd.clear();
    lastServo = currentServo;
  }

  lcd.setCursor(0, 0);
  lcd.print("Servo ");
  lcd.print(currentServo + 1);
  lcd.print(" PWM:");
  lcd.print(pulseWidth);
  lcd.print("   ");

  lcd.setCursor(0, 1);
  lcd.print("Mn:");
  lcd.print(readPulse(getMinAddress(currentServo)));
  lcd.print(" Mx:");
  lcd.print(readPulse(getMaxAddress(currentServo)));
  lcd.print("   ");

  delay(150);
}

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(20);
    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW);
      return true;
    }
  }
  return false;
}

void savePulse(int addr, int val) {
  EEPROM.put(addr, val);
}

int readPulse(int addr) {
  int val;
  EEPROM.get(addr, val);
  if (val < 500 || val > 2500) {
    return (addr % 4 == 0) ? minPulseDefault : maxPulseDefault;
  }
  return val;
}

void exportToSerial() {
  Serial.println("Servo,Min,Max");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Exporting...");
  lcd.setCursor(0, 1);

  int barsPrinted = 0;
  int step = servoCount / 16;
  if (step < 1) step = 1;

  for (int i = 0; i < servoCount; i++) {
    int minVal = readPulse(getMinAddress(i));
    int maxVal = readPulse(getMaxAddress(i));

    Serial.print(i + 1);
    Serial.print(",");
    Serial.print(minVal);
    Serial.print(",");
    Serial.println(maxVal);

    if ((i + 1) % step == 0 && barsPrinted < 16) {
      lcd.print((char)255);
      barsPrinted++;
    }

    delay(50);
  }

  Serial.println("Export complete.");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Export Complete");
  lcd.setCursor(0, 1);
  lcd.print("Check Serial");
  delay(2000);
  lcd.clear();
}