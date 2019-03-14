#define MAX_CORES			(512)
#define MAX_TASKS			(((MAX_CORES * 2) + 2))
#define MAX_ISLE			(8)
#define KERNEL_STACK_SIZE	(8192)
#define DEFAULT_STACK_SIZE	(262144)
#define PACKAGE_VERSION 	"0.2.8"
#define MAX_FNAME			(128)

#define SAVE_FPU

#define DYNAMIC_TICKS

/* Define to use machine specific version of memcpy */
#define HAVE_ARCH_MEMCPY		(1)

/* Define to use machine specific version of memset */
#define HAVE_ARCH_MEMSET		(1)

/* Define to use machine specific version of strcpy */
/* #undef HAVE_ARCH_STRCPY */

/* Define to use machine specific version of strlen */
#define HAVE_ARCH_STRLEN		(1)

/* Define to use machine specific version of strncpy */
/* #undef HAVE_ARCH_STRNCPY */
