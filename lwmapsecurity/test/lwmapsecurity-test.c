/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software. All rights reserved.
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
 *        lwmapsecurity-test.c
 *
 * Abstract:
 *
 *        Likewise Map Security - Test Program
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include <lwmapsecurity/lwmapsecurity.h>
#include <lw/base.h>
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define LOG(Format, ...) \
    printf(Format "\n", ## __VA_ARGS__)

#define ASSERT_NT_SUCCESS_STATUS(status) \
    do { \
        if (!NT_SUCCESS(status)) \
        { \
            fprintf(stderr, "status = 0x%08x\n", status); \
            assert(FALSE); \
        } \
    } while (0)

static
VOID
DumpTokenInfo(
    IN PACCESS_TOKEN Token,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE tokenInfo = NULL;
    ULONG size = 0;
    ULONG savedSize = 0;

    status = RtlQueryAccessTokenInformation(
                    Token,
                    TokenInformationClass,
                    tokenInfo,
                    size,
                    &size);
    assert(STATUS_BUFFER_TOO_SMALL == status);

    savedSize = size;

    status = RTL_ALLOCATE(&tokenInfo, BYTE, size);
    ASSERT_NT_SUCCESS_STATUS(status);

    status = RtlQueryAccessTokenInformation(
                    Token,
                    TokenInformationClass,
                    tokenInfo,
                    size,
                    &size);
    ASSERT_NT_SUCCESS_STATUS(status);

    assert(size == savedSize);

    switch (TokenInformationClass)
    {
        case TokenUser:
        {
            PTOKEN_USER tokenUser = (PTOKEN_USER) tokenInfo;
            PSTR sidString = NULL;

            status = RtlAllocateCStringFromSid(&sidString, tokenUser->User.Sid);
            ASSERT_NT_SUCCESS_STATUS(status);

            LOG("User = %s", sidString);
            break;
        }
        case TokenGroups:
        {
            PTOKEN_GROUPS tokenGroups = (PTOKEN_GROUPS) tokenInfo;
            ULONG i = 0;
            PSTR sidString = NULL;

            LOG("GroupCount = %u", tokenGroups->GroupCount);

            for (i = 0; i < tokenGroups->GroupCount; i++)
            {
                status = RtlAllocateCStringFromSid(&sidString, tokenGroups->Groups[i].Sid);
                ASSERT_NT_SUCCESS_STATUS(status);

                LOG("Groups[%u] = (0x%08x, %s)", i, tokenGroups->Groups[i].Attributes, sidString);
            }
            break;
        }
        case TokenOwner:
        {
            PTOKEN_OWNER tokenOwner = (PTOKEN_OWNER) tokenInfo;
            PSTR sidString = NULL;

            status = RtlAllocateCStringFromSid(&sidString, tokenOwner->Owner);
            ASSERT_NT_SUCCESS_STATUS(status);

            LOG("Owner = %s", sidString);
            break;
        }
        case TokenPrimaryGroup:
        {
            PTOKEN_PRIMARY_GROUP tokenPrimaryGroup = (PTOKEN_PRIMARY_GROUP) tokenInfo;
            PSTR sidString = NULL;

            status = RtlAllocateCStringFromSid(&sidString, tokenPrimaryGroup->PrimaryGroup);
            ASSERT_NT_SUCCESS_STATUS(status);

            LOG("PrimaryGroup = %s", sidString);
            break;
        }
        case TokenDefaultDacl:
        {
            PTOKEN_DEFAULT_DACL tokenDefaultDacl = (PTOKEN_DEFAULT_DACL) tokenInfo;

            LOG("Have DefaultdDacl = %c", tokenDefaultDacl->DefaultDacl ? 'Y' : 'N');
            break;
        }
    }
}

static
VOID
DumpTokenUnixInfo(
    IN PACCESS_TOKEN Token
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TOKEN_UNIX tokenUnix = { 0 };

    status = RtlQueryAccessTokenUnixInformation(
                    Token,
                    &tokenUnix);
    ASSERT_NT_SUCCESS_STATUS(status);

    LOG("Unix.Uid = %u", tokenUnix.Uid);
    LOG("Unix.Gid = %u", tokenUnix.Uid);
    LOG("Unix.Umask = 0%03o", tokenUnix.Umask);
}

static
VOID
DumpToken(
    IN PACCESS_TOKEN Token
    )
{
    DumpTokenInfo(Token, TokenUser);
    DumpTokenInfo(Token, TokenGroups);
    DumpTokenInfo(Token, TokenOwner);
    DumpTokenInfo(Token, TokenPrimaryGroup);
    DumpTokenInfo(Token, TokenDefaultDacl);
    DumpTokenUnixInfo(Token);
}

static
VOID
Usage(
    IN PCSTR pszProgramName
    )
{
    printf("Usage: %s <uid> <gid>\n"
           "   or: %s <username>\n"
           "\n"
           "  Test lwmapsecurity library.\n"
           "\n",
           pszProgramName,
           pszProgramName);
}

int
main(
    IN int argc,
    IN PCSTR argv[]
    )
{
    NTSTATUS status = 0;
    PCSTR pszUsageError = NULL;
    PCSTR pszProgramName = argv[0];
    PLW_MAP_SECURITY_CONTEXT context = NULL;
    PACCESS_TOKEN accessToken = NULL;

    if ((argc < 2) || !strcasecmp("--help", argv[1]))
    {
        Usage(pszProgramName);
        return 0;
    }

    status = LwMapSecurityCreateContext(&context);
    ASSERT_NT_SUCCESS_STATUS(status);

    if (argc == 2)
    {
        status = LwMapSecurityCreateAccessTokenFromCStringUsername(
                        context,
                        &accessToken,
                        argv[1]);
        ASSERT_NT_SUCCESS_STATUS(status);
    }
    else if (argc == 3)
    {
        int uid = atoi(argv[1]);
        int gid = atoi(argv[2]);

        status = LwMapSecurityCreateAccessTokenFromUidGid(
                        context,
                        &accessToken,
                        (ULONG) uid,
                        (ULONG) gid);
        ASSERT_NT_SUCCESS_STATUS(status);
    }
    else
    {
        pszUsageError = "Too many arguments.\n";
        goto cleanup;
    }

    DumpToken(accessToken);

cleanup:
    if (pszUsageError)
    {
        printf("%s", pszUsageError);
        Usage(pszProgramName);
        status = STATUS_INVALID_PARAMETER;
    }

    RtlReleaseAccessToken(&accessToken);
    LwMapSecurityFreeContext(&context);

    return NT_SUCCESS(status) ? 0 : 1;
}
