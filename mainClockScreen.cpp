#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <TouchScreen.h>
#include "createAlarm.h"
// initiate time array, to be filled in the format hhmmss
int time[6] = {0};
// i and read are used to make sure we read all 6 digits of time.
int serialReadCounter = 0;
bool read = 0, colonState = 0;
int hoursDig1 = 0, hoursDig2 = 0, minDig1 = 0, minDig2 = 0, loopCounter=0;

#define RESET_TIME_PIN 11
#define BUZZER 12

#define TFT_DC 9
#define TFT_CS 10
// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000
// touch screen pins, obtained from the documentaion
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM  5  // can be a digital pin
#define XP  4  // can be a digital pin
// width/height of the display when rotated horizontally
#define TFT_WIDTH 320
#define TFT_HEIGHT 240
#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 50
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
}
void drawButton(){
	tft.drawRect(TFT_WIDTH/2 - BUTTON_WIDTH, TFT_HEIGHT/2 - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT, ILI9341_WHITE);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	//tft.println("Create Alarm");
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

void setNumOf7SegDisplay(int digitVal, int digitLoc, int cursorStart){
	tft.setCursor((digitLoc*(DIGIT_WIDTH+SPACING_BETWEEN_DIGITS)+cursorStart), 15);
	tft.setTextSize(FONTSIZE);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.print(digitVal);
}

void initializeFour7SegDisplays(){
		for (int i = 0; i < 4; i++){
			setNumOf7SegDisplay(time[i], i, 0);
		}
}

void setTimeToDisplay(){
	hoursDig1 = time[0];
	hoursDig2 = time[1];
	minDig1 = time[2];
	minDig2 = time[3];
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

int main(){
	setup();
	tft.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(3);
	downloadTimeFromComputer();
	for (int i = 0; i < 6; i++){
		Serial.print(time[i]);
	}
	setTimeToDisplay();
	initializeFour7SegDisplays();
	//drawButton();
	// while (true){
	// 	read=0;
	// 	serialReadCounter = 0;
	// 	loopCounter++;
	// 	if (loopCounter == 60){
	// 		loopCounter = 0;
	// 		advanceClock();
	// 	}
	// }

	Serial.end();
	return 0;
}
