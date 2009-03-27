#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>

#include <dce/rpc.h>
#include <dce/dcethread.h>
#include <compat/rpcstatus.h>
#include <lw/ntstatus.h>

#include <lsa/lsa.h>
#include <lsarpcsrv.h>

#include "rpcctl-register.h"
#include "externs.h"


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
