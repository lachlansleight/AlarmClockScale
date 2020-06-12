#define DEBOUNCE_TIME 50

#define ENCODER_RIGHT 0
#define ENCODER_LEFT 1
#define BUTTON_R_DOWN 2
#define BUTTON_R_UP 3
#define BUTTON_A_DOWN 4
#define BUTTON_A_UP 5
#define BUTTON_B_DOWN 6
#define BUTTON_B_UP 7

#define PIN_ROT_A 6
#define PIN_ROT_B 5
#define PIN_BTN_R 7
#define PIN_BTN_A 8
#define PIN_BTN_B 9

#define PIN_BUZZER 4

#include <SimpleRotary.h>
#include <ButtonDebounce.h>

ButtonDebounce btnR(PIN_BTN_R, DEBOUNCE_TIME);
ButtonDebounce btnA(PIN_BTN_A, DEBOUNCE_TIME);
ButtonDebounce btnB(PIN_BTN_B, DEBOUNCE_TIME);
SimpleRotary encoder(PIN_ROT_A, PIN_ROT_B, -1);

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

long onTime;
long offTime;
long triggeredTime;
int pitch;

const char beepModeNames[][16] = {
    "Alarm",
    "Chime",
    "Pips",
    "Random",
    "Escalating"
};

#define PATTERN_COUNT 5

#define PATTERN_ALARM 0
#define PATTERN_CHIME 1
#define PATTERN_PIPS 2
#define PATTERN_RANDOM 3
#define PATTERN_ESCALATING 4

#define RESONANT_PITCH 2650

byte pattern = 0;
byte beepSequence = 0;
boolean beep = false;

void setup()
{
  Serial.begin(9600);
  
  btnR.setCallback(buttonChangedR);
  btnA.setCallback(buttonChangedA);
  btnB.setCallback(buttonChangedB);

  lcd.init();
  lcd.backlight();
}


void loop()
{
  updateInputs();
  
  if(beep) {
    alarmBeep();
  }
}

void handleInput(int event) {
#ifdef ENABLE_SERIAL
  Serial.println("Input Event: " + String(event));
#endif

  switch(event) {
    case ENCODER_RIGHT:
      encoderLeft();
      break;
    case ENCODER_LEFT:
      encoderRight();
      break;
    case BUTTON_R_DOWN:
      buttonEncoder(true);
      break;
    case BUTTON_A_DOWN:
      buttonA();
      break;
    case BUTTON_B_DOWN:
      buttonB();
      break;
    case BUTTON_R_UP:
      buttonEncoder(false);
      break;
    case BUTTON_A_UP:
      break;
    case BUTTON_B_UP:
      break;
  }
}

void updateInputs()
{
    btnR.update();
    btnA.update();
    btnB.update();

    int i = encoder.rotate();
    if(i == 1) handleInput(ENCODER_LEFT);
    else if(i == 2) handleInput(ENCODER_RIGHT);
}
void buttonChangedR(int state) { 
    handleInput(state == 0 ? BUTTON_R_UP : BUTTON_R_DOWN);
}
void buttonChangedA(int state) { 
    handleInput(state == 0 ? BUTTON_A_UP : BUTTON_A_DOWN);
}
void buttonChangedB(int state) { 
    handleInput(state == 0 ? BUTTON_B_UP : BUTTON_B_DOWN);
}



void encoderRight()
{
    updateValue(1);
}

void encoderLeft()
{
  updateValue(-1);
}

void updateValue(int offset)
{
  if(pattern == 0 && offset < 0) return;
  if(pattern == 4 && offset > 4) return;
  pattern += offset;
  lcdPrintCenter(beepModeNames[pattern], 0);
}

void buttonEncoder(boolean val)
{
  beep = val;
  if(!beep) {
    noTone(PIN_BUZZER);
  }
  else {
    triggeredTime = millis();
    onTime = millis();
    offTime = -1;
  }
}

void buttonA()
{
}

void buttonB()
{
}














void alarmBeep()
{
    switch(pattern) {
        case PATTERN_ALARM:
            if(onTime > 0 && onTime < millis()) {
              offTime = millis() + 100;
              onTime = -1;
              tone(PIN_BUZZER, RESONANT_PITCH);
            } else if(offTime > 0 && offTime < millis()) {
              onTime = millis() + 100;
              offTime = -1;            
              noTone(PIN_BUZZER);
            }
          break;
        case PATTERN_CHIME:
          tone(PIN_BUZZER, RESONANT_PITCH + ((millis() % 1000) / 200) - 25);
          break;
        case PATTERN_PIPS:
            if(onTime > 0 && onTime < millis()) {              
              offTime = millis() + 50;
              onTime = -1;
              tone(PIN_BUZZER, RESONANT_PITCH);
            } else if(offTime > 0 && offTime < millis()) {
              if(beepSequence == 0) onTime = millis() + 400;
              else if(beepSequence == 1) onTime = millis() + 400;
              else if(beepSequence == 2) onTime = millis() + 400;
              else if(beepSequence == 3) onTime = millis() + 100;
              else if(beepSequence == 4) onTime = millis() + 100;
              beepSequence++;
              if(beepSequence >= 5) beepSequence = 0;
              offTime = -1;
              noTone(PIN_BUZZER);
            }
          break;
        case PATTERN_RANDOM:
            if(onTime > 0 && onTime < millis()) {
              offTime = millis() + random(20, 500);
              onTime = -1;
              tone(PIN_BUZZER, RESONANT_PITCH);             
            } else if(offTime > 0 && offTime < millis()) {
              onTime = millis() + random(20, 500);
              offTime = -1;
              noTone(PIN_BUZZER);              
            }
            break;
        case PATTERN_ESCALATING:
            //Starts out as 50ms on, 2150ms off - after 30 seconds is 200ms on, 50ms off
            int timeSinceLerp = ((millis() - triggeredTime) / 1000);
            if(timeSinceLerp > 30) timeSinceLerp = 30;
            if(onTime > 0 && onTime < millis()) {
              offTime = millis() + (50 + (5 * timeSinceLerp));
              onTime = -1;
              tone(PIN_BUZZER, RESONANT_PITCH);             
            } else if(offTime > 0 && offTime < millis()) {
              onTime = millis() + 150 + (2000 - 70 * timeSinceLerp);
              offTime = -1;
              noTone(PIN_BUZZER);              
            }
            break;
    }
}















//=================================
//            LCD STUFF
//=================================
void lcdPrint(String value)
{
    lcdPrint(value, 0);
}
void lcdPrint(String value, int row)
{
  if(value.length() > 16) {
    value = value.substring(16);
  }
  while(value.length() < 16) {
    value += " ";
  }
  lcd.setCursor(0, row);
  lcd.print(value);
}

void lcdPrintCenter(String value)
{
    lcdPrintCenter(value, 0);
}
void lcdPrintCenter(String value, int row)
{
  if(value.length() > 16) {
    value = value.substring(16);
  }

  int spaceCount = 16 - value.length();
  while(spaceCount > 0) {
    if(spaceCount % 2 == 0) value = value + " ";
    else value = " " + value;
    spaceCount--;
  }
  lcd.setCursor(0, row);
  lcd.print(value);
}
