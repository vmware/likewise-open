/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsaaccess.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LSAACCESS_H__
#define __LSAACCESS_H__

#include "lsautils.h"

DWORD
LsaAccessGetData(
    PCSTR * ppczConfigData,
    PVOID * ppAccessData
    );

DWORD
LsaAccessCheckData(
    PCSTR pczUserName,
    PCVOID pAccessData
    );

DWORD
LsaAccessFreeData(
    PVOID pAccessData
    );

#endif /* __LSAACCESS_H__ */
