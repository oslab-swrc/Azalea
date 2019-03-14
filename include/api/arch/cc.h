/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

/* Include some files for defining library routines */
#include <string.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

#define LWIP_NO_STDINT_H 1
#define LWIP_NO_INTTYPES_H 1

int lwip_rand(void);
#define LWIP_RAND lwip_rand

/* Define platform endianness */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif /* BYTE_ORDER */

/* Define generic types used in lwIP */
typedef uint8_t		u8_t;
typedef int8_t		s8_t;
typedef uint16_t	u16_t;
typedef int16_t		s16_t;
typedef uint32_t	u32_t;
typedef int32_t		s32_t;
typedef uint64_t	u64_t;
typedef int64_t		s64_t;

typedef size_t mem_ptr_t;

/* Define (sn)printf formatters for these lwIP types */
#define X8_F  "02x"
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "zu"

/* Compiler hints for packing structures */
//#define PACK_STRUCT_FIELD(x) x __attribute__((packed))
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

#define LWIP_PLATFORM_BYTESWAP 1
#define LWIP_PLATFORM_HTONS(x) ( (((u16_t)(x))>>8) | (((x)&0xFF)<<8) )
#define LWIP_PLATFORM_HTONL(x) ( (((u32_t)(x))>>24) | (((x)&0xFF0000)>>8) | (((x)&0xFF00)<<8) | (((x)&0xFF)<<24) )

#define MEM_ALIGNMENT 8
#define ETH_PAD_SIZE  2

#define LWIP_CHKSUM_ALGORITHM	3

/* define errno to determine error code */
#define ERRNO
#ifdef __KERNEL__
#define errno per_core(current_task)->lwip_err
#endif

/* prototypes for printf() and do_abort() */
#ifdef __KERNEL__
#include <hermit/stdio.h>
#include <hermit/stdlib.h>
#else
#ifndef NORETURN
#define NORETURN	__attribute__((noreturn))
#endif

int kprintf(const char*, ...);
void NORETURN do_abort(void);
#endif

/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)	do {kprintf x;} while(0)

#define LWIP_PLATFORM_ASSERT(x) do {kprintf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); do_abort();} while(0)

#if NO_SYS
typedef uint32_t sys_prot_t;

static inline sys_prot_t sys_arch_protect(void)
{
	return irq_nested_disable();
}

static inline void sys_arch_unprotect(sys_prot_t pval)
{
	irq_nested_enable(pval);
}
#endif

#define LWIP_FD_BIT	(1 << 30)

#ifndef __KERNEL__
struct sockaddr;
struct timeval;
struct hostent;
struct addrinfo;
typedef u32_t socklen_t;

/* FD_SET used for lwip_select */
#undef  FD_SETSIZE
/* Make FD_SETSIZE match NUM_SOCKETS in socket.c */
#define FD_SETSIZE    MEMP_NUM_NETCONN
#define FD_SET(n, p)  ((p)->fd_bits[((n) & ~LWIP_FD_BIT)/8] |=  (1 << (((n) & ~LWIP_FD_BIT) & 7)))
#define FD_CLR(n, p)  ((p)->fd_bits[((n) & ~LWIP_FD_BIT)/8] &= ~(1 << (((n) & ~LWIP_FD_BIT) & 7)))
#define FD_ISSET(n,p) ((p)->fd_bits[((n) & ~LWIP_FD_BIT)/8] &   (1 << (((n) & ~LWIP_FD_BIT) & 7)))
#define FD_ZERO(p)    memset((void*)(p),0,sizeof(*(p)))

/* Maximum queue length specifiable by listen.  */
#define SOMAXCONN	1

typedef struct fd_set {
	unsigned char fd_bits [(FD_SETSIZE+7)/8];
} fd_set;

int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int bind(int s, const struct sockaddr *name, socklen_t namelen);
int getpeername (int s, struct sockaddr *name, socklen_t *namelen);
int getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen);
int connect(int s, const struct sockaddr *name, socklen_t namelen);
int listen(int s, int backlog);
int recv(int s, void *mem, size_t len, int flags);
int recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int send(int s, const void *dataptr, size_t size, int flags);
int sendto(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int socket(int domain, int type, int protocol);
int select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
int shutdown(int socket, int how);
//int fcntl(int s, int cmd, int val);
struct hostent *gethostbyname(const char* name);
struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type);
int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
int gethostname(char *name, size_t len);
#else
#define O_NONBLOCK	00004000
#endif

#endif /* __ARCH_CC_H__ */
