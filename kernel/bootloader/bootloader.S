// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "arch.h"

.text
.code16

.global _start

_start:
	jmp START
	nop

.rep 0x57
.byte 0
.endr

TOTALSECTORCOUNT:   	.word	0x01
KERNEL32SECTORCOUNT:	.word	0x00
PAGETABELCOUNT:		.word	0x00
KERNEL64SECTORCOUNT:	.word	0x00
BOOTSTRAPPROCESSOR:	.byte	0x01
BOOTDRIVE:		.byte	0x00

START:
	cli
	nop
	nop

	ljmp $0, $begin
begin:
	xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %ss

//	movw $0x2000, %sp
//	movw $0x2000, %bp
/*
	movb 	(BOOTDRIVE), %al
	cmpb 	$0xff, %al
	je 	RESETDISK
	mov	%al, %dl
	
.forced_disk_reference:
	movb	%dl, (BOOTDRIVE)
*/
	
RESETDISK:
	movw $0, %ax
	movb (BOOTDRIVE), %dl
	int $0x13
	jc HANDLEDISKERROR

	movb $0x8, %ah
	movb BOOTDRIVE, %dl
	int $0x13
	jc HANDLEDISKERROR

	movb %dh, (LASTHEAD)
	movb %cl, %al
	and $0x3f, %al
	
	movb %al, (LASTSECTOR)
	movb %ch, (LASTTRACK)

	mov $0x1000, %si
	mov %si, %es
	mov $0x0, %bx

	mov (TOTALSECTORCOUNT), %di

READDATA:
	cmp $0x0,%di
	je READEND
	sub $0x1, %di

	mov $0x2, %ah
	mov $0x1, %al
	movb TRACKNUMBER, %ch
	movb SECTORNUMBER, %cl
	movb HEADNUMBER, %dh
	movb BOOTDRIVE, %dl
	int $0x13
	jc HANDLEDISKERROR


	addw $0x20, %si
	movw %si, %es
	
	movb SECTORNUMBER, %al
	addb $0x01, %al
	movb %al, SECTORNUMBER

	cmpb LASTSECTOR, %al
	jle READDATA

	addb $0x01, HEADNUMBER
	movb $0x01, SECTORNUMBER
	
	movb LASTHEAD, %al
	cmpb %al, HEADNUMBER
	jg .ADDTRACK
	
	jmp READDATA

.ADDTRACK:
	movb $0x00, HEADNUMBER
	addb $0x01, TRACKNUMBER
	jmp READDATA

READEND:


/* 0x10000 ==> BOOT_ADDR, size=0x400 */
COPYKERNEL32:
	mov $0x1000, %si
	mov %si, %es
	mov $(BOOT_ADDR>>4), %si
	mov %si, %fs
	mov $0, %bx

LOOP:
	cmp $0x400, %bx
	je LOOPEND
	mov %es:(%bx), %al
	mov %al, %fs:(%bx)
	add $1, %bx
	jmp LOOP

LOOPEND:

/* 0x10400 ==> BOOT_ADDR+0x1000, size=0x4000 */
	mov $(BOOT_ADDR>>4), %si
	mov %si, %es
	movw $(META_OFFSET+KERNEL64_COUNT), %bx
	movw KERNEL64SECTORCOUNT, %ax
	movw %ax, %es:(%bx)
	movw $(META_OFFSET+KERNEL32_COUNT), %bx
	movw KERNEL32SECTORCOUNT, %ax
	movw %ax, %es:(%bx)
	movw $(META_OFFSET+TOTAL_COUNT), %bx
	movw TOTALSECTORCOUNT, %ax
	movw %ax, %es:(%bx)
	movw $(META_OFFSET+QEMU_OFFSET), %bx
        movw $0xFFEE, %es:(%bx)	

	ljmp $(BOOT_ADDR>>4), $0x0

HANDLEDISKERROR:
        jmp .

SECTORNUMBER:   .byte 0x02
HEADNUMBER: 	.byte 0x00
TRACKNUMBER:	.byte 0x00

LASTSECTOR:     .byte 0x00
LASTHEAD:       .byte 0x00
LASTTRACK:      .byte 0x00

. = 510

.byte 0x55
.byte 0xAA








	
	






