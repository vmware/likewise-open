/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Listener structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __SMB_CONNECTION
{
    int fd;
    struct sockaddr_in cliaddr;

} SMB_CONNECTION, *PSMB_CONNECTION;

#endif /* __STRUCTS_H__ */
