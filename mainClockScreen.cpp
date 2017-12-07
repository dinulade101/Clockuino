#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <EEPROM.h>
#include "alarm.h"

// initialize a time array, to be filled in the format hhmmss and alarm array
// that saves alarms input by user in format hhmm.
int time[6] = {0}, alarmTime[4] = {0};

// initalize 2 constant arrays, one for the 4 LEDs that show the random
// pattern and one for the button pins that you need to press to deactivate
// the alarm.
const int patternPins[4] = {22, 24, 26, 28};
const int patternLED[4] = {23, 25, 27, 29};

// counter that checks if arduino read all the time values from computer.
int serialReadCounter = 0;

// the boolean value read checks tells us if we read the time from computer.
// new alarm created checks if an alarm is created
bool read = 0, newAlarmCreated = 0;

// the 4 values of the current time
int hoursDig1 = 0, hoursDig2 = 0, minDig1 = 0, minDig2 = 0;

// the array containing the random integers that the user has to follow
// to disable the alarm
uint8_t patternToSolve[4] = {0};

// initializes vars to keep track of which screen of alarm interface we are on, number of alarms currently set
// and page number on the view alarms page
int screenState = 0;
int numOfAlarms = 0;
int alarmPageNum = 1;

// To implement snooze function
int snooze = 0;

// initialize array to store alarms
Alarm alarmArrayGlobal[100];

// #define ALARM_ON 44

// define digital pin numbers for alarm
#define RESET_TIME_PIN 11   // pin that you press to receive time
#define BUZZER 12           // the buzzer
#define ALARM_FLASH 43      // flashes when the alarm is turned on
#define alarmLED 8          // if there are any alarms turned on
#define analogPin 1         // to get random numbers

#define TFT_DC 9
#define TFT_CS 10
// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000
// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940
// touch screen pins, obtained from the documentaion
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM  5  // can be a digital pin
#define XP  4  // can be a digital pin

// width/height of the display when rotated horizontally
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

//width and height of button, default font-size for time, digit width for time
#define BUTTON_WIDTH 215
#define BUTTON_HEIGHT 30
#define FONTSIZE 11
#define DIGIT_WIDTH 5*FONTSIZE
#define SPACING_BETWEEN_DIGITS 15

// define extra colors
#define GREEN 0x07E0
#define BLUE 0x001F

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// a multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//forward declarations
void drawButton();
void initializeFour7SegDisplays();
void checkAlarmCond();
void solveThePattern();

void setup(){
	init();
	Serial.begin(9600);

	//initalize Arduino pins

	//set RESET_TIME_PIN to input and turn on internal pull up resistor
	pinMode(RESET_TIME_PIN, INPUT);
	digitalWrite(RESET_TIME_PIN, HIGH);

  // initialize analog pin as input
	pinMode(analogPin, INPUT);

  // initalize the pattern LEDs and the pattern pins
	for (int i = 0; i < 4; i++){
		pinMode(patternPins[i], INPUT);
		digitalWrite(patternPins[i], HIGH);
		pinMode(patternLED[i],OUTPUT);
	}

  // initalize LEDs and Buzzers
	pinMode(alarmLED, OUTPUT);
	pinMode(BUZZER, OUTPUT);
	pinMode(ALARM_FLASH, OUTPUT);

	tft.begin();
}

// draw on screen one digit of time display
void setNumOf7SegDisplay(int digitVal, int digitLoc, int cursorStart){
	tft.setCursor((digitLoc*(DIGIT_WIDTH+SPACING_BETWEEN_DIGITS)+cursorStart), 15);
	tft.setTextSize(FONTSIZE);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.print(digitVal);
}


void createNewAlarm(){
  // Screen that takes in user's input, takes that value and creates a new
  // alarm with that input.
	tft.fillScreen(ILI9341_BLACK);
	tft.fillRect(115, 113, 100, 40, ILI9341_WHITE);
	tft.setTextSize(4);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(117, 115);
	tft.println("Hour");
	tft.fillTriangle(80, 113, 110, 113, 95, 143,ILI9341_WHITE);
	tft.fillTriangle(225, 143, 255, 143, 240, 113,ILI9341_WHITE);

	tft.fillRect(105, 168, 140, 35, ILI9341_WHITE);
	tft.setTextSize(4);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(107, 170);
	tft.println("Minutes");
	tft.fillTriangle(60, 168, 90, 168, 75, 203,ILI9341_WHITE);
	tft.fillTriangle(255, 203, 285, 203, 270, 168,ILI9341_WHITE);

	tft.setTextSize(3);
	tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
	tft.setCursor(70, 220);
	tft.println("Save Alarm");

	tft.setTextSize(5);
	tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
	tft.setCursor(0, 200);
	tft.println("X");

	// draw digits
	for (int i = 0; i < 2; i++){
		setNumOf7SegDisplay(0, i, 0);
	}

	for (int i = 2; i < 4; i++){
		setNumOf7SegDisplay(0, i, SPACING_BETWEEN_DIGITS*2);
	}

}

