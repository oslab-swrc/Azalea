#ifndef __THREAD_H__
#define __THREAD_H__

#include "atomic.h"
#include "list.h"
#include "localapic.h"
#include "multiprocessor.h"
#include "sync.h"
#include "types.h"
#include "utility.h"
#include "signal.h"

#define NO_TCB_LOCK

#define THREAD_INT_STACK_REG_COUNT  (5)
#define THREAD_USER_REG_COUNT	    (19)

#define THREAD_REGISTER_COUNT	    (THREAD_INT_STACK_REG_COUNT + THREAD_USER_REG_COUNT)
#define THREAD_REGISTER_SIZE	    (8)

#define THREAD_GS_OFFSET	    0
#define THREAD_FS_OFFSET	    1
#define THREAD_ES_OFFSET	    2
#define THREAD_DS_OFFSET	    3
#define THREAD_R15_OFFSET	    4
#define THREAD_R14_OFFSET	    5
#define THREAD_R13_OFFSET	    6
#define THREAD_R12_OFFSET	    7
#define THREAD_R11_OFFSET	    8
#define THREAD_R10_OFFSET	    9
#define THREAD_R9_OFFSET	    10
#define THREAD_R8_OFFSET	    11
#define THREAD_RSI_OFFSET	    12
#define THREAD_RDI_OFFSET	    13
#define THREAD_RDX_OFFSET	    14
#define THREAD_RCX_OFFSET	    15
#define THREAD_RBX_OFFSET	    16
#define THREAD_RAX_OFFSET	    17
#define THREAD_RBP_OFFSET	    18
#define THREAD_RIP_OFFSET	    19
#define THREAD_CS_OFFSET	    20
#define THREAD_RFLAGS_OFFSET	    21
#define THREAD_RSP_OFFSET	    22
#define THREAD_SS_OFFSET	    23

#define THREAD_TYPE_BSP		0
#define THREAD_TYPE_AP		1

#define THREAD_USER		    0x1
#define THREAD_KERNEL		    0x0
#if 0
#define THREAD_INFINITE_TIME_QUANTUM  (0)
#define THREAD_INFINITE_TIME_SLICE    (0)
#define THREAD_DEFAULT_TIME_QUANTUM   THREAD_INFINITE_TIME_QUANTUM
#define THREAD_DEFAULT_TIME_SLICE     (1* 60 * 60 * 1000)		// 1 hour
#define THREAD_IDLE_THREAD_TIME_SLICE (10)
#define RUNNABLE_MAX			1
#else
#define THREAD_INFINITE_TIME_QUANTUM  (0)
#define THREAD_INFINITE_TIME_SLICE    (0)
#define THREAD_DEFAULT_TIME_QUANTUM   THREAD_INFINITE_TIME_QUANTUM
#define THREAD_DEFAULT_TIME_SLICE     (10)
#define THREAD_IDLE_THREAD_TIME_SLICE (10)
#define RUNNABLE_MAX			20
#endif

#define THREAD_INIT_NAME	      (~0x0)

// Thread state definitions
enum thread_state_enum {
    THREAD_STATE_NOTALLOC=0,
    THREAD_STATE_CREATED=1,
    THREAD_STATE_RUNNING,
    THREAD_STATE_READY,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_EXITED,
};

#define THREAD_INTENTION_READY		(0x1 << THREAD_STATE_READY)
#define THREAD_INTENTION_BLOCKED	(0x1 << THREAD_STATE_BLOCKED)
#define THREAD_INTENTION_EXITED		(0x1 << THREAD_STATE_EXITED)

#define THREAD_COMMAND_NONE		(0)
#define THREAD_COMMAND_ALLOC_TID	(1)
#define THREAD_COMMAND_CREATE		(2)
#define THREAD_COMMAND_EXIT		(3)
#define THREAD_COMMAND_JOIN		(4)
#define THREAD_COMMAND_YIELD		(5)

#define THREAD_CONTROL_H		(1 << 0)    // specifying suspending
#define THREAD_CONTROL_HE		(1 << 1)    // enabling the suspending

#define THREAD_ID_IDLE			(0)

#define THREAD_FLAGS_IF			(0x200)
#define THREAD_FLAGS_IOPL		(0x3000)    // always 1 on 8086 and 186

#define THREAD_NUM_GP_REGISTER		(14)

#define THREAD_CREATE			0
#define THREAD_DELETE			1
#define THREAD_EXIT_INTENTION		0
#define THREAD_EXIT			1

#define THREAD_IPI_DEST		(APIC_DESTINATION_SHORT_HAND_NO_SHORT_HAND >> 18)
#define THREAD_IPI_SELF		(APIC_DESTINATION_SHORT_HAND_SELF >> 18)
#define THREAD_IPI_ALL		(APIC_DESTINATION_SHORT_HAND_ALL_INCLUDING_SELF >> 18)
#define THREAD_IPI_ALL_BUT_SELF	(APIC_DESTINATION_SHORT_HAND_ALL_EXCLUDING_SELF >> 18)


