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
    CENTERROR ceError = CENTERROR_SUCCESS;

    /* Check for lwiauthd and likewise-open */

    ceError =  DJGetDaemonStatus("centeris.com-lwiauthd", &running);
    if (ceError == CENTERROR_DOMAINJOIN_MISSING_DAEMON) {
        ceError =  DJGetDaemonStatus("likewise-open", &running);
    }

    LW_CLEANUP_CTERR(exc, ceError);
    if(running)
        result = NotConfigured;

    ceError = DJGetDaemonStatus("centeris.com-gpagentd", &running);
    if (ceError == CENTERROR_DOMAINJOIN_MISSING_DAEMON) {
        /* The gpagentd may not be installed so ignore */
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, ceError);
    if(running)
        result = NotConfigured;

cleanup:
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
    LW_CLEANUP_CTERR(exc, CTStrdup("Run '/etc/init.d/centeris.com-gpagentd stop'\nRun '/etc/init.d/centeris.com-lwiauthd stop'\n", &ret));
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

    if(!options->joiningDomain)
    {
        result = NotApplicable;
        goto cleanup;
    }

    /* Check for lwiauthd and likewise-open */

    ceError =  DJGetDaemonStatus("centeris.com-lwiauthd", &running);
    if (ceError == CENTERROR_DOMAINJOIN_MISSING_DAEMON) {
        ceError =  DJGetDaemonStatus("likewise-open", &running);
    }

    LW_CLEANUP_CTERR(exc, ceError);
    if(!running)
        result = NotConfigured;
    
    ceError = DJGetDaemonStatus("centeris.com-gpagentd", &running);
    if (ceError == CENTERROR_DOMAINJOIN_MISSING_DAEMON) {
        /* The gpagentd may not be installed so ignore */
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, ceError);
    if(!running)
        result = NotConfigured;

    if(stopState != NULL && stopState->runModule)
        result = NotConfigured;

cleanup:
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
"Run '/etc/init.d/centeris.com-lwiauthd start'\n"
"Configure daemons to start automatically on reboot\n"
        , &ret));
cleanup:
    return ret;
}

const JoinModule DJDaemonStartModule = { TRUE, "start", "start daemons", QueryStartDaemons, StartDaemons, GetStartDescription };

void DJRestartIfRunning(PCSTR daemon, LWException **exc)
{
    BOOLEAN running;
    PSTR daemonPath = NULL;

    LW_CLEANUP_CTERR(exc, CTFindFileInPath(daemon, "/etc/init.d:/etc/rc.d/init.d", &daemonPath));

    LW_CLEANUP_CTERR(exc, DJGetDaemonStatus(daemonPath, &running));
    if(!running)
        goto cleanup;

    LW_TRY(exc, DJStartStopDaemon(daemonPath, FALSE, NULL, &LW_EXC));
    LW_TRY(exc, DJStartStopDaemon(daemonPath, TRUE, NULL, &LW_EXC));

cleanup:
    CT_SAFE_FREE_STRING(daemonPath);
}
