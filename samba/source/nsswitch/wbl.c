/* Editor settings: use real tab characters and display them as 4 spaces.
 * ex: set noexpandtab tabstop=4 shiftwidth=4: */
/* Winbind Logon (WBL) Library (aka "wobble")

   Copyright Danilo Almeida <dalmeida@centeris.com> 2007

   Based on pam_winbind.c:

   Copyright Andrew Tridgell <tridge@samba.org> 2000
   Copyright Tim Potter <tpot@samba.org> 2000
   Copyright Andrew Bartlett <abartlet@samba.org> 2002
   Copyright Guenther Deschner <gd@samba.org> 2005-2007
   Copyright Gerald (Jerry) Carter <jerry@samba.org> 2006-2007

   which is largely based on pam_userdb by Cristian Gafton <gafton@redhat.com>
   also contains large slabs of code from pam_unix by Elliot Lee <sopwith@redhat.com>
   (see copyright below for full details)
*/

#if defined(__hpux__) && defined(__LP64__) && defined(__hppa__)
#define _REENTRANT
#endif

#include "lib/replace/replace.h"
#include "wbl.h"
#include "wb_gp.h"

#include "system/time.h"

#include <iniparser.h>

#include "winbind_client.h"

#include <ctype.h>
#include <dirent.h>


#define DEFAULT_DAYS_TO_WARN_BEFORE_PWD_EXPIRES 14

#define SECONDS_PER_DAY 86400


/* from include/rpc_samr.h */
#define DOMAIN_PASSWORD_COMPLEX            0x00000001

#define REJECT_REASON_OTHER		0x00000000
#define REJECT_REASON_TOO_SHORT		0x00000001
#define REJECT_REASON_IN_HISTORY	0x00000002
#define REJECT_REASON_NOT_COMPLEX	0x00000005

/* from include/smb.h */
#define ACB_PWNOEXP			0x00000200

/* from include/rpc_netlogon.h */
#define LOGON_CACHED_ACCOUNT		0x00000004
#define LOGON_GRACE_LOGON		0x01000000
#define LOGON_KRB5_FAIL_CLOCK_SKEW	0x02000000



/* Flags below 0x1000 are for externally visible flags */

#define WBL_CONFIG_FLAG_INTERNAL_MASK         0xFFFFF000

#define WBL_CONFIG_FLAG_REQUIRED_MEMBERSHIP   0x00002000 // required_membership
#define WBL_CONFIG_FLAG_KRB5_AUTH             0x00004000
#define WBL_CONFIG_FLAG_KRB5_CCACHE_TYPE      0x00008000 // krb5_ccache_type

#define WBL_CONFIG_FLAG_CACHED_LOGIN          0x00010000
#define WBL_CONFIG_FLAG_CONFIG_FILE           0x00020000 // config_file

#define WBL_CONFIG_FLAG_CREATE_HOMEDIR        0x00040000
#define WBL_CONFIG_FLAG_SKEL_DIR              0x00080000 // skel_dir
#define WBL_CONFIG_FLAG_HOMEDIR_UMASK         0x00100000
#define WBL_CONFIG_FLAG_CREATE_K5LOGIN        0x00200000


typedef struct _WBL_CONFIG {
	WBL_CONFIG_FLAGS Flags;
	char* RequiredMembership;
	char* Krb5CCacheType;
	char* SkelDir;
	int HomedirUmask;
	int WarnPasswordChangeDays;
	dictionary* Dictionary;
} WBL_CONFIG;


typedef struct _WBL_PASSWD_INFO {
	char* pw_name;
	char* pw_passwd;
	uid_t pw_uid;
	gid_t pw_gid;
	char* pw_gecos;
	char* pw_dir;
	char* pw_shell;
	char buffer[1];
} WBL_PASSWD_INFO;


typedef uint32_t WBL_STATE_FLAGS;

#define WBL_STATE_FLAG_AUTHENTICATED        0x00000001
#define WBL_STATE_FLAG_AUTHORIZED           0x00000002
#define WBL_STATE_FLAG_UNAUTHORIZED         0x00000004
#define WBL_STATE_FLAG_POLICY_INFO          0x00000008
#define WBL_STATE_FLAG_NEXT_CHANGE_TIME     0x00000010
#define WBL_STATE_FLAG_PASSWORD_CHANGED     0x00000020

#define WBL_STATE_FLAG_RESET_LOGON_MASK \
( \
	WBL_STATE_FLAG_AUTHENTICATED | \
	WBL_STATE_FLAG_AUTHORIZED | \
	WBL_STATE_FLAG_UNAUTHORIZED | \
	WBL_STATE_FLAG_POLICY_INFO | \
	WBL_STATE_FLAG_NEXT_CHANGE_TIME | \
	0 \
)

struct _WBL_STATE {
	WBL_CONFIG* Config;
	void* Context;
	WBL_LOG_CALLBACK LogFunction;
	WBL_LOG_CALLBACK MessageFunction;
	WBL_STATUS TrustDomStatus;
	WBL_STATUS PasswdInfoStatus;
	WBL_STATUS AuthenticateStatus;
	WBL_STATUS ChangePasswordStatus;
	WBL_STATUS ChangePasswordRejectStatus;
	WBL_STATUS PrincipalNameStatus;
	WBL_STATE_FLAGS Flags;
	char *Krb5ConfigPath;
	char* TrustDomResult;
	char* Username;
	char* OriginalUsername;
	char* RequiredMembershipSids;
	char* PrincipalName;
	struct {
		WBL_PASSWORD_POLICY_INFO PasswordPolicy;
		time_t PasswordLastSetTime;
		time_t PasswordMustChangeTime;
		time_t NextPasswordChangeTime;
		uint32_t AccountFlags;
		uint32_t UserFlags;
		char* CanonicalUserName;
		char* UserPrincipalName;
		char* HomeDirectory;
		char* LogonScript;
		char* LogonServer;
		char* ProfilePath;
		char* Krb5CCacheName;
	} AuthInfo;
	WBL_PASSWD_INFO* PasswdInfo;
};

/* TODO: Set up assert macro */
#define WBL_ASSERT(x)
#define WBL_XOR(x, y) (!(x) ^ !(y))

#define WBL_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define WBL_MIN(x, y) (((x) < (y)) ? (x) : (y))

#define IS_NULL_OR_EMPTY_STR(str) (!(str) || !((str)[0]))
#define SAFE_LOG_STR(x) ((x) ? (x) : "(null)")

#define GOTO_CLEANUP() \
	do { \
		goto cleanup; \
	} while (0)

#define GOTO_CLEANUP_EE(EE) \
	do { \
		EE = __LINE__; \
		GOTO_CLEANUP(); \
	} while (0)

#define GOTO_CLEANUP_ON_WBL_STATUS(status) \
	do { \
		if (status) { \
			GOTO_CLEANUP(); \
		} \
	} while (0)

#define GOTO_CLEANUP_ON_WBL_STATUS_EE(status, EE) \
	do { \
		if (status) { \
			GOTO_CLEANUP_EE(EE); \
		} \
	} while (0)

#define SetFlag(Flags, Values) \
	((Flags) |= (Values))

#define TestFlag(Flags, Values) \
	((Flags) & (Values))

#define TestFlagAll(Flags, Values) \
	(((Flags) & (Values)) == (Values))

#define ClearFlag(Flags, Values) \
	((Flags) &= ~(Values))

#define IsNoCaseEqual(x, y) (strcasecmp(x, y) == 0)
#define IsNoCaseEqualN(x, y) (strncasecmp(x, y, strlen(y)) == 0)
#define IsNoCaseEqualStaticN(x, y) (strncasecmp(x, y, sizeof(y)-1) == 0)

#define WBL_FREE(Pointer) SAFE_FREE(Pointer)

/* TODO: Make use of DEBUG_PASSWORD */
#ifdef DEBUG_PASSWORD
#define _LOG_PASSWORD_AS_STRING 1
#else
#define _LOG_PASSWORD_AS_STRING 0
#endif

static
WBL_STATUS
WblpCopyTree(
	WBL_STATE* State,
	const char* SourcePath,
	const char* TargetPath,
	uid_t uid,
	gid_t gid
	);

static bool WblpIsSidString(const char* name)
{
	const char* p;

	if (strncmp("S-", name, 2)) {
		return false;
	}

	for (p = name + 2; *p; p++) {
		if (!(('-' == *p) || isdigit(*p))) {
			return false;
		}
	}

	return true;
}

#define UPN_SEPARATOR '@'

static bool WblpIsUpn(const char* Name)
{
	return (strchr(Name, UPN_SEPARATOR) != NULL);
}

const char*
WblStatusToString(
	WBL_STATUS Status
	)
{
	/* TODO: Use these?  Perhaps expose some of these?  Or just use these error messages. */
#if 0
static const struct ntstatus_errors {
	const char *ntstatus_string;
	const char *error_string;
} ntstatus_errors[] = {
	{"NT_STATUS_OK", "Success"},
	{"NT_STATUS_BACKUP_CONTROLLER", "No primary Domain Controler available"},
	{"NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND", "No domain controllers found"},
	{"NT_STATUS_NO_LOGON_SERVERS", "No logon servers"},
	{"NT_STATUS_PWD_TOO_SHORT", "Password too short"},
	{"NT_STATUS_PWD_TOO_RECENT", "The password of this user is too recent to change"},
	{"NT_STATUS_PWD_HISTORY_CONFLICT", "Password is already in password history"},
	{"NT_STATUS_PASSWORD_EXPIRED", "Your password has expired"},
	{"NT_STATUS_PASSWORD_MUST_CHANGE", "You need to change your password now"},
	{"NT_STATUS_INVALID_WORKSTATION", "You are not allowed to logon from this workstation"},
	{"NT_STATUS_INVALID_LOGON_HOURS", "You are not allowed to logon at this time"},
	{"NT_STATUS_ACCOUNT_EXPIRED", "Your account has expired. Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_ACCOUNT_DISABLED", "Your account is disabled. Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_ACCOUNT_LOCKED_OUT", "Your account has been locked. Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT", "Invalid Trust Account"},
	{"NT_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT", "Invalid Trust Account"},
	{"NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT", "Invalid Trust Account"},
	{"NT_STATUS_ACCESS_DENIED", "Access is denied"},
	{NULL, NULL}
};
#endif
	switch (Status)
	{
	case WBL_STATUS_OK:
		return "Success";
	case WBL_STATUS_ERROR:
		return "Error";
	case WBL_STATUS_INVALID_STATE:
		return "Inconsistent state detected";
	case WBL_STATUS_ACCOUNT_DISABLED:
		return "The account has been disabled";
	case WBL_STATUS_ACCOUNT_EXPIRED:
		return "The account has expired";
	case WBL_STATUS_ACCOUNT_UNKNOWN:
		return "There is no such user";
	case WBL_STATUS_ACCOUNT_LOCKED_OUT:
		return "The account has been automatically locked out due to too many invalid attempts to logon or change the password";
	case WBL_STATUS_ACCOUNT_INVALID:
		return "The account name is not properly formed";

	case WBL_STATUS_LOGON_BAD:
		return "Logon failed due to bad username or password";
	case WBL_STATUS_PASSWORD_EXPIRED:
		return "The password has expired";
	case WBL_STATUS_PASSWORD_MUST_CHANGE:
		return "The password must be changed before the first logon";

	case WBL_STATUS_PASSWORD_INVALID:
		return "The new password contains disallowed values";
	case WBL_STATUS_PASSWORD_WRONG:
		return "The provided password does not match the current password";
	case WBL_STATUS_PASSWORD_RESTRICTION:
		return "The new password does not satisfy the current password rules";
	case WBL_STATUS_PASSWORD_TOO_RECENT:
		return "The password has been changed too recently to comply with the password policy which disallows frequent password changes";
	case WBL_STATUS_PASSWORD_TOO_SHORT:
		return "The new password is too short to comply with the minimum password length policy";
	case WBL_STATUS_PASSWORD_IN_HISTORY:
		return "The password policy does not allow a password that has been recently used";
	case WBL_STATUS_PASSWORD_NOT_COMPLEX:
		return "The password does not meet complexity requirements";

	case WBL_STATUS_MEMORY_INSUFFICIENT:
		return "There is not enough memory";
	case WBL_STATUS_BUFFER_INSUFFICIENT:
		return "The buffer provided is too small";
	case WBL_STATUS_SERVER_UNAVAILABLE:
		return "No servers are available to service the logon request";
	case WBL_STATUS_ACCESS_DENIED:
		return "Access denied";

	case WBL_STATUS_LOGON_RESTRICTED_ACCOUNT:
		return "Logon by this account is restricted";
	case WBL_STATUS_LOGON_RESTRICTED_COMPUTER:
		return "Logon  by this account from this computer is restricted";
	case WBL_STATUS_LOGON_RESTRICTED_TIME:
		return "Logon by this account at this time is restricted";
	case WBL_STATUS_LOGON_TYPE_DENIED:
		return "The requested logon type is not permitted by policy";

	default:
		return "UNKNOWN";
	}
}

