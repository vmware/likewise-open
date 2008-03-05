/* ex: set tabstop=4 shiftwidth=4: */
/* pam_lwidentity module

   Copyright Andrew Tridgell <tridge@samba.org> 2000
   Copyright Tim Potter <tpot@samba.org> 2000
   Copyright Andrew Bartlett <abartlet@samba.org> 2002
   Copyright Guenther Deschner <gd@samba.org> 2005-2007
   Copyright Gerald (Jerry) Carter <jerry@samba.org> 2006-2007

   largely based on pam_userdb by Cristian Gafton <gafton@redhat.com> also
   contains large slabs of code from pam_unix by Elliot Lee
   <sopwith@redhat.com> (see copyright below for full details)
*/

#include "pam_winbind.h"
#include "wbl.h"

#include <dirent.h>

/*
 * All Centeris LWI changes should go at the bottom of the file 
 * with any necessary forward declarations here.  This helps
 * with the upstream merges when we pull.
 */

static int _make_remark_v(pam_handle_t *pamh,
			  WBL_STATE *state,
			  int type,
			  const char *format,
			  va_list args);




/*************************************************************************
*************************************************************************/

#if defined(HPUX) || defined(NO_PAM_GETENV)

const char *pam_getenv_stub(pam_handle_t *pamh, const char *name)
{
	return name;
}

int pam_putenv_stub(pam_handle_t *pamh, const char *name_value)
{
	return 0;
}

const char *pam_getenv_wrapper(pam_handle_t *pamh, const char *name)
{
	void *handle;
	static const char *(*pam_getenv_ptr)(pam_handle_t *pamh, const char *name) = NULL;

	if(pam_getenv_ptr == NULL)
	{
		handle = dlopen(NULL, RTLD_LAZY);
		*(void **)(&pam_getenv_ptr) = dlsym(handle, "pam_getenv");

		if(pam_getenv_ptr == NULL)
		{
			*(void **)(&pam_getenv_ptr) = pam_getenv_stub;
		}
      
		dlclose(handle);
	}

	return (*pam_getenv_ptr)(pamh, name);
}

int pam_putenv_wrapper(pam_handle_t *pamh, const char *name)
{
	void *handle;
	static int (*pam_putenv_ptr)(pam_handle_t *pamh, const char *name) = NULL;

	if(pam_putenv_ptr == NULL)
	{
		handle = dlopen(NULL, RTLD_LAZY);
		*(void **)(&pam_putenv_ptr) = dlsym(handle, "pam_putenv");
      
		if(pam_putenv_ptr == NULL)
		{
			*(void **)(&pam_putenv_ptr) = pam_putenv_stub;
		}
      
		dlclose(handle);
	}

	return (*pam_putenv_ptr)(pamh, name);
}

#define pam_getenv pam_getenv_wrapper
#define pam_putenv pam_putenv_wrapper

#endif /* HPUX */
  
#define _PAM_LOG_FUNCTION_ENTER(function, pamh, state, flags) \
	do { \
		_pam_log_debug(pamh, state, LOG_DEBUG, "[pamh: %p] ENTER: " \
			       function " (flags: 0x%04x)", pamh, flags); \
		_pam_log_state(pamh, state); \
	} while (0)

#define _PAM_LOG_FUNCTION_LEAVE(function, pamh, state, retval) \
	do { \
		_pam_log_debug(pamh, state, LOG_DEBUG, "[pamh: %p] LEAVE: " \
			       function " returning %d", pamh, retval); \
		_pam_log_state(pamh, state); \
	} while (0)

/* data tokens */

#define MAX_PASSWD_TRIES	3

/*
 * Work around the pam API that has functions with void ** as parameters
 * These lead to strict aliasing warnings with gcc.
 */
static int _pam_get_item(const pam_handle_t *pamh,
			 int item_type,
			 const void *_item)
{
	const void **item = (const void **)_item;
	return pam_get_item(pamh, item_type, item);
}
static int _pam_get_data(const pam_handle_t *pamh,
			 const char *module_data_name,
			 const void *_data)
{
	const void **data = (const void **)_data;
	return pam_get_data(pamh, module_data_name, data);
}

/* some syslogging */

#ifdef HAVE_PAM_VSYSLOG
static void _pam_log_int(const pam_handle_t *pamh,
			 int err,
			 const char *format,
			 va_list args)
{
	pam_vsyslog(pamh, err, format, args);
}
#else
static void _pam_log_int(const pam_handle_t *pamh,
			 int err,
			 const char *format,
			 va_list args)
{
	char *format2 = NULL;
	const char *service;

	_pam_get_item(pamh, PAM_SERVICE, &service);

	format2 = (char *)malloc(strlen(MODULE_NAME)+strlen(format)+strlen(service)+5);
	if (format2 == NULL) {
		/* what else todo ? */
		vsyslog(err, format, args);
		return;
	}

	sprintf(format2, "%s(%s): %s", MODULE_NAME, service, format);
	vsyslog(err, format2, args);
	SAFE_FREE(format2);
}
#endif /* HAVE_PAM_VSYSLOG */

int WblStatusToPamError(WBL_STATUS status)
{
	switch(status)
	{
		case WBL_STATUS_OK:
			return PAM_SUCCESS;
		default:
		case WBL_STATUS_ERROR:
			return PAM_SYSTEM_ERR;
		case WBL_STATUS_MEMORY_INSUFFICIENT:
			return PAM_BUF_ERR;
		case WBL_STATUS_BUFFER_INSUFFICIENT:
			return PAM_BUF_ERR;
		case WBL_STATUS_LICENSE_ERROR:
		case WBL_STATUS_SERVER_UNAVAILABLE:
			return PAM_AUTHINFO_UNAVAIL;
		case WBL_STATUS_LOGON_BAD:
			return PAM_AUTH_ERR;
		case WBL_STATUS_ACCOUNT_UNKNOWN:
			return PAM_USER_UNKNOWN;
		case WBL_STATUS_ACCOUNT_INVALID:
		case WBL_STATUS_ACCOUNT_DISABLED:
		case WBL_STATUS_ACCOUNT_LOCKED_OUT:
		case WBL_STATUS_ACCOUNT_EXPIRED:
			return PAM_PERM_DENIED;

/* Account and policy logon restrictions: */
		case WBL_STATUS_LOGON_RESTRICTED_ACCOUNT:
			return PAM_ACCT_EXPIRED;
		case WBL_STATUS_LOGON_RESTRICTED_COMPUTER:
		case WBL_STATUS_LOGON_RESTRICTED_TIME:
		case WBL_STATUS_LOGON_TYPE_DENIED:
			return PAM_PERM_DENIED;

		case WBL_STATUS_PASSWORD_EXPIRED:
		case WBL_STATUS_PASSWORD_MUST_CHANGE:
			return PAM_NEW_AUTHTOK_REQD;

/* For password change: */
		case WBL_STATUS_PASSWORD_WRONG:
		case WBL_STATUS_PASSWORD_INVALID:
		case WBL_STATUS_PASSWORD_RESTRICTION:
			return PAM_AUTHTOK_ERR;

/* Specific password change restrictions: */
		case WBL_STATUS_PASSWORD_TOO_SHORT:
		case WBL_STATUS_PASSWORD_TOO_RECENT:
		case WBL_STATUS_PASSWORD_IN_HISTORY:
		case WBL_STATUS_PASSWORD_NOT_COMPLEX:
			return PAM_AUTHTOK_ERR;

		case WBL_STATUS_INVALID_STATE:
			return PAM_SYSTEM_ERR;
		case WBL_STATUS_ACCESS_DENIED:
			return PAM_AUTH_ERR;
	}
}

static bool _pam_message_is_silent(WBL_STATE *state)
{
	if (state == NULL)
	{
		return false;
	}
	if (WblStateGetConfigFlags(state) & WBL_CONFIG_FLAG_SILENT) {
		return true;
	}

	return false;
}

static void _pam_log(const pam_handle_t *pamh, WBL_STATE *state, int err, const char *format, ...) PRINTF_ATTRIBUTE(4,5);
static void _pam_log(const pam_handle_t *pamh, WBL_STATE *state, int err, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	_pam_log_int(pamh, err, format, args);
	va_end(args);
}

static bool _pam_log_is_debug_enabled(WBL_STATE *state)
{
	if(state == NULL)
		return false;

	if (!(WblStateGetConfigFlags(state) & WBL_CONFIG_FLAG_DEBUG)) {
		return false;
	}

	return true;
}