// redraws and initializes the home scren of alarm clock
void reloadMainScreen(){
	screenState = 0;
	alarmPageNum = 1;
	tft.fillScreen(ILI9341_BLACK);
	initializeFour7SegDisplays();
	drawButton();
}

// takes user set alarm numbers, and saves it in the EEPRROM using the
// saveAlarm function
void saveAlarmMainScreen(){
	Alarm alarm;
	alarm.h1 = alarmTime[0];
	alarm.h2 = alarmTime[1];
	alarm.m1 = alarmTime[2];
	alarm.m2 = alarmTime[3];
	alarm.state = true;
	int nextAlarmAddress = getNextAlarmAddress();

	// ensures that the next alarm address is at an appropriate location on EEPROM accounting for the storage taken up by alarm object
	saveAlarm(nextAlarmAddress*sizeof(alarm)+sizeof(int), alarm);
	saveNextAlarmAddress(nextAlarmAddress+1);

	reloadMainScreen();
	alarmTime[0] = 0;
	alarmTime[1] = 0;
	alarmTime[2] = 0;
	alarmTime[3] = 0;
	checkAlarmCond();
}

/****************************alarm sorting ********/

void swap(Alarm& r1, Alarm& r2) {
	Alarm tmp = r1;
	r1 = r2;
	r2 = tmp;
}

int pivot(Alarm alarmsArray[], int n, int pi) {
  // swap the alarm at the pivot with the last alarm
  swap(alarmsArray[pi], alarmsArray[n-1]);

  // initallize lo to be the first element and hi to be the element before last
  int lo = 0;
  int hi = n-2;

  // iterate until lo>high
  while (lo <= hi){
    // the time of the alarm in minutes at the position hi
    int hiAlarmMins = (alarmsArray[hi].h1 * 10 + alarmsArray[hi].h2) * 60 +
                      alarmsArray[hi].m1 * 10 + alarmsArray[hi].m2;

    // the time of the alarm in minutes at the position n-1 (last)
    int nMinus1AlarmMins = (alarmsArray[n-1].h1 * 10 +alarmsArray[n-1].h2) * 60
                          + alarmsArray[n-1].m1 * 10 + alarmsArray[n-1].m2;

    // the time of the alarm in minutes at the position lo
    int loAlarmMins = (alarmsArray[lo].h1 * 10 + alarmsArray[lo].h2) * 60 +
                      alarmsArray[lo].m1 * 10 + alarmsArray[lo].m2;

    if (hiAlarmMins > nMinus1AlarmMins) {
      // decrease hi by one if the time is greater than the previous time
      hi--;
    }
    else if (loAlarmMins <= nMinus1AlarmMins) {
      // or increase lo by 1 if the time is less than or equal the previous time
      lo++;
    }
    else{
      // if those 2 cases fail, swap lo and hi
      swap(alarmsArray[lo], alarmsArray[hi]);
    }
  }

  // swap the alarm at lo's final position with the last alarm
  swap(alarmsArray[lo], alarmsArray[n-1]);

  // return lo which is the pivot
  return lo;
}

//Sort an array with n elements using Quick Sort
void qsort(Alarm alarmsArray[], int n) {
  // if there's nothing to sort
	if (n < 1){return;}
  // start with pivot at the middle
  int pi = n/2;

  // find the pivot and store it as newPi
  int newPi = pivot(alarmsArray, n, pi);

  // recurse it using part before and part after the pivot
  qsort(alarmsArray, newPi);
  qsort(alarmsArray + (newPi + 1), n - (newPi + 1));
}