#if 0
/* TODO: Move this to daemon */
static
WBL_STATUS
WblpStatusFromNtStatus(
	NTSTATUS Status
	)
{
	WBL_STATUS result;

	switch (Status)
	{
	case NT_STATUS_NO_LOGON_SERVERS: /* No logon servers */
	case NT_STATUS_BACKUP_CONTROLLER: /* No primary Domain Controler available */
	case NT_STATUS_TRUSTED_DOMAIN_FAILURE:
	case NT_STATUS_TRUSTED_RELATIONSHIP_FAILURE:
	case NT_STATUS_NETLOGON_NOT_STARTED:
	case NT_STATUS_DOMAIN_TRUST_INCONSISTENT:
	case NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND: /* No domain controllers found */
	/* not sure about these: */
	case NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT: /* Invalid Trust Account */
	case NT_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT: /* Invalid Trust Account */
	case NT_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT: /* Invalid Trust Account */
		/* No servers are available to service the logon request. */
		result = WBL_STATUS_SERVER_UNAVAILABLE;
		break;

	case NT_STATUS_NO_SUCH_USER:
		/* There is no such user. */
		result = WBL_STATUS_ACCOUNT_UNKNOWN;
		break;

	case NT_STATUS_WRONG_PASSWORD:
		/* The provided password does not match the current password. */
		result = WBL_STATUS_PASSWORD_WRONG;
		break;

	case NT_STATUS_INVALID_ACCOUNT_NAME:
		/* The account name is not properly formed. */
		result = WBL_STATUS_ACCOUNT_INVALID;
		break;

	case NT_STATUS_ILL_FORMED_PASSWORD:
		/* The new password contains disallowed values. */
		result = WBL_STATUS_PASSWORD_INVALID;
		break;

	case NT_STATUS_PASSWORD_RESTRICTION:
		/* The new password does not satisfy the current password rules. */
		result = WBL_STATUS_PASSWORD_RESTRICTION;
		break;

	case NT_STATUS_LOGON_FAILURE:
		/* Logon failed due to bad username or password. */
		result = WBL_STATUS_LOGON_BAD;
		break;

	case NT_STATUS_NO_MEMORY:
		/* Not enough memory. */
		result = WBL_STATUS_MEMORY_INSUFFICIENT;
		break;

	case NT_STATUS_ACCOUNT_RESTRICTION:
		/* Logon by this account is restricted. */
		result = WBL_STATUS_LOGON_RESTRICTED_ACCOUNT;
		break;

	case NT_STATUS_INVALID_WORKSTATION:
		/* Logon  by this account from this computer is restricted. */
		result = WBL_STATUS_LOGON_RESTRICTED_COMPUTER;
		break;

	case NT_STATUS_INVALID_LOGON_HOURS:
		/* Logon by this account at this time is restricted. */
		result = WBL_STATUS_LOGON_RESTRICTED_TIME;
		break;

	case NT_STATUS_PASSWORD_EXPIRED:
		/* The password has expired. */
		result = WBL_STATUS_PASSWORD_EXPIRED;
		break;

	case NT_STATUS_ACCOUNT_DISABLED:
		/* The account has been disabled. */
		result = WBL_STATUS_ACCOUNT_DISABLED;
		break;

	case NT_STATUS_PASSWORD_MUST_CHANGE:
		/* The password must be changed before the first logon. */
		result = WBL_STATUS_PASSWORD_MUST_CHANGE;
		break;

	case NT_STATUS_ACCOUNT_EXPIRED:
		/* The account has expired. */
		result = WBL_STATUS_ACCOUNT_EXPIRED;
		break;

	case NT_STATUS_ACCOUNT_LOCKED_OUT:
		/* The account has been automatically locked out due to too many invalid attempts to logon or change the password. */
		result = WBL_STATUS_ACCOUNT_LOCKED_OUT;
		break;

	/* Password restriction sub-cases: */
	case NT_STATUS_PWD_TOO_SHORT:
		/* The password is too short.  The password policy does not allow a password to be so short. */
		result = WBL_STATUS_PASSWORD_TOO_SHORT;
		break;

	case NT_STATUS_PWD_TOO_RECENT:
		/* The password policy does not allow a password to change too frequently. */
		result = WBL_STATUS_PASSWORD_TOO_RECENT;
		break;

	case NT_STATUS_PWD_HISTORY_CONFLICT:
		/* The password policy does not allow a password that has been recently used. */
		result = WBL_STATUS_PASSWORD_IN_HISTORY;
		break;

	case NT_STATUS_LOGON_NOT_GRANTED:
	case NT_STATUS_LOGON_TYPE_NOT_GRANTED:
		/* The requested logon type is not permitted by policy. */
		result = WBL_STATUS_LOGON_TYPE_DENIED;
		break;

	case NT_STATUS_ACCESS_DENIED:
		result = WBL_STATUS_ACCESS_DENIED;
		break;

	default:
		/* Operation failed */
		result = WBL_STATUS_ERROR;
		break;
	}

	return result;
}
#endif

static
void
WblpMessageV(
	WBL_STATE* State,
	WBL_LOG_LEVEL Level,
	const char* Format,
	va_list Args
	)
{
	if (!TestFlag(WblStateGetConfigFlags(State), WBL_CONFIG_FLAG_SILENT)) {
		State->MessageFunction(State->Context, Level, Format, Args);
	}
}

static
void
WblpMessage(
	WBL_STATE* State,
	WBL_LOG_LEVEL Level,
	const char* Format,
	...
	)
{
	va_list args;
	va_start(args, Format);
	WblpMessageV(State, Level, Format, args);
	va_end(args);
}

static
void
WblpLogV(
	WBL_STATE* State,
	WBL_LOG_LEVEL Level,
	const char* Format,
	va_list Args
	)
{
	State->LogFunction(State->Context, Level, Format, Args);
}

static
void
WblpLog(
	WBL_STATE* State,
	WBL_LOG_LEVEL Level,
	const char* Format,
	...
	)
{
	va_list args;
	va_start(args, Format);
	WblpLogV(State, Level, Format, args);
	va_end(args);
}

static
WBL_STATUS
WblpAllocate(
	size_t Size,
	void** Pointer
	)
{
	*Pointer = malloc(Size);
	if (!*Pointer) {
		return WBL_STATUS_MEMORY_INSUFFICIENT;
	}
	memset(*Pointer, 0, Size);
	return WBL_STATUS_OK;
}

static
WBL_STATUS
WblpStrDup(
	char** Duplicate,
	const char* Original
	)
{
	if (!Original) {
		*Duplicate = NULL;
	} else {
		*Duplicate = strdup(Original);
		if (!*Duplicate) {
			return WBL_STATUS_MEMORY_INSUFFICIENT;
		}
	}
	return WBL_STATUS_OK;
}

static
bool
WblpParseStringOptionValueN(
	WBL_STATE* State,
	const char* Arg,
	const char* OptionName,
	size_t OptionNameLength,
	const char** Value
	)
{
	bool found = false;

	if (!strncasecmp(Arg, OptionName, OptionNameLength)) {
		char *p = strchr(Arg + OptionNameLength, '=');
		if (!p) {
			WblpLog(State, WBL_LOG_LEVEL_INFO, "No \"=\" delimiter for \"%s\" found", OptionName);
			goto cleanup;
		}
		found = true;
		if (!*Value) {
			*Value = p + 1;
			WblpLog(State, WBL_LOG_LEVEL_DEBUG, "PAM config: %s '%s'", OptionName, *Value);
		}
	}

cleanup:
	return found;
}

#define WblpParseStringOptionValueStatic(State, Arg, OptionNameStatic, Value) \
	WblpParseStringOptionValueN(State, Arg, OptionNameStatic, sizeof(OptionNameStatic)-1, Value)

static
void
WblpGetStringOptionValue(
	WBL_STATE* State,
	dictionary* IniDictionary,
	const char* IniKey,
	const char** Value
	)
{
	if (!*Value && IniDictionary) {
		*Value = iniparser_getstr(IniDictionary, IniKey);
		if (*Value) {
			WblpLog(State, WBL_LOG_LEVEL_DEBUG, "PAM config: %s '%s'", IniKey, *Value);
		}
	}
}

static
WBL_STATUS
WblpDictionarySetKey(
	dictionary* Dictionary,
	const char* Key,
	const char* Value
	)
{
	WBL_STATUS status;
	const char* result;

	dictionary_set(Dictionary, (char*) Key, (char*) Value);
	result = dictionary_get(Dictionary, (char*) Key, NULL);
	if(result != Value &&
			(result == NULL || Value == NULL || strcmp(result, Value)))
		status = WBL_STATUS_MEMORY_INSUFFICIENT;
	else
		status = WBL_STATUS_OK;
	return status;
}

static
WBL_STATUS
WblpGetGpCombinedMembershipRequirement(
	WBL_STATE* State,
	const char *local_membership,
	char **combined_membership
	)
{
	WBL_STATUS status;
	char *combined = NULL;
	char *gp_membership = NULL;
	size_t gp_len;
	size_t local_len;
	size_t len;


	/*
	 * Check with Identity GPAgent to see if we have logon restrictions to also add to list.
	 */
	if (!gp_get_interactive_logon_rights(&gp_membership)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "failed to get GP info");
#if 0  /* FIXME!! ignore the error for now  --jerry */
		status = WBL_STATUS_ERROR;
		goto cleanup;
#endif
	}

	if (!gp_membership) {
		gp_len = 0;
	} else {
		gp_len = strlen(gp_membership);
	}
	if (gp_len < 1) {
		if (!local_membership) {
			status = WBL_STATUS_OK;
			goto cleanup;
		}
	status = WblpStrDup(&combined, local_membership);
		/* Ok on success, otherwise an error, done in any case */
		goto cleanup;
	}

	local_len = local_membership ? strlen(local_membership) : 0;
	len = (local_len > 0) ? (local_len + 1) : 0;
	len += gp_len + 1;

	status = WblpAllocate(len, (void**)&combined);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (local_len > 0) {
		strcpy(combined, local_membership);
		strcat(combined, ",");
	} else {
		combined[0] = 0;
	}
	strcat(combined, gp_membership);

	status = WBL_STATUS_OK;

cleanup:
	if (status) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "failed to get combined membership requirement");
	} else if (combined) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "requiring membership: \"%s\"", combined);
	}

	if (gp_membership) {
		gp_free_buffer(gp_membership);
	}

	if (status) {
		WBL_FREE(combined);
	}

	*combined_membership = combined;

	return status;
}

static
void
WblpConfigRelease(
	WBL_CONFIG* Config
	)
{
	if (Config) {
		if (Config->Dictionary) {
			dictionary_del(Config->Dictionary);
		}
		WBL_FREE(Config->RequiredMembership);
		WBL_FREE(Config->Krb5CCacheType);
		WBL_FREE(Config->SkelDir);
		WBL_FREE(Config);
	}
}

static
WBL_STATUS
WblpConfigParse(
	WBL_STATE* State,
	WBL_CONFIG** Config,
	const char** AdditionalKeys,
	const char* DefaultConfigPath,
	int argc,
	const char** argv
	)
{
	WBL_STATUS status;
	WBL_CONFIG_FLAGS flags = 0;
	WBL_CONFIG* config = NULL;
	const char* config_file = NULL;
	const char* krb5_ccache_type = NULL;
	const char* required_membership = NULL;
	const char* skel_dir = NULL;
	const char* homedir_umask_string = NULL;
	int homedir_umask = 0022;
	int warn_password_expire_days = DEFAULT_DAYS_TO_WARN_BEFORE_PWD_EXPIRES;
	int i;
	dictionary* d = NULL;
	char* combined_required_membership = NULL;
	dictionary* extra_keys = NULL;

	if (AdditionalKeys) {
		extra_keys = dictionary_new(0);
		if (!extra_keys) {
			status = WBL_STATUS_MEMORY_INSUFFICIENT;
			GOTO_CLEANUP();
		}
	}

	/* argv-type options are a subset of the config file options */
	for (i = 0 ; i < argc; i++) {
		/* Note that config option work differently from others */
		if (IsNoCaseEqualStaticN(argv[i], "config")) {
			if (i < (argc-1)) {
				config_file = argv[i+1];
				i++;
			} else {
				WblpLog(State, WBL_LOG_LEVEL_ERROR, "Missing config file");
			}
		} else if (WblpParseStringOptionValueStatic(State, argv[i], "require_membership_of", &required_membership)) {
		} else if (WblpParseStringOptionValueStatic(State, argv[i], "require-membership-of", &required_membership)) {
		} else if (WblpParseStringOptionValueStatic(State, argv[i], "krb5_ccache_type", &krb5_ccache_type)) {
		} else if (!strcasecmp(argv[i], "debug")) {
			SetFlag(flags, WBL_CONFIG_FLAG_DEBUG);
		} else if (!strcasecmp(argv[i], "debug_state")) {
			SetFlag(flags, WBL_CONFIG_FLAG_DEBUG_STATE);
		} else if (!strcasecmp(argv[i], "silent")) {
			SetFlag(flags, WBL_CONFIG_FLAG_SILENT);
		} else if (!strcasecmp(argv[i], "use_authtok")) {
			SetFlag(flags, WBL_CONFIG_FLAG_USE_AUTHTOK);
		} else if (!strcasecmp(argv[i], "use_first_pass")) {
			SetFlag(flags, WBL_CONFIG_FLAG_USE_FIRST_PASS);
		} else if (!strcasecmp(argv[i], "try_first_pass")) {
			SetFlag(flags, WBL_CONFIG_FLAG_TRY_FIRST_PASS);
		} else if (!strcasecmp(argv[i], "unknown_ok")) {
			SetFlag(flags, WBL_CONFIG_FLAG_UNKNOWN_OK);
		} else if (!strcasecmp(argv[i], "remember_chpass")) {
			SetFlag(flags, WBL_CONFIG_FLAG_REMEMBER_CHPASS);
		} else if (!strcasecmp(argv[i], "krb5_auth")) {
			SetFlag(flags, WBL_CONFIG_FLAG_KRB5_AUTH);
		} else if (!strcasecmp(argv[i], "cached_login")) {
			SetFlag(flags, WBL_CONFIG_FLAG_CACHED_LOGIN);
		} else if (!strcasecmp(argv[i], "create_homedir")) {
			SetFlag(flags, WBL_CONFIG_FLAG_CREATE_HOMEDIR);
		} else if (!strcasecmp(argv[i], "create_k5login")) {
			SetFlag(flags, WBL_CONFIG_FLAG_CREATE_K5LOGIN);
		} else {
			bool found = false;
			if (AdditionalKeys) {
				const char* key = NULL;
				const char* value = NULL;
				int keyIndex;
				for (keyIndex = 0; key = AdditionalKeys[keyIndex]; keyIndex++) {
					if (WblpParseStringOptionValueStatic(State, argv[i], key, &value)) {
						found = true;
						status = WblpDictionarySetKey(extra_keys, key, value);
						GOTO_CLEANUP_ON_WBL_STATUS(status);
						break;
					}
				}
			}
			if (!found) {
				WblpLog(State, WBL_LOG_LEVEL_ERROR, "Unknown arg-style option: %s", argv[i]);
				/* Apparently, this used to be a critical error, so
				   keep it as such. */
				status = WBL_STATUS_ERROR;
				goto cleanup;
			}
		}
	}

	if (!config_file) {
		config_file = DefaultConfigPath;
	}

	/* Unfortunately, there is no good way to distinguish errors */
	d = iniparser_load(config_file);
	if (d) {
		/* Strings */
		WblpGetStringOptionValue(State, d, "global:require_membership_of", &required_membership);
		WblpGetStringOptionValue(State, d, "global:require-membership-of", &required_membership);
		WblpGetStringOptionValue(State, d, "global:krb5_ccache_type", &krb5_ccache_type);
		WblpGetStringOptionValue(State, d, "global:skel", &skel_dir);
		WblpGetStringOptionValue(State, d, "global:umask", &homedir_umask_string);

		if (AdditionalKeys) {
			const char* key = NULL;
			const char* value = NULL;
			int keyIndex;
			for (keyIndex = 0; key = AdditionalKeys[keyIndex]; keyIndex++) {
				char* useKey = NULL;
				if (asprintf(&useKey, "global:%s", key) <= 0) {
					status = WBL_STATUS_MEMORY_INSUFFICIENT;
					GOTO_CLEANUP();
				}

				WblpGetStringOptionValue(State, d, useKey, &value);
				free(useKey);

				status = WblpDictionarySetKey(extra_keys, key, value);
				GOTO_CLEANUP_ON_WBL_STATUS(status);
			}
		}

		/* Booleans */
		if (iniparser_getboolean(d, "global:silent", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_SILENT);
		}
		if (iniparser_getboolean(d, "global:debug", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_DEBUG);
		}
		if (iniparser_getboolean(d, "global:debug_state", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_DEBUG_STATE);
		}
		if (iniparser_getboolean(d, "global:try_first_pass", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_TRY_FIRST_PASS);
		}
		if (iniparser_getboolean(d, "global:krb5_auth", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_KRB5_AUTH);
		}
		if (iniparser_getboolean(d, "global:cached_login", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_CACHED_LOGIN);
		}
		if (iniparser_getboolean(d, "global:create_homedir", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_CREATE_HOMEDIR);
		}
		if (iniparser_getboolean(d, "global:create_k5login", false)) {
			SetFlag(flags, WBL_CONFIG_FLAG_CREATE_K5LOGIN);
		}

		/* Integers with defaults */
		warn_password_expire_days = iniparser_getint(d, "global:warn_pwd_expire", DEFAULT_DAYS_TO_WARN_BEFORE_PWD_EXPIRES);
	}

	if (homedir_umask_string) {
		homedir_umask = (int)strtol(homedir_umask_string, NULL, 0);
	}

	status = WblpGetGpCombinedMembershipRequirement(State, required_membership, &combined_required_membership);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpAllocate(sizeof(*config), (void**)&config);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	config->Dictionary = extra_keys;
	extra_keys = NULL;
	config->Flags = flags;
	config->WarnPasswordChangeDays = warn_password_expire_days;
	config->HomedirUmask = homedir_umask;
	config->RequiredMembership = combined_required_membership;
	combined_required_membership = NULL;

	status = WblpStrDup(&config->Krb5CCacheType, krb5_ccache_type);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStrDup(&config->SkelDir, skel_dir);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

cleanup:
	WBL_FREE(combined_required_membership);
	if (d) {
		iniparser_freedict(d);
	}
	if (extra_keys) {
		dictionary_del(extra_keys);
	}


	if (status) {
		if (config) {
			WblpConfigRelease(config);
		}
	}

	*Config = config;
	return status;
}

