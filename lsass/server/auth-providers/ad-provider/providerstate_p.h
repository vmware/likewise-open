/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        providerstate_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        AD auth provider state (Private Header)
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 */
#ifndef __PROVIDERSTATE_P_H__
#define __PROVIDERSTATE_P_H__

VOID
ADProviderFreeCellInfoNode(
    IN PVOID pData,
    IN PVOID pUserData
    );

#endif /* __PROVIDERSTATE_P_H__ */
