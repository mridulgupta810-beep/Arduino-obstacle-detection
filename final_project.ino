#include <LiquidCrystal.h>
#include <IRremote.h>
#include <EEPROM.h>

#define IR_RECIEVE_PIN 5

#define ECHO_PIN 3
#define TRIGGER_PIN 4

#define WARNING_LED_PIN 11
#define ERROR_LED_PIN 12
#define LIGHT_LED_PIN 10

#define BUTTON_PIN 2

#define PHOTORESISTOR_PIN A0

//lcd
#define LCD_RS_PIN A5
#define LCD_E_PIN A4
#define LCD_D4_PIN 6
#define LCD_D5_PIN 7
#define LCD_D6_PIN 8
#define LCD_D7_PIN 9

LiquidCrystal lcd(LCD_RS_PIN ,LCD_E_PIN,LCD_D4_PIN , LCD_D5_PIN , LCD_D6_PIN , LCD_D7_PIN);

#define LOCK_DISTANCE 10.0
#define WARNING_DISTANCE 50.0

// IR BUTTON
#define IR_BUTTON_PLAY 28 // ok
#define IR_BUTTON_OFF 22 //*
#define IR_BUTTON_EQ 13// #
#define IR_BUTTON_UP 24
#define IR_BUTTON_DOWN 82
// distance unit
#define DISTANCE_UNIT_CM 0
#define DISTANCE_UNIT_IN 1
#define CM_TO_INCHES 0.393701
#define EEPROM_ADDRESS_DISTANCE_UNIT 50
//lcd mode
#define LCD_MODE_DISTANCE 0
#define LCD_MODE_SETTINGS 1
#define LCD_MODE_LUMINOSITY 2

//UTRASONIC
unsigned long lastTimeUltrasonicTrigger = millis();
unsigned long untrasonicTriggerDelay = 60;

volatile unsigned long pulseInTimebegin;
volatile unsigned long pulseTimeEnd;
volatile bool newDidtanceAvailable = false;

double previousDistance = 400.0;
// WARNING LED
unsigned long lastTimeWarningLEDBlinked = millis();
unsigned long WarningLEDDelay = 500;
byte WarningLEDState = LOW;

//ERROR LED
unsigned long lastTimeErrorLEDBlinked = millis();
unsigned long ErrorLEDDelay = 300;
byte errorLEDState = LOW;

//PHOTORESISTOR
unsigned long lastTimeReadLuminosity = millis();
unsigned long readLuminosityDelay = 100;

// push button
unsigned long lastTimeButtonChanged = millis();
unsigned long buttonDebounceDelay = 50;
byte buttonState;

bool isLocked = false;

int distanceUnit = DISTANCE_UNIT_CM;
int lcdMode =  LCD_MODE_DISTANCE;

void triggerUltrasonicSensor(){
  digitalWrite(TRIGGER_PIN , LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN , HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN , LOW);

}
double getUltrasonicDistance(){
  double durationMicros = pulseTimeEnd - pulseInTimebegin;
  double distance = durationMicros / 58.0;
  if(distance > 400){
    return previousDistance;
  }
  distance = previousDistance *0.6 + distance *0.4;
  previousDistance = distance;

  return distance;
}

void echoPinInterrupt(){
  if(digitalRead(ECHO_PIN) == HIGH){ 
    // PULSE STARTED
    pulseInTimebegin = micros();
  }
  else{
    // pulse stopped
    pulseTimeEnd = micros();
    newDidtanceAvailable = true;

  }
 

}

void toggleErrorLED(){
  errorLEDState = (errorLEDState == HIGH)? LOW:HIGH;
  digitalWrite(ERROR_LED_PIN , errorLEDState);

}

void toggleWarningLED(){
  // if(WarningLEDState == HIGH){
  //   WarningLEDState == LOW;
  // }
  // else{
  //   WarningLEDState == HIGH;
  // }
  // digitalWrite(WARNING_LED_PIN , WarningLEDState);
  WarningLEDState = (WarningLEDState == HIGH)? LOW : HIGH;
  digitalWrite(WARNING_LED_PIN , WarningLEDState);

}

