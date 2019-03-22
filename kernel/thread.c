#include <sys/lock.h>
#include "thread.h"
#include "assemblyutility.h"
#include "atomic.h"
#include "console.h"
#include "debug.h"
#include "descriptor.h"
#include "map.h"
#include "memory_config.h"
#include "thread.h"
#include "multiprocessor.h"
#include "localapic.h"
#include "page.h"
#include "shellstorage.h"
#include "sync.h"
#include "systemcall.h"
#include "utility.h"
#include "elf_load.h"
#include "memory.h"
#include "signal.h"

extern void HALT() ;
unsigned int shutdown_kernel = 0 ;

extern QWORD elf_load_entry;
extern QWORD g_memory_start;
extern int g_cpu_size;
extern int g_ap_count;

static TCB *g_idle_thread_list[MAX_PROCESSOR_COUNT];
static TCB *g_global_thread_list[CONFIG_NUM_THREAD + MAX_PROCESSOR_COUNT];
static TCB *g_running_thread_list[MAX_PROCESSOR_COUNT];

DECLARE_PERCPU(struct thread_list, runnable_list);
DECLARE_PERCPU(struct thread_list, blocked_list);
DECLARE_PERCPU(struct thread_list, local_tcb_list);
//struct thread_list local_tcb_list[MAX_PROCESSOR_COUNT] __attribute__ ((aligned (64)));

struct thread_list g_global_tcb_list;
struct thread_list g_migrating_list;

static inline BOOL core_is_allowed(TCB * t, int cid)
{
  return ISSET_CORE_MASK(&t->core_mask, cid) ? FALSE : TRUE;
}

static int refill_time_slice(TCB * tcb);

/*
 * initialize thread list
 */
static void thread_list_init(struct thread_list *l)
{
  spinlock_init(&l->lock);

  l->count = 0;
  dl_list_init(&l->tcb_list);
}

/*
 * initialize schedule
 */
void sched_init(void)
{
  int i = 0;

  for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
    thread_list_init(&per_cpu_byid(runnable_list, i));
    thread_list_init(&per_cpu_byid(blocked_list, i));

    running_thread[i] = NULL;
  }

  thread_list_init(&g_migrating_list);
}

/*
 * initlaize thread
 */
void thread_init(void)
{
  int i = 0;
  TCB *tcb = NULL;

  thread_list_init(&g_global_tcb_list);

  for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
    thread_list_init(&per_cpu_byid(local_tcb_list, i));

    g_running_thread_list[i] = NULL;
  }

  // Initialize TCBs
  for (i = 0; i < CONFIG_NUM_THREAD; i++) {
    tcb = (TCB *) alloc(PAGE_SIZE_4K);
    if(tcb == NULL)
      debug_halt((char *) __func__, __LINE__);

    tcb->state = THREAD_STATE_NOTALLOC;
    tcb->id = MAX_PROCESSOR_COUNT + i;
    tcb->gen = 0;
    tcb->stack = (QWORD) tcb + CONFIG_TCB_SIZE;
    tcb->stack_base = tcb->stack;
    tcb->user_stack = 0x0;
    tcb->time_slice = THREAD_DEFAULT_TIME_SLICE;
    tcb->remaining_time_slice = THREAD_DEFAULT_TIME_SLICE;
    tcb->time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
    tcb->remaining_time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
    atomic_set(&tcb->intention, 0);
    tcb->running_core = -1;
    INIT_CORE_MASK(&tcb->core_mask);
    tcb->name = THREAD_INIT_NAME;
    tcb->signal_flag = 0;
    tcb->signal_handler = 0;

    tcb->acc_ltc = 0;

    tcb->sched_list = &g_global_tcb_list;
    dl_list_init(&tcb->tcb_link);
    tcb_lock_init(tcb);
    atomic_set(&tcb->refc, 0);

    dl_list_add_tail(&tcb->tcb_link, &g_global_tcb_list.tcb_list);
    g_global_tcb_list.count++;

    g_global_thread_list[i + MAX_PROCESSOR_COUNT] = tcb;

    store_tcb_info(i + MAX_PROCESSOR_COUNT, tcb);
  }

  // Initialize idle threads
  for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
    g_idle_thread_list[i] = (TCB *) va(IDLE_THREAD_ADDRESS + CONFIG_TCB_SIZE * i);
    g_idle_thread_list[i]->state = THREAD_STATE_CREATED;
    g_idle_thread_list[i]->id = i;
    g_idle_thread_list[i]->gen = 0;
    g_idle_thread_list[i]->stack = (QWORD) g_idle_thread_list[i] + CONFIG_TCB_SIZE;
    g_idle_thread_list[i]->stack_base = g_idle_thread_list[i]->stack;
    g_idle_thread_list[i]->user_stack = 0x0;
    g_idle_thread_list[i]->time_slice = THREAD_IDLE_THREAD_TIME_SLICE;
    g_idle_thread_list[i]->remaining_time_slice = THREAD_IDLE_THREAD_TIME_SLICE;
    g_idle_thread_list[i]->time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
    g_idle_thread_list[i]->remaining_time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
    atomic_set(&g_idle_thread_list[i]->intention, 0);
    g_idle_thread_list[i]->running_core = i;
    INIT_CORE_MASK(&g_idle_thread_list[i]->core_mask);
    g_idle_thread_list[i]->name = 0;

    g_idle_thread_list[i]->signal_flag = 0;
    g_idle_thread_list[i]->signal_handler = 0;

    g_idle_thread_list[i]->acc_ltc = 0;

    g_idle_thread_list[i]->sched_list = 0;
    dl_list_init(&g_idle_thread_list[i]->tcb_link);
    tcb_lock_init(g_idle_thread_list[i]);
    atomic_set(&g_idle_thread_list[i]->refc, 0);

    g_global_thread_list[i] = g_idle_thread_list[i];

    store_tcb_info(i, g_idle_thread_list[i]);
  }
}