WBL_STATUS
WblStateCreate(
	/* OUT */ WBL_STATE** State,
	/* IN */ WBL_LOG_CALLBACK LogFunction,
	/* IN */ WBL_LOG_CALLBACK MessageFunction,
	/* OPTIONAL IN */ void* Context,
	/* OPTIONAL IN */ const char** AdditionalKeys,
	/* IN */ const char* DefaultConfigPath,
	/* IN */ int argc,
	/* IN */ const char** argv
	)
{
	WBL_STATUS status;
	WBL_STATE* state = NULL;

	if (!LogFunction) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	if (!MessageFunction) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	status = WblpAllocate(sizeof(*state), (void**)&state);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	state->Context = Context;
	state->LogFunction = LogFunction;
	state->MessageFunction = MessageFunction;

	status = WblpConfigParse(state, &state->Config, AdditionalKeys, DefaultConfigPath, argc, argv);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WBL_STATUS_OK;

cleanup:
	if (status) {
		if (state) {
			WblStateRelease(state);
			state = NULL;
		}
	}

	*State = state;

	return status;
}

static
void
WblpStateAuthInfoPasswordPolicyFromResponse(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ struct winbindd_response* Response
	)
{
	SetFlag(State->Flags, WBL_STATE_FLAG_POLICY_INFO);
	State->AuthInfo.PasswordPolicy.ExpireTime = Response->data.auth.policy.expire;
	State->AuthInfo.PasswordPolicy.MinimumAge = Response->data.auth.policy.min_passwordage;
	State->AuthInfo.PasswordPolicy.MinimumLength = Response->data.auth.policy.min_length_password;
	State->AuthInfo.PasswordPolicy.History = Response->data.auth.policy.password_history;
	State->AuthInfo.PasswordPolicy.Complexity = TestFlag(Response->data.auth.policy.password_properties, DOMAIN_PASSWORD_COMPLEX) ? true : false;
}

static
void
WblpStateAuthInfoCleanup(
	/* IN OUT */ WBL_STATE* State
	)
{
	WBL_FREE(State->AuthInfo.CanonicalUserName);
	WBL_FREE(State->AuthInfo.UserPrincipalName);
	WBL_FREE(State->AuthInfo.HomeDirectory);
	WBL_FREE(State->AuthInfo.LogonScript);
	WBL_FREE(State->AuthInfo.LogonServer);
	WBL_FREE(State->AuthInfo.ProfilePath);
	WBL_FREE(State->AuthInfo.Krb5CCacheName);
	ClearFlag(State->Flags, WBL_STATE_FLAG_POLICY_INFO);
}

static
WBL_STATUS
WblpStateAuthInfoFromResponse(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ struct winbindd_response* Response
	)
{
	WBL_STATUS status;

	WblpStateAuthInfoCleanup(State);

	status = WblpStrDup(&State->AuthInfo.CanonicalUserName, Response->extra_data.data);
	GOTO_CLEANUP_ON_WBL_STATUS(status);
	status = WblpStrDup(&State->AuthInfo.UserPrincipalName, Response->data.auth.krb5upn);
	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Received UPN of: %s %s", Response->data.auth.krb5upn, State->AuthInfo.UserPrincipalName);
	GOTO_CLEANUP_ON_WBL_STATUS(status);
	status = WblpStrDup(&State->AuthInfo.HomeDirectory, Response->data.auth.info3.home_dir);
	GOTO_CLEANUP_ON_WBL_STATUS(status);
	status = WblpStrDup(&State->AuthInfo.LogonScript, Response->data.auth.info3.logon_script);
	GOTO_CLEANUP_ON_WBL_STATUS(status);
	status = WblpStrDup(&State->AuthInfo.LogonServer, Response->data.auth.info3.logon_srv);
	GOTO_CLEANUP_ON_WBL_STATUS(status);
	status = WblpStrDup(&State->AuthInfo.ProfilePath, Response->data.auth.info3.profile_path);
	GOTO_CLEANUP_ON_WBL_STATUS(status);
	status = WblpStrDup(&State->AuthInfo.Krb5CCacheName, Response->data.auth.krb5ccname);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	State->AuthInfo.AccountFlags = Response->data.auth.info3.acct_flags;
	State->AuthInfo.UserFlags = Response->data.auth.info3.user_flgs;

	State->AuthInfo.PasswordLastSetTime = Response->data.auth.info3.pass_last_set_time;
	State->AuthInfo.PasswordMustChangeTime = Response->data.auth.info3.pass_must_change_time;

	WblpStateAuthInfoPasswordPolicyFromResponse(State, Response);

cleanup:
	if (status) {
		WblpStateAuthInfoCleanup(State);
	}
	return status;
}

void
WblStateRelease(
	/* IN OUT */ WBL_STATE* State
	)
{
	if (State) {
		WBL_FREE(State->TrustDomResult);
		WBL_FREE(State->Krb5ConfigPath);
		WBL_FREE(State->Username);
		WBL_FREE(State->OriginalUsername);
		WBL_FREE(State->RequiredMembershipSids);
		WblpStateAuthInfoCleanup(State);
		WBL_FREE(State->PasswdInfo);
		WblpConfigRelease(State->Config);
		State->Config = NULL;
	}
}

bool
WblPing(
	/* IN OUT */ WBL_STATE* State
	)
{
	NSS_STATUS nssStatus;

	nssStatus = winbindd_request_response(WINBINDD_PING, NULL, NULL);
	if (nssStatus != NSS_STATUS_SUCCESS) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "cannot contact daemon");
		return false;
	}
	return true;
}

static
WBL_STATUS
WblpStatusFromResponse(
	WBL_STATE* State,
	struct winbindd_response* Response
	)
{
	if (Response->result == WINBINDD_OK) {
		return WBL_STATUS_OK;
	} else {
		/* Default to generic error */
		return WBL_STATUS_ERROR;
	}
}

static
WBL_STATUS
WblpStatusFromLookupResponse(
	WBL_STATE* State,
	struct winbindd_response* Response
	)
{
	WBL_STATUS status;

	status = WblpStatusFromResponse(State, Response);
	if (status) {
		if (Response->data.auth.nt_status_string[0]) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "request failed, NT error was %s", Response->data.auth.nt_status_string);
		} else {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "request failed");
		}
		/* TODO: Unfortunately, we cannot distinguish the user not existing
		   versus some other kind of error.  We would need to tweak
		   the winbind interface to get that kind of information. */
		status = WBL_STATUS_ACCOUNT_UNKNOWN;
	}
	return status;
}

static
WBL_STATUS
WblpStatusFromAuthResponse(
	WBL_STATE* State,
	struct winbindd_response* Response
	)
{
	WBL_STATUS status;

	status = WblpStatusFromResponse(State, Response);
	if (status) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR,
			"request failed: %s, WBL error was %s (%d), NT error was %s, PAM error %d",
			 Response->data.auth.error_string,
			 WblStatusToString(Response->data.auth.wbl_status),
			 Response->data.auth.wbl_status,
			 Response->data.auth.nt_status_string,
			 Response->data.auth.pam_error);
		if (Response->data.auth.wbl_status) {
			status = Response->data.auth.wbl_status;
		} else {
			/* This really should not happen */
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "request failed, but WBL error is 0!");
		}
	}

	return status;
}

static
WBL_STATUS
WblpWinbindRequest(
	WBL_STATE* State,
	enum winbindd_cmd req_type,
	struct winbindd_request *request,
	struct winbindd_response *response
	)
{
	WBL_STATUS status;
	NSS_STATUS nssStatus;

	/* Fill in request and send down pipe */
	winbindd_init_request(request, req_type);

	/* We need to make sure to get the error response back */
	nssStatus = winbindd_request_error_response(req_type, request, response);
	/* We are done with the socket - close it and avoid mischief */
	winbind_close_sock();
	if (nssStatus != NSS_STATUS_SUCCESS) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	status = WBL_STATUS_OK;

cleanup:
	return status;
}

#if 0
/* TODO: Use logging function? */
static
void
WblpLogUserRequestStatus(
	WBL_STATE* State,
	enum winbindd_cmd req_type,
	const char *user,
	WBL_STATUS status
	)
{
	switch (status) {
	case WBL_STATUS_LOGON_BAD:
		/* incorrect password */
		WblpLog(State, WBL_LOG_LEVEL_WARN, "user '%s' denied access (incorrect password or invalid membership)", user);
		break;
	case WBL_STATUS_ACCOUNT_EXPIRED:
		/* account expired */
		WblpLog(State, WBL_LOG_LEVEL_WARN, "user '%s' account expired", user);
		break;
	case WBL_STATUS_PASSWORD_EXPIRED:
		/* password expired */
		WblpLog(State, WBL_LOG_LEVEL_WARN, "user '%s' password expired", user);
		break;
	case WBL_STATUS_PASSWORD_MUST_CHANGE:
		/* new password required */
		WblpLog(State, WBL_LOG_LEVEL_WARN, "user '%s' new password required", user);
		break;
	case WBL_STATUS_ACCOUNT_UNKNOWN:
		/* the user does not exist */
		WblpLog(State, WBL_LOG_LEVEL_NOTICE, "user '%s' not found", user);
#if 0
		/* TODO: Make sure that this lives on in PAM code */
		if (ctrl & WINBIND_UNKNOWN_OK_ARG) {
			return PAM_IGNORE;
		}
#endif
		break;
	case WBL_STATUS_OK:
		/* Otherwise, the authentication looked good */
		switch (req_type) {
		case WINBINDD_INFO:
			break;
		case WINBINDD_PAM_AUTH:
			WblpLog(State, WBL_LOG_LEVEL_NOTICE, "user '%s' granted access", user);
			break;
		case WINBINDD_PAM_CHAUTHTOK:
			WblpLog(State, WBL_LOG_LEVEL_NOTICE, "user '%s' password changed", user);
			break;
		default:
			WblpLog(State, WBL_LOG_LEVEL_NOTICE, "user '%s' OK", user);
		}
		break;
	default:
		/* we don't know anything about this return value */
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "internal module error (retval = %d, user = '%s')",
			status, user);
	}
}
#endif

static
WBL_STATUS
WblpWinbindGetSeparator(
	/* IN */ WBL_STATE* State,
	/* OUT */ char* Separator
	)
/**
 * Get the winbind separator.
 *
 * @param State WBL state
 * @param Separator returns the separator ('\0' when failure is returned).
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	char result = 0;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	status = WblpWinbindRequest(State, WINBINDD_INFO, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	result = response.data.info.winbind_separator;

cleanup:
	*Separator = result;

	return status;
}

static
WBL_STATUS
WblpWinbindNameToSidString(
	WBL_STATE* State,
	const char* Name,
	char* SidBuffer,
	size_t SidBufferSize
	)
/**
 * Convert a name into a SID string.
 *
 * @param State WBL state
 * @param Name Name to convert.
 * @param SidBuffer Buffer where to return the SID string.
 * @param SidBufferSize Size of SidBuffer (in bytes).
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status;
	const char* sid_string;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t len;

	if (WblpIsSidString(Name)) {
		sid_string = Name;
	} else {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Looking up name '%s'", Name);

		len = strlen(Name);
		if (len >= sizeof(request.data.name.name)) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Name '%s' is too long at %d characters", Name, len);
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}

		ZERO_STRUCT(request);
		ZERO_STRUCT(response);

		/* fortunatly winbindd can handle non-separated names */
		memcpy(request.data.name.name, Name, len);

		status = WblpWinbindRequest(State, WINBINDD_LOOKUPNAME, &request, &response);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		status = WblpStatusFromResponse(State, &response);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		sid_string = response.data.sid.sid;
	}

	if (strlen(sid_string) < SidBufferSize) {
		strcpy(SidBuffer, sid_string);
		status = WBL_STATUS_OK;
	} else {
		status = WBL_STATUS_BUFFER_INSUFFICIENT;
	}

cleanup:
	return status;
}

static
WBL_STATUS
WblpWinbindNameToUid(
	WBL_STATE* State,
	const char* Name,
	uid_t *uid)
/**
 * Convert a name into a uid.
 *
 * @param State WBL state
 * @param Name Name to convert.
 * @param SidBuffer Buffer where to return the SID string.
 * @param SidBufferSize Size of SidBuffer (in bytes).
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t len;

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Looking up name '%s'", Name);

	len = strlen(Name);
	if (len >= sizeof(request.data.username)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Name '%s' is too long "
			"at %d characters", Name, len);
		status = WBL_STATUS_ERROR;
		GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	/* fortunatly winbindd can handle non-separated names */
	memcpy(request.data.username, Name, len);

	status = WblpWinbindRequest(State, WINBINDD_GETPWNAM, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	*uid = response.data.pw.pw_uid;

cleanup:
	return status;
}

static
WBL_STATUS
WblpWinbindSidStringToName(
	WBL_STATE* State,
	const char* Sid,
	char** Name
	)
