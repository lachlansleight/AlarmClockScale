//=============================================
//           ALARM CLOCK SCALE FIRMWARE       
//                    v2.2.1                
//=============================================
#define VERSION "v2.2.1"

#define VARIANT_PCB 1
//#define VARIANT_LEGACY 1

//=================================
//            INPUT STUFF
//=================================

#include <SimpleRotary.h>
#include <ButtonDebounce.h>

#ifdef VARIANT_PCB
//PCB Pins
#define PIN_ROT_A 6
#define PIN_ROT_B 5
#define PIN_BTN_R 7
#define PIN_BTN_A 8
#define PIN_BTN_B 9
#endif

#ifdef VARIANT_LEGACY
//Legacy Pins
#define PIN_ROT_A A1
#define PIN_ROT_B A0
#define PIN_BTN_R 14
#define PIN_BTN_A 16
#define PIN_BTN_B 10
#endif

#define DEBOUNCE_TIME 50

#define ENCODER_RIGHT 0
#define ENCODER_LEFT 1
#define BUTTON_R_DOWN 2
#define BUTTON_R_UP 3
#define BUTTON_A_DOWN 4
#define BUTTON_A_UP 5
#define BUTTON_B_DOWN 6
#define BUTTON_B_UP 7

ButtonDebounce btnR(PIN_BTN_R, DEBOUNCE_TIME);
ButtonDebounce btnA(PIN_BTN_A, DEBOUNCE_TIME);
ButtonDebounce btnB(PIN_BTN_B, DEBOUNCE_TIME);
SimpleRotary encoder(PIN_ROT_A, PIN_ROT_B, -1);

//=================================
//            CLOCK STUFF
//=================================
#include <DS1302.h>

#ifdef VARIANT_PCB
#define PIN_RTC_DAT 14
#define PIN_RTC_CLK 15
#define PIN_RTC_RST A0
#endif

#ifdef VARIANT_LEGACY
#define PIN_RTC_DAT 8
#define PIN_RTC_CLK 7
#define PIN_RTC_RST 9
#endif

DS1302 rtc(PIN_RTC_RST, PIN_RTC_DAT, PIN_RTC_CLK);
int timestamp;
int lastTimestamp;
String currentTimeString;
byte currentHour;
byte currentMinute;


//=================================
//            SCALE STUFF
//=================================
#include <HX711.h>

#ifdef VARIANT_PCB
#define PIN_SCL_DAT 10
#define PIN_SCL_SCK 16
#endif

#ifdef VARIANT_LEGACY
#define PIN_SCL_DAT 4
#define PIN_SCL_SCK 5
#endif

#define SCALE_DISPLAY_THRESHOLD 10

int scaleCalibrationFactor;
int scaleCalibrationDefault = 20000;

HX711 scale;
float scaleReading;
String scaleReadingString;

float calibrationWeight;
float calibrationWeightDefault = 60.0;


//=================================
//            ALARM STUFF
//=================================
#ifdef VARIANT_PCB
#define PIN_BUZZER 4
#endif

#ifdef VARIANT_LEGACY
#define PIN_BUZZER 6
#endif

#define PATTERN_COUNT 5
#define RESONANT_PITCH 2650

#define PATTERN_ALARM 0
#define PATTERN_WAIL 1
#define PATTERN_PIPS 2
#define PATTERN_RANDOM 3
#define PATTERN_ESCALATING 4

boolean alarmTriggeredA = false;
boolean alarmTriggeredB = false;

long alarmTriggeredTime = 0;
long disarmCounter = 0;
long toleranceCounter = 0;

long onTime = 0;
long offTime = 0;
byte beepSequence = 0;

const char beepModeNames[][16] = {
    "Alarm",
    "Wail",
    "Pips",
    "Random",
    "Escalating"
};


//=================================
//            LCD STUFF
//=================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);


//=================================
//            EEPROM STUFF
//=================================
#include <EEPROM.h>

#define ADDR_DEFAULTS 0
#define ADDR_ALARM_ARMED_A 1
#define ADDR_ALARM_HOUR_A 2
#define ADDR_ALARM_MINUTE_A 3
#define ADDR_ALARM_ARMED_B 4
#define ADDR_ALARM_HOUR_B 5
#define ADDR_ALARM_MINUTE_B 6
#define ADDR_DISARM_DURATION 7
#define ADDR_DISARM_TOLERANCE 8
#define ADDR_DISARM_REQUIREMENT 9
#define ADDR_BEEP_PATTERN 10
#define ADDR_CALIBRATION_LOW 11
#define ADDR_CALIBRATION_HIGH 12
#define ADDR_CALIBRATION_WEIGHT_INT 13
#define ADDR_CALIBRATION_WEIGHT_DEC 14

//=================================
//            DATA STUFF
//=================================
#define ALARM_ARMED_A 0
#define ALARM_HOUR_A 1
#define ALARM_MINUTE_A 2
#define ALARM_ARMED_B 3
#define ALARM_HOUR_B 4
#define ALARM_MINUTE_B 5
#define DISARM_DURATION 6
#define DISARM_TOLERANCE 7
#define DISARM_REQUIREMENT 8
#define BEEP_PATTERN 9

#define MENUPOS_ALARM_ARMED_A 0
#define MENUPOS_ALARM_TIME_A 1
#define MENUPOS_ALARM_ARMED_B 2
#define MENUPOS_ALARM_TIME_B 3
#define MENUPOS_TIME 4
#define MENUPOS_DISARM_DURATION 5
#define MENUPOS_DISARM_TOLERANCE 6
#define MENUPOS_DISARM_REQUIREMENT 7
#define MENUPOS_BEEP_PATTERN 8
#define MENUPOS_CALIBRATION 9

