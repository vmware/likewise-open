/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "ctbase.h"
#include "config/config.h"
#include <syslog.h>
#include "sysfuncs.h"

void
sys_vsyslog(
    int priority,
    const char *format,
    va_list ap
    )
{
#if defined(HAVE_VSYSLOG)
    vsyslog(priority, format, ap);
#else
    CENTERROR ceError;
    PSTR buffer = NULL;

    ceError = CTAllocateStringPrintfV(&buffer, format, ap);
    if (!ceError)
    {
        syslog(priority, "%s", buffer);
    }

    CT_SAFE_FREE_STRING(buffer);
#endif /* ! HAVE_VSYSLOG */
}

#if !defined(HAVE_RPL_MALLOC)
#undef malloc

//See http://wiki.buici.com/wiki/Autoconf_and_RPL_MALLOC
void*
rpl_malloc(size_t n)
{
    if (n == 0)
        n = 1;
    return malloc(n);
}

#endif /* ! HAVE_RPL_MALLOC */

#if !defined(HAVE_RPL_REALLOC)
#undef realloc

void*
rpl_realloc(void* buf, size_t n)
{
    return realloc(buf, n);
}

#endif /* ! HAVE_RPL_REALLOC */

BOOLEAN
IsRoot()
{
    return (getuid() == 0);
}

#ifndef HAVE_MKDTEMP
char *mkdtemp(char *template)
{
    unsigned int seed = 0;
    int attempt;
    struct timespec curtime;
    size_t templateLen;

    if(template == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    templateLen = strlen(template);
    if(templateLen < 6)
    {
        errno = EINVAL;
        return NULL;
    }
    if(strcmp(template + templateLen - 6, "XXXXXX"))
    {
        errno = EINVAL;
        return NULL;
    }

    for(attempt = 0; attempt < 50; attempt++)
    {
        if(clock_gettime(CLOCK_REALTIME, &curtime) < 0)
            return NULL;
        seed += (unsigned int)curtime.tv_nsec;
        seed += (unsigned int)getpid();

        sprintf(template + templateLen - 6, "%.6X", rand_r(&seed) & 0xFFFFFF);

        if(mkdir(template, 0700) < 0)
        {
            if(errno == EEXIST || errno == ELOOP || errno == ENOENT || errno == ENOTDIR)
                continue;
            return NULL;
        }
        return template;
    }
    return NULL;
}
#endif /* ! HAVE_MKDTEMP */
