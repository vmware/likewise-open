/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        crypt.h
 *
 * Abstract:
 *
 *        lsass crypto helper functions.
 *
 * Author: Todd Stecher (2007)
 *
 */

#ifndef _CRYPT_H_
#define _CRYPT_H_

DWORD
ComputeNTOWF(
    PLSA_STRING password,
    UCHAR owf[16]
    );

DWORD
ComputeLMOWF(
    PLSA_STRING password,
    UCHAR owf[16]
    );


#endif
