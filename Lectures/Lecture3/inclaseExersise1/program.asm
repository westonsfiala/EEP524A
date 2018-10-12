	.text
	.file	"main"
	.globl	mmul_v1
	.align	16, 0x90
	.type	mmul_v1,@function
mmul_v1:
	.cfi_startproc
	pushq	%r15
.Ltmp0:
	.cfi_def_cfa_offset 16
	pushq	%r14
.Ltmp1:
	.cfi_def_cfa_offset 24
	pushq	%r13
.Ltmp2:
	.cfi_def_cfa_offset 32
	pushq	%r12
.Ltmp3:
	.cfi_def_cfa_offset 40
	pushq	%rsi
.Ltmp4:
	.cfi_def_cfa_offset 48
	pushq	%rdi
.Ltmp5:
	.cfi_def_cfa_offset 56
	pushq	%rbp
.Ltmp6:
	.cfi_def_cfa_offset 64
	pushq	%rbx
.Ltmp7:
	.cfi_def_cfa_offset 72
	subq	$80, %rsp
.Ltmp8:
	.cfi_def_cfa_offset 152
.Ltmp9:
	.cfi_offset %rbx, -72
.Ltmp10:
	.cfi_offset %rbp, -64
.Ltmp11:
	.cfi_offset %rdi, -56
.Ltmp12:
	.cfi_offset %rsi, -48
.Ltmp13:
	.cfi_offset %r12, -40
.Ltmp14:
	.cfi_offset %r13, -32
.Ltmp15:
	.cfi_offset %r14, -24
.Ltmp16:
	.cfi_offset %r15, -16
	movq	(%rdx), %rdi
	leaq	1(%rdi), %rax
	cmpq	%rax, 136(%rcx)
	movslq	(%rcx), %r8
	sete	%al
	movzbl	%al, %eax
	testq	%r8, %r8
	jle	.LBB0_15
	movq	88(%rcx), %rbx
	movq	%rbx, 8(%rsp)
	movq	40(%rcx), %rbp
	movq	%rbp, 24(%rsp)
	movq	8(%rdx), %rdx
	imulq	%rdi, %rbx
	movq	%rdi, 16(%rsp)
	addq	%rbp, %rbx
	movq	%rbx, 32(%rsp)
	movq	96(%rcx), %rbp
	imulq	%rdx, %rbp
	addq	48(%rcx), %rbp
	movq	%rbp, 56(%rsp)
	movq	8(%rcx), %rbp
	movq	%rbp, 48(%rsp)
	movq	16(%rcx), %r15
	movq	24(%rcx), %rsi
	leaq	(%rax,%rax,2), %rax
	movq	88(%rcx,%rax,8), %rax
	movq	%rax, 40(%rsp)
	addq	$1, %rdx
	cmpq	%rdx, 144(%rcx)
	leaq	32(%rcx), %rax
	sete	%cl
	movzbl	%cl, %ecx
	leaq	(%rcx,%rcx,2), %rcx
	movq	64(%rax,%rcx,8), %r9
	movq	%r9, %rcx
	sarq	$3, %rcx
	movq	%r9, %rax
	andq	$-8, %rax
	movq	%rax, (%rsp)
	testq	%rcx, %rcx
	je	.LBB0_8
	leaq	(,%r8,4), %rdi
	movq	8(%rsp), %rax
	movl	%eax, %edx
	movq	16(%rsp), %rax
	imull	%eax, %edx
	movq	24(%rsp), %rax
	addl	%eax, %edx
	imull	%r8d, %edx
	xorl	%eax, %eax
	movq	32(%rsp), %r11
	.align	16, 0x90
.LBB0_3:
	movq	%rax, 64(%rsp)
	movl	%edx, 76(%rsp)
	movslq	%edx, %rax
	movq	48(%rsp), %rdx
	leaq	(%rdx,%rax,4), %rbp
	movl	%r11d, %r10d
	imull	%r8d, %r10d
	xorl	%r13d, %r13d
	movq	56(%rsp), %rbx
	.align	16, 0x90
.LBB0_4:
	movslq	%ebx, %rdx
	leaq	(%r15,%rdx,4), %rax
	addl	%r10d, %edx
	movslq	%edx, %rdx
	vmovups	(%rsi,%rdx,4), %ymm0
	movq	%rbp, %r12
	movl	%r8d, %r14d
	.align	16, 0x90
.LBB0_5:
	vbroadcastss	(%r12), %ymm1
	vfmadd231ps	(%rax), %ymm1, %ymm0
	vmovups	%ymm0, (%rsi,%rdx,4)
	addq	%rdi, %rax
	addq	$4, %r12
	addl	$-1, %r14d
	jne	.LBB0_5
	addq	$1, %r13
	addq	$8, %rbx
	cmpq	%rcx, %r13
	jne	.LBB0_4
	movq	64(%rsp), %rax
	addq	$1, %rax
	addq	$1, %r11
	movl	76(%rsp), %edx
	addl	%r8d, %edx
	cmpq	40(%rsp), %rax
	jne	.LBB0_3
