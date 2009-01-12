/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Test Program for Red Black Tree Implementation
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"

static
int
CompareValues(
    PVOID pVal1,
    PVOID pVal2
    )
{
    PDWORD pdwVal1 = (PDWORD)pVal1;
    PDWORD pdwVal2 = (PDWORD)pVal2;

    if (*pdwVal1 < *pdwVal2)
    {
        return -1;
    }
    else if (*pdwVal1 > *pdwVal2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[]
    );

VOID
ShowUsage();

static
DWORD
PrintTree(
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSMB_RB_TREE pTree = NULL;
    DWORD valArray[10];
    DWORD dwValueToFind = 0;
    DWORD iValue = 0;

    dwError = ParseArgs(argc, argv);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBRBTreeCreate(
                 &CompareValues,
                 NULL,
                 &pTree);
    BAIL_ON_SMB_ERROR(dwError);

    for (iValue = 0; iValue < 10; iValue++)
    {
        valArray[iValue] = iValue;

        dwError = SMBRBTreeAdd(pTree, &valArray[iValue]);
        BAIL_ON_SMB_ERROR(dwError);
    }

    printf("========================\n");
    printf("\nPrinting tree pre-order\n");
    printf("========================\n");

    dwError = SMBRBTreeTraverse(
                    pTree,
                    SMB_TREE_TRAVERSAL_TYPE_PRE_ORDER,
                    &PrintTree,
                    NULL);
    BAIL_ON_SMB_ERROR(dwError);

    printf("========================\n");
    printf("\nPrinting tree in-order\n");
    printf("========================\n");
    dwError = SMBRBTreeTraverse(
                    pTree,
                    SMB_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &PrintTree,
                    NULL);
    BAIL_ON_SMB_ERROR(dwError);

    printf("========================\n");
    printf("\nPrinting tree post-order\n");
    printf("========================\n");
    dwError = SMBRBTreeTraverse(
                    pTree,
                    SMB_TREE_TRAVERSAL_TYPE_POST_ORDER,
                    &PrintTree,
                    NULL);
    BAIL_ON_SMB_ERROR(dwError);

    dwValueToFind = 7;
    if (!SMBRBTreeFind(pTree, &dwValueToFind))
    {
       fprintf(stderr, "Failed to find value [%d] in tree\n", dwValueToFind);
    }
    else
    {
       fprintf(stdout, "Successfully found value [%d] in tree\n", dwValueToFind);
    }

    printf("Deleting key [%d] from tree\n", dwValueToFind);

    dwError = SMBRBTreeRemove(pTree, &dwValueToFind);
    BAIL_ON_SMB_ERROR(dwError);

    printf("========================\n");
    printf("\nPrinting tree in-order\n");
    printf("========================\n");
    dwError = SMBRBTreeTraverse(
                    pTree,
                    SMB_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &PrintTree,
                    NULL);
    BAIL_ON_SMB_ERROR(dwError);

    printf("Finding key [%d] in tree\n", dwValueToFind);

    if (!SMBRBTreeFind(pTree, &dwValueToFind))
    {
       fprintf(stderr, "Failed to find value [%d] in tree\n", dwValueToFind);
    }
    else
    {
       fprintf(stdout, "Successfully found value [%d] in tree\n", dwValueToFind);
    }

    for (iValue = 0; iValue < 10; iValue++)
    {
        if (iValue != dwValueToFind)
        {
            printf("Removing value [%d] from tree\n", iValue);

            dwError = SMBRBTreeRemove(pTree, &iValue);
            BAIL_ON_SMB_ERROR(dwError);
        }
    }

    printf("========================\n");
    printf("\nPrinting tree in-order\n");
    printf("========================\n");
    dwError = SMBRBTreeTraverse(
                    pTree,
                    SMB_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &PrintTree,
                    NULL);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pTree) {
       SMBRBTreeFree(pTree);
    }

    return (dwError);

error:

    fprintf(stderr, "Failed Red Black Tree test. Error code [%u]\n", dwError);

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[]
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                break;

        }

    } while (iArg < argc);

    return dwError;
}

void
ShowUsage()
{
    printf("Usage: test-rbtree\n");
}

static
DWORD
PrintTree(
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    if (!pData)
    {
        printf("<invalid item>\n");
    }
    else
    {
        printf("%d\n", *((PDWORD)(pData)));
    }

    return 0;
}


