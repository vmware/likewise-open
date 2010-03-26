/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwsessioninfo.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        SMB session library definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *          Sriram Nambakam  (snambakam@likewise.com)
 */

#ifndef _LWSESSIONINFO_H_
#define _LWSESSIONINFO_H_

typedef union _SESSION_INFO_UNION
{
    PSESSION_INFO_0   p0;
    PSESSION_INFO_1   p1;
    PSESSION_INFO_2   p2;
    PSESSION_INFO_10  p10;
    PSESSION_INFO_502 p502;

} SESSION_INFO_UNION, *PSESSION_INFO_UNION;

typedef struct _SESSION_INFO_ENUM_PARAMS
{
    DWORD              dwInfoLevel;
    DWORD              dwNumEntries;
    SESSION_INFO_UNION info;
} SESSION_INFO_ENUM_PARAMS, *PSESSION_INFO_ENUM_PARAMS;

typedef struct _SESSION_INFO_DELETE_PARAMS
{
    PWSTR pwszServername;
    PWSTR pwszUncClientname;
    PWSTR pwszUncUsername;
} SESSION_INFO_DELETE_PARAMS, *PSESSION_INFO_DELETE_PARAMS;

LW_NTSTATUS
LwSessionInfoMarshalEnumParameters(
    PSESSION_INFO_ENUM_PARAMS pParams,
    PBYTE*                    ppBuffer,
    ULONG*                    pulBufferSize
    );

LW_NTSTATUS
LwSessionInfoUnmarshalEnumParameters(
    PBYTE                      pBuffer,
    ULONG                      ulBufferSize,
    PSESSION_INFO_ENUM_PARAMS* ppParams
    );

LW_NTSTATUS
LwSessionInfoMarshalDeleteParameters(
    PSESSION_INFO_DELETE_PARAMS pParams,
    PBYTE*                      ppBuffer,
    ULONG*                      pulBufferSize
    );

LW_NTSTATUS
LwSessionInfoUnmarshalDeleteParameters(
    PBYTE                        pBuffer,
    ULONG                        ulBufferSize,
    PSESSION_INFO_DELETE_PARAMS* ppParams
    );

#endif /* _LWSESSIONINFO_H_ */