/*
 * allocate a TCB
 */
TCB *alloc_tcb(void)
{
  TCB *tcb = NULL;

#if 0
  if (per_cpu(local_tcb_list).count) {
    tcb = dl_list_entry(per_cpu(local_tcb_list).tcb_list.next, TCB, tcb_link);
    dl_list_del(&tcb->tcb_link);
    per_cpu(local_tcb_list).count--;

    if (tcb->refc.c > 0) {
      while(1) {
        lk_print_xy(40, 6, "refcbug: %d", tcb->refc.c);
      }
    }
  } else {
    spinlock_lock(&g_global_tcb_list.lock);
    if (dl_list_empty(&g_global_tcb_list.tcb_list)) {
      debug_halt((char *) __func__, __LINE__);
    }
    tcb = dl_list_entry(g_global_tcb_list.tcb_list.next, TCB, tcb_link);

    dl_list_del(&tcb->tcb_link);
    g_global_tcb_list.count--;
    mfence();
    spinlock_unlock(&g_global_tcb_list.lock);
  }
#else
  spinlock_lock(&g_global_tcb_list.lock);
  if (dl_list_empty(&g_global_tcb_list.tcb_list)) {
    debug_halt((char *) __func__, __LINE__);
  }
  tcb = dl_list_entry(g_global_tcb_list.tcb_list.next, TCB, tcb_link);

  dl_list_del(&tcb->tcb_link);
  g_global_tcb_list.count--;
  mfence();
  spinlock_unlock(&g_global_tcb_list.lock);
#endif

  // initlaize tcb
  atomic_set(&tcb->refc, 1);

  tcb_lock_init(tcb);
  dl_list_init(&tcb->tcb_link);

  tcb->user_stack = 0x0;
  tcb->time_slice = THREAD_DEFAULT_TIME_SLICE;
  tcb->remaining_time_slice = THREAD_DEFAULT_TIME_SLICE;
  tcb->time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
  tcb->remaining_time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
  atomic_set(&tcb->intention, 0);
  tcb->running_core = -1;
  INIT_CORE_MASK(&tcb->core_mask);
  tcb->name = THREAD_INIT_NAME;

  tcb->state = THREAD_STATE_CREATED;
  tcb->sched_list = NULL;

  return tcb;
}

/**
 * allocate a TCB for the idle thread
 */
TCB *alloc_tcb_idle(int cid)
{
  TCB* tcb = NULL;
  tcb = g_idle_thread_list[cid];
  atomic_inc(&tcb->refc);

  return tcb;
}

int is_idle_thread(TCB * tcb)
{
  return (tcb->id < MAX_PROCESSOR_COUNT) ? 1 : 0;
}

void init_tcb(TCB * tcb, QWORD stack)
{
  if (is_idle_thread(tcb)) {
    tcb->remaining_time_slice = THREAD_IDLE_THREAD_TIME_SLICE;
    tcb->remaining_time_quantum = THREAD_IDLE_THREAD_TIME_SLICE;
  } else {
//TODO:
#if 0 
    tcb->stack = (QWORD) stack + CONFIG_TCB_SIZE;
    tcb->stack_base = tcb->stack;
    tcb->remaining_time_slice = THREAD_DEFAULT_TIME_SLICE;
    tcb->remaining_time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
#else
    tcb->stack_base = (QWORD) stack + CONFIG_TCB_SIZE;
    tcb->remaining_time_slice = THREAD_DEFAULT_TIME_SLICE;
    tcb->remaining_time_quantum = THREAD_DEFAULT_TIME_QUANTUM;
#endif

    tcb->name = THREAD_INIT_NAME;
  }

  /*tcb->state = THREAD_STATE_CREATED;*/

  tcb->acc_ltc = 0;
}

void init_tcb_destroy(TCB * tcb)
{
  // free the allocated stack memory
  free((void *) (tcb->stack_base - CONFIG_TCB_SIZE));

  init_tcb(tcb, 0);
  tcb->state = THREAD_STATE_NOTALLOC;  // XXX:
  tcb->running_core = -1;
}

#if 1
static atomic_t tdcnt;
static atomic_t trcnt;
#endif

