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

#include "iop.h"
#include <lwmapsecurity/lwmapsecurity.h>

typedef struct _IO_CREATE_SECURITY_CONTEXT {
    IO_SECURITY_CONTEXT_PROCESS_INFORMATION Process;
    PACCESS_TOKEN AccessToken;
    LW_PIO_ACCESS_TOKEN Credentials;
} IO_CREATE_SECURITY_CONTEXT;

PIO_SECURITY_CONTEXT_PROCESS_INFORMATION
IoSecurityGetProcessInfo(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    )
{
    return &SecurityContext->Process;
}

PACCESS_TOKEN
IoSecurityGetAccessToken(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    )
{
    return SecurityContext->AccessToken;
}

LW_PIO_ACCESS_TOKEN
IoSecurityGetCredentials(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    )
{
    return SecurityContext->Credentials;
}

VOID
IoSecurityFreeSecurityContext(
    IN OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext
    )
{
    PIO_CREATE_SECURITY_CONTEXT securityContext = *SecurityContext;

    if (securityContext)
    {
        RtlReleaseAccessToken(&securityContext->AccessToken);
        IO_FREE(&securityContext);
    }
}

static
NTSTATUS
IopSecurityCreateSecurityContext(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN uid_t Uid,
    IN gid_t Gid,
    IN PACCESS_TOKEN AccessToken,
    IN OPTIONAL LW_PIO_ACCESS_TOKEN Credentials
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;

    status = RTL_ALLOCATE(&securityContext, IO_CREATE_SECURITY_CONTEXT, sizeof(*securityContext));
    GOTO_CLEANUP_ON_STATUS(status);

    securityContext->Process.Uid = Uid;
    securityContext->Process.Gid = Gid;
    securityContext->AccessToken = AccessToken;
    RtlReferenceAccessToken(AccessToken);
    securityContext->Credentials = Credentials;

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        IoSecurityFreeSecurityContext(&securityContext);
    }

    *SecurityContext = securityContext;

    return status;
}

NTSTATUS
IopSecurityCreateSecurityContextFromUidGid(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN uid_t Uid,
    IN gid_t Gid,
    IN OPTIONAL LW_PIO_ACCESS_TOKEN Credentials
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    PACCESS_TOKEN accessToken = NULL;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;

    // TODO-Use single map security context.
    status = LwMapSecurityCreateContext(&context);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUidGid(
                    context,
                    &accessToken,
                    Uid,
                    Gid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopSecurityCreateSecurityContext(
                    &securityContext,
                    Uid,
                    Gid,
                    accessToken,
                    Credentials);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        IoSecurityFreeSecurityContext(&securityContext);
    }

    RtlReleaseAccessToken(&accessToken);
    LwMapSecurityFreeContext(&context);

    *SecurityContext = securityContext;

    return status;
}

NTSTATUS
IoSecurityCreateSecurityContextFromUsername(
    OUT PIO_CREATE_SECURITY_CONTEXT* SecurityContext,
    IN PUNICODE_STRING Username
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    PACCESS_TOKEN accessToken = NULL;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;
    TOKEN_UNIX tokenUnix = { 0 };

    // TODO-Use single map security context.
    status = LwMapSecurityCreateContext(&context);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LwMapSecurityCreateAccessTokenFromUnicodeStringUsername(
                    context,
                    &accessToken,
                    Username);
    GOTO_CLEANUP_ON_STATUS(status);

    // TODO-Do we want to keep process information as the
    // current process or just take TOKEN_UNIX info?

    status = RtlQueryAccessTokenUnixInformation(
                    accessToken,
                    &tokenUnix);
    GOTO_CLEANUP_ON_STATUS(status);

    status = IopSecurityCreateSecurityContext(
                    &securityContext,
                    tokenUnix.Uid,
                    tokenUnix.Gid,
                    accessToken,
                    NULL);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        IoSecurityFreeSecurityContext(&securityContext);
    }

    RtlReleaseAccessToken(&accessToken);
    LwMapSecurityFreeContext(&context);

    *SecurityContext = securityContext;

    return status;
}