static bool _pam_log_is_debug_state_enabled(WBL_STATE *state)
{
	if(state == NULL)
		return false;

	if (!(WblStateGetConfigFlags(state) & WBL_CONFIG_FLAG_DEBUG_STATE)) {
		return false;
	}

	return _pam_log_is_debug_enabled(state);
}

static void _pam_log_debug(const pam_handle_t *pamh, WBL_STATE *state, int err, const char *format, ...) PRINTF_ATTRIBUTE(4,5);
static void _pam_log_debug(const pam_handle_t *pamh, WBL_STATE *state, int err, const char *format, ...)
{
	va_list args;

	if (!_pam_log_is_debug_enabled(state)) {
		return;
	}

	va_start(args, format);
	_pam_log_int(pamh, err, format, args);
	va_end(args);
}

static void _pam_log_state_datum(const pam_handle_t *pamh,
				 WBL_STATE *state,
				 int item_type,
				 const char *key,
				 int is_string)
{
	const void *data = NULL;
	if (item_type != 0) {
		pam_get_item(pamh, item_type, &data);
	} else {
		pam_get_data(pamh, key, &data);
	}
	if (data != NULL) {
		const char *type = (item_type != 0) ? "ITEM" : "DATA";
		if (is_string != 0) {
			_pam_log_debug(pamh, state, LOG_DEBUG,
				       "[pamh: %p] STATE: %s(%s) = \"%s\" (%p)",
				       pamh, type, key, (const char *)data,
				       data);
		} else {
			_pam_log_debug(pamh, state, LOG_DEBUG,
				       "[pamh: %p] STATE: %s(%s) = %p",
				       pamh, type, key, data);
		}
	}
}

#define _PAM_LOG_STATE_DATA_POINTER(pamh, state, module_data_name) \
	_pam_log_state_datum(pamh, state, 0, module_data_name, 0)

#define _PAM_LOG_STATE_DATA_STRING(pamh, state, module_data_name) \
	_pam_log_state_datum(pamh, state, 0, module_data_name, 1)

