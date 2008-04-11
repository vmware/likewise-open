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
#include "djdaemonmgr.h"
#include "ctstrutils.h"
#include "djauthinfo.h"

// aka: CENTERROR_LICENSE_INCORRECT
static DWORD GPAGENT_LICENSE_ERROR = 0x00002001;

// CENTERROR_LICENSE_EXPIRED
static DWORD GPAGENT_LICENSE_EXPIRED_ERROR = 0x00002002;

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

static QueryResult QueryStopDaemons(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN running;
    QueryResult result = FullyConfigured;
    LWException *inner = NULL;

    /* Check for lwiauthd and likewise-open */

    DJGetDaemonStatus("centeris.com-lwiauthd", &running, &inner);
    if (!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        LW_HANDLE(&inner);
        DJGetDaemonStatus("likewise-open", &running, &inner);
    }
    LW_CLEANUP(exc, inner);

    if(running)
        result = NotConfigured;

    DJGetDaemonStatus("centeris.com-gpagentd", &running, &inner);
    if (!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        /* The gpagentd may not be installed so ignore */
        goto cleanup;
    }
    LW_CLEANUP(exc, inner);

    if(running)
        result = NotConfigured;

cleanup:
    LW_HANDLE(&inner);
    return result;
}

static void StopDaemons(JoinProcessOptions *options, LWException **exc)
{
    LW_TRY(exc, DJManageDaemons(options->shortDomainName, FALSE, &LW_EXC));
cleanup:
    ;
}

static PSTR GetStopDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    LW_CLEANUP_CTERR(exc, CTStrdup("Run '/etc/init.d/centeris.com-gpagentd stop'\nRun '/etc/init.d/likewise-open stop'\n", &ret));
cleanup:
    return ret;
}

const JoinModule DJDaemonStopModule = { TRUE, "stop", "stop daemons", QueryStopDaemons, StopDaemons, GetStopDescription };

static QueryResult QueryStartDaemons(const JoinProcessOptions *options, LWException **exc)
{
    BOOLEAN running;
    QueryResult result = FullyConfigured;
    const ModuleState *stopState = DJGetModuleStateByName((JoinProcessOptions *)options, "stop");
    CENTERROR ceError = CENTERROR_SUCCESS;
    LWException *inner = NULL;

    if(!options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    /* Check for lwiauthd and likewise-open */

    DJGetDaemonStatus("centeris.com-lwiauthd", &running, &inner);
    if (!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        LW_HANDLE(&inner);
        DJGetDaemonStatus("likewise-open", &running, &inner);
    }
    LW_CLEANUP(exc, inner);

    if(!running)
        result = NotConfigured;
    
    DJGetDaemonStatus("centeris.com-gpagentd", &running, &inner);
    if (!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        /* The gpagentd may not be installed so ignore */
        LW_HANDLE(&inner);
        running = TRUE;
    }
    LW_CLEANUP(exc, inner);

    LW_CLEANUP_CTERR(exc, ceError);
    if(!running)
        result = NotConfigured;

    if(stopState != NULL && stopState->runModule)
        result = NotConfigured;

cleanup:
    LW_HANDLE(&inner);
    return result;
}

static void StartDaemons(JoinProcessOptions *options, LWException **exc)
{
    LW_CLEANUP_CTERR(exc, DJRemoveCacheFiles());

    LW_TRY(exc, DJManageDaemons(options->shortDomainName, TRUE, &LW_EXC));
cleanup:
    ;
}

static PSTR GetStartDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    LW_CLEANUP_CTERR(exc, CTStrdup(
"rm /var/lib/lwidentity/*_cache.tdb\n"
"Run '/etc/init.d/centeris.com-gpagentd start'\n"
"Run '/etc/init.d/likewise-open start'\n"
"Configure daemons to start automatically on reboot\n"
        , &ret));
cleanup:
    return ret;
}

const JoinModule DJDaemonStartModule = { TRUE, "start", "start daemons", QueryStartDaemons, StartDaemons, GetStartDescription };

void DJRestartIfRunning(PCSTR daemon, LWException **exc)
{
    BOOLEAN running;
    LWException *inner = NULL;

    DJGetDaemonStatus(daemon, &running, &inner);
    if(!LW_IS_OK(inner) && inner->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        //The daemon isn't installed
        LW_HANDLE(&inner);
        running = FALSE;
    }
    LW_CLEANUP(exc, inner);
    if(!running)
        goto cleanup;

    DJ_LOG_INFO("Restarting '%s'", daemon);
    LW_TRY(exc, DJStartStopDaemon(daemon, FALSE, NULL, &LW_EXC));
    DJ_LOG_INFO("Starting '%s'", daemon);
    LW_TRY(exc, DJStartStopDaemon(daemon, TRUE, NULL, &LW_EXC));

cleanup:
    LW_HANDLE(&inner);
}

