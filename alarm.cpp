#include "alarm.h"

void saveAlarm(int eeAddress, Alarm alarmObj){
	EEPROM.put(eeAddress, alarmObj);
}

//get next alarm address
int getNextAlarmAddress(){
	int nextVal = 0;
	EEPROM.get(0, nextVal);
	return nextVal;
}

void saveNextAlarmAddress(int nextVal){
	EEPROM.put(0, nextVal);
}
