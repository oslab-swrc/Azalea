#ifndef __CMDS_H__
#define	__CMDS_H__

#define BOOL	unsigned char
#define BYTE	unsigned char
#define WORD	unsigned short
#define DWORD   unsigned int
#define QWORD   unsigned long

#define CONFIG_PAGE_OFFSET		0xFFFF8000C0000000
#define pa(v)  (((QWORD)v)-CONFIG_PAGE_OFFSET+(memory_start_addr))

//#define NO_TCB_LOCK

#define MAX_PROCESSOR_COUNT		288
#define CONFIG_NUM_THREAD		32767
#define MAX_PAGEFAULT_SIZE      1024

#define LK_CMD_PAGEFAULT        0x51
#define LK_CMD_STAT             0x52
#define LK_CMD_TIMER            0x53
#define LK_CMD_THREAD           0x54
#define LK_CMD_CONSOLE          0x55
#define LK_CMD_LOG              0x56

// init state
#define INIT_IA32E_START_STAT               0x01
#define INIT_GDT_SWITCH_IA32E_STAT          (0x01 << 1)
#define INIT_IDT_STAT                       (0x01 << 2)
#define INIT_PAGETABLE_STAT                 (0x01 << 3)
#define INIT_SYSTEMCALL_STAT                (0x01 << 4)
#define INIT_MEMORY_STAT                    (0x01 << 5)
#define INIT_SCHEDULER_STAT                 (0x01 << 6)
#define INIT_TASK_STAT                      (0x01 << 7)
#define INIT_ADDRESSSPACE_STAT              (0x01 << 8)
#define INIT_INTERRUPT_STAT                 (0x01 << 9)
#define INIT_TIMER_STAT                     (0x01 << 10)
#define INIT_APS_STAT                       (0x01 << 11)
#define INIT_LOAD_TSS_STAT                  (0x01 << 12)
#define INIT_LAPIC_STAT                     (0x01 << 13)
#define INIT_LOADER_STAT                    (0x01 << 14)
#define INIT_IDLE_THREAD_STAT               (0x01 << 15)

typedef struct spinlock_struct {
  volatile unsigned long lock;
} spinlock_u __attribute__ ((aligned (8)));

typedef struct {
  int     thread_id;
  int     cid;
  QWORD   fault_addr;
  QWORD   error_code;
  QWORD   rip;
} PF_INFO;

typedef struct {
  QWORD   pf_count;
  PF_INFO info[MAX_PAGEFAULT_SIZE];
} PF_AREA;

// Thread state definitions
enum thread_state_enum {
  THREAD_STATE_NOTALLOC=0,
  THREAD_STATE_CREATED=1,
  THREAD_STATE_RUNNING,
  THREAD_STATE_READY,
  THREAD_STATE_BLOCKED,
  THREAD_STATE_EXITED,
};

struct link {
  struct link *next;
  struct link *prev;
};

struct thread_list {
  spinlock_u lock;
  QWORD count;
  struct link tcb_list;
};

typedef struct
{
  int c;
} atomic_u;

typedef struct core_set_struct {
  QWORD mask[4];
} core_set_t;

typedef void (*signal_handler_t)(int);
typedef struct thread_control_block_struct TCB;

struct thread_control_block_struct {
  QWORD stack;	      // XXX: don't move this field. This must be the frist field.
  QWORD user_stack;
  QWORD stack_base;
  QWORD id;
  DWORD gen;
  QWORD flags;
  WORD running_core;
  enum thread_state_enum state;
  atomic_u intention;
  QWORD  name;
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
  struct link tcb_link;

#ifndef NO_TCB_LOCK
  spinlock_u tcb_lock;
#endif
  atomic_u refc;
};

typedef struct {
  QWORD     init_stat;
  QWORD     timer[MAX_PROCESSOR_COUNT];
  PF_AREA   pf_area;
  TCB       *thread_area[MAX_PROCESSOR_COUNT+CONFIG_NUM_THREAD];
} SHELL_STORAGE_AREA;

#endif  /* __CMDS_H__ */
