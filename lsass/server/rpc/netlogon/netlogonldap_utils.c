#include "includes.h"

DWORD
NetlogonGetDefaultSearchBase(
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
        BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

        LwStrToUpper(pszBindDN_local);

        pszDC = strstr(pszBindDN_local, pszDCPrefix);
        if (!pszDC)
        {
                dwError = ERROR_NO_SUCH_DOMAIN;
                BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
        }

        if (IsNullOrEmptyString(pszDC))
        {
                dwError = ERROR_NO_SUCH_DOMAIN;
                BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
        }

        dwError = LwAllocateString(pszDC, &pszSearchBase);
        BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

        *ppszSearchBase = pszSearchBase;

cleanup:

        LW_SAFE_FREE_MEMORY(pszBindDN_local);

        return dwError;

error:

        *ppszSearchBase = NULL;

        LW_SAFE_FREE_MEMORY(pszSearchBase);

        goto cleanup;
}
