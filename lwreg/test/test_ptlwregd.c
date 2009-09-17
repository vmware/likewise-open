/*
 * Copyright Likewise Software    2004-2009
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
 *        test_ptlwregd.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Multi-threaded lwregd test client
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#define LWREGD_MAX_THEADS 2
#define LWREGD_MAX_ITERATIONS 10

#include <config.h>
#include <regsystem.h>

#include <reg/reg.h>
#include <regutils.h>
#include <regdef.h>
#include <regclient.h>
#include "rsutils.h"

#include <lw/base.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwstr.h>
#include <lwmem.h>
#include <lwerror.h>

typedef struct _PTLWREGD_CONTEXT
{
    HANDLE hReg;
    pthread_t thread;
    PSTR pszKeyPath;
    PSTR pszKeyNamePrefix;
    DWORD dwRange;
    DWORD dwIterations;
    DWORD dwOperation;
} PTLWREGD_CONTEXT, *PPTLWREGD_CONTEXT;


DWORD
ThreadTestAddKey(
    HANDLE hReg,
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwRange,
    DWORD dwIterations)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    DWORD dwKeyNum = 0;
    PSTR pszKeyName = NULL;

    dwError = LwAllocateMemory(
                  strlen(pszKeyNamePrefix) + 20,
                  (LW_PVOID)&pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    for (dwCount=0; dwCount<dwIterations; dwCount++)
    {
        printf("ThreadTestAddKey: %d %s\\%s\n",
               dwCount,
               pszKeyPath,
               pszKeyNamePrefix);
        for (dwKeyNum=0; dwKeyNum<dwRange; dwKeyNum++)
        {
            sprintf(pszKeyName,
                    "%s-%d",
                    pszKeyNamePrefix,
                    dwKeyNum);
            dwError = RegShellUtilAddKey(hReg, pszKeyPath, pszKeyName);
            printf("    >>ThreadTestAddKey: %d %s\\%s\n",
                   dwCount,
                   pszKeyPath,
                   pszKeyName);
            BAIL_ON_REG_ERROR(dwError);
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
ThreadTestDeleteKey(
    HANDLE hReg,
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwRange,
    DWORD dwIterations)
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    DWORD dwKeyNum = 0;
    PSTR pszKeyName = NULL;

    dwError = LwAllocateMemory(
                  strlen(pszKeyNamePrefix) + 20,
                  (LW_PVOID)&pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    for (dwCount=0; dwCount<dwIterations; dwCount++)
    {
        printf("ThreadTestDeleteKey: %d %s\\%s\n",
               dwCount,
               pszKeyPath,
               pszKeyNamePrefix);
        for (dwKeyNum=0; dwKeyNum<dwRange; dwKeyNum++)
        {
            sprintf(pszKeyName,
                    "%s-%d",
                    pszKeyNamePrefix,
                    dwKeyNum);
            dwError = RegShellUtilDeleteKey(hReg, pszKeyPath, pszKeyName);
            if (dwError)
            {
                PrintError("ThreadTestDeleteKey", dwError);
            }
            printf("    >>ThreadTestDeleteKey: %d %s\\%s\n",
                   dwCount,
                   pszKeyPath,
                   pszKeyName);
        }
    }

cleanup:
    return dwError;

error:
    if (dwError == LW_ERROR_FAILED_DELETE_HAS_SUBKEY ||
        dwError == LW_ERROR_KEY_IS_ACTIVE ||
        dwError == LW_ERROR_NO_SUCH_KEY)
    {
        dwError = 0;
    }
    goto cleanup;
}


void *
ThreadTestPtKey(
    void *pctx)
{
    DWORD dwError = 0;
    PPTLWREGD_CONTEXT context = (PPTLWREGD_CONTEXT) pctx;
    PSTR pszOperation = context->dwOperation == 0 ?  "Add" : "Delete";

    printf("ThreadTestPt%sKey: starting %s\\%s\n",
           pszOperation,
           context->pszKeyPath,
           context->pszKeyNamePrefix);

    if (context->dwOperation == 0)
    {
        dwError = ThreadTestAddKey(
                      context->hReg,
                      context->pszKeyPath,
                      context->pszKeyNamePrefix,
                      context->dwRange,
                      context->dwIterations);
    }
    else
    {
        dwError = ThreadTestDeleteKey(
                      context->hReg,
                      context->pszKeyPath,
                      context->pszKeyNamePrefix,
                      context->dwRange,
                      context->dwIterations);
    }
    BAIL_ON_REG_ERROR(dwError);
    printf("ThreadTestPt%sKey: %s\\%s done.\n",
           pszOperation,
           context->pszKeyPath,
           context->pszKeyNamePrefix);
cleanup:
    return NULL;

error:
    goto cleanup;
}


void *
ThreadTestPtDeleteKey(
    void *pctx)
{
    DWORD dwError = 0;
    PPTLWREGD_CONTEXT context = (PPTLWREGD_CONTEXT) pctx;

    printf("ThreadTestPtDeleteKey: starting %s\\%s\n",
           context->pszKeyPath,
           context->pszKeyNamePrefix);

    dwError = ThreadTestDeleteKey(
                  context->hReg,
                  context->pszKeyPath,
                  context->pszKeyNamePrefix,
                  context->dwRange,
                  context->dwIterations);
    BAIL_ON_REG_ERROR(dwError);
    printf("ThreadTestPtDeleteKey: %s\\%s done.\n",
           context->pszKeyPath,
           context->pszKeyNamePrefix);
cleanup:
    return NULL;

error:
    goto cleanup;
}


DWORD
ThreadTestPtFree(
    PPTLWREGD_CONTEXT pCtx)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_HANDLE(pCtx);

    RegCloseServer(pCtx->hReg);
    LwFreeMemory(pCtx->pszKeyNamePrefix);
    LwFreeMemory(pCtx->pszKeyPath);
    LwFreeMemory(pCtx);
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
ThreadTestPtInit(
    PSTR pszKeyPath,
    PSTR pszKeyNamePrefix,
    DWORD dwKeyNameSuffix,
    DWORD dwIterations,
    DWORD dwOperation,
    PPTLWREGD_CONTEXT *ppRetCtx)
{
    PPTLWREGD_CONTEXT pCtx = NULL;
    DWORD dwError = 0;
    HANDLE hReg = NULL;

    dwError = LwAllocateMemory(sizeof(PTLWREGD_CONTEXT), (LW_PVOID) &pCtx);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenServer(&hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateString(pszKeyPath, &pCtx->pszKeyPath);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateMemory(strlen(pszKeyNamePrefix) + 11,
                               (LW_PVOID) &pCtx->pszKeyNamePrefix);
    BAIL_ON_REG_ERROR(dwError);

    sprintf(pCtx->pszKeyNamePrefix, "%s%d", pszKeyNamePrefix, dwKeyNameSuffix);

    pCtx->hReg = hReg;
    pCtx->dwRange = 1000;
    pCtx->dwIterations = dwIterations;
    pCtx->dwOperation = dwOperation;

    *ppRetCtx = pCtx;

cleanup:
    return dwError;

error:
    goto cleanup;
}


int main(int argc, char *argv[])
{
    PPTLWREGD_CONTEXT ctxAdd[LWREGD_MAX_THEADS] = {0};
    PPTLWREGD_CONTEXT ctxDel[LWREGD_MAX_THEADS] = {0};
    DWORD dwError = 0;

    int sts = 0;
    DWORD i = 0;

    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtInit("thread_tests",
                         "TestKey",
                         i,
                         LWREGD_MAX_ITERATIONS,
                         0,
                         &ctxAdd[i]);
    }

    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtInit("thread_tests",
                         "TestKey",
                         i,
                         LWREGD_MAX_ITERATIONS,
                         1,
                         &ctxDel[i]);
    }

ThreadTestPtKey(ctxAdd[0]);
ThreadTestPtKey(ctxDel[0]);
//exit(0);
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        sts = pthread_create(&ctxAdd[i]->thread,
                             NULL,
                             ThreadTestPtKey,
                             ctxAdd[i]);
        if (sts == -1)
        {
            printf("pthread_create: Error ThreadTestPtAddkey(ctxAdd[%d])\n", i);
            return 1;
        }
    }
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        sts = pthread_create(&ctxDel[i]->thread,
                             NULL,
                             ThreadTestPtKey,
                             ctxDel[i]);
        if (sts == -1)
        {
            printf("pthread_create: Error ThreadTestPtAddkey(ctxDel[%d])\n", i);
            return 1;
        }
    }
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        pthread_join(ctxAdd[i]->thread, NULL);
        pthread_join(ctxDel[i]->thread, NULL);
    }
    for (i=0; i<LWREGD_MAX_THEADS; i++)
    {
        ThreadTestPtFree(ctxAdd[i]);
        ThreadTestPtFree(ctxDel[i]);
    }
    return dwError;
}