void setWarningLEDBlinkRateFromDistance(double distance){
  // 0...400cm -> 0....1600ms
  WarningLEDDelay = distance*4;
  Serial.println(WarningLEDDelay);

}

 void lock(){
    if(!isLocked){
      Serial.println("locking");
      isLocked = true;
      WarningLEDState = LOW;
      errorLEDState = LOW;
      digitalWrite(WARNING_LED_PIN , LOW);
      digitalWrite(ERROR_LED_PIN , LOW);
    }


 }

void unlock(){
  if(isLocked){
    Serial.println("unlocking");
    isLocked = false;
    errorLEDState = LOW;
    digitalWrite(ERROR_LED_PIN, errorLEDState);
  }
}

void printDistanceOnLCD(double distance){
  if(isLocked){
    lcd.setCursor(0,0);
    lcd.print("!!!obstacle!!!");
    lcd.setCursor(0,1);
    lcd.print("Press to unlock.   ");
  }
  else if(lcdMode == LCD_MODE_DISTANCE){
    lcd.setCursor( 0 , 0);
    lcd.print("Dis: ");
    if(distanceUnit == DISTANCE_UNIT_IN){
      lcd.print(distance * CM_TO_INCHES);
      lcd.print(" in     ");

    }
    else{
      lcd.print(distance);
      lcd.print(" cm   ");

    }

    lcd.setCursor( 0,1);
    if(distance > WARNING_DISTANCE){
      lcd.print(" No Obstacle   ");
    }
    else{
      lcd.print("!! WARNING !!");
    }
  }
}

void toggleDitanceUnit(){
  if(distanceUnit == DISTANCE_UNIT_CM){
    distanceUnit = DISTANCE_UNIT_IN;

  }
  else{
    distanceUnit = DISTANCE_UNIT_CM;
  }
  EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT , distanceUnit);
}

void toggleLCDScreen(bool next){
  switch(lcdMode){
    case LCD_MODE_DISTANCE:{
      lcdMode =(next) ? LCD_MODE_SETTINGS : LCD_MODE_LUMINOSITY ;
      break;
    }
    case LCD_MODE_SETTINGS:{
      lcdMode = (next) ? LCD_MODE_LUMINOSITY : LCD_MODE_DISTANCE;
      break;
    }
    case LCD_MODE_LUMINOSITY:{
      lcdMode = (next) ? LCD_MODE_DISTANCE : LCD_MODE_SETTINGS;
      break;
    }
    default:{
      lcdMode = LCD_MODE_DISTANCE;
    }
  }

  lcd.clear();
  if(lcdMode == LCD_MODE_SETTINGS){  //  show settings hint when entering settings
    lcd.setCursor(0,0);
    lcd.print(" Press OFF to  ");
    lcd.setCursor(0,1);
    lcd.print("reset Settings.");
  }
}

void resetSettingsToDefault(){
  if(lcdMode == LCD_MODE_SETTINGS){
    distanceUnit = DISTANCE_UNIT_CM;
    EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT , distanceUnit);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Settings have");
    lcd.setCursor(0,1);
    lcd.print("been reset");
  }
}

void handleIRCommand(long command){
  switch(command){
    case IR_BUTTON_PLAY:{
      unlock();
      break;
    }
    case IR_BUTTON_OFF:{
      resetSettingsToDefault();

      break;
    }
    case IR_BUTTON_EQ:{
      toggleDitanceUnit();
      break;
    }
    case IR_BUTTON_UP:{
      toggleLCDScreen(true);
      break;
    }
    case IR_BUTTON_DOWN:{
      toggleLCDScreen(false);
      break;
    } 
    default:{
      // do nothing
    }
  }
}

void setLightLEDFromLuminosity(int luminosity){
  byte brightness = 255 - luminosity / 4;
  analogWrite(LIGHT_LED_PIN , brightness);
}

void printLuminosityOnLcd(int luminosity){
  if(!isLocked && lcdMode == LCD_MODE_LUMINOSITY){
    lcd.setCursor(0,0);
    lcd.print("luminosity:  ");
    lcd.print(luminosity);
    lcd.print("     ");
  }
}