// tcb destory mean to move input TCB into g_global_tcb_list.
// it doesn't need to free memory
static void destroy_tcb(TCB *t)
{
#if 1
  if (per_cpu(local_tcb_list).count > 1 && spinlock_trylock(&g_global_tcb_list.lock)) {
    // if local_tcb_list is not empty and g_global_tcb_list is race free,
    // then insert free tcb to the g_global_tcb_list
    dl_list_add_tail(&t->tcb_link, &g_global_tcb_list.tcb_list);
    g_global_tcb_list.count++;
    mfence();
    init_tcb_destroy(t);
    t->gen++;
    spinlock_unlock(&g_global_tcb_list.lock);
  } else if (per_cpu(local_tcb_list).count > 4) {
    // if local_tcb_list is too long, it append local_tcb_list to g_global_tcb_list,
    // and then free tcb is inserted to local_tcb_list
    spinlock_lock(&g_global_tcb_list.lock);
    dl_list_append(&per_cpu(local_tcb_list).tcb_list, &g_global_tcb_list.tcb_list);
    g_global_tcb_list.count += per_cpu(local_tcb_list).count;
    mfence();
    spinlock_unlock(&g_global_tcb_list.lock);

    init_tcb_destroy(t);
    t->gen++;
    dl_list_init(&per_cpu(local_tcb_list).tcb_list);
    dl_list_add_tail(&t->tcb_link, &per_cpu(local_tcb_list).tcb_list);
    per_cpu(local_tcb_list).count = 1;
  } else {
    init_tcb_destroy(t);
    t->gen++;
    dl_list_add_tail(&t->tcb_link, &per_cpu(local_tcb_list).tcb_list);
    per_cpu(local_tcb_list).count++;
  }
#else
  spinlock_lock(&g_global_tcb_list.lock);
  list_insert_tail(&t->tcb_link, &g_global_tcb_list.tcb_list);
  g_global_tcb_list.count++;
  mfence();
  init_tcb_destroy(t);
  t->gen++;
  spinlock_unlock(&g_global_tcb_list.lock);
#endif

#if 1
  atomic_inc(&tdcnt);
  lk_print_xy(50, 23, "tc: %q, td: %q, %q  ", trcnt.c, tdcnt.c, trcnt.c -tdcnt.c);
#endif
}

/*
 * get current TCB
 */
TCB *get_current_tcb(void)
{
  TCB *tcb = get_current();
  atomic_inc(&tcb->refc);
  return tcb;
}

/*
 * get TCB
 */
TCB *get_tcb(QWORD tid)
{
  TCB *tcb = NULL;

  if (unlikely(tid < 0 || tid >= MAX_PROCESSOR_COUNT + CONFIG_NUM_THREAD))
    return NULL;
  tcb = g_global_thread_list[tid];
  if (tcb->state == THREAD_STATE_NOTALLOC)
    return NULL;
  atomic_inc(&tcb->refc);

  return tcb;
}

/*
 * put TCB
 */
void put_tcb(TCB *tcb)
{
  if (atomic_dec_and_test(&tcb->refc)) {
    if (tcb->state == THREAD_STATE_EXITED) {
      destroy_tcb(tcb);
    } else if (tcb->state != THREAD_STATE_NOTALLOC) {
      debug_halt((char *)__func__, tcb->id);
    }
  }
}

extern void return_from_setup_thread(void);

/* 
 * Store the information of context and stack into TCB
 */
static void setup_thread(TCB * tcb, QWORD flags, QWORD entrypoint, void *stack_address, QWORD thread_param)
{
  QWORD *kstack = (QWORD *) tcb->stack;

  *(QWORD*) ((QWORD) stack_address + CONFIG_TCB_SIZE - sizeof(QWORD)) = (QWORD) __thread_exit;

  // user context setup
  *(--kstack) = GDT_KERNEL_DATA_SEGMENT | SELECTOR_RPL_0; // SS
  *(--kstack) = (QWORD) stack_address + CONFIG_TCB_SIZE - sizeof(QWORD); // RSP
  *(--kstack) = THREAD_FLAGS_IOPL | THREAD_FLAGS_IF;	    // RFLAGS, interrupt enable
  *(--kstack) = GDT_KERNEL_CODE_SEGMENT | SELECTOR_RPL_0; // CS

  *(--kstack) = (QWORD) entrypoint;   // RIP
  *(kstack - 6) = thread_param;			// thread parameters, RDI
  *(kstack - 7) = thread_param;			// TODO, RSI

  *(--kstack) = (QWORD) stack_address + CONFIG_TCB_SIZE; // RBP
//  *(--kstack) = (QWORD) stack_address + CONFIG_TCB_SIZE - sizeof(QWORD); // RBP
  kstack -= THREAD_NUM_GP_REGISTER;

  *(--kstack) = GDT_KERNEL_DATA_SEGMENT | SELECTOR_RPL_0;       // DS
  *(--kstack) = GDT_KERNEL_DATA_SEGMENT | SELECTOR_RPL_0;       // ES
  *(--kstack) = GDT_KERNEL_DATA_SEGMENT | SELECTOR_RPL_0;       // FS
  *(--kstack) = GDT_KERNEL_DATA_SEGMENT | SELECTOR_RPL_0;       // GS

  // kernel context setup
  *(--kstack) = (QWORD) return_from_setup_thread;
  *(--kstack) = 0x0;            // dummy RBP
  *(--kstack) = 0x0;            // dummy RBX
  *(--kstack) = 0x0;            // dummy R12
  *(--kstack) = 0x0;            // dummy R13
  *(--kstack) = 0x0;            // dummy R14
  *(--kstack) = 0x0;            // dummy R15
  //

  tcb->stack = (QWORD) kstack;
}

