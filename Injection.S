		// this //ed code seems to mess things up when un//ed
		/*.section .exceptions.entry.user, "xa"
		call    check_timer_flag
		beq   r2, r0, entry_not_timer
		stw   fp, 84(sp) //fp doesn't actually move....
		entry_not_timer:
		nop*/
		.section .exceptions.exit.user, "xa"
		// Disable Interrupts
		wrctl status, zero
		// Check timerflag
		call    check_timer_flag
		beq   r2, r0, not_timer
		//stw   fp, 84(sp)
		mov   r5, fp
		mov   r4, sp
		call mythread_scheduler
		/* TODO: check if context changes? Maybe a -1 or something if it should not. Or a zero*/
		beq   r2, r0, no_change
		mov   sp, r2
		mov   fp, r3
		/* Set the new frame pointer */
        //ldw   fp, 84(sp)
        no_change:
		not_timer:
		call    reset_timer_flag
		// Enable interrupts
		movi et, 1
		wrctl status, et
