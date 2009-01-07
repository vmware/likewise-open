/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        client.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        API (Client)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "config.h"
#include "lsmbsys.h"

#include <krb5.h>
#include <lsmb/lsmb.h>

#include <lwmsg/lwmsg.h>
#include <lwmsg/protocol.h>

#include <smbdef.h>
#include <smbutils.h>
#include <smbipc.h>

#include "structs.h"

#include "externs.h"


