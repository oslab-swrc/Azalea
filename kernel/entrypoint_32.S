// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "arch.h"

.text
.code16

.global _start

_start:
  cli
  nop
  nop
  ljmp $(BOOT_ADDR>>4), $begin 

begin:  
  movw $(BOOT_ADDR>>4), %ax
  movw %ax,%ds
  movw %ax,%ss

  lgdtl GDTR

  // disable paging, disable cache, internal FPU, disable align check, enable protected mode
  //mov $1, %ax
  //lmsw %eax
  movl $0x4000003B,%eax // PG=0, CD=1, NW=0, AM=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
  movl %eax, %cr0

  ljmpl $0x18, $(PROTECTEDMODE - _start + BOOT_ADDR)

.code32
PROTECTEDMODE: 
  movw $0x20,%ax
  movw %ax,%ds
  movw %ax,%es
  movw %ax,%fs
  movw %ax,%gs
  movw %ax,%ss

  movl $(BOOT_ADDR+0x0FFE), %esp
  movl $(BOOT_ADDR+0x0FFE), %ebp       

#ifdef _qemu_
COPYPAGETABLE:
        mov $0x10400, %esi
        mov $(PML4_ADDR), %ebx
	mov $0x0, %ecx

LOOP1:
        cmp $0x4000, %ecx
        je LOOP1END
        movb (%esi), %al
        mov %al, (%ebx)
        add $0x1, %ecx
	add $0x1, %esi
	add $0x1, %ebx 
        jmp LOOP1
LOOP1END:

COPYKERNEL64:

  movl $(CONFIG_PML4_ADDR), %ebx
  movl $(PML4_ADDR), (%ebx)

  movl $(MEMORY_START+KERNEL_ADDR), %eax

  movl $(CONFIG_KERNEL64), %ebx
  movw (%ebx), %cx
  shll $9, %ecx

//kernel 64 start addr : skip 0x10000+512*kernel32+pagetable*512
  movl $(0x10000+0x400+0x4000), %esi
  movl %eax, %edi

LOOP:
  cmpl $0, %ecx
  je COPYKERNEL64END

  movb (%esi), %al
  movb %al, (%edi)

  addl $1, %esi
  addl $1, %edi
  subl $1, %ecx

  jmp LOOP
COPYKERNEL64END:

COPYAPP:
  movl $0x0, %ecx
  movw CONFIG_KERNEL32, %cx
  addw $32, %cx      // 4K * 4
  addw CONFIG_KERNEL64, %cx
  movl %ecx, %esi
  movw CONFIG_TOTAL_COUNT, %cx
  subl %esi, %ecx

  shll $0x9, %ecx
  shll $0x9, %esi
  addl $0x10000, %esi
  movl $(MEMORY_START+APP_ADDR), %edi

LOOP2:
  cmpl $0x0, %ecx
  je COPYAPPEND
  movb (%esi), %al
  movb %al, (%edi)

  addl $0x1, %esi
  addl $0x1, %edi
  subl $0x1, %ecx

  jmp LOOP2

COPYAPPEND:
COPYEND:
#endif

  ljmpl $0x18, $(BOOT_ADDR+0x200) 

.align 8
GDTR: 
  .word  GDTEND- GDT - 1
  .long  (GDT - _start + BOOT_ADDR)
GDT: 
nulldesc: 
  .word 0x0000
  .word 0x0000
  .byte 0x00
  .byte 0x00
  .byte 0x00
  .byte 0x00

IA_32ecodedesc: 
  .word 0xFFFF
  .word 0x0000
  .byte 0x00
  .byte 0x9A
  .byte 0xAF
  .byte 0x00

IA_32edatadesc: 
  .word 0xFFFF
  .word 0x0000
  .byte 0x00
  .byte 0x92
  .byte 0xAF
  .byte 0x00

codedesc: 
  .word 0xFFFF
  .word 0x0000
  .byte 0x00
  .byte 0x9B
  .byte 0xCF
  .byte 0x00

datadesc: 
  .word 0xFFFF
  .word 0x0000
  .byte 0x00
  .byte 0x93
  .byte 0xCF
  .byte 0x00

GDTEND:
  . = 512 