/**
 * Change thread's intention to THREAD_INTENTION_EXITED
 * The thread is terminatedby other thread
 */
int __thread_exit(int tid)
{
  TCB *tcb = NULL;

// TODO:
#if 0
  tcb = (tid == (QWORD) NULL) ? get_current_tcb() : get_tcb(tid);
#else
  tcb = get_tcb(get_current()->id);
#endif

  atomic_set(&tcb->intention, THREAD_INTENTION_EXITED);
  tcb->name = THREAD_INIT_NAME;

  put_tcb(tcb);
  schedule(THREAD_INTENTION_EXITED);

  return 0;
}

/**
 * exit thread
 */
int thread_exit(TCB *tcb)
{
  tcb->state = THREAD_STATE_EXITED;
  /*atomic_and(~THREAD_INTENTION_EXITED, &t->intention);*/
  atomic_set(&tcb->intention, atomic_get(&tcb->intention) & ~THREAD_INTENTION_EXITED);

  return 0;
}

/*
 * suspend thread
 */
int thread_suspend(int tid)
{
  TCB *tcb = get_tcb(tid);

  tcb_lock(tcb);
  /*if (t->state == THREAD_STATE_READY || t->state == THREAD_STATE_RUNNING)*/
  atomic_or(THREAD_INTENTION_BLOCKED, &tcb->intention);
  tcb_unlock(tcb);

  put_tcb(tcb);

  return 0;
}

/*
 * wake up thread
 */
int thread_wake_up(int tid)
{                               // resume
  TCB *tcb = get_tcb(tid);

  tcb_lock(tcb);
  /*if (t->state == THREAD_STATE_BLOCKED)*/
  atomic_or(THREAD_INTENTION_READY, &tcb->intention);
  tcb_unlock(tcb);

  put_tcb(tcb);

  return 0;
}

/*
 * set thread name
 */
void lk_thread_set_name(QWORD tid, QWORD name)
{
  TCB *tcb = NULL;
  tcb = get_tcb(tid);
  if (tcb == NULL)
    return;
  tcb->name = name;
  put_tcb(tcb);
}

/*
 * look up thread tid
 */
QWORD lk_thread_lookup_tid(QWORD name)
{
  int i = 0;
  TCB *tcb = NULL;
  QWORD tid = 0;

  for (i = MAX_PROCESSOR_COUNT; i < CONFIG_NUM_THREAD + MAX_PROCESSOR_COUNT; i++) {
    tcb = get_tcb(i);
    if (tcb == NULL)
      continue;

    if (tcb->state == THREAD_STATE_NOTALLOC) {
      put_tcb(tcb);
      continue;
    }

    tcb_lock(tcb);
    if (tcb->name == name) {
      tid = tcb->id;
      tcb_unlock(tcb);
      put_tcb(tcb);
      break;
    }
    tcb_unlock(tcb);
    put_tcb(tcb);
  }

  return tid;
}

/*
 * look up thread state
 */
QWORD lk_thread_lookup_state(QWORD tid)
{
  int i = 0;
  TCB *tcb = NULL;
  QWORD state = 0;

  for (i = MAX_PROCESSOR_COUNT; i < CONFIG_NUM_THREAD + MAX_PROCESSOR_COUNT; i++) {
    tcb = get_tcb(i);
    if (tcb == NULL)
      continue;

    tcb_lock(tcb);
    if (tcb->name == tid) {

      tcb_unlock(tcb);
      put_tcb(tcb);
      break;
    }
    tcb_unlock(tcb);
    put_tcb(tcb);
  }

  return state;
}

/**
 * Create thread
 */
