#include <Arduino.h>
#include <Adafruit_ILI9341.h>


// initiate time array, to be filled in the format hhmmss
int time[6] = {0};
// i and read are used to make sure we read all 6 digits of time.
int i = 0;
bool read = 0;

#define TFT_DC 9
#define TFT_CS 10

#define fontSize 11
#define digitWidth 5*fontSize
#define spacingBetweenDigits 15

bool colonState = false;


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup(){
	init();
	Serial.begin(9600);
}

void setNumOf7SegDisplay(int digitVal, int digitLoc, int cursorStart){
	tft.setCursor((digitLoc*(digitWidth+spacingBetweenDigits)+cursorStart), 15);
	tft.setTextSize(fontSize);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.print(digitVal);
}

void setColon(){
	tft.setCursor(120, 15);
	tft.setTextSize(fontSize);
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
	for (int j=0; j<8; j++){
		for (int i=0; i<2; i++){
			setNumOf7SegDisplay(j, i, 0);
		}
		for (int i=2; i<4; i++){
			setNumOf7SegDisplay(j+1, i, spacingBetweenDigits*2);
		}
	}

}

void loop(){
  while (!read){
    if(Serial.available()>0){
      int temp = Serial.read();
      if(temp != ','){
          int num = temp - 48;
          time[i] = num;
          i++;
      }
    }
    if (i == 6){read = 1;}
  }
}

int main(){
	setup();

	tft.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(3);
	initializeFour7SegDisplays();

	while (true){
		setColon();
		delay(100);
	}

	//loop();

	for (int i = 0; i < 6; i++){
		Serial.print(time[i]);
	}

	Serial.end();
	return 0;
}
