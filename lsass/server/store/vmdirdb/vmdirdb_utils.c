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
    PSTR **pppszAttributes)
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

cleanup:
    return dwError;

error:
    if (ppszRetAttributes)
    {
        for (i=0; ppszRetAttributes[i]; i++)
        {
            LW_SAFE_FREE_STRING(ppszRetAttributes[i]);
        }
        LW_SAFE_FREE_MEMORY(ppszRetAttributes);
    }
    goto cleanup;
}

DWORD
VmDirAttributesFree(
    PSTR **pppszAttributes)
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR *ppszAttributes = NULL;

    if (!pppszAttributes)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    ppszAttributes = *pppszAttributes;

    if (ppszAttributes)
    {
        for (i=0; ppszAttributes[i]; i++)
        {
            LW_SAFE_FREE_STRING(ppszAttributes[i]);
        }
        LW_SAFE_FREE_MEMORY(ppszAttributes);
    }
    *pppszAttributes = NULL;

error:
    return dwError;

}

DWORD
VmDirAttributesWc16FromCAttributes(
    PSTR *pszAttributes,
    PWSTR **pppwszAttributes)
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwAttrCount = 0;
    PWSTR pwszAttr = NULL;
    PWSTR *ppwszRetAttributes = NULL;

    /* Count the number of attributes to convert */
    for (i=0; pszAttributes[i]; i++)
        ;
    dwAttrCount = i;

    /* Allocate NULL-terminated return array  */
    dwError = LwAllocateMemory((dwAttrCount+1) * sizeof(PWSTR),
                               (PVOID*)&ppwszRetAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);


    for (i=0; i<dwAttrCount; i++)
    {
        ntStatus = LwRtlWC16StringAllocateFromCString(&pwszAttr, pszAttributes[i]);
        if (ntStatus)
        {
            dwError =  LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_VMDIRDB_ERROR(dwError);
        }
        ppwszRetAttributes[i] = pwszAttr;
    }

    *pppwszAttributes = ppwszRetAttributes;
    ppwszRetAttributes = NULL;

cleanup:
    if (ppwszRetAttributes)
    {
        for (i=0; ppwszRetAttributes[i]; i++)
        {
            LW_SAFE_FREE_MEMORY(ppwszRetAttributes[i]);
        }
        LW_SAFE_FREE_MEMORY(ppwszRetAttributes);
    }
    return dwError;

error:
    goto cleanup;
}


DWORD
VmDirGetDNFromFQDN(
    PCSTR pszFqdn,
    PSTR *ppszDn)
{
    DWORD dwError = 0;
    DWORD dwNameComps = 0;
    DWORD i = 0;
    DWORD j = 0;
    DWORD bFoundDot = 1;
    DWORD dwDnLen = 0;
    PSTR pszRetDn = NULL;


    /*
     * Convert string of format a.b.c -> dn=a,dn=b,dn=c
     */

    /* Count number of name components in input string */
    for (i=0; pszFqdn[i]; i++)
    {
        if (pszFqdn[i] == '.')
        {
            dwNameComps++;
        }
    }
    
    /*
     * Length: a.b.c -> dn=; , are accounted for by . in string
     */
    dwDnLen = strlen(pszFqdn) + dwNameComps * 3 + 1;
    dwError = LwAllocateMemory((dwDnLen) * sizeof(PSTR),
                               (PVOID*)&pszRetDn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    for (i=0; pszFqdn[i]; i++)
    {
        if (bFoundDot)
        {
            pszRetDn[j++] = 'd';
            pszRetDn[j++] = 'n';
            pszRetDn[j++] = '=';
            bFoundDot =  0;
        }
        if (pszFqdn[i] == '.')
        {
            pszRetDn[j++] = ',';
            bFoundDot = 1;
        }
        pszRetDn[j++] = pszFqdn[i];
    }
    pszRetDn[j++] = '\0';
    *ppszDn = pszRetDn;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszRetDn);
    goto cleanup;
}

DWORD
VmDirAttributesWC16Free(
    PWSTR **pppwszAttributes)
{
    DWORD dwError = 0;
    DWORD i = 0;
    PWSTR *ppwszAttributes = NULL;

    if (!pppwszAttributes)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
    ppwszAttributes = *pppwszAttributes;

    if (ppwszAttributes)
    {
        for (i=0; ppwszAttributes[i]; i++)
        {
            LW_SAFE_FREE_MEMORY(ppwszAttributes[i]);
        }
        LW_SAFE_FREE_MEMORY(ppwszAttributes);
    }
    *pppwszAttributes = NULL;

error:
    return dwError;

}
