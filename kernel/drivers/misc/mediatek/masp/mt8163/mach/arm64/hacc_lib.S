	.cpu generic+fp+simd
	.file	"hacc_lib.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.align	2
	.global	masp_hal_secure_algo_init
	.type	masp_hal_secure_algo_init, %function
masp_hal_secure_algo_init:
.LFB447:
	.file 1 "dummy"
	.loc 1 60 0
	.cfi_startproc
	.loc 1 64 0
	mov	w0, 1
	ret
	.cfi_endproc
.LFE447:
	.size	masp_hal_secure_algo_init, .-masp_hal_secure_algo_init
	.align	2
	.global	masp_hal_secure_algo_deinit
	.type	masp_hal_secure_algo_deinit, %function
masp_hal_secure_algo_deinit:
.LFB448:
	.loc 1 70 0
	.cfi_startproc
	.loc 1 74 0
	mov	w0, 1
	ret
	.cfi_endproc
.LFE448:
	.size	masp_hal_secure_algo_deinit, .-masp_hal_secure_algo_deinit
	.align	2
	.global	masp_hal_secure_algo
	.type	masp_hal_secure_algo, %function
masp_hal_secure_algo:
.LFB449:
	.loc 1 87 0
	.cfi_startproc
.LVL0:
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	stp	x19, x20, [sp,16]
	stp	x21, x22, [sp,32]
	str	x23, [sp,48]
	.cfi_offset 19, -48
	.cfi_offset 20, -40
	.cfi_offset 21, -32
	.cfi_offset 22, -24
	.cfi_offset 23, -16
	.loc 1 87 0
	uxtb	w21, w0
.LVL1:
	mov	w19, w2
	mov	x22, x3
	mov	x20, x4
	.loc 1 92 0
	uxtw	x23, w1
.LVL2:
.L4:
	.loc 1 101 0 discriminator 1
	bl	osal_hacc_lock
.LVL3:
	.loc 1 102 0 discriminator 1
	cbnz	w0, .L4
	.loc 1 106 0
	mov	x0, x22
	mov	w1, 16
	bl	masp_hal_sp_hacc_init
.LVL4:
	mov	w22, w0
.LVL5:
	.loc 1 108 0
	cbnz	w0, .L5
	.loc 1 118 0
	cbz	w21, .L6
	cmp	w21, 1
	bne	.L16
	.loc 1 124 0
	mov	x0, x23
	mov	w1, w19
	mov	w2, w21
	mov	w3, 2
	mov	w4, w22
	bl	masp_hal_sp_hacc_enc
.LVL6:
.L8:
	.loc 1 141 0
	mov	x3, 0
	cbz	w19, .L10
.LVL7:
.L13:
	.loc 1 143 0 discriminator 3
	ldrb	w5, [x0,x3]
	strb	w5, [x20,x3]
	add	x3, x3, 1
	.loc 1 141 0 discriminator 3
	cmp	w19, w3
	bhi	.L13
.L10:
	.loc 1 147 0
	bl	osal_hacc_unlock
.LVL8:
	.loc 1 158 0
	ldr	x23, [sp,48]
	.cfi_remember_state
	.cfi_restore 23
.LVL9:
	ldp	x19, x20, [sp,16]
	.cfi_restore 20
	.cfi_restore 19
.LVL10:
	ldp	x21, x22, [sp,32]
	.cfi_restore 22
	.cfi_restore 21
.LVL11:
	ldp	x29, x30, [sp], 64
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
.LVL12:
.L16:
	.cfi_restore_state
	.loc 1 135 0
	mov	w22, 24576
.LVL13:
.L5:
	.loc 1 153 0
	bl	osal_hacc_unlock
.LVL14:
	.loc 1 155 0
	adrp	x1, .LC1
	adrp	x0, .LC0
	mov	w2, w22
	add	x1, x1, :lo12:.LC1
	add	x0, x0, :lo12:.LC0
	bl	printk