void setup() {
  Serial.begin(115200);
  lcd.begin(16,2);
  IrReceiver.begin(IR_RECIEVE_PIN);
  pinMode(ECHO_PIN , INPUT);
  pinMode(TRIGGER_PIN , OUTPUT);
  pinMode(WARNING_LED_PIN , OUTPUT);
  pinMode(ERROR_LED_PIN , OUTPUT);
  pinMode(LIGHT_LED_PIN , OUTPUT);
  pinMode(BUTTON_PIN , INPUT);
  

  buttonState = digitalRead(BUTTON_PIN);
  attachInterrupt(digitalPinToInterrupt(ECHO_PIN),echoPinInterrupt , CHANGE);
  
  distanceUnit = EEPROM.read(EEPROM_ADDRESS_DISTANCE_UNIT);
  if(distanceUnit == 255){
    distanceUnit = DISTANCE_UNIT_CM;
  }


  lcd.print("Initializing...");
  delay(1000);
  lcd.clear();
}



void loop() {
  unsigned long timeNow = millis();

  //  IR handled always, not just when locked
  if(IrReceiver.decode()){
    IrReceiver.resume();
    long command = IrReceiver.decodedIRData.command;
    handleIRCommand(command);
  }

  if(isLocked){
    if(timeNow - lastTimeErrorLEDBlinked > ErrorLEDDelay){
      lastTimeErrorLEDBlinked += ErrorLEDDelay;
      toggleErrorLED();
      toggleWarningLED();
    }

    if(timeNow - lastTimeButtonChanged > buttonDebounceDelay ){
      byte newButtonState = digitalRead(BUTTON_PIN);
      if(newButtonState != buttonState){
        lastTimeButtonChanged = timeNow;
        buttonState = newButtonState;
        if(buttonState == LOW){
          unlock();
        }
      }
    }
  }
  else{
    if(timeNow - lastTimeWarningLEDBlinked > WarningLEDDelay){
      lastTimeWarningLEDBlinked += WarningLEDDelay;
      toggleWarningLED();
    }
  }

  if(timeNow - lastTimeUltrasonicTrigger > untrasonicTriggerDelay ){
    lastTimeUltrasonicTrigger += untrasonicTriggerDelay;
    triggerUltrasonicSensor();
  }

  if(newDidtanceAvailable){
    newDidtanceAvailable = false;
    double distance = getUltrasonicDistance();
    setWarningLEDBlinkRateFromDistance(distance);
    printDistanceOnLCD(distance);
    if(distance < LOCK_DISTANCE){
      lock();
    }
  }
  if(timeNow - lastTimeReadLuminosity > readLuminosityDelay){
    lastTimeReadLuminosity += readLuminosityDelay;
    int luminosity = analogRead(PHOTORESISTOR_PIN);
    setLightLEDFromLuminosity(luminosity);
    printLuminosityOnLcd(luminosity);

  }
}





// #include <LiquidCrystal.h>
// #include <IRremote.h>
// #include <EEPROM.h>

// #define IR_RECIEVE_PIN 5

// #define ECHO_PIN 3
// #define TRIGGER_PIN 4

// #define WARNING_LED_PIN 11
// #define ERROR_LED_PIN 12
// #define LIGHT_LED_PIN 10

// #define BUTTON_PIN 2

// #define PHOTORESISTOR_PIN A0

// // LCD
// #define LCD_RS_PIN A5
// #define LCD_E_PIN A4
// #define LCD_D4_PIN 6
// #define LCD_D5_PIN 7
// #define LCD_D6_PIN 8
// #define LCD_D7_PIN 9

// LiquidCrystal lcd(LCD_RS_PIN, LCD_E_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

// #define LOCK_DISTANCE 10.0
// #define WARNING_DISTANCE 50.0

// // IR BUTTONS
// #define IR_BUTTON_PLAY 28
// #define IR_BUTTON_OFF  22
// #define IR_BUTTON_EQ   13
// #define IR_BUTTON_UP   24
// #define IR_BUTTON_DOWN 82

