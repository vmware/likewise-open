/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

#ifndef __LW_MAP_SECURITY_H__
#define __LW_MAP_SECURITY_H__

#include <lw/security-types.h>
#include <lw/ntstatus.h>
#include <lw/mapsecurity-types.h>

LW_BEGIN_EXTERN_C

typedef struct _LW_MAP_SECURITY_CONTEXT *PLW_MAP_SECURITY_CONTEXT;

typedef struct gss_ctx_id_struct *LW_MAP_SECURITY_GSS_CONTEXT;

//
// Every successful call to LwMapSecurityInitialize() must have
// exactly one corresponding call to LwMapSecurityCleanup().
// However, it is not necessary to call LwMapSecurityInitialize()
// before calling LwMapSecurityCreateContext().  Also, the library
// may return references to existing contexts and just do reference
// counting.
//

NTSTATUS
LwMapSecurityInitialize(
    VOID
    );

VOID
LwMapSecurityCleanup(
    VOID
    );

NTSTATUS
LwMapSecurityCreateContext(
    OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

VOID
LwMapSecurityFreeContext(
    IN OUT PLW_MAP_SECURITY_CONTEXT* Context
    );

NTSTATUS
LwMapSecurityGetIdFromSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PBOOLEAN IsUser,
    OUT PULONG Id,
    IN PSID Sid
    );

NTSTATUS
LwMapSecurityGetSidFromId(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PSID* Sid,
    IN BOOLEAN IsUser,
    IN ULONG Id
    );

VOID
LwMapSecurityFreeSid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    IN OUT PSID* Sid
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromUidGid(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN ULONG Uid,
    IN ULONG Gid
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PUNICODE_STRING Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromAnsiStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PANSI_STRING Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromWC16StringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCWSTR Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromCStringUsername(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN PCSTR Username
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromGssContext(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* AccessToken,
    IN LW_MAP_SECURITY_GSS_CONTEXT GssContext
    );

NTSTATUS
LwMapSecurityCreateAccessTokenFromNtlmLogon(
    IN PLW_MAP_SECURITY_CONTEXT Context,
    OUT PACCESS_TOKEN* pAccessToken,
    IN PLW_MAP_SECURITY_NTLM_LOGON_INFO pNtlmInfo,
    OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT* ppNtlmResult
    );

VOID
LwMapSecurityFreeNtlmLogonResult(
    IN PLW_MAP_SECURITY_CONTEXT                 Context,
    IN OUT PLW_MAP_SECURITY_NTLM_LOGON_RESULT*  pNtlmResult
    );

LW_END_EXTERN_C

#endif /* __LW_MAP_SECURITY_H__ */
