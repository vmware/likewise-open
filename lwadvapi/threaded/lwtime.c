/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwtime.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *
 *        Time Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

//Convert to seconds string of form ##s, ##m, ##h, or ##d
//where s,m,h,d = seconds, minutes, hours, days.
DWORD
LwParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    )
{
    DWORD  dwError = 0;
    DWORD  dwTimeInterval = 0;
    PSTR   pszTimeIntervalLocal = 0;
    DWORD  dwTimeIntervalLocalLen = 0;
    DWORD  dwUnitMultiplier = 0;
    PSTR   pszUnitCode = NULL;

    LwStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);

    LW_BAIL_ON_INVALID_STRING(pszTimeInterval);

    dwError = LwAllocateString(
                    pszTimeInterval,
                    &pszTimeIntervalLocal
                    );
    BAIL_ON_LW_ERROR(dwError);

    dwTimeIntervalLocalLen = strlen(pszTimeIntervalLocal);

    pszUnitCode = pszTimeIntervalLocal + dwTimeIntervalLocalLen - 1;

    if (isdigit((int)(*pszUnitCode)))
    {
        dwUnitMultiplier = 1;
    }

    else
    {

        switch(*pszUnitCode)
        {
            case 's':
            case 'S':
	        dwUnitMultiplier = 1;
	        break;

            case 'm':
            case 'M':
	        dwUnitMultiplier = LW_SECONDS_IN_MINUTE;
	        break;

            case 'h':
            case 'H':
                dwUnitMultiplier = LW_SECONDS_IN_HOUR;
                break;

            case 'd':
            case 'D':
                dwUnitMultiplier = LW_SECONDS_IN_DAY;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LW_ERROR(dwError);
                break;
        }

        *pszUnitCode = ' ';
    }

    LwStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);

    dwTimeInterval = (DWORD) atoi(pszTimeIntervalLocal) * dwUnitMultiplier;

    *pdwTimeInterval = dwTimeInterval;

cleanup:

    LW_SAFE_FREE_STRING(pszTimeIntervalLocal);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSetSystemTime(
    time_t ttCurTime
    )
{
    DWORD dwError = 0;
    BOOLEAN bTimeset = FALSE;
    DWORD dwCount = 0;

    // The aix implementation of clock_settime segfaults
#ifdef __LWI_AIX__
#undef HAVE_CLOCK_SETTIME
#endif

#if !defined(HAVE_CLOCK_SETTIME) && !defined(HAVE_SETTIMEOFDAY)
#error Either clock_settime or settimeofday is needed
#endif

#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_CLOCK_SETTIME)
    struct timespec systemspec;
#endif
#if HAVE_SETTIMEOFDAY || HAVE_GETTIMEOFDAY
    struct timeval systemval;
#endif
    long long readTime = -1;

#ifdef HAVE_CLOCK_SETTIME
    memset(&systemspec, 0, sizeof(systemspec));
    systemspec.tv_sec = ttCurTime;
#endif
#if HAVE_SETTIMEOFDAY
    memset(&systemval, 0, sizeof(systemval));
    systemval.tv_sec = ttCurTime;
#endif

#ifdef HAVE_CLOCK_SETTIME
    if (!bTimeset)
    {
        if (clock_settime(CLOCK_REALTIME, &systemspec) == -1)
        {
            LW_LOG_VERBOSE("Setting time with clock_settime failed %d", errno);
        }
        else
        {
            LW_LOG_VERBOSE("Setting time with clock_settime worked");
            bTimeset = TRUE;
        }
    }
#endif

#ifdef HAVE_SETTIMEOFDAY
    if (!bTimeset)
    {
        if (settimeofday(&systemval, NULL) == -1)
        {
            LW_LOG_VERBOSE("Setting time with settimeofday failed %d", errno);
        }
        else
        {
            LW_LOG_VERBOSE("Setting time with settimeofday worked");
            bTimeset = TRUE;
        }
    }
#endif

    if (!bTimeset)
    {
        dwError = LW_ERROR_FAILED_TO_SET_TIME;
        BAIL_ON_LW_ERROR(dwError);
    }

    //Verify the clock got set
    bTimeset = FALSE;
#ifdef HAVE_CLOCK_GETTIME
    if (!bTimeset && clock_gettime(CLOCK_REALTIME, &systemspec) >= 0)
    {
        bTimeset = TRUE;
        readTime = systemspec.tv_sec;
    }
#endif

#ifdef HAVE_GETTIMEOFDAY
    if (!bTimeset && gettimeofday(&systemval, NULL) >= 0)
    {
        bTimeset = TRUE;
        readTime = systemval.tv_sec;
    }
#endif

    if (!bTimeset) {
        dwError = LW_ERROR_FAILED_TO_SET_TIME;
        BAIL_ON_LW_ERROR(dwError);
    }

    //Make sure the time is now within 5 seconds of what we set
    if (labs(readTime - ttCurTime) > 5)
    {
        LW_LOG_ERROR("Attempted to set time to %ld, but it is now %lld.", ttCurTime, readTime);
        dwError = LW_ERROR_FAILED_TO_SET_TIME;
        BAIL_ON_LW_ERROR(dwError);
    }

    //Make sure the time reported by time() is now within 5 seconds of
    //what we set.  On virtual systems it may be slow to update.
    for ( dwCount = 0 ; dwCount < 5 ; dwCount++ )
    {
        readTime = time(NULL);

        if (labs(readTime - ttCurTime) > 5)
        {
            LW_LOG_DEBUG("Time is slow to update...waiting");
            sleep(1);
        }
        else
        {
            break;
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

// This function is just a wrapper to getting the current time as a time_t.
DWORD
LwGetCurrentTimeSeconds(
    OUT time_t* pTime
    )
{
    DWORD dwError = 0;
    struct timeval current_tv;

    // ISSUE-2008/10/30-dalmeida -- Is gettimeofday() any better worse than time()?
    // Preserve gettimeofday() since the code we are replacing currently uses that.
    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    *pTime = current_tv.tv_sec;

cleanup:
    return dwError;

error:
    *pTime = 0;
    goto cleanup;
}


DWORD
LwGetNtTime(
    PULONG64 pullTime
    )
{
    DWORD dwError = ERROR_SUCCESS;
    ULONG64 ullTime = 0;
    time_t t = 0;

    dwError = LwGetCurrentTimeSeconds(&t);
    BAIL_ON_LW_ERROR(dwError);

    ullTime = LwWinTimeToNtTime((DWORD)t);

    *pullTime = ullTime;

cleanup:
    return dwError;

error:
    *pullTime = 0;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