.LVL15:
	.loc 1 156 0
	adrp	x0, .LC2
	adrp	x1, .LC3
	adrp	x3, .LANCHOR0
	add	x0, x0, :lo12:.LC2
	add	x1, x1, :lo12:.LC3
	mov	w2, 156
	add	x3, x3, :lo12:.LANCHOR0
	bl	printk
.LVL16:
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	panic
.LVL17:
.L6:
	.loc 1 131 0
	mov	x0, x23
.LVL18:
	mov	w1, w19
	mov	w2, 1
	mov	w3, 2
	mov	w4, w21
	bl	masp_hal_sp_hacc_dec
.LVL19:
	.loc 1 132 0
	b	.L8
	.cfi_endproc
.LFE449:
	.size	masp_hal_secure_algo, .-masp_hal_secure_algo
	.section	.rodata
	.align	3
.LANCHOR0 = . + 0
	.type	__func__.5800, %object
	.size	__func__.5800, 21
__func__.5800:
	.string	"masp_hal_secure_algo"
	.section	.rodata.str1.8,"aMS",%progbits,1
	.align	3
.LC0:
	.string	"[%s] masp_hal_secure_algo error (0x%x)\n"
.LC1:
	.string	"CE"
	.zero	5
.LC2:
	.string	"BUG: failure at %s:%d/%s()!\n"
	.zero	3
.LC3:
	.string	"dummy"
	.zero	1
.LC4:
	.string	"BUG!"
	.text
