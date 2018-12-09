#include "memory_config.h"

.code32
.global switch_and_execute_64bit_kernel

.text
switch_and_execute_64bit_kernel: 
// Set PAE bit to 1 in CR4
// Enable double
  movl $0x40620, %eax
  movl %eax, %cr4

  movl (CONFIG_PML4_ADDR), %eax
  mov %eax, %cr3
  jmp switch_and_execute_64bit_kernel.MSR

switch_and_execute_64bit_kernel.MSR:

// IA32_EFER MS register address
  movl $0xC0000080, %ecx
// set LME(bit 8)
  movl $(1 << 8), %eax
  xorl %edx, %edx
  wrmsr

  movl $0x80000000, %eax  # set PG
  xorl $0x00000001, %eax  # set PE
  movl %eax, %cr0         # unset NW and CD bit. Set PG bit

  ljmpl $0x08, $(CONFIG_KERNEL64_START_ADDR)
  jmp . 
