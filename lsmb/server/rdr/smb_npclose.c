/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        open.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB Client Close Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

uint32_t
NPClose(
    SMB_TREE *pTree,
    uint16_t  fid)
{
    return 0;
}