.Letext0:
	.file 2 "dummy"
	.file 3 "dummy"
	.file 4 "dummy"
	.file 5 "dummy"
	.file 6 "dummy"
	.file 7 "dummy"
	.file 8 "dummy"
	.file 9 "dummy"
	.file 10 "dummy"
	.file 11 "dummy"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x3cf
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF42
	.byte	0x1
	.4byte	.LASF43
	.4byte	.LASF44
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF0
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x3
	.byte	0x8
	.4byte	0x41
	.uleb128 0x4
	.4byte	0x46
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF3
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF4
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF7
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.uleb128 0x6
	.string	"u64"
	.byte	0xa
	.byte	0x19
	.4byte	0x7e
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF10
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF11
	.uleb128 0x7
	.4byte	.LASF12
	.byte	0x2
	.byte	0xa2
	.4byte	0x85
	.uleb128 0x3
	.byte	0x8
	.4byte	0x62
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF13
	.uleb128 0x8
	.byte	0x4
	.byte	0x3
	.byte	0x12
	.4byte	0xd7
	.uleb128 0x9
	.4byte	.LASF14
	.sleb128 0
	.uleb128 0x9
	.4byte	.LASF15
	.sleb128 1
	.uleb128 0x9
	.4byte	.LASF16
	.sleb128 2
	.uleb128 0x9
	.4byte	.LASF17
	.sleb128 3
	.byte	0
	.uleb128 0x7
	.4byte	.LASF18
	.byte	0x3
	.byte	0x17
	.4byte	0xb6
	.uleb128 0x2
	.byte	0x10
	.byte	0x7
	.4byte	.LASF19
	.uleb128 0x7
	.4byte	.LASF20
	.byte	0x4
	.byte	0xe
	.4byte	0x4d
	.uleb128 0x7
	.4byte	.LASF21
	.byte	0x4
	.byte	0xf
	.4byte	0x62
	.uleb128 0xa
	.4byte	.LASF22
	.byte	0x1
	.byte	0x3b
	.4byte	0x62
	.8byte	.LFB447
	.8byte	.LFE447-.LFB447
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x12c
	.uleb128 0xb
	.string	"ret"
	.byte	0x1
	.byte	0x3d
	.4byte	0x97
	.byte	0
	.uleb128 0xa
	.4byte	.LASF23
	.byte	0x1
	.byte	0x45
	.4byte	0x62
	.8byte	.LFB448
	.8byte	.LFE448-.LFB448
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x159
	.uleb128 0xb
	.string	"ret"
	.byte	0x1
	.byte	0x47
	.4byte	0x97
	.byte	0
	.uleb128 0xc
	.4byte	.LASF45
	.byte	0x1
	.byte	0x56
	.8byte	.LFB449
	.8byte	.LFE449-.LFB449
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x2a5
	.uleb128 0xd
	.4byte	.LASF24
	.byte	0x1
	.byte	0x56
	.4byte	0x62
	.4byte	.LLST0
	.uleb128 0xd
	.4byte	.LASF25
	.byte	0x1
	.byte	0x56
	.4byte	0x4d
	.4byte	.LLST1
	.uleb128 0xd
	.4byte	.LASF26
	.byte	0x1
	.byte	0x56
	.4byte	0x4d
	.4byte	.LLST2
	.uleb128 0xd
	.4byte	.LASF27
	.byte	0x1
	.byte	0x56
	.4byte	0xa9
	.4byte	.LLST3
	.uleb128 0xd
	.4byte	.LASF28
	.byte	0x1
	.byte	0x56
	.4byte	0xa9
	.4byte	.LLST4
	.uleb128 0xe
	.string	"err"
	.byte	0x1
	.byte	0x58
	.4byte	0xe9
	.4byte	.LLST5
	.uleb128 0xb
	.string	"src"
	.byte	0x1
	.byte	0x59
	.4byte	0x2a5
	.uleb128 0xe
	.string	"dst"
	.byte	0x1
	.byte	0x59
	.4byte	0x2a5
	.4byte	.LLST6
	.uleb128 0xf
	.4byte	.LASF29
	.byte	0x1
	.byte	0x59
	.4byte	0x2a5
	.uleb128 0xb
	.string	"i"
	.byte	0x1
	.byte	0x5a
	.4byte	0xe9
	.uleb128 0x10
	.4byte	.LASF30
	.byte	0x1
	.byte	0x5c
	.4byte	0x7e
	.4byte	.LLST7
	.uleb128 0x11
	.4byte	.LASF46
	.byte	0x1
	.byte	0x97
	.8byte	.L5
	.uleb128 0x12
	.4byte	.LASF31
	.4byte	0x2bb
	.uleb128 0x9
	.byte	0x3
	.8byte	__func__.5800
	.uleb128 0x13
	.8byte	.LVL3
	.4byte	0x330
	.uleb128 0x13
	.8byte	.LVL4
	.4byte	0x33b
	.uleb128 0x13
	.8byte	.LVL6
	.4byte	0x355
	.uleb128 0x13
	.8byte	.LVL8
	.4byte	0x37e
	.uleb128 0x13
	.8byte	.LVL14
	.4byte	0x37e
	.uleb128 0x13
	.8byte	.LVL15
	.4byte	0x385
	.uleb128 0x13
	.8byte	.LVL16
	.4byte	0x385
	.uleb128 0x13
	.8byte	.LVL17
	.4byte	0x39b
	.uleb128 0x13
	.8byte	.LVL19
	.4byte	0x3ad
	.byte	0
	.uleb128 0x3
	.byte	0x8
	.4byte	0xf4
	.uleb128 0x14
	.4byte	0x46
	.4byte	0x2bb
	.uleb128 0x15
	.4byte	0x34
	.byte	0x14
	.byte	0
	.uleb128 0x4
	.4byte	0x2ab
	.uleb128 0x14
	.4byte	0x54
	.4byte	0x2cb
	.uleb128 0x16
	.byte	0
	.uleb128 0x17
	.4byte	.LASF32
	.byte	0x5
	.byte	0x24
	.4byte	0x2c0
	.uleb128 0x14
	.4byte	0x46
	.4byte	0x2e1
	.uleb128 0x16
	.byte	0
	.uleb128 0x18
	.4byte	.LASF33
	.byte	0x6
	.2byte	0x1b2
	.4byte	0x2ed
	.uleb128 0x4
	.4byte	0x2d6
	.uleb128 0x17
	.4byte	.LASF34
	.byte	0x7
	.byte	0x61
	.4byte	0x9e
	.uleb128 0x14
	.4byte	0x2d
	.4byte	0x313
	.uleb128 0x15
	.4byte	0x34
	.byte	0x40
	.uleb128 0x15
	.4byte	0x34
	.byte	0
	.byte	0
	.uleb128 0x18
	.4byte	.LASF35
	.byte	0x8
	.2byte	0x2f9
	.4byte	0x31f
	.uleb128 0x4
	.4byte	0x2fd
	.uleb128 0x18
	.4byte	.LASF36
	.byte	0x9
	.2byte	0x1db
	.4byte	0x2d
	.uleb128 0x19
	.4byte	.LASF47
	.byte	0xb
	.byte	0x16
	.4byte	0x54
	.uleb128 0x1a
	.4byte	.LASF37
	.byte	0x3
	.byte	0x21
	.4byte	0x4d
	.4byte	0x355
	.uleb128 0x1b
	.4byte	0xa9
	.uleb128 0x1b
	.4byte	0x4d
	.byte	0
	.uleb128 0x1a
	.4byte	.LASF38
	.byte	0x3
	.byte	0x23
	.4byte	0xa9
	.4byte	0x37e
	.uleb128 0x1b
	.4byte	0xa9
	.uleb128 0x1b
	.4byte	0x4d
	.uleb128 0x1b
	.4byte	0x62
	.uleb128 0x1b
	.4byte	0xd7
	.uleb128 0x1b
	.4byte	0x62
	.byte	0
	.uleb128 0x1c
	.4byte	.LASF48
	.byte	0xb
	.byte	0x17
	.uleb128 0x1a
	.4byte	.LASF39
	.byte	0x5
	.byte	0x7e
	.4byte	0x54
	.4byte	0x39b
	.uleb128 0x1b
	.4byte	0x3b
	.uleb128 0x1d
	.byte	0
	.uleb128 0x1e
	.4byte	.LASF40
	.byte	0x6
	.byte	0xd0
	.4byte	0x3ad
	.uleb128 0x1b
	.4byte	0x3b
	.uleb128 0x1d
	.byte	0
	.uleb128 0x1f
	.4byte	.LASF41
	.byte	0x3
	.byte	0x25
	.4byte	0xa9
	.uleb128 0x1b
	.4byte	0xa9
	.uleb128 0x1b
	.4byte	0x4d
	.uleb128 0x1b
	.4byte	0x62
	.uleb128 0x1b
	.4byte	0xd7
	.uleb128 0x1b
	.4byte	0x62
	.byte	0
	.byte	0
	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x4
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0xa
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x11
	.uleb128 0x1
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x34
	.uleb128 0x19
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0x4109
	.byte	0
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1b
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1c
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x1d
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x1e
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1f
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL1-.Ltext0
	.8byte	.LVL11-.Ltext0
	.2byte	0x1
	.byte	0x65
	.8byte	.LVL12-.Ltext0
	.8byte	.LFE449-.Ltext0
	.2byte	0x1
	.byte	0x65
	.8byte	0
	.8byte	0
