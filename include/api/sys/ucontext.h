/* libc/sys/hermit/sys/ucontext.h - get and set user thread context */

/*
 * Written 2016 by Stefan Lankes
 *
 * derived from libc/sys/linux/sys/ucontext.h
 */


#ifndef _SYS_UCONTEXT_H
#define _SYS_UCONTEXT_H

#include <stdint.h>
#include <signal.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mregs {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rbp;
	uint64_t rbx;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rsp;
	uint64_t rip;
	uint32_t mxcsr;
} mregs_t;

typedef struct {
        uint16_t control_word;
        uint16_t unused1;
        uint16_t status_word;
        uint16_t unused2;
        uint16_t tags;
        uint16_t unused3;
        uint32_t eip;
        uint16_t cs_selector;
        uint32_t opcode:11;
        uint32_t unused4:5;
        uint32_t data_offset;
        uint16_t data_selector;
        uint16_t unused5;
} fenv_t;

typedef struct ucontext {
	mregs_t		uc_mregs;
	fenv_t		uc_fenv;
	struct ucontext	*uc_link;
	stack_t		uc_stack;
} ucontext_t;

int getcontext(ucontext_t *ucp);
int setcontext(const ucontext_t *ucp);
void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
int swapcontext(ucontext_t *oucp, const ucontext_t *ucp);

#ifdef __cplusplus
}
#endif

#endif
