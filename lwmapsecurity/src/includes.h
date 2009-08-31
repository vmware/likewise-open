#include "config.h"

#define ENABLE_LOGGING 0

#if ENABLE_LOGGING
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
   #include <stdlib.h>
#endif

/* pthread.h must be included before errno so that a thread-safe errno is
 used if available.
*/
#include <pthread.h>
#include <errno.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <lw/security-api.h>
#include <lw/rtlgoto.h>
#include <lw/base.h>
#include <lw/safeint.h>

#include <lwmapsecurity/lwmapsecurity-plugin.h>
#include <lwmapsecurity/lwmapsecurity.h>

#include "defs.h"
#include "structs.h"

#include "externs.h"

