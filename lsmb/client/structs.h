/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __SMB_SERVER_CONNECTION
{
    LWMsgAssoc* pAssoc;
} SMB_SERVER_CONNECTION, *PSMB_SERVER_CONNECTION;

typedef enum __SMB_API_HANDLE_TYPE
{
    SMB_API_HANDLE_FILE,
    SMB_API_HANDLE_ACCESS
} SMB_API_HANDLE_TYPE;

typedef struct __SMB_API_HANDLE
{
    SMB_API_HANDLE_TYPE type;
    union
    {
        HANDLE hIPCHandle;
        SMB_SECURITY_TOKEN_REP securityToken;
    } variant;
} SMB_API_HANDLE, *PSMB_API_HANDLE;

typedef struct __SMB_CLIENT_CONTEXT
{
    HANDLE hAccessToken;
    DWORD dwLastError;
    unsigned bSecurityTokenIsPrivate:1;
} SMB_CLIENT_CONTEXT, *PSMB_CLIENT_CONTEXT;

#define BAIL_IF_NOT_FILE_HANDLE(h)                                      \
    do                                                                  \
    {                                                                   \
        if (((PSMB_API_HANDLE)(h))->type != SMB_API_HANDLE_FILE)        \
        {                                                               \
            dwError = SMB_ERROR_INVALID_HANDLE;                         \
            goto error;                                                 \
        }                                                               \
    } while (0)

#endif /* __STRUCTS_H__ */