int create_thread(QWORD ip, QWORD argv, int core_mask)
{
  TCB *thr = NULL;
  void *stack_address;
  core_set_t cst;

  lk_memset(&cst, 0, sizeof(core_set_t));

#if 0  
  if (task->thread_count.c >= MAX_NUM_THREAD)
    debug_halt((char *)__func__, __LINE__);
#endif

  // TODO: If fail ????
  thr = alloc_tcb();

  get_tcb(thr->id);

  tcb_lock(thr);

  stack_address = alloc(PAGE_SIZE_4K);
  lk_print("Thread created: %d, stack: %q\n", thr->id, stack_address);

  init_tcb(thr, (QWORD) stack_address);
  
  thr->running_core = core_mask;
  set_core(thr, &cst, core_mask);

  // Initialize context of the thread
  if (ip == 0)
    setup_thread(thr, THREAD_KERNEL, (QWORD) elf_load_entry, stack_address, argv);
  else
    setup_thread(thr, THREAD_KERNEL, ip, stack_address, argv);

  // add migrating list
  spinlock_lock(&g_migrating_list.lock);

  dl_list_add_tail(&thr->tcb_link, &g_migrating_list.tcb_list);
  g_migrating_list.count++;
  thr->sched_list = &g_migrating_list;
  thr->state = THREAD_STATE_READY;

  spinlock_unlock(&g_migrating_list.lock);

  tcb_unlock(thr);

  put_tcb(thr);

  atomic_inc(&trcnt);
  lk_print_xy(50, 23, "tc: %q, td: %q, %q  ", trcnt.c, tdcnt.c, trcnt.c-tdcnt.c);

  return thr->id;
}

/*
 * Execute unikernel application
 */
int lk_app_exec(void *app_ptr)
{
  int tid;

  elf_load(app_ptr);

  // Clear screen
  lk_clear_screen();

  tid = create_thread(0, 0, 0);

  atomic_inc(&trcnt);

  lk_print_xy(50, 23, "tc: %q, td: %q, %q  ", trcnt.c, tdcnt.c, trcnt.c-tdcnt.c);

  return tid;
}

/*
 * is available next TCB
 */
static inline BYTE available_next(TCB * tcb)
{
  return ((tcb->remaining_time_slice > 0)
          && (tcb->state == THREAD_STATE_READY) ? TRUE : FALSE);
}

/*
 * select next thread
 */
static TCB *select_next_thread(void)
{
  int found = 0;
  TCB *next_tcb = NULL, *tmp_ptr = NULL, *found_tcb = NULL;
  int cid = get_current()->running_core;

retry:
  found = 0;
  dl_list_for_each_safe(next_tcb, tmp_ptr, &per_cpu(runnable_list).tcb_list, TCB, tcb_link) {
    tcb_lock(next_tcb);

    // If there exists only one thread in runnable list then
    // Move to-be-BLOCKED thread in the runnable list to the blocked list.
    // Move to-be-EXITED thread in the runnable and blocked list to the free list
    if (atomic_get(&next_tcb->intention) & THREAD_INTENTION_EXITED) {

      // and if prev=next, schedule idle thread
      if (get_current()->id == next_tcb->id) {
        refill_time_slice(g_idle_thread_list[cid]);

        return g_idle_thread_list[cid];
      }

      if (thread_exit(next_tcb) == -1) {
        tcb_unlock(next_tcb);
        continue;
      }

      per_cpu(runnable_list).count--;
      dl_list_del_init(&next_tcb->tcb_link);

      tcb_unlock(next_tcb);

      // Destroy TCB
      put_tcb(next_tcb);
      continue;
    } else if (atomic_get(&next_tcb->intention) & THREAD_INTENTION_BLOCKED) {
      per_cpu(runnable_list).count--;
      dl_list_del_init(&next_tcb->tcb_link);

      next_tcb->state = THREAD_STATE_BLOCKED;
      /*atomic_and(~THREAD_INTENTION_BLOCKED, &next_tcb->intention);*/
      atomic_set(&next_tcb->intention, atomic_get(&next_tcb->intention) & ~THREAD_INTENTION_BLOCKED);

      dl_list_add_tail(&next_tcb->tcb_link,
                       &per_cpu(blocked_list).tcb_list);
      per_cpu(blocked_list).count++;

      tcb_unlock(next_tcb);

      continue;
    }

    if (available_next(next_tcb) == TRUE) {
      if (core_is_allowed(next_tcb, cid)) {
        if (!found) {
          found = 1;
          found_tcb = next_tcb;
          next_tcb->state = THREAD_STATE_RUNNING;
          /*per_cpu(runnable_list).count--;*/
          /*list_remove_init(&next_tcb->tcb_link);*/
          g_running_thread_list[cid] = next_tcb;
        }
      } else if (spinlock_trylock(&g_migrating_list.lock)) {
        // migrate the next_tcb to the g_migrating_list
        per_cpu(runnable_list).count--;
        dl_list_del_init(&next_tcb->tcb_link);

        dl_list_add_tail(&next_tcb->tcb_link, &g_migrating_list.tcb_list);
        g_migrating_list.count++;
	spinlock_unlock(&g_migrating_list.lock);
      }
    }
    tcb_unlock(next_tcb);
  }

  if (!found) {
    int refilled = 0;

    dl_list_for_each(next_tcb, &per_cpu(runnable_list).tcb_list, TCB, tcb_link) {
      tcb_lock(next_tcb);
      if (next_tcb->state == THREAD_STATE_READY && next_tcb->remaining_time_slice <= 0) {
        refill_time_slice(next_tcb);
        refilled = 1;
      }
      tcb_unlock(next_tcb);
    }
    if (refilled)
      goto retry;

    refill_time_slice(g_idle_thread_list[cid]);
    return g_idle_thread_list[cid];
  }

  return found_tcb;
}

