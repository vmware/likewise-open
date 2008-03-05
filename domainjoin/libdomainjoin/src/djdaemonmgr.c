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
    PSTR initPath = NULL;
    PSTR daemonPath = NULL;
    CENTERROR ceError;

    LW_CLEANUP_CTERR(exc, CTFindFileInPath(daemon, "/etc/init.d:/etc/rc.d/init.d", &initPath));
    DJ_LOG_INFO("Found '%s' at '%s'", daemon, initPath);

    LW_TRY(exc, DJGetDaemonStatus(initPath, &running, &LW_EXC));
    if(!running)
    {
        //The nscd init script on Solaris does not support the query option,
        //so it looks like the daemon is never running. So we'll run a ps
        //command and HUP the daemon if it is running

        pid_t daemonPid;
        LW_CLEANUP_CTERR(exc, CTFindFileInPath(daemon, "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", &daemonPath));
        ceError = CTGetPidOfCmdLine(NULL, daemonPath, NULL, 0, &daemonPid, NULL);
        if(ceError == CENTERROR_NO_SUCH_PROCESS || ceError == CENTERROR_NOT_IMPLEMENTED)
        {
            //Nope, couldn't find the daemon running
            goto cleanup;
        }
        LW_CLEANUP_CTERR(exc, ceError);

        DJ_LOG_INFO("Sending HUP to '%s' binary, pid '%d'.", daemonPath, daemonPid);

        LW_CLEANUP_CTERR(exc, CTSendSignal(daemonPid, SIGHUP));
        goto cleanup;
    }

    DJ_LOG_INFO("Restarting '%s'", initPath);
    LW_TRY(exc, DJStartStopDaemon(initPath, FALSE, NULL, &LW_EXC));
    LW_TRY(exc, DJStartStopDaemon(initPath, TRUE, NULL, &LW_EXC));

cleanup:
    CT_SAFE_FREE_STRING(initPath);
    CT_SAFE_FREE_STRING(daemonPath);
}
