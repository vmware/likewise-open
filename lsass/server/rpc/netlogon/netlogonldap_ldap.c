/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

static
DWORD
NetlogonLdapInitializeWithKerberos(
    PCSTR            pszURI,
    PCSTR            pszUPN,
    PCSTR            pszPassword,
    PCSTR            pszCachePath,
    LDAP**           ppLd
    );

static
DWORD
NetlogonLdapInitializeWithSRP(
    PCSTR            pszURI,
    PCSTR            pszUPN,
    PCSTR            pszPassword,
    PCSTR            pszCachePath,
    LDAP**           ppLd
    );

static
int
NetlogonSASLInteractionKerberos(
    LDAP*    pLd,
    unsigned flags,
    PVOID    pDefaults,
    PVOID    pIn
    );

static
int
NetlogonSASLInteractionSRP(
     LDAP*    pLd,
     unsigned flags,
     PVOID    pDefaults,
     PVOID    pIn
     );


DWORD
NetlogonLdapInitialize(
	PCSTR            pszURI,
	PCSTR            pszUPN,
	PCSTR            pszPassword,
    PCSTR            pszCachePath,
	LDAP**           ppLd
	)
{
    DWORD dwError = 0;
    
    switch (gNetlogonGlobals.bindProtocol)
    {
        case NETLOGON_LDAP_BIND_PROTOCOL_KERBEROS:
            
            dwError = NetlogonLdapInitializeWithKerberos(
                            pszURI,
                            pszUPN,
                            pszPassword,
                            pszCachePath,
                            ppLd);
            
            break;
            
        case NETLOGON_LDAP_BIND_PROTOCOL_SRP:
            
            dwError = NetlogonLdapInitializeWithSRP(
                            pszURI,
                            pszUPN,
                            pszPassword,
                            pszCachePath,
                            ppLd);
            
            break;
            
        default:
            
            dwError = ERROR_INVALID_STATE;
            
            break;
    }
    
    return dwError;
}

DWORD
NetlogonLdapQueryObjects(
	LDAP*         pLd,
	PCSTR         pszBaseDN,
	int           scope,
	PCSTR         pszFilter,
	char**        attrs,
	int           sizeLimit,
	LDAPMessage** ppMessage
	)
{
	DWORD dwError = 0;

	struct timeval waitTime = {0};

	waitTime.tv_sec  = DEFAULT_LDAP_QUERY_TIMEOUT_SECS;
	waitTime.tv_usec = 0;

	dwError = LwMapLdapErrorToLwError(
				ldap_search_ext_s(
					pLd,
					pszBaseDN,
					scope,
					pszFilter,
					attrs,
					FALSE,     /* Attrs only      */
					NULL,      /* Server controls */
					NULL,      /* Client controls */
					&waitTime,
					sizeLimit, /* size limit      */
					ppMessage));

	return dwError;
}


VOID
NetlogonLdapFreeMessage(
	LDAPMessage* pMessage
	)
{
	ldap_msgfree(pMessage);
}

VOID
NetlogonLdapClose(
	LDAP* pLd
	)
{
	ldap_unbind_ext(pLd, NULL, NULL);
}

static
DWORD
NetlogonLdapInitializeWithKerberos(
    PCSTR            pszURI,
    PCSTR            pszUPN,
    PCSTR            pszPassword,
    PCSTR            pszCachePath,
    LDAP**           ppLd
    )
{
    DWORD dwError = 0;
    const int ldapVer = LDAP_VERSION3;
    PSTR  pszUPN_local = NULL;
    LDAP* pLd = NULL;
    PSTR pszOldCachePath = NULL;
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_initialize(&pLd, pszURI));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_set_option(pLd, LDAP_OPT_PROTOCOL_VERSION, &ldapVer));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_set_option(
                                  pLd,
                                  LDAP_OPT_X_SASL_NOCANON,
                                  LDAP_OPT_ON));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = LwKrb5SetThreadDefaultCachePath(
                    pszCachePath,
                    &pszOldCachePath);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                    ldap_sasl_interactive_bind_s(
                                               pLd,
                                               NULL,
                                               "GSSAPI",
                                               NULL,
                                               NULL,
                                               LDAP_SASL_QUIET,
                                               &NetlogonSASLInteractionKerberos,
                                               NULL));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    *ppLd = pLd;
    
