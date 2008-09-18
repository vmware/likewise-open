/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsasystem.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) System Headers
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifdef UNICODE
      #undef UNICODE
#endif

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#ifdef WIN32

   #include <windows.h>
   #include <rpc.h>
   #define SECURITY_WIN32
   #include <security.h>
   #include <ntsecapi.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
   #include <stdlib.h>
#endif

#ifdef HAVE_SYS_VARARGS_H
   #include <sys/varargs.h>
#endif

#include <fcntl.h>

#ifdef HAVE_TIME_H
   #include <time.h>
#endif

#ifdef HAVE_SYS_TIME_H
   #include <sys/time.h>
#endif

#include <string.h>

#ifdef HAVE_STRINGS_H
   #include <strings.h>
#endif

#ifdef HAVE_STDBOOL_H
   #include <stdbool.h>
#endif

#ifndef WIN32
  #include <stdarg.h>
  #include <errno.h>	
  #include <netdb.h>
  #include <ctype.h>
  #include <wctype.h>
  #include <sys/types.h>
  #include <pthread.h>
  #include <syslog.h>
  #include <signal.h>
  #include <limits.h>
  #include <unistd.h>
  #include <sys/stat.h>
  #include <dirent.h>
  #include <pwd.h>
  #include <grp.h>
  #include <regex.h>
  #include <sys/un.h>
  #include <dlfcn.h>
  #include <arpa/inet.h>
  #include <arpa/nameser.h>
  #include <netinet/in.h>
  #include <resolv.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if HAVE_WC16STR_H
#include <wc16str.h>
#endif

#if defined(__hpux__) && defined(_XOPEN_SOURCE_EXTENDED)
#    include "xpg_socket.h"
#endif

#include <assert.h>

#endif