// handle Thread Load Balancing
// 각 코어별 최저 thread 개수에 따른 thread 할당
int get_cid_of_min_thread(TCB * next_tcb, int cid)
{
  TCB *tcb = NULL;
  QWORD min_thread_count = 0xFFFFFFFFFFFFFFFF;
  QWORD cid_thread_count = 0;
  QWORD tmp_thread_count = 0;
  int i = 0, min_cid = -1;

  // core_mask 가 해당 코어에만 지정한 경우
  // 무조건 해당 코어에 할당
#if 0
  QWORD core_affinity;

  core_affinity = ~(0x01 << cid);
  if ((next_tcb->core_mask != 0x0)
      && ((next_tcb->core_mask & core_affinity) == core_affinity))
    return cid;
#else
  if(!ISONLY_CORE_MASK(&next_tcb->core_mask, cid))
    return cid;
#endif

  // 아래 요청 코어의 로직은 for loop 안에 넣어서 처리해도 되나
  // 빠른 응답을 위해 따로 분리
  // 요청 코어의 thread 개수 검사
  tcb = g_running_thread_list[cid];

  if (tcb->id >= MAX_PROCESSOR_COUNT)
    cid_thread_count = runnable_list[cid].count + 1;
  else
    cid_thread_count = runnable_list[cid].count;

  // 할당된 thread 가 0인 경우, 요청 코어에 할당
  if (!cid_thread_count)
    return cid;

  min_cid = cid;
  // 전체 코어에서 최저 thread 가 할당된 코어 검색
  for (i = 0; i < MAX_PROCESSOR_COUNT; i++) {
    // check whether core is waked or not
    if (g_running_thread_list[i] == NULL)
      continue;

    // core affinity check
    if ((i == cid) || !core_is_allowed(next_tcb, i))
      continue;

    tcb = g_running_thread_list[i];

    if (tcb->id >= MAX_PROCESSOR_COUNT)
      tmp_thread_count = runnable_list[i].count + 1;
    else
      tmp_thread_count = runnable_list[i].count;

    if (tmp_thread_count < min_thread_count) {
      min_thread_count = tmp_thread_count;

      min_cid = i;

      if (!min_thread_count)
        break;
    }
  }

  // 요청 코어의 thread 개수와 최저 thread 개수가 동일한 경우
  // 요청 코어에 할당
  if (cid_thread_count <= min_thread_count)
    return cid;
  else
    return min_cid;
}

/*
 * post context switch for previous thread
 * - THREAD_INTENTION_READY
 * - THREAD_INTENTION_BLOCKED
 * - THREAD_INTENTION_EXITED
 */
void __post_context_switch(TCB * prev)
{
  int prev_intention = 0;

  if ((prev->state == THREAD_STATE_EXITED) || (prev->state == THREAD_STATE_NOTALLOC))
    return;

  // spinlock_lock(&prev->tcb_lock); ???
  prev_intention = atomic_get(&prev->intention);
  if (prev_intention & THREAD_INTENTION_READY) {
    tcb_lock(prev);

    prev->state = THREAD_STATE_READY;
    /*atomic_and(~THREAD_INTENTION_READY, &prev->intention);*/
    atomic_set(&prev->intention, atomic_get(&prev->intention) &  ~(THREAD_INTENTION_READY | THREAD_INTENTION_BLOCKED));

    tcb_unlock(prev);
  } else if (prev_intention & THREAD_INTENTION_BLOCKED) {
    tcb_lock(prev);

    per_cpu(runnable_list).count--;
    dl_list_del_init(&prev->tcb_link);

    prev->state = THREAD_STATE_BLOCKED;
    /*atomic_and(~THREAD_INTENTION_BLOCKED, &prev->intention);*/
    atomic_set(&prev->intention, atomic_get(&prev->intention) & ~THREAD_INTENTION_BLOCKED);

    dl_list_add_tail(&prev->tcb_link, &per_cpu(blocked_list).tcb_list);
    per_cpu(blocked_list).count++;

    tcb_unlock(prev);
  } else if (prev_intention & THREAD_INTENTION_EXITED) {
    tcb_lock(prev);

    // If prev=next, schedule idle thread
    if (get_current()->id == prev->id) {
      tcb_unlock(prev);
    } else {

      if (thread_exit(prev) == -1) {
        tcb_unlock(prev);
      } else {
        per_cpu(runnable_list).count--;
        dl_list_del_init(&prev->tcb_link);

        tcb_unlock(prev);

        // Destroy TCB
        put_tcb(prev);
      }
    }
  } else {
#if 0
    void *ra = __builtin_return_address(0);
    while (1)
      lk_print_xy(40, 1, "ra: %q, %d", prev->id, (unsigned long) ra);
#else
//    debug_halt((char *)__func__, (QWORD)prev->id);
#endif
  }
}


/*
 * post context switch
 * - post context switch for previous thread
 * - move threads in the migrating list to the runnable list
 * - move to-be-READY thread in the blocked list to the runnable list
 */