/**
 * Convert a name into a SID string.
 *
 * @param State WBL state
 * @param Sid SID string to convert.
 * @param Name returns buffer containing name as DOMAIN\samAccountName on
 * success or NULL otherwise.  Should be freed with WBL_FREE.
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status;
	char* account_name = NULL;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t len;

	if (!WblpIsSidString(Sid)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "'%s' is not a SID", Sid);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Looking up SID '%s'", Sid);

	len = strlen(Sid);
	if (len >= sizeof(request.data.sid)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "SID '%s' is too long at %d characters", Sid, len);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	/* fortunatly winbindd can handle non-separated names */
	memcpy(request.data.sid, Sid, len);

	status = WblpWinbindRequest(State, WINBINDD_LOOKUPSID, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (asprintf(&account_name, "%s\\%s",
		     response.data.name.dom_name,
		     response.data.name.name) <= 0)
	{
		status = WBL_STATUS_MEMORY_INSUFFICIENT;
		goto cleanup;
	}

cleanup:
	if (status) {
		WBL_FREE(account_name);
	}
	*Name = account_name;
	return status;
}

static
WBL_STATUS
WblpWinbindSidStringToPrincipalName(
	WBL_STATE* State,
	const char* Sid,
	char** PrincipalName
	)
/**
 * Convert a name into a SID string.
 *
 * @param State WBL state
 * @param Sid SID string to convert.
 * @param PrincipalName returns buffer containing corresponding principal name
 * on success or NULL otherwise.  Should be freed with WBL_FREE.
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status;
	char* principal_name = NULL;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t len;

	if (!WblpIsSidString(Sid)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "'%s' is not a SID", Sid);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Converting SID '%s' to principal name", Sid);

	len = strlen(Sid);
	if (len >= sizeof(request.data.sid)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "SID '%s' is too long at %d characters", Sid, len);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	/* fortunatly winbindd can handle non-separated names */
	memcpy(request.data.sid, Sid, len);

	status = WblpWinbindRequest(State, WINBINDD_SID_TO_PRINCIPAL, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStrDup(&principal_name, response.data.name.name);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

cleanup:
	if (status) {
		WBL_FREE(principal_name);
	}
	*PrincipalName = principal_name;
	return status;
}

static
WBL_STATUS
WblpWinbindGetUserSids(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Sid,
	/* OUT */ size_t* Count,
	/* OUT */ char** Sids
	)
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t len;
	size_t count = 0;
	char* sids = NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	len = strlen(Sid);
	if (len >= sizeof(request.data.sid)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "SID '%s' is too long at %d characters", Sid, len);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	memcpy(request.data.sid, Sid, len);

	status = WblpWinbindRequest(State, WINBINDD_GETUSERSIDS, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	/* The response comes back in as a count and a buffer.  The buffer
	   contains count entries, each null-terminated, one right after
	   the other */

	count = response.data.num_entries;
	sids = (char *)response.extra_data.data;

	/*
	 * Note that we do not clean up the response extra data because we
	 * return it.
	 *
	 * TODO: Perhaps make the interface a little safer by allocating
	 * a new buffer and cleaning up the response.  Ideally, the extra
	 * data size should be returned by winbind.
	 */

cleanup:
	*Count = count;
	*Sids = sids;

	return status;
}

static
WBL_STATUS
WblpWinbindUidToSid(
	WBL_STATE* State,
	uid_t Uid,
	char** Sid
	)
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t size;
	char* sid = NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.uid = Uid;

	status = WblpWinbindRequest(State, WINBINDD_UID_TO_SID, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	size = strlen(response.data.sid.sid) + 1;

	status = WblpAllocate(size, (void**)&sid);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	memcpy(sid, response.data.sid.sid, size);

cleanup:
	*Sid = sid;
	return status;
}


static
WBL_STATUS
WblpWinbindResponseToPw(
	/* IN */ struct winbindd_response* Response,
	/* OUT */ WBL_PASSWD_INFO** PasswdInfo
	)
{
	WBL_STATUS status;
	size_t len;
	WBL_PASSWD_INFO* info = NULL;
	size_t pw_name_len;
	size_t pw_passwd_len;
	size_t pw_gecos_len;
	size_t pw_dir_len;
	size_t pw_shell_len;

	pw_name_len = strlen(Response->data.pw.pw_name);
	pw_passwd_len = strlen(Response->data.pw.pw_passwd);
	pw_gecos_len = strlen(Response->data.pw.pw_gecos);
	pw_dir_len = strlen(Response->data.pw.pw_dir);
	pw_shell_len = strlen(Response->data.pw.pw_shell);

	len = ( pw_name_len + 1 +
		pw_passwd_len + 1 +
		pw_gecos_len + 1 +
		pw_dir_len + 1 +
		pw_shell_len + 1 );

	status = WblpAllocate(sizeof(*info) + len, (void**)&info);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	info->pw_uid = Response->data.pw.pw_uid;
	info->pw_gid = Response->data.pw.pw_gid;
	info->pw_name = info->buffer;
	info->pw_passwd= info->pw_name + pw_name_len + 1;
	info->pw_gecos = info->pw_passwd + pw_passwd_len + 1;
	info->pw_dir = info->pw_gecos + pw_gecos_len + 1;
	info->pw_shell = info->pw_dir + pw_dir_len + 1;
	memcpy(info->pw_name, Response->data.pw.pw_name, pw_name_len + 1);
	memcpy(info->pw_passwd, Response->data.pw.pw_passwd, pw_passwd_len + 1);
	memcpy(info->pw_gecos, Response->data.pw.pw_gecos, pw_gecos_len + 1);
	memcpy(info->pw_dir, Response->data.pw.pw_dir, pw_dir_len + 1);
	memcpy(info->pw_shell, Response->data.pw.pw_shell, pw_shell_len + 1);

cleanup:
	if (status) {
		WBL_FREE(info);
	}
	*PasswdInfo = info;
	return status;
}

static
WBL_STATUS
WblpWinbindGetPwByName(
	WBL_STATE* State,
	const char* Username,
	WBL_PASSWD_INFO** PasswdInfo
	)
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t len;
	WBL_PASSWD_INFO* info = NULL;

	len = strlen(Username);

	if (len >= sizeof(request.data.username)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Username '%s' is too long at %d characters", Username, len);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	memcpy(request.data.username, Username, len);

	status = WblpWinbindRequest(State, WINBINDD_GETPWNAM, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromLookupResponse(State, &response);
	if (WBL_STATUS_ACCOUNT_UNKNOWN == status) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "User '%s' is not known.",
			Username);
	}
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpWinbindResponseToPw(&response, &info);

cleanup:
	if (status) {
		WBL_FREE(info);
	}

	*PasswdInfo = info;

	return status;
}

#if defined(AIX)
static
WBL_STATUS
WblpWinbindGetPwByUid(
	WBL_STATE* State,
	uid_t Uid,
	WBL_PASSWD_INFO** PasswdInfo
	)
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	WBL_PASSWD_INFO* info = NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.uid = Uid;

	status = WblpWinbindRequest(State, WINBINDD_GETPWUID, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromLookupResponse(State, &response);
	if (WBL_STATUS_ACCOUNT_UNKNOWN == status) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Uid %d is not known.",
			Uid);
	}
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpWinbindResponseToPw(&response, &info);

cleanup:
	if (status) {
		WBL_FREE(info);
	}

	*PasswdInfo = info;

	return status;
}
#endif

static
WBL_STATUS
WblpWinbindLogoff(
	WBL_STATE* State,
	const char* Username,
	uid_t Uid,
	const char* Krb5CCacheName
	)
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t usernameLen;
	size_t krb5CCacheNameLen;

	usernameLen = strlen(Username);
	if (usernameLen >= sizeof(request.data.logoff.user)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Username '%s' is too long at %d characters", Username, usernameLen);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	if (Krb5CCacheName) {
		krb5CCacheNameLen = strlen(Krb5CCacheName);
		if (krb5CCacheNameLen >= sizeof(request.data.logoff.krb5ccname)) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Kerberos Cred Cache Name %s is too long at %d characters", Krb5CCacheName, krb5CCacheNameLen);
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	memcpy(request.data.logoff.user, Username, usernameLen);
	if (Krb5CCacheName) {
		memcpy(request.data.logoff.krb5ccname, Krb5CCacheName,
		       krb5CCacheNameLen);
	}

	request.data.logoff.uid = Uid;

	request.flags = WBFLAG_PAM_KRB5 | WBFLAG_PAM_CONTACT_TRUSTDOM;

	status = WblpWinbindRequest(State, WINBINDD_PAM_LOGOFF, &request, &response);
	if (status) {
		status = WblpStatusFromAuthResponse(State, &response);
	}
	/* really should log outside of this call */

cleanup:
	return status;
}


static
WBL_STATUS
WblpWinbindChangePassword(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ uint32_t Flags,
	/* IN */ const char* Username,
	/* IN */ const char* OldPassword,
	/* IN */ const char* NewPassword
	)
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t usernameLen = 0;
	size_t oldPasswordLen = 0;
	size_t newPasswordLen = 0;
	WBL_STATUS rejectStatus = WBL_STATUS_OK;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	usernameLen = strlen(Username);
	if (usernameLen >= sizeof(request.data.auth.user)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Username %s is too long at %d characters", Username, usernameLen);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	/* On some systems, an empty password can come down as NULL */
	if (OldPassword) {
		oldPasswordLen = strlen(OldPassword);
		if (oldPasswordLen >= sizeof(request.data.chauthtok.oldpass)) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Old password is too long");
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
	}
	if (NewPassword) {
		newPasswordLen = strlen(NewPassword);
		if (newPasswordLen >= sizeof(request.data.chauthtok.newpass)) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "New password is too long");
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
	}

	request.flags = Flags;
	memcpy(request.data.chauthtok.user, Username, usernameLen);
	if (OldPassword) {
		memcpy(request.data.chauthtok.oldpass, OldPassword, oldPasswordLen);
	}
	if (NewPassword) {
		memcpy(request.data.chauthtok.newpass, NewPassword, newPasswordLen);
	}

	status = WblpWinbindRequest(State, WINBINDD_PAM_CHAUTHTOK, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromAuthResponse(State, &response);
	if (WBL_STATUS_PASSWORD_RESTRICTION == status) {
		rejectStatus = WBL_STATUS_PASSWORD_RESTRICTION;
		WblpStateAuthInfoPasswordPolicyFromResponse(State, &response);
		switch (response.data.auth.reject_reason) {
			case -1:
				break;
			case REJECT_REASON_OTHER:
				if ((State->AuthInfo.PasswordPolicy.MinimumAge > 0) &&
				    (State->AuthInfo.PasswordLastSetTime > 0) &&
				    (State->AuthInfo.PasswordLastSetTime + State->AuthInfo.PasswordPolicy.MinimumAge > time(NULL)))
				{
					rejectStatus = WBL_STATUS_PASSWORD_TOO_RECENT;
				}
				break;
			case REJECT_REASON_TOO_SHORT:
				rejectStatus = WBL_STATUS_PASSWORD_TOO_SHORT;
				break;
			case REJECT_REASON_IN_HISTORY:
				rejectStatus = WBL_STATUS_PASSWORD_IN_HISTORY;
				break;
			case REJECT_REASON_NOT_COMPLEX:
				rejectStatus = WBL_STATUS_PASSWORD_NOT_COMPLEX;
				break;
			default:
				WblpLog(State, WBL_LOG_LEVEL_ERROR,
					"Unknown password change reject reason: %d",
					response.data.auth.reject_reason);
				break;
		}
	}

cleanup:
	State->ChangePasswordStatus = status;
	State->ChangePasswordRejectStatus = rejectStatus;

	return status;
}

static
WBL_STATUS
WblpCheckPasswordExpiration(
	WBL_STATE* State
	)
{
	WBL_STATUS status = WBL_STATUS_OK;
	time_t now = time(NULL);
	time_t next_change;
	time_t min_next_change;

	State->AuthInfo.NextPasswordChangeTime = 0;

	/* accounts with ACB_PWNOEXP set never receive a warning */
	if (TestFlag(State->AuthInfo.AccountFlags, ACB_PWNOEXP)) {
		goto cleanup;
	}

	/* no point in sending a warning if this is a grace logon */
	if (TestFlag(State->AuthInfo.UserFlags, LOGON_GRACE_LOGON)) {
		goto cleanup;
	}

	/* check if the info3 must change timestamp has been set */
	next_change = State->AuthInfo.PasswordMustChangeTime;

	if (next_change <= now) {
		SetFlag(State->Flags, WBL_STATE_FLAG_NEXT_CHANGE_TIME);
		State->AuthInfo.NextPasswordChangeTime = next_change;
		status = WBL_STATUS_PASSWORD_MUST_CHANGE;
		goto cleanup;
	}

	min_next_change = next_change;

	/* now check for the global password policy */
	/* good catch from Ralf Haferkamp: an expiry of "never" is translated
	 * to -1 */
	if (State->AuthInfo.PasswordPolicy.ExpireTime > 0) {
		next_change = State->AuthInfo.PasswordLastSetTime + State->AuthInfo.PasswordPolicy.ExpireTime;

		if (next_change < min_next_change) {
			min_next_change = next_change;
		}

		if (next_change <= now) {
			SetFlag(State->Flags, WBL_STATE_FLAG_NEXT_CHANGE_TIME);
			State->AuthInfo.NextPasswordChangeTime = next_change;
			status = WBL_STATUS_PASSWORD_EXPIRED;
			goto cleanup;
		}
	}

	SetFlag(State->Flags, WBL_STATE_FLAG_NEXT_CHANGE_TIME);
	State->AuthInfo.NextPasswordChangeTime = min_next_change;

cleanup:
	return status;
}

static
WBL_STATUS
WblpWinbindAuthenticate(
	WBL_STATE* State,
	const char* Username,
	const char* Password,
	uint32_t Flags,
	uid_t Uid,
	const char* Krb5CCacheType,
	const char* RequiredMembershipSids
	)
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	size_t usernameLen = 0;
	size_t passwordLen = 0;
	size_t krb5CCacheTypeLen = 0;
	size_t requiredMembershipSidsLen = 0;
	bool is_response_initialized = false;

	usernameLen = strlen(Username);
	if (usernameLen >= sizeof(request.data.auth.user)) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Username %s is too long at %d characters", Username, usernameLen);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	/* On some systems, an empty password can come down as NULL */
	if (Password) {
		passwordLen = strlen(Password);
		if (passwordLen >= sizeof(request.data.auth.pass)) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Password is too long");
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
	}

	if (Krb5CCacheType) {
		krb5CCacheTypeLen = strlen(Krb5CCacheType);
		if (krb5CCacheTypeLen >= sizeof(request.data.auth.krb5_cc_type)) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Kerberos CCache Type is too long at %d characters (%s)", krb5CCacheTypeLen, Krb5CCacheType);
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
	}

	if (RequiredMembershipSids) {
		requiredMembershipSidsLen = strlen(RequiredMembershipSids);
		if (requiredMembershipSidsLen >= sizeof(request.data.auth.require_membership_of_sid)) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Required membership sids is too long at %d characters (%s)", requiredMembershipSidsLen, RequiredMembershipSids);
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);
	is_response_initialized = true;

	request.flags = Flags;
	request.data.auth.uid = Uid;
	memcpy(request.data.auth.user, Username, usernameLen);
	if (Password) {
		memcpy(request.data.auth.pass, Password, passwordLen);
	}
	if (Krb5CCacheType) {
		memcpy(request.data.auth.krb5_cc_type, Krb5CCacheType, krb5CCacheTypeLen);
	}
	if (RequiredMembershipSids) {
		memcpy(request.data.auth.require_membership_of_sid, RequiredMembershipSids, requiredMembershipSidsLen);
	}

	status = WblpWinbindRequest(State, WINBINDD_PAM_AUTH, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromAuthResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStateAuthInfoFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

cleanup:
	if (is_response_initialized) {
		winbindd_free_response(&response);
	}
	if (status) {
		WblpStateAuthInfoCleanup(State);
	}
	return status;
}