#define SET_CORE_MASK(core_mask, i)	((core_mask)->mask[i / (sizeof(DWORD)*8)]) |= (0x1 << (i % (sizeof(DWORD)*8)))
#define CLEAR_CORE_MASK(core_mask, i)	(core_mask)->mask[i / (sizeof(DWORD)*8)] &= ~(0x1 << (i % (sizeof(DWORD)*8)))
#define FILL_CORE_MASK(core_mask)	lk_memset((void *) (core_mask)->mask, ~0x0, sizeof(core_set_t))
#define INIT_CORE_MASK(core_mask)	lk_memset((void *) (core_mask)->mask, 0x0, sizeof(core_set_t))
#define COPY_CORE_MASK(core_mask_dest, core_mask_src) lk_memcpy((void *)((core_mask_dest)->mask), ((void*)(core_mask_src)->mask), sizeof(core_set_t))
#define ISSET_CORE_MASK(core_mask, i)	(core_mask)->mask[i / (sizeof(DWORD)*8)] & (0x1 << (i % (sizeof(DWORD)*8)))
#if 0
#define ISONLY_CORE_MASK(core_mask, cid) ({ \
  int i; DWORD p=1, q=0; \
  if(ISSET_CORE_MASK(core_mask, cid)) { \
    p = ~((core_mask)->mask[cid / (sizeof(DWORD)*8)]); \
    for(i=0; i<sizeof(core_set_t); i++) \
      q += ~((core_mask)->mask[i]); \
  }\
  p==q;\
})
#else
#define ISONLY_CORE_MASK(core_mask, cid) ({ \
0; })
#endif

struct thread_list {
  spinlock_t lock;
  QWORD count;
  struct dl_list tcb_list;
};

typedef struct core_set_struct {
  DWORD mask[8];
} core_set_t;

typedef struct thread_control_block_struct {
  QWORD stack;     // XXX: don't move this field. This must be the frist field.
  QWORD user_stack;
  QWORD stack_base;
  QWORD id;
  DWORD gen;
  QWORD flags;
  WORD running_core;
  enum thread_state_enum state;
  atomic_t intention;
  QWORD name;
  int signal_flag;
  signal_handler_t signal_handler;

  long time_slice;
  long remaining_time_slice;
  long time_quantum;
  long remaining_time_quantum;
  QWORD acc_ltc;
  core_set_t core_mask;

  // Sched list
  struct thread_list *sched_list;
  struct dl_list tcb_link;

#ifndef NO_TCB_LOCK
  spinlock_t tcb_lock;
#endif
  atomic_t refc;
} TCB;

#define MAX_NUM_THREAD		1024

// Functions
int create_thread(QWORD ip, QWORD argv, int core_mask);

void sched_init(void);
void thread_init(void);
void init_tcb(TCB *tcb, QWORD stack);
void lk_thread_set_name(QWORD tid, QWORD name);
QWORD lk_thread_lookup_tid(QWORD name);
QWORD lk_thread_lookup_state(QWORD tid);

void start_idle_thread(int thread_type);
TCB *context_switch(TCB *prev, TCB *next);
TCB* alloc_tcb(void);
TCB* alloc_tcb_idle(int cid);
TCB* get_tcb(QWORD tid);
void put_tcb(TCB *tcb);

int __thread_exit(int tid);
int thread_exit(TCB *tcb);
int lk_app_exec(void *app_ptr);
BOOL schedule(QWORD intention);
int thread_suspend(int tid);
int thread_wake_up(int tid);

long adjust_time_slice(void);
void decrease_remaining_time_slice(long consumed_time_slice);

void set_core(TCB *tcb, core_set_t *cst, int core);

DECLARE_PERCPU(TCB *, running_thread);

static inline TCB *get_current(void)
{
  return (TCB *)running_thread[get_apic_id()];
}

#ifndef NO_TCB_LOCK
static inline void tcb_lock_init(TCB *t)
{
  spinlock_init(&t->tcb_lock);
}

static inline void tcb_lock(TCB *t)
{
  spinlock_lock(&t->tcb_lock);
}

static inline void tcb_unlock(TCB *t)
{
  spinlock_unlock(&t->tcb_lock);
}

#else
static inline void tcb_lock_init(TCB *t) {}
static inline void tcb_lock(TCB *t) {}
static inline void tcb_unlock(TCB *t) {}
#endif

void do_exit(int arg);
void block_current_task(void);
void reschedule(void);
int wakeup_task(tid_t id);

tid_t sys_getpid(void);
int sys_getprio(tid_t *id);
int sys_setprio(tid_t *id, int prio);
void sys_exit(int arg);
int sys_clone(tid_t *id, void *ep, void *argv);
void sys_yield(void);

#endif  /* __THREAD_H__ */
