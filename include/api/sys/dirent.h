/* <dirent.h> includes <sys/dirent.h>, which is this file.  On a
   system which supports <dirent.h>, this file is overridden by
   dirent.h in the libc/sys/.../sys directory.  On a system which does
   not support <dirent.h>, we will get this file which uses #error to force
   an error.  */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
#error "<dirent.h> not supported"
#endif
#ifdef __cplusplus
}
#endif

struct __dirstream
{
        off_t tell;
        int fd;
        int buf_pos;
        int buf_end;
        volatile int lock[1];
        /* Any changes to this struct must preserve the property:
 *          * offsetof(struct __dirent, buf) % sizeof(off_t) == 0 */
        char buf[2048];
};

typedef struct __dirstream DIR;

