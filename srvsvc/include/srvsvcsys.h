/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvsvc.h
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        System headers
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32

   #include <windows.h>
   #include <rpc.h>
   #define SECURITY_WIN32
   #include <ntsecapi.h>

#else

  #include <stdarg.h>
  #include <errno.h>
  #include <netdb.h>
  #include <ctype.h>

#if HAVE_LIMITS_H
  #include <limits.h>
#endif

#if HAVE_SYS_LIMITS_H
  #include <sys/limits.h>
#endif

#if HAVE_SYS_SYSLIMITS_H
  #include <sys/syslimits.h>
#endif

#include <sys/types.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <compat/dcerpc.h>
#include <dce/dcethread.h>
#include <compat/rpcstatus.h>
#include <dlfcn.h>
#include <dce/rpcexc.h>

#if HAVE_WC16STR_H
  #include <wc16str.h>
#endif

#endif