// // Distance unit
// #define DISTANCE_UNIT_CM 0
// #define DISTANCE_UNIT_IN 1
// #define CM_TO_INCHES 0.393701
// #define EEPROM_ADDRESS_DISTANCE_UNIT 50

// // LCD mode
// #define LCD_MODE_DISTANCE  0
// #define LCD_MODE_SETTINGS  1
// #define LCD_MODE_LUMINOSITY 2

// // Ultrasonic
// unsigned long lastTimeUltrasonicTrigger = 0;
// unsigned long ultrasonicTriggerDelay = 60;

// volatile unsigned long pulseInTimeBegin = 0;
// volatile unsigned long pulseTimeEnd = 0;
// volatile bool newDistanceAvailable = false;

// double previousDistance = 400.0;

// // Warning LED
// unsigned long lastTimeWarningLEDBlinked = 0;
// unsigned long warningLEDDelay = 500;
// byte warningLEDState = LOW;

// // Error LED
// unsigned long lastTimeErrorLEDBlinked = 0;
// unsigned long errorLEDDelay = 300;
// byte errorLEDState = LOW;

// // Photoresistor
// unsigned long lastTimeReadLuminosity = 0;
// unsigned long readLuminosityDelay = 100;

// // Push button
// unsigned long lastTimeButtonChanged = 0;
// unsigned long buttonDebounceDelay = 50;
// byte buttonState;

// bool isLocked = false;

// int distanceUnit = DISTANCE_UNIT_CM;
// int lcdMode = LCD_MODE_DISTANCE;

// // ─────────────────────────────────────────────
// // Ultrasonic
// // ─────────────────────────────────────────────

// void triggerUltrasonicSensor() {
//   digitalWrite(TRIGGER_PIN, LOW);
//   delayMicroseconds(2);
//   digitalWrite(TRIGGER_PIN, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(TRIGGER_PIN, LOW);
// }

// double getUltrasonicDistance() {
//   double durationMicros = pulseTimeEnd - pulseInTimeBegin;
//   double distance = durationMicros / 58.0;
//   if (distance > 400) {
//     return previousDistance;
//   }
//   distance = previousDistance * 0.6 + distance * 0.4;
//   previousDistance = distance;
//   return distance;
// }

// void echoPinInterrupt() {
//   if (digitalRead(ECHO_PIN) == HIGH) {
//     pulseInTimeBegin = micros();
//   } else {
//     pulseTimeEnd = micros();
//     newDistanceAvailable = true;
//   }
// }

// // ─────────────────────────────────────────────
// // LEDs
// // ─────────────────────────────────────────────

// void toggleErrorLED() {
//   errorLEDState = (errorLEDState == HIGH) ? LOW : HIGH;
//   digitalWrite(ERROR_LED_PIN, errorLEDState);
// }

// void toggleWarningLED() {
//   warningLEDState = (warningLEDState == HIGH) ? LOW : HIGH;
//   digitalWrite(WARNING_LED_PIN, warningLEDState);
// }

// void setWarningLEDBlinkRateFromDistance(double distance) {
//   // 0…400 cm → 0…1600 ms
//   warningLEDDelay = (unsigned long)(distance * 4);
// }

// // ─────────────────────────────────────────────
// // Lock / Unlock
// // ─────────────────────────────────────────────

// void lock() {
//   if (!isLocked) {
//     Serial.println("locking");
//     isLocked = true;
//     warningLEDState = LOW;
//     digitalWrite(WARNING_LED_PIN, LOW);   // FIX: actually turn the LED off
//     errorLEDState = LOW;
//     digitalWrite(ERROR_LED_PIN, LOW);
//   }
// }

// void unlock() {
//   if (isLocked) {
//     Serial.println("unlocking");
//     isLocked = false;
//     errorLEDState = LOW;
//     digitalWrite(ERROR_LED_PIN, LOW);
//     lcd.clear();
//   }
// }

// // ─────────────────────────────────────────────
// // LCD
// // ─────────────────────────────────────────────

