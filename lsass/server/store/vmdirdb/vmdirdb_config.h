#define VMDIR_REG_KEY "Services\\vmdir"

#define VMDIR_AUTH_PROVIDER_KEY "Services\\lsass\\Parameters\\Providers\\VmDir"

#define VMDIR_REG_KEY_BIND_PROTOCOL       "BindProtocol"
#define VMDIR_BIND_PROTOCOL_KERBEROS_STR  "kerberos"
#define VMDIR_BIND_PROTOCOL_SRP_STR       "srp"

#define VMDIR_REG_KEY_BIND_INFO_ACCOUNT   "dcAccount"
#define VMDIR_REG_KEY_BIND_INFO_BIND_DN   "dcAccountDN"
#define VMDIR_REG_KEY_BIND_INFO_PASSWORD  "dcAccountPassword"

#define VMAFD_REG_KEY "Services\\vmafd\\Parameters"

#define VMAFD_REG_KEY_DOMAIN_STATE "DomainState"
#define VMAFD_REG_KEY_DOMAIN_NAME  "DomainName"
#define VMAFD_REG_KEY_DC_NAME      "DCName"

#define VMDIR_KRB5_CC_NAME "FILE:/var/lib/likewise/krb5cc_vmdir_provider"


typedef struct _VMDIR_BIND_INFO
{
    LONG refCount;

    PSTR pszURI;
    PSTR pszUPN;
    PSTR pszPassword;
    PSTR pszDomainFqdn;
    PSTR pszDomainShort;
    PSTR pszSearchBase;

} VMDIR_BIND_INFO, *PVMDIR_BIND_INFO;

DWORD
VmDirGetBindProtocol(
    VMDIRDB_BIND_PROTOCOL* pBindProtocol
    );

DWORD
VmDirCreateBindInfo(
    PVMDIR_BIND_INFO* ppBindInfo
    );

PVMDIR_BIND_INFO
VmDirAcquireBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    );

VOID
VmDirReleaseBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    );