static
WBL_STATUS
WblpWinbindListTrustedDomains(
	WBL_STATE* State,
	char** TrustedDomains
	)
/* TODO: Return an array that we can more easily walk? */
{
	WBL_STATUS status;
	struct winbindd_request request;
	struct winbindd_response response;
	char* result = NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.list_all_domains = true;

	status = WblpWinbindRequest(State, WINBINDD_LIST_TRUSTDOM, &request, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStatusFromResponse(State, &response);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpStrDup(&result, (char*)response.extra_data.data);

cleanup:
	if (status) {
		WBL_FREE(result);
	}
	*TrustedDomains = result;
	return status;
}

static
bool
WblpIsEqualStringsWithLengths(
	const char *s1,
	int len1,
	const char *s2,
	int len2,
	bool case_sensitive
	)
{
	if (len1 != len2) {
		return false;
	}
	if (len1 == 0) {
		return true;
	}
	if (case_sensitive) {
		return !strncmp(s1, s2, len1) ? true : false;
	} else {
		return !strncasecmp(s1, s2, len1) ? true : false;
	}
}

static
bool
WblpIsEqualStringsWithLengthsLog(
	WBL_STATE* State,
	const char *s1,
	int len1,
	const char *s2,
	int len2,
	bool case_sensitive
	)
{
	bool result;
	char *ps1 = NULL;
	char *ps2 = NULL;

	/* We need to malloc to log... */
	/* TODO: Perhaps use %.*s */
	if (len1 > 0) {
		ps1 = malloc(len1 + 1);
		if (ps1) {
			strncpy(ps1, s1, len1);
			ps1[len1] = 0;
		}
	}

	if (len2 > 0) {
		ps2 = malloc(len2 + 1);
		if (ps2) {
			strncpy(ps2, s2, len2);
			ps2[len2] = 0;
		}
	}

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "COMPARE: \"%s\" (%d), \"%s\" (%d)",
		ps1 ? ps1 : "", len1, ps2 ? ps2 : "", len2);

	result = WblpIsEqualStringsWithLengths(s1, len1, s2, len2, case_sensitive);

	SAFE_FREE(ps1);
	SAFE_FREE(ps2);

	return result;
}

/* GP License Code */

static void _gp_pam_log(void *context, int is_err, char *format, ...)
{
	WBL_STATE* state = (WBL_STATE*)context;
	va_list args;

	va_start(args, format);
	WblpLogV(state, is_err ? WBL_LOG_LEVEL_ERROR : WBL_LOG_LEVEL_DEBUG, format, args);
	va_end(args);
}

static void _gp_pam_user_msg(void *context, int is_err, char *format, ...)
{
	WBL_STATE* state = (WBL_STATE*)context;
	va_list args;

	va_start(args, format);
	WblpMessageV(state, is_err ? WBL_LOG_LEVEL_ERROR : WBL_LOG_LEVEL_INFO, format, args);
	va_end(args);
}

static
WBL_STATUS
WblpCheckLicense(
	/* IN */ WBL_STATE* State
	)
{
	return WBL_STATUS_OK;
}

WBL_STATUS
WblApplyUserLoginPolicies(
        /* IN */ WBL_STATE* State,
        /* IN */ const char* Username
        )
{
	WBL_STATUS status = WBL_STATUS_OK;
	char *account_name = NULL;
	char separator;
	const char *name = Username;
	char *sid = NULL;
	uid_t uid;

	/* Convert the username to its canonical DOMAIN\username form */

	status = WblpWinbindGetSeparator(State, &separator);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (strchr(Username, separator) != NULL) {
		/* Convert the name to a uid */
		status = WblpWinbindNameToUid(State, Username, &uid);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		/* Convert the uid to a SID */

		status = WblpWinbindUidToSid(State, uid, &sid);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		/* Convert the the SID back to the DOMAIN\sAMAccountName */
		status = WblpWinbindSidStringToName(State, sid, &account_name);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

	}

	if (account_name)
		name = account_name;

	/* Send request to gpagentd */
        if (!gp_process_login(State, name, WblStateIsCachedLogon(State),
				  _gp_pam_log, _gp_pam_user_msg))
	{
/* bkoropoff: don't bail out of pam session if GP is MIA -- fixes LWO
   FIXME: we still want to report errors if GP is present but doesn't work
 */
#if 0
		status = WBL_STATUS_USER_POLICY_ERROR;
		GOTO_CLEANUP_ON_WBL_STATUS(status);
#endif
        }

cleanup:
	WBL_FREE(account_name);
	WBL_FREE(sid);

	return status;
}

WBL_STATUS
WblApplyUserLogoutPolicies(
        /* IN */ WBL_STATE* State,
        /* IN */ const char* Username
        )
{
	WBL_STATUS status = WBL_STATUS_OK;
	char *account_name = NULL;
	char separator;
	const char *name = Username;
	char *sid = NULL;
	uid_t uid;

	/* Convert the username to its canonical DOMAIN\username form */

	status = WblpWinbindGetSeparator(State, &separator);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (strchr(Username, separator) != NULL) {
		/* Convert the name to a uid */
		status = WblpWinbindNameToUid(State, Username, &uid);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		/* Convert the uid to a SID */

		status = WblpWinbindUidToSid(State, uid, &sid);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		/* Convert the the SID back to the DOMAIN\sAMAccountName */
		status = WblpWinbindSidStringToName(State, sid, &account_name);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

	}

	if (account_name)
		name = account_name;

        if (gp_process_logout(State, name,
				WblStateIsCachedLogon(State),
				_gp_pam_log, _gp_pam_user_msg))
	{
		status = WBL_STATUS_USER_POLICY_ERROR;
		GOTO_CLEANUP_ON_WBL_STATUS(status);
        }

cleanup:
	WBL_FREE(account_name);
	WBL_FREE(sid);

	return status;
}

static
bool
WblpCheckDirectoryExists(
	/* IN */ const char* Path,
	/* OUT */ bool* Exists
	)
{
	WBL_STATUS status = WBL_STATUS_OK;
	bool exists = false;
	struct stat st;

	if ( !Path ) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	if ( stat(Path, &st) != 0 ) {
		if (errno != ENOENT && errno != ENOTDIR) {
			status = WBL_STATUS_ERROR;
		}
		goto cleanup;
	}

	if ( S_ISDIR(st.st_mode) ) {
		exists = true;
	}

cleanup:
	*Exists = exists;
	return status;
}


static
WBL_STATUS
WblpTranslateUpnToUsername(
	/* IN */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* OUT */ char** TranslatedUsername
	)
/**
 * Translate a username encoded as a user principal name (UPN) to a proper
 * local username.
 *
 * @param State WBL state
 * @param Username Name to convert.
 * @param TranslatedUsername If the name is translated, returns the new name
 *     to use.	Otherwise, returns NULL.
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status = WBL_STATUS_OK;
	char *account_name = NULL;
	fstring sid;
	char separator;

	if (WblpIsUpn(Username)) {

		/* We get the separator after checking whether this is
		   a potential UPN to optimize out calls to winbind. */
		status = WblpWinbindGetSeparator(State, &separator);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		/* If the separator character is the UPN separator, then
		   we cannot do UPN translation because we cannot distinguish
		   DOMAIN<sep>username from username@DOMAIN. */
		if (UPN_SEPARATOR == separator) {
			status = WBL_STATUS_OK;
			goto cleanup;
		}

		/* Convert the UPN to a SID */
		status = WblpWinbindNameToSidString(State, Username, sid, sizeof(sid));
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		/* Convert the the SID back to the sAMAccountName */
		status = WblpWinbindSidStringToName(State, sid, &account_name);
		GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

cleanup:
	if (status) {
		WBL_FREE(account_name);
	}
	*TranslatedUsername = account_name;
	return status;
}


#if defined(AIX)
static
WBL_STATUS
WblpTranslateAixToUsername(
	/* IN */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* OUT */ char** TranslatedUsername
	)
/**
 * Translate a username encoded as _<uid> to a proper local username.
 * The _<uid> encoding is how AIX can encode long usernames.
 *
 * For example, "userwithlongname" might be encoded as "_1057".
 *
 * @param State WBL state
 * @param Username Name to convert.
 * @param TranslatedUsername If the name is translated, returns the new name
 *     to use.  Otherwise, returns NULL.
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status = WBL_STATUS_OK;
	char *real_username = NULL;
	WBL_PASSWD_INFO* pwd = NULL;

	/* Decode the user name since AIX does not support long user
	   names by default.  The name is encoded as _<uid>.  */

	if (Username[0] == '_')
	{
	    uid_t uid = (uid_t) WblUnmangleAIX(Username);
	    
	    status = WblpWinbindGetPwByUid(State, uid, &pwd);
	    GOTO_CLEANUP_ON_WBL_STATUS(status);
	    status = WblpStrDup(&real_username, pwd->pw_name);
	    GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

cleanup:
	WBL_FREE(pwd);

	if (status) {
		WBL_FREE(real_username);
	}

	*TranslatedUsername = real_username;
	return status;
}
#endif /* AIX */

static
WBL_STATUS
WblpTranslateUsername(
	/* IN */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* OUT */ char** TranslatedUsername
	)
/**
 * Translate a username to a canonical representation that we can
 * authenticate.  This translation includes handling user principal
 * names (UPN) as well as any platform-specific username conversions.
 *
 * @param State WBL state
 * @param Username Name to convert.
 * @param TranslatedUsername If the name is translated, returns the new name
 *     to use.  Otherwise, returns NULL.
 *
 * @return WBL_STATUS_OK on success, another otherwise.
 */
{
	WBL_STATUS status;

	status = WblpTranslateUpnToUsername(State, Username, TranslatedUsername);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (*TranslatedUsername) {
		goto cleanup;
	}

#if defined(AIX)
	status = WblpTranslateAixToUsername(State, Username, TranslatedUsername);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (*TranslatedUsername) {
		goto cleanup;
	}
#endif

	/* TODO: Canonicalize name to login name based on getpwnam-type call */

cleanup:
	return status;
}


/* TODO: Make this call check whether this user is known, and perhaps do a
   license check too.  Noite that we can only do a license check _after_
   checking whether the user is ours. Also, we might want to compare the
   retults of getpwnam vs GetPwByName to make sure that NSS and WB agree. */
WBL_STATUS
WblSetUsername(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	)
{
	WBL_STATUS status;
	char* translatedUsername = NULL;

	if (!Username) {
		if (State->Username) {
			status = WBL_STATUS_OK;
		} else {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Missing username");
			status = WBL_STATUS_INVALID_STATE;
		}
	} else if (!State->Username) {
		status = WblpTranslateUsername(State, Username, &translatedUsername);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		if (translatedUsername) {
			status = WblpStrDup(&State->OriginalUsername, Username);
			GOTO_CLEANUP_ON_WBL_STATUS(status);

			State->Username = translatedUsername;
			translatedUsername = NULL;
		} else {
			status = WblpStrDup(&State->Username, Username);
			GOTO_CLEANUP_ON_WBL_STATUS(status);
		}
	} else if (!strcasecmp(State->Username, Username)) {
		status = WBL_STATUS_OK;
	} else if (State->OriginalUsername && !strcasecmp(State->OriginalUsername, Username)) {
		status = WBL_STATUS_OK;
	} else {
		status = WBL_STATUS_INVALID_STATE;
	}

cleanup:
	WBL_FREE(translatedUsername);
	return status;
}

const char *
WblGetUsername(
	/* IN OUT */ WBL_STATE* State
	)
{
	return State->Username;
}


static
WBL_STATUS
WblpGetPwByName(
	WBL_STATE* State,
	const char* Username,
	WBL_PASSWD_INFO** PasswdInfo
	)
{
	WBL_STATUS status;
	WBL_PASSWD_INFO* passwdInfo = NULL;

	status = WblSetUsername(State, Username);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (State->PasswdInfoStatus) {
		status = State->PasswdInfoStatus;
		goto cleanup;
	} else if (!State->PasswdInfo) {
		status = WblpWinbindGetPwByName(State, State->Username, &State->PasswdInfo);
		if (status) {
			State->PasswdInfoStatus = status;
			goto cleanup;
		}
	}

	passwdInfo = State->PasswdInfo;
	status = WBL_STATUS_OK;

cleanup:
	*PasswdInfo = passwdInfo;
	WBL_ASSERT(WBL_XOR(status, passwdInfo));
	return status;
}


static
WBL_STATUS
WblpCopyFile(
	WBL_STATE* State,
	const char* SourcePath,
	const char* TargetPath,
	uid_t uid,
	gid_t gid,
	mode_t mode
	)
/**
 * Copy a file from SourcePath to TargetPath using the given uid, gid, and
 * mode for the ownership and permissions of the new file.
 * This will replace an existing file.
 *
 * @param State WBL state
 * @param SourcePath source file
 * @param TargetPath target file
 * @param uid user id to own the new file.
 * @param gid group id to own the new file.
 * @param mode permissions for the new file.
 *
 * @return WBL_STATUS_OK on succuess, or a another code on failure.
 */
{
	WBL_STATUS status;
	int sourceFd = -1;
	int targetFd = -1;
	char buf[BUFSIZ];
	int bytesRead;
	int bytesWritten;
	mode_t permMode;

	/* Make sure that we are only trying to mess with perm bits */
	permMode = mode & (S_IRWXU | S_IRWXG | S_IRWXO );
	if ( permMode != mode ) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR,
			"Non-permission bits present in mode %04o while trying to copy %s to %s",
			mode, SourcePath, TargetPath);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	sourceFd = open(SourcePath, O_RDONLY);
	if (sourceFd < 0) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to open source file %s: %s",
			SourcePath, strerror(errno));
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}
	targetFd = open(TargetPath, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (targetFd < 0) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to open destination file %s:",
			TargetPath, strerror(errno));
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	do {
		bytesRead = read(sourceFd, buf, sizeof(buf));
		if (bytesRead < 0) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "IO error on read %s: %s",
				SourcePath, strerror(errno));
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
		else if (bytesRead == 0) {
			continue;
		}
		bytesWritten = write(targetFd, buf, bytesRead);
		if (bytesWritten < 0) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "IO error on write to %s: %s",
				TargetPath, strerror(errno));

			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
	} while (bytesRead > 0);

	close(sourceFd);
	sourceFd = -1;
	close(targetFd);
	targetFd = -1;

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Attempting to chown(%s, %d, %d)",
		TargetPath, uid, gid);
	if (chown(TargetPath, uid, gid) != 0) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to chown(%s, %d, %d): %s",
			TargetPath, uid, gid, strerror(errno));
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}
	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Attempting to chmod(%s, %04o)",
		TargetPath, mode);
	if (chmod(TargetPath, mode) != 0) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to chmod(%s, %04o): %s",
			TargetPath, mode, strerror(errno));
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	status = WBL_STATUS_OK;