cleanup:
    
    if (pszOldCachePath)
    {
        LwKrb5SetThreadDefaultCachePath(
                pszOldCachePath,
                NULL);
        LwFreeString(pszOldCachePath);
    }
    
    LW_SAFE_FREE_STRING(pszUPN_local);
    
    return dwError;
    
error:
    
    *ppLd = NULL;
    
    if (pLd)
    {
        NetlogonLdapClose(pLd);
    }
    
    goto cleanup;
}

static
DWORD
NetlogonLdapInitializeWithSRP(
   PCSTR            pszURI,
   PCSTR            pszUPN,
   PCSTR            pszPassword,
   PCSTR            pszCachePath,
   LDAP**           ppLd
   )
{
    DWORD dwError = 0;
    const int ldapVer = LDAP_VERSION3;
    NETLOGON_SASL_INFO srpDefault = {0};
    PSTR  pszUPN_local = NULL;
    LDAP* pLd = NULL;
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_initialize(&pLd, pszURI));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_set_option(pLd, LDAP_OPT_PROTOCOL_VERSION, &ldapVer));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_set_option(
                                  pLd,
                                  LDAP_OPT_X_SASL_NOCANON,
                                  LDAP_OPT_ON));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = LwAllocateString(pszUPN, &pszUPN_local);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    LwStrToLower(pszUPN_local);
    
    srpDefault.pszAuthName = pszUPN_local;
    
    srpDefault.pszPassword = pszPassword;
    
    dwError = LwMapLdapErrorToLwError(
                  ldap_sasl_interactive_bind_s(
                                               pLd,
                                               NULL,
                                               "SRP",
                                               NULL,
                                               NULL,
                                               LDAP_SASL_QUIET,
                                               &NetlogonSASLInteractionSRP,
                                               &srpDefault));
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    *ppLd = pLd;
    
cleanup:
    
    LW_SAFE_FREE_STRING(pszUPN_local);
    
    return dwError;
    
error:
    
    *ppLd = NULL;
    
    if (pLd)
    {
        NetlogonLdapClose(pLd);
    }
    
    goto cleanup;
}

static
int
NetlogonSASLInteractionKerberos(
    LDAP *      pLd,
    unsigned    flags,
    void *      pDefaults,
    void *      pIn
    )
{
    // dummy function to satisfy ldap_sasl_interactive_bind call
    return LDAP_SUCCESS;
}

static
int
NetlogonSASLInteractionSRP(
    LDAP *      pLd,
    unsigned    flags,
    void *      pDefaults,
    void *      pIn
    )
{
    sasl_interact_t* pInteract = pIn;
    PNETLOGON_SASL_INFO pDef = pDefaults;
    
    while( (pDef != NULL) && (pInteract->id != SASL_CB_LIST_END) )
    {
        switch( pInteract->id )
        {
            case SASL_CB_GETREALM:
                pInteract->defresult = pDef->pszRealm;
                break;
            case SASL_CB_AUTHNAME:
                pInteract->defresult = pDef->pszAuthName;
                break;
            case SASL_CB_PASS:
                pInteract->defresult = pDef->pszPassword;
                break;
            case SASL_CB_USER:
                pInteract->defresult = pDef->pszUser;
                break;
            default:
                break;
        }
        
        pInteract->result = (pInteract->defresult) ? pInteract->defresult : "";
        pInteract->len    = strlen( pInteract->result );
        
        pInteract++;
    }
    
    return LDAP_SUCCESS;
}
