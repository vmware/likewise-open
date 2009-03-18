#include "includes.h"

static
VOID
SamDbFreeAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

static
int
SamDbCompareAttributeLookupKeys(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SamDbFreeAttributeLookupData(
    PVOID pData
    );

DWORD
SamDbBuildAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP* ppAttrLookup
    )
{
    DWORD dwError = 0;
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(SAMDB_ATTRIBUTE_LOOKUP),
                    (PVOID*)&pAttrLookup);
    BAIL_ON_SAMDB_ERROR(dwError);

    SamDbInitializeInterlockedCounter(&pAttrLookup->counter);
    SamDbInterlockedIncrement(&pAttrLookup->counter);

    dwError = LwRtlRBTreeCreate(
                    &SamDbCompareAttributeLookupKeys,
                    NULL,
                    &SamDbFreeAttributeLookupData,
                    &pAttrLookup->pAttrTree);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddUserAttrLookups(pAttrLookup);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddGroupAttrLookups(pAttrLookup);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddDomainAttrLookups(pAttrLookup);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppAttrLookup = pAttrLookup;

cleanup:

    return dwError;

error:

    *ppAttrLookup = NULL;

    if (pAttrLookup)
    {
        SamDbFreeAttributeLookup(pAttrLookup);
    }

    goto cleanup;
}

DWORD
SamDbAcquireAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP  pAttrLookup,
    PSAMDB_ATTRIBUTE_LOOKUP* ppAttrLookup
    )
{
    DWORD dwError = 0;

    SamDbInterlockedIncrement(&pAttrLookup->counter);

    *ppAttrLookup = pAttrLookup;

    return dwError;
}

VOID
SamDbReleaseAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    )
{
    SamDbInterlockedDecrement(&pAttrLookup->counter);

    if (SamDbInterlockedCounter(&pAttrLookup->counter) == 0)
    {
        SamDbFreeAttributeLookup(pAttrLookup);
    }
}

static
VOID
SamDbFreeAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    )
{
    if (pAttrLookup->pAttrTree)
    {
        LwRtlRBTreeFree(pAttrLookup->pAttrTree);
    }

    DirectoryFreeMemory(pAttrLookup);
}

static
int
SamDbCompareAttributeLookupKeys(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PWSTR pwszKey1 = (PWSTR)pKey1;
    PWSTR pwszKey2 = (PWSTR)pKey2;

    return wc16scasecmp(pwszKey1, pwszKey2);
}

static
VOID
SamDbFreeAttributeLookupData(
    PVOID pData
    )
{
    SamDbFreeAttributeLookupEntry((PSAMDB_ATTRIBUTE_LOOKUP_ENTRY)pData);
}

VOID
SamDbFreeAttributeLookupEntry(
    PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pLookupEntry
    )
{
    if (pLookupEntry->pwszAttributeName)
    {
        DirectoryFreeMemory(pLookupEntry->pwszAttributeName);
    }

    DirectoryFreeMemory(pLookupEntry);
}
