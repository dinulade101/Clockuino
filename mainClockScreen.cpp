#include <Arduino.h>
#include <Adafruit_ILI9341.h>


// initiate time array, to be filled in the format hhmmss
int time[6] = {0};
// i and read are used to make sure we read all 6 digits of time.
int serialReadCounter = 0;
bool read = 0;

#define RESET_TIME_PIN 11

#define TFT_DC 9
#define TFT_CS 10

#define FONTSIZE 11
#define DIGIT_WIDTH 5*FONTSIZE
#define SPACING_BETWEEN_DIGITS 15

bool colonState = false;


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup(){
	init();
	Serial.begin(9600);

	//set RESET_TIME_PIN to input and turn on internal pull up resistor
	pinMode(RESET_TIME_PIN, INPUT);
	digitalWrite(RESET_TIME_PIN, HIGH);
}

void setNumOf7SegDisplay(int digitVal, int digitLoc, int cursorStart){
	tft.setCursor((digitLoc*(DIGIT_WIDTH+SPACING_BETWEEN_DIGITS)+cursorStart), 15);
	tft.setTextSize(FONTSIZE);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.print(digitVal);
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
		for (int i=0; i<2; i++){
			setNumOf7SegDisplay(1, i, 0);
		}
		for (int i=2; i<4; i++){
			setNumOf7SegDisplay(1, i, SPACING_BETWEEN_DIGITS*2);
		}
}

void setTimeToDisplay(){
	for (int i=0; i<2; i++){
		setNumOf7SegDisplay(time[i], i, 0);
	}
	for (int i=2; i<4; i++){
		setNumOf7SegDisplay(time[i], i, SPACING_BETWEEN_DIGITS*2);
	}
}

void downloadTimeFromComputer(){
  while (!read){
		Serial.print("reading");
    if(Serial.available()>0){
      int temp = Serial.read();
      if(temp != ','){
          int num = temp - 48;
          time[serialReadCounter] = num;
          serialReadCounter++;
      }
    }
    if (serialReadCounter == 6)
		{
			read = 1;
			for (int i=0; i<6; i++){
				Serial.print(time[i]);
				setTimeToDisplay();
			}
		}
  }
}

int main(){
	setup();
	//
	 tft.begin();
	 tft.fillScreen(ILI9341_BLACK);
	 tft.setRotation(3);
	 initializeFour7SegDisplays();
	while (true){
		setColon();
		 if (digitalRead(RESET_TIME_PIN)==LOW){
		 	Serial.println("reset pin");
		 	downloadTimeFromComputer();
		 	while (digitalRead(RESET_TIME_PIN)==LOW){Serial.println("stuck in loop");};
		 }
		 read=0;
		 serialReadCounter = 0;
		// //Serial.println(digitalRead(RESET_TIME_PIN));

		//delay(1000);
	}


	for (int i = 0; i < 6; i++){
		Serial.print(time[i]);
	}

	Serial.end();
	return 0;
}
