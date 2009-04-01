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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samdbmisc.c
 *
 * Abstract:
 *
 *        Likewise SAM DB
 *
 *        Misc Functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

DWORD
SamDbComputeLMHash(
    PCSTR pszPassword,
    PBYTE pHash,
    DWORD dwHashByteLen
    )
{
    DWORD dwError = 0;

    if (!pHash || (dwHashByteLen != 16))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    memset(pHash, 0, dwHashByteLen);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbComputeNTHash(
    PCSTR pszPassword,
    PBYTE pHash,
    DWORD dwHashByteLen
    )
{
    DWORD dwError = 0;

    if (!pHash || (dwHashByteLen != 16))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    memset(pHash, 0, dwHashByteLen);

    if (pszPassword)
    {
        MD4((PBYTE)pszPassword, strlen(pszPassword), pHash);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbGetObjectClass(
    DIRECTORY_MOD       Modifications[],
    SAMDB_OBJECT_CLASS* pObjectClass
    )
{
    DWORD dwError = 0;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    wchar16_t pwszObjectClassAttr[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    DWORD dwNumMods = 0;

    while (Modifications[dwNumMods].pwszAttrName &&
           Modifications[dwNumMods].pAttrValues)
    {
        if (!wc16scasecmp(&pwszObjectClassAttr[0],
                          Modifications[dwNumMods].pwszAttrName))
        {
            PATTRIBUTE_VALUE pAttrValue = NULL;

            if (Modifications[dwNumMods].ulNumValues != 1)
            {
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            pAttrValue = &Modifications[dwNumMods].pAttrValues[0];

            if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
            {
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            switch (pAttrValue->ulValue)
            {
                case SAMDB_OBJECT_CLASS_DOMAIN:
                case SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN:
                case SAMDB_OBJECT_CLASS_CONTAINER:
                case SAMDB_OBJECT_CLASS_GROUP:
                case SAMDB_OBJECT_CLASS_USER:

                    objectClass = pAttrValue->ulValue;

                    break;

                default:

                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);

                    break;
            }

            break;
        }

        dwNumMods++;
    }

    *pObjectClass = objectClass;

cleanup:

    return dwError;

error:

    *pObjectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    goto cleanup;
}

DWORD
SamDbFindObjectClassMapInfo(
    SAMDB_OBJECT_CLASS                   objectClass,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO  pMapInfos,
    DWORD                                dwNumMapInfos,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO* ppMapInfo
    )
{
    DWORD dwError = 0;
    DWORD iMap = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pMapInfo = NULL;

    for (; iMap < dwNumMapInfos; iMap++)
    {
        PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pIterMapInfo = &pMapInfos[iMap];

        if (pIterMapInfo->objectClass == objectClass)
        {
            pMapInfo = pIterMapInfo;
            break;
        }
    }

    if (!pMapInfo)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppMapInfo = pMapInfo;

cleanup:

    return dwError;

error:

    *ppMapInfo = NULL;

    goto cleanup;
}

PSAM_DB_COLUMN_VALUE
SamDbReverseColumnValueList(
    PSAM_DB_COLUMN_VALUE pColumnValueList
    )
{
    PSAM_DB_COLUMN_VALUE pP = NULL;
    PSAM_DB_COLUMN_VALUE pQ = pColumnValueList;
    PSAM_DB_COLUMN_VALUE pR = NULL;

    while( pQ )
    {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

VOID
SamDbFreeColumnValueList(
    PSAM_DB_COLUMN_VALUE pColValueList
    )
{
    while (pColValueList)
    {
        PSAM_DB_COLUMN_VALUE pTmp = pColValueList;

        pColValueList = pColValueList->pNext;

        if (pTmp->pAttrValues)
        {
            DirectoryFreeAttributeValues(
                    pTmp->pAttrValues,
                    pTmp->ulNumValues);
        }

        DirectoryFreeMemory(pTmp);
    }
}

DWORD
SamDbGetNumberOfDependents_inlock(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PDWORD                 pdwNumDependents
    )
{
    DWORD dwError = 0;

    // TODO:

    *pdwNumDependents = 0;

    return dwError;
}
