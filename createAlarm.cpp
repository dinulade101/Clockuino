#include "createAlarm.h"



void saveAlarm(int eeAddress, Alarm alarmObj){
	EEPROM.put(eeAddress, alarmObj);
}