.LBB0_8:
	movq	(%rsp), %rcx
	cmpq	%rcx, %r9
	movq	24(%rsp), %r13
	movq	32(%rsp), %rdx
	movq	16(%rsp), %rax
	je	.LBB0_15
	addq	%rcx, 56(%rsp)
	subq	%rcx, %r9
	movq	%r8, %rcx
	shlq	$2, %rcx
	movq	8(%rsp), %rbp
	imull	%eax, %ebp
	addl	%ebp, %r13d
	imull	%r8d, %r13d
	xorl	%r10d, %r10d
	.align	16, 0x90
.LBB0_10:
	movq	%rdx, 32(%rsp)
	movslq	%r13d, %rax
	movq	48(%rsp), %rbp
	leaq	(%rbp,%rax,4), %r11
	movl	%edx, %r14d
	imull	%r8d, %r14d
	xorl	%r12d, %r12d
	movq	56(%rsp), %rax
	.align	16, 0x90
.LBB0_11:
	movslq	%eax, %rdi
	leaq	(%r15,%rdi,4), %rdx
	addl	%r14d, %edi
	movslq	%edi, %rbp
	vmovss	(%rsi,%rbp,4), %xmm0
	movq	%r11, %rdi
	movl	%r8d, %ebx
	.align	16, 0x90
.LBB0_12:
	vmovss	(%rdx), %xmm1
	vfmadd231ss	(%rdi), %xmm1, %xmm0
	vmovss	%xmm0, (%rsi,%rbp,4)
	addq	%rcx, %rdx
	addq	$4, %rdi
	addl	$-1, %ebx
	jne	.LBB0_12
	addq	$1, %r12
	addq	$1, %rax
	cmpq	%r9, %r12
	jne	.LBB0_11
	addq	$1, %r10
	movq	32(%rsp), %rdx
	addq	$1, %rdx
	addl	%r8d, %r13d
	cmpq	40(%rsp), %r10
	jne	.LBB0_10
.LBB0_15:
	addq	$80, %rsp
	popq	%rbx
	popq	%rbp
	popq	%rdi
	popq	%rsi
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	vzeroupper
	retq
.Lfunc_end0:
	.size	mmul_v1, .Lfunc_end0-mmul_v1
	.cfi_endproc

	.globl	mmul_v2
	.align	16, 0x90
	.type	mmul_v2,@function
mmul_v2:
	.cfi_startproc
	pushq	%r15
.Ltmp17:
	.cfi_def_cfa_offset 16
	pushq	%r14
.Ltmp18:
	.cfi_def_cfa_offset 24
	pushq	%r13
.Ltmp19:
	.cfi_def_cfa_offset 32
	pushq	%r12
.Ltmp20:
	.cfi_def_cfa_offset 40
	pushq	%rsi
.Ltmp21:
	.cfi_def_cfa_offset 48
	pushq	%rdi
.Ltmp22:
	.cfi_def_cfa_offset 56
	pushq	%rbp
.Ltmp23:
	.cfi_def_cfa_offset 64
	pushq	%rbx
.Ltmp24:
	.cfi_def_cfa_offset 72
	subq	$80, %rsp
.Ltmp25:
	.cfi_def_cfa_offset 152
.Ltmp26:
	.cfi_offset %rbx, -72
.Ltmp27:
	.cfi_offset %rbp, -64
.Ltmp28:
	.cfi_offset %rdi, -56
.Ltmp29:
	.cfi_offset %rsi, -48
.Ltmp30:
	.cfi_offset %r12, -40
.Ltmp31:
	.cfi_offset %r13, -32
.Ltmp32:
	.cfi_offset %r14, -24
.Ltmp33:
	.cfi_offset %r15, -16
	movl	(%rcx), %r13d
	movslq	%r13d, %rax
	movq	8(%rcx), %rbp
	movq	%rbp, 48(%rsp)
	movq	16(%rcx), %r10
	movq	24(%rcx), %r11
	movq	88(%rcx), %rbx
	movq	%rbx, 16(%rsp)
	movq	40(%rcx), %rbp
	movq	%rbp, 32(%rsp)
	movq	(%rdx), %rdi
	movq	%rdi, 8(%rsp)
	movq	8(%rdx), %rdx
	imulq	%rdi, %rbx
	addq	%rbp, %rbx
	movq	%rbx, 24(%rsp)
	movq	96(%rcx), %rbp
	imulq	%rdx, %rbp
	addq	48(%rcx), %rbp
	movq	%rbp, 56(%rsp)
	leaq	1(%rdi), %rbp
	cmpq	%rbp, 136(%rcx)
	sete	%bl
	movzbl	%bl, %ebp
	leaq	(%rbp,%rbp,2), %rbp
	addq	$1, %rdx
	cmpq	%rdx, 144(%rcx)
	movq	88(%rcx,%rbp,8), %rdx
	movq	%rdx, 40(%rsp)
	sete	%dl
	movzbl	%dl, %edx
	leaq	(%rdx,%rdx,2), %rdx
	movq	96(%rcx,%rdx,8), %r15
	movq	%r15, %rcx
	sarq	$3, %rcx
	movq	%r15, %rdx
	andq	$-8, %rdx
	movq	%rdx, (%rsp)
	testq	%rcx, %rcx
	je	.LBB1_8
	leaq	(,%rax,4), %rbp
	movq	16(%rsp), %rdx
	movl	%edx, %edi
	movq	8(%rsp), %rdx
	imull	%edx, %edi
	movq	32(%rsp), %rdx
	addl	%edx, %edi
	imull	%r13d, %edi
	xorl	%edx, %edx
	movq	24(%rsp), %rbx
	.align	16, 0x90
