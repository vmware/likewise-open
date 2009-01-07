/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        servermain.h
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __SERVERMAIN_H__
#define __SERVERMAIN_H__

int
main(
    int argc,
    char* argv[]
    );

BOOLEAN
SMBSrvShouldProcessExit();

VOID
SMBSrvSetProcessToExit(
    BOOLEAN bExit
    );

#endif /* __SERVERMAIN_H__ */

