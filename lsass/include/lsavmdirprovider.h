/*
 * Copyright (C) 2015 VMware, Inc. All rights reserved.
 */
#ifndef __LSAVMDIRPROVIDER_H__
#define __LSAVMDIRPROVIDER_H__

#include "lsautils.h"
#include <lsa/vmdir-types.h>

#define LSA_VMDIR_IO_SIGNAL                 1

typedef struct __LSA_VMDIR_IPC_SIGNAL_REQ
{
    DWORD dwFlags;
} LSA_VMDIR_IPC_SIGNAL_REQ, *PLSA_VMDIR_IPC_SIGNAL_REQ;

LWMsgTypeSpec*
LsaVmdirIPCGetStringSpec(
    VOID
    );

LWMsgTypeSpec*
LsaVmdirIPCGetSignalReqSpec(
    void
    );

VOID
LsaVmdirIPCSetMemoryFunctions(
    IN LWMsgContext* pContext
    );

#endif /* __LSAVMDIRPROVIDER_H__ */
