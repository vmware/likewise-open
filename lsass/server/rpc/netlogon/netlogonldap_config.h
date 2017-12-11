#define NETLOGON_REG_KEY "Services\\vmdir"

#define VMDIR_AUTH_PROVIDER_KEY "Services\\lsass\\Parameters\\Providers\\VmDir"

#define NETLOGON_REG_KEY_BIND_PROTOCOL       "BindProtocol"
#define NETLOGON_LDAP_BIND_PROTOCOL_KERBEROS_STR  "kerberos"
#define NETLOGON_LDAP_BIND_PROTOCOL_SRP_STR       "srp"

#define NETLOGON_REG_KEY_BIND_INFO_ACCOUNT   "dcAccount"
#define NETLOGON_REG_KEY_BIND_INFO_BIND_DN   "dcAccountDN"
#define NETLOGON_REG_KEY_BIND_INFO_PASSWORD  "dcAccountPassword"

#define VMAFD_REG_KEY "Services\\vmafd\\Parameters"

#define VMAFD_REG_KEY_DOMAIN_STATE "DomainState"
#define VMAFD_REG_KEY_DOMAIN_NAME  "DomainName"
#define VMAFD_REG_KEY_DC_NAME      "DCName"

#define VMDIR_KRB5_CC_NAME "FILE:/var/lib/likewise/krb5cc_vmdir_provider"


typedef struct _NETLOGON_LDAP_BIND_INFO
{
    LONG refCount;

    PSTR pszURI;
    PSTR pszUPN;
    PSTR pszPassword;
    PSTR pszDomainFqdn;
    PSTR pszDomainShort;
    PSTR pszSearchBase;

} NETLOGON_LDAP_BIND_INFO, *PNETLOGON_LDAP_BIND_INFO;

DWORD
NetlogonGetBindProtocol(
    NETLOGON_LDAP_BIND_PROTOCOL* pBindProtocol
    );

DWORD
NetlogonCreateBindInfo(
    PNETLOGON_LDAP_BIND_INFO* ppBindInfo
    );


VOID
NetlogonReleaseBindInfo(
    PNETLOGON_LDAP_BIND_INFO pBindInfo
    );
