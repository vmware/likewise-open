/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        includes.h
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 *        Internal Includes
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include "config.h"
#include "lwiosys.h"

#include <lw/rtlstring.h>
#include <lw/rtlgoto.h>

#include "iodriver.h"

#include "ntlogmacros.h"
#include "lwioutils.h"

#define IOTEST_DEVICE_NAME "iotest"
#define IOTEST_DEVICE_PATH "/" IOTEST_DEVICE_NAME

#define IOTEST_INTERNAL_PATH_ALLOW "/allow"
#define IOTEST_PATH_ALLOW IOTEST_DEVICE_PATH IOTEST_INTERNAL_PATH_ALLOW

typedef struct _IT_CCB {
    UNICODE_STRING Path;
} IT_CCB, *PIT_CCB;

NTSTATUS
ItpCreateCcb(
    OUT PIT_CCB* ppCcb,
    IN PUNICODE_STRING pPath
    );

VOID
ItpDestroyCcb(
    IN OUT PIT_CCB* ppCcb
    );

NTSTATUS
ItDispatchCreate(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchClose(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchRead(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchWrite(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchDeviceIoControl(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchFsControl(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchFlushBuffers(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchQueryInformation(
    IN PIRP pIrp
    );

NTSTATUS
ItDispatchSetInformation(
    IN PIRP pIrp
    );

#endif /* __INCLUDES_H__ */