#define _PAM_LOG_STATE_ITEM_POINTER(pamh, state, item_type) \
	_pam_log_state_datum(pamh, state, item_type, #item_type, 0)

#define _PAM_LOG_STATE_ITEM_STRING(pamh, state, item_type) \
	_pam_log_state_datum(pamh, state, item_type, #item_type, 1)

#ifdef DEBUG_PASSWORD
#define _LOG_PASSWORD_AS_STRING 1
#else
#define _LOG_PASSWORD_AS_STRING 0
#endif

#define _PAM_LOG_STATE_ITEM_PASSWORD(pamh, state, item_type) \
	_pam_log_state_datum(pamh, state, item_type, #item_type, \
			     _LOG_PASSWORD_AS_STRING)

static void _pam_log_state(const pam_handle_t *pamh, WBL_STATE *state)
{
	if (!_pam_log_is_debug_state_enabled(state)) {
		return;
	}

	_PAM_LOG_STATE_ITEM_STRING(pamh, state, PAM_SERVICE);
	_PAM_LOG_STATE_ITEM_STRING(pamh, state, PAM_USER);
	_PAM_LOG_STATE_ITEM_STRING(pamh, state, PAM_TTY);
	_PAM_LOG_STATE_ITEM_STRING(pamh, state, PAM_RHOST);
	_PAM_LOG_STATE_ITEM_STRING(pamh, state, PAM_RUSER);
	_PAM_LOG_STATE_ITEM_PASSWORD(pamh, state, PAM_OLDAUTHTOK);
	_PAM_LOG_STATE_ITEM_PASSWORD(pamh, state, PAM_AUTHTOK);
	_PAM_LOG_STATE_ITEM_STRING(pamh, state, PAM_USER_PROMPT);
	_PAM_LOG_STATE_ITEM_POINTER(pamh, state, PAM_CONV);
#ifdef PAM_FAIL_DELAY
	_PAM_LOG_STATE_ITEM_POINTER(pamh, state, PAM_FAIL_DELAY);
#endif
#ifdef PAM_REPOSITORY
	_PAM_LOG_STATE_ITEM_POINTER(pamh, state, PAM_REPOSITORY);
#endif

	_PAM_LOG_STATE_DATA_STRING(pamh, state, PAM_WINBIND_HOMEDIR);
	_PAM_LOG_STATE_DATA_STRING(pamh, state, PAM_WINBIND_LOGONSCRIPT);
	_PAM_LOG_STATE_DATA_STRING(pamh, state, PAM_WINBIND_LOGONSERVER);
	_PAM_LOG_STATE_DATA_STRING(pamh, state, PAM_WINBIND_PROFILEPATH);
	_PAM_LOG_STATE_DATA_STRING(pamh, state,
				   PAM_WINBIND_NEW_AUTHTOK_REQD);
				   /* Use atoi to get PAM result code */
	_PAM_LOG_STATE_DATA_STRING(pamh, state,
				   PAM_WINBIND_NEW_AUTHTOK_REQD_DURING_AUTH);
	_PAM_LOG_STATE_DATA_POINTER(pamh, state, PAM_WINBIND_PWD_LAST_SET);
	_PAM_LOG_STATE_DATA_STRING(pamh, state, PAM_WINBIND_USER_PRINCIPAL_NAME);
}

//This is used to interact with WBL. The context must be the pam handle
void LogToSyslog(
	void* Context,
	WBL_LOG_LEVEL Level,
	const char* Format,
	va_list Args
	)
{
	const pam_handle_t *pamh = (const pam_handle_t *) Context;
	int pam_level;
	switch(Level)
	{
	case WBL_LOG_LEVEL_ERROR:
		pam_level = LOG_ERR;
		break;
	case WBL_LOG_LEVEL_WARN:
		pam_level = LOG_WARNING;
		break;
	case WBL_LOG_LEVEL_INFO:
		pam_level = LOG_NOTICE;
		break;
	case WBL_LOG_LEVEL_VERBOSE:
		pam_level = LOG_INFO;
		break;
	case WBL_LOG_LEVEL_NONE:
	case WBL_LOG_LEVEL_DEBUG:
	default:
		pam_level = LOG_DEBUG;
		break;
	}
	_pam_log_int(pamh,
			 pam_level,
			 Format,
			 Args);
}

//This is used to interact with WBL. The context must be the pam handle
void MessageToPam(
	void* Context,
	WBL_LOG_LEVEL Level,
	const char* Format,
	va_list Args
	)
{
	int pam_level;

	switch(Level)
	{
	case WBL_LOG_LEVEL_WARN:
	case WBL_LOG_LEVEL_ERROR:
		pam_level = PAM_ERROR_MSG;
		break;
	case WBL_LOG_LEVEL_NONE:
	case WBL_LOG_LEVEL_DEBUG:
	case WBL_LOG_LEVEL_VERBOSE:
	case WBL_LOG_LEVEL_INFO:
	default:
		pam_level = PAM_TEXT_INFO;
		break;
	}
	_make_remark_v((pam_handle_t *)Context,
			0, //The silent config flag is already handled inside of WBL
			pam_level,
			Format,
			Args);
}

static void _pam_winbind_cleanup_func(pam_handle_t *pamh,
				      void *data,
				      int error_status)
{
	WBL_STATE *state = NULL;
	WBL_STATUS wbl_result;

	wbl_result = WblStateCreate(&state, LogToSyslog, MessageToPam, pamh,
			NULL, PAM_WINBIND_CONFIG_FILE, 0, NULL);
	if(wbl_result != WBL_STATUS_OK)
		state = NULL;

	if (_pam_log_is_debug_state_enabled(state)) {
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "[pamh: %p] CLEAN: cleaning up PAM data %p "
			       "(error_status = %d)", pamh, data,
			       error_status);
	}
	SAFE_FREE(data);

	if(state != NULL)
		WblStateRelease(state);
}

static void _pam_winbind_state_release(pam_handle_t *pamh,
				      void *data,
				      int error_status)
{
	WBL_STATE *state = (WBL_STATE *)data;

	if(state != NULL)
		WblStateRelease(state);
}

static const struct ntstatus_errors {
	const char *ntstatus_string;
	const char *error_string;
} ntstatus_errors[] = {
	{"NT_STATUS_OK",
		"Success"},
	{"NT_STATUS_BACKUP_CONTROLLER",
		"No primary Domain Controler available"},
	{"NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND",
		"No domain controllers found"},
	{"NT_STATUS_NO_LOGON_SERVERS",
		"No logon servers"},
	{"NT_STATUS_PWD_TOO_SHORT",
		"Password too short"},
	{"NT_STATUS_PWD_TOO_RECENT",
		"The password of this user is too recent to change"},
	{"NT_STATUS_PWD_HISTORY_CONFLICT",
		"Password is already in password history"},
	{"NT_STATUS_PASSWORD_EXPIRED",
		"Your password has expired"},
	{"NT_STATUS_PASSWORD_MUST_CHANGE",
		"You need to change your password now"},
	{"NT_STATUS_INVALID_WORKSTATION",
		"You are not allowed to logon from this workstation"},
	{"NT_STATUS_INVALID_LOGON_HOURS",
		"You are not allowed to logon at this time"},
	{"NT_STATUS_ACCOUNT_EXPIRED",
		"Your account has expired. "
		"Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_ACCOUNT_DISABLED",
		"Your account is disabled. "
		"Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_ACCOUNT_LOCKED_OUT",
		"Your account has been locked. "
		"Please contact your System administrator"}, /* SCNR */
	{"NT_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT",
		"Invalid Trust Account"},
	{"NT_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT",
		"Invalid Trust Account"},
	{"NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT",
		"Invalid Trust Account"},
	{"NT_STATUS_ACCESS_DENIED",
		"Access is denied"},
	{NULL, NULL}
};

const char *_get_ntstatus_error_string(const char *nt_status_string)
{
	int i;
	for (i=0; ntstatus_errors[i].ntstatus_string != NULL; i++) {
		if (!strcasecmp(ntstatus_errors[i].ntstatus_string,
				nt_status_string)) {
			return ntstatus_errors[i].error_string;
		}
	}
	return NULL;
}

/* --- authentication management functions --- */

/* Attempt a conversation */

static int converse(pam_handle_t *pamh,
		    int nargs,
		    struct pam_message **message,
		    struct pam_response **response)
{
	int retval;
	struct pam_conv *conv;

	retval = _pam_get_item(pamh, PAM_CONV, &conv);
	if (retval == PAM_SUCCESS) {
		retval = conv->conv(nargs,
				    (const struct pam_message **)message,
				    response, conv->appdata_ptr);
	}

	return retval; /* propagate error status */
}


static int _make_remark(pam_handle_t * pamh,
			WBL_STATE *state,
			int type,
			const char *text)
{
	int retval = PAM_SUCCESS;

	struct pam_message *pmsg[1], msg[1];
	struct pam_response *resp;

	if (_pam_message_is_silent(state)) {
		return PAM_SUCCESS;
	}

	pmsg[0] = &msg[0];
	msg[0].msg = discard_const_p(char, text);
	msg[0].msg_style = type;

	resp = NULL;
	retval = converse(pamh, 1, pmsg, &resp);

	if (resp) {
		_pam_drop_reply(resp, 1);
	}
	return retval;
}

static int _make_remark_v(pam_handle_t *pamh,
			  WBL_STATE *state,
			  int type,
			  const char *format,
			  va_list args)
{
	char *var;
	int ret;

	ret = vasprintf(&var, format, args);
	if (ret < 0) {
		_pam_log(pamh, 0, LOG_ERR, "memory allocation failure");
		return ret;
	}

	ret = _make_remark(pamh, state, type, var);
	SAFE_FREE(var);
	return ret;
}

static int _make_remark_format(pam_handle_t * pamh, WBL_STATE *state, int type, const char *format, ...) PRINTF_ATTRIBUTE(4,5);
static int _make_remark_format(pam_handle_t * pamh, WBL_STATE *state, int type, const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = _make_remark_v(pamh, state, type, format, args);
	va_end(args);
	return ret;
}

static int pam_winbind_request(pam_handle_t *pamh,
			       WBL_STATE *state,
			       enum winbindd_cmd req_type,
			       struct winbindd_request *request,
			       struct winbindd_response *response)
{
	/* Fill in request and send down pipe */
	winbindd_init_request(request, req_type);
	
	if (winbind_write_sock(request, sizeof(*request), 0, 0) == -1) {
		_pam_log(pamh, state, LOG_ERR, 
			 "pam_lwidentity_request: write to socket failed!");
		winbind_close_sock();
		return PAM_SERVICE_ERR;
	}

	/* Wait for reply */
	if (winbindd_read_reply(response) == -1) {
		_pam_log(pamh, state, LOG_ERR, 
			 "pam_lwidentity_request: read from socket failed!");
		winbind_close_sock();
		return PAM_SERVICE_ERR;
	}

	/* We are done with the socket - close it and avoid mischeif */
	winbind_close_sock();

	/* Copy reply data from socket */
	if (response->result == WINBINDD_OK) {
		return PAM_SUCCESS;
	}

	/* no need to check for pam_error codes for getpwnam() */
	switch (req_type) {

		case WINBINDD_GETPWNAM:
		case WINBINDD_LOOKUPNAME:
			if (strlen(response->data.auth.nt_status_string) > 0) {
				_pam_log(pamh, state, LOG_ERR,
					 "request failed, NT error was %s",
					 response->data.auth.nt_status_string);
			} else {
				_pam_log(pamh, state, LOG_ERR, "request failed");
			}
			return PAM_USER_UNKNOWN;
		default:
			break;
	}

	if (response->data.auth.pam_error != PAM_SUCCESS) {
		if (response->data.auth.pam_error == PAM_USER_UNKNOWN) {
			_pam_log_debug(pamh, state, LOG_ERR, "request failed: %s, PAM error was %s (%d), NT error was %s", 
				       response->data.auth.error_string,
				       pam_strerror(pamh, response->data.auth.pam_error),
				       response->data.auth.pam_error,
				       response->data.auth.nt_status_string);
		} else {
			_pam_log(pamh, state, LOG_ERR, "request failed: %s, PAM error was %s (%d), NT error was %s", 
				 response->data.auth.error_string,
				 pam_strerror(pamh, response->data.auth.pam_error),
				 response->data.auth.pam_error,
				 response->data.auth.nt_status_string);
		}
		return response->data.auth.pam_error;
	}

	_pam_log(pamh, state, LOG_ERR, "request failed, but PAM error 0!");

	return PAM_SERVICE_ERR;
}

/**
 * put krb5ccname variable into environment
 *
 * @param pamh PAM handle
 * @param ctrl PAM winbind options.
 * @param krb5ccname env variable retrieved from winbindd.
 *
 * @return void.
 */

static void _pam_setup_krb5_env(pam_handle_t *pamh,
				WBL_STATE *state)
{
	char var[PATH_MAX];
	int ret;
	const char *krb5ccname = WblStateGetKrb5CCacheName(state);

	if (!krb5ccname || (strlen(krb5ccname) == 0)) {
		return;
	}

	_pam_log_debug(pamh, state, LOG_DEBUG,
		       "request returned KRB5CCNAME: %s", krb5ccname);

	if (snprintf(var, sizeof(var), "KRB5CCNAME=%s", krb5ccname) == -1) {
		return;
	}

	ret = pam_putenv(pamh, var);
	if (ret) {
		_pam_log(pamh, state, LOG_ERR,
			 "failed to set KRB5CCNAME to %s: %s",
			 var, pam_strerror(pamh, ret));
	}

#if defined(AIX) 
	/* SSH on AIX 5.3.0.50 ignores the environmental variables set
	   with pam_setenv. Calling setenv will work around this. */
	char *dupped = strdup(var);
	if(dupped != NULL)
	{
		if(putenv(dupped) != 0)
			free(dupped);
	}

	struct winbindd_request request;
	struct winbindd_response response;
	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.list_all_domains = true;
	if (pam_winbind_request(pamh, state, WINBINDD_LIST_TRUSTDOM, &request, &response) ==
	    NSS_STATUS_SUCCESS &&
	    response.extra_data.data != NULL)
	{
		char *krb5_config = strdup("KRB5_CONFIG=/etc/krb5.conf");
		char *extra_data = (char *)response.extra_data.data;
		char *p;
		char *token;

		if(krb5_config == NULL)
			goto krb_config_error;

		while(krb5_config != NULL && (token = strtok(extra_data, "\n")) != NULL) {
			extra_data = NULL;
			p = strchr(token, '\\');
			if (p == 0) {
				_pam_log(pamh, state, LOG_ERR, "Invalid domain response %s", token);
				goto krb_config_error;
			}
			*p = 0;

			asprintf(&p, "%s:%s/smb_krb5/krb5.conf.%s",
			    krb5_config, LOCKDIR, token);
			free(krb5_config);
			krb5_config = p;
		}

		if(krb5_config != NULL)
		{
			ret = pam_putenv(pamh, krb5_config);
			if(putenv(krb5_config) == 0)
				krb5_config = NULL;
		}

	krb_config_error:
		SAFE_FREE(response.extra_data.data);
		SAFE_FREE(krb5_config);
	}
#endif
}


/**
 * Set string into the PAM stack.
 *
 * @param pamh PAM handle
 * @param ctrl PAM winbind options.
 * @param data_name Key name for pam_set_data.
 * @param value String value.
 *
 * @return void.
 */

static void _pam_set_data_string(pam_handle_t *pamh,
				 WBL_STATE *state,
				 const char *data_name,
				 const char *value)
{
	int ret;
	char *dupped_value = NULL;

	if (!data_name || !value || (strlen(data_name) == 0) ||
	     (strlen(value) == 0)) {
		return;
	}

	dupped_value = strdup(value);
	ret = pam_set_data(pamh, data_name, dupped_value,
			   _pam_winbind_cleanup_func);
	if (ret != PAM_SUCCESS) {
		free(dupped_value);
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "Could not set data %s: %s\n",
			       data_name, pam_strerror(pamh, ret));
	}

}

