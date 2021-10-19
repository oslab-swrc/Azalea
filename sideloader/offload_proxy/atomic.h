// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#define ATOMIC_INIT(i)  { (i) }

#define	QWORD	unsigned long

typedef struct {
  int c;
} atomic_t;

typedef struct {
  QWORD c;
} atomic64_t;

// Functions
static inline int atomic_get(atomic_t *a)
{
  return a->c;
}

static inline void atomic_set(atomic_t *a, int v)
{
  a->c = v;
}

static inline void atomic_inc(atomic_t *a)
{
  asm volatile ("lock; incl %0":"+m" (a->c));
}

static inline int atomic_inc_return(atomic_t *a)
{
  asm volatile ("lock; incl %0":"+m" (a->c));

  return a->c;
}

static inline void atomic_dec(atomic_t *a)
{
  asm volatile ("lock; decl %0":"+m" (a->c));
}

static inline int atomic_dec_and_test(atomic_t *a)
{
  char c;
  asm volatile ("lock; decl %0; sete %1":"+m" (a->c), "=qm"(c):: "memory");
  return c != 0;
}

static inline void atomic_and(int v, atomic_t *a)
{
  asm volatile ("lock; andl %1, %0":"+m" (a->c):"er"(v):"memory");
}

static inline void atomic_or(int v, atomic_t *a)
{
  asm volatile ("lock; orl %1, %0":"+m" (a->c):"er"(v):"memory");
}

static inline void atomic_xor(int v, atomic_t *a)
{
  asm volatile ("lock; xorl %1, %0":"+m" (a->c):"er"(v):"memory");
}

static inline int atomic_read(atomic_t *a)
{
  return a->c;
}

static inline QWORD atomic64_test_and_set(atomic64_t *a, QWORD ret)
{
  asm volatile ("xchgq %0, %1" : "=r"(ret) : "m"(a->c), "0"(ret) : "memory");

  return ret;
}

static inline QWORD atomic64_add(atomic64_t *a, QWORD i)
{
  QWORD ret = i;
  asm volatile("lock; xaddq %0, %1" : "+r"(i), "+m"(a->c) : : "memory", "cc");

  return ret+i;
}

static inline QWORD atomic64_sub(atomic64_t *a, QWORD i)
{
  return atomic64_add(a, -i);
}

static inline QWORD atomic64_inc(atomic64_t *a)
{
  QWORD ret = 1;
  asm volatile("lock; xaddq %0, %1" : "+r"(ret), "+m"(a->c) : : "memory", "cc");

  return ++ret;
}

static inline QWORD atomic64_dec(atomic64_t *a)
{
  QWORD ret = -1;
  asm volatile("lock; xaddq %0, %1" : "+r"(ret), "+m"(a->c) : : "memory", "cc");

  return --ret;
}

static inline QWORD atomic64_read(atomic64_t *a)
{
  return a->c;
}

static inline void atomic64_set(atomic64_t *a, QWORD v) 
{
  atomic64_test_and_set(a, v);
}

#endif  /* __ATOMIC_H__ */