#define TIMEPOS_APPLY 0
#define TIMEPOS_HOUR 1
#define TIMEPOS_MINUTE 2
#define TIMEPOS_AMPM 3

#define VAL_COUNT 10
byte curVal[VAL_COUNT]; //loaded from EEPROM during setup, or set to defaults if EEPROM not initialized
byte defaultVal[] = {
    false, 7, 0,    //alarm A disarmed for 7am
    false, 8, 0,    //alarm B disarmed for 8am
    240,            //240 second disarm duration
    10,             //10 second disarm tolerance
    50,             //50kg alarm disarm requirement
    PATTERN_WAIL   //'chime' pattern
};
//if the user edits the time, we stop actually using the real clock to update it, instead
//when editing starts we set these to the current time and use them in the menu until the new time is applied


//=================================
//            MENU STUFF
//=================================
#define MENU_COUNT 10

boolean menuShowing = false;
boolean menuEditing = false;
boolean timeEditing = false;

byte menuPosition = MENUPOS_ALARM_ARMED_A;
byte timePosition = TIMEPOS_APPLY;

const char menuNames[][16] = {
    "Alarm A Armed",
    "Alarm A Time",
    "Alarm B Armed",
    "Alarm B Time",
    "Set Time",
    "Disarm Dur.",
    "Disarm Tol.",
    "Disarm Req.",
    "Beep Pattern",
    "Calibration"
};

const byte menuValIndices[] = {
    0, 0, 3, 0, 0, 6, 7, 8, 9, 0
};


//=================================
//             STATE
//=================================
long deltaMillis;
long lastMillis;

//these are for storing a property's current value for editing, applied only when the user says
byte editVal;
byte editHour;
byte editMinute;
float editValFloat;



//=================================
//             DEBUG
//=================================
//#define ENABLE_SERIAL 1
//#define FORCE_VALUE_DEFAULTS 1
//#define FORCE_SERIAL 1
//#define DUMP_EEPROM 1

//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================









void setup() 
{
#ifdef ENABLE_SERIAL
    Serial.begin(9600);
#ifdef FORCE_SERIAL 
    while(!Serial) { ; }
#endif

    Serial.println("==============================");
    Serial.println("Initializing Alarm Clock Scale");

    dumpEEPROM();

    Serial.println("  Loading data from EEPROM...");
#endif

    //EEPROM Stuff
    loadDataFromEEPROM();

#ifdef ENABLE_SERIAL
    Serial.print("  Initializing Input...");
#endif

    //Input Stuff
    btnR.setCallback(buttonChangedR);
    btnA.setCallback(buttonChangedA);
    btnB.setCallback(buttonChangedB);

#ifdef ENABLE_SERIAL
    Serial.println("Done");
    Serial.print("  Initializing Scale...");
#endif

    //Scale Stuff
    scale.begin(PIN_SCL_DAT, PIN_SCL_SCK);
    scale.set_scale(scaleCalibrationFactor);

#ifdef ENABLE_SERIAL
    Serial.println("Done");
    Serial.print("  Initializing Display...");
#endif

    //Display Stuff
    lcd.init();
    lcd.backlight();

#ifdef ENABLE_SERIAL
    Serial.println("Done");
#endif


    //Reset all values if the user is holding down A and B buttons during startup
    if(digitalRead(PIN_BTN_B) && digitalRead(PIN_BTN_A)) {
        #ifdef ENABLE_SERIAL
        Serial.println("Resetting all values to default");
        #endif

        lcdPrintCenter(F("Resetting all"), 0);
        lcdPrintCenter(F("data to default"), 1);
        //reset
        EEPROM.write(ADDR_DEFAULTS, 100);
        loadDataFromEEPROM();
        delay(2000);
    }

#ifdef ENABLE_SERIAL
    Serial.print("  Displaying Welcome...");
#endif

    //Welcome message and tare
    lcdPrintCenter(F("AlarmClockScale"), 0);
    lcdPrintCenter(F(VERSION), 1);
    scale.tare();
    delay(2000);
    lcd.clear();
    lcd.noBacklight();
    lastTimestamp = 9999;
    updateTime();

#ifdef ENABLE_SERIAL
    Serial.println("Done");
    Serial.println("");
    Serial.println("Initialization Complete");
    Serial.println("==============================");
#endif
}