/**
 * Set info3 strings into the PAM stack.
 *
 * @param pamh PAM handle
 * @param ctrl PAM winbind options.
 * @param data_name Key name for pam_set_data.
 * @param value String value.
 *
 * @return void.
 */

#define PROTECT_NULL_STR(x)	(((x) == NULL)? "(null)" : (x))

static void _pam_set_data_info3(pam_handle_t *pamh,
				WBL_STATE *state)
{
	_pam_set_data_string(pamh, state, PAM_WINBIND_HOMEDIR,
			     WblStateGetHomeDirectory(state));
	_pam_set_data_string(pamh, state, PAM_WINBIND_LOGONSCRIPT,
			     WblStateGetLogonScript(state));
	_pam_set_data_string(pamh, state, PAM_WINBIND_LOGONSERVER,
			     WblStateGetLogonServer(state));
	_pam_set_data_string(pamh, state, PAM_WINBIND_PROFILEPATH,
			     WblStateGetProfilePath(state));
	_pam_log_debug(pamh, state, LOG_DEBUG,
			     "Setting UPN to %s\n", PROTECT_NULL_STR(WblGetUserPrincipalName(state)));
	_pam_set_data_string(pamh, state, PAM_WINBIND_USER_PRINCIPAL_NAME, 
			     WblGetUserPrincipalName(state));
}

/**
 * Free info3 strings in the PAM stack.
 *
 * @param pamh PAM handle
 *
 * @return void.
 */

static void _pam_free_data_info3(pam_handle_t *pamh)
{
	pam_set_data(pamh, PAM_WINBIND_HOMEDIR, NULL, NULL);
	pam_set_data(pamh, PAM_WINBIND_LOGONSCRIPT, NULL, NULL);
	pam_set_data(pamh, PAM_WINBIND_LOGONSERVER, NULL, NULL);
	pam_set_data(pamh, PAM_WINBIND_PROFILEPATH, NULL, NULL);
}

/**
 * Send PAM_ERROR_MSG for cached or grace logons.
 *
 * @param pamh PAM handle
 * @param ctrl PAM winbind options.
 * @param username User in PAM request.
 * @param info3_user_flgs Info3 flags containing logon type bits.
 *
 * @return void.
 */

static void _pam_warn_logon_type(pam_handle_t *pamh,
				 WBL_STATE *state,
				 const char *username,
				 uint32_t info3_user_flgs)
{
	/* inform about logon type */
	if (PAM_WB_GRACE_LOGON(info3_user_flgs)) {

		_make_remark(pamh, state, PAM_ERROR_MSG,
			     "Grace login. "
			     "Please change your password as soon you're "
			     "online again");
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "User %s logged on using grace logon\n",
			       username);

	} else if (PAM_WB_CACHED_LOGON(info3_user_flgs)) {

		_make_remark(pamh, state, PAM_ERROR_MSG,
			     "Domain Controller unreachable, "
			     "using cached credentials instead. "
			     "Network resources may be unavailable");
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "User %s logged on using cached credentials\n",
			       username);
	}
}

/**
 * Send PAM_ERROR_MSG for krb5 errors.
 *
 * @param pamh PAM handle
 * @param ctrl PAM winbind options.
 * @param username User in PAM request.
 * @param info3_user_flgs Info3 flags containing logon type bits.
 *
 * @return void.
 */

static void _pam_warn_krb5_failure(pam_handle_t *pamh,
				   WBL_STATE *state,
				   const char *username,
				   uint32_t info3_user_flgs)
{
	if (PAM_WB_KRB5_CLOCK_SKEW(info3_user_flgs)) {
		_make_remark(pamh, state, PAM_ERROR_MSG,
			     "Failed to establish your Kerberos Ticket cache "
			     "due time differences\n"
			     "with the domain controller.  "
			     "Please verify the system time.\n");
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "User %s: Clock skew when getting Krb5 TGT\n",
			       username);
	}
}

/**
 * Compose Password Restriction String for a PAM_ERROR_MSG conversation.
 *
 * @param response The struct winbindd_response.
 *
 * @return string (caller needs to free).
 */

static char *_pam_compose_pwd_restriction_string(struct winbindd_response *response)
{
	char *str = NULL;
	size_t offset = 0, ret = 0, str_size = 1024;

	str = (char *)malloc(str_size);
	if (!str) {
		return NULL;
	}

	memset(str, '\0', str_size);

	offset = snprintf(str, str_size, "Your password ");
	if (offset == -1) {
		goto failed;
	}

	if (response->data.auth.policy.min_length_password > 0) {
		ret = snprintf(str+offset, str_size-offset,
			       "must be at least %d characters; ",
			       response->data.auth.policy.min_length_password);
		if (ret == -1) {
			goto failed;
		}
		offset += ret;
	}

	if (response->data.auth.policy.password_history > 0) {
		ret = snprintf(str+offset, str_size-offset,
			       "cannot repeat any of your previous %d "
			       "passwords; ",
			       response->data.auth.policy.password_history);
		if (ret == -1) {
			goto failed;
		}
		offset += ret;
	}

	if (response->data.auth.policy.password_properties &
	    DOMAIN_PASSWORD_COMPLEX) {
		ret = snprintf(str+offset, str_size-offset,
			       "must contain capitals, numerals "
			       "or punctuation; "
			       "and cannot contain your account "
			       "or full name; ");
		if (ret == -1) {
			goto failed;
		}
		offset += ret;
	}

	ret = snprintf(str+offset, str_size-offset,
		       "Please type a different password. "
		       "Type a password which meets these requirements in "
		       "both text boxes.");
	if (ret == -1) {
		goto failed;
	}

	return str;

 failed:
 	SAFE_FREE(str);
	return NULL;
}

static char *_pam_delete(register char *xx)
{
	_pam_overwrite(xx);
	_pam_drop(xx);
	return NULL;
}

/*
 * obtain a password from the user
 */