.LBB1_2:
	movq	%rdx, 64(%rsp)
	movl	%edi, 76(%rsp)
	movslq	%edi, %rdx
	movq	48(%rsp), %rdi
	leaq	(%rdi,%rdx,4), %r14
	movl	%ebx, %r12d
	imull	%r13d, %r12d
	xorl	%edx, %edx
	movq	56(%rsp), %rsi
	.align	16, 0x90
.LBB1_3:
	vxorps	%ymm0, %ymm0, %ymm0
	testl	%r13d, %r13d
	jle	.LBB1_6
	movslq	%esi, %rdi
	leaq	(%r10,%rdi,4), %rdi
	movq	%r14, %r9
	movl	%r13d, %r8d
	.align	16, 0x90
.LBB1_5:
	vbroadcastss	(%r9), %ymm1
	vfmadd231ps	(%rdi), %ymm1, %ymm0
	addq	%rbp, %rdi
	addq	$4, %r9
	addl	$-1, %r8d
	jne	.LBB1_5
.LBB1_6:
	leal	(%r12,%rsi), %edi
	movslq	%edi, %rdi
	vaddps	(%r11,%rdi,4), %ymm0, %ymm0
	vmovups	%ymm0, (%r11,%rdi,4)
	addq	$1, %rdx
	addq	$8, %rsi
	cmpq	%rcx, %rdx
	jne	.LBB1_3
	movq	64(%rsp), %rdx
	addq	$1, %rdx
	addq	$1, %rbx
	movl	76(%rsp), %edi
	addl	%r13d, %edi
	cmpq	40(%rsp), %rdx
	jne	.LBB1_2
.LBB1_8:
	movq	(%rsp), %rcx
	cmpq	%rcx, %r15
	movq	32(%rsp), %r14
	movq	24(%rsp), %r12
	je	.LBB1_16
	addq	%rcx, 56(%rsp)
	subq	%rcx, %r15
	shlq	$2, %rax
	movq	16(%rsp), %rcx
	movq	8(%rsp), %rdx
	imull	%edx, %ecx
	addl	%ecx, %r14d
	imull	%r13d, %r14d
	xorl	%r9d, %r9d
	.align	16, 0x90
.LBB1_10:
	movslq	%r14d, %rcx
	movq	48(%rsp), %rdx
	leaq	(%rdx,%rcx,4), %r8
	movl	%r12d, %edx
	imull	%r13d, %edx
	xorl	%esi, %esi
	movq	56(%rsp), %rdi
	.align	16, 0x90
.LBB1_11:
	vxorps	%xmm0, %xmm0, %xmm0
	testl	%r13d, %r13d
	jle	.LBB1_14
	movslq	%edi, %rcx
	leaq	(%r10,%rcx,4), %rcx
	movq	%r8, %rbp
	movl	%r13d, %ebx
	.align	16, 0x90
.LBB1_13:
	vmovss	(%rcx), %xmm1
	vfmadd231ss	(%rbp), %xmm1, %xmm0
	addq	%rax, %rcx
	addq	$4, %rbp
	addl	$-1, %ebx
	jne	.LBB1_13
.LBB1_14:
	leal	(%rdx,%rdi), %ecx
	movslq	%ecx, %rcx
	vaddss	(%r11,%rcx,4), %xmm0, %xmm0
	vmovss	%xmm0, (%r11,%rcx,4)
	addq	$1, %rsi
	addq	$1, %rdi
	cmpq	%r15, %rsi
	jne	.LBB1_11
	addq	$1, %r9
	addq	$1, %r12
	addl	%r13d, %r14d
	cmpq	40(%rsp), %r9
	jne	.LBB1_10
.LBB1_16:
	addq	$80, %rsp
	popq	%rbx
	popq	%rbp
	popq	%rdi
	popq	%rsi
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	vzeroupper
	retq
.Lfunc_end1:
	.size	mmul_v2, .Lfunc_end1-mmul_v2
	.cfi_endproc


	.ident	"clang version 3.8.1 "
	.section	".note.GNU-stack","",@progbits
