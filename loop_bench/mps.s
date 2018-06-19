	.text
	.file	"mps.cpp"
	.globl	_Z6serialPii            # -- Begin function _Z6serialPii
	.p2align	4, 0x90
	.type	_Z6serialPii,@function
_Z6serialPii:                           # @_Z6serialPii
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%rdi, -8(%rbp)
	movl	%esi, -12(%rbp)
	movl	$0, -16(%rbp)
	movl	$-99999, -20(%rbp)      # imm = 0xFFFE7961
	movl	$1, -24(%rbp)
.LBB0_1:                                # %for.cond
                                        # =>This Inner Loop Header: Depth=1
	movl	-24(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jge	.LBB0_6
# %bb.2:                                # %for.body
                                        #   in Loop: Header=BB0_1 Depth=1
	movq	-8(%rbp), %rax
	movl	-24(%rbp), %ecx
	subl	$1, %ecx
	movslq	%ecx, %rdx
	movl	(%rax,%rdx,4), %ecx
	addl	-16(%rbp), %ecx
	movl	%ecx, -16(%rbp)
	movl	-20(%rbp), %ecx
	cmpl	-16(%rbp), %ecx
	jge	.LBB0_4
# %bb.3:                                # %if.then
                                        #   in Loop: Header=BB0_1 Depth=1
	movl	-16(%rbp), %eax
	movl	%eax, -20(%rbp)
.LBB0_4:                                # %if.end
                                        #   in Loop: Header=BB0_1 Depth=1
	jmp	.LBB0_5
.LBB0_5:                                # %for.inc
                                        #   in Loop: Header=BB0_1 Depth=1
	movl	-24(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -24(%rbp)
	jmp	.LBB0_1
.LBB0_6:                                # %for.end
	popq	%rbp
	retq
.Lfunc_end0:
	.size	_Z6serialPii, .Lfunc_end0-_Z6serialPii
	.cfi_endproc
                                        # -- End function

	.ident	"clang version 6.0.0 (trunk 321123)"
	.section	".note.GNU-stack","",@progbits
