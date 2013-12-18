/* 
 	Name: 	exception_handler.S
 	Author: Dongyuan Zhan
 	Date:	11/20/2010

Description:
	This section will be invoked at the end of every exception.
 	When it gets invoked, the flag "alarmflag" is checked.
 	If the interrupt was fired by the alarm, 
 		then "alarmflag" is reset and "mythread_scheduler" is called.
	Otherwise, exit.
*/

.section .exceptions.exit.user, "xa"  	/* assembler directive to place code in the corresponding section */
call is_alarmflag_set					/* check the alarmflag */
beq  r2, r0, jump_here				   	/* if alarmflag has been set, then do (1) and (2); else, goto exit_here*/
call reset_alarmflag					/* (1) reset alarmflag */
mov  r4, sp								/* pass the current stack pointer to mythread_scheduler */
call mythread_schedule					/* (2) call mythread_scheduler to save the current stack pointer and return the next stack pointer */
stw  fp, -4(sp)							/* save the current frame pointer on top of the current stack */
mov  sp, r2								/* load the next/new stack pointer */
ldw  fp, -4(sp)							/* load the next/new frame pointer */
jump_here:								/* in the following, there is certain code that will be executed to restore the context from the stack pointed by the new sp
										   at the section ".exceptions.exit.label" in altera\90sp2\nios2eds\components\altera_nios2\HAL\src\alt_exception_entry.S */

