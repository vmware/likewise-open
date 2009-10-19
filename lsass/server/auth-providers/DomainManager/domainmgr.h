typedef struct _LSA_DM_CONST_DC_INFO {
    DWORD dwDsFlags;
    PWSTR pszName;
    PWSTR pszAddress;
    PWSTR pszSiteName;
} LSA_DM_CONST_DC_INFO, *PLSA_DM_CONST_DC_INFO;

typedef struct _DOMAIN_ENTRY
{
   LSA_DM_DOMAIN_FLAGS Flags;
   DOMAIN_TYPE DomainType;

   PWSTR pszDnsName;
   PWSTR pszNetbiosName;
   PWSTR pszTrusteeDnsName;
   DWORD dwTrustFlags;
   DWORD dwTrustType;
   DWORD dwTrustAttributes;

   PSID pSid;
   uuid_t Guid;

   PWSTR pszForestName;

   /// Lsa internal trust category
   LSA_TRUST_DIRECTION dwTrustDirection;
   LSA_TRUST_MODE dwTrustMode;

   /// These three are NULL unless someone explictily stored DC/GC info.
   PWSTR pszClientSiteName;
   PLSA_DM_DC_INFO pDcInfoList;
   PLSA_DM_DC_INFO pGcInfoList;
 struct _DOMAIN_ENTRY * pNext;
} DOMAIN_ENTRY, *PDOMAIN_ENTRY;


DWORD
DomainMgrAddDomain(
	LPWSTR pszDomainName,
	PDOMAIN_INFO pDomainInfo
	)
{

	ENTER_CRITICAL_SECTION()

	dwError = FindDomainEntry(
					pszDomainName,
					&pDomainEntry
					);
	if (dwError == 0){
		dwError = LW_ERROR_DOMAIN_ENTRY_EXISTS;
		BAIL_ON_ERROR(dwError);
	}
	dwError = CreateDomainEntry(
					pDomainInfo,
					&pDomainEntry
					);
	BAIL_ON_ERROR(dwError);

	pDomainEntry->pNext = gpDomainList;
	gpDomainList = pDomainEntry;

cleanup:

	LEAVE_CRITICAL_SECTION();

	return dwError;

error:

	goto cleanup;
}

DWORD
CreateDomainEntry(
	PDOMAIN_INFO pDomainInfo,
	PDOMAIN_ENTRY * ppDomainEntry
	)
{
	dwError = LsaAllocateMemory(
					sizeof(DOMAIN_INFO),
					&pDomainEntry
					);
	BAIL_ON_ERROR(dwError);

	return dwError;
}

DWORD
DomainMgrOpenLdapConnection(
	PWSTR pszDomainName,
	PLSA_LDAP_CONNECTION * ppLsaLdapConnection
	)
{

}

DWORD
DomainMgrCloseLdapConnection(
	PLSA_LDAP_CONNECTION  pLsaLdapConnection
	)
{
}