void loadDataFromEEPROM() 
{
    #ifdef FORCE_VALUE_DEFAULTS
    byte forceDefaults = 100;
    #else
    byte forceDefaults = EEPROM.read(ADDR_DEFAULTS);
    #endif

    //101 is an arbitrary marker that I hope won't be there by default!
    if(forceDefaults != 101) {
        //Indicates that this is the first time the scale has been turned on, or EEPROM data is otherwise corrupted
        //So load default values

        for(int i = 0; i < VAL_COUNT; i++) {
            curVal[i] = defaultVal[i];
        }
        scaleCalibrationFactor = scaleCalibrationDefault;
        calibrationWeight = calibrationWeightDefault;

        EEPROM.write(ADDR_DEFAULTS, 101);
    } else {
        curVal[ALARM_ARMED_A] = EEPROM.read(ADDR_ALARM_ARMED_A);
        curVal[ALARM_HOUR_A] = EEPROM.read(ADDR_ALARM_HOUR_A);
        curVal[ALARM_MINUTE_A] = EEPROM.read(ADDR_ALARM_MINUTE_A);
        curVal[ALARM_ARMED_B] = EEPROM.read(ADDR_ALARM_ARMED_B);
        curVal[ALARM_HOUR_B] = EEPROM.read(ADDR_ALARM_HOUR_B);
        curVal[ALARM_MINUTE_B] = EEPROM.read(ADDR_ALARM_MINUTE_B);

        curVal[DISARM_DURATION] = EEPROM.read(ADDR_DISARM_DURATION);
        curVal[DISARM_TOLERANCE] = EEPROM.read(ADDR_DISARM_TOLERANCE);
        curVal[DISARM_REQUIREMENT] = EEPROM.read(ADDR_DISARM_REQUIREMENT);
        curVal[BEEP_PATTERN] = EEPROM.read(ADDR_BEEP_PATTERN);

        scaleCalibrationFactor = (EEPROM.read(ADDR_CALIBRATION_HIGH) << 8) | EEPROM.read(ADDR_CALIBRATION_LOW);
        calibrationWeight = (float)EEPROM.read(ADDR_CALIBRATION_WEIGHT_INT) + ((float)EEPROM.read(ADDR_CALIBRATION_WEIGHT_DEC)) * 0.01;

#ifdef ENABLE_SERIAL
        Serial.println("  ALARM_ARMED_A: " + String(curVal[ALARM_ARMED_A]));
        Serial.println("  ALARM_HOUR_A: " + String(curVal[ALARM_HOUR_A]));
        Serial.println("  ALARM_MINUTE_A: " + String(curVal[ALARM_MINUTE_A]));
        Serial.println("  ALARM_ARMED_B: " + String(curVal[ALARM_ARMED_B]));
        Serial.println("  ALARM_HOUR_B: " + String(curVal[ALARM_HOUR_B]));
        Serial.println("  ALARM_MINUTE_B: " + String(curVal[ALARM_MINUTE_B]));
        Serial.println("  DISARM_DURATION: " + String(curVal[DISARM_DURATION]));
        Serial.println("  DISARM_TOLERANCE: " + String(curVal[DISARM_TOLERANCE]));
        Serial.println("  DISARM_REQUIREMENT: " + String(curVal[DISARM_REQUIREMENT]));
        Serial.println("  BEEP_PATTERN: " + String(curVal[BEEP_PATTERN]));
        Serial.println("  CALIBRATION_FACTOR: " + String(scaleCalibrationFactor));
        Serial.println("  CALIBRATION_WEIGHT: " + String(calibrationWeight));
#endif
    }

    //Enforce minimum and maximum values, to ensure that default EEPROM values don't break things
    //If anything is out of range, we just set it to the default value
    if(curVal[ALARM_HOUR_A] > 23) 
        curVal[ALARM_HOUR_A] = defaultVal[ALARM_HOUR_A];
    if(curVal[ALARM_MINUTE_A] > 59) 
        curVal[ALARM_MINUTE_A] = defaultVal[ALARM_MINUTE_A];
    if(curVal[ALARM_HOUR_B] > 23) 
        curVal[ALARM_HOUR_B] = defaultVal[ALARM_HOUR_B];
    if(curVal[ALARM_MINUTE_B] > 59) 
        curVal[ALARM_MINUTE_B] = defaultVal[ALARM_MINUTE_B];
    if(curVal[DISARM_DURATION] < 10 || curVal[DISARM_DURATION] > 240) 
        curVal[DISARM_DURATION] = defaultVal[DISARM_DURATION];
    if(curVal[DISARM_TOLERANCE] < 10 || curVal[DISARM_TOLERANCE] > 120) 
        curVal[DISARM_TOLERANCE] = defaultVal[DISARM_TOLERANCE];
    if(curVal[DISARM_REQUIREMENT] < 5 || curVal[DISARM_REQUIREMENT] > 100) 
        curVal[DISARM_REQUIREMENT] = defaultVal[DISARM_REQUIREMENT];
    if(curVal[BEEP_PATTERN] > (PATTERN_COUNT - 1)) 
        curVal[BEEP_PATTERN] = defaultVal[BEEP_PATTERN];
}

void dumpEEPROM()
{
    Serial.println("EEPROM");
    Serial.println("----------EEPROM--------");
    for(int i = 0; i < 24; i++) {
        for(int j = 0; j < 6; j++) {
            if(j != 0) Serial.print("\t");

            byte val = EEPROM.read(i);
            String outString = String(val);
            if(val == 0) outString = "  0";
            else if(val < 10) outString = "  " + outString;
            else if(val < 100) outString = " " + outString;

            Serial.print(outString);
            if(j != 5) i++;
        }
        Serial.print("\n");
    }
    Serial.println("----------EEPROM--------");
}








//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================





void loop()
{
    //Get buttons and rotary encoder
    updateInputs();
    
    //Update time, check whether we need to trigger alarm
    updateTime();

    //Update scale readings and whatnot
    updateScale();

    //Play sounds and disarm etc
    alarmLoop();

    deltaMillis = millis() - lastMillis;
    lastMillis = millis();
    lastTimestamp = timestamp;
}










//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================

