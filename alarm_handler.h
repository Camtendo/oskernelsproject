/* 
    Name:   alarm_handler.h
    Author: Dongyuan Zhan
    Date:   11/20/2010

Description:
    Functions used to handle things related to alarm interrupts
*/

#include <alt_types.h>

unsigned int is_alarmflag_set();

void reset_alarmflag();

alt_u32 myinterrupt_handler(void* context);

unsigned int start_alarm_succeed();
