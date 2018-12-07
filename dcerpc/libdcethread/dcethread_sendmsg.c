/* Clean */

#include <config.h>

#include "dcethread-private.h"
#include "dcethread-util.h"
#include "dcethread-debug.h"
#include "dcethread-exception.h"

#ifdef API

#ifdef _WIN32

/*
 * dcethread_sendmsg() won't be supported at this level for
 * WIN32. The actual abstraction for this function is
 * found in ncklib/comsoc_bsd.c.
 */
ssize_t
dcethread_sendmsg(int s, const struct msghdr *msg, int flags)
{
    int sts = -1;

    return sts;
}


#else
ssize_t
dcethread_sendmsg(int s, const struct msghdr *msg, int flags)
{
    DCETHREAD_SYSCALL(ssize_t, sendmsg(s, msg, flags));
}
#endif

#endif /* API */
