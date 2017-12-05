#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <EEPROM.h>
#include "alarm.h"

// initiate time array, to be filled in the format hhmmss
int time[6] = {0};
int alarmTime[4] = {0};
// i and read are used to make sure we read all 6 digits of time.
const int patternPins[4] = {22, 24, 26, 28};
const int patternLED[4] = {23, 25, 27, 29};
int serialReadCounter = 0;
bool read = 0, colonState = 0, makeAlarm = 0, newAlarmCreated = 0, alarmsOn = 0;
bool patternSolved = 0;
int hoursDig1 = 0, hoursDig2 = 0, minDig1 = 0, minDig2 = 0, alarmLED = 8;
uint8_t patternToSolve[4] = {0};
int alarmListVisible = 0;
int screenState = 0;
int numOfAlarms = 0;
int snooze = 0;
const int analogPin = 1;
#define ALARM_ON 44
#define RESET_TIME_PIN 11
#define BUZZER 12
#define ALARM_FLASH 43

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
#define BUTTON_WIDTH 215
#define BUTTON_HEIGHT 30
#define FONTSIZE 11
#define DIGIT_WIDTH 5*FONTSIZE
#define SPACING_BETWEEN_DIGITS 15
#define GREEN 0x07E0
#define BLUE 0x001F

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// a multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

void drawButton();
void initializeFour7SegDisplays();
void checkAlarmCond();

void setup(){
	init();
	Serial.begin(9600);

	//set RESET_TIME_PIN to input and turn on internal pull up resistor
	pinMode(RESET_TIME_PIN, INPUT);
	digitalWrite(RESET_TIME_PIN, HIGH);
	pinMode(ALARM_ON, INPUT);
	digitalWrite(ALARM_ON, HIGH);
	pinMode(analogPin, INPUT);
	for (int i = 0; i < 4; i++){
		pinMode(patternPins[i], INPUT);
		digitalWrite(patternPins[i], HIGH);
		pinMode(patternLED[i],OUTPUT);
	}
	pinMode(alarmLED, OUTPUT);
	pinMode(BUZZER, OUTPUT);
	pinMode(ALARM_FLASH, OUTPUT);
	tft.begin();
}

void setNumOf7SegDisplay(int digitVal, int digitLoc, int cursorStart){
	tft.setCursor((digitLoc*(DIGIT_WIDTH+SPACING_BETWEEN_DIGITS)+cursorStart), 15);
	tft.setTextSize(FONTSIZE);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.print(digitVal);
}

void createNewAlarm(){
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

	//tft.fillRect(105, 168, 140, 35, ILI9341_WHITE);
	tft.setTextSize(3);
	tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
	tft.setCursor(70, 220);
	tft.println("Save Alarm");

	tft.setTextSize(5);
	tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
	tft.setCursor(0, 200);
	tft.println("X");

	//tft.fillTriangle(60, 203, 90, 203, 75, 168,ILI9341_WHITE);
	//tft.fillTriangle(255, 168, 285, 168, 270, 203,ILI9341_WHITE);

	//draw digits
	for (int i = 0; i < 2; i++){
		setNumOf7SegDisplay(0, i, 0);
	}
	for (int i = 2; i < 4; i++){
		setNumOf7SegDisplay(0, i, SPACING_BETWEEN_DIGITS*2);
	}

}

void reloadMainScreen(){
	screenState = 0;
	tft.fillScreen(ILI9341_BLACK);
	initializeFour7SegDisplays();
	drawButton();
	//checkAlarmCond();
}

