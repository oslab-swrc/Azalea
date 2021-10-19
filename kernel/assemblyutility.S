// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assemblyutility.h"

.text
.code64

ENTRY(read_msr)
  pushq %rdx
  pushq %rax
  pushq %rcx
  pushq %rbx

  movq %rdx,%rbx
  movq %rdi,%rcx

  rdmsr

  movq %rdx, (%rsi)
  movq %rax, (%rbx)

  popq %rbx
  popq %rcx
  popq %rax
  popq %rdx

  ret

ENTRY(write_msr) 
  pushq %rdx
  pushq %rax
  pushq %rcx

  movq %rdi,%rcx
  movq %rdx,%rax
  movq %rsi,%rdx

  wrmsr

  popq %rcx
  popq %rax
  popq %rdx

  ret

ENTRY(read_tsc) 
  pushq %rdx

  rdtsc

  shlq $32,%rdx
  orq %rdx,%rax

  popq %rdx

  ret

ENTRY(load_gdtr) 
  lgdt (%rdi)
  ret

ENTRY(load_tr) 
  ltrw %di
  ret

ENTRY(load_idtr) 
  lidt (%rdi)
  ret

ENTRY(enable_interrupt) 
  sti
  ret

ENTRY(disable_interrupt) 
  cli
  ret

ENTRY(read_flags) 
  pushfq
  popq %rax
  ret

ENTRY(enable_globallocalapic) 
  pushq %rax
  pushq %rcx
  pushq %rdx

  movq $27,%rcx
  rdmsr

  orl $0x0800,%eax
  wrmsr

  popq %rdx
  popq %rcx
  popq %rax
  ret

ENTRY(set_bit) 
  movq $0x01,%rax
  lock btsq %rsi,(%rdi)
  jc set_bit.BITSET1
  movq $0x00,%rax
set_bit.BITSET1: 
  ret

ENTRY(clear_bit) 
  movq $0x01,%rax
  lock btrq %rsi,(%rdi)
  jc clear_bit.BITSET2
  movq $0x00,%rax
clear_bit.BITSET2: 
  ret

ENTRY(toggle_bit) 
  movq $0x01,%rax
  lock btcq %rsi,(%rdi)
  jc toggle_bit.BITSET3
  movq $0x00,%rax
toggle_bit.BITSET3: 
  ret

ENTRY(set_cr3) 
  pushq %rax
  movq %rdi,%rax
  movq %rax, %cr3
  popq %rax
  ret

ENTRY(get_cr3) 
  movq %cr3, %rax
  ret

ENTRY(read_cr2) 
  movq %cr2, %rax
  ret

ENTRY(in_port_byte)
  pushq %rdx

  movq %rdi, %rdx
  movq $0x0, %rax
  in (%dx), %al

  popq %rdx
  ret

ENTRY(out_port_byte)
  pushq %rdx
  pushq %rax

  movq %rdi, %rdx
  movq %rsi, %rax
  out %al, (%dx)

  popq %rax
  popq %rdx
  ret

ENTRY(pause)
  pause
  ret

ENTRY(hlt)
  hlt
  ret

ENTRY(nop)
  nop
  ret
