#include "includes.h"


DWORD
VmDirGetDefaultSearchBase(
        PCSTR pszBindDN,
        PSTR* ppszSearchBase
        )
{
        DWORD dwError = LW_ERROR_SUCCESS;
        PSTR  pszBindDN_local = NULL;
        PCSTR pszDCPrefix   = "DC=";
        PCSTR pszDC = NULL;
        PSTR  pszSearchBase = NULL;

        dwError = LwAllocateString(pszBindDN, &pszBindDN_local);
        BAIL_ON_VMDIRDB_ERROR(dwError);

        LwStrToUpper(pszBindDN_local);

        pszDC = strstr(pszBindDN_local, pszDCPrefix);
        if (!pszDC)
        {
                dwError = ERROR_NO_SUCH_DOMAIN;
                BAIL_ON_VMDIRDB_ERROR(dwError);
        }

        if (IsNullOrEmptyString(pszDC))
        {
                dwError = ERROR_NO_SUCH_DOMAIN;
                BAIL_ON_VMDIRDB_ERROR(dwError);
        }

        dwError = LwAllocateString(pszDC, &pszSearchBase);
        BAIL_ON_VMDIRDB_ERROR(dwError);

        *ppszSearchBase = pszSearchBase;

cleanup:

        LW_SAFE_FREE_MEMORY(pszBindDN_local);

        return dwError;

error:

        *ppszSearchBase = NULL;

        LW_SAFE_FREE_MEMORY(pszSearchBase);

        goto cleanup;
}

DWORD
VmDirAttributesFromWc16Attributes(
    PWSTR *wszAttributes,
    PSTR **pppszAttributes,
    DWORD *pdwAttrCount)
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwAttrCount = 0;
    PSTR pszAttr = NULL;
    PSTR *ppszRetAttributes = NULL;

    
    /* Count the number of attributes to convert */
    for (i=0; wszAttributes[i]; i++)
        ;
    dwAttrCount = i;

    /* Allocate NULL-terminated return array  */
    dwError = LwAllocateMemory((dwAttrCount+1) * sizeof(PSTR),
                               (PVOID*)&ppszRetAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);


    for (i=0; i<dwAttrCount; i++)
    {
        ntStatus = LwRtlCStringAllocateFromWC16String(&pszAttr, wszAttributes[i]);
        if (ntStatus)
        {
            dwError =  LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_VMDIRDB_ERROR(dwError);
        }
        ppszRetAttributes[i] = pszAttr;
    }

    *pppszAttributes = ppszRetAttributes;
    *pdwAttrCount = dwAttrCount;
    ppszRetAttributes = NULL;

cleanup:
    if (ppszRetAttributes)
    {
        for (i=0; ppszRetAttributes[i]; i++)
        {
            LW_SAFE_FREE_STRING(ppszRetAttributes[i]);
        }
        LW_SAFE_FREE_MEMORY(ppszRetAttributes);
    }
    return dwError;

error:
    goto cleanup;
}

