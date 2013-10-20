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
	//pop %eax
	//call __lwt_start_test
	//ret

	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl    16(%ebp), %esp
	movl	12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	8(%ebp), %eax
	movl	%eax, (%esp)
	call	__lwt_start
	leave
	ret
