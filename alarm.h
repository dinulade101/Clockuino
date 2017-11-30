#ifndef _CREATE_ALARM_H_
#define _CREATE_ALARM_H_

#include <Arduino.h>
#include <EEPROM.h>

class Alarm{
	public:
	//uint8_t alarmTime[3];
	uint8_t h1, h2, m1, m2;
	bool state;
};
void saveAlarm(int eeAddress, Alarm alarmObj);
int getNextAlarmAddress();
void saveNextAlarmAddress(int nextVal);

#endif