.LLST1:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL3-1-.Ltext0
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST2:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL10-.Ltext0
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL12-.Ltext0
	.8byte	.LFE449-.Ltext0
	.2byte	0x1
	.byte	0x63
	.8byte	0
	.8byte	0
.LLST3:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL5-.Ltext0
	.2byte	0x1
	.byte	0x53
	.8byte	0
	.8byte	0
.LLST4:
	.8byte	.LVL0-.Ltext0
	.8byte	.LVL10-.Ltext0
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL12-.Ltext0
	.8byte	.LFE449-.Ltext0
	.2byte	0x1
	.byte	0x64
	.8byte	0
	.8byte	0
.LLST5:
	.8byte	.LVL5-.Ltext0
	.8byte	.LVL11-.Ltext0
	.2byte	0x1
	.byte	0x66
	.8byte	.LVL12-.Ltext0
	.8byte	.LVL13-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL13-.Ltext0
	.8byte	.LVL17-.Ltext0
	.2byte	0x1
	.byte	0x66
	.8byte	.LVL17-.Ltext0
	.8byte	.LVL18-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL18-.Ltext0
	.8byte	.LFE449-.Ltext0
	.2byte	0x1
	.byte	0x66
	.8byte	0
	.8byte	0
.LLST6:
	.8byte	.LVL7-.Ltext0
	.8byte	.LVL8-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.8byte	0
	.8byte	0
