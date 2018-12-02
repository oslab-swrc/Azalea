#include "signal.h"
#include "thread.h"
#include "dequeue.h"
#include "irq.h"
#include "console.h"
#include "sync.h"
#include "memory.h"

DECLARE_PERCPU(struct signal_list, local_sig_list);

/**
 * Initialize signal structure
 */
void signal_init(void)
{
  int i;

  for (i=0; i< MAX_PROCESSOR_COUNT; i++) {
    struct signal_list *l;
    l = &per_cpu_byid(local_sig_list, i);

    spinlock_init(&l->lock);
    l->count = 0;
    dl_list_init(&l->sig_list);
  }
}

/**
 *
 */
void signal_handler(int vector, QWORD rip)
{

}

/**
 * sys_signal register the signal hander in current thread
 */
int sys_signal(signal_handler_t handler)
{
  TCB *curr = get_current();
  curr->signal_handler = handler;

  return 0;
}

/**
 * sys_kill add signum to the tcb of destination thread's
 */
int sys_kill(tid_t dest, int signum)
{
  int dest_core;
  TCB *curr_tcb = get_current();
  TCB *dest_tcb = get_tcb(dest);

  dest_core = dest_tcb->running_core;

  // If target == curr, deliver signal to itself->call handler immediately
  if (dest_tcb->id == curr_tcb->id) {
    if (curr_tcb->signal_handler) {
      curr_tcb->signal_handler(signum);
    }
    put_tcb(dest_tcb);

    return 0;
  }

  // Add signal to task's signal list
  signal_t *signal = (signal_t *) alloc(sizeof(signal_t));
  signal->dest = dest;
  signal->signum = signum;

  spinlock_lock(&local_sig_list[dest_core].lock);
  dl_list_add_tail(&signal->sig_link, &per_cpu_byid(local_sig_list, dest_core).sig_list);
  local_sig_list[dest_core].count++;
  spinlock_unlock(&per_cpu_byid(local_sig_list, dest_core).lock);

  // TODO: If the caller thread is same as the calling thread?

  dest_tcb->signal_flag = 1;

  put_tcb(dest_tcb);

  return 0;
}

/**
 * do_signal call the handler registered in the tcb
 */
void do_signal(signal_handler_t handler, tid_t tid)
{
  signal_t *signal = NULL, *tmp_ptr = NULL;

  spinlock_lock(&per_cpu(local_sig_list).lock);
  dl_list_for_each_safe(signal, tmp_ptr, &per_cpu(local_sig_list).sig_list, signal_t, sig_link) { 
    if(signal->dest == tid) {
      __asm__ __volatile__ ("movq %0, %%rdi\n" : : "r"((QWORD) signal->signum));
      __asm__ __volatile__ ("call *%0\n" : : "r"(handler));
    
      dl_list_del(&signal->sig_link);
      per_cpu(local_sig_list).count--;
      free(signal);
    }
  }
  spinlock_unlock(&per_cpu(local_sig_list).lock);
}