void viewAlarmsList(){

	// shows list of all alarms, with each page containing four alarms
  screenState = 2;
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(5);
  tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
  tft.setCursor(0, 200);
  tft.println("X");

  tft.fillTriangle(TFT_WIDTH/2, TFT_HEIGHT-35, TFT_WIDTH/2+17, TFT_HEIGHT, TFT_WIDTH/2+34, TFT_HEIGHT-35,ILI9341_WHITE);
  tft.fillTriangle(TFT_WIDTH/2-40, TFT_HEIGHT, TFT_WIDTH/2-23, TFT_HEIGHT-35, TFT_WIDTH/2-6, TFT_HEIGHT,ILI9341_WHITE);

	// gets the current number of stored alarms from EEPROM
  numOfAlarms = EEPROM.read(0);
  for (int i=1; i<=numOfAlarms; i++){
    if (i > numOfAlarms){
      break;
    }
    Alarm alarm;

    EEPROM.get((i-1)*sizeof(alarm)+sizeof(int), alarm);

		// keep track of the index of the alarm object in the EEPROM
    alarm.origIdx = i;

		// savealarm object in different array to sort later on
    alarmArrayGlobal[i-1]=alarm;
  }

	// run qsort to sort the alarm times according to alarm time in least to greatest fashion
  qsort(alarmArrayGlobal, numOfAlarms);

	for (int i=((alarmPageNum-1)*4)+1; i<=4*(alarmPageNum); i++){
		if (i > numOfAlarms){
			break;
		}
		Alarm alarm = alarmArrayGlobal[i-1];

		tft.setTextSize(5);
		tft.fillRect(0, 50*(i-1-(4)*(alarmPageNum-1)), TFT_WIDTH, 35, ILI9341_BLACK);

		// if alarm.state is on,
		if (alarm.state){
			tft.fillRect(TFT_WIDTH/2, 50*(i-1-(4)*(alarmPageNum-1)), TFT_WIDTH/3, 35, GREEN);
		}
		else{
			tft.fillRect(TFT_WIDTH/2, 50*(i-1-(4)*(alarmPageNum-1)), TFT_WIDTH/3, 35, ILI9341_RED);
		}
		tft.setCursor(0, 50*(i-1 - (4)*(alarmPageNum-1)));
		String alarmObjectTimeStr = String();
		alarmObjectTimeStr = String(alarm.h1, HEX) + String(alarm.h2, HEX) + ':' + String(alarm.m1, HEX) + String(alarm.m2, HEX);
		tft.setTextColor(ILI9341_WHITE);
		tft.println(alarmObjectTimeStr);
		tft.setCursor(TFT_WIDTH/3+TFT_WIDTH/2+20, 50*(i-1 - (4)*(alarmPageNum-1)));
		tft.setTextColor(ILI9341_WHITE);
		tft.println("X");
	}

}
void drawButton(){
	tft.fillRect(TFT_WIDTH/2 - BUTTON_WIDTH/2, TFT_HEIGHT - BUTTON_HEIGHT - 20, BUTTON_WIDTH, BUTTON_HEIGHT, ILI9341_WHITE);
	tft.setTextSize(3);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(TFT_WIDTH/2 - BUTTON_WIDTH/2, TFT_HEIGHT - BUTTON_HEIGHT - 18);
	tft.println("Create Alarm");

	tft.fillRect(TFT_WIDTH/2 - BUTTON_WIDTH/2, TFT_HEIGHT - BUTTON_HEIGHT*2 - 30, BUTTON_WIDTH, BUTTON_HEIGHT, ILI9341_WHITE);
	tft.setTextSize(3);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(TFT_WIDTH/2 - BUTTON_WIDTH/2 +5 , TFT_HEIGHT - BUTTON_HEIGHT*2 - 28);
	tft.println("View Alarms");

	tft.setTextSize(5);
	tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
	tft.setCursor(0, 200);
	tft.println("R");
}

void clearEEPROM(){
	for (int i=0; i<EEPROM.length(); i++){
		EEPROM.write(i, 0);
	}
	checkAlarmCond();
}

void changeAlarmState(int alarmNum, int alarmNumOrg){
	Alarm alarm;
	EEPROM.get((alarmNumOrg-1)*sizeof(alarm)+sizeof(int), alarm);
	alarm.state = !alarm.state;
	saveAlarm((alarmNumOrg-1)*sizeof(alarm)+sizeof(int), alarm);
	if (alarm.state){
		tft.fillRect(TFT_WIDTH/2, 50*(alarmNum-1-(4)*(alarmPageNum-1)), TFT_WIDTH/3, 35, GREEN);
	}
	else{
		tft.fillRect(TFT_WIDTH/2, 50*(alarmNum-1-(4)*(alarmPageNum-1)), TFT_WIDTH/3, 35, ILI9341_RED);
	}
	checkAlarmCond();
	delay(200);
}