static int _winbind_read_password(pam_handle_t * pamh,
				  WBL_STATE *state,
				  unsigned int lctrl,
				  const char *comment,
				  const char *prompt1,
				  const char *prompt2,
				  const char **pass)
{
	int authtok_flag;
	int retval;
	const char *item;
	char *token;

	_pam_log(pamh, state, LOG_DEBUG, "getting password (0x%08x)", lctrl);

	/*
	 * make sure nothing inappropriate gets returned
	 */

	*pass = token = NULL;

	/*
	 * which authentication token are we getting?
	 */

	if (on(WINBIND__OLD_PASSWORD, lctrl)) {
		authtok_flag = PAM_OLDAUTHTOK;
	} else {
		authtok_flag = PAM_AUTHTOK;
	}

	/*
	 * should we obtain the password from a PAM item ?
	 */

	if (on(WINBIND_TRY_FIRST_PASS_ARG, lctrl) ||
	    on(WINBIND_USE_FIRST_PASS_ARG, lctrl) ||
		on(WBL_CONFIG_FLAG_TRY_FIRST_PASS, WblStateGetConfigFlags(state)) ||
		on(WBL_CONFIG_FLAG_USE_FIRST_PASS, WblStateGetConfigFlags(state))) {
		retval = _pam_get_item(pamh, authtok_flag, &item);
		if (retval != PAM_SUCCESS) {
			/* very strange. */
			_pam_log(pamh, state, LOG_ALERT,
				 "pam_get_item returned error "
				 "to unix-read-password");
			return retval;
		} else if (item != NULL) {	/* we have a password! */
			*pass = item;
			item = NULL;
			_pam_log(pamh, state, LOG_DEBUG,
				 "pam_get_item returned a password");
			return PAM_SUCCESS;
		} else if (on(WINBIND_USE_FIRST_PASS_ARG, lctrl) ||
				on(WBL_CONFIG_FLAG_USE_FIRST_PASS, WblStateGetConfigFlags(state))) {
			return PAM_AUTHTOK_RECOVER_ERR;	/* didn't work */
		} else if ((on(WINBIND_USE_AUTHTOK_ARG, lctrl) ||
				on(WBL_CONFIG_FLAG_USE_AUTHTOK, WblStateGetConfigFlags(state))) &&
				off(WINBIND__OLD_PASSWORD, lctrl)) {
			return PAM_AUTHTOK_RECOVER_ERR;
		}
	}
	/*
	 * getting here implies we will have to get the password from the
	 * user directly.
	 */

	{
		struct pam_message msg[3], *pmsg[3];
		struct pam_response *resp;
		int i, replies;

		/* prepare to converse */

		if (comment != NULL && !_pam_message_is_silent(state)) {
			pmsg[0] = &msg[0];
			msg[0].msg_style = PAM_TEXT_INFO;
			msg[0].msg = discard_const_p(char, comment);
			i = 1;
		} else {
			i = 0;
		}

		pmsg[i] = &msg[i];
		msg[i].msg_style = PAM_PROMPT_ECHO_OFF;
		msg[i++].msg = discard_const_p(char, prompt1);
		replies = 1;

		if (prompt2 != NULL) {
			pmsg[i] = &msg[i];
			msg[i].msg_style = PAM_PROMPT_ECHO_OFF;
			msg[i++].msg = discard_const_p(char, prompt2);
			++replies;
		}
		/* so call the conversation expecting i responses */
		resp = NULL;
		retval = converse(pamh, i, pmsg, &resp);
		if (resp == NULL) {
			if (retval == PAM_SUCCESS) {
				retval = PAM_AUTHTOK_RECOVER_ERR;
			}
			goto done;
		}
		if (retval != PAM_SUCCESS) {
			_pam_drop_reply(resp, i);
			goto done;
		}

		/* interpret the response */

		token = x_strdup(resp[i - replies].resp);
		if (!token) {
			_pam_log(pamh, state, LOG_NOTICE,
				 "could not recover "
				 "authentication token");
			retval = PAM_AUTHTOK_RECOVER_ERR;
			goto done;
		}

		if (replies == 2) {
			/* verify that password entered correctly */
			if (!resp[i - 1].resp ||
			    strcmp(token, resp[i - 1].resp)) {
				_pam_delete(token);	/* mistyped */
				retval = PAM_AUTHTOK_RECOVER_ERR;
				_make_remark(pamh, state, PAM_ERROR_MSG,
					     MISTYPED_PASS);
			}
		}

		/*
		 * tidy up the conversation (resp_retcode) is ignored
		 * -- what is it for anyway? AGM
		 */
		_pam_drop_reply(resp, i);
	}

 done:
	if (retval != PAM_SUCCESS) {
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "unable to obtain a password");
		return retval;
	}
	/* 'token' is the entered password */

	/* we store this password as an item */
#if defined(HPUX)
	/* HP-UX seems to clean the PAM_OLDAUTHTOK flag between the 
	   prelim check and the chauthtok calls.  So squirrel it away 
	   somewhere else */
	{
		char *s = strdup(token);

		if (!s) {
			 _pam_log(pamh, state, LOG_CRIT, "error manipulating current "
				  "password token");
			return PAM_BUF_ERR;
		}
		_pam_set_data_string(pamh, state, PAM_WINBIND_OLDAUTHTOK, s);
	}
#endif	/* HPUX */

	retval = pam_set_item(pamh, authtok_flag, token);
	_pam_delete(token);	/* clean it up */
	if (retval != PAM_SUCCESS ||
	    (retval = _pam_get_item(pamh, authtok_flag, &item)) != PAM_SUCCESS) {

		_pam_log(pamh, state, LOG_CRIT, "error manipulating password");
		return retval;
	}

	*pass = item;
	item = NULL;		/* break link to password */

	return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_authenticate(pam_handle_t *pamh, int flags,
			int argc, const char **argv)
{
	const char *username;
	const char *password;
	int retval = PAM_AUTH_ERR;
	char *new_authtok_required = NULL;
	char *user_principal_name = NULL;
	const char *additionalKeys[] = {"not_a_member_error", NULL};
	const char *notAMemberError;

	WBL_STATE *state = NULL;
	WBL_STATUS wbl_result;

	if(argc == 1 && !strcmp(argv[0], "set_default_repository"))
	{
#ifdef HAVE_STRUCT_PAM_REPOSITORY
		struct pam_repository *currentRepository = NULL;
		pam_get_item(pamh, PAM_REPOSITORY, &currentRepository);
		if(currentRepository == NULL)
		{
			struct pam_repository files = { "files", NULL, 0 };
			if(pam_set_item(pamh, PAM_REPOSITORY, &files) != PAM_SUCCESS)
			{
				_pam_log(pamh, state, LOG_WARNING,
					 "Could not set the pam repository to the default value of 'files'");
			}
		}
		else
		{
			_pam_log_debug(pamh, state, LOG_DEBUG,
					   "The pam repository is already set to %s", currentRepository->type);
		}
		return PAM_IGNORE;
#else
		_pam_log(pamh, state, LOG_ERR,
			 "set_default_repository is not a valid option on this system.");
		return PAM_SERVICE_ERR;
#endif
	}

	wbl_result = WblStateCreate(&state, LogToSyslog, MessageToPam, pamh,
			additionalKeys, PAM_WINBIND_CONFIG_FILE, argc, argv);
	if(wbl_result != WBL_STATUS_OK)
		return PAM_BUF_ERR;

	_PAM_LOG_FUNCTION_ENTER("pam_sm_authenticate", pamh, state, flags);

	/* Get the username */
	retval = pam_get_user(pamh, &username, NULL);
	if ((retval != PAM_SUCCESS) || (!username)) {
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "can not get the username");
		retval = PAM_SERVICE_ERR;
		goto out;
	}

	wbl_result = WblSetUsername(state, username);
	if(wbl_result != WBL_STATUS_OK)
	{
		retval = PAM_BUF_ERR;
		goto out;
	}

	retval = _winbind_read_password(pamh, state, 0, NULL,
					"Password: ", NULL,
					&password);

	if (retval != PAM_SUCCESS) {
		_pam_log(pamh, state, LOG_ERR,
			 "Could not retrieve user's password");
		retval = PAM_AUTHTOK_ERR;
		goto out;
	}

	/* Let's not give too much away in the log file */

#ifdef DEBUG_PASSWORD
	_pam_log_debug(pamh, state, LOG_INFO,
		       "Verify user '%s' with password '%s'",
		       WblGetUsername(state), password);
#else
	_pam_log_debug(pamh, state, LOG_INFO,
		       "Verify user '%s'", WblGetUsername(state));
#endif

	wbl_result = WblAuthenticate(state, NULL, password);
	if(wbl_result == WBL_STATUS_LOGON_RESTRICTED_ACCOUNT)
	{
		/* This account did not meet the allowed sid logon requirements. */
		retval = PAM_AUTH_ERR;
		notAMemberError = WblStateGetConfigString(state, "not_a_member_error");
		/* Use the user supplied error message if it exists, otherwise the
		 * user has to look in the winbind log to find out why the logon
		 * failed. */
		if(notAMemberError != NULL)
		{
			_make_remark(pamh, state, PAM_ERROR_MSG, notAMemberError);
		}
		goto out;
	}
	else
	{
		retval = WblStatusToPamError(wbl_result);
		WblShowStatusMessages(state, wbl_result);
	}
	if(!WblStatusIsAuthenticated(wbl_result))
	{
		goto out;
	}
	/* set some info3 info for other modules in the
	 * stack */
	_pam_set_data_info3(pamh, state);

	/* put krb5ccname into env */
	_pam_setup_krb5_env(pamh, state);

