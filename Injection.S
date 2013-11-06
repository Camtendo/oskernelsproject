		.section .exceptions.exit.user, "xa"
		// Disable Interrupts
		wrctl status, zero
		// Check timerflag
		call    check_timer_flag
		beq   r2, r0, not_timer
		mov   r5, fp
		mov   r4, sp
		call mythread_scheduler
		// Check if context changes
		beq   r2, r0, no_change
		mov   sp, r2
		mov   fp, r3
		/* Set the new frame pointer */
        no_change:
		not_timer:
		call    reset_timer_flag
		// Enable interrupts
		movi et, 1
		wrctl status, et
