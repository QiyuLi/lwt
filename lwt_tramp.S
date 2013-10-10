.text
.align 16
.globl __lwt_trampoline
__lwt_trampoline:
	movl $0, %ebp
	call __lwt_start
	// will cause a fault...you should never get here!
	movl $0, %eax
	movl (%eax), %ebx

.align 16
.globl __lwt_trampoline_test
__lwt_trampoline_test:
	pop %eax
	call __lwt_start_test
	ret