cleanup:
	if (sourceFd >= 0) {
		close(sourceFd);
	}
	if (targetFd >= 0) {
		close(targetFd);
	}
	return status;
}

static
WBL_STATUS
WblpCopyEntry(
	WBL_STATE* State,
	const char* SourcePath,
	const char* TargetPath,
	uid_t uid,
	gid_t gid
	)
/**
 * Copy a directory entry from SourcePath to TargetPath
 * using the given uid and gid for the ownership information of
 * the new entry.  If SourcePath is a directory, this will copy
 * the entire subtree.  Any entries that are not directories or
 * regular files are skipped with a success code.
 *
 * @param State WBL state
 * @param SourcePath source entry
 * @param TargetPath target entry
 * @param uid user id to own the new files/directories.
 * @param gid group id to own the new files/directories.
 *
 * @return WBL_STATUS_OK on succuess, or a another code on failure.
 */
{
	WBL_STATUS status;
	struct stat statbuf;
	bool exists;
	mode_t permMode;

	if (lstat(SourcePath, &statbuf) != 0) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to lstat(%s): %s",
			SourcePath, strerror(errno));
		status = WBL_STATUS_ERROR;
	}

	permMode = statbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO );

	if (S_ISDIR(statbuf.st_mode)) {

		status = WblpCheckDirectoryExists(TargetPath, &exists);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		if (!exists) {
			WblpLog(State, WBL_LOG_LEVEL_DEBUG, "subdir: attempting to mkdir(%s, %04o)", TargetPath, permMode);
			if (mkdir(TargetPath, permMode) != 0)
			{
				WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to create directory %s: %s",
					TargetPath, strerror(errno));
				status = WBL_STATUS_ERROR;
				goto cleanup;
			}
			WblpLog(State, WBL_LOG_LEVEL_DEBUG, "subdir: attempting to chown(%s, %d, %d)", TargetPath, uid, gid);
			if (chown(TargetPath, uid, gid) != 0)
			{
				WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to chown(%s, %d, %d): %s",
					TargetPath, uid, gid, strerror(errno));
				status = WBL_STATUS_ERROR;
				goto cleanup;
			}
		}
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "subdir: recursing to copy from %s to %s.",
			SourcePath, TargetPath);
		status = WblpCopyTree(State, SourcePath, TargetPath, uid, gid);
	} else if (!S_ISREG(statbuf.st_mode)) {
		/* Let's ignore anything that isn't a regular file */
		status = WBL_STATUS_OK;
	} else {
		/* At this point we're copying over a regular file */
		status = WblpCopyFile(State, SourcePath, TargetPath, uid, gid, permMode);
	}

cleanup:
	return status;
}

static
WBL_STATUS
WblpCopyTree(
	WBL_STATE* State,
	const char* SourcePath,
	const char* TargetPath,
	uid_t uid,
	gid_t gid
	)
/**
 * Copy a directory tree from SourcePath to a pre-existing directory at
 * TargetPath using the given uid and gid for the ownership information of
 * the new directories and files.
 *
 * @param State WBL state
 * @param SourcePath source directory
 * @param TargetPath target directory (which needs to already exist)
 * @param uid user id to own the new files/directories.
 * @param gid group id to own the new files/directories.
 *
 * @return WBL_STATUS_OK on succuess, or a another code on failure.
 */
{
	WBL_STATUS status = WBL_STATUS_OK;
	DIR *d = NULL;

	if (!SourcePath) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR,
			"No source directory specified");
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	d = opendir(SourcePath);
	if (!d) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR,
			"Unable to open source file directory [%s]", SourcePath);
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	for (;;) {
		struct dirent* dir;
		char sourceEntry[PATH_MAX];
		char targetEntry[PATH_MAX];

		dir = readdir(d);
		if (!dir) {
			/* TODO: Add proper error checking */
			break;
		}

		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "copy: examining [%s]",
			dir->d_name);

		/* Skip "." and ".." */
		if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) {
			continue;
		}

		/* TODO: Ideally, allocate string or verify that we did not truncate */
		snprintf(sourceEntry, sizeof(sourceEntry), "%s/%s", SourcePath, dir->d_name);
		sourceEntry[sizeof(sourceEntry)-1] = 0;

		snprintf(targetEntry, sizeof(targetEntry), "%s/%s", TargetPath, dir->d_name);
		targetEntry[sizeof(targetEntry)-1] = 0;

		WblpLog(State, WBL_LOG_LEVEL_DEBUG,
			"copy: src=[%s], dest=[%s]",
			sourceEntry, targetEntry);

		status = WblpCopyEntry(State, sourceEntry, targetEntry, uid, gid);
		GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

cleanup:
	if (d) {
		closedir(d);
	}

	return status;
}


WBL_STATUS
WblCreateK5Login(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* OPTIONAL IN */ const char* UserPrincipalName
	)
/**
 * Create .k5login in user's home directory if one does not already exist.
 * The created file will contain the user's UPN in canonical and lowercase
 * formats.
 *
 * @param State WBL state
 * @param Username The username
 * @param UserPrincipalName The user pricipal name to use.  If NULL,
 *     we attempt to use any UPN information we already have from
 *     WblAuthenticate.  If none if found, this operation is a no-op.
 *
 * @return WBL_STATUS_OK on succuess, or a another code on failure.
 */
{
	WBL_STATUS status;
	WBL_PASSWD_INFO* pwd = NULL;
	struct stat st;
	char *path = NULL;
	char *temp = NULL;
	char *data = NULL;
	char *lower = NULL;
	int data_len;
	int data_out;
	int i;
	int fd = -1;
	bool need_unlink = false;
	const char *user_principal_name = NULL;

	if (!TestFlagAll(State->Config->Flags,
			 WBL_CONFIG_FLAG_CREATE_K5LOGIN | WBL_CONFIG_FLAG_KRB5_AUTH)) {
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	if (!IS_NULL_OR_EMPTY_STR(UserPrincipalName)) {
		user_principal_name = UserPrincipalName;
	} else if (!IS_NULL_OR_EMPTY_STR(State->AuthInfo.UserPrincipalName)) {
		user_principal_name = State->AuthInfo.UserPrincipalName;
	} else {
		WblpLog(State, WBL_LOG_LEVEL_WARN, "Missing UPN for user '%s'", State->Username);
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	status = WblSetUsername(State, Username);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpGetPwByName(State, State->Username, &pwd);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (IS_NULL_OR_EMPTY_STR(pwd->pw_dir)) {
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	if (asprintf(&path, "%s/%s", pwd->pw_dir, ".k5login") <= 0) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	/* If the file exists, we are done */
	if (stat(path, &st) == 0) {
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	/* Make sure we got ENOENT */
	if (errno != ENOENT) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	if (asprintf(&temp, "%s.lwidentity.temp", path) <= 0) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	fd = open(temp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	need_unlink = true;

	if (fchown(fd, pwd->pw_uid, pwd->pw_gid) < 0) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	status = WblpStrDup(&lower, user_principal_name);
	GOTO_CLEANUP_ON_WBL_STATUS(status);
	for (i = 0; lower[i] && lower[i] != '@'; i++)
	{
		lower[i] = tolower(lower[i]);
	}

	data_len = asprintf(&data, "%s\n%s\n", user_principal_name, lower);
	if (data_len <= 0) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	data_out = write(fd, data, data_len);
	if (data_out < data_len) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	if (fsync(fd) < 0) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	close(fd);
	fd = -1;

	if (rename(temp, path) < 0) {
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	need_unlink = false;

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "created .k5login for user '%s'", State->Username);

	status = WBL_STATUS_OK;

cleanup:
	if (need_unlink) {
		unlink(temp);
	}

	if (fd >= 0) {
		close(fd);
	}

	SAFE_FREE(data);
	SAFE_FREE(temp);
	SAFE_FREE(path);
	SAFE_FREE(lower);

	return status;
}


WBL_STATUS
WblCreateHomeDirectory(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	)
/**
 * Create home directory for a user, if so inidicated by the state.  If no
 * home directory needs to created, nothing happens (and success is returned).
 * Otherwise, the home directory is created using any configured skel and
 * umask information.  Note that if the skel copy fails partway, that's that.
 * This routine will not try to continue the next time it is called.
 * (TODO: Perhaps we want to change that -- which would be relatively
 * straightfoward, but require a little more bookkeeping.)
 *
 * @param State WBL state
 * @param Username The username
 *
 * @return WBL_STATUS_OK on succuess, or a another code on failure.
 */
{
	WBL_STATUS status;
	WBL_PASSWD_INFO* pwd;
	char tok[1024], create_dir[1024];
	char *p;
	bool exists;

	if (!TestFlag(State->Config->Flags, WBL_CONFIG_FLAG_CREATE_HOMEDIR)) {
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	status = WblSetUsername(State, Username);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpGetPwByName(State, State->Username, &pwd);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (IS_NULL_OR_EMPTY_STR(pwd->pw_dir)) {
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "homedir is %s", pwd->pw_dir);

	status = WblpCheckDirectoryExists(pwd->pw_dir, &exists);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (exists) {
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	strcpy(create_dir, "/");
	strncpy(tok, pwd->pw_dir, sizeof(tok)-1 );
	tok[sizeof(tok)-1] = 0;

	p = tok+1;

	while ( p && *p ) {
		mode_t mode = 0755;
		int dsize;
		char* s;

		if ( (s = strchr(p, '/')) != NULL ) {
			*s = 0;
		}

		dsize = strlen(create_dir)+1;
		strncat( create_dir, p, sizeof(create_dir)-dsize );
		dsize += strlen(p);
		strncat( create_dir, "/", sizeof(create_dir)-dsize);

		status = WblpCheckDirectoryExists(create_dir, &exists);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		if (!exists) {
			char parentDir[PATH_MAX];
			struct stat statbuf;

			WblpLog(State, WBL_LOG_LEVEL_DEBUG,
				"dir: attempting to mkdir [%s] to [%04o]",
				create_dir, mode);

			if (mkdir(create_dir, mode) != 0) {
				WblpLog(State, WBL_LOG_LEVEL_ERROR,
					"could not create dir: %s (%s)",
					create_dir, strerror(errno));
				status = WBL_STATUS_ERROR;
				goto cleanup;
			}

			memset(parentDir, 0, sizeof(parentDir));

			snprintf(parentDir, sizeof(parentDir), "%s/..", create_dir);
			if (stat(parentDir, &statbuf) != 0) {
				WblpLog(State, WBL_LOG_LEVEL_ERROR,
					"could not stat parent directory: %s (%s)",
					parentDir, strerror(errno));
				status = WBL_STATUS_ERROR;
				goto cleanup;
			}
			if (chown(create_dir, statbuf.st_uid, statbuf.st_gid) != 0) {
				WblpLog(State, WBL_LOG_LEVEL_ERROR,
					"failed to chown [%s] to (%d,%d)",
					create_dir, statbuf.st_uid, statbuf.st_gid);
				status = WBL_STATUS_ERROR;
				goto cleanup;
			}
			/*Even though the mode was passed to mkdir,
			 * mkdir is affected by the umask. This will
			 * set the mode to exactly what we want.
			 */
			if(chmod(create_dir, mode) != 0)
			{
				WblpLog(State, WBL_LOG_LEVEL_ERROR,
					"failed to chmod [%s] to [%04o]",
					create_dir, mode);
				status = WBL_STATUS_ERROR;
				goto cleanup;
			}
		}

		/* Check to see if we have handled the last path
		   component.  Yes, I'm actually pretesting the loop
		   conditional here. */

		if ( !s ) {
			break;
		}

		p = s+1;
	}

	WblpLog(State, WBL_LOG_LEVEL_DEBUG,
		"dir: attempting to chown [%s] to (%d,%d)",
		create_dir, pwd->pw_uid, pwd->pw_gid);

	if (chown(create_dir, pwd->pw_uid, pwd->pw_gid) != 0) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "failed to chown user homedir: %s (%s)",
			create_dir, strerror(errno));
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}
	if (chmod(create_dir, 0755 & ~(State->Config->HomedirUmask)) != 0) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "failed to chmod user homedir file: %s (%s)",
			create_dir, strerror(errno));
		status = WBL_STATUS_ERROR;
		goto cleanup;
	}

	/* At this point, we've created the home directory. Let's copy over
	   any files from /etc/skel that we need */

	if (State->Config->SkelDir) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG,
			"dir: Copying skeleton dir \"%s\" into \"%s\"",
			State->Config->SkelDir, create_dir);
		status = WblpCopyTree(State, State->Config->SkelDir,
				      create_dir, pwd->pw_uid, pwd->pw_gid);
		GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

	status = WBL_STATUS_OK;

cleanup:
	return status;
}


bool
WblpCheckSidRequirement(
	/* IN */ WBL_STATE* State,
	/* IN */ size_t SidsCount,
	/* IN */ char* Sids,
	/* IN */ char* RequiredMembershipSids
	)
/**
 * Check whether one of the SIDs in Sids is in RequiredMembershipSids.
 *
 * @param State WBL state
 * @param SidCount count of SIDs in Sids
 * @param Sids a buffer containing SidCount consecutive NULL-terminated SID
 * strings.
 * @param RequiredMembershipSids a comma-delimited list of SID strings.
 *
 * @return true if a SID in Sids is in RequiredMembershipSids, false otherwise.
 */
{
	bool ok = false;
	char* sid;
	size_t i;

	sid = Sids;
	for (i = 0; i < SidsCount; i++) {
		size_t sid_len;
		char *req_sid;
		char *p;

		sid_len = strlen(sid);
		p = req_sid = RequiredMembershipSids;
		while (1) {
			if (',' == *p || 0 == *p) {
				size_t req_sid_len = p - req_sid;
				if (WblpIsEqualStringsWithLengthsLog(State,
								     req_sid,
								     req_sid_len,
								     sid,
								     sid_len,
								     false)) {
					ok = true;
					goto cleanup;
				}
				if (!*p) {
					break;
				}
				req_sid = p + 1;
			}
			p++;
		}
		sid += sid_len + 1;
	}

cleanup:
	return ok;
}


static
WBL_STATUS
WblpAppendStringWithGrowthAsNeeded(
	/* IN */ WBL_STATE* State,
	/* IN OUT */ char** Buffer,
	/* IN OUT */ size_t* Size,
	/* IN OUT */ size_t* Offset,
	/* IN */ size_t MaxSize,
	/* IN */ const char* StringToAppend
	)
/**
 * Append a string to a buffer, growing the buffer as needed.
 *
 * @param State WBL state
 * @param Buffer Buffer to which we want to append, returns new buffer is not
 * big enough.	Buffer returned can be NULL on error.
 * @param Size On input, size of Buffer.  On output, size of Buffer (if no
 * error occurred).
 * @param Offset Offset of buffer into which we want to append.  On output,
 * returns new offset to the end of buffer where we can append more.
 * @param MaxSize Maximum size that we can grow buffer to.
 * @param StringToAppend the string that we want to append to the buffer.
 *
 * @return WBL_STATUS_OK on success, or something else on failure.
 */
{
	WBL_STATUS status;
	size_t len = strlen(StringToAppend);

	while ((*Offset - *Size) < (len + 1)) {
		WBL_FREE(*Buffer);
		if (*Size >= MaxSize) {
			status = WBL_STATUS_MEMORY_INSUFFICIENT;
			goto cleanup;
		}
		*Size *= 2;
		status = WblpAllocate(*Size, (void**)Buffer);
		GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

	memcpy((*Buffer) + *Offset, StringToAppend, len + 1);
	*Offset += len;
	status = WBL_STATUS_OK;

cleanup:
	return status;
}


static
WBL_STATUS
WblpNameListToSidStringListAppend(
	/* IN */ WBL_STATE* State,
	/* IN */ const char* Name,
	/* IN OUT */ char** Buffer,
	/* IN OUT */ size_t* Size,
	/* IN OUT */ size_t* Offset,
	/* IN */ size_t MaxSize
	)
/**
 * Convert a name into a SID, appending it to the specified buffer (and growing
 * the buffer as needed).
 *
 * @param State WBL state
 * @param Name Name to convert into a SID, or a SID string.
 * @param Buffer Buffer to which we want to append, returns new buffer is not
 * big enough.  Buffer returned can be NULL on error.
 * @param Size On input, size of Buffer.  On output, size of Buffer (if no
 * error occurred).
 * @param Offset Offset of buffer into which we want to append.  On output,
 * returns new offset to the end of buffer where we can append more.
 * @param MaxSize Maximum size that we can grow buffer to.
 *
 * @return WBL_STATUS_OK on success, or something else on failure.
 */
{
	WBL_STATUS status;
	fstring sidBuffer;
	const char* sid;

	if (WblpIsSidString(Name)) {
		sid = Name;
	} else {
		status = WblpWinbindNameToSidString(State, Name, sidBuffer, sizeof(sidBuffer));
		if (status) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to lookup SID for '%s'", Name);
		}
		GOTO_CLEANUP_ON_WBL_STATUS(status);
		sid = sidBuffer;
	}

	status = WblpAppendStringWithGrowthAsNeeded(State, Buffer, Size, Offset, MaxSize, sid);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

cleanup:
	return status;
}


static
WBL_STATUS
WblpNameListToSidStringList(
	/* IN */ WBL_STATE* State,
	/* IN */ const char* NameList,
	/* OUT OPTIONAL */ size_t* SidListCount,
	/* OUT */ char** SidList,
	/* OUT OPTIONAL */ size_t* SidListLength
	)
/**
 * Convert a list of names into a list of sids.
 *
 * @param State WBL state
 * @param NameList List of names or string SIDs, separated by commas.
 * @param SidListCount Returns number of SIDs in SidList.
 * @param SidList Returns buffer w/command-delimited list of string SIDs.
 * @param SidListSize Returns the strlen of SidList.
 *
 * @return WBL_STATUS_OK on success, or something else on failure.
 */
{
	WBL_STATUS status;
	char *current_name = NULL;
	const char *search_location;
	const char *comma;
	char* resultBuffer = NULL;
	size_t size = 4096;
	size_t offset = 0;
	size_t maxSize = 4096 * 16; /* 64k */
	size_t count = 0;

	status = WblpAllocate(size, (void**)&resultBuffer);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	resultBuffer[0] = 0;

	search_location = NameList;
	while ( (comma = strstr(search_location, ",")) != NULL ) {
		current_name = strndup(search_location, comma - search_location);
		if (NULL == current_name) {
			status = WBL_STATUS_MEMORY_INSUFFICIENT;
			goto cleanup;
		}

		status = WblpNameListToSidStringListAppend(State, current_name, &resultBuffer, &size, &offset, maxSize);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		status = WblpAppendStringWithGrowthAsNeeded(State, &resultBuffer, &size, &offset, maxSize, ",");
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		search_location = comma + 1;
		count++;
	}

	status = WblpNameListToSidStringListAppend(State, search_location, &resultBuffer, &size, &offset, maxSize);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	count++;

cleanup:
	SAFE_FREE(current_name);
	if (status) {
		WBL_FREE(resultBuffer);
	}

	if (SidListCount) {
		*SidListCount = resultBuffer ? count : 0;
	}
	*SidList = resultBuffer;
	if (SidListLength) {
		*SidListLength = resultBuffer ? offset : 0;
	}

	return status;
}


static
WBL_STATUS
WblpFetchRequiredMembershipSids(
	WBL_STATE* State
	)
{
	WBL_STATUS status = WBL_STATUS_OK;

	if (State->Config->RequiredMembership) {
		if (!State->RequiredMembershipSids) {
			status = WblpNameListToSidStringList(State,
							     State->Config->RequiredMembership,
							     NULL,
							     &State->RequiredMembershipSids,
							     NULL);
			if (status) {
				WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to get SIDs for membership: \"%s\"", State->Config->RequiredMembership);
			}
			GOTO_CLEANUP_ON_WBL_STATUS(status);
		}
	}

cleanup:
	return status;
}


WBL_STATUS
WblAuthorize(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	)
/**
 * Verify that the user is authorized to log on.  First, we make sure that
 * the user is valid/known.  Then we enforce the group membership requirement
 * if we did not go through 'authenticate'.
 *
 * @param State WBL state
 * @param Username The username
 *
 * @return WBL_STATUS_OK on success, WBL_STATUS_ACCOUNT_UNKNOWN if not
 * a user that we handle, WBL_STATUS_LOGON_RESTRICTED_ACCOUNT if the user
 * is not allowed to log on, or something else on failure.
 */
{
	WBL_STATUS status;
	WBL_PASSWD_INFO* pwd;
	char* sid = NULL;
	size_t sidsCount;
	char* sids = NULL;
	char* requiredSids = NULL;

	status = WblSetUsername(State, Username);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpGetPwByName(State, State->Username, &pwd);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	/* We now know that the user exists as one of ours */

	if (!State->Config->RequiredMembership) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "No membership check being enforced");
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	if (TestFlag(State->Flags, WBL_STATE_FLAG_AUTHORIZED)) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "User %s is already authorized", State->Username);
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	if (TestFlag(State->Flags, WBL_STATE_FLAG_UNAUTHORIZED)) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "User %s is already unauthorized", State->Username);
		status = WBL_STATUS_LOGON_RESTRICTED_ACCOUNT;
		goto cleanup;
	}

	/* We need to do a membership check */

	status = WblpWinbindUidToSid(State, pwd->pw_uid, &sid);
	if (status) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to get SID for user '%s' (uid = %d)", State->Username, pwd->pw_uid);
	}
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	WblpLog(State, WBL_LOG_LEVEL_DEBUG,
		"Checking membership of %s (%s) against: \"%s\"", State->Username, sid,
		State->Config->RequiredMembership);

	status = WblpWinbindGetUserSids(State, sid, &sidsCount, &sids);
	if (status) {
		WblpLog(State, WBL_LOG_LEVEL_ERROR, "Failed to get membership sids for user '%s' (sid = %s)", State->Username, sid);
	}
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpFetchRequiredMembershipSids(State);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	/* Now compare the lists... */
	if (WblpCheckSidRequirement(State, sidsCount, sids, State->RequiredMembershipSids)) {
		SetFlag(State->Flags, WBL_STATE_FLAG_AUTHORIZED);
		status = WBL_STATUS_OK;
	} else {
		SetFlag(State->Flags, WBL_STATE_FLAG_UNAUTHORIZED);
		status = WBL_STATUS_LOGON_RESTRICTED_ACCOUNT;
	}

