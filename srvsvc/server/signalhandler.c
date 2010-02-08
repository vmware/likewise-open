/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (SRVSVC)
 *
 * Server Main
 *
 * Signal Handling
 *
 */

#include "includes.h"

static
PVOID
SrvSvcHandleSignals(
    PVOID pArg
    );

static
VOID
SrvSvcInterruptHandler(
    int Signal
    );

VOID
SrvSvcBlockSelectedSignals(
    VOID
    )
{
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    sigaddset(&default_signal_mask, SIGINT);
    sigaddset(&default_signal_mask, SIGTERM);
    sigaddset(&default_signal_mask, SIGHUP);
    sigaddset(&default_signal_mask, SIGQUIT);
    sigaddset(&default_signal_mask, SIGPIPE);

    pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
}

/*
 * Set up the process environment to properly deal with signals.
 * By default, we isolate all threads from receiving asynchronous
 * signals. We create a thread that handles all async signals.
 * The signal handling actions are handled in the handler thread.
 *
 * For AIX, we cant use a thread that sigwaits() on a specific signal,
 * we use a plain old, lame old Unix signal handler.
 *
 */
DWORD
SrvSvcStartSignalHandler(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = pthread_create(&gServerInfo.signalHandlerThread,
                             NULL,
                             &SrvSvcHandleSignals,
                             NULL);
    BAIL_ON_SRVSVC_ERROR(dwError);

    gServerInfo.pSignalHandlerThread = &gServerInfo.signalHandlerThread;

error:

    return dwError;
}

DWORD
SrvSvcStopSignalHandler(
    VOID
    )
{
    DWORD dwError = 0;
    UINT32 status = 0;
    BOOLEAN bInLock = FALSE;

    rpc_mgmt_stop_server_listening(NULL, (unsigned32*)&status);

    SRVSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    if (gServerInfo.pSignalHandlerThread)
    {
        pthread_kill(gServerInfo.signalHandlerThread, SIGTERM);
        pthread_join(gServerInfo.signalHandlerThread, NULL);
        gServerInfo.pSignalHandlerThread = NULL;
    }

    SRVSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return (dwError);
}

static
PVOID
SrvSvcHandleSignals(
    PVOID pArg
    )
{
    DWORD dwError = 0;
    struct sigaction action;
    sigset_t catch_signal_mask;
    sigset_t old_signal_mask;
    int which_signal = 0;
    int sysRet = 0;
    unsigned32 status = 0;

    // After starting up threads, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = SrvSvcInterruptHandler;

    sysRet = sigaction(SIGINT, &action, NULL);
    dwError = (sysRet != 0) ? errno : 0;
    BAIL_ON_SRVSVC_ERROR(dwError);

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    dwError = pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL);
    BAIL_ON_SRVSVC_ERROR(dwError);

    // These should already be blocked...
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGTERM);
    sigaddset(&catch_signal_mask, SIGQUIT);
    sigaddset(&catch_signal_mask, SIGHUP);
    sigaddset(&catch_signal_mask, SIGPIPE);

    dwError = pthread_sigmask(SIG_BLOCK, &catch_signal_mask, &old_signal_mask);
    BAIL_ON_SRVSVC_ERROR(dwError);

    while (1)
    {
        /* Wait for a signal to arrive */
        sigwait(&catch_signal_mask, &which_signal);

        switch (which_signal)
        {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
            {
                rpc_mgmt_stop_server_listening(NULL, &status);
                SrvSvcSetProcessShouldExit(TRUE);

                goto error;
            }

            case SIGPIPE:
            {
                SRVSVC_LOG_DEBUG("Handled SIGPIPE");

                break;
            }
            case SIGHUP:
            {
                dwError = SrvSvcReadConfigSettings();
                BAIL_ON_SRVSVC_ERROR(dwError);

                break;
            }
        }
    }

error:
    return NULL;
}

static
VOID
SrvSvcInterruptHandler(
    int Signal
    )
{
    if (Signal == SIGINT)
    {
        raise(SIGTERM);
    }
}
