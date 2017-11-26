#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include <EEPROM.h>
#include "createAlarm.h"

// initiate time array, to be filled in the format hhmmss
int time[6] = {0};
// i and read are used to make sure we read all 6 digits of time.
int serialReadCounter = 0;
bool read = 0, colonState = 0;
int hoursDig1 = 0, hoursDig2 = 0, minDig1 = 0, minDig2 = 0;
bool makeAlarm = 0;

#define RESET_TIME_PIN 11
#define BUZZER 12

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

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// a multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

void setup(){
	init();
	Serial.begin(9600);

	//set RESET_TIME_PIN to input and turn on internal pull up resistor
	pinMode(RESET_TIME_PIN, INPUT);
	digitalWrite(RESET_TIME_PIN, HIGH);
	tft.begin();
}
void drawButton(){
	tft.fillRect(TFT_WIDTH/2 - BUTTON_WIDTH/2, TFT_HEIGHT - BUTTON_HEIGHT - 20, BUTTON_WIDTH, BUTTON_HEIGHT, ILI9341_WHITE);
	tft.setTextSize(3);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(TFT_WIDTH/2 - BUTTON_WIDTH/2, TFT_HEIGHT - BUTTON_HEIGHT - 18);
	tft.println("Create Alarm");
}
void buttonClick(){
	TSPoint touch = ts.getPoint();
	if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {return;}
	int touchY = map(touch.x, TS_MINX, TS_MAXX, 0, TFT_HEIGHT - 1);
	int touchX = map(touch.y, TS_MINY, TS_MAXY, TFT_WIDTH - 1, 0);
	bool inRange = touchX > TFT_WIDTH/2 - BUTTON_WIDTH/2 && touchX< TFT_WIDTH/2 - BUTTON_WIDTH/2 + BUTTON_WIDTH;
	inRange = inRange && (touchY > TFT_HEIGHT - BUTTON_HEIGHT - 20) && (touchY<TFT_HEIGHT - BUTTON_HEIGHT - 20+BUTTON_HEIGHT);
	if (inRange){makeAlarm = 1;}
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
void createNewAlarm(){
	tft.fillScreen(ILI9341_BLACK);
	tft.fillRect(115, 90, 100, 40, ILI9341_WHITE);
	tft.setTextSize(4);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(117, 92);
	tft.println("Hour");
	tft.fillTriangle(80, 90, 110, 90, 95, 120,ILI9341_WHITE);
	tft.fillTriangle(225, 120, 255, 120, 240, 90,ILI9341_WHITE);

	tft.fillRect(105, 145, 140, 35, ILI9341_WHITE);
	tft.setTextSize(4);
	tft.setTextColor(ILI9341_BLACK);
	tft.setCursor(107, 147);
	tft.println("Minutes");
	tft.fillTriangle(60, 180, 90, 180, 75, 145,ILI9341_WHITE);
	tft.fillTriangle(255, 145, 285, 145, 270, 180,ILI9341_WHITE);

}

void setNumOf7SegDisplay(int digitVal, int digitLoc, int cursorStart){
	tft.setCursor((digitLoc*(DIGIT_WIDTH+SPACING_BETWEEN_DIGITS)+cursorStart), 15);
	tft.setTextSize(FONTSIZE);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.print(digitVal);
}

void initializeFour7SegDisplays(){
		for (int i = 0; i < 2; i++){
			setNumOf7SegDisplay(time[i], i, 0);
		}
		for (int i = 2; i < 4; i++){
			setNumOf7SegDisplay(time[i], i, SPACING_BETWEEN_DIGITS*2);
		}
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

void advanceClock(){
	minDig2++;
	if (minDig2==10){
		minDig1++;
		minDig2=0;
		setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
		setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);
	}
	else{
		setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);
	}
	if (minDig1==6){
		hoursDig2++;
		minDig1=0;
		minDig2=0;

		setNumOf7SegDisplay(minDig1, 2, SPACING_BETWEEN_DIGITS*2);
		setNumOf7SegDisplay(minDig2, 3, SPACING_BETWEEN_DIGITS*2);
		if (hoursDig2 <10){
			setNumOf7SegDisplay(hoursDig2, 1, 0);
		}
	}

	if (hoursDig2 == 10){
		hoursDig1++;
		hoursDig2 = 0;

		setNumOf7SegDisplay(hoursDig2, 1, 0);
		setNumOf7SegDisplay(hoursDig1, 0, 0);
	}
	if (hoursDig1 == 2 && hoursDig2 == 4){
		hoursDig1 = 0;
		hoursDig2 = 0;
		minDig1 = 0;
		minDig2 = 0;

		setNumOf7SegDisplay(hoursDig1, 0, 0);
		setNumOf7SegDisplay(hoursDig2, 1, 0);
	}
}
void clockMode(){}
int main(){
	setup();
	tft.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(3);
	initializeFour7SegDisplays();
	drawButton();
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
		if (makeAlarm == 1){
			createNewAlarm();
		}
	}

	Serial.end();
	return 0;
}
