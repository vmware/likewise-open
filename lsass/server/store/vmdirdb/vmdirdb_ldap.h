#define DEFAULT_LDAP_QUERY_TIMEOUT_SECS  15

#ifndef IsNullOrEmptyString
#define IsNullOrEmptyString(str) (!(str) || !(*str))
#endif


typedef enum
{
    VMDIR_ATTR_TYPE_UNKNOWN = 0,
    VMDIR_ATTR_TYPE_INT32,
    VMDIR_ATTR_TYPE_UINT32,
    VMDIR_ATTR_TYPE_INT64,
    VMDIR_ATTR_TYPE_UINT64,
    VMDIR_ATTR_TYPE_STRING,
    VMDIR_ATTR_TYPE_MULTI_STRING,
    VMDIR_ATTR_TYPE_DN,
    VMDIR_ATTR_TYPE_BINARY

} VMDIR_ATTR_TYPE;

typedef struct _VMDIR_ATTR
{
    PCSTR           pszName;

    VMDIR_ATTR_TYPE type;

    union
    {
        PINT32   pData_int32;
        PUINT32  pData_uint32;
        PINT64   pData_int64;
        PUINT64  pData_uint64;
        PSTR*    ppszData;
        PSTR**   pppszStrArray;
        PBYTE*   ppData;
    } dataRef;

    size_t  size;

    PDWORD  pdwCount;

    BOOLEAN bOptional;

} VMDIR_ATTR, *PVMDIR_ATTR;


typedef struct _VMDIR_DIR_CONTEXT
{
    PVMDIR_BIND_INFO pBindInfo;
    LDAP*            pLd;

} VMDIR_DIR_CONTEXT, *PVMDIR_DIR_CONTEXT;

typedef struct _VMDIR_AUTH_PROVIDER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    uid_t peer_uid;
    gid_t peer_gid;
    pid_t peer_pid;

    VMDIR_DIR_CONTEXT  dirContext;

} VMDIR_AUTH_PROVIDER_CONTEXT, *PVMDIR_AUTH_PROVIDER_CONTEXT;


typedef struct _VMDIR_SASL_INFO
{
    PCSTR pszRealm;
    PCSTR pszAuthName;
    PCSTR pszUser;
    PCSTR pszPassword;
} VMDIR_SASL_INFO, *PVMDIR_SASL_INFO;


DWORD
VmDirLdapQueryObjects(
	LDAP*         pLd,
	PCSTR         pszBaseDN,
	int           scope,
	PCSTR         pszFilter,
	char**        attrs,
	int           sizeLimit,
	LDAPMessage** ppMessage
	);

DWORD
VmDirLdapGetValues(
	LDAP*        pLd,
	LDAPMessage* pMessage,
	PVMDIR_ATTR  pValueArray,
	DWORD        dwNumValues
	);

VOID
VmDirLdapFreeMessage(
	LDAPMessage* pMessage
	);

DWORD
VmDirLdapInitialize(
        PCSTR            pszURI,
        PCSTR            pszUPN,
        PCSTR            pszPassword,
        PCSTR            pszCachePath,
        LDAP**           ppLd
        );

VOID
VmDirLdapClose(
	LDAP* pLd
	);

DWORD
VmDirAllocLdapQueryMap(
    PSTR pszSearchBase,
    PVMDIRDB_LDAPQUERY_MAP *ppLdapMap
    );

/* Map SQL attribute(s) to LDAP attribute(s) */
DWORD
VmdirFindLdapAttributeList(
    PSTR *ppszAttributes,
    PSTR **pppszLdapAttributes,
    PDWORD *ppdwLdapAttributeTypes);

/* Map override attribute(s) to LDAP attribute(s) */
DWORD
VmdirFindLdapPwszAttributeList(
    PWSTR *ppwszAttributes,
    PSTR **pppszLdapAttributes,
    PDWORD *ppdwLdapAttributeTypes);

DWORD
VmDirFreeLdapQueryMap(
    PVMDIRDB_LDAPQUERY_MAP *ppLdapMap
    );


DWORD
VmDirAllocLdapAttributeMap(
    PVMDIRDB_LDAPATTR_MAP *ppAttrMap
    );

DWORD
VmdirFreeLdapAttributeMap(
    PVMDIRDB_LDAPATTR_MAP *ppAttrMap
    );

DWORD
VmdirFreeLdapAttributeList(
    PSTR **pppszLdapAttributes
    );

DWORD
VmDirFindLdapQueryMapEntry(
    PSTR pszSqlQuery,
    PVMDIRDB_LDAPQUERY_MAP_ENTRY *ppQueryMapEntry);

DWORD
VmdirDbAllocateEntriesAndAttributes(
    DWORD dwNumEntries,     /* Number of directory entries */
    PDWORD pdwAttributesCount,
    PDIRECTORY_ENTRY *ppDirectoryEntries);

DWORD
VmDirAttributeCopyEntry(
    PDIRECTORY_ATTRIBUTE out,
    PDIRECTORY_ATTRIBUTE in);

DWORD
VmDirConstructMachineDN(
    PSTR pszSqlDomainName,
    PSTR *ppszMachineAcctDN);

DWORD
VmDirConstructServicePrincipalName(
    PSTR pszSqlDomainName,
    PSTR pszServiceName,
    PSTR *ppszAcctSpn);

DWORD
VmDirConstructServicePrincipalNameEx(
    PSTR pszSqlDomainName,
    PSTR pszServiceName,
    BOOLEAN bUpperCaseHost,
    BOOLEAN bShortHost,
    PSTR *ppszAcctSpn);

DWORD
VmDirConstructMachineUPN(
    PSTR pszSqlDomainName,
    PSTR *ppszMachineAcctUpn);
