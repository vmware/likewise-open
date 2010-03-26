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
 *        lmfileinfo.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        SMB file library definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *          Sriram Nambakam  (snambakam@likewise.com)
 */

#ifndef _LWFILEINFO_H_
#define _LWFILEINFO_H_

typedef union _FILE_INFO_UNION
{
    PFILE_INFO_2  p2;
    PFILE_INFO_3  p3;

} FILE_INFO_UNION, *PFILE_INFO_UNION;

typedef struct _FILE_INFO_ENUM_PARAMS
{
    PWSTR           pwszBasepath;
    PWSTR           pwszUsername;
    DWORD           dwInfoLevel;
    DWORD           dwNumEntries;
    FILE_INFO_UNION info;
} FILE_INFO_ENUM_PARAMS, *PFILE_INFO_ENUM_PARAMS;

typedef struct _FILE_INFO_GET_INFO_PARAMS
{
    DWORD           dwInfoLevel;
    DWORD           dwFileId;
    FILE_INFO_UNION info;
} FILE_INFO_GET_INFO_PARAMS, *PFILE_INFO_GET_INFO_PARAMS;

typedef struct _FILE_INFO_CLOSE_PARAMS
{
    DWORD              dwFileId;
} FILE_INFO_CLOSE_PARAMS, *PFILE_INFO_CLOSE_PARAMS;

LW_NTSTATUS
LwFileInfoMarshalEnumParameters(
    PFILE_INFO_ENUM_PARAMS pParams,
    PBYTE*                 ppBuffer,
    ULONG*                 pulBufferSize
    );

LW_NTSTATUS
LwFileInfoUnmarshalEnumParameters(
    PBYTE                   pBuffer,
    ULONG                   ulBufferSize,
    PFILE_INFO_ENUM_PARAMS* ppParams
    );

LW_NTSTATUS
LwFileInfoMarshalGetInfoParameters(
    PFILE_INFO_GET_INFO_PARAMS pParams,
    PBYTE*                     ppBuffer,
    ULONG*                     pulBufferSize
    );

LW_NTSTATUS
LwFileInfoUnmarshalGetInfoParameters(
    PBYTE                       pBuffer,
    ULONG                       ulBufferSize,
    PFILE_INFO_GET_INFO_PARAMS* ppParams
    );

LW_NTSTATUS
LwFileInfoMarshalCloseParameters(
    PFILE_INFO_CLOSE_PARAMS pParams,
    PBYTE*                  ppBuffer,
    ULONG*                  pulBufferSize
    );

LW_NTSTATUS
LwFileInfoUnmarshalCloseParameters(
    PBYTE                    pBuffer,
    ULONG                    ulBufferSize,
    PFILE_INFO_CLOSE_PARAMS* ppParams
    );

#endif /* _LWFILEINFO_H_ */
