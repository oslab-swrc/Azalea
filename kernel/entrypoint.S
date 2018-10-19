#include "arch.h"
#include "entry.h"
#include "assemblyutility.h"
#include "localapic.h" 
#include "memory_config.h"

.text
.code64
.extern Main
.extern apic_id_address, g_ap_count
.extern process_systemcall
.extern release_thread_spl 
.extern disable_x2apic
.global systemcall_entrypoint, context_switch, return_from_setup_thread

start:
  movw $KERNEL_DATA_SEGMENT, %ax
  movw %ax,%ds
  movw %ax,%es
  movw %ax,%fs
  movw %ax,%gs
  movw %ax,%ss

#ifdef _qemu_
COPYPARAMETER:
  
  movl $(CONFIG_QEMU), %eax
  cmpw $0xFFEE, (%eax)
  jne  COPYEND

  movq $(CONFIG_VCON_ADDR), %rbx
  movq $(VCON_ADDR), (%rbx)

  movq $(CONFIG_APIC_ADDR), %rbx
  movl $(APIC_ADDR), (%rbx)

  movq $(CONFIG_CPU_START), %rbx
  movq $(CPU_START), (%rbx)

  movq $(CONFIG_CPU_END), %rbx
  movq $(CPU_END), (%rbx)

  movq $(MEMORY_START), %rbx
  shr  $30, %rbx          //30bit shift right : Unit : Gbyte
  movq %rbx, (CONFIG_MEM_START)

  movq $(MEMORY_END), %rbx
  shr  $30, %rbx
  movq %rbx, (CONFIG_MEM_END)

COPYEND:
#endif

  call disable_x2apic

  // Get APIC ID from register. ID is in bit 24-31
  mov (CONFIG_APIC_ADDR), %eax
  mov APIC_REGISTER_APIC_ID(%eax), %eax
  shr $24, %rax

  // Substract the number of start core for to make the start core '0'
  sub (CONFIG_CPU_START), %eax

  // Set the size of stack to 0x1000 (4KB)
  add $1, %rax
  shl $12, %rax

  /* MSR[ECX] --> EDX:EAX */
  //mov $0x802, %ecx
  //rdmsr

  // Calculate and set the stack address of the core
  movabs $IDLE_THREAD_ADDRESS_VA, %rbx
  addq %rbx, %rax
	
  // Set the RSP and RBP register
  movq %rax, %rsp
  movq %rax, %rbp
 
  // Increase the number of waken core
  xorl %eax, %eax
  xorl %edx, %edx
  incl %edx
  lock cmpxchg %edx, g_ap_count(%rip)
  jne start.AP

  movq $CONFIG_KERNEL64_START_VA, %rax
  add $(start.Haddr - start), %rax
  movq $1, %rdi
  jmp *%rax

start.AP: 
  movq $CONFIG_KERNEL64_START_VA, %rax
  add $(start.HaddrAP - start), %rax
  movq $0, %rdi
  jmp *%rax

start.HaddrAP: 
  lock incq g_ap_count(%rip)

start.BOOTSTRAPPROCESSORSTARTPOINT: 
start.Haddr: 
  call Main
  jmp .

systemcall_entrypoint: 
// fetch kernel stack pointer
//  swapgs
//  movq	%rsp,%gs:USER_STACK_OFFSET
//  movq	%gs:STACK_BASE_OFFSET,%rsp  // current_tcb->stack_base

// begin of building interrupt stack
//  subq	$40,%rsp
//  pushq	%rax
//  movq	%gs:USER_STACK_OFFSET,%rax
//  swapgs
//  mov 	%rcx, 0x8(%rsp)
//  movq 	$USER_CODE_SEGMENT, 0x10(%rsp)
//  mov 	%r11, 0x18(%rsp)
//  mov	%rax, 0x20(%rsp)
//  movq	$USER_DATA_SEGMENT,0x28(%rsp)
    	
//  popq %rax
// end of building the interrupt stack

// Saving user context and setting kernel segment registers
  SAVECONTEXT
  movq -8*5(%rbp),%rdi 	//rdi, 1st param
  movq -8*6(%rbp),%rsi 	//rsi, 2nd param
  movq -8*4(%rbp),%rdx 	//rdx, 3rd param
  movq -8*9(%rbp),%rcx 	//r10, 4th param
  
  movq -8*7(%rbp),%r8
  movq -8*8(%rbp),%r9
  movq -8*1(%rbp),%rax	//rax: system call number, 7th param
  pushq %rax

  call process_systemcall

  movq %rax,-8(%rbp)
  popq %rax

  LOADCONTEXT

  push %r11
  popfq
  jmp *%rcx
//  iretq

return_from_setup_thread: 
  movq %rax,%rdi
  call release_thread_spl

  LOADCONTEXT
  iretq

context_switch: 
  pushq %rbp
  movq %rsp,%rbp

  pushq %rbx
  push %r12
  push %r13
  push %r14
  push %r15
  movq %rdi,%rax
  mov  %rsp, (%rdi) 
  movq (%rsi),%rsp

  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  popq %rbp

  ret
