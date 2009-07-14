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
 *        dirattr.c
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
 *
 *      Directory Attribute Management
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

VOID
DirectoryFreeEntries(
    PDIRECTORY_ENTRY pEntries,
    DWORD            dwNumEntries
    )
{
    DWORD iEntry = 0;

    for (; iEntry < dwNumEntries; iEntry++)
    {
        PDIRECTORY_ENTRY pEntry = &pEntries[iEntry];

        if (pEntry->pAttributes)
        {
            DWORD iDirAttr = 0;

            for (; iDirAttr < pEntry->ulNumAttributes; iDirAttr++)
            {
                PDIRECTORY_ATTRIBUTE pDirAttr = NULL;

                pDirAttr = &pEntry->pAttributes[iDirAttr];

                if (pDirAttr->pwszName)
                {
                    DirectoryFreeStringW(pDirAttr->pwszName);
                }

                if (pDirAttr->pValues)
                {
                    DirectoryFreeAttributeValues(
                            pDirAttr->pValues,
                            pDirAttr->ulNumValues);
                }
            }

            DirectoryFreeMemory(pEntry->pAttributes);
        }
    }

    DirectoryFreeMemory(pEntries);
}

VOID
DirectoryFreeAttributeValues(
    PATTRIBUTE_VALUE pAttrValues,
    DWORD            dwNumValues
    )
{
    DWORD iValue = 0;

    for (; iValue < dwNumValues; iValue++)
    {
        PATTRIBUTE_VALUE pAttrValue = &pAttrValues[iValue];

        switch (pAttrValue->Type)
        {
            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:

                if (pAttrValue->data.pwszStringValue)
                {
                    DirectoryFreeMemory(pAttrValue->data.pwszStringValue);
                }

                break;

            case DIRECTORY_ATTR_TYPE_ANSI_STRING:

                if (pAttrValue->data.pszStringValue)
                {
                    DirectoryFreeMemory(pAttrValue->data.pszStringValue);
                }

                break;

            case DIRECTORY_ATTR_TYPE_OCTET_STREAM:

                if (pAttrValue->data.pOctetString)
                {
                    DIRECTORY_FREE_MEMORY(pAttrValue->data.pOctetString->pBytes);

                    DirectoryFreeMemory(pAttrValue->data.pOctetString);
                }

                break;

            default:

                break;
        }
    }

    DirectoryFreeMemory(pAttrValues);
}


DWORD
DirectoryGetEntryAttributeSingle(
    PDIRECTORY_ENTRY pEntry,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttribute = NULL;

    if (pEntry == NULL || ppAttribute == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pEntry->ulNumAttributes) {
        pAttribute = &(pEntry->pAttributes[0]);
    }

    *ppAttribute = pAttribute;

error:
    return dwError;
}


DWORD
DirectoryGetEntryAttributeByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    )
{
    DWORD dwError = 0;
    PWSTR pwszAttrName = NULL;
    PDIRECTORY_ATTRIBUTE pAttribute = NULL;
    PDIRECTORY_ATTRIBUTE pAttrFound = NULL;
    DWORD i = 0;

    if (pEntry == NULL || ppAttribute == NULL ||
        pwszAttributeName == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pwszAttrName = wc16sdup(pwszAttributeName);
    if (pwszAttrName == NULL) {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (i = 0; i < pEntry->ulNumAttributes; i++) {
        pAttribute = &(pEntry->pAttributes[i]);

        if (wc16scmp(pAttribute->pwszName,
                     pwszAttrName) == 0) {
            pAttrFound = pAttribute;
            break;
        }
    }

    *ppAttribute = pAttrFound;

cleanup:
    LSA_SAFE_FREE_MEMORY(pwszAttrName);

    return dwError;

error:
    goto cleanup;
}


DWORD
DirectoryGetEntryAttributeByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    )
{
    DWORD dwError = 0;
    PWSTR pwszAttributeName = NULL;
    PDIRECTORY_ATTRIBUTE pAttribute = NULL;

    dwError = LsaMbsToWc16s(pszAttributeName, &pwszAttributeName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttributeByName(pEntry,
                                               pwszAttributeName,
                                               &pAttribute);

    *ppAttribute = pAttribute;

cleanup:
    if (pwszAttributeName) {
        LSA_SAFE_FREE_MEMORY(pwszAttributeName);
    }

    return dwError;

error:
    *ppAttribute = NULL;
    goto cleanup;
}


DWORD
DirectoryGetAttributeValue(
    PDIRECTORY_ATTRIBUTE pAttribute,
    PATTRIBUTE_VALUE *ppAttrValue
    )
{
    DWORD dwError = 0;
    PATTRIBUTE_VALUE pValue = NULL;

    if (pAttribute == NULL || ppAttrValue == NULL) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pAttribute->ulNumValues) {
        pValue = &(pAttribute->pValues[0]);
    }

    *ppAttrValue = pValue;

error:
    return dwError;
}


DWORD
DirectoryGetEntryAttrValueByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    BOOLEAN *pbValue = NULL;
    ULONG *pulValue = NULL;
    LONG64 *pllValue = NULL;
    PWSTR *ppwszValue = NULL;
    PSTR *ppszValue = NULL;
    BOOLEAN bTypeIsCorrect = FALSE;

    dwError = DirectoryGetEntryAttributeByName(pEntry,
                                               pwszAttrName,
                                               &pAttr);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwError = DirectoryGetAttributeValue(pAttr,
                                         &pAttrVal);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    if (!pAttrVal) {
        goto cleanup;
    }

    bTypeIsCorrect = (pAttrVal->Type == AttrType);

    switch (pAttrVal->Type) {
    case DIRECTORY_ATTR_TYPE_BOOLEAN:
        pbValue = (BOOLEAN*)pValue;
        *pbValue = (bTypeIsCorrect) ? pAttrVal->data.bBooleanValue : FALSE;
        break;

    case DIRECTORY_ATTR_TYPE_INTEGER:
        pulValue = (ULONG*)pValue;
        *pulValue = (bTypeIsCorrect) ? pAttrVal->data.ulValue : 0;
        break;

    case DIRECTORY_ATTR_TYPE_LARGE_INTEGER:
        pllValue = (LONG64*)pValue;
        *pllValue = (bTypeIsCorrect) ? pAttrVal->data.llValue : 0;
        break;

    case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
        ppwszValue = (PWSTR*)pValue;
        *ppwszValue = (bTypeIsCorrect) ? pAttrVal->data.pwszStringValue : NULL;
        break;

    case DIRECTORY_ATTR_TYPE_ANSI_STRING:
        ppszValue = (PSTR*)pValue;
        *ppszValue = (bTypeIsCorrect) ? pAttrVal->data.pszStringValue : NULL;
        break;

    default:
        dwError = LW_ERROR_INVALID_PARAMETER;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
DirectoryGetEntryAttrValueByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    )
{
    DWORD dwError = 0;
    PWSTR pwszAttrName = NULL;

    dwError = LsaMbsToWc16s(pszAttrName, &pwszAttrName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               pwszAttrName,
                                               AttrType,
                                               pValue);

cleanup:
    if (pwszAttrName) {
        LSA_SAFE_FREE_MEMORY(pwszAttrName);
    }

    return dwError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