//=================================
//            MENU STUFF
//=================================
void displayMenu()
{
    if(!menuEditing) {
        //Display main navigation
        String nav = "|";
        for(int i = 0; i < MENU_COUNT; i++) {
            nav += i == menuPosition ? "O" : "-";
        }
        nav += "|";

        lcdPrintCenter(nav, 0);
        lcdPrintCenter(menuNames[menuPosition], 1);
        return;
    }

    if(menuPosition == MENUPOS_ALARM_TIME_A || menuPosition == MENUPOS_ALARM_TIME_B || menuPosition == MENUPOS_TIME) {
        //Special case for time editing options
        /*
        String timeString = "";
        if(menuPosition == MENUPOS_ALARM_TIME_A) timeString = getTimeString(curVal[ALARM_HOUR_A], curVal[ALARM_MINUTE_A]);
        else if(menuPosition == MENUPOS_ALARM_TIME_B) timeString = getTimeString(curVal[ALARM_HOUR_B], curVal[ALARM_MINUTE_B]);
        else timeString = getTimeString(editHour, editMinute);
        */
        String timeString = getTimeString(editHour, editMinute);


        #ifdef ENABLE_SERIAL
        Serial.println("Time string is " + timeString);
        #endif

        lcdPrint("Apply    " + timeString, 0);
        if(timeEditing) {
            lcdPrint(
                String((timePosition == TIMEPOS_APPLY ? "^^^^^" : "     ")) + String("    ") +
                String((timePosition == TIMEPOS_HOUR ? "^^" : "  ")) + String(" ") +
                String((timePosition == TIMEPOS_MINUTE ? "^^" : "  ")) +
                String((timePosition == TIMEPOS_AMPM ? "^^" : "  ")),
                1
            );
        } else {
            lcdPrint(
                String((timePosition == TIMEPOS_APPLY ? "---->" : "     ")) + String("    ") +
                String((timePosition == TIMEPOS_HOUR ? "<>" : "  ")) + String(" ") +
                String((timePosition == TIMEPOS_MINUTE ? "<>" : "  ")) +
                String((timePosition == TIMEPOS_AMPM ? "<-" : "  ")),
                1
            );
        }

        return;
    }

    lcdPrintCenter(menuNames[menuPosition], 0);
    switch(menuPosition) {
        case MENUPOS_ALARM_ARMED_A:
            lcdPrintCenter(editVal > 0 ? "ON" : "OFF", 1);
            break;
        case MENUPOS_ALARM_ARMED_B:
            lcdPrintCenter(editVal > 0 ? "ON" : "OFF", 1);
            break;
        case MENUPOS_DISARM_DURATION:
            lcdPrintCenter(String(editVal) + " seconds", 1);
            break;
        case MENUPOS_DISARM_TOLERANCE:
            lcdPrintCenter(String(editVal) + " seconds", 1);
            break;
        case MENUPOS_DISARM_REQUIREMENT:
            lcdPrintCenter(String(editVal) + " kg", 1);
            break;
        case MENUPOS_BEEP_PATTERN:
            lcdPrintCenter(beepModeNames[editVal], 1);
            break;
        case MENUPOS_CALIBRATION:
            lcdPrintCenter(String(editValFloat) + " kg", 1);
            break;
    }
}

void setMenu(boolean on)
{
    if(on) {
        //turn on the backlight and display the menu home screen
        lcd.backlight();
        menuPosition = MENUPOS_ALARM_ARMED_A;
        menuShowing = true;
        menuEditing = false;
        timeEditing = false;
        displayMenu();
    } else {
        //turn off the backlight and display only the current time again
        menuShowing = false;
        lcd.noBacklight();
        lcd.clear();
        lcdPrintCenter(currentTimeString, 0);
        lastTimestamp = 0;
    }
}

//=================================
//            INPUT STUFF
//=================================
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
      buttonEncoder();
      break;
    case BUTTON_A_DOWN:
      buttonA();
      break;
    case BUTTON_B_DOWN:
      buttonB();
      break;
    case BUTTON_R_UP:
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
    if(!menuEditing) {
        if(menuPosition >= MENUPOS_CALIBRATION) {
            menuPosition = MENUPOS_CALIBRATION;
            return;
        }

        menuPosition++;
        displayMenu();
        return;
    }

    if((menuPosition == MENUPOS_ALARM_TIME_A || menuPosition == MENUPOS_ALARM_TIME_B || menuPosition == MENUPOS_TIME) && !timeEditing) {
        if(timePosition >= 3) {
            timePosition = TIMEPOS_AMPM;
            return;
        }
        timePosition++;
        displayMenu();
        return;
    }

    updateValue(1);
}

void encoderLeft()
{
    if(!menuEditing) {
        if(menuPosition <= 0) {
            menuPosition = MENUPOS_ALARM_ARMED_A;
            return;
        }

        menuPosition--;
        displayMenu();
        return;
    }

    if((menuPosition == MENUPOS_ALARM_TIME_A || menuPosition == MENUPOS_ALARM_TIME_B || menuPosition == MENUPOS_TIME) && !timeEditing) {
        if(timePosition <= 0) {
            timePosition = TIMEPOS_APPLY;
            return;
        }
        timePosition--;
        displayMenu();
        return;
    }

    updateValue(-1);
}