void post_context_switch(TCB * prev)
{
  TCB *next_tcb = NULL, *tmp_ptr = NULL;
  int cid = -1;
  int next_intention = 0;
  int found = 0;
  int min_cid;

  __post_context_switch(prev);

  // Move threads in the migrating list to the runnable list (runqueue) if any.
  if (per_cpu(runnable_list).count + per_cpu(blocked_list).count < RUNNABLE_MAX) {

    if (unlikely(g_migrating_list.count) && spinlock_trylock(&g_migrating_list.lock)) {
      cid = get_current()->running_core;

      // Find next_tcb->core_mask thread that matched cid
      dl_list_for_each_safe(next_tcb, tmp_ptr, &g_migrating_list.tcb_list, TCB, tcb_link) {

       if (next_tcb->running_core == cid) {
          tcb_lock(next_tcb);

          dl_list_del_init(&next_tcb->tcb_link);
          g_migrating_list.count--;

          dl_list_add_tail(&next_tcb->tcb_link, &per_cpu(runnable_list).tcb_list);
          per_cpu(runnable_list).count++;

          next_tcb->running_core = cid;
          tcb_unlock(next_tcb);
        }

        // If threads in runnable and block list exceed MAX_NUM, break
        if (per_cpu(runnable_list).count + per_cpu(blocked_list).count == RUNNABLE_MAX) {
          found = 1;
          break;
        }
      }

      // Find at most one next_tcb->core_mask(include -1) thread that matched cid
      if (!found) {
        dl_list_for_each_safe(next_tcb, tmp_ptr, &g_migrating_list.tcb_list, TCB, tcb_link) {
          if (core_is_allowed(next_tcb, cid)) {

            min_cid = get_cid_of_min_thread(next_tcb, cid);

            if (cid == min_cid) {
              tcb_lock(next_tcb);

              dl_list_del_init(&next_tcb->tcb_link);
              g_migrating_list.count--;

              dl_list_add_tail(&next_tcb->tcb_link, &per_cpu(runnable_list).tcb_list);
              per_cpu(runnable_list).count++;

              next_tcb->running_core = cid;
              tcb_unlock(next_tcb);
              break;
            }
          }

          // If threads in runnable and block list exceed MAX_NUM, break
          if (per_cpu(runnable_list).count + per_cpu(blocked_list).count == RUNNABLE_MAX) {
            break;
          }
        }
      }
      spinlock_unlock(&g_migrating_list.lock);
    }
  }

  // Move to-be-READY thread in the blocked list to the runnable list (runqueue).
  if (unlikely(per_cpu(blocked_list).count)) {
    dl_list_for_each_safe(next_tcb, tmp_ptr, &per_cpu(blocked_list).tcb_list, TCB, tcb_link) {
#ifndef NO_TCB_LOCK
      if (!(atomic_get(&next_tcb->intention) & (THREAD_INTENTION_READY | THREAD_INTENTION_EXITED)))
        continue;
#endif
      tcb_lock(next_tcb);
      next_intention = atomic_get(&next_tcb->intention);
      if (next_intention & THREAD_INTENTION_EXITED) {
        if (get_current()->id == next_tcb->id) {
          tcb_unlock(next_tcb);
        } else {
          if (thread_exit(next_tcb) == -1) {
            tcb_unlock(next_tcb);
          } else {
            per_cpu(blocked_list).count--;
            dl_list_del_init(&next_tcb->tcb_link);

            tcb_unlock(next_tcb);

            // Destroy TCB
            put_tcb(next_tcb);
          }
        }
      } else if (next_intention & THREAD_INTENTION_READY) {
        per_cpu(blocked_list).count--;
        dl_list_del_init(&next_tcb->tcb_link);

        next_tcb->state = THREAD_STATE_READY;
	/*atomic_and(~THREAD_INTENTION_READY, &next_tcb->intention);*/
	atomic_set(&next_tcb->intention, atomic_get(&next_tcb->intention) & 
			~(THREAD_INTENTION_READY | THREAD_INTENTION_BLOCKED));

        dl_list_add_tail(&next_tcb->tcb_link,
                         &per_cpu(runnable_list).tcb_list);
        per_cpu(runnable_list).count++;

        tcb_unlock(next_tcb);
      }
    }
  }
}

/*
 * refill time slice
 */
static int refill_time_slice(TCB * tcb)
{
  long q = 0;

  if (tcb->remaining_time_slice <= 0) {
    if (tcb->time_quantum == THREAD_INFINITE_TIME_QUANTUM)
      tcb->remaining_time_slice = tcb->time_slice;
    else {
      q = tcb->remaining_time_quantum;
      q -= tcb->time_slice;
      if (q < 0)
        debug_niy((char *) __func__, __LINE__);
      tcb->remaining_time_quantum = q;
      tcb->remaining_time_slice = tcb->time_slice;
    }
  }

  return 0;
}

/**
 * Adjust consumed time slice and decrease remaining time slice
 */