#if defined(SUNOS5) || defined(SUNOS4)
	/* Solaris does not store the current password in PAM_OLDAUTHTOK, it
	 * uses PAM_AUTHTOK for both the new and current password. However, Solaris
	 * does call authenticate before chauthtok prelim, so we'll just copy the
	 * current password to PAM_WINBIND_OLDAUTHTOK for later.
	 */
	_pam_log_debug(pamh, state, LOG_DEBUG,
			   "Copying authtok to winbind_oldauthtok for future password changes");
	{
		char *s = strdup(password);

		if (!s) {
			 _pam_log(pamh, state, LOG_CRIT, "error manipulating current "
				  "password token");
			retval = PAM_BUF_ERR;
			goto out;
		}
		_pam_set_data_string(pamh, state, PAM_WINBIND_OLDAUTHTOK, s);
		if (retval != PAM_SUCCESS) {
			_pam_log_debug(pamh, state, LOG_WARNING,
					   "Unable to copy authtok to winbind oldauthtok");
			free(s);
		}
	}
#endif

	if (WblStatusIsPasswordChangeRequired(wbl_result)) {
		char *new_authtok_required_during_auth = NULL;

#ifdef AIX
		char *expiration_env = NULL;
		struct passwd *pw = NULL;		

		/* Make sure we get the right capitalization */
		pw = getpwnam( WblGetUsername(state) );

		if(asprintf(&expiration_env, "%d:%s", getpid(), pw->pw_name) != -1)
		{
			setenv("LWIDENTITY_PASSWD_EXPIRED", expiration_env, 1);
			free(expiration_env);
		}
		expiration_env = NULL;
#endif
		if (!asprintf(&new_authtok_required, "%d", retval)) {
			retval = PAM_BUF_ERR;
			goto out;
		}

		if (pam_set_data(pamh, PAM_WINBIND_NEW_AUTHTOK_REQD, 
				 new_authtok_required, 
				 _pam_winbind_cleanup_func) 
		    != PAM_SUCCESS) 
		{
			retval = PAM_BUF_ERR;
			SAFE_FREE(new_authtok_required);
			goto out;
		}

		retval = PAM_SUCCESS;

		if (!asprintf(&new_authtok_required_during_auth, "%d", true)) {
			retval = PAM_BUF_ERR;
			goto out;
		}

		if (pam_set_data(pamh, PAM_WINBIND_NEW_AUTHTOK_REQD_DURING_AUTH, 
				 new_authtok_required_during_auth, 
				 _pam_winbind_cleanup_func) 
		    != PAM_SUCCESS) 
		{
			retval = PAM_BUF_ERR;
			SAFE_FREE(new_authtok_required_during_auth);
			goto out;
		}

		goto out;
	}

out:
	if (retval == PAM_SUCCESS) {
#ifdef AIX
		/* On AIX in the SSH from AIX 5.3.0.50,
		   pam_sm_open_session and pam_sm_setcred will be
		   called with the uid already set to the destination
		   user. The make homedir will fail due to permissions.
		   As a work around we create the homedirectory in
		   authenticate when the process still has superuser
		   access. */
		//Do not return a pam failure if the home directory cannot be
		//created
		wbl_result = WblCreateHomeDirectory(state, NULL);
#endif
	}

	SAFE_FREE(user_principal_name);

	if (!new_authtok_required) {
		pam_set_data(pamh, PAM_WINBIND_NEW_AUTHTOK_REQD, NULL, NULL);
#ifdef AIX
		unsetenv("LWIDENTITY_PASSWD_EXPIRED");
#endif
	}

	if (retval != PAM_SUCCESS) {
		_pam_free_data_info3(pamh);
	} else {
		char *temp = malloc(1);
		if (temp) {
			if (pam_set_data(pamh, PAM_WINBIND_DID_AUTHENTICATE, temp, _pam_winbind_cleanup_func) != PAM_SUCCESS) {
				free(temp);
			}
		}
	}

	_PAM_LOG_FUNCTION_LEAVE("pam_sm_authenticate", pamh, state, retval);

	WblStateRelease(state);

	return retval;
}

PAM_EXTERN
int pam_sm_setcred(pam_handle_t *pamh, int flags,
		   int argc, const char **argv)
{
	int ret = PAM_SYSTEM_ERR;
	WBL_STATE *state = NULL;
	WBL_STATUS wbl_result;
	const char *username;

	if(argc == 1 && !strcmp(argv[0], "set_default_repository"))
	{
		return PAM_IGNORE;
	}

	wbl_result = WblStateCreate(&state, LogToSyslog, MessageToPam, pamh,
			NULL, PAM_WINBIND_CONFIG_FILE, argc, argv);
	if(wbl_result != WBL_STATUS_OK)
		return PAM_BUF_ERR;

	_PAM_LOG_FUNCTION_ENTER("pam_sm_setcred", pamh, state, flags);

	switch (flags & ~PAM_SILENT) {

		case PAM_DELETE_CRED:
			ret = pam_sm_close_session(pamh, flags, argc, argv);
			break;
		case PAM_REFRESH_CRED:
			_pam_log_debug(pamh, state, LOG_WARNING,
				       "PAM_REFRESH_CRED not implemented");
			ret = PAM_SUCCESS;
			break;
		case PAM_REINITIALIZE_CRED:
			_pam_log_debug(pamh, state, LOG_WARNING,
				       "PAM_REINITIALIZE_CRED not implemented");
			ret = PAM_SUCCESS;
			break;
		case PAM_ESTABLISH_CRED:
			_pam_log_debug(pamh, state, LOG_WARNING,
				       "PAM_ESTABLISH_CRED not implemented");
			ret = PAM_SUCCESS;
			break;
		default:
			ret = PAM_SYSTEM_ERR;
			break;
	}

	if(pam_get_user(pamh, &username, NULL) == PAM_SUCCESS)
	{
		WblCreateHomeDirectory(state, username);
	}

	_PAM_LOG_FUNCTION_LEAVE("pam_sm_setcred", pamh, state, ret);
	WblStateRelease(state);

	return ret;
}

/*
 * Account management. We want to verify that the account exists
 * before returning PAM_SUCCESS
 */
PAM_EXTERN
int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags,
		   int argc, const char **argv)
{
	const char *username;
	int ret = PAM_USER_UNKNOWN;
	void *tmp = NULL;

	WBL_STATE *state = NULL;
	WBL_STATUS wbl_result;
	const char *additionalKeys[] = {"not_a_member_error", NULL};
	const char *notAMemberError;

	wbl_result = WblStateCreate(&state, LogToSyslog, MessageToPam, pamh,
			additionalKeys, PAM_WINBIND_CONFIG_FILE, argc, argv);
	if(wbl_result != WBL_STATUS_OK)
		return PAM_BUF_ERR;

	_PAM_LOG_FUNCTION_ENTER("pam_sm_acct_mgmt", pamh, state, flags);

	/* Get the username */
	ret = pam_get_user(pamh, &username, NULL);
	if ((ret != PAM_SUCCESS) || (!username)) {
		_pam_log_debug(pamh, state, LOG_DEBUG,
			       "can not get the username");
		ret = PAM_SERVICE_ERR;
		goto out;
	}

	/* Assume that we should ignore if winbind is not there. */
	if (!WblPing(state)) {
		_pam_log_debug(pamh, state, LOG_DEBUG, "cannot contact daemon");
		ret = PAM_IGNORE;
		goto out;
	}

	/* Verify the username */
	wbl_result = WblAuthorize(state, username);
	switch (wbl_result) {
	case WBL_STATUS_LOGON_RESTRICTED_ACCOUNT:
		/* This account did not meet the allowed sid logon requirements. */
		notAMemberError = WblStateGetConfigString(state, "not_a_member_error");
		/* Use the user supplied error message if it exists, otherwise the
		 * user has to look in the winbind log to find out why the logon
		 * failed. */
		if(notAMemberError != NULL)
		{
			_make_remark(pamh, state, PAM_ERROR_MSG, notAMemberError);
		}
		ret = PAM_AUTH_ERR;
		goto out;
	case WBL_STATUS_ACCOUNT_UNKNOWN:
		/* the user does not exist */
		_pam_log_debug(pamh, state, LOG_NOTICE, "user '%s' not found",
			       username);
		if (WblStateGetConfigFlags(state) & WBL_CONFIG_FLAG_UNKNOWN_OK) {
			ret = PAM_IGNORE;
			goto out;
		}
		ret = PAM_USER_UNKNOWN;
		goto out;
	case 0:
		pam_get_data(pamh, PAM_WINBIND_NEW_AUTHTOK_REQD,
			     (const void **)&tmp);
		if (tmp != NULL) {
			ret = atoi((const char *)tmp);
			switch (ret) {
			case PAM_AUTHTOK_EXPIRED:
				/* fall through, since new token is required in this case */
			case PAM_NEW_AUTHTOK_REQD:
				_pam_log(pamh, state, LOG_WARNING,
					 "pam_sm_acct_mgmt success but %s is set",
					 PAM_WINBIND_NEW_AUTHTOK_REQD);
				_pam_log(pamh, state, LOG_NOTICE,
					 "user '%s' needs new password",
					 username);
				/* PAM_AUTHTOKEN_REQD does not exist, but is documented in the manpage */
				ret = PAM_NEW_AUTHTOK_REQD;
				goto out;
			default:
				_pam_log(pamh, state, LOG_WARNING,
					 "pam_sm_acct_mgmt success");
				_pam_log(pamh, state, LOG_NOTICE,
					 "user '%s' granted access", username);
				ret = PAM_SUCCESS;
				goto out;
			}
		}

		/* Otherwise, the authentication looked good */
		_pam_log(pamh, state, LOG_NOTICE,
			 "user '%s' granted access", username);
		ret = PAM_SUCCESS;
		goto out;
	default:
		/* we don't know anything about this return value */
		_pam_log(pamh, state, LOG_ERR,
			 "internal module error (ret = %d, user = '%s')",
			 ret, username);
		ret = PAM_SERVICE_ERR;
		goto out;
	}

	/* should not be reached */
	ret = PAM_IGNORE;

 out:

	/* Some programs (e.g., sshd 3.9p1-8) call authenticate/acct_mgmt
	   with one pam handle and then call open_session/setcred with a
	   different pam handle.  This causes us to lose the value of
	   PAM_WINBIND_USER_PRINCIPAL_NAME by the time we get to
	   open_session/setcred.  Therefore, we want to trigger the .k5login
	   creation before that happens while still doing it as late
	   as possible.  This means that we do it here. */
	if (PAM_SUCCESS == ret || PAM_NEW_AUTHTOK_REQD == ret) {
		tmp = NULL;
		pam_get_data(pamh, PAM_WINBIND_USER_PRINCIPAL_NAME, (const void **)&tmp);
		//Do not return a pam failure if these functions fail
		wbl_result = WblCreateHomeDirectory(state, NULL);
		if(wbl_result == WBL_STATUS_OK) {
			WblCreateK5Login(state, WblGetUsername(state), tmp);
		}
	}

	_PAM_LOG_FUNCTION_LEAVE("pam_sm_acct_mgmt", pamh, state, ret);

	WblStateRelease(state);

	return ret;
}

