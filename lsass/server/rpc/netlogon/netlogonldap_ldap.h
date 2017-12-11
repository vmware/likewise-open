#define DEFAULT_LDAP_QUERY_TIMEOUT_SECS  15

#ifndef IsNullOrEmptyString
#define IsNullOrEmptyString(str) (!(str) || !(*str))
#endif

typedef struct _NETLOG0N_DIR_CONTEXT
{
    PNETLOGON_LDAP_BIND_INFO pBindInfo;
    LDAP*            pLd;

} NETLOG0N_DIR_CONTEXT, *PNETLOG0N_DIR_CONTEXT;

typedef struct _NETLOGON_AUTH_PROVIDER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    uid_t peer_uid;
    gid_t peer_gid;
    pid_t peer_pid;

    NETLOG0N_DIR_CONTEXT  dirContext;

} NETLOGON_AUTH_PROVIDER_CONTEXT, *PNETLOGON_AUTH_PROVIDER_CONTEXT;


typedef struct _NETLOGON_SASL_INFO
{
    PCSTR pszRealm;
    PCSTR pszAuthName;
    PCSTR pszUser;
    PCSTR pszPassword;
} NETLOGON_SASL_INFO, *PNETLOGON_SASL_INFO;


DWORD
NetlogonLdapOpen(
    PHANDLE phDirectory
    );


DWORD
NetlogonLdapQueryObjects(
	LDAP*         pLd,
	PCSTR         pszBaseDN,
	int           scope,
	PCSTR         pszFilter,
	char**        attrs,
	int           sizeLimit,
	LDAPMessage** ppMessage
	);

DWORD
NetlogonLdapGetValues(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	DWORD        dwNumValues
	);

VOID
NetlogonLdapFreeMessage(
	LDAPMessage* pMessage
	);

DWORD
NetlogonLdapInitialize(
        PCSTR            pszURI,
        PCSTR            pszUPN,
        PCSTR            pszPassword,
        PCSTR            pszCachePath,
        LDAP**           ppLd
        );

VOID
NetlogonLdapClose(
	LDAP* pLd
	);

DWORD
NetlogonLdapAllocateEntriesAndAttributes(
    DWORD dwNumEntries,     /* Number of directory entries */
    PDWORD pdwAttributesCount,
    PDIRECTORY_ENTRY *ppDirectoryEntries);

DWORD
NetlogonAttributeCopyEntry(
    PDIRECTORY_ATTRIBUTE out,
    PDIRECTORY_ATTRIBUTE in);

DWORD
NetlogonConstructMachineDN(
    PSTR pszSqlDomainName,
    PSTR *ppszMachineAcctDN);

DWORD
NetlogonConstructMachineUPN(
    PSTR pszSqlDomainName,
    PSTR *ppszMachineAcctUpn);

DWORD
NetlogonLdapQuerySingleObject(
        LDAP*         pLd,
        PCSTR         pszBaseDN,
        int           scope,
        PCSTR         pszFilter,
        char**        attrs,
        LDAPMessage** ppMessage
        );