cleanup:
	WBL_FREE(sid);
	WBL_FREE(requiredSids);
	SAFE_FREE(sids);

	WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Returning %d for user \"%s\"", status , Username);

	return status;
}


WBL_STATUS
WblLogoff(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* IN OPTIONAL */ const char* Krb5CCacheName
	)
/**
 * Log off a user
 *
 * @param State WBL state
 * @param Username The username
 * @param Krb5CCacheName Kerberos 5 Credentials Cache Name
 *
 * @return WBL_STATUS_OK, WBL_STATUS_ACCOUNT_UNKNOWN, or other.
 */
{
	WBL_STATUS status = WBL_STATUS_OK;
	WBL_PASSWD_INFO* pwd;

	if (TestFlag(State->Config->Flags, WBL_CONFIG_FLAG_KRB5_AUTH)) {
		status = WblSetUsername(State, Username);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		status = WblpGetPwByName(State, State->Username, &pwd);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		status = WblpWinbindLogoff(State, State->Username, pwd->pw_uid, Krb5CCacheName);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		status = WblApplyUserLogoutPolicies(State, State->Username);
		GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

cleanup:
	return status;
}


const char* WblStateGetCanonicalUserName(WBL_STATE* State)
{
	const char* result = NULL;
	if (WblStateIsAuthenticated(State)) {
		result = State->AuthInfo.CanonicalUserName;
	}
	return result;
}

WBL_STATUS WblQueryUserPrincipalName(WBL_STATE* State, const char* Username, const char** PrincipalName)
{
	WBL_STATUS status;
	const char* principal = NULL;
	fstring sid;

	status = WblSetUsername(State, Username);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (WblStateIsAuthenticated(State)) {
		principal = State->AuthInfo.UserPrincipalName;
		status = WBL_STATUS_OK;
	} else if (State->PrincipalName) {
		principal = State->PrincipalName;
		status = WBL_STATUS_OK;
	} else if (State->PrincipalNameStatus) {
		status = State->PrincipalNameStatus;
	} else {
		status = WblpWinbindNameToSidString(State, State->Username, sid, sizeof(sid));
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		status = WblpWinbindSidStringToPrincipalName(State, sid, &State->PrincipalName);
		if (status) {
			State->PrincipalNameStatus = status;
		}
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		principal = State->PrincipalName;
		status = WBL_STATUS_OK;
	}

cleanup:
	*PrincipalName = principal;
	return status;
}


const char* WblGetUserPrincipalName(WBL_STATE* State)
{
	const char* result = NULL;
	if (WblStateIsAuthenticated(State)) {
		result = State->AuthInfo.UserPrincipalName;
	}
	return result;
}

const char* WblStateGetHomeDirectory(WBL_STATE* State)
{
	const char* result = NULL;
	if (WblStateIsAuthenticated(State)) {
		result = State->AuthInfo.HomeDirectory;
	}
	return result;
}

const char* WblStateGetLogonScript(WBL_STATE* State)
{
	const char* result = NULL;
	if (WblStateIsAuthenticated(State)) {
		result = State->AuthInfo.LogonScript;
	}
	return result;
}

const char* WblStateGetLogonServer(WBL_STATE* State)
{
	const char* result = NULL;
	if (WblStateIsAuthenticated(State)) {
		result = State->AuthInfo.LogonServer;
	}
	return result;
}

const char* WblStateGetProfilePath(WBL_STATE* State)
{
	const char* result = NULL;
	if (WblStateIsAuthenticated(State)) {
		result = State->AuthInfo.ProfilePath;
	}
	return result;
}

const char* WblStateGetKrb5CCacheName(WBL_STATE* State)
{
	const char* result = NULL;
	if (WblStateIsAuthenticated(State)) {
		result = State->AuthInfo.Krb5CCacheName;
	}
	return result;
}

const char* WblStateGetKrb5ConfigPath(WBL_STATE* State)
{
	WBL_STATUS status;
	char *krb5_config = NULL;
	char* temp;
	char* parse_data;
	char* token;

	if (State->Krb5ConfigPath) {
		status = WBL_STATUS_OK;
		goto cleanup;
	}

	if (!WblStateIsAuthenticated(State)) {
		status = WBL_STATUS_INVALID_STATE;
		goto cleanup;
	}

	if (!State->TrustDomResult) {
		status = State->TrustDomStatus;
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		status = WblpWinbindListTrustedDomains(State, &State->TrustDomResult);
		if (status) {
			State->TrustDomStatus = status;
		}
		GOTO_CLEANUP_ON_WBL_STATUS(status);
	}

	status = WblpStrDup(&krb5_config, "KRB5_CONFIG=/etc/krb5.conf");
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	parse_data = State->TrustDomResult;
	while (krb5_config != NULL && (token = strtok_r(parse_data, "\n", &temp)) != NULL) {
		char *p;
		parse_data = NULL;
		p = strchr(token, '\\');
		if (p == 0) {
			WblpLog(State, WBL_LOG_LEVEL_ERROR, "Invalid domain response '%s'", token);
			status = WBL_STATUS_ERROR;
			goto cleanup;
		}
		*p = 0;

		if (asprintf(&p, "%s:%s/smb_krb5/krb5.conf.%s",
			     krb5_config, LOCKDIR, token) <= 0)
		{
			status = WBL_STATUS_MEMORY_INSUFFICIENT;
			goto cleanup;
		}
		free(krb5_config);
		krb5_config = p;
	}

	State->Krb5ConfigPath = krb5_config;
	krb5_config = NULL;
	status = WBL_STATUS_OK;

cleanup:
	if (krb5_config) {
		WBL_FREE(krb5_config);
	}

	return State->Krb5ConfigPath;
}

bool WblStateIsCachedLogon(WBL_STATE* State)
{
	return (TestFlag(State->Flags, WBL_STATE_FLAG_AUTHENTICATED) &&
		TestFlag(State->AuthInfo.UserFlags, LOGON_CACHED_ACCOUNT));
}

bool WblStateIsPasswordChanged(WBL_STATE* State)
{
	return TestFlag(State->Flags, WBL_STATE_FLAG_PASSWORD_CHANGED);
}

bool WblStateIsGraceLogon(WBL_STATE* State)
{
	return (TestFlag(State->Flags, WBL_STATE_FLAG_AUTHENTICATED) &&
		TestFlag(State->AuthInfo.UserFlags, LOGON_GRACE_LOGON));
}


bool WblStateIsKrb5ClockSkewFailure(WBL_STATE* State)
{
	return (TestFlag(State->Flags, WBL_STATE_FLAG_AUTHENTICATED) &&
		TestFlag(State->AuthInfo.UserFlags, LOGON_KRB5_FAIL_CLOCK_SKEW));
}


WBL_CONFIG_FLAGS WblStateGetConfigFlags(WBL_STATE* State)
{
	return TestFlag(State->Config->Flags, WBL_CONFIG_FLAG_EXTERNAL_MASK);
}


const char* WblStateGetConfigString(WBL_STATE* State, const char* Key)
{
	const char* result = NULL;
	if (State->Config->Dictionary) {
		result = dictionary_get(State->Config->Dictionary, (char*)Key, NULL);
	}
	return result;
}

bool WblStateGetNextPasswordChangeTime(WBL_STATE* State, time_t* NextChangeTime)
{
	bool result = false;
	time_t nextChangeTime = 0;

	if (TestFlag(State->Flags, WBL_STATE_FLAG_NEXT_CHANGE_TIME)) {
		nextChangeTime = State->AuthInfo.NextPasswordChangeTime;
		result = true;
	}

	*NextChangeTime = nextChangeTime;
	return result;
}

bool WblStateGetNextPasswordChangeTimeWarningSeconds(WBL_STATE* State, int* Seconds)
{
	*Seconds = State->Config->WarnPasswordChangeDays * SECONDS_PER_DAY;
	return true;
}

const WBL_PASSWORD_POLICY_INFO* WblStateGetPasswordPolicy(WBL_STATE* State)
{
	WBL_PASSWORD_POLICY_INFO* result = NULL;
	if (TestFlag(State->Flags, WBL_STATE_FLAG_POLICY_INFO)) {
		result = &State->AuthInfo.PasswordPolicy;
	}
	return result;
}


WBL_STATUS
WblAuthenticate(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* IN */ const char* Password
	)
{
	WBL_STATUS status, licenseStatus;
	uint32_t flags = WBFLAG_PAM_INFO3_TEXT | WBFLAG_PAM_CONTACT_TRUSTDOM;
	uid_t uid = -1;

	/* Reset flags */
	ClearFlag(State->Flags, WBL_STATE_FLAG_RESET_LOGON_MASK);
	State->TrustDomStatus = WBL_STATUS_OK;
	WBL_FREE(State->TrustDomResult);
	WBL_FREE(State->Krb5ConfigPath);

	status = WblSetUsername(State, Username);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (TestFlag(State->Config->Flags, WBL_CONFIG_FLAG_KRB5_AUTH | WBL_CONFIG_FLAG_CACHED_LOGIN)) {
		WBL_PASSWD_INFO* pwd;

		status = WblpGetPwByName(State, State->Username, &pwd);
		GOTO_CLEANUP_ON_WBL_STATUS(status);

		uid = pwd->pw_uid;
	}

	if (TestFlag(State->Config->Flags, WBL_CONFIG_FLAG_KRB5_AUTH)) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "enabling krb5 login flags");
		SetFlag(flags, WBFLAG_PAM_KRB5 | WBFLAG_PAM_FALLBACK_AFTER_KRB5);
	}

	if (TestFlag(State->Config->Flags, WBL_CONFIG_FLAG_CACHED_LOGIN)) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "enabling cached login flag");
		SetFlag(flags, WBFLAG_PAM_CACHED_LOGIN);
	}

	/* We always want to get back the canonical name */
	SetFlag(flags, WBFLAG_PAM_UNIX_NAME);

	if (State->Config->Krb5CCacheType) {
		WblpLog(State, WBL_LOG_LEVEL_DEBUG,
			"enabling request for a %s krb5 ccache type",
			State->Config->Krb5CCacheType);
	}

	status = WblpFetchRequiredMembershipSids(State);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpWinbindAuthenticate(State, State->Username, Password,
					 flags, uid, State->Config->Krb5CCacheType,
					 State->RequiredMembershipSids);
	/* TODO: Log a bunch of stuff */
	// Only check the license if the user will be able to login
	if(WblStatusIsAuthenticated(status))
	{
		licenseStatus = WblpCheckLicense(State);
		if(licenseStatus)
		{
			status = licenseStatus;
			GOTO_CLEANUP_ON_WBL_STATUS(status);
		}
	}
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	status = WblpCheckPasswordExpiration(State);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