.LLST7:
	.8byte	.LVL2-.Ltext0
	.8byte	.LVL9-.Ltext0
	.2byte	0x1
	.byte	0x67
	.8byte	.LVL12-.Ltext0
	.8byte	.LFE449-.Ltext0
	.2byte	0x1
	.byte	0x67
	.8byte	0
	.8byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.Ltext0
	.8byte	.Letext0-.Ltext0
	.8byte	0
	.8byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF19:
	.string	"__int128 unsigned"
.LASF32:
	.string	"console_printk"
.LASF34:
	.string	"memstart_addr"
.LASF44:
	.string	"dummy"
.LASF14:
	.string	"HACC_USER1"
.LASF7:
	.string	"short unsigned int"
.LASF36:
	.string	"zero_pfn"
.LASF35:
	.string	"cpu_bit_bitmap"
.LASF41:
	.string	"masp_hal_sp_hacc_dec"
.LASF31:
	.string	"__func__"
.LASF13:
	.string	"float"
.LASF39:
	.string	"printk"
.LASF5:
	.string	"unsigned char"
.LASF27:
	.string	"CustomSeed"
.LASF38:
	.string	"masp_hal_sp_hacc_enc"
.LASF0:
	.string	"long unsigned int"
.LASF45:
	.string	"masp_hal_secure_algo"
.LASF30:
	.string	"addr"
.LASF43:
	.string	"dummy"
.LASF17:
	.string	"HACC_USER4"
.LASF21:
	.string	"uchar"
.LASF28:
	.string	"ResText"
.LASF18:
	.string	"HACC_USER"
.LASF26:
	.string	"ContentLen"
.LASF20:
	.string	"uint32"
.LASF37:
	.string	"masp_hal_sp_hacc_init"
.LASF22:
	.string	"masp_hal_secure_algo_init"
.LASF3:
	.string	"unsigned int"
.LASF48:
	.string	"osal_hacc_unlock"
.LASF47:
	.string	"osal_hacc_lock"
.LASF2:
	.string	"char"
.LASF12:
	.string	"phys_addr_t"
.LASF1:
	.string	"sizetype"
.LASF40:
	.string	"panic"
.LASF8:
	.string	"long long int"
.LASF23:
	.string	"masp_hal_secure_algo_deinit"
.LASF6:
	.string	"short int"
.LASF33:
	.string	"hex_asc"
.LASF29:
	.string	"seed"
.LASF24:
	.string	"Direction"
.LASF10:
	.string	"long int"
.LASF4:
	.string	"signed char"
.LASF25:
	.string	"ContentAddr"
.LASF15:
	.string	"HACC_USER2"
.LASF16:
	.string	"HACC_USER3"
.LASF11:
	.string	"_Bool"
.LASF46:
	.string	"_error"
.LASF9:
	.string	"long long unsigned int"
.LASF42:
	.ascii	"GNU C 4.9 20140514 (prerelease) -fpreprocessed -mbionic -mli"
	.ascii	"ttle-end"
	.string	"ian -mgeneral-regs-only -mabi=lp64 -g -O2 -fno-strict-aliasing -fno-common -fno-delete-null-pointer-checks -fno-pic -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -fno-strict-overflow -fconserve-stack"
	.ident	"GCC: (GNU) 4.9 20140514 (prerelease)"