void saveAlarmMainScreen(){
	Alarm alarm;
	/*for (int i=0; i<4; i++){
		alarm.alarmTime[i] = alarmTime[i];
		Serial.println(alarm.alarmTime[i]);
	}*/
	alarm.h1 = alarmTime[0];
	alarm.h2 = alarmTime[1];
	alarm.m1 = alarmTime[2];
	alarm.m2 = alarmTime[3];
	alarm.state = true;
	int nextAlarmAddress = getNextAlarmAddress();
	Serial.print("size of alarm: ");
	Serial.println(sizeof(alarm));
	saveAlarm(nextAlarmAddress*sizeof(alarm)+sizeof(int), alarm);
	saveNextAlarmAddress(nextAlarmAddress+1);
	//Alarm alarm2;

	/*EEPROM.get(0, alarm2);
	 for (int i=0; i<4; i++){
	 	Serial.println(alarm2.alarmTime[i]);
	}*/
	reloadMainScreen();
	Serial.println("save alarm called");
	checkAlarmCond();
}
void viewAlarmsList(){
	screenState = 2;
	tft.fillScreen(ILI9341_BLACK);

	tft.setTextSize(5);
	tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
	tft.setCursor(0, 200);
	tft.println("X");

	tft.fillTriangle(TFT_WIDTH/2-17, TFT_HEIGHT-35, TFT_WIDTH/2, TFT_HEIGHT, TFT_WIDTH/2+17, TFT_HEIGHT-35,ILI9341_WHITE);

	Serial.println("print alarms");
	Serial.print("EEPROM legnth");
	Serial.println(EEPROM.length());
	numOfAlarms = EEPROM.read(0);
	Serial.print("numer of alarms");
	Serial.println(numOfAlarms);
	for (int i=1; i<numOfAlarms+1; i++){
		Alarm alarm;
		/*for (int i=0; i<4; i++){
			alarm.alarmTime[i] = 0;
		}*/
		EEPROM.get((i-1)*sizeof(alarm)+sizeof(int), alarm);
		//delay(1000);
		/*for (int j=0; j<4; j++){
			Serial.print(alarm.alarmTime[j]);
		}*/
		Serial.print(alarm.h1);
		Serial.print(alarm.h2);
		Serial.print(alarm.m1);
		Serial.print(alarm.m2);

		tft.setTextSize(5);
		//tft.fillRect(0, 50*(i-1), TFT_WIDTH, 35, ILI9341_BLACK);
		if (alarm.state){
			//tft.fillCircle(3*TFT_WIDTH/4, 50*(i-1)+12, 10, ILI9341_RED);
			tft.fillRect(TFT_WIDTH/2, 50*(i-1), TFT_WIDTH/2, 35, GREEN);
		}
		else{
			tft.fillRect(TFT_WIDTH/2, 50*(i-1), TFT_WIDTH/2, 35, ILI9341_RED);
		}
		tft.setCursor(0, 50*(i-1));
		String alarmObjectTimeStr = String();
		alarmObjectTimeStr = String(alarm.h1, HEX) + String(alarm.h2, HEX) + ':' + String(alarm.m1, HEX) + String(alarm.m2, HEX);
		tft.setTextColor(ILI9341_WHITE);
		tft.println(alarmObjectTimeStr);
		//Serial.println();
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
	tft.println("X");
}

void clearEEPROM(){
	for (int i=0; i<EEPROM.length(); i++){
		EEPROM.write(i, 0);
	}
	checkAlarmCond();
}

void changeAlarmState(int alarmNum){
	Alarm alarm;
	EEPROM.get((alarmNum-1)*sizeof(alarm)+sizeof(int), alarm);
	alarm.state = !alarm.state;
	saveAlarm((alarmNum-1)*sizeof(alarm)+sizeof(int), alarm);
	if (alarm.state){
		tft.fillRect(TFT_WIDTH/2, 50*(alarmNum-1), TFT_WIDTH/2, 35, GREEN);
	}
	else{
		tft.fillRect(TFT_WIDTH/2, 50*(alarmNum-1), TFT_WIDTH/2, 35, ILI9341_RED);
	}
	checkAlarmCond();
	delay(1000);
}

void moveAlarmsDown(){

}

void changeAlarmTime(){

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
	Serial.print("touch X");
	Serial.println(touchX);
	Serial.print("touch Y");
	Serial.println(touchY);
	if (screenState == 0){
		bool inRangeCreateAlarmButton = touchX > TFT_WIDTH/2 - BUTTON_WIDTH/2 && touchX< TFT_WIDTH/2 + BUTTON_WIDTH/2;
		inRangeCreateAlarmButton = inRangeCreateAlarmButton && (touchY > TFT_HEIGHT - BUTTON_HEIGHT - 20) && (touchY<TFT_HEIGHT - 20);
		if (inRangeCreateAlarmButton)
		{
			screenState = 1;
			createNewAlarm();
		}

		if (touchY >= TFT_HEIGHT - BUTTON_HEIGHT*2 - 50 && touchY <= TFT_HEIGHT - BUTTON_HEIGHT - 30){
			if (touchX >= TFT_WIDTH/2 - BUTTON_WIDTH/2 && touchX <= TFT_WIDTH/2 - BUTTON_WIDTH/2 + BUTTON_WIDTH &&(alarmListVisible==0)){
				viewAlarmsList();
				//alarmListVisible = 1;
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
						alarmTime[1]=4;
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
			if (touchX >= TFT_WIDTH/2 && touchX <= TFT_WIDTH){
				for (int i=1; i<numOfAlarms+1; i++){
					if ((touchY >= 50*(i-1)) && (touchY <= 50*(i-1)+35)){
						Serial.println(i);
						changeAlarmState(i);
					}
				}
			}
			if (touchX >= TFT_WIDTH/2-17 && touchX <= TFT_WIDTH/2+17){
				if (touchY >= TFT_HEIGHT-35 && touchY <= TFT_HEIGHT){
					moveAlarmsDown();
				}
			}
		}

	delay(100);
}
void setColon(){
	tft.setCursor(120, 15);
	tft.setTextSize(FONTSIZE);
	if (colonState){
		tft.setTextColor(ILI9341_BLACK, ILI9341_BLACK);
		colonState = false;
	}
	else{
		colonState = true;
		tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	}
	tft.print(":");
}


void initializeFour7SegDisplays(){
	setNumOf7SegDisplay(hoursDig1, 0, 0);
	setNumOf7SegDisplay(hoursDig2, 1, 0);
	setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
	setNumOf7SegDisplay(minDig2,3, SPACING_BETWEEN_DIGITS*2);

/*
		for (int i = 0; i < 2; i++){
			setNumOf7SegDisplay(time[i], i, 0);
		}
		for (int i = 2; i < 4; i++){
			setNumOf7SegDisplay(time[i], i, SPACING_BETWEEN_DIGITS*2);
		}*/
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
/*
void soundTheTone(){
	long elapsedTime = 0;
	int halfPeriod = 500/2;
	while (elapsedTime < 10 * 1000){
		digitalWrite(BUZZER, HIGH);
		delayMicroseconds(halfPeriod);
		digitalWrite(BUZZER, LOW);
		delayMicroseconds(halfPeriod);
	}
	digitalWrite(BUZZER, HIGH);
}
*/

void solveThePattern(){
	delay(2000);
	int buttonsReceived[4];
	enum patternStates {waiting, gettingPattern1, gettingPattern2, gettingPattern3, patternCorrect};
	patternStates currentState = waiting;

	while (true){
		digitalWrite(BUZZER, HIGH);
		delayMicroseconds(100);
		digitalWrite(BUZZER, LOW);
		delayMicroseconds(100);
		if(digitalRead(patternPins[0]) == LOW){}
		else if(digitalRead(patternPins[1]) == LOW){}
		else if(digitalRead(patternPins[2]) == LOW){}
		else if(digitalRead(patternPins[patternToSolve[3]]) == LOW){}

		// if (currentState == waiting){
		// 	Serial.println("waiting");
		// 	if (digitalRead(patternPins[patternToSolve[0]]) == LOW){currentState = gettingPattern1;}
		// }
		// else if (currentState == gettingPattern1){
		// 	Serial.println("gettingpat1");
		// 	// user started interacting, start getting the
		// 	if(digitalRead(patternPins[patternToSolve[1]]) == LOW){currentState = gettingPattern2;}
		// 	else if (digitalRead(patternPins[0]) == HIGH && digitalRead(patternPins[1]) == HIGH
		// 				&&digitalRead(patternPins[2]) == HIGH && digitalRead(patternPins[3]) == HIGH){
		// 					// currentState = gettingPattern1;
		// 				}
		// 	if(digitalRead(patternPins[patternToSolve[1]]) == LOW){currentState = gettingPattern2;}
		// 	else if (digitalRead(patternPins[0]) == HIGH && digitalRead(patternPins[1]) == HIGH
		// 				&&digitalRead(patternPins[2]) == HIGH && digitalRead(patternPins[3]) == HIGH){
		// 					currentState = gettingPattern1;
		// 				}
		// 	else{currentState = waiting;}
		// }
		// else if (currentState == gettingPattern2){
		// 	Serial.println("gettingpat2");
		// 	// user started interacting, start getting the pattern
		// 	if(digitalRead(patternPins[patternToSolve[2]]) == LOW){currentState = gettingPattern3;}
		// 	else if (digitalRead(patternPins[0]) == HIGH && digitalRead(patternPins[1]) == HIGH
		// 				&&digitalRead(patternPins[2]) == HIGH && digitalRead(patternPins[3]) == HIGH){
		// 					// currentState = gettingPattern1
		// 					currentState = gettingPattern1;
		// 				}
		// 	else{currentState = waiting;}
		// }
		// else if (currentState == gettingPattern3){
		// 	Serial.println("gettingpat3");
		// 	// user started interacting, start getting the pattern
		// 	if(digitalRead(patternPins[patternToSolve[3]]) == LOW){currentState = patternCorrect;}
		// 	else if (digitalRead(patternPins[0]) == HIGH && digitalRead(patternPins[1]) == HIGH
		// 				&&digitalRead(patternPins[2]) == HIGH && digitalRead(patternPins[3]) == HIGH){
		// 				}
		// 	else{currentState = waiting;}
		// }
		// else if (currentState == patternCorrect){
		// 	Serial.println("Pattern solved!");
		// 	// turn buzzer off
		// 	//digitalWrite(BUZZER, LOW);
		// 	break;
		// }
	}
}
void alarmGoOff(){
	uint8_t randomNumber;
	for (int i = 0; i < 4; i++){
		// gets a "random" number between 0-3 and then adds it to the pattern
		randomNumber = (analogRead(analogPin) % 4);
		patternToSolve[i] = randomNumber;
		digitalWrite(patternLED[patternToSolve[i]], HIGH);
		delay(2000);
		digitalWrite(patternLED[patternToSolve[i]], LOW);
	}
	solveThePattern();
}

void advanceClock(){
	minDig2++;
	if (minDig2==10){
		minDig1++;
		minDig2=0;

		if (!makeAlarm){
			//setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
			//setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);
		}
	}
	else{
		if (!makeAlarm){
			//setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);
		}
	}
	if (minDig1==6){
		hoursDig2++;
		minDig1=0;
		minDig2=0;

		if (screenState == 0){
			//setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
			//setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);

			if (hoursDig2 <10){
				setNumOf7SegDisplay(hoursDig2, 1, 0);
			}
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
		//tft.fillRect(0, 50*(i-1), TFT_WIDTH, 35, ILI9341_BLACK);
		if (hoursDig1 == alarm.h1 && hoursDig2 == alarm.h2 && minDig1 == alarm.m1 && minDig2 == alarm.m2 && alarm.state){
			while (!snooze){
				Serial.println("alarm on");
				alarmGoOff();
				digitalWrite(ALARM_FLASH, HIGH);
				// digitalWrite(BUZZER, HIGH);
				// delay(100);
				digitalWrite(ALARM_FLASH, LOW);
				// digitalWrite(BUZZER, LOW);
				// delay(100);
			}
		}
	}
}

void checkAlarmCond(){
	bool ledOn = false;
	numOfAlarms = EEPROM.read(0);
	for (int i=1; i<numOfAlarms+1; i++){
		Alarm alarm;

		EEPROM.get((i-1)*sizeof(alarm)+sizeof(int), alarm);

		//tft.fillRect(0, 50*(i-1), TFT_WIDTH, 35, ILI9341_BLACK);
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
	// initializeFour7SegDisplays();
	// drawButton();
	//Alarm alarm;
	// for (int i=0; i<4; i++){
	// 	alarm.alarmTime[i] = 5;
	// }
	//saveAlarm(1, alarm);
	//EEPROM.get(0, alarm);
	// for (int i=0; i<4; i++){
	// 	Serial.println(alarm.alarmTime[i]);
	// }

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
		//if (digitalRead(ALARM_ON) == LOW){alarmsOn = !alarmsOn;}
		// check if there's an alarm and if alarms are turned on:
		// if no alarms or alarms turned off : bulb off
		//if(!alarmsOn || EEPROM.read(0) == 0){digitalWrite(alarmLED, LOW);}
		// if alarm present and alarms turned on: bulb on
		// if(EEPROM.read(0) != 0){
		// 	Serial.println("alarm");
		// }
	}

	Serial.end();
	return 0;
}
