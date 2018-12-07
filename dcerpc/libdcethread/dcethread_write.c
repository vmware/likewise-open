/* Clean */

#include <config.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#define write _write
#endif

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

ssize_t
dcethread_write(int fd, void *buf, size_t count)
{
    DCETHREAD_SYSCALL(ssize_t, write(fd, buf, (DCETHREAD_SIZE_T) count));
}

#endif /* API */