long adjust_time_slice(void)
{
  long consumed_time_slice = 0;
  long consumed_time_tick = 0;
  TCB *curr = get_current();

  lapic_consumed_time_slice(&consumed_time_slice, &consumed_time_tick);
  if (consumed_time_slice == 0) {
    curr->acc_ltc += consumed_time_tick;

    if (curr->acc_ltc >= lapic_get_ticks_in_1ms()) {
      curr->acc_ltc = 0;
      consumed_time_slice = 1;
    }
  }
  return consumed_time_slice;
}

/**
 * Decrease remaining time slice of input tcb
 */
void decrease_remaining_time_slice(long consumed_time_slice)
{
  TCB *curr = get_current();
  int int_pending = 0;

  curr->remaining_time_slice -= consumed_time_slice;

  int_pending = lapic_timer_interrupt_pending();
  if (int_pending)
    lapic_send_eoi();
}

/**
 * Schedule
 */
BOOL schedule(QWORD intention)
{
  TCB *curr = NULL;
  TCB *next = NULL, *prev = NULL;
  int cid = -1;
  long consumed_time_slice = 0;

  curr = get_current();
  cid = curr->running_core;

  consumed_time_slice = adjust_time_slice();
  lapic_stop_timer();
  decrease_remaining_time_slice(consumed_time_slice);

  tcb_lock(curr);
  /*atomic_or(intention, &curr->intention);*/
  atomic_set(&curr->intention, intention | atomic_get(&curr->intention));
  tcb_unlock(curr);

  next = select_next_thread();
  if (next == curr) {
    lapic_start_timer_oneshot(curr->remaining_time_slice);

    post_context_switch(curr);
    curr->state = THREAD_STATE_RUNNING;
    return TRUE;
  }

  lapic_start_timer_oneshot(next->remaining_time_slice);

  next->running_core = cid;
  running_thread[get_apic_id()] = next;

  prev = context_switch(curr, next);

  post_context_switch(prev);

  // signal handling
  if (curr->signal_flag != 0) {
    do_signal(curr->signal_handler, curr->id);

    // initialize signal handler
    curr->signal_flag = 0;
    curr->signal_handler = 0;
  }

  return FALSE;
}

void release_thread_spl(TCB * prev)
{
  post_context_switch(prev);
}

/**
 * Initially setup for idle thread
 */
void setup_idle_thread()
{
  TCB *thread_idle = NULL;
  int cid = get_apic_id();

  // idle thread(thread0) creation for BSP
  // This must be the first alloc_tcb() call
  thread_idle = alloc_tcb_idle(cid);
  init_tcb(thread_idle, 0);

  thread_idle->state = THREAD_STATE_RUNNING;
  set_cr3(CONFIG_KERNEL_PAGETABLE_ADDRESS);

  g_running_thread_list[cid] = thread_idle;
  running_thread[cid] = thread_idle;

  lapic_start_timer_oneshot(thread_idle->remaining_time_slice);

  enable_interrupt();
}

/**
 * IDLE thread for BSP
 */
void start_idle_thread(int thread_type)
{
  int cid = get_apic_id();
  int cnt = 0;
  int wait_cnt = 0;

  setup_idle_thread();

  if (thread_type == THREAD_TYPE_BSP) {
    while (g_ap_count != g_cpu_size)
      // TODO: if no wait_cnt, dont work
      lk_print_xy(0, 20, "Wait until all cores are ready!!: (%d/%d) %q  ", g_ap_count, g_cpu_size, wait_cnt++);

    lk_app_exec((void *) (va(CONFIG_APP_ADDRESS)));
  }

  while (!shutdown_kernel) {
    // CPU IDLE
    lk_print_xy(30, 23, "%d, %x", cid, cnt++);
  }

  disable_interrupt() ;
  HALT() ;
}

/**
 * Core mask
 */
void set_core(TCB *tcb, core_set_t *cst, int core)
{
  if (core == -1) {
    INIT_CORE_MASK(cst);
  } else {
    FILL_CORE_MASK(cst);
    CLEAR_CORE_MASK(cst, core);
  }

  COPY_CORE_MASK(&tcb->core_mask, cst);
}

void do_exit(int arg)
{
  __thread_exit(arg);
}

void block_current_task(void)
{
  // To be implemented
}

void reschedule(void)
{
  // To be implemented
}

int wakeup_task(tid_t id)
{
  // To be implemented
  return 0;
}

tid_t sys_getpid(void)
{
  return get_current()->id;
}

int sys_getprio(tid_t *id)
{
  return -ENOSYS;
}

int sys_setprio(tid_t *id, int prio)
{
  return -ENOSYS;
}

void sys_exit(int arg)
{
  // To be implemented
}

int sys_clone(tid_t *id, void *ep, void *argv)
{
  tid_t tid;

  tid = create_thread((QWORD) ep, (QWORD) argv, -1);

  if (tid) {
    if (id != NULL) 
      *id = tid;

    return 0;
  } else {
    return -1;
  }
}

void sys_yield(void)
{
  // To be implemented
}
