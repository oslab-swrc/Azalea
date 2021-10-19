// SPDX-FileCopyrightText: Copyright (c) 2021 Electronics and Telecommunications Research Institute
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __IRQ_H__
#define __IRQ_H__

#include "az_types.h"

/** 
 * Determines, if the interrupt flags (IF) is set
 * return 1 if interrupt flag is set
 *        0 if interrupt flag is cleared
 */
static inline BOOL is_irq_enabled(void)
{
  QWORD flags;

  asm volatile("pushf; pop %0": "=r"(flags) : : "memory");
  if (flags & (1 << 9))
    return 1;

  return 0;
}

/** 
 * Disable IRQs (nested)
 *
 * return Whether IRQs had been enabled or not before disabling
 */
static inline BOOL irq_nested_disable(void)
{
  BOOL was_enabled = is_irq_enabled();
  disable_interrupt();

  return was_enabled;
}

/**
 * Enable IRQs (nested)
 */
static inline void irq_nested_enable(BOOL was_enabled)
{
  if (was_enabled)
    enable_interrupt();
}

#endif  /* __IRQ_H__ */