void moveAlarmsDown(){
	if (numOfAlarms > (alarmPageNum)*4){
		alarmPageNum++;
		viewAlarmsList();
		delay(200);
	}
}

void moveAlarmsUp(){
	if (alarmPageNum != 1){
		alarmPageNum--;
		viewAlarmsList();
		delay(500);
	}
}

void deleteAlarm(int alarmNum){
	numOfAlarms = EEPROM.read(0);
		if (numOfAlarms != 0){
		int counter = 0;
		for (int i=1; i<=numOfAlarms; i++){
			Alarm alarm;
			EEPROM.get((i-1)*sizeof(alarm)+sizeof(int), alarm);
			if (i != alarmNum){
				saveAlarm((counter)*sizeof(alarm)+sizeof(int), alarm);
				counter++;
			}
		}
		EEPROM.put(0, numOfAlarms-1);
		viewAlarmsList();
	}
	checkAlarmCond();
}

void buttonClick(){
  TSPoint touch = ts.getPoint();
  if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {return;}
  // get the y coordinate of where the display was touched
  // remember the x-coordinate of touch is really our y-coordinate
  // on the display
  int touchY = map(touch.x, TS_MINX, TS_MAXX, 0, TFT_HEIGHT - 1);

  // need to invert the x-axis, so reverse the
  // range of the display coordinates
  int touchX = map(touch.y, TS_MINY, TS_MAXY, TFT_WIDTH - 1, 0) - 20;
  if (screenState == 0){
    bool inRangeCreateAlarmButton = touchX > TFT_WIDTH/2 - BUTTON_WIDTH/2 && touchX< TFT_WIDTH/2 + BUTTON_WIDTH/2;
    inRangeCreateAlarmButton = inRangeCreateAlarmButton && (touchY > TFT_HEIGHT - BUTTON_HEIGHT - 20) && (touchY<TFT_HEIGHT - 20);
    if (inRangeCreateAlarmButton)
    {
      screenState = 1;
      createNewAlarm();
    }

    if (touchY >= TFT_HEIGHT - BUTTON_HEIGHT*2 - 50 && touchY <= TFT_HEIGHT - BUTTON_HEIGHT - 30){
      if (touchX >= TFT_WIDTH/2 - BUTTON_WIDTH/2 && touchX <= TFT_WIDTH/2 - BUTTON_WIDTH/2 + BUTTON_WIDTH){
        viewAlarmsList();
      }
    }
    if (touchY >= 200 && touchY <= 240){
      if (touchX >= 0 && touchX <= 25){
        clearEEPROM();
      }
    }
  }
  else if (screenState == 1){
    if (touchY>= 113 && touchY<= 143){
      if (touchX >= 80 && touchX <= 110){
        //down triangle for hours
        alarmTime[1]--;
        if (alarmTime[1]<0){
          alarmTime[0]--;
          alarmTime[1]=9;
          if (alarmTime[0]<0){
            alarmTime[1]=3;
            alarmTime[0]=2;
          }
          setNumOf7SegDisplay(alarmTime[0], 0, 0);
        }
        setNumOf7SegDisplay(alarmTime[1], 1, 0);
        newAlarmCreated = 0;

      }
      else if (touchX >= 225 && touchX <= 255){
        //up triangle for hours
        alarmTime[1]++;
        if (alarmTime[1]>9){
          alarmTime[0]++;
          alarmTime[1]=0;
        }
        if (alarmTime[0]>1 && alarmTime[1]>3){
          alarmTime[1]=0;
          alarmTime[0]=0;
        }
        setNumOf7SegDisplay(alarmTime[0], 0, 0);
        setNumOf7SegDisplay(alarmTime[1], 1, 0);
        newAlarmCreated = 0;
      }
    }

    else if (touchY>= 168 && touchY<= 203){
      if (touchX >= 60 && touchX <= 90){
        //down triangle for minutes
        alarmTime[3]--;
        if (alarmTime[3]<0){
          alarmTime[2]--;
          alarmTime[3]=9;
          if (alarmTime[2]<0){
            alarmTime[3]=9;
            alarmTime[2]=5;
          }
          setNumOf7SegDisplay(alarmTime[2], 2, SPACING_BETWEEN_DIGITS*2);
        }
        setNumOf7SegDisplay(alarmTime[3], 3, SPACING_BETWEEN_DIGITS*2);
        newAlarmCreated = 0;

      }
      else if (touchX >= 255 && touchX <= 285){
        //up triangle for minutes
        alarmTime[3]++;
        if (alarmTime[3]>9){
          alarmTime[2]++;
          alarmTime[3]=0;
          if (alarmTime[2]>5){
            alarmTime[3]=0;
            alarmTime[2]=0;
          }
          setNumOf7SegDisplay(alarmTime[2], 2, SPACING_BETWEEN_DIGITS*2);
        }
        setNumOf7SegDisplay(alarmTime[3], 3, SPACING_BETWEEN_DIGITS*2);
        newAlarmCreated = 0;
      }
    }

    else if (newAlarmCreated == 0 && touchY >= 220 && touchY <=240){
      if (touchX >= 70 && touchX <= 270){
        saveAlarmMainScreen();
        newAlarmCreated = 1;
      }
    }

    if (touchY >= 200 && touchY <= 240){
      if (touchX >= 0 && touchX <= 25){
        reloadMainScreen();
      }
    }
  }
  else if (screenState == 2){
    if (touchY >= 200 && touchY <= 240){
      if (touchX >= 0 && touchX <= 25){
        reloadMainScreen();
      }
    }
    if (touchX >= TFT_WIDTH/2 && touchX <= TFT_WIDTH/2+TFT_WIDTH/3){
      for (int i=((alarmPageNum-1)*4)+1; i<=4*(alarmPageNum); i++){
        if (i-1 < numOfAlarms){
          if ((touchY >= 50*((i-1-(4)*(alarmPageNum-1)))) && (touchY <= 50*((i-1-(4)*(alarmPageNum-1)))+35)){
            changeAlarmState(i, alarmArrayGlobal[i-1].origIdx);
          }
        }
      }
    }
    if (touchX >= TFT_WIDTH/2+TFT_WIDTH/3 && touchX <= TFT_WIDTH){
      for (int i=((alarmPageNum-1)*4)+1; i<=4*(alarmPageNum); i++){
        if ((touchY >= 50*((i-1-(4)*(alarmPageNum-1)))) && (touchY <= 50*((i-1-(4)*(alarmPageNum-1)))+35)){
          deleteAlarm(alarmArrayGlobal[i-1].origIdx);
        }
      }
    }
    if (touchY >= TFT_HEIGHT-35 && touchY <= TFT_HEIGHT){
      if (touchX >= TFT_WIDTH/2 && touchX <= TFT_WIDTH/2+34){
        moveAlarmsDown();
      }
      if (touchX >= TFT_WIDTH/2-40 && touchX <= TFT_WIDTH/2-23){
        moveAlarmsUp();
      }
    }
  }
  delay(100);
}

