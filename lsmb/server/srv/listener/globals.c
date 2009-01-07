/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Listener Globals
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


pthread_mutex_t gListenerLock = PTHREAD_MUTEX_INITIALIZER;
BOOLEAN         gbStopListener = FALSE;
pthread_t       gListenerThread;
PVOID           gpListenerThread = NULL;

PSTR gpszSrvProviderName = "cifs server";

NTVFS_DRIVER gSrvProviderTable =
{
        &SrvCreateFileEx,
        &SrvReadFileEx,
        &SrvWriteFileEx,
        &SrvGetSessionKey,
        &SrvCloseFileEx,
        &SrvTreeConnect,
        &SrvNTCreate,
        &SrvNTTransactCreate,
        &SrvCreateTemporary,
        &SrvReadFile,
        &SrvWriteFile,
        &SrvLockFile,
        &SrvSeekFile,
        &SrvFlushFile,
        &SrvCloseFile,
        &SrvCloseFileAndDisconnect,
        &SrvDeleteFile,
        &SrvRenameFile,
        &SrvCopyFile,
        &SrvTrans2QueryFileInformation,
        &SrvTrans2SetPathInformation,
        &SrvTrans2QueryPathInformation,
        &SrvTrans2CreateDirectory,
        &SrvTrans2DeleteDirectory,
        &SrvTrans2CheckDirectory,
        &SrvTrans2FindFirst2,
        &SrvTrans2FindNext2,
        &SrvNTTransactNotifyChange,
        &SrvTrans2GetDFSReferral
};
