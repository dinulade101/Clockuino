#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ILI9341.h>
#include "sendTime.h"

// initiate time array, to be filled in the format hhmmss
int time[6] = {0};
// i and read are used to make sure we read all 6 digits of time.
int i = 0;
bool read = 0;

void setup(){
	init();
	Serial.begin(9600);

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
	sendTime();
	loop();

	for (int i = 0; i < 6; i++){
		Serial.print(time[i];)
	}

	Serial.end();
	return 0;
}
