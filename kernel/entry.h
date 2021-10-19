// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __ENTRY_H__
#define __ENTRY_H__

#define USER_STACK_OFFSET   8
#define STACK_BASE_OFFSET   16

#define USER_CODE_SEGMENT  0x23
#define USER_DATA_SEGMENT  0x1B

#define KERNEL_CODE_SEGMENT 0x8
#define KERNEL_DATA_SEGMENT 0x10

// Save Context
.macro SAVECONTEXT
    push %rbp
    mov %rsp,%rbp
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rdi
    push %rsi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %ds, %ax
    push %rax
    mov %es, %ax
    push %rax
    push %fs
    push %gs

    mov $KERNEL_DATA_SEGMENT, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %gs
    mov %ax, %fs
.endm

// Load Context
.macro LOADCONTEXT
    popq   %gs
    popq   %fs
    pop    %rax
    mov    %eax,%es
    pop    %rax
    mov    %eax,%ds
    pop    %r15
    pop    %r14
    pop    %r13
    pop    %r12
    pop    %r11
    pop    %r10
    pop    %r9
    pop    %r8
    pop    %rsi
    pop    %rdi
    pop    %rdx
    pop    %rcx
    pop    %rbx
    pop    %rax
    pop    %rbp
.endm

#endif  /*__ENTRY_H__ */