void initializeFour7SegDisplays(){
	setNumOf7SegDisplay(hoursDig1, 0, 0);
	setNumOf7SegDisplay(hoursDig2, 1, 0);
	setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
	setNumOf7SegDisplay(minDig2,3, SPACING_BETWEEN_DIGITS*2);
}

void setTimeToDisplay(){
	hoursDig1 = time[0];
	hoursDig2 = time[1];
	minDig1 = time[2];
	minDig2 = time[3];
	setNumOf7SegDisplay(hoursDig1, 0, 0);
	setNumOf7SegDisplay(hoursDig2, 1, 0);
	setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
	setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);
}

void downloadTimeFromComputer(){
  while (!read){
    if(Serial.available()>0){
      int temp = Serial.read();
      if(temp != ','){
        int num = temp - 48;
        time[serialReadCounter] = num;
        serialReadCounter++;
      }
    }
    if (serialReadCounter == 6){
      read = 1;
      setTimeToDisplay();
    }
  }
}

// launches new screen to indicate that alarm is going off
void launchAlarmPopUp(){
	screenState = 3;
	tft.fillScreen(ILI9341_BLACK);

	tft.setTextSize(7);
	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(0, TFT_HEIGHT/4);
	tft.println("WAKE UP");

	tft.setTextSize(4);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(TFT_WIDTH/2 - BUTTON_WIDTH/2, TFT_HEIGHT - BUTTON_HEIGHT - 18);
	tft.println("Turn Alarm Off");
}

void alarmGoOff(){
	uint8_t randomNumber;
  launchAlarmPopUp();
	for (int i = 0; i < 4; i++){
		// gets a "random" number between 0-3 and then adds it to the pattern
		randomNumber = (analogRead(analogPin) % 4);
		patternToSolve[i] = randomNumber;
		digitalWrite(patternLED[patternToSolve[i]], LOW);
		delay(1000);
		digitalWrite(patternLED[patternToSolve[i]], HIGH);
		delay(1000);
		digitalWrite(patternLED[patternToSolve[i]], LOW);
	}
	solveThePattern();
  reloadMainScreen();
}