void
DJManageDaemons(
    PSTR pszDomainName,
    BOOLEAN bStart,
    LWException **exc
    )
{
    CHAR szPreCommand[512];
    PSTR pszDomainNameAllUpper = NULL;
    BOOLEAN bFileExists = TRUE;
    FILE* fp = NULL;
    PSTR pszErrFilePath = "/var/cache/centeris/grouppolicy/gpagentd.err";
    CHAR szBuf[256+1];
    DWORD dwGPErrCode = 0;
    LWException *innerExc = NULL;
    int daemonCount;
    int i;

#define PWGRD "/etc/rc.config.d/pwgr"
    LW_CLEANUP_CTERR(exc, CTCheckFileExists(PWGRD, &bFileExists));
    if(bFileExists)
    {
        //Shutdown pwgr (a nscd-like daemon) on HP-UX because it only handles
        //usernames up to 8 characters in length.
        LW_TRY(exc, DJStartStopDaemon("pwgr", FALSE, NULL, &LW_EXC));
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(PWGRD, PWGRD, FALSE, "s/=1/=0/"));
    }

    //Figure out how many daemons there are
    for(daemonCount = 0; daemonList[daemonCount].primaryName != NULL; daemonCount++);

    if(bStart)
    {
        CHAR szStartPriority[32];
        CHAR szStopPriority[32];

        //Start the daemons in ascending order
        for(i = 0; i < daemonCount; i++)
        {
            sprintf(szStartPriority, "%d", daemonList[i].startPriority);
            sprintf(szStopPriority,  "%d", daemonList[i].stopPriority);
 
            DJManageDaemon(daemonList[i].primaryName,
                             bStart,
                             NULL,
                             szStartPriority,
                             szStopPriority,
                             &innerExc);

            //Try the alternate daemon name if there is one
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    daemonList[i].alternativeName != NULL)
            {
                LW_HANDLE(&innerExc);
                DJManageDaemon(daemonList[i].alternativeName,
                                 bStart,
                                 NULL,
                                 szStartPriority,
                                 szStopPriority,
                                 &innerExc);
            }
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    !daemonList[i].required)
            {
                LW_HANDLE(&innerExc);
            }
            if (LW_IS_OK(innerExc) && !strcmp(daemonList[i].primaryName, "centeris.com-gpagentd"))
            {
                LW_CLEANUP_CTERR(exc, CTCheckFileExists(pszErrFilePath, &bFileExists));

                if (bFileExists) {

                    LW_HANDLE(&innerExc);
                    fp = fopen(pszErrFilePath, "r");
                    if (fp != NULL) {

                        if (fgets(szBuf, 256, fp) != NULL) {

                            CTStripWhitespace(szBuf);

                            dwGPErrCode = atoi(szBuf);

                            if (dwGPErrCode == GPAGENT_LICENSE_ERROR ||
                                dwGPErrCode == GPAGENT_LICENSE_EXPIRED_ERROR) {

                                LW_RAISE(exc, CENTERROR_DOMAINJOIN_LICENSE_ERROR);
                                goto cleanup;

                            }
                        }

                    } else {

                        DJ_LOG_ERROR("Failed to open file [%s]", pszErrFilePath);

                    }
                }
            }
            LW_CLEANUP(exc, innerExc);
        }
    }
    else
    {
        CHAR szStartPriority[32];
        CHAR szStopPriority[32];

        //Stop the daemons in descending order
        for(i = daemonCount - 1; i >= 0; i--)
        {
            sprintf(szStartPriority, "%d", daemonList[i].startPriority);
            sprintf(szStopPriority,  "%d", daemonList[i].stopPriority);

            DJManageDaemon(daemonList[i].primaryName,
                             bStart,
                             NULL,
                             szStartPriority,
                             szStopPriority,
                             &innerExc);

            //Try the alternate daemon name if there is one
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    daemonList[i].alternativeName != NULL)
            {
                LW_HANDLE(&innerExc);
                DJManageDaemon(daemonList[i].alternativeName,
                                 bStart,
                                 NULL,
                                 szStartPriority,
                                 szStopPriority,
                                 &innerExc);
            }
            if (!LW_IS_OK(innerExc) &&
                    innerExc->code == CENTERROR_DOMAINJOIN_MISSING_DAEMON &&
                    !daemonList[i].required)
            {
                LW_HANDLE(&innerExc);
            }
            LW_CLEANUP(exc, innerExc);
        }
    }

cleanup:
    CTSafeCloseFile(&fp);

    if (pszDomainNameAllUpper)
        CTFreeString(pszDomainNameAllUpper);

    LW_HANDLE(&innerExc);
}
