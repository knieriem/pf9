/* Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */

.globl getmcontext
getmcontext:
	mov	4(%esp), %eax

	mov	%fs, 8(%eax)
	mov	%es, 12(%eax)
	mov	%ds, 16(%eax)
	mov	%ss, 76(%eax)
	movl	%edi, 20(%eax)
	movl	%esi, 24(%eax)
	movl	%ebp, 28(%eax)
	movl	%ebx, 36(%eax)
	movl	%edx, 40(%eax)		 
	movl	%ecx, 44(%eax)

	movl	$1, 48(%eax)	/* %eax */
	movl	(%esp), %ecx	/* %eip */
	movl	%ecx, 60(%eax)
	leal	4(%esp), %ecx	/* %esp */
	movl	%ecx, 72(%eax)

	movl	44(%eax), %ecx	/* restore %ecx */
	movl	$0, %eax
	ret

.globl setmcontext
setmcontext:
	movl	4(%esp), %eax

	mov	8(%eax), %fs
	mov	12(%eax), %es
	mov	16(%eax), %ds
	mov	76(%eax), %ss
	movl	20(%eax), %edi
	movl	24(%eax), %esi
	movl	28(%eax), %ebp
	movl	36(%eax), %ebx
	movl	40(%eax), %edx
	movl	44(%eax), %ecx

	movl	72(%eax), %esp
	pushl	60(%eax)	/* new %eip */
	movl	48(%eax), %eax
	ret