// void printDistanceOnLCD(double distance) {
//   if (isLocked) {
//     lcd.setCursor(0, 0);
//     lcd.print("!!!obstacle!!!! ");
//     lcd.setCursor(0, 1);
//     lcd.print("Press to unlock.");
//   } else if (lcdMode == LCD_MODE_DISTANCE) {
//     lcd.setCursor(0, 0);
//     lcd.print("Dis: ");
//     if (distanceUnit == DISTANCE_UNIT_IN) {
//       lcd.print(distance * CM_TO_INCHES);
//       lcd.print(" in     ");
//     } else {
//       lcd.print(distance);
//       lcd.print(" cm   ");
//     }

//     lcd.setCursor(0, 1);
//     if (distance > WARNING_DISTANCE) {
//       lcd.print(" No Obstacle   ");
//     } else {
//       lcd.print("!! WARNING !!   ");
//     }
//   }
// }

// void printLuminosityOnLcd(int luminosity) {
//   // FIX: show luminosity when NOT locked (was checking isLocked instead of !isLocked)
//   if (!isLocked && lcdMode == LCD_MODE_LUMINOSITY) {
//     lcd.setCursor(0, 0);
//     lcd.print("Luminosity:     ");
//     lcd.setCursor(0, 1);
//     lcd.print(luminosity);
//     lcd.print("     ");
//   }
// }

// // ─────────────────────────────────────────────
// // Settings
// // ─────────────────────────────────────────────

// void toggleDistanceUnit() {
//   distanceUnit = (distanceUnit == DISTANCE_UNIT_CM) ? DISTANCE_UNIT_IN : DISTANCE_UNIT_CM;
//   EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT, distanceUnit);
// }

// void toggleLCDScreen(bool next) {
//   switch (lcdMode) {
//     case LCD_MODE_DISTANCE:
//       lcdMode = next ? LCD_MODE_SETTINGS : LCD_MODE_LUMINOSITY;
//       break;
//     case LCD_MODE_SETTINGS:
//       lcdMode = next ? LCD_MODE_LUMINOSITY : LCD_MODE_DISTANCE;
//       break;
//     case LCD_MODE_LUMINOSITY:
//       lcdMode = next ? LCD_MODE_DISTANCE : LCD_MODE_SETTINGS;
//       break;
//     default:
//       lcdMode = LCD_MODE_DISTANCE;
//   }

//   lcd.clear();
//   if (lcdMode == LCD_MODE_SETTINGS) {
//     lcd.setCursor(0, 0);
//     lcd.print(" Press OFF to  ");
//     lcd.setCursor(0, 1);
//     lcd.print("reset Settings.");
//   }
// }

// void resetSettingsToDefault() {
//   if (lcdMode == LCD_MODE_SETTINGS) {
//     distanceUnit = DISTANCE_UNIT_CM;
//     EEPROM.write(EEPROM_ADDRESS_DISTANCE_UNIT, distanceUnit);
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Settings have");
//     lcd.setCursor(0, 1);
//     lcd.print("been reset");
//   }
// }

// // ─────────────────────────────────────────────
// // IR remote
// // ─────────────────────────────────────────────

// void handleIRCommand(long command) {
//   switch (command) {
//     case IR_BUTTON_PLAY:  unlock();                  break;
//     case IR_BUTTON_OFF:   resetSettingsToDefault();  break;
//     case IR_BUTTON_EQ:    toggleDistanceUnit();      break;
//     case IR_BUTTON_UP:    toggleLCDScreen(true);     break;
//     case IR_BUTTON_DOWN:  toggleLCDScreen(false);    break;
//     default: break;
//   }
// }

// // ─────────────────────────────────────────────
// // Light LED (photoresistor-controlled)
// // ─────────────────────────────────────────────

// void setLightLEDFromLuminosity(int luminosity) {
//   byte brightness = 255 - (luminosity / 4);
//   analogWrite(LIGHT_LED_PIN, brightness);  // FIX: was analogRead() — must be analogWrite()
// }

