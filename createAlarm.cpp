#include "createAlarm.h"
#include <EEPROM.h>
#include "alarm.h"

void createAlarm(){

}

void saveAlarm(int eeAddress, Alarm alarmObj){
	EEPROM.put(eeAddress, alarmObj);
}
