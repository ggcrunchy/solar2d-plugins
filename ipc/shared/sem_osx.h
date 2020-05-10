/* Mac OSX doesn't have an implementation of `sem_timedwait()`, so we
 * provide one. This implementation was written by Keith Shortridge
 * at the Australian Astronomical Observatory and can be found
 * here[1].
 * I think it contains a race condition where the monitoring thread
 * sends the disrupting signal before the calling thread actually
 * blocks on `sem_wait()` but so far I haven't found a better
 * solution.
 *
 *  [1]: https://github.com/attie/libxbee3/blob/master/xsys_darwin/sem_timedwait.c
 */

#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>


int ipc_sem_timedwait_( sem_t* sem, const struct timespec* abs_timeout );
#define  sem_timedwait  ipc_sem_timedwait_

#include "sem_posix.h"
/* we include it last, because it defines some "useful" macros */
#include "osx/sem_timedwait.c"

