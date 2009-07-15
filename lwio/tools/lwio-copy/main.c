/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Tool to copy files/directories
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "includes.h"

static
NTSTATUS
ParseArgs(
    int      argc,
    char*    argv[],
    PSTR*    ppszCachePath,
    PSTR*    ppszSourcePath,
    PSTR*    ppszTargetPath,
    PBOOLEAN pbCopyRecursive
    );

static
VOID
ShowUsage(
    VOID
    );

static
NTSTATUS
GetKrb5PrincipalName(
    PCSTR pszCachePath,
    PSTR* ppszPrincipalName
    );

static
NTSTATUS
MapErrorCode(
    NTSTATUS status
    );

int
main(
    int argc,
    char* argv[]
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bPrintOrigError = TRUE;
    ULONG   ulErrorBufferSize = 0;
    PSTR pszSourcePath = NULL;
    PSTR pszTargetPath = NULL;
    PSTR pszCachePath = NULL;
    PSTR pszPrincipal = NULL;
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    BOOLEAN bRevertThreadAccessToken = FALSE;
    BOOLEAN bCopyRecursive = FALSE;

    status = ParseArgs(
                argc,
                argv,
                &pszCachePath,
                &pszSourcePath,
                &pszTargetPath,
                &bCopyRecursive);
    BAIL_ON_NT_STATUS(status);

    if (!IsNullOrEmptyString(pszCachePath))
    {
        status = GetKrb5PrincipalName(pszCachePath, &pszPrincipal);
        BAIL_ON_NT_STATUS(status);

        status = LwIoCreateKrb5AccessTokenA(
                        pszPrincipal,
                        pszCachePath,
                        &pAccessToken);
        BAIL_ON_NT_STATUS(status);

        status = LwIoSetThreadAccessToken(pAccessToken);
        BAIL_ON_NT_STATUS(status);

        bRevertThreadAccessToken = TRUE;
    }

    status = CopyFile(pszSourcePath, pszTargetPath, bCopyRecursive);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (pAccessToken)
    {
        if (bRevertThreadAccessToken)
        {
            LwIoSetThreadAccessToken(NULL);
        }

        LwIoDeleteAccessToken(pAccessToken);
    }

    LWIO_SAFE_FREE_STRING(pszTargetPath);
    LWIO_SAFE_FREE_STRING(pszSourcePath);
    LWIO_SAFE_FREE_STRING(pszCachePath);
    LWIO_SAFE_FREE_STRING(pszPrincipal);

    return (status);

error:

    status = MapErrorCode(status);

    ulErrorBufferSize = SMBStrError(status, NULL, 0);

    if (ulErrorBufferSize > 0)
    {
        NTSTATUS status2 = 0;
        PSTR   pszErrorBuffer = NULL;

        status2 = SMBAllocateMemory(ulErrorBufferSize, (PVOID*)&pszErrorBuffer);

        if (status2 == STATUS_SUCCESS)
        {
            ULONG ulLen = SMBStrError(status, pszErrorBuffer, ulErrorBufferSize);

            if ((ulLen == ulErrorBufferSize) &&
                !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to query status from SMB service.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LWIO_SAFE_FREE_STRING(pszErrorBuffer);
    }

    goto cleanup;
}

static
NTSTATUS
ParseArgs(
    int      argc,
    char*    argv[],
    PSTR*    ppszCachePath,
    PSTR*    ppszSourcePath,
    PSTR*    ppszTargetPath,
    PBOOLEAN pbCopyRecursive
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    typedef enum
    {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_KRB5_CACHE_PATH
    } ParseMode;
    int iArg = 1;
    ParseMode parseMode = PARSE_MODE_OPEN;
    PSTR pszSourcePath = NULL;
    PSTR pszTargetPath = NULL;
    PSTR pszCachePath = NULL;
    BOOLEAN bCopyRecursive = FALSE;

    for (iArg = 1; iArg < argc; iArg++)
    {
        PSTR pszArg = argv[iArg];

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if (!strcmp(pszArg, "-h"))
                {
                    ShowUsage();
                    exit(0);
                }
                else if (!strcmp(pszArg, "-r"))
                {
                    bCopyRecursive = TRUE;
                }
                else if (!strcmp(pszArg, "-k"))
                {
                    parseMode = PARSE_MODE_KRB5_CACHE_PATH;
                }
                else if (IsNullOrEmptyString(pszSourcePath))
                {
                    status = SMBAllocateString(
                                pszArg,
                                &pszSourcePath);
                    BAIL_ON_NT_STATUS(status);
                }
                else if (IsNullOrEmptyString(pszTargetPath))
                {
                    status = SMBAllocateString(
                                pszArg,
                                &pszTargetPath);
                    BAIL_ON_NT_STATUS(status);
                }
                else
                {
                    ShowUsage();
                    exit(0);
                }

                break;

            case PARSE_MODE_KRB5_CACHE_PATH:

                LWIO_SAFE_FREE_STRING(pszCachePath);

                status = SMBAllocateString(
                            pszArg,
                            &pszCachePath);
                BAIL_ON_NT_STATUS(status);

                parseMode = PARSE_MODE_OPEN;

                break;

            default:

                ShowUsage();
                exit(0);

                break;
        }
    }

    *ppszCachePath = pszCachePath;
    *ppszSourcePath = pszSourcePath;
    *ppszTargetPath = pszTargetPath;
    *pbCopyRecursive = bCopyRecursive;

cleanup:

    return status;

error:

    *ppszCachePath = NULL;
    *ppszSourcePath = NULL;
    *ppszTargetPath = NULL;
    *pbCopyRecursive = FALSE;

    LWIO_SAFE_FREE_STRING(pszTargetPath);
    LWIO_SAFE_FREE_STRING(pszSourcePath);
    LWIO_SAFE_FREE_STRING(pszCachePath);

    goto cleanup;
}

static
NTSTATUS
GetKrb5PrincipalName(
    PCSTR pszCachePath,
    PSTR* ppszPrincipalName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    krb5_error_code ret = 0;
    krb5_context    ctx = NULL;
    krb5_ccache     cc = NULL;
    krb5_principal  pKrb5Principal = NULL;
    PSTR  pszKrb5PrincipalName = NULL;
    PSTR  pszPrincipalName = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_get_principal(ctx, cc, &pKrb5Principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_unparse_name(ctx, pKrb5Principal, &pszKrb5PrincipalName);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ntStatus = SMBAllocateString(pszKrb5PrincipalName, &pszPrincipalName);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszPrincipalName = pszPrincipalName;

cleanup:

    if (ctx)
    {
        if (pszKrb5PrincipalName)
        {
            krb5_free_unparsed_name(ctx, pszKrb5PrincipalName);
        }
        if (pKrb5Principal)
        {
            krb5_free_principal(ctx, pKrb5Principal);
        }
        if (cc)
        {
            krb5_cc_close(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    return ntStatus;

error:

    *ppszPrincipalName = NULL;

    LWIO_SAFE_FREE_STRING(pszPrincipalName);

    ntStatus = STATUS_INVALID_ACCOUNT_NAME;

    goto cleanup;
}

static
VOID
ShowUsage(
    VOID
    )
{
    // printf("Usage: lwio-copy [-h] [-r] [-k <path>] <source path> <target path>\n");
    printf("Usage: lwio-copy [-h] [-k <path>] <source path> <target path>\n");
    printf("\t-h Show help\n");
    // printf("\t-r Recurse when copying a directory\n");
    printf("\t-k kerberos cache path\n");
    printf("Usage: lwio-copy -r //imgserver.abc.com/public/apple.jpg .\n");
}

static
NTSTATUS
MapErrorCode(
    NTSTATUS status
    )
{
    NTSTATUS status2 = status;

    switch (status)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:

            status2 = LWIO_ERROR_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return status2;
}