PAM_EXTERN
int pam_sm_open_session(pam_handle_t *pamh, int flags,
			int argc, const char **argv)
{
	int ret = PAM_SYSTEM_ERR;
	WBL_STATE *state = NULL;
	WBL_STATUS wbl_result;
	const char *username;
        short bCreatedHomeDir = 1;

	wbl_result = WblStateCreate(&state, LogToSyslog, MessageToPam, pamh,
			NULL, PAM_WINBIND_CONFIG_FILE, argc, argv);
	if(wbl_result != WBL_STATUS_OK)
		return PAM_BUF_ERR;

	_PAM_LOG_FUNCTION_ENTER("pam_sm_open_session", pamh, state, flags);

	if(pam_get_user(pamh, &username, NULL) == PAM_SUCCESS)
	{
		wbl_result = WblCreateHomeDirectory(state, username);
                bCreatedHomeDir = (wbl_result == WBL_STATUS_OK);
	}
#if ! defined(AIX)
	if (wbl_result != WBL_STATUS_OK) {
		_pam_log(pamh, state, LOG_WARNING, "could not create home directory");
	}
	ret = PAM_SUCCESS;
#endif

        if (bCreatedHomeDir) {
           wbl_result = WblApplyUserLoginPolicies(state, username);
           if (wbl_result != WBL_STATUS_OK) {
              _pam_log(pamh, state, LOG_WARNING, "could not apply user policies");
              ret = PAM_SYSTEM_ERR;
           }
        }

 out:
	_PAM_LOG_FUNCTION_LEAVE("pam_sm_open_session", pamh, state, ret);

	WblStateRelease(state);

	return ret;
}

PAM_EXTERN
int pam_sm_close_session(pam_handle_t *pamh, int flags,
			 int argc, const char **argv)
{
	int retval = PAM_SUCCESS;
	WBL_STATE *state = NULL;
	WBL_STATUS wbl_result;
	const char *user;
	const char *ccname;

	wbl_result = WblStateCreate(&state, LogToSyslog, MessageToPam, pamh,
			NULL, PAM_WINBIND_CONFIG_FILE, argc, argv);
	if(wbl_result != WBL_STATUS_OK)
		return PAM_BUF_ERR;

	_PAM_LOG_FUNCTION_ENTER("pam_sm_close_session", pamh, state, flags);

	if (!(flags & PAM_DELETE_CRED)) {
		retval = PAM_SUCCESS;
		goto out;
	}

	retval = pam_get_user(pamh, &user, "Username: ");
	if (retval) {
		_pam_log(pamh, state, LOG_ERR,
			 "could not identify user");
		retval = PAM_USER_UNKNOWN;
		goto out;
	}

	if (user == NULL) {
		_pam_log(pamh, state, LOG_ERR,
			 "username was NULL!");
		retval = PAM_USER_UNKNOWN;
		goto out;
	}

	_pam_log_debug(pamh, state, LOG_DEBUG,
			   "username [%s] obtained", user);

	ccname = pam_getenv(pamh, "KRB5CCNAME");
	if (ccname == NULL) {
		_pam_log_debug(pamh, state, LOG_DEBUG,
				   "user has no KRB5CCNAME environment");
	}

	wbl_result = WblLogoff(state, user, ccname);
	retval = WblStatusToPamError(wbl_result);

out:
	_PAM_LOG_FUNCTION_LEAVE("pam_sm_close_session", pamh, state, retval);
	WblStateRelease(state);

	return retval;
}