cleanup:
	State->AuthenticateStatus = status;
	SetFlag(State->Flags, WBL_STATE_FLAG_AUTHENTICATED | WBL_STATE_FLAG_AUTHORIZED);

	return status;
}


bool WblStatusIsAuthenticated(WBL_STATUS Status)
{
	bool result = false;
	switch (Status) {
	case WBL_STATUS_OK:
	case WBL_STATUS_PASSWORD_EXPIRED:
	case WBL_STATUS_PASSWORD_MUST_CHANGE:
		result = true;
	}
	return result;
}

bool WblStatusIsPasswordChangeRequired(WBL_STATUS Status)
{
	bool result = false;
	switch (Status) {
	case WBL_STATUS_PASSWORD_EXPIRED:
	case WBL_STATUS_PASSWORD_MUST_CHANGE:
		result = true;
	}
	return result;
}

bool WblStatusHavePolicyRestriction(WBL_STATUS Status)
{
	bool result = false;
	switch (Status) {
	case WBL_STATUS_PASSWORD_RESTRICTION:
	case WBL_STATUS_PASSWORD_TOO_SHORT:
	case WBL_STATUS_PASSWORD_TOO_RECENT:
	case WBL_STATUS_PASSWORD_IN_HISTORY:
	case WBL_STATUS_PASSWORD_NOT_COMPLEX:
		result = true;
	}
	return result;
}

bool WblStateIsAuthenticated(WBL_STATE* State)
{
	return TestFlag(State->Flags, WBL_STATE_FLAG_AUTHENTICATED) ?
		WblStatusIsAuthenticated(State->AuthenticateStatus) : false;
}

bool WblStateIsPasswordChangeRequiredFromState(WBL_STATE* State)
{
	bool result = false;
	if (TestFlag(State->Flags, WBL_STATE_FLAG_AUTHENTICATED)) {
		result = WblStatusIsPasswordChangeRequired(State->AuthenticateStatus);
	}
	return result;
}

bool WblStateHavePolicyRestriction(WBL_STATE* State)
{
	return WblStatusHavePolicyRestriction(State->ChangePasswordStatus);
}

WBL_STATUS
WblChangePassword(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* IN */ const char* OldPassword,
	/* IN */ const char* NewPassword
	)
{
	WBL_STATUS status;
	uint32_t flags = 0;

	/* Reset flags */
	ClearFlag(State->Flags, WBL_STATE_FLAG_RESET_LOGON_MASK);

	status = WblSetUsername(State, Username);
	GOTO_CLEANUP_ON_WBL_STATUS(status);

	if (TestFlag(State->Config->Flags, WBL_CONFIG_FLAG_KRB5_AUTH)) {
		SetFlag(flags, WBFLAG_PAM_KRB5 | WBFLAG_PAM_CONTACT_TRUSTDOM);
	}

	status = WblpWinbindChangePassword(State, flags, State->Username, OldPassword, NewPassword);
	if(status == WBL_STATUS_OK) {
		SetFlag(State->Flags, WBL_STATE_FLAG_PASSWORD_CHANGED);
	}

cleanup:
	return status;
}

WBL_STATUS
WblIsKnownUser(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	)
{
	WBL_PASSWD_INFO* passwdInfo;
	return WblpGetPwByName(State, Username, &passwdInfo);
}

static void WblpPrintMessageHeader(char *buffer, size_t buffer_len)
{
	if(strlen(buffer) == 0)
	{
		snprintf(buffer, buffer_len, "Your password must meet the following restrictions:\n");
	}
}

static void WblpShowPasswordRestrictions(WBL_STATE *State)
{
	char buffer[1024] = "";
	int restriction_number = 0;
	const WBL_PASSWORD_POLICY_INFO *policy = WblStateGetPasswordPolicy(State);
	if(policy != NULL && policy->MinimumLength > 0)
	{
		WblpPrintMessageHeader(buffer, sizeof(buffer));
		snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
				"%d) at least %d characters long\n",
				++restriction_number, policy->MinimumLength);
	}
	if(policy != NULL && policy->History > 0)
	{
		WblpPrintMessageHeader(buffer, sizeof(buffer));
		snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
				"%d) unique from last %d password(s)\n",
				++restriction_number, policy->History);
	}
	if(policy != NULL && policy->Complexity)
	{
		WblpPrintMessageHeader(buffer, sizeof(buffer));
		snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
				"%d) contain capitals, numerals "
			    "or punctuation, but not "
			    "contain your account "
			    "or full name\n",
				++restriction_number);
	}
	snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
		       "Please type a different password. "
		       "Type a password that meets the domain requirements in "
		       "both text boxes.");
	WblpMessage(
		State,
		WBL_LOG_LEVEL_INFO,
		"%s",
		buffer);
}

void WblShowStatusMessages(WBL_STATE *State, WBL_STATUS Status)
{
	time_t nextChange;
	int warningTime;
	const char *username = WblGetUsername(State);
	if(!username)
	{
		username = "(unknown)";
	}

	switch(Status)
	{
		//Don't show an error message for these conditions
		case WBL_STATUS_ACCOUNT_UNKNOWN:
		case WBL_STATUS_LOGON_BAD:
		case WBL_STATUS_OK:
			break;

		//Give a description of the password restriction that failed, and
		//describe what all of the restrictions are.
		case WBL_STATUS_PASSWORD_RESTRICTION:
		case WBL_STATUS_PASSWORD_TOO_SHORT:
		case WBL_STATUS_PASSWORD_TOO_RECENT:
		case WBL_STATUS_PASSWORD_IN_HISTORY:
		case WBL_STATUS_PASSWORD_NOT_COMPLEX:
			WblpMessage(
				State,
				WBL_LOG_LEVEL_ERROR,
				"%s",
				WblStatusToString(Status));
			WblpShowPasswordRestrictions(State);
			break;

		//Just print a description of the error code
		default:
			WblpMessage(
				State,
				WBL_LOG_LEVEL_ERROR,
				"%s",
				WblStatusToString(Status));
			break;
	}
	if(WblStateIsGraceLogon(State))
	{
		WblpMessage(
			State,
			WBL_LOG_LEVEL_WARN,
			 "Grace login. "
			 "Please change your password as soon you're "
			 "online again");

		WblpLog(
			State,
			WBL_LOG_LEVEL_WARN,
			"User %s logged on using grace logon",
			username);
	}
	if(WblStateIsCachedLogon(State))
	{
		WblpMessage(
			State,
			WBL_LOG_LEVEL_WARN,
			 "Domain Controller unreachable, "
			 "using cached credentials instead. "
			 "Network resources may be unavailable");

		WblpLog(
			State,
			WBL_LOG_LEVEL_WARN,
		    "User %s logged on using cached credentials\n",
			username);
	}
	if(WblStateIsKrb5ClockSkewFailure(State))
	{
		WblpMessage(
			State,
			WBL_LOG_LEVEL_WARN,
			"Failed to establish your Kerberos Ticket cache "
			"due time differences\n"
			"with the domain controller.  "
			"Please verify the system time.\n");

		WblpLog(
			State,
			WBL_LOG_LEVEL_WARN,
		    "User %s: Clock skew when getting Krb5 TGT\n",
			username);
	}
	//Don't print a password expiration warning if the password is already
	//expired
	if(Status != WBL_STATUS_OK)
		return;
	if(WblStateGetNextPasswordChangeTime(State, &nextChange) &&
        WblStateGetNextPasswordChangeTimeWarningSeconds(State, &warningTime)) {
		time_t now = time(NULL);
		WblpLog(State, WBL_LOG_LEVEL_DEBUG, "Password for user %s will need to change at %d. It is now %d\n", username, nextChange, now);
		if(nextChange < now + warningTime && nextChange != 0)
		{
			//Print out a warning message
			int days = (nextChange - now) / 24 / 60 / 60;
			if(days < 1)
			{
				WblpMessage(
					State,
					WBL_LOG_LEVEL_WARN,
					"Your password will expire within 24 hours");
			}
			else if(days == 1)
			{
				WblpMessage(
					State,
					WBL_LOG_LEVEL_WARN,
					"Your password will expire in one day");
			}
			else
			{
				WblpMessage(
					State,
					WBL_LOG_LEVEL_WARN,
					"Your password will expire in %d days", days);
			}
		}
	}
}

#if defined(AIX)

static char 
mangle_digit(unsigned int digit)
{
    if (digit <= 9)
    {
	return '0' + digit;
    }
    else
    {
	return 'a' + (digit - 10);
    }
}

static unsigned int
unmangle_digit(char digit)
{
    if ('0' <= digit && digit <= '9')
    {
	return digit - '0';
    }
    else
    {
	return (digit - 'a') + 10;
    }
}

void
WblMangleAIX(unsigned int id, char buffer[9])
{
    buffer[0] = '_';
    int i;

    if (id <= 9999999)
    {
	sprintf(buffer + 1, "%07u", id);
    }
    else for (i = 7; i >= 1; i--)
    {
	buffer[i] = mangle_digit((id % 32) + (i == 1 ? 10 : 0));
	id /= 32;
    }
    
    buffer[8] = '\0';
}

unsigned int
WblUnmangleAIX(const char* buffer)
{
    int i;
    unsigned int id = 0;

    if (!WblIsMangledAIX(buffer))
    {
	return 0;
    }

    if ('0' <= buffer[1] && buffer[1] <= '9')
    {
	return (unsigned int) strtoul(buffer+1, NULL, 10); 
    }
    else for (i = 1; i <= 7; i++)
    {
	id = id * 32 + unmangle_digit(buffer[i] - (i == 1 ? 10 : 0));
    }

    return id;
}

bool
WblIsMangledAIX(const char* name)
{
    int i;

    if (!(name && name[0] == '_' && strlen(name) == 8))
	return false;
    
    if ('0' <= name[1] && name[1] <= '9')
    {
	for (i = 2; i < 8; i++)
	{
	    if (!('0' <= name[i] && name[i] <= '9'))
		return false;
	}

	return true;
    }
    else if ('a' <= name[1] && name[1] <= 'd')
    {
	for (i = 2; i < 8; i++)
	{
	    if (!(('0' <= name[i] && name[i] <= '9') ||
		  ('a' <= name[i] && name[i] <= 'v')))
		return false;
	}
	
	return true;
    }
    else
    {
	return false;
    }
}

#endif /* AIX */


/*
 * Copyright (c) Danilo Almeida <dalmeida@centeris.com> 2007
 * Copyright (c) Andrew Tridgell  <tridge@samba.org>   2000
 * Copyright (c) Tim Potter       <tpot@samba.org>     2000
 * Copyright (c) Andrew Bartlettt <abartlet@samba.org> 2002
 * Copyright (c) Guenther Deschner <gd@samba.org>      2005-2007
 * Copyright (c) Gerald (Jerry) Carter <jerry@samba.org> 2006-2007
 * Copyright (c) Jan Rêkorajski 1999.
 * Copyright (c) Andrew G. Morgan 1996-8.
 * Copyright (c) Alex O. Yuriev, 1996.
 * Copyright (c) Cristian Gafton 1996.
 * Copyright (C) Elliot Lee <sopwith@redhat.com> 1996, Red Hat Software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED `AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