void solveThePattern(){
  delay(2000);
  // initialize an array that records the buttons that user clicked so far
  // j is a counter that tells us how many buttons were pressed so far.
  int buttonsReceived[4], j = 0;

  // boolean value to see if user solved pattern
  bool correct = 1;
	while (true){
    // flash LED
    // keep buzzer on till user gets the pattern right

		delayMicroseconds(100);
		digitalWrite(BUZZER, LOW);
		delayMicroseconds(100);
		digitalWrite(BUZZER, HIGH);
    /*
      Record the buttons that the user presses, and fill the array
      buttonsReceived with them, to later compare if the user solved the pattern
    */
		if(digitalRead(patternPins[0]) == LOW){
			while(digitalRead(patternPins[0]) == LOW){delay(10);}
			buttonsReceived[j] = 0;
			j++;
		}

		else if(digitalRead(patternPins[1]) == LOW){
			while(digitalRead(patternPins[1]) == LOW){delay(10);}
			buttonsReceived[j] = 1;
			j++;
		}

		else if(digitalRead(patternPins[2]) == LOW){
			while(digitalRead(patternPins[2]) == LOW){delay(10);}
			buttonsReceived[j] = 2;
			j++;
		}

		else if(digitalRead(patternPins[3]) == LOW){
			while(digitalRead(patternPins[3]) == LOW){delay(10);}
			buttonsReceived[j] = 3;
			j++;
		}

		if (j >= 4){
			for (int i = 0; i < 4; i++){
				if (buttonsReceived[i] != patternToSolve[i]){
					correct = 0;
				}
			}

			if(correct){break;}     // break the pattern if the user gets it
			else {                   // reset the pattern if the user doesn't
				alarmGoOff();
			}
		}
	}
}

void advanceClock(){
  minDig2++;
  if (minDig2 == 10){
    minDig1++;
    minDig2=0;
  }

  if (minDig1 == 6){
    hoursDig2++;
    minDig1=0;
    minDig2=0;

    if (hoursDig2 <10 && screenState == 0){
      setNumOf7SegDisplay(hoursDig2, 1, 0);
    }
  }

  if (screenState == 0){
    setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
    setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);
  }

  if (hoursDig2 == 10){
    hoursDig1++;
    hoursDig2 = 0;

    if (screenState == 0){
      setNumOf7SegDisplay(hoursDig2, 1, 0);
      setNumOf7SegDisplay(hoursDig1, 0, 0);
    }
  }

  if (hoursDig1 == 2 && hoursDig2 == 4){
    hoursDig1 = 0;
    hoursDig2 = 0;
    minDig1 = 0;
    minDig2 = 0;

    if (screenState == 0){
      setNumOf7SegDisplay(hoursDig1, 0, 0);
      setNumOf7SegDisplay(hoursDig2, 1, 0);
    }
  }

  numOfAlarms = EEPROM.read(0);
  for (int i=1; i<numOfAlarms+1; i++){
    Alarm alarm;
    EEPROM.get((i-1)*sizeof(alarm)+sizeof(int), alarm);
    if (hoursDig1 == alarm.h1 && hoursDig2 == alarm.h2 && minDig1 == alarm.m1
        && minDig2 == alarm.m2 && alarm.state){
      launchAlarmPopUp();
      alarmGoOff();
    }
  }
}

void checkAlarmCond(){
  bool ledOn = false;
  numOfAlarms = EEPROM.read(0);
  for (int i=1; i<numOfAlarms+1; i++){
    Alarm alarm;
    EEPROM.get((i-1)*sizeof(alarm)+sizeof(int), alarm);
    if (alarm.state){
      ledOn = true;
    }
    else{
    }
  }
  if (ledOn){
    digitalWrite(alarmLED, HIGH);
  }
  else{
    digitalWrite(alarmLED, LOW);
  }
}

int main(){
	setup();
	tft.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(3);
	initializeFour7SegDisplays();
	drawButton();
	checkAlarmCond();

  while (!read){
    if (digitalRead(RESET_TIME_PIN)==LOW){
      downloadTimeFromComputer();
      read = 1;
    }
  }

  while (true){
    if (millis()%60000 == 0){
      advanceClock();
    }
    buttonClick();
  }

  Serial.end();
  return 0;
}
