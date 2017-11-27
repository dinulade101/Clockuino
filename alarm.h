#ifndef _CREATE_ALARM_H_
#define _CREATE_ALARM_H_

#include <Arduino.h>
#include <EEPROM.h>

class Alarm{
	public:
	uint8_t alarmTime[3];
};
void saveAlarm(int eeAddress, Alarm alarmObj);
int getNextAlarmAddress();
void saveNextAlarmAddress(int nextVal);

#endif
