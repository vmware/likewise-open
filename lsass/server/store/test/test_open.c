#include "includes.h"

DWORD test1(HANDLE hDirectory)
{
    DWORD dwError = 0;
    DWORD dwNumEntries = 0;
    PDIRECTORY_ENTRY  pDirectoryEntries = NULL;
    PWSTR *wszFilter = NULL;
    PWSTR *wszAttributes = NULL;
    PSTR ppszFilter[] = {"ObjectClass=1 OR ObjectClass=2", 0};
    PSTR pszAttributes[] = {"CommonName", 0};

    dwError = VmDirAttributesWc16FromCAttributes(pszAttributes, &wszAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAttributesWc16FromCAttributes(ppszFilter, &wszFilter);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = DirectorySearch(
                  hDirectory,
                  NULL,                     /* pwszBase */
                  0,                        /* ulScope */
                  wszFilter[0],             /* Filter term is first PWSTR of array */
                  wszAttributes,
                  0,                        /* ulAttributesOnly */
                  &pDirectoryEntries,
                  &dwNumEntries);

error:
    VmDirAttributesWC16Free(&wszFilter);
    VmDirAttributesWC16Free(&wszAttributes);


    return dwError;
}

DWORD test2(HANDLE hDirectory)
{
    DWORD dwError = 0;
    DWORD dwNumEntries = 0;
    PDIRECTORY_ENTRY  pDirectoryEntries = NULL;
    PWSTR *wszFilter = NULL;
    PWSTR *wszAttributes = NULL;
    PSTR ppszFilter[] = {"ObjectClass=1", 0};
    PSTR pszAttributes[] = {"CommonName", 0};

    dwError = VmDirAttributesWc16FromCAttributes(pszAttributes, &wszAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAttributesWc16FromCAttributes(ppszFilter, &wszFilter);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = DirectorySearch(
                  hDirectory,
                  NULL,                     /* pwszBase */
                  0,                        /* ulScope */
                  wszFilter[0],             /* Filter term is first PWSTR of array */
                  wszAttributes,
                  0,                        /* ulAttributesOnly */
                  &pDirectoryEntries,
                  &dwNumEntries);

error:
    VmDirAttributesWC16Free(&wszFilter);
    VmDirAttributesWC16Free(&wszAttributes);


    return dwError;
}

    
/* Builtin Domain */
DWORD test3(HANDLE hDirectory)
{
    DWORD dwError = 0;
    DWORD dwNumEntries = 0;
    PDIRECTORY_ENTRY  pDirectoryEntries = NULL;
    PWSTR *wszBase = NULL;
    PWSTR *wszBaseAlloc = NULL;
    PWSTR *wszFilter = NULL;
    PWSTR *wszAttributes = NULL;
#if 1
    PSTR ppszFilter[] = {"ObjectClass=1 OR ObjectClass=2",  0};
    PSTR pszAttributes[] = {"CommonName", 0};
#else
    PSTR ppszBase[] = {"cn=builtin,dc=lightwave,dc=local", 0};
    PSTR ppszFilter[] = {"(cn=*)", 0};
    PSTR pszAttributes[] = {"cn", 0};

    dwError = VmDirAttributesWc16FromCAttributes(ppszBase, &wszBaseAlloc);
    BAIL_ON_VMDIRDB_ERROR(dwError);
    wszBase = wszBaseAlloc;
#endif

    /* ldapsearch $logindata -b cn=builtin,dc=lightwave,dc=local '(cn=*)' cn  objectSid  */

    dwError = VmDirAttributesWc16FromCAttributes(pszAttributes, &wszAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirAttributesWc16FromCAttributes(ppszFilter, &wszFilter);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = DirectorySearch(
                  hDirectory,
                  wszBase ? wszBase[0] : NULL, /* pwszBase */
                  0,                        /* ulScope */
                  wszFilter[0],             /* Filter term is first PWSTR of array */
                  wszAttributes,
                  0,                        /* ulAttributesOnly */
                  &pDirectoryEntries,
                  &dwNumEntries);

error:
    VmDirAttributesWC16Free(&wszBaseAlloc);
    VmDirAttributesWC16Free(&wszFilter);
    VmDirAttributesWC16Free(&wszAttributes);


    return dwError;
}

    

int main(void)
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;

    printf("test_main: called\n");
    dwError = DirectoryOpen(&hDirectory);
    if (dwError)
    {
        printf("DirectoryOpen: failed %d (0x%x)\n", dwError, dwError);
        return 1;
    }

//    dwError = test1(hDirectory);
//    dwError = test2(hDirectory);
    dwError = test3(hDirectory);


    return 0;
}
