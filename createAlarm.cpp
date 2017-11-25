#include "createAlarm.h"

int createNewAlarm(int x){return x;}

void saveAlarm(int eeAddress, Alarm alarmObj){
	EEPROM.put(eeAddress, alarmObj);
}
