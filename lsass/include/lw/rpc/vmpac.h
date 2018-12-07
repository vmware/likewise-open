/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        vmpac.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        VMware PAC library
 */

#ifndef _RPC_VMPAC_H_H
#define _RPC_VMPAC_H_H

typedef struct _VMDIR_GROUP_MEMBERSHIP
{
    ULONG Identifier[2];
    ULONG Attributes;
} VMDIR_GROUP_MEMBERSHIP, *PVMDIR_GROUP_MEMBERSHIP;

typedef struct _VMPAC_ACCESS_INFO {
    UNICODE_STRING account_name;
    PSID user_sid;
    UNICODE_STRING domain;
    PSID domain_sid;
    ULONG group_count;
#ifdef _DCE_IDL_
    [size_is(group_count)]
#endif
    PVMDIR_GROUP_MEMBERSHIP groups;
    UINT32 sidcount;
#ifdef _DCE_IDL_
    [size_is(sidcount)]
#endif
    NetrSidAttr *sids;
} VMPAC_ACCESS_INFO, *PVMPAC_ACCESS_INFO;

#ifndef _DCE_IDL_

NTSTATUS
DecodeVmPacAccessInfo(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT VMPAC_ACCESS_INFO** ppAccessInfo
    );

NTSTATUS
EncodeVmPacAccessInfo(
    IN VMPAC_ACCESS_INFO* pAccessInfo,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    );

VOID
FreeVmPacAccessInfo(
    VMPAC_ACCESS_INFO *pAccessInfo
    );

#endif /* _DCE_IDL_ */

#endif /* _RPC_VMPAC_H_H */
