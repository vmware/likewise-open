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

#include "domainjoin.h"
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

CENTERROR GetServerTime(
    PSTR pszDCName, time_t *result)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR netOutput = NULL;
    PSTR netCommand = NULL;
    PSTR escapedDC = NULL;
    PSTR intParseEnd;

    GCE(ceError = CTEscapeString(pszDCName, &escapedDC));

    GCE(ceError = CTAllocateStringPrintf(&netCommand,
        "%s/bin/lwinet time seconds -S %s", PREFIXDIR, escapedDC));
    ceError = CTCaptureOutput(netCommand, &netOutput);
    if(!CENTERROR_IS_OK(ceError))
        GCE(ceError = CENTERROR_DOMAINJOIN_LWINET_TIME_FAILED);

    *result = strtoul(netOutput, &intParseEnd, 10);

    if(*intParseEnd != '\0' && !isspace((char)*intParseEnd))
    {
        DJ_LOG_ERROR("Unable to parse lwinet time output '%s'", netOutput);
        GCE(ceError = CENTERROR_DOMAINJOIN_LWINET_TIME_FAILED);
    }

cleanup:
    CT_SAFE_FREE_STRING(netOutput);
    CT_SAFE_FREE_STRING(netCommand);
    CT_SAFE_FREE_STRING(escapedDC);
    return ceError;
}

CENTERROR
DJSetTime(time_t timesec)
{
    BOOLEAN timeset = FALSE;
#if !defined(HAVE_CLOCK_SETTIME) && !defined(HAVE_SETTIMEOFDAY)
#error Either clock_settime or settimeofday is needed
#endif

#ifdef HAVE_CLOCK_SETTIME
    struct timespec systemspec;
#endif
#if HAVE_SETTIMEOFDAY
    struct timeval systemval;
#endif
    long long readTime = -1;

#ifdef HAVE_CLOCK_SETTIME
    memset(&systemspec, 0, sizeof(systemspec));
    systemspec.tv_sec = timesec;
#endif
#if HAVE_SETTIMEOFDAY
    memset(&systemval, 0, sizeof(systemval));
    systemval.tv_sec = timesec;
#endif

#ifdef HAVE_CLOCK_SETTIME
    if(!timeset)
    {
        if(clock_settime(CLOCK_REALTIME, &systemspec) == -1)
        {
            DJ_LOG_INFO("Setting time with clock_settime failed %d", errno);
        }
        else
        {
            DJ_LOG_INFO("Setting time with clock_settime worked");
            timeset = TRUE;
        }
    }
#endif
#ifdef HAVE_SETTIMEOFDAY
    if(!timeset)
    {
        if(settimeofday(&systemval, NULL) == -1)
        {
            DJ_LOG_INFO("Setting time with settimeofday failed %d", errno);
        }
        else
        {
            DJ_LOG_INFO("Setting time with settimeofday worked");
            timeset = TRUE;
        }
    }
#endif
    if(!timeset)
    {
        DJ_LOG_ERROR("Couldn't find a method to set the time with");
        return CENTERROR_DOMAINJOIN_TIME_NOT_SET;
    }

    //Verify the clock got set
    timeset = FALSE;
#ifdef HAVE_CLOCK_GETTIME
    if(!timeset && clock_gettime(CLOCK_REALTIME, &systemspec) >= 0)
    {
        timeset = TRUE;
        readTime = systemspec.tv_sec;
    }
#endif
#ifdef HAVE_GETTIMEOFDAY
    if(!timeset && gettimeofday(&systemval, NULL) >= 0)
    {
        timeset = TRUE;
        readTime = systemval.tv_sec;
    }
#endif
    if(!timeset)
        return CTMapSystemError(errno);

    //Make sure the time is now within 5 seconds of what we set
    if(labs(readTime - timesec) > 5)
    {
        DJ_LOG_ERROR("Attempted to set time to %ld, but it is now %ld.", timesec, readTime);
        return CENTERROR_DOMAINJOIN_TIME_NOT_SET;
    }

    return CENTERROR_SUCCESS;
}

CENTERROR
DJSyncTimeToDC(
    PSTR pszDCName,
    int allowedDrift
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    time_t serverTime;
    time_t localTime;

    GCE(ceError = GetServerTime(pszDCName, &serverTime));
    if(time(&localTime) == (time_t)-1)
    {
        GCE(ceError = CTMapSystemError(errno));
    }
    DJ_LOG_INFO("Server time is %ld. Local time is %ld.", serverTime, localTime);
    if(labs(serverTime - localTime) > labs(allowedDrift))
    {
        //Got to sync the time
        GCE(ceError = DJSetTime(serverTime));
    }

cleanup:
    return ceError;
}