void updateValue(int offset)
{
    if(menuPosition == MENUPOS_ALARM_TIME_A || menuPosition == MENUPOS_ALARM_TIME_B || menuPosition == MENUPOS_TIME) {
        //we only get here if timeEditing is true
        //so this means we can update hours, minutes or switch between AM and PM

        switch(timePosition) {
            case 0: return; //if we are on 'apply' then we should never be able to set timeEditing to true
            case 1:
                if(offset < 0 && editHour == 0) editHour = 23;
                else if(offset > 0 && editHour == 23) editHour = 0;
                else editHour += offset;
                break;
            case 2:
                if(offset < 0 && editMinute == 0) editMinute = 59;
                else if(offset > 0 && editMinute == 59) editMinute = 0;
                else editMinute += offset;
                break;
            case 3:
                if(editHour < 12) editHour += 12;
                else editHour -= 12;
                break;
        }
        displayMenu();
        return;
    }

    switch(menuPosition) {
        case MENUPOS_ALARM_TIME_A:
        case MENUPOS_ALARM_TIME_B:
        case MENUPOS_TIME:
            //these menu positions do not have values directly controlled by the rotary encoder
            return;
        case MENUPOS_ALARM_ARMED_A:
            editVal = editVal > 0 ? 0 : 1;
            break;
        case MENUPOS_ALARM_ARMED_B:
            editVal = editVal > 0 ? 0 : 1;
            break;
        case MENUPOS_DISARM_DURATION:
            if(editVal <= 10 && offset < 0) editVal = 240;
            else if(editVal >= 240 && offset > 0) editVal = 10;
            else editVal += offset * 10;
            break;
        case MENUPOS_DISARM_TOLERANCE:
            if(editVal <= 5 && offset < 0) editVal = 120;
            else if(editVal >= 120 && offset > 0) editVal = 5;
            else editVal += offset * 5;
            break;
        case MENUPOS_DISARM_REQUIREMENT:
            if(editVal <= 5 && offset < 0) editVal = 100;
            else if(editVal >= 100 && offset > 0) editVal = 5;
            else editVal += offset * 5;
            break;
        case MENUPOS_BEEP_PATTERN:
            if(editVal <= 0 && offset < 0) editVal = PATTERN_COUNT - 1;
            else if(editVal >= (PATTERN_COUNT - 1) && offset > 0) editVal = 0;
            else editVal += offset;
            break;
        case MENUPOS_CALIBRATION:
            if(editValFloat <= 0.5 && offset < 0) editValFloat = 100.0;
            else if(editValFloat >= 100.0 && offset > 0) editValFloat = 0.5;
            else editValFloat += (float)offset * 0.5;
            break;
    }
    displayMenu();
}

void buttonEncoder()
{
    //Show menu if not showing (unless we're currently alarm-beeping)
    if(!menuShowing && !(alarmTriggeredA || alarmTriggeredB)) {
        setMenu(true);
        return;
    }

    //Begin editing if in root of menu
    if(!menuEditing) {
        menuEditing = true;
        
        switch(menuPosition) {
            case MENUPOS_ALARM_ARMED_A:
                editVal = curVal[ALARM_ARMED_A];
                break;
            case MENUPOS_ALARM_TIME_A:
                editHour = curVal[ALARM_HOUR_A];
                editMinute = curVal[ALARM_MINUTE_A];
                break;
            case MENUPOS_ALARM_ARMED_B:
                editVal = curVal[ALARM_ARMED_B];
                break;
            case MENUPOS_ALARM_TIME_B:
                editHour = curVal[ALARM_HOUR_B];
                editMinute = curVal[ALARM_MINUTE_B];
                break;
            case MENUPOS_TIME:
                editHour = currentHour;
                editMinute = currentMinute;
                break;
            case MENUPOS_DISARM_DURATION:
                editVal = curVal[DISARM_DURATION];
                break;
            case MENUPOS_DISARM_TOLERANCE:
                editVal = curVal[DISARM_TOLERANCE];
                break;
            case MENUPOS_DISARM_REQUIREMENT:
                editVal = curVal[DISARM_REQUIREMENT];
                break;
            case MENUPOS_BEEP_PATTERN:
                editVal = curVal[BEEP_PATTERN];
                break;
            case MENUPOS_CALIBRATION:
                editValFloat = calibrationWeight;
                break;
        }

        displayMenu();
        return;
    }

    //Special case, these options have a special time submenu so users can edit hours, minutes separately
    if(menuPosition == MENUPOS_ALARM_TIME_A || menuPosition == MENUPOS_ALARM_TIME_B || menuPosition == MENUPOS_TIME) {
        if(timeEditing) {
            //stop editing time and go back to hour/minute selection
            timeEditing = false;
        } else {
            if(timePosition == TIMEPOS_APPLY) {
                //if we are on 'confirm', exit and apply changes (after writing to EEPROM)
                if(menuPosition == MENUPOS_ALARM_TIME_A) {
                    byte preHour = curVal[ALARM_HOUR_A];
                    byte preMin = curVal[ALARM_MINUTE_A];

                    curVal[ALARM_HOUR_A] = editHour;
                    curVal[ALARM_MINUTE_A] = editMinute;

                    if(preHour != curVal[ALARM_HOUR_A]) {
                        EEPROM.write(ADDR_ALARM_HOUR_A, curVal[ALARM_HOUR_A]);
                        #ifdef ENABLE_SERIAL
                        dumpEEPROM();
                        #endif
                    }
                    if(preMin != curVal[ALARM_MINUTE_A]) {
                        EEPROM.write(ADDR_ALARM_MINUTE_A, curVal[ALARM_MINUTE_A]);
                        #ifdef ENABLE_SERIAL
                        dumpEEPROM();
                        #endif
                    }

                } else if(menuPosition == MENUPOS_ALARM_TIME_B) {
                    byte preHour = curVal[ALARM_HOUR_B];
                    byte preMin = curVal[ALARM_MINUTE_B];

                    curVal[ALARM_HOUR_B] = editHour;
                    curVal[ALARM_MINUTE_B] = editMinute;

                    if(preHour != curVal[ALARM_HOUR_B]) {
                        EEPROM.write(ADDR_ALARM_HOUR_B, curVal[ALARM_HOUR_B]);
                        
                        #ifdef ENABLE_SERIAL
                        dumpEEPROM();
                        #endif
                    }
                    if(preMin != curVal[ALARM_MINUTE_B]) {
                        EEPROM.write(ADDR_ALARM_MINUTE_B, curVal[ALARM_MINUTE_B]);                    
                        
                        #ifdef ENABLE_SERIAL
                        dumpEEPROM();
                        #endif
                    }

                } else {
                    Time t(1992, 05, 12, editHour, editMinute, 0, Time::kSunday);
                    rtc.time(t);
                    updateTime();
                }

                menuEditing = false;
            } else {
                //otherwise, begin editing the selected time interval
                timeEditing = true;
            }
        }
        displayMenu();
        return;
    }

    if(menuPosition == MENUPOS_CALIBRATION) {
        //if we are calibrating, then we complete the actual calibration routine
        float preFloat = calibrationWeight;
        calibrationWeight = editValFloat;
        if(calibrationWeight != preFloat) {
            EEPROM.write(ADDR_CALIBRATION_WEIGHT_INT, (int)editValFloat);
            EEPROM.write(ADDR_CALIBRATION_WEIGHT_DEC, editValFloat - (int)editValFloat);
        }
        calibrateScale();
        setMenu(false);
    }

    //Otherwise, a rotary encoder press indicates that we should apply any changes
    //Also write to the EEPROM!
    byte pre;
    switch(menuPosition) {
        case 0:
            pre = curVal[ALARM_ARMED_A];
            curVal[ALARM_ARMED_A] = editVal;
            if(curVal[ALARM_ARMED_A] != pre) {
                EEPROM.write(ADDR_ALARM_ARMED_B, curVal[ALARM_ARMED_A]);
                
                #ifdef ENABLE_SERIAL
                dumpEEPROM();
                #endif
            }
            break;
        case 2:
            pre = curVal[ALARM_ARMED_B];
            curVal[ALARM_ARMED_B] = editVal;
            if(curVal[ALARM_ARMED_B] != pre) {
                EEPROM.write(ADDR_ALARM_ARMED_A, curVal[ALARM_ARMED_B]);
                
                #ifdef ENABLE_SERIAL
                dumpEEPROM();
                #endif
            }
            break;
        case 5:
            pre = curVal[DISARM_DURATION];
            curVal[DISARM_DURATION] = editVal;
            if(curVal[DISARM_DURATION] != pre) {
                EEPROM.write(ADDR_DISARM_DURATION, curVal[DISARM_DURATION]);
                
                #ifdef ENABLE_SERIAL
                dumpEEPROM();
                #endif
            }
            break;
        case 6:
            pre = curVal[DISARM_TOLERANCE];
            curVal[DISARM_TOLERANCE] = editVal;
            if(curVal[DISARM_TOLERANCE] != pre) {
                EEPROM.write(ADDR_DISARM_TOLERANCE, curVal[DISARM_TOLERANCE]);
                
                #ifdef ENABLE_SERIAL
                dumpEEPROM();
                #endif
            }
            break;
        case 7:
            pre = curVal[DISARM_REQUIREMENT];
            curVal[DISARM_REQUIREMENT] = editVal;
            if(curVal[DISARM_REQUIREMENT] != pre) {
                EEPROM.write(ADDR_DISARM_REQUIREMENT, curVal[DISARM_REQUIREMENT]);
                
                #ifdef ENABLE_SERIAL
                dumpEEPROM();
                #endif
            }
            break;
        case 8:
            pre = curVal[BEEP_PATTERN];
            curVal[BEEP_PATTERN] = editVal;
            if(curVal[BEEP_PATTERN] != pre) {
                EEPROM.write(ADDR_BEEP_PATTERN, curVal[BEEP_PATTERN]);
                
                #ifdef ENABLE_SERIAL
                dumpEEPROM();
                #endif
            }
            break;
    }

    menuEditing = false;
    displayMenu();
}

