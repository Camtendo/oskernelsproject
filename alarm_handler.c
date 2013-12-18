#include <stdio.h>
#include <sys/alt_alarm.h>

#include "alarm_handler.h"
#include "thread_handler.h"

/* a local alarm instance */
static alt_alarm alarm;

/* a global flag for the alarm interrupt */
unsigned int alarmflag = 0;

/* test if "alarmflag" is set */
unsigned int is_alarmflag_set()
{
    return alarmflag != 0;
}

/* reset "alarmflag" */
void reset_alarmflag()
{
    alarmflag = 0;
}

/* the alarm interrupt handler */
alt_u32 myinterrupt_handler(void* context)
{
    alarmflag = 1;
    if(mythread_isQempty())
    {
        printf("Interrupted by the DE2 timer!\n");
    }
    
    return alt_ticks_per_second();
}

unsigned int start_alarm_succeed()
{
    return alt_alarm_start(&alarm, alt_ticks_per_second(), myinterrupt_handler, NULL) >= 0;
}