PAM_EXTERN
int pam_sm_chauthtok(pam_handle_t * pamh, int flags,
		     int argc, const char **argv)
{
	unsigned int lctrl;
	int ret;
	unsigned int ctrl;

	/* <DO NOT free() THESE> */
	const char *user;
	char *pass_old, *pass_new;
	/* </DO NOT free() THESE> */

	char *Announce;

	int retry = 0;
	const char *username_ret = NULL;

	WBL_STATE *state = NULL;
	WBL_STATUS wbl_result;
	bool free_state = false;
	bool clear_pwd_last_set = false;
	const char *additionalKeys[] = {"chauthtok_success_code", NULL};
	const char *successCodeString;
	char *parseEnd = NULL;
	int successCode;

	ret = _pam_get_data( pamh, PAM_WINBIND_PWD_LAST_SET,
				 &state);
	if(ret != PAM_SUCCESS)
		state = NULL;
	if(state == NULL)
	{
		_pam_log_debug(pamh, state, LOG_DEBUG, "Creating wbl state");
		wbl_result = WblStateCreate(&state, LogToSyslog, MessageToPam, pamh,
				additionalKeys, PAM_WINBIND_CONFIG_FILE, argc, argv);
		if(wbl_result != WBL_STATUS_OK)
			return PAM_BUF_ERR;
		free_state = true;
	}

	_PAM_LOG_FUNCTION_ENTER("pam_sm_chauthtok", pamh, state, flags);

	if(WblStateIsPasswordChanged(state))
	{
		_pam_log_debug(pamh, state, LOG_DEBUG, "password already changed. Returning success");
		ret = PAM_SUCCESS;
		goto out;
	}

	/* clearing offline bit for the auth in the password change */
	ctrl &= ~WINBIND_CACHED_LOGIN;

	/*
	 * First get the name of a user
	 */
	ret = pam_get_user(pamh, &user, "Username: ");
	if (ret) {
		_pam_log(pamh, state, LOG_ERR,
			 "password - could not identify user");
		goto out;
	}

	if (user == NULL) {
		_pam_log(pamh, state, LOG_ERR, "username was NULL!");
		ret = PAM_USER_UNKNOWN;
		goto out;
	}

	_pam_log_debug(pamh, state, LOG_DEBUG, "username [%s] obtained", user);

	/* check if this is really a user in winbindd, not only in NSS */
	wbl_result = WblIsKnownUser(state, user);
	switch (wbl_result) {
		case WBL_STATUS_ACCOUNT_UNKNOWN:
			if (WblStateGetConfigFlags(state) & WBL_CONFIG_FLAG_UNKNOWN_OK) {
				ret = PAM_IGNORE;
			} else {
				ret = PAM_USER_UNKNOWN;
			}
			goto out;
		case WBL_STATUS_OK:
			break;
		default:
			ret = PAM_SYSTEM_ERR;
			goto out;
	}

#ifdef AIX
	/* If the PAM module attempts to change the password of a domain user,
	 * don't let the LAM module try after a failure.
	 */
	{
		char *expiration_env = NULL;
		struct passwd *pw = NULL;		

		/* Make sure we get the right capitalization */
		pw = getpwnam( user );

		if(asprintf(&expiration_env, "%d:%s", getpid(), pw->pw_name) != -1)
		{
			setenv("LWIDENTITY_PAM_CHAUTHTOK", expiration_env, 1);
			free(expiration_env);
		}
		expiration_env = NULL;
	}
#endif


	/*
	 * obtain and verify the current password (OLDAUTHTOK) for
	 * the user.
	 */

	if (flags & PAM_PRELIM_CHECK) {

		if(!WblStateIsAuthenticated(state))
		{
			/* instruct user what is happening */
#define greeting "Changing password for "
			Announce = (char *) malloc(sizeof(greeting) + strlen(user));
			if (Announce == NULL) {
				_pam_log(pamh, state, LOG_CRIT,
					 "password - out of memory");
				ret = PAM_BUF_ERR;
				goto out;
			}
			(void) strcpy(Announce, greeting);
			(void) strcpy(Announce + sizeof(greeting) - 1, user);
#undef greeting

			ret = _winbind_read_password(pamh, state, WINBIND__OLD_PASSWORD,
							Announce,
							"(current) NT password: ",
							NULL,
							(const char **) &pass_old);
			if (ret != PAM_SUCCESS) {
				_pam_log(pamh, state, LOG_NOTICE,
					 "password - (old) token not obtained");
				goto out;
			}

			/* verify that this is the password for this user */

			wbl_result = WblAuthenticate(state, user, pass_old);
			ret = WblStatusToPamError(wbl_result);
			pass_old = NULL;
			if(!WblStatusIsAuthenticated(wbl_result))
			{
				WblShowStatusMessages(state, wbl_result);
				goto out;
			}
		}

		if(free_state)
		{
			ret = pam_set_data(pamh, PAM_WINBIND_PWD_LAST_SET, state, _pam_winbind_state_release);
			if(ret == PAM_SUCCESS)
				free_state = false;
		}

		successCodeString = WblStateGetConfigString(state, "chauthtok_success_code");
		if(successCodeString != NULL)
		{
			successCode = (int)strtol(successCodeString, &parseEnd, 10);
			if(*parseEnd != '\0')
			{
				_pam_log_debug(pamh, state, LOG_NOTICE, "Unable to parse chauthtok_success_code '%s' into an integer", successCodeString);
				successCodeString = NULL;
			}
		}
		if(successCodeString == NULL)
		{
#ifdef AIX
			/* If PAM_SUCCESS is returned on AIX 5.3 TL4, AIX will not call back
			   with PAM_UPDATE_AUTHTOK; AIX will think the password change has
			   finished and was successful. It seems like any error code other
			   than PAM_SUCCESS will convince AIX to call back.

			   On AIX 5.3 TL6, PAM_SUCCESS can be returned, however most other
			   error codes (including PAM_NEW_AUTHTOK_REQD) will cause AIX to
			   think an unrecoverable error occurred.

			   PAM_IGNORE is the magic error code that works on both AIX 5.3
			   TL4 and AIX 5.3 TL6.
			 */
			successCode = PAM_IGNORE;
#else
			successCode = PAM_SUCCESS;
#endif
		}

		if (ret == PAM_SUCCESS && successCode != PAM_SUCCESS) {
			_pam_log_debug(pamh, state, LOG_DEBUG, "Returning %d instead of PAM_SUCCESS during chauthtok prelim stage.", successCode);
			ret = successCode;
		}
	} else if (flags & PAM_UPDATE_AUTHTOK) {

		/*
		 * obtain the proposed password
		 */

		/*
		 * get the old token back.
		 */

#if defined(HPUX) || defined(SUNOS5) || defined(SUNOS4)
		ret = _pam_get_data(pamh, PAM_WINBIND_OLDAUTHTOK, &pass_old);
#else
		ret = _pam_get_item(pamh, PAM_OLDAUTHTOK, &pass_old);
#endif
		if (ret != PAM_SUCCESS) {
			_pam_log(pamh, state, LOG_NOTICE,
				 "user not authenticated");
			goto out;
		}

		_pam_log_debug(pamh, state, LOG_DEBUG, "password [%s] obtained", pass_old);

		lctrl = 0;

		if (on(WINBIND_USE_AUTHTOK_ARG, lctrl)) {
			lctrl |= WINBIND_USE_FIRST_PASS_ARG;
		}
		retry = 0;
		ret = PAM_AUTHTOK_ERR;
		while ((ret != PAM_SUCCESS) && (retry++ < MAX_PASSWD_TRIES)) {
			/*
			 * use_authtok is to force the use of a previously entered
			 * password -- needed for pluggable password strength checking
			 */

			ret = _winbind_read_password(pamh, state, lctrl,
						     NULL,
						     "Enter new NT password: ",
						     "Retype new NT password: ",
						     (const char **)&pass_new);

			if (ret != PAM_SUCCESS) {
				_pam_log_debug(pamh, state, LOG_ALERT,
					       "password - "
					       "new password not obtained");
				pass_old = NULL;/* tidy up */
				goto out;
			}

			/*
			 * At this point we know who the user is and what they
			 * propose as their new password. Verify that the new
			 * password is acceptable.
			 */

			if (pass_new[0] == '\0') {/* "\0" password = NULL */
				pass_new = NULL;
			}
		}

		/*
		 * By reaching here we have approved the passwords and must now
		 * rebuild the password database file.
		 */

		wbl_result = WblChangePassword(state, user, pass_old, pass_new);
		WblShowStatusMessages(state, wbl_result);
		ret = WblStatusToPamError(wbl_result);
		if (ret) {
			_pam_overwrite(pass_new);
			_pam_overwrite(pass_old);
			pass_old = pass_new = NULL;
			goto out;
		}
#ifdef AIX
		unsetenv("LWIDENTITY_PASSWD_EXPIRED");
#endif

		wbl_result = WblAuthenticate(state, user, pass_new);
		WblShowStatusMessages(state, wbl_result);
		ret = WblStatusToPamError(wbl_result);
		_pam_overwrite(pass_new);
		_pam_overwrite(pass_old);
		pass_old = pass_new = NULL;

		if (ret == PAM_SUCCESS) {

			/* set some info3 info for other modules in the
			 * stack */
			_pam_set_data_info3(pamh, state);

			/* put krb5ccname into env */
			_pam_setup_krb5_env(pamh, state);

			username_ret = WblGetUsername(state);
			if (username_ret) {
				pam_set_item(pamh, PAM_USER,
						 username_ret);
				_pam_log_debug(pamh, state, LOG_INFO,
						   "Returned user was '%s'",
						   username_ret);
			}
		}
		_pam_log_debug(pamh, state, LOG_DEBUG, "password change succeeded");
		if ((WblStateGetConfigFlags(state) & WBL_CONFIG_FLAG_REMEMBER_CHPASS) != WBL_CONFIG_FLAG_REMEMBER_CHPASS)
		{
			_pam_log_debug(pamh, state, LOG_DEBUG, "Clearing wbl state");
			clear_pwd_last_set = true;
		}
	} else {
		ret = PAM_SERVICE_ERR;
	}

out:
	_PAM_LOG_FUNCTION_LEAVE("pam_sm_chauthtok", pamh, state, ret);
	if(free_state)
		WblStateRelease(state);
	if(clear_pwd_last_set)
	{
		/*The state saved in pwd_last_set would get cleared anyway when the
		 * pam handle is destroyed, but this will free it sooner.
		 */
		pam_set_data(pamh, PAM_WINBIND_PWD_LAST_SET, NULL, NULL);
	}

	return ret;
}

#ifdef PAM_STATIC

/* static module data */

struct pam_module _pam_winbind_modstruct = {
	MODULE_NAME,
	pam_sm_authenticate,
	pam_sm_setcred,
	pam_sm_acct_mgmt,
	pam_sm_open_session,
	pam_sm_close_session,
	pam_sm_chauthtok
};

#endif

/*
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