void buttonA()
{
    if(!menuShowing) {
        lcd.backlight();
        lcdPrintCenter("Taring Scale", 1);
        scale.tare();
        delay(1000);
        
        lcd.noBacklight();
        lcdPrintCenter("", 1);
        return;
    }

    if(menuEditing) {
        if(timeEditing) {
            timeEditing = false;
        } else {
            menuEditing = false;
            lcdPrintCenter("Discarding", 1);
            delay(1000);
        }
        displayMenu();
    } else {
        setMenu(false);
    }
}

void buttonB()
{
    //same as button A, but also resets things to default
    //for now, just do the same as button A, but will add reset functionality later
    if(!menuShowing) {
        lcd.backlight();
        lcdPrintCenter("Taring Scale", 1);
        scale.tare();
        delay(700);
        
        lcd.noBacklight();
        lcdPrintCenter("", 1);
        return;
    }

    if(menuEditing) {
        if(timeEditing) {
            timeEditing = false;
        } else {
            menuEditing = false;
            //except for if we're editing the time - there is no 'default'
            if(menuPosition == MENUPOS_TIME) {
                lcdPrintCenter("Discarding", 1);
                delay(1000);
                displayMenu();
                return;
            }
            lcdPrint("Reset to Default", 1);
            delay(700);
            switch(menuPosition) {
                case MENUPOS_ALARM_ARMED_A:
                    curVal[ALARM_ARMED_A] = defaultVal[ALARM_ARMED_A];
                    EEPROM.write(ADDR_ALARM_ARMED_A, curVal[ALARM_ARMED_A]);
                    break;
                case MENUPOS_ALARM_TIME_A:
                    curVal[ALARM_HOUR_A] = defaultVal[ALARM_HOUR_A];
                    EEPROM.write(ADDR_ALARM_HOUR_A, curVal[ALARM_HOUR_A]);
                    curVal[ALARM_MINUTE_A] = defaultVal[ALARM_MINUTE_A];
                    EEPROM.write(ADDR_ALARM_MINUTE_A, curVal[ALARM_MINUTE_A]);
                    break;
                case MENUPOS_ALARM_ARMED_B:
                    curVal[ALARM_ARMED_B] = defaultVal[ALARM_ARMED_B];
                    EEPROM.write(ADDR_ALARM_ARMED_B, curVal[ALARM_ARMED_B]);
                    break;
                case MENUPOS_ALARM_TIME_B:
                    curVal[ALARM_HOUR_B] = defaultVal[ALARM_HOUR_B];
                    EEPROM.write(ADDR_ALARM_HOUR_B, curVal[ALARM_HOUR_B]);
                    curVal[ALARM_MINUTE_B] = defaultVal[ALARM_MINUTE_B];
                    EEPROM.write(ADDR_ALARM_MINUTE_B, curVal[ALARM_MINUTE_B]);
                    break;
                case MENUPOS_DISARM_DURATION:
                    curVal[DISARM_DURATION] = defaultVal[DISARM_DURATION];
                    EEPROM.write(ADDR_DISARM_DURATION, curVal[DISARM_DURATION]);
                    break;
                case MENUPOS_DISARM_TOLERANCE:
                    curVal[DISARM_TOLERANCE] = defaultVal[DISARM_TOLERANCE];
                    EEPROM.write(ADDR_DISARM_TOLERANCE, curVal[DISARM_TOLERANCE]);
                    break;
                case MENUPOS_DISARM_REQUIREMENT:
                    curVal[DISARM_REQUIREMENT] = defaultVal[DISARM_REQUIREMENT];
                    EEPROM.write(ADDR_DISARM_REQUIREMENT, curVal[DISARM_REQUIREMENT]);
                    break;
                case MENUPOS_BEEP_PATTERN:
                    curVal[BEEP_PATTERN] = defaultVal[BEEP_PATTERN];
                    EEPROM.write(ADDR_BEEP_PATTERN, curVal[BEEP_PATTERN]);
                    break;
                case MENUPOS_CALIBRATION:
                    calibrationWeight = calibrationWeightDefault;
                    EEPROM.write(ADDR_CALIBRATION_WEIGHT_INT, (int)calibrationWeight);
                    EEPROM.write(ADDR_CALIBRATION_WEIGHT_DEC, calibrationWeight - (int)calibrationWeight);
                    break;
            }
        }
        displayMenu();
    } else {
        setMenu(false);
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


//=================================
//            TIME STUFF
//=================================
void updateTime()
{
    Time t = rtc.time();
    timestamp = t.hr * 60 + t.min;
    currentTimeString = getTimeString(t.hr, t.min);
    currentHour = t.hr;
    currentMinute = t.min; 

    if(timestamp == lastTimestamp) return;
    if(alarmTriggeredA || alarmTriggeredB) return;

    //Check if the current time is equal to the time that either alarm should be triggered at
    if(timestamp == curVal[ALARM_HOUR_A] * 60 + curVal[ALARM_MINUTE_A] && curVal[ALARM_ARMED_A] > 0) {
        alarmTriggeredA = true;
    } else if(timestamp == curVal[ALARM_HOUR_B] * 60 + curVal[ALARM_MINUTE_B] && curVal[ALARM_ARMED_B] > 0) {
        alarmTriggeredB = true;
    }

    //If so, trigger the alarm!
    if(alarmTriggeredA || alarmTriggeredB) {
        triggerAlarm();
    } else if(!menuShowing) {
        //if not, and we're not currently in the menu, update the time display on the home screen
        lcdPrintCenter(currentTimeString, 0);
    }
}

void triggerAlarm()
{
    alarmTriggeredTime = millis();
    onTime = millis();
    offTime = millis();
    beepSequence = 0;
    if(menuShowing) {
        setMenu(false);
    }
}

void alarmLoop()
{
    //don't do anything if the alarm hasn't been triggered
    //it gets triggered by updateTime which is called before this function in loop()
    if(!alarmTriggeredA && !alarmTriggeredB) return;

    //turn on the backlight
    lcd.backlight();

    if(scaleReading > curVal[DISARM_REQUIREMENT]) {
        //if the user is standing on the scale, update disarm amount
        disarmCounter += deltaMillis;
        toleranceCounter = 1;

        //stop buzzing!
        noTone(PIN_BUZZER);
        
        //display disarm progress bar
        lcdPrintCenter("Disarming Alarm", 0);
        lcdPrint(getProgressString((float)disarmCounter / 1000.0, (float)curVal[DISARM_DURATION], true), 1);

        //after enough time passes, disarm the scale and go back to the menu
        if((disarmCounter / 1000) > curVal[DISARM_DURATION]) {
            lcdPrintCenter("Alarm Disarmed", 0);
            lcdPrintCenter("Go get 'em!", 1);
            alarmTriggeredA = false;
            alarmTriggeredB = false;
            disarmCounter = 0;
            toleranceCounter = 0;
            delay(2000);
            setMenu(false);
        }
    } else {
        //if they aren't standing on it, and the tolerance time has elapsed (or hasn't started)
        //beep!
        if(toleranceCounter == 0 || (toleranceCounter / 1000) > curVal[DISARM_TOLERANCE]) {
            alarmBeep();
            disarmCounter = 0;
            toleranceCounter = 0;

            lcdPrintCenter("Rise & Shine!", 0);
            lcdPrintCenter(currentTimeString, 1);
        } else {
            //otherwise, display a tolerance bar
            lcdPrintCenter("Get back on!", 0);
            lcdPrint(getProgressString((float)toleranceCounter / 1000.0, (float)curVal[DISARM_TOLERANCE], false), 1);

            alarmWarning();

            toleranceCounter += deltaMillis;
        }
    }
}

//for warning the user to get  back on
void alarmWarning()
{
    if(millis() % 1000 < 10) {
        tone(PIN_BUZZER, 2000);
    } else {
        noTone(PIN_BUZZER);
    }
}

void alarmBeep()
{
    switch(curVal[BEEP_PATTERN]) {
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
        case PATTERN_WAIL:
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
            int timeSinceLerp = ((millis() - alarmTriggeredTime) / 1000);

            //actually, make it 60 seconds
            //the math is all assuming 30 seconds so just modify this multiplier in the future
            timeSinceLerp /= 2;

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
//              SCALE STUFF
//=================================

void updateScale()
{
    //when we're in the menu we don't need to do any scale updateScale
    //unless we're in calibration, but that's in a separate function
    if(menuShowing) return;

    //update units and display string
    scaleReading = scale.get_units(1);
    byte readingDecimal = (int)(scaleReading * 10) % 10;
    scaleReadingString = String((int)scaleReading) + "." + String(readingDecimal) + " kg";
    
    //if we're not currently beeping, update the home screen with the weight reading
    if(!alarmTriggeredA && !alarmTriggeredB) {
        if(scaleReading > SCALE_DISPLAY_THRESHOLD) {
            lcd.backlight();
            lcdPrintCenter(scaleReadingString, 1);
        } else {
            lcdPrint("", 1);
            lcd.noBacklight();
        }
    }    
}

void calibrateScale()
{
    lcdPrint("Calibrating...", 0);
    for(byte i = 5; i > 0; i--) {
        lcdPrint("Get off: " + String(i) + "...", 1);
        scale.tare();
        delay(500);
    }
    
    for(byte i = 5; i > 0; i--) {
        lcdPrint("Get on: " + String(i) + "...", 1);
        delay(1000);
    }

    lcdPrint("Stay put!", 1);

#ifdef ENABLE_SERIAL
    Serial.println("Calibrating against reference weight of " + String(calibrationWeight));
#endif

    scaleCalibrationFactor = 0;
    scale.set_scale(scaleCalibrationFactor);
    scaleReading = scale.get_units(1);
  
    float offset = 10000;
    float delta = scaleReading - calibrationWeight;
    float lastDelta = delta;

#ifdef ENABLE_SERIAL    
    Serial.println("d:" + String(delta) + "\to:" + String(offset) + "\tf:" + String(scaleCalibrationFactor));
#endif    

    scaleCalibrationFactor += offset;
    scale.set_scale(scaleCalibrationFactor);
    scaleReading = scale.get_units(1);
    delta = scaleReading - calibrationWeight;
    if(abs(delta) > abs(lastDelta)) {
        offset *= -1;
    }

    
#ifdef ENABLE_SERIAL
    Serial.println("d:" + String(delta) + "\to:" + String(offset) + "\tf:" + String(scaleCalibrationFactor));
#endif

    scaleCalibrationFactor += offset;
    scale.set_scale(scaleCalibrationFactor);
    scaleReading = scale.get_units(1);
    delta = scaleReading - calibrationWeight;
    if(abs(delta) > abs(lastDelta)) {
        offset *= -1;
    }

    
#ifdef ENABLE_SERIAL
    Serial.println("d:" + String(delta) + "\to:" + String(offset) + "\tf:" + String(scaleCalibrationFactor));
    Serial.println("-----");
#endif    

    byte count = 3;
    char calibratingDots[][16] = {
        "Calibrating",
        "Calibrating.",
        "Calibrating..",
        "Calibrating...",
        "Calibrating....",
        "Calibrating....."
    };
    while(abs(delta) > 0.2 && abs(offset) > 1.0) {
        if(abs(delta) > abs(lastDelta)) {
            offset *= -0.5;
        }

        if(32767 - offset < scaleCalibrationFactor && scaleCalibrationFactor > 0) scaleCalibrationFactor = 32766;
        else if(-32767 + offset > scaleCalibrationFactor && scaleCalibrationFactor < 0) scaleCalibrationFactor = -32766;
        else scaleCalibrationFactor += offset;
        scale.set_scale(scaleCalibrationFactor);
        scaleReading = scale.get_units(1);
        lastDelta = delta;
        delta = scaleReading - calibrationWeight;

#ifdef ENABLE_SERIAL
        Serial.println("d:" + String(delta) + "\to:" + String(offset) + "\tf:" + String(scaleCalibrationFactor));
#endif

        lcdPrint(calibratingDots[count], 0);
        count++;
        if(count > 5) count = 0;
    }

#ifdef ENABLE_SERIAL
    Serial.println("Calibration Complete");
    Serial.println("d:" + String(delta) + "\to:" + String(offset) + "\tf:" + String(scaleCalibrationFactor));
#endif

    byte calibrationLow = scaleCalibrationFactor % 256;
    byte calibrationHigh = scaleCalibrationFactor / 256;
    EEPROM.write(ADDR_CALIBRATION_LOW, calibrationLow);
    EEPROM.write(ADDR_CALIBRATION_HIGH, calibrationHigh);
}


//=================================
//            GENERAL STUFF
//=================================
String getTimeString(byte hour, byte minute) 
{    
    String hourString = (hour > 12 ? String(hour - 12) : String(hour));
    if(hour == 0) hourString = "00";
    else if(hour == 12) hourString = "12";
    else if(hour % 12 < 10) hourString = "0" + hourString;

    String minuteString = String(minute);
    if(minute == 0) minuteString = "00";
    else if(minute < 10) minuteString = "0" + minuteString;

    if(hour < 12) return hourString + ":" + minuteString + "am";
    else return hourString + ":" + minuteString + "pm";
}

String getProgressString(float cur, float max, boolean forward)
{
    String output = "|";
    for(float i = 0.0; i < 14.0; i++) {
        float lerp = (cur * 14.0) / max;
        if(forward) {
            if(lerp > i) output += "#";
            else output += "-";
        } else {
            if(lerp < (14.0 - i)) output += "#";
            else output += "-";
        }
    }
    output += "|";
    return output;
}
