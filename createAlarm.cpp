#include "createAlarm.h"

void createNewAlarm(){}

void saveAlarm(int eeAddress, Alarm alarmObj){
	EEPROM.put(eeAddress, alarmObj);
}