// // ─────────────────────────────────────────────
// // Setup
// // ─────────────────────────────────────────────

// void setup() {
//   Serial.begin(115200);
//   lcd.begin(16, 2);
//   IrReceiver.begin(IR_RECIEVE_PIN);

//   // FIX: set pin modes BEFORE reading initial button state
//   pinMode(ECHO_PIN, INPUT);
//   pinMode(TRIGGER_PIN, OUTPUT);
//   pinMode(WARNING_LED_PIN, OUTPUT);
//   pinMode(ERROR_LED_PIN, OUTPUT);
//   pinMode(LIGHT_LED_PIN, OUTPUT);
//   pinMode(BUTTON_PIN, INPUT_PULLUP);  // FIX: INPUT_PULLUP for reliable button reading

//   buttonState = digitalRead(BUTTON_PIN);  // now safe to read

//   attachInterrupt(digitalPinToInterrupt(ECHO_PIN), echoPinInterrupt, CHANGE);

//   distanceUnit = EEPROM.read(EEPROM_ADDRESS_DISTANCE_UNIT);
//   if (distanceUnit == 255) {
//     distanceUnit = DISTANCE_UNIT_CM;
//   }

//   // Initialise timing variables after millis() is available
//   lastTimeUltrasonicTrigger = millis();
//   lastTimeWarningLEDBlinked = millis();
//   lastTimeErrorLEDBlinked   = millis();
//   lastTimeReadLuminosity    = millis();
//   lastTimeButtonChanged     = millis();

//   lcd.print("Initializing...");
//   delay(1000);
//   lcd.clear();
// }

// // ─────────────────────────────────────────────
// // Loop
// // ─────────────────────────────────────────────

// void loop() {
//   unsigned long timeNow = millis();

//   // IR — handled always, not just when locked
//   if (IrReceiver.decode()) {
//     IrReceiver.resume();
//     long command = IrReceiver.decodedIRData.command;
//     handleIRCommand(command);
//   }

//   if (isLocked) {
//     // Blink both LEDs together while locked
//     if (timeNow - lastTimeErrorLEDBlinked > errorLEDDelay) {
//       lastTimeErrorLEDBlinked += errorLEDDelay;
//       toggleErrorLED();
//       toggleWarningLED();
//     }

//     // Button debounce — FIX: update lastTimeButtonChanged on every state sample
//     if (timeNow - lastTimeButtonChanged > buttonDebounceDelay) {
//       lastTimeButtonChanged = timeNow;  // FIX: was never updated → button fired only once
//       byte newButtonState = digitalRead(BUTTON_PIN);
//       if (newButtonState != buttonState) {
//         buttonState = newButtonState;
//         // FIX: with INPUT_PULLUP, pressed = LOW
//         if (buttonState == LOW) {
//           unlock();
//         }
//       }
//     }
//   } else {
//     // Blink warning LED at distance-based rate
//     if (timeNow - lastTimeWarningLEDBlinked > warningLEDDelay) {
//       lastTimeWarningLEDBlinked += warningLEDDelay;
//       toggleWarningLED();
//     }
//   }

//   // Trigger ultrasonic sensor periodically
//   if (timeNow - lastTimeUltrasonicTrigger > ultrasonicTriggerDelay) {
//     lastTimeUltrasonicTrigger += ultrasonicTriggerDelay;
//     triggerUltrasonicSensor();
//   }

//   // Process new distance reading
//   if (newDistanceAvailable) {
//     newDistanceAvailable = false;
//     double distance = getUltrasonicDistance();
//     setWarningLEDBlinkRateFromDistance(distance);
//     printDistanceOnLCD(distance);
//     if (distance < LOCK_DISTANCE) {
//       lock();
//     }
//   }

//   // Read and apply luminosity
//   if (timeNow - lastTimeReadLuminosity > readLuminosityDelay) {
//     lastTimeReadLuminosity += readLuminosityDelay;
//     int luminosity = analogRead(PHOTORESISTOR_PIN);
//     setLightLEDFromLuminosity(luminosity);
//     printLuminosityOnLcd(luminosity);
//   }
// }





