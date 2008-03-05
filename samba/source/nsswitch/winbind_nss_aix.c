/* 
   Unix SMB/CIFS implementation.

   AIX loadable authentication module, providing identification and
   authentication routines against Samba winbind/Windows NT Domain

   Copyright (C) Tim Potter 2003
   Copyright (C) Steve Roylance 2003
   Copyright (C) Andrew Tridgell 2003-2004
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*

  To install this module copy nsswitch/WINBIND to /usr/lib/security and add
  "WINBIND" in /usr/lib/security/methods.cfg and /etc/security/user

  Note that this module also provides authentication and password
  changing routines, so you do not need to install the winbind PAM
  module.

  see 
  http://publib16.boulder.ibm.com/doc_link/en_US/a_doc_lib/aixprggd/kernextc/sec_load_mod.htm
  for some information in the interface that this module implements

  Many thanks to Julianne Haugh for explaining some of the finer
  details of this interface.

  To debug this module use uess_test.c (which you can get from tridge)
  or set "options=debug" in /usr/lib/security/methods.cfg

*/

#include "winbind_nss_aix.h"
#include "winbind_client.h"
#include <usersec.h>
#include <syslog.h>

/* S_PGID is apparently not defined in the AIX 5.2 headers,
 * but is available in AIX 5.3 (_AIXVERSION_530) */
#ifndef S_PGID
#define S_PGID "pgid"
#endif

#define HANDLE_ERRORS(ret) do { \
	if ((ret) == NSS_STATUS_NOTFOUND) { \
		logit(LOG_ERR, "Returned NSS_STATUS_NOTFOUND\n");\
		errno = ENOENT; \
		return NULL; \
	} else if ((ret) != NSS_STATUS_SUCCESS) { \
		logit(LOG_ERR, "Returned error %d\n", ret);\
		errno = EIO; \
		return NULL; \
	} \
} while (0)

#define STRCPY_RET(dest, src) \
do { \
	if (strlen(src)+1 > sizeof(dest)) { errno = EINVAL; return -1; } \
	strcpy(dest, src); \
} while (0)

#define STRCPY_RETNULL(dest, src) \
do { \
	if (strlen(src)+1 > sizeof(dest)) { errno = EINVAL; return NULL; } \
	strcpy(dest, src); \
} while (0)

/* 
   each function uses one of the following lists of memory, declared 
   static in each backend method. This allows the function to destroy
   the memory when that backend is called next time
*/
struct mem_list {
	struct mem_list *next, *prev;
	void *p;
};


/* allocate some memory on a mem_list */
static void *list_alloc(struct mem_list **list, size_t size)
{
	struct mem_list *m;
	m = malloc(sizeof(*m));
	if (!m) {
		errno = ENOMEM;
		return NULL;
	}
	m->p = calloc(1, size);
	if (!m->p) {
		errno = ENOMEM;
		free(m);
		return NULL;
	}
	m->next = *list;
	m->prev = NULL;
	if (*list) {
		(*list)->prev = m;
	}
	(*list) = m;
	return m->p;
}

static void *list_realloc(struct mem_list**list, void* p, size_t size)
{
	struct mem_list *m;
	
	for (m = *list; m; m = m->next)	{
		if (m->p == p) {
			void* tmp = m->p;
		        tmp = realloc(tmp, size);
			if (!tmp) {
				errno = ENOMEM;
				return NULL;
			}
			m->p = tmp;
			return m->p;
		}
	}
	errno = EINVAL;
	return NULL;
}

/* duplicate a string using list_alloc() */
static char *list_strdup(struct mem_list **list, const char *s)
{
	char *ret = list_alloc(list, strlen(s)+1);
	if (!ret) return NULL;
	strcpy(ret, s);
	return ret;
}

/* destroy a mem_list */
static void list_destroy(struct mem_list **list)
{
	struct mem_list *m, *next;
	for (m=*list; m; m=next) {
		next = m->next;
		free(m->p);
		free(m);
	}
	(*list) = NULL;
}

/* enable this to log which entry points have not been
  completed yet */
#define LOG_UNIMPLEMENTED_CALLS 0

#define WB_AIX_ENCODED '_'

static int debug_enabled = false;
static int log_silent = false;

static void logit_base(int err, const char *format, va_list args)
{
	if (log_silent) {
		return;
	}

	char *format2 = NULL;

	format2 = malloc(strlen(MODULE_NAME)+strlen(format)+5);
	if (format2 == NULL) {
		/* what else todo ? */
		vsyslog(err, format, args);
		return;
	}

	sprintf(format2, "%s: %s", MODULE_NAME, format);
	vsyslog(err, format2, args);
	SAFE_FREE(format2);
}

static void logit_debug(int err, const char *format, ...)
{
	va_list args;

	if (!debug_enabled) {
		return;
	}
	
	va_start(args, format);

	logit_base(err, format, args);

	va_end(args);
}

static void logit(int err, const char *format, ...)
{
	va_list args;

	va_start(args, format);

	logit_base(err, format, args);
	
	va_end(args);
}

const char *get_conf_item_string(int ctrl,
				 dictionary *d,
				 const char *item, 
				 int config_flag)
{
	const char *parm_opt = NULL;
	char *key = NULL;	

	if (!(ctrl & config_flag)) {
		goto out;
	}

	if (d != NULL) {
		
		if (!asprintf(&key, "global:%s", item)) {
			goto out;
		}
		
		parm_opt = iniparser_getstr(d, key);
		SAFE_FREE(key);
	}
	
	if (d != NULL) {
		logit_debug(LOG_DEBUG, "CONFIG file: %s '%s'\n", item, parm_opt);
	}

out:
	return parm_opt;
}

const char *get_winbind_homedir_umask_from_config(int ctrl, dictionary *d)
{
	return get_conf_item_string(ctrl, d, "umask", WINBIND_HOMEDIR_UMASK);
}

const char *get_krb5_cc_type_from_config(int ctrl, dictionary *d)
{
	return get_conf_item_string(ctrl, d, "krb5_ccache_type", WINBIND_KRB5_CCACHE_TYPE);
}

const char *get_member_from_config(int ctrl, dictionary *d)
{
	const char *ret = NULL;
	ret = get_conf_item_string(ctrl, d, "require_membership_of", WINBIND_REQUIRED_MEMBERSHIP);
	if (ret) {
		return ret;
	}
	return get_conf_item_string(ctrl, d, "require-membership-of", WINBIND_REQUIRED_MEMBERSHIP);
}

/* put krb5ccname variable into environment
 *
 * @param ctrl PAM winbind options.
 * @param krb5ccname env variable retrieved from winbindd.
 *
 * @return void.
 */

static void setup_krb5_env(int ctrl, const char *krb5ccname)
{
	int ret;

	char *var = malloc(PATH_MAX);
	if(var == NULL)
	{
		logit_debug(LOG_DEBUG, "setup_krb5_env failed to allocate need memory");
		goto error;
	}

	if (!(ctrl & WINBIND_KRB5_AUTH)) {
		logit_debug(LOG_DEBUG, "WINBIND_KRB5_AUTH not set");
		goto error;
	}

	logit_debug(LOG_DEBUG, "WINBIND_KRB5_AUTH set");

	if (!krb5ccname || (strlen(krb5ccname) == 0)) {
		goto error;
	}

	logit_debug(LOG_DEBUG, "request returned KRB5CCNAME: %s", krb5ccname);
	
	if (snprintf(var, PATH_MAX, "KRB5CCNAME=%s", krb5ccname) == -1) {
		goto error;
	}
	
	ret = putenv(var);
	if (ret) {
		logit(LOG_ERR, "failed to set KRB5CCNAME to %s: %d", var, ret);
	}
	else
	{
		// We set to NULL so we do not free memory at the end of function...
		var = NULL;
	}

	struct winbindd_request request;
	struct winbindd_response response;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.list_all_domains = true;

	if (winbindd_request_response(WINBINDD_LIST_TRUSTDOM, &request, &response) == NSS_STATUS_SUCCESS &&
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
				logit(LOG_ERR, "Invalid domain response %s", token);
				goto krb_config_error;
			}
			*p = 0;

			asprintf(&p, "%s:%s/smb_krb5/krb5.conf.%s", krb5_config, LOCKDIR, token);
			free(krb5_config);
			krb5_config = p;
		}

		if(krb5_config != NULL)
		{
			ret = putenv(krb5_config);
			if(ret == 0)
				logit_debug(LOG_DEBUG, "%s", krb5_config);
				krb5_config = NULL;
		}

	krb_config_error:
		SAFE_FREE(response.extra_data.data);
		SAFE_FREE(krb5_config);
	}

error:

	SAFE_FREE(var);

	return;
}	

static int directory_exist( const char* path )
{
	struct stat st;
  
	if ( !path ) {
	  return false;
	}

	if ( stat( path, &st ) != 0 ) {
	  return false;
	}
  
	if ( !S_ISDIR(st.st_mode) ) {
	  return false;
	}

	return true;
}

static void _lam_create_k5login(const char *homedir, uid_t uid, gid_t gid, const char *principal_name)
{
	struct stat st;
	char *path = NULL;
	char *temp = NULL;
	char *data = NULL;
	char *lower = NULL;
	int data_len;
	int data_out;
	int fd = -1;
	int i;
	bool need_unlink = false;
	
	if (!homedir || !homedir[0] || !principal_name || !principal_name[0]) {
		goto out;
	}

	if (asprintf(&path, "%s/%s", homedir, ".k5login") <= 0) {
		goto out;
	}

	/* If the file exists, we are done */
	if (stat(path, &st) == 0) {
		goto out;
	}

	/* Make sure we got ENOENT */
	if (errno != ENOENT) {
		goto out;
	}

	if (asprintf(&temp, "%s.lwidentity.temp", path) <= 0) {
		goto out;
	}

	fd = open(temp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		goto out;
	}

	need_unlink = true;

	if (fchown(fd, uid, gid) < 0) {
		goto out;
	}

	lower = strdup(principal_name);
	if (!lower)
	{
		goto out;
	}
	for (i = 0; lower[i] && lower[i] != '@'; i++)
	{
		lower[i] = tolower(lower[i]);
	}
	
	data_len = asprintf(&data, "%s\n%s\n", principal_name, lower);
	if (data_len <= 0) {
		goto out;
	}

	data_out = write(fd, data, data_len);
	if (data_out < data_len) {
		goto out;
	}

	if (fsync(fd) < 0) {
		goto out;
	}

	close(fd);

	if (rename(temp, path) < 0) {
		goto out;
	}

	need_unlink = false;

	logit_debug(LOG_DEBUG, "created .k5login for uid %d", uid);


out:
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
}

static int
mkhomedir(const char *username, int ctrl, dictionary *d, char * user_principal_name)
{
  	struct passwd *pwd = NULL;
	char tok[1024], create_dir[1024];
	char *s, *p;
	const char *umask_str;
	int mkdir_umask = 0022;
	struct stat statbuf;
	int ret;
	char parentDir[PATH_MAX];

	if (!(ctrl & (WINBIND_CREATE_HOMEDIR | WINBIND_CREATE_K5LOGIN))) {
		ret = 0;
		goto out;
	}

	if (!username) {
		logit(LOG_ERR, "invalid username\n");
		ret = ENOENT;
		goto out;
	}

	/* FIXME: shouldn't we be using our own getpwnam directly? */
	pwd = getpwnam(username);
	if (pwd == NULL) {
		logit(LOG_ERR, "getpwnam call failed for user: %s\n", username);
		ret = ENOENT;
		goto out;
	}

	/* make sure we have a path to work with */
	if ( !pwd->pw_dir || strlen(pwd->pw_dir)==0 ) {
		ret = 0;
		goto out;
	}
	
	logit_debug(LOG_DEBUG, "homedir is: %s\n", pwd->pw_dir);
	
	if (directory_exist(pwd->pw_dir)) {
		ret = 0;
		goto out;
	}

	if (ctrl & WINBIND_CREATE_HOMEDIR) {
		/* Zero memory so that we don't have to worry
		   about adding the terminating NULL */

		create_dir[0] = '\0';
		tok[0] = '\0';

		strncat( create_dir, "/", sizeof(create_dir)-1);
		strncat( tok, pwd->pw_dir, sizeof(tok)-1 );
		p = tok+1;


		if(ctrl & WINBIND_HOMEDIR_UMASK)
		{
			umask_str = get_winbind_homedir_umask_from_config(ctrl, d);
			mkdir_umask = strtol(umask_str, NULL, 0);
		}

		while ( p && *p ) {
			mode_t mode = 0755;
			int dsize;
	    
			if ( (s = strchr(p, '/')) != NULL ) {
				*s = '\0';
			}
	  
			dsize = strlen(create_dir)+1;
			strncat( create_dir, p, sizeof(create_dir)-dsize );
			dsize += strlen(p);
			strncat( create_dir, "/", sizeof(create_dir)-dsize);
	   
			if (!directory_exist(create_dir)) {
				logit_debug(LOG_DEBUG, "dir: attempting to mkdir [%s] to [%04o]\n", create_dir, mode);
				if (mkdir(create_dir, mode) != 0) {
					logit(LOG_ERR, "could not create dir: %s (%s)\n",
						  create_dir, 
						  strerror(errno));
					ret = ENOENT;
					goto out;
				}
				memset(parentDir, 0, sizeof(parentDir));
				snprintf(parentDir, sizeof(parentDir), "%s/..", create_dir);
				if(stat(parentDir, &statbuf) != 0){
					logit(LOG_ERR, "could not stat parent directory: %s (%s)\n",
						  parentDir, 
						  strerror(errno));
					ret = ENOENT;
					goto out;
				}
				if(chown(create_dir, statbuf.st_uid, statbuf.st_gid) != 0){
					logit(LOG_ERR, "failed to chown [%s] to (%d,%d)\n", create_dir, statbuf.st_uid, statbuf.st_gid);
					ret = ENOENT;
					goto out;
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

		logit_debug(LOG_DEBUG, "dir: attempting to chown [%s] to (%d,%d)\n", create_dir, pwd->pw_uid, pwd->pw_gid);
		if (chown(create_dir, pwd->pw_uid, pwd->pw_gid) != 0) {
			logit(LOG_ERR, "failed to chown user homedir: %s (%s)\n",
				  create_dir, 
				  strerror(errno));
			ret = ENOENT;
			goto out;
		}

		if (chmod(create_dir, 0755 & ~mkdir_umask) != 0){
			logit(LOG_ERR, "failed to chmod user homedir file: %s (%s)\n",
						   create_dir, strerror(errno));
			ret = ENOENT;
			goto out;
		}
	}

	ret = 0;

out:
	if ((0 == ret) && pwd && pwd->pw_dir && pwd->pw_dir[0] && (ctrl & WINBIND_CREATE_K5LOGIN) && (ctrl & WINBIND_KRB5_AUTH)) {
		if (user_principal_name && user_principal_name[0]) {
			_lam_create_k5login(pwd->pw_dir, pwd->pw_uid, pwd->pw_gid, user_principal_name);
		}
	}

	return ret;
}

static int _pam_parse(dictionary **result_d)
{
	int ctrl = 0;
	const char *config_file = NULL;
	dictionary *d = NULL;

	if (config_file == NULL) {
		config_file = PAM_WINBIND_CONFIG_FILE;
	}

	d = iniparser_load(config_file);
	if (d == NULL) {
		goto error;
	}

	/* Currently not supported by LAM */
	/* The logging done by LAM is not secure so we do not want to turn it on here...*/
	if (iniparser_getboolean(d, "global:debug", false)) {
		ctrl |= WINBIND_DEBUG_ARG;
		 debug_enabled = true;
	}

	/* Currently not supported by LAM */
	if (iniparser_getboolean(d, "global:debug_state", false)) {
		ctrl |= WINBIND_DEBUG_STATE;
	}

	if (iniparser_getboolean(d, "global:cached_login", false)) {
		ctrl |= WINBIND_CACHED_LOGIN;
	}

	if (iniparser_getboolean(d, "global:krb5_auth", false)) {
		ctrl |= WINBIND_KRB5_AUTH;
	}

	if (iniparser_getboolean(d, "global:silent", false)) {
		ctrl |= WINBIND_SILENT;
		log_silent = true;
	}

	if (iniparser_getstr(d, "global:krb5_ccache_type") != NULL) {
		ctrl |= WINBIND_KRB5_CCACHE_TYPE;
	}

	if ((iniparser_getstr(d, "global:require-membership-of") != NULL) ||
	    (iniparser_getstr(d, "global:require_membership_of") != NULL)) {
		ctrl |= WINBIND_REQUIRED_MEMBERSHIP;
	}

	/* Currently not supported by LAM */
	if (iniparser_getboolean(d, "global:try_first_pass", false)) {
		ctrl |= WINBIND_TRY_FIRST_PASS_ARG;
	}

	if (iniparser_getboolean(d, "global:create_homedir", false)) {
		ctrl |= WINBIND_CREATE_HOMEDIR;
	}

	if (iniparser_getboolean(d, "global:create_k5login", false)) {
		ctrl |= WINBIND_CREATE_K5LOGIN;
	}

	/* Currently not supported by LAM */
	if (iniparser_getstr(d, "global:skel") != NULL) {
	        ctrl |= WINBIND_SKEL_DIR;
	}

	if (iniparser_getstr(d, "global:umask") != NULL) {
	        ctrl |= WINBIND_HOMEDIR_UMASK;
	}

	if (result_d) {
		*result_d = d;
	} else {
		if (d) {
			iniparser_freedict(d);
		}
	}

	return ctrl;
	
error:
	if (d) {
		iniparser_freedict(d);
	}
	
	return -1;
}

static bool safe_append_string(char *dest,
							   const char *src,
							   int dest_buffer_size)
/**
 * Append a string, making sure not to overflow and to always return a NULL-terminated
 * string.
 *
 * @param dest Destination string buffer (must already be NULL-terminated).
 * @param src Source string buffer.
 * @param dest_buffer_size Size of dest buffer in bytes.
 *
 * @return false if dest buffer is not big enough (no bytes copied), true on success.
 */
{
	int dest_length = strlen(dest);
	int src_length = strlen(src);

	if ( dest_length + src_length + 1 > dest_buffer_size ) {
		return false;
	}

	memcpy(dest + dest_length, src, src_length + 1);
	return true;
}

static bool winbind_name_to_sid_string(const char *name,
									   char *sid_list_buffer,
									   int sid_list_buffer_size)
/*
 * Convert a name into a SID string, appending it to a buffer.
 *
 * @param name Name to convert.
 * @param sid_list_buffer Where to append the string sid.
 * @param sid_list_buffer Size of sid_list_buffer (in bytes).
 *
 * @return false on failure, true on success.
 */
{
	const char* sid_string;
	struct winbindd_response sid_response;
	bool result = false;

	/* lookup name? */ 
	if (IS_SID_STRING(name)) {
		sid_string = name;
	} else {
		struct winbindd_request sid_request;

		ZERO_STRUCT(sid_request);
		ZERO_STRUCT(sid_response);

		logit_debug(LOG_DEBUG, "no sid given, looking up: %s\n", name);
		
		/* fortunatly winbindd can handle non-separated names */
		strncpy(sid_request.data.name.name, name,
				sizeof(sid_request.data.name.name) - 1);

		if (winbindd_request_response(WINBINDD_LOOKUPNAME, &sid_request, &sid_response) != NSS_STATUS_SUCCESS) {
			logit(LOG_ERR, "could not lookup name: %s\n", name); 
			goto out;
		}

		sid_string = sid_response.data.sid.sid;
	}

	if (!safe_append_string(sid_list_buffer, sid_string, sid_list_buffer_size)) {
		goto out;
	}

	result = true;

out:
	winbindd_free_response(&sid_response);
	return result;
}

static bool winbind_name_list_to_sid_string_list(const char *name_list,
												 char *sid_list_buffer,
												 int sid_list_buffer_size)
/**
 * Convert a list of names into a list of sids.
 *
 * @param name_list List of names or string sids, separated by commas.
 * @param sid_list_buffer Where to put the list of string sids.
 * @param sid_list_buffer Size of sid_list_buffer (in bytes).
 *
 * @return false on failure, true on success.
 */
{
	bool result = false;
	char *current_name = NULL;
	const char *search_location;
	const char *comma;

	if ( sid_list_buffer_size > 0 ) {
		sid_list_buffer[0] = 0;
	}

	search_location = name_list;
	while ( (comma = strstr(search_location, ",")) != NULL ) {
		
		current_name = strndup(search_location, comma - search_location);
		
		if (NULL == current_name) {
			goto out;
		}

		if (!winbind_name_to_sid_string(current_name, sid_list_buffer, sid_list_buffer_size)) {
			goto out;
		}

		SAFE_FREE(current_name);

		if (!safe_append_string(sid_list_buffer, ",", sid_list_buffer_size)) {
			goto out;
		}

		search_location = comma + 1;
	}

	if (!winbind_name_to_sid_string(search_location, sid_list_buffer, sid_list_buffer_size)) {
		goto out;
	}

	result = true;

out:
	SAFE_FREE(current_name);
	return result;
}

/* talk to winbindd */
static int winbind_auth_request(int ctrl, 
								dictionary *d,
								const char *user, 
								const char *pass, 
								const char *member, 
								const char *cctype)
{
	int ret_value = AUTH_FAILURE;
	NSS_STATUS result;

	struct winbindd_request request;
	struct winbindd_response response;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	strncpy(request.data.auth.user, user, 
		sizeof(request.data.auth.user)-1);

	strncpy(request.data.auth.pass, pass, 
		sizeof(request.data.auth.pass)-1);

	request.data.auth.krb5_cc_type[0] = '\0';
	request.data.auth.uid = -1;
	
	request.flags = WBFLAG_PAM_INFO3_TEXT | WBFLAG_PAM_CONTACT_TRUSTDOM;

	if (ctrl & (WINBIND_KRB5_AUTH|WINBIND_CACHED_LOGIN)) {
		struct passwd *pwd = NULL;

		pwd = getpwnam(user);
		if (pwd == NULL) {
			return  AUTH_NOTFOUND;
		}
		
		request.data.auth.uid = pwd->pw_uid;
	}

	if (ctrl & WINBIND_KRB5_AUTH) {
		logit_debug(LOG_DEBUG, "Enabling krb5 login flag\n for user: %s\n", user);
		request.flags |= WBFLAG_PAM_KRB5 | WBFLAG_PAM_FALLBACK_AFTER_KRB5;
	}

	if (ctrl & WINBIND_CACHED_LOGIN) {
		logit_debug(LOG_DEBUG, "Enabling cached login flag\n for user: %s\n", user);
		request.flags |= WBFLAG_PAM_CACHED_LOGIN;
	}

	if (cctype != NULL) {
		strncpy(request.data.auth.krb5_cc_type, cctype, 
			sizeof(request.data.auth.krb5_cc_type) - 1);
		logit_debug(LOG_DEBUG, "Enable request for a %s krb5 ccache\n", cctype);
	}

	request.data.auth.require_membership_of_sid[0] = '\0';

	if (member != NULL) {

		if (!winbind_name_list_to_sid_string_list(member,
												  request.data.auth.require_membership_of_sid,
									   			  sizeof(request.data.auth.require_membership_of_sid))) {

			logit(LOG_ERR, "failed to serialize membership of sid \"%s\"\n", member);
			
			return AUTH_FAILURE;
		}
	}

	result = winbindd_request_response(WINBINDD_PAM_AUTH, &request, &response);

	logit_debug(LOG_DEBUG, "auth result %d for '%s'\n", result, user);

	if (result == NSS_STATUS_SUCCESS) {
		/* put krb5ccname into env */
		setup_krb5_env(ctrl, response.data.auth.krb5ccname);
		
		char *user_principal_name = NULL;
		if (response.data.auth.krb5upn[0]) {
			user_principal_name = strdup(response.data.auth.krb5upn);
		}

		mkhomedir(user, ctrl, d, user_principal_name);
		errno = 0;
		ret_value = AUTH_SUCCESS;
		SAFE_FREE(user_principal_name);
	}
	
	const char *ntstatus = response.data.auth.nt_status_string; 
	if (!strcasecmp(ntstatus, "NT_STATUS_PASSWORD_EXPIRED") || 
		!strcasecmp(ntstatus, "NT_STATUS_PASSWORD_MUST_CHANGE")) 
	{
		logit_debug(LOG_DEBUG, "Password needs to changes...");

		char *expiration_env = NULL;
		struct passwd *pw = NULL;		

		/* Make sure we get the right capitalization */
		pw = getpwnam( user );

		if(pw && (asprintf(&expiration_env, "%d:%s", getpid(), pw->pw_name) != -1))
		{
			char *user_principal_name = NULL;
			if (response.data.auth.krb5upn[0]) {
				user_principal_name = strdup(response.data.auth.krb5upn);
			}

			mkhomedir(user, ctrl, d, user_principal_name);

			setenv("LWIDENTITY_PASSWD_EXPIRED", expiration_env, 1);
			logit_debug(LOG_DEBUG, "%s", expiration_env);
			free(expiration_env);
			SAFE_FREE(user_principal_name);
		}
		
		expiration_env = NULL;
		ret_value = AUTH_PASSWORD_MUST_CHANGE;
	}
	else
	{
		unsetenv("LWIDENTITY_PASSWD_EXPIRED");
	}

	winbindd_free_response(&response);

	return ret_value;
}


#if 0
/* free a passwd structure */
static void free_pwd(struct passwd *pwd)
{
	free(pwd->pw_name);
	free(pwd->pw_passwd);
	free(pwd->pw_gecos);
	free(pwd->pw_dir);
	free(pwd->pw_shell);
	free(pwd);
}

/* free a group structure */
static void free_grp(struct group *grp)
{
	int i;

	free(grp->gr_name);
	free(grp->gr_passwd);
	
	if (!grp->gr_mem) {
		free(grp);
		return;
	}
	
	for (i=0; grp->gr_mem[i]; i++) {
		free(grp->gr_mem[i]);
	}

	free(grp->gr_mem);
	free(grp);
}
#endif

/* replace commas with nulls, and null terminate */
static void replace_commas(char *s)
{
	char *p, *p0=s;
	for (p=strchr(s, ','); p; p = strchr(p+1, ',')) {
		*p=0;
		p0 = p+1;
	}

	p0[strlen(p0)+1] = 0;
}

/* the decode_*() routines are used to cope with the fact that AIX 5.2
   and below cannot handle user or group names longer than 8
   characters in some interfaces. We use the normalize method to
   provide a mapping to a username that fits, by using the form '_UID'
   or '_GID'.

   this only works if you can guarantee that the WB_AIX_ENCODED char
   is not used as the first char of any other username
*/
static unsigned decode_id(const char *name)
{
    return WblUnmangleAIX(name);
}

static struct passwd *__wb_aix_getpwuid(struct mem_list** list, uid_t uid);

static char *decode_user(const char *name)
{
	struct passwd *pwd;
	unsigned id;
	char *ret;
	struct mem_list* list = NULL;
	
	if (!WblIsMangledAIX(name))
	    goto cleanup;

	id = decode_id(name);

	if (!id)
	    goto cleanup;

	pwd = __wb_aix_getpwuid(&list, id);

	if (!pwd) {
	goto cleanup;
	}
	ret = strdup(pwd->pw_name);

	logit_debug(LOG_DEBUG, "decoded '%s' -> '%s'\n", name, ret);

	list_destroy(&list);
	return ret;

cleanup:
	list_destroy(&list);
	return NULL;
}

/*
  fill a struct passwd from a winbindd_pw struct, allocating as a single block
*/
static struct passwd *fill_pwent(struct mem_list **list, struct winbindd_pw *pw)
{
	struct passwd *result;

	result = list_alloc(list, sizeof(struct passwd));
	if (!result) {
		return NULL;
	}

	result->pw_uid = pw->pw_uid;
	result->pw_gid = pw->pw_gid;
	result->pw_name   = list_strdup(list, pw->pw_name);
	if (!result->pw_name)
		goto cleanup;
	result->pw_passwd = list_strdup(list, pw->pw_passwd);
	if (!result->pw_passwd)
		goto cleanup;
	result->pw_gecos  = list_strdup(list, pw->pw_gecos);
	if (!result->pw_gecos)
		goto cleanup;
	result->pw_dir    = list_strdup(list, pw->pw_dir);
	if (!result->pw_dir)
		goto cleanup;
	result->pw_shell  = list_strdup(list, pw->pw_shell);
	if (!result->pw_shell)
		goto cleanup;
	
	return result;
	
cleanup:
	return NULL;
}

/*
  fill a struct group from a winbindd_pw struct, allocating as a single block
*/
static struct group *fill_grent(struct mem_list **list, struct winbindd_gr *gr, char *gr_mem)
{
	int i;
	struct group *result;
	char *p, *name;

	result = list_alloc(list, sizeof(struct group));
	if (!result) {
		errno = ENOMEM;
		return NULL;
	}

	result->gr_gid = gr->gr_gid;
	result->gr_name   = list_strdup(list, gr->gr_name);
	if (!result->gr_name)
		goto cleanup;
	result->gr_passwd = list_strdup(list, gr->gr_passwd);
	if (!result->gr_passwd)
		goto cleanup;
	
	/* Group membership */
	if ((gr->num_gr_mem < 0) || !gr_mem) {
		gr->num_gr_mem = 0;
	}
	
	if (gr->num_gr_mem == 0) {
		/* Group is empty */		
		return result;
	}
	
	result->gr_mem = (char **)list_alloc(list, sizeof(char *) * (gr->num_gr_mem+1));
	if (!result->gr_mem)
		goto cleanup;

	/* Start looking at extra data */
	i=0;
	for (name = strtok_r(gr_mem, ",", &p); 
	     name; 
	     name = strtok_r(NULL, ",", &p)) {
		if (i == gr->num_gr_mem) {
			break;
		}
		result->gr_mem[i] = list_strdup(list, name);
		if (!result->gr_mem[i])
			goto cleanup;
		i++;
	}

	/* Terminate list */
	result->gr_mem[i] = NULL;

	return result;
	
cleanup:
	return NULL;
}

/* take a group id and return a filled struct group */	
static struct group *__wb_aix_getgrgid(struct mem_list** list, gid_t gid)
{
	struct winbindd_response response;
	struct winbindd_request request;
	struct group *grp;
	NSS_STATUS ret;
	
	logit_debug(LOG_DEBUG, "getgrgid %d\n", gid);
	
	ZERO_STRUCT(response);
	ZERO_STRUCT(request);
	
	request.data.gid = gid;
	
	ret = winbindd_request_response(WINBINDD_GETGRGID, &request, &response);
	
	logit_debug(LOG_DEBUG, "getgrgid ret=%d\n", ret);

	HANDLE_ERRORS(ret);

	grp = fill_grent(list, &response.data.gr, response.extra_data.data);	

	winbindd_free_response(&response);

	return grp;
}

static struct mem_list* getgrgid_memlist;
static struct group* wb_aix_getgrgid(gid_t gid)
{
	list_destroy(&getgrgid_memlist);
	return __wb_aix_getgrgid(&getgrgid_memlist, gid);
}


/* take a group name and return a filled struct group */
static struct group *__wb_aix_getgrnam(struct mem_list** list, const char *name)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	struct group *grp;

	if (*name == WB_AIX_ENCODED) {
		return __wb_aix_getgrgid(list, decode_id(name));
	}

	logit_debug(LOG_DEBUG, "getgrnam '%s'\n", name);

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);

	STRCPY_RETNULL(request.data.groupname, name);

	ret = winbindd_request_response(WINBINDD_GETGRNAM, &request, &response);
	
	HANDLE_ERRORS(ret);

	grp = fill_grent(list, &response.data.gr, response.extra_data.data);

	winbindd_free_response(&response);

	return grp;
}

static struct mem_list* getgrnam_memlist;
static struct group *wb_aix_getgrnam(const char *name)
{
	list_destroy(&getgrnam_memlist);
	return __wb_aix_getgrnam(&getgrnam_memlist, name);
}


/* this call doesn't have to fill in the gr_mem, but we do anyway
   for simplicity */
static struct group *__wb_aix_getgracct(struct mem_list** list, void *id, int type)
{
	logit_debug(LOG_DEBUG, "getgracct %d\n", type);
	if (type == 1) {
		return __wb_aix_getgrnam(list, (char *)id);
	}
	if (type == 0) {
		return __wb_aix_getgrgid(list, *(int *)id);
	}
	errno = EINVAL;
	return NULL;
}

static struct mem_list* getgracct_memlist;
static struct group* wb_aix_getgracct(void *id, int type)
{
	list_destroy(&getgracct_memlist);
	return __wb_aix_getgracct(&getgracct_memlist, id, type);
}

/* take a username and return a string containing a comma-separated
   list of group id numbers to which the user belongs */
static char *__wb_aix_getgrset(struct mem_list** list, char *user)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	int i, idx;
	char *tmpbuf;
	int num_gids;
	gid_t *gid_list;
	char *r_user = user;
	
	if (*user == WB_AIX_ENCODED) {
		r_user = decode_user(r_user);
	}
	
	logit_debug(LOG_DEBUG, "getgrset %s '%s'\n", user, r_user);
	
	if (!r_user) {
		errno = ENOENT;
		return NULL;
	}
	
        ZERO_STRUCT(response);
        ZERO_STRUCT(request);
	
	STRCPY_RETNULL(request.data.username, r_user);
	
	if (*user == WB_AIX_ENCODED) {
		free(r_user);
	}
	
	ret = winbindd_request_response(WINBINDD_GETGROUPS, &request, &response);
	
	HANDLE_ERRORS(ret);
	
	num_gids = response.data.num_entries;
	gid_list = (gid_t *)response.extra_data.data;
	
	/* allocate a space large enough to contruct the string */
	tmpbuf = (char*) list_alloc(list, num_gids*12);
	if (!tmpbuf) {
		return NULL;
	}

	for (idx=i=0; i < num_gids-1; i++) {
		idx += sprintf(tmpbuf+idx, "%u,", gid_list[i]);	
	}
	idx += sprintf(tmpbuf+idx, "%u", gid_list[i]);	

	winbindd_free_response(&response);

	return tmpbuf;
}

static struct mem_list* getgrset_memlist;
static char *wb_aix_getgrset(char *user)
{
	list_destroy(&getgrset_memlist);
	return __wb_aix_getgrset(&getgrset_memlist, user);
}

/* take a uid and return a filled struct passwd */	
static struct passwd *__wb_aix_getpwuid(struct mem_list** list, uid_t uid)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	struct passwd *pwd;

	logit_debug(LOG_DEBUG, "getpwuid '%d'\n", uid);

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);
		
	request.data.uid = uid;
	
	ret = winbindd_request_response(WINBINDD_GETPWUID, &request, &response);

	HANDLE_ERRORS(ret);

	pwd = fill_pwent(list, &response.data.pw);

	winbindd_free_response(&response);

	logit_debug(LOG_DEBUG, "getpwuid gave ptr %p\n", pwd);

	return pwd;
}

static struct mem_list *getpwuid_memlist;
static struct passwd *wb_aix_getpwuid(uid_t uid)
{
	list_destroy(&getpwuid_memlist);
	return __wb_aix_getpwuid(&getpwuid_memlist, uid);
}

/* take a username and return a filled struct passwd */
static struct passwd *__wb_aix_getpwnam(struct mem_list **list, const char *name)
{
	struct winbindd_response response;
	struct winbindd_request request;
	NSS_STATUS ret;
	struct passwd *pwd;

	logit_debug(LOG_DEBUG, "getpwnam '%s'\n", name);

	if (*name == WB_AIX_ENCODED) {
		logit_debug(LOG_DEBUG, "getpwnam - decoding username\n");
		return __wb_aix_getpwuid(list, decode_id(name));
	}

	ZERO_STRUCT(response);
	ZERO_STRUCT(request);

	STRCPY_RETNULL(request.data.username, name);

	ret = winbindd_request_response(WINBINDD_GETPWNAM, &request, &response);

	HANDLE_ERRORS(ret);
	
	pwd = fill_pwent(list, &response.data.pw);

	winbindd_free_response(&response);

	logit_debug(LOG_DEBUG, "getpwnam gave ptr %p\n", pwd);

	return pwd;
}

static struct mem_list *getpwnam_memlist;
static struct passwd *wb_aix_getpwnam(const char *name)
{
	list_destroy(&getpwnam_memlist);
	return __wb_aix_getpwnam(&getpwnam_memlist, name);
}

/*
  list users
*/
static int __wb_aix_lsuser(struct mem_list** list, char *attributes[], attrval_t results[], int size)
{
	NSS_STATUS ret;
	struct winbindd_request request;
	struct winbindd_response response;
	int len;
	char *s;

	logit_debug(LOG_DEBUG, "lsuser called\n");

	if (size != 1 || strcmp(attributes[0], S_USERS) != 0) {
		logit(LOG_ERR, "invalid lsuser op\n");
		errno = EINVAL;
		return -1;
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);
	
	ret = winbindd_request_response(WINBINDD_LIST_USERS, &request, &response);
	if (ret != 0) {
		errno = EINVAL;
		return -1;
	}

	len = strlen(response.extra_data.data);

	s = list_alloc(list, len+2);
	if (!s) {
		winbindd_free_response(&response);
		errno = ENOMEM;
		return -1;
	}
	
	memcpy(s, response.extra_data.data, len+1);

	replace_commas(s);

	results[0].attr_un.au_char = s;
	results[0].attr_flag = 0;

	winbindd_free_response(&response);
	
	return 0;
}

static struct mem_list* lsuser_memlist;
static int wb_aix_lsuser(char *attributes[], attrval_t results[], int size)
{
	list_destroy(&lsuser_memlist);
	return __wb_aix_lsuser(&lsuser_memlist, attributes, results, size);
}

/*
  list groups
*/
static int __wb_aix_lsgroup(struct mem_list** list, char *attributes[], attrval_t results[], int size)
{
	NSS_STATUS ret;
	struct winbindd_request request;
	struct winbindd_response response;
	int len;
	char *s;

	logit_debug(LOG_DEBUG, "lsgroup called\n");

	if (size != 1 || strcmp(attributes[0], S_GROUPS) != 0) {
		logit(LOG_ERR, "invalid lsgroup op\n");
		errno = EINVAL;
		return -1;
	}

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);
	
	ret = winbindd_request_response(WINBINDD_LIST_GROUPS, &request, &response);
	if (ret != 0) {
		errno = EINVAL;
		return -1;
	}

	len = strlen(response.extra_data.data);

	s = list_alloc(list, len+2);
	if (!s) {
		winbindd_free_response(&response);
		errno = ENOMEM;
		return -1;
	}
	
	memcpy(s, response.extra_data.data, len+1);

	replace_commas(s);

	results[0].attr_un.au_char = s;
	results[0].attr_flag = 0;

	winbindd_free_response(&response);
	
	return 0;
}
static struct mem_list *lsgroup_memlist;
static int wb_aix_lsgroup(char *attributes[], attrval_t results[], int size)
{
	list_destroy(&lsgroup_memlist);
	return __wb_aix_lsgroup(&lsgroup_memlist, attributes, results, size);
}

static attrval_t pwd_to_group(struct mem_list** list, struct passwd *pwd)
{
	attrval_t r;
	struct group *grp = __wb_aix_getgrgid(list, pwd->pw_gid);
	
	if (!grp) {
		r.attr_flag = EINVAL;				
	} else {
		r.attr_flag = 0;
		r.attr_un.au_char = list_strdup(list, grp->gr_name);
		if (!r.attr_un.au_char)
			r.attr_flag = ENOMEM;
	}

	return r;
}

static attrval_t pwd_to_groupsids(struct mem_list** list, struct passwd *pwd)
{
	attrval_t r;
	char *s, *p;

	r.attr_flag = 0;

	if ( (s = __wb_aix_getgrset(list, pwd->pw_name)) == NULL ) {
		r.attr_flag = EINVAL;
		return r;
	}

	if ( (p = list_alloc(list, strlen(s)+2)) == NULL ) {
		r.attr_flag = ENOMEM;
		return r;
	}

	strcpy(p, s);
	replace_commas(p);

	r.attr_un.au_char = p;

	return r;
}

static attrval_t pwd_to_groupsnames(struct mem_list** mlist, struct passwd *pwd)
{
 	attrval_t r;
        int len = 0; 
        int buf_current_size = 1024;
	int buf_used = 0;
        char *list = NULL; 
	char *s = NULL;
	char *p = NULL;
	char *gid = NULL;
	struct group *group = NULL;

	if ( (s = __wb_aix_getgrset(mlist, pwd->pw_name)) == NULL ) {
		r.attr_flag = EINVAL;
		return r;
	}

	list = (char *)list_alloc(mlist, buf_current_size + 1);
	if (!list) {  
 		//SAFE_FREE(s);
		errno = ENOMEM;
		r.attr_flag = ENOMEM;
		return r;
	 }

	/* Walk the list of group id's and find the group name for each */
	for (gid = strtok_r(s, ",", &p); 
	     gid; 
	     gid = strtok_r(NULL, ",", &p)) {
		
		group = __wb_aix_getgrgid(mlist, strtoul(gid, NULL, 10));
		
		len = strlen(group->gr_name); 
		
		/* See if we have enought memory */ 
		if(buf_used + len + 1 > buf_current_size){
			buf_current_size *= 2;
			buf_current_size += len + 1;
			list = (char*)list_realloc(mlist, list, buf_current_size);
			if(!list) {
				SAFE_FREE(group);
				errno = ENOMEM;
				r.attr_flag = ENOMEM;
				return r;
			}
		}
		
		strcpy(list + buf_used, group->gr_name);
		buf_used += len + 1;
		
		SAFE_FREE(group);
	}
	
	/* Terminate list - we now have each element in the list separted by a single '/0'
	   with the end of the list having a double '/0' */
	list[buf_used] = '\0';
	
	/* If we have not entries we need to added a double '\0' */
	if(buf_used == 0){
		list[buf_used + 1] = '\0';
	}
	
	r.attr_flag = 0;
	r.attr_un.au_char = list;
	
	return r;
}

static attrval_t pwd_to_sid(struct mem_list** list, struct passwd *pwd)
{
	struct winbindd_request request;
	struct winbindd_response response;
	attrval_t r;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.uid = pwd->pw_uid;

	if (winbindd_request_response(WINBINDD_UID_TO_SID, &request, &response) !=
	    NSS_STATUS_SUCCESS) {
		r.attr_flag = ENOENT;
	} else {
		r.attr_flag = 0;
		r.attr_un.au_char = list_strdup(list, response.data.sid.sid);
        if (!r.attr_un.au_char)
            r.attr_flag = ENOMEM;
	}

	return r;
}

static void log_sec_list(const char *list)
{
	int i;
	if (!debug_enabled || log_silent) {
		return;
	}
	for(i=0; list[0]!='\0' || list[1]!='\0'; i++)
	{
		if(i!=0)
		{
			/* this skips over the null from the previous entry */
			list++;
		}
		logit_debug(LOG_DEBUG, "\t%d: '%s'\n", i, list);
		list+=strlen(list);
	}
}

static int __wb_aix_user_attrib(struct mem_list** list, const char *key, char *attributes[],
                                attrval_t results[], int size)
{
	struct passwd *pwd;
	int i;

	pwd = __wb_aix_getpwnam(list, key);
	if (!pwd) {
		errno = ENOENT;
		for(i=0; i<size; i++)
			results[i].attr_flag = ENOENT;
		return -1;
	}

	for (i=0;i<size;i++) {
		results[i].attr_flag = 0;

		if (strcmp(attributes[i], S_ID) == 0) {
			results[i].attr_un.au_int = pwd->pw_uid;
		} else if (strcmp(attributes[i], S_PGID) == 0) {
			results[i].attr_un.au_int = pwd->pw_gid;
		} else if (strcmp(attributes[i], S_PWD) == 0) {
			results[i].attr_un.au_char = list_strdup(list, pwd->pw_passwd);
		} else if (strcmp(attributes[i], S_HOME) == 0) {
			results[i].attr_un.au_char = list_strdup(list, pwd->pw_dir);
		} else if (strcmp(attributes[i], S_SHELL) == 0) {
			results[i].attr_un.au_char = list_strdup(list, pwd->pw_shell);
		} else if (strcmp(attributes[i], S_REGISTRY) == 0) {
			results[i].attr_un.au_char = list_strdup(list, "LWIDENTITY");
		} else if (strcmp(attributes[i], S_GECOS) == 0) {
			results[i].attr_un.au_char = list_strdup(list, pwd->pw_gecos);
		} else if (strcmp(attributes[i], S_PGRP) == 0) {
			results[i] = pwd_to_group(list, pwd);
		} else if (strcmp(attributes[i], S_GROUPS) == 0) {
			results[i] = pwd_to_groupsnames(list, pwd);
			logit_debug(LOG_DEBUG, "Returning group list for user %s (flag %d):\n", pwd->pw_name, results[i].attr_flag);
			log_sec_list(results[i].attr_un.au_char);
	       	} else if (strcmp(attributes[i], S_GROUPSIDS) == 0) {
			results[i] = pwd_to_groupsids(list, pwd);
			logit_debug(LOG_DEBUG, "Returning group list by id for user %s (flag %d):\n", pwd->pw_name, results[i].attr_flag);
			log_sec_list(results[i].attr_un.au_char);
		} else if (strcmp(attributes[i], S_LOCKED) == 0) {
			results[i].attr_un.au_int = 0;
		} else if (strcmp(attributes[i], "SID") == 0) {
			results[i] = pwd_to_sid(list, pwd);
		} else {
			logit(LOG_ERR, "Unknown user attribute '%s'\n", attributes[i]);
			results[i].attr_flag = EINVAL;
		}
	}

	return 0;
}

static struct mem_list* user_attrib_memlist;
static int wb_aix_user_attrib(const char *key, char *attributes[],
			      attrval_t results[], int size)
{
	list_destroy(&user_attrib_memlist);
	return __wb_aix_user_attrib(&user_attrib_memlist, key, attributes, results, size);
}

static int __wb_aix_group_attrib(struct mem_list** list, const char *key, char *attributes[],
                                 attrval_t results[], int size)
{
	struct group *grp;
	int i;
    
	grp = __wb_aix_getgrnam(list, key);
	if (!grp) {
		errno = ENOENT;
		return -1;
	}
	
	for (i=0;i<size;i++) {
		results[i].attr_flag = 0;

		if (strcmp(attributes[i], S_PWD) == 0) {
			results[i].attr_un.au_char = list_strdup(list, grp->gr_passwd);
		} else if (strcmp(attributes[i], S_ID) == 0) {
			results[i].attr_un.au_int = grp->gr_gid;
                } else if (strcmp(attributes[i], S_ADMIN) == 0) {
                        results[i].attr_un.au_int = 0;
                } else if (strcmp(attributes[i], S_ADMS) == 0) {
                        results[i].attr_un.au_char = "\0";
		} else {
			logit(LOG_ERR, "Unknown group attribute '%s'\n", attributes[i]);
			results[i].attr_flag = EINVAL;
		}
	}

	return 0;
}

static struct mem_list* group_attrib_memlist;
static int wb_aix_group_attrib(const char *key, char *attributes[],
			       attrval_t results[], int size)
{
	list_destroy(&group_attrib_memlist);
	return __wb_aix_group_attrib(&group_attrib_memlist, key, attributes, results, size);
}

/*
  called for user/group enumerations
*/
static int __wb_aix_getentry(struct mem_list** list, char *key, char *table, char *attributes[], 
			   attrval_t results[], int size)
{
	logit_debug(LOG_DEBUG, "Got getentry with key='%s' table='%s' size=%d attributes[0]='%s'\n", 
		    key, table, size, (size>0)? attributes[0] : "");

	if (strcmp(key, "ALL") == 0 && 
	    strcmp(table, "user") == 0) {
		return __wb_aix_lsuser(list, attributes, results, size);
	}
	
	if (strcmp(key, "ALL") == 0 && 
	    strcmp(table, "group") == 0) {
		return __wb_aix_lsgroup(list, attributes, results, size);
	}

	if (strcmp(table, "user") == 0) {
		int ret = __wb_aix_user_attrib(list, key, attributes, results, size);
		logit(LOG_DEBUG, "wb_aix_getentry returning %d errno %d\n", ret, errno);
		return ret;
	}

	if (strcmp(table, "group") == 0) {
		return __wb_aix_group_attrib(list, key, attributes, results, size);
	}

	logit(LOG_ERR, "Unknown getentry operation key='%s' table='%s'\n", key, table);

	errno = ENOSYS;
	return -1;
}

#define MEMORY_ROTATION 50

static struct mem_list* getentry_memlist[MEMORY_ROTATION];
static unsigned int getentry_memlist_index = 0;

static int wb_aix_getentry(char *key, char *table, char *attributes[], 
			   attrval_t results[], int size)
{
    int result;
	list_destroy(&getentry_memlist[getentry_memlist_index]);
	result = __wb_aix_getentry(&getentry_memlist[getentry_memlist_index], key, table, attributes, results, size);
    /* If 0 results are being returned, then we can reuse the same memlist
       index next time. Otherwise, choose a new index for next time. */
    if(size > 0)
    {
        getentry_memlist_index = (getentry_memlist_index+1) %
            (sizeof(getentry_memlist)/sizeof(getentry_memlist[0]));
    }
    return result;
}

/*
  called to start the backend
*/
static void *wb_aix_open(const char *name, const char *domain, int mode, char *options)
{
	if (strstr(options, "debug")) {
		debug_enabled = true;
	}
	logit_debug(LOG_DEBUG, "open name='%s' mode=%d domain='%s' options='%s'\n", name, domain, 
		    mode, options);

	return NULL;
}

static void wb_aix_close(void *token)
{
	logit_debug(LOG_DEBUG, "close\n");
	return;
}

#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_ATTRLIST
/* 
   return a list of additional attributes supported by the backend 
*/
static attrlist_t **__wb_aix_attrlist(struct mem_list** list)
{
  /* Make sure we have N+1 pointers so we can NULL terminate the list */

	attrlist_t **ret = NULL;
	int i;
	int n;
	size_t size;
	
	struct attr_types {
		const char *name; 
		int flags;
		int type;
	} attr_list[] = {
		/* user attributes */
		{S_ID, 		AL_USERATTR, 	SEC_INT},
		{S_PGRP, 	AL_USERATTR,	SEC_CHAR},
		{S_HOME, 	AL_USERATTR, 	SEC_CHAR},
		{S_SHELL, 	AL_USERATTR,	SEC_CHAR},
		{S_PGID, 	AL_USERATTR,	SEC_INT},
		{S_GECOS, 	AL_USERATTR,	SEC_CHAR},
		{S_GROUPS, 	AL_USERATTR, 	SEC_LIST},
		{S_GROUPSIDS, 	AL_USERATTR, 	SEC_LIST},
		{S_LOCKED, 	AL_USERATTR,	SEC_BOOL},
		{"SID", 	AL_USERATTR,	SEC_CHAR},

		/* group attributes */
		{S_ID, 		AL_GROUPATTR,	SEC_INT}
	};

	logit_debug(LOG_DEBUG, "method attrlist called\n");
	
	n = sizeof(attr_list) / sizeof(attr_list[0]);
	size = ((n + 1) * sizeof(attrlist_t *));
	
	if ( (ret = list_alloc(list, size)) == NULL ) {
		errno = ENOMEM;
		return NULL;
	}
	
	/* now loop over the user_attr_list[] array and add
	   all the members */

	for ( i=0; i<n; i++ ) {
			attrlist_t *a = list_alloc(list, sizeof(attrlist_t));

		if ( !a ) {
			/* this is bad.  Just bail */
			errno = ENOMEM;
			SAFE_FREE(ret);
			return NULL;
		}

		a->al_name  = list_strdup(list, attr_list[i].name);
		a->al_flags = attr_list[i].flags;
		a->al_type  = attr_list[i].type;

		ret[i] = a;
	}
	ret[n] = NULL;

	return ret;
}

static struct mem_list* attrlist_memlist;
static attrlist_t **wb_aix_attrlist_result = NULL;
static attrlist_t **wb_aix_attrlist(void)
{
	if(wb_aix_attrlist_result == NULL)
	{
		list_destroy(&attrlist_memlist);
		wb_aix_attrlist_result = __wb_aix_attrlist(&attrlist_memlist);
	}
	else
		logit_debug(LOG_DEBUG, "Reusing wb_aix_attrlist\n");
	return wb_aix_attrlist_result;
}
#endif

/*
  turn a long username into a short one. Needed to cope with the 8 char 
  username limit in AIX 5.2 and below
*/
static int __wb_aix_normalize(struct mem_list** list, char *longname, char *shortname)
{
	struct passwd *pwd;

	logit_debug(LOG_DEBUG, "normalize '%s'\n", longname);

	pwd = __wb_aix_getpwnam(list, longname);
	if (!pwd) {
		errno = ENOENT;
		logit_debug(LOG_DEBUG, "normalize - username not found\n");
		return 0;
	}

	/* automatically cope with AIX 5.3 with longer usernames
	   when it comes out */
	if (S_NAMELEN > strlen(longname)) {
		strcpy(shortname, longname);
	}
	else
	{
	    WblMangleAIX((unsigned int) pwd->pw_uid, shortname);
	}

	logit_debug(LOG_DEBUG, "normalize - returning '%s'\n", shortname);
	return strlen(shortname);
}

static struct mem_list* normalize_memlist;
static int wb_aix_normalize(char *longname, char *shortname)
{
	list_destroy(&normalize_memlist);
	return __wb_aix_normalize(&normalize_memlist, longname, shortname);
}

/*
  authenticate a user
 */
static int wb_aix_authenticate(char *user, char *pass, int *reenter, char **message)
{
	logit_debug(LOG_DEBUG, "authenticate '%s' response='%s'\n", user, pass);

	*message = NULL;
	
	int retval = AUTH_FAILURE;
	dictionary *d = NULL;
	
	char *username = NULL;
	char *password = pass;

	const char *member = NULL;
	const char *cctype = NULL;

	/* Parse configuration file */
	int ctrl = _pam_parse(&d);
	if (ctrl == -1) {
		logit_debug(LOG_DEBUG, "Can not parse configuration file: %s\n", PAM_WINBIND_CONFIG_FILE);	
		retval = AUTH_FAILURE;
		goto out;
	}
	
	logit_debug(LOG_DEBUG, "Entered wb_aix_authenticate with user = %s\n", user);	
	
	/* Decode the user name since AIX does not support long user
	   names by default.  The name is encoded as _#uid.  */
	if ( user[0] == WB_AIX_ENCODED ) 
	{
 		char *endptr = NULL;
		uid_t id = strtoul( &user[1], &endptr, 10 );
		struct passwd *pw = NULL;		
		
		if ( isdigit(username[1]) && *endptr == '\0' && ((pw = getpwuid( id )) != NULL)) 
		{
			username = strdup( pw->pw_name );
		}
	}
	
	if ( !username ) {
		/* Just making a copy of the username we got from LAM */
		if ( (username = strdup( user )) == NULL ) {
			logit(LOG_ERR, "memory allocation failure when copying username\n");
			retval = AUTH_FAILURE;
			goto out;
		}
	}	
	
	/* Let's not give too much away in the log file */
	
#ifdef DEBUG_PASSWORD
	logit_debug(LOG_DEBUG, "Verify user '%s' with password '%s'\n",  username, password);
#else
	logit_debug(LOG_DEBUG, "Verify user '%s'\n",  username);	
#endif

	member = get_member_from_config(ctrl, d);
	cctype = get_krb5_cc_type_from_config(ctrl, d);

	retval = winbind_auth_request(ctrl, 
				      d,
				      username, 
				      password, 
				      member,
				      cctype);
	if(retval == AUTH_SUCCESS)
	{
        	logit_debug(LOG_DEBUG, "Winbind auth successful\n");
	}
	
	if(retval == AUTH_PASSWORD_MUST_CHANGE)
	{
		retval = AUTH_SUCCESS;
	}
	
out:
	if(*message == NULL)
	{
		*reenter = 0;
	}
	
	if ( username ) {		
		free( username );
	}	
	
	if (d) {
		iniparser_freedict(d);
	}
	
	logit_debug(LOG_DEBUG, "Leaving wb_aix_authenticate with user = %s\n", user);	


	return retval;
}

bool IsPamToLamBoolSet(const char *envVariable, const char *username)
{
	/*
	  If the environmental variable is set to the current pid colon the
	  requested username, then we should return true
	*/
	const char *value = getenv(envVariable);
	if(value == NULL)
		return 0;

	logit_debug(LOG_DEBUG, "%s is set to '%s'. My pid is %d\n", envVariable, value, getpid());

	char *end_ptr = NULL;
	pid_t pid_value = strtoul(value, &end_ptr, 10);
	if(*end_ptr != ':')
		return 0;

	char *username_value = end_ptr + 1;
	if(getpid() != pid_value)
	{
		return 0;
	}
	logit_debug(LOG_DEBUG, "PIDs match, validating username");

	if(strcasecmp(username_value, username) == 0)
	{
		logit_debug(LOG_DEBUG, "Username matches");
		return 1;
	}
	return 0;
}

/*
  change a user password
*/
static int wb_aix_chpass(char *user, char *oldpass, char *newpass, char **message)
{
	struct winbindd_request request;
	struct winbindd_response response;
        NSS_STATUS result;
	char *r_user = user;

	logit_debug(LOG_DEBUG, "Entering wb_aix_chpass");

	/*
	  We only want to return a message if we own the user and there is a
	  problem.
	*/
	*message = NULL;

	if (*user == WB_AIX_ENCODED) {
		r_user = decode_user(r_user);
	}

	if(IsPamToLamBoolSet("LWIDENTITY_PAM_CHAUTHTOK", r_user))
	{
		logit_debug(LOG_DEBUG, "Pam already tried to change the password for %s. Failing in LAM", r_user);
		errno = ENOENT;
		return -1;
	}

	logit_debug(LOG_DEBUG, "chpass %s '%s' old='%s' new='%s'\n", user, r_user, oldpass, newpass);
	if (!r_user) {
		errno = ENOENT;
		return -1;
	}

	/* Send off request */
	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	STRCPY_RET(request.data.chauthtok.user, r_user);
	STRCPY_RET(request.data.chauthtok.oldpass, oldpass);
	STRCPY_RET(request.data.chauthtok.newpass, newpass);

	if (*user == WB_AIX_ENCODED) {
		free(r_user);
	}

	result = winbindd_request_response(WINBINDD_PAM_CHAUTHTOK, &request, &response);

	const char *ntstatus = response.data.auth.nt_status_string;
	logit(LOG_DEBUG, "NTSTATUS from call to winbindd_request_response:WINBINDD_PAM_CHAUTHTOK is = %s", ntstatus);

	winbindd_free_response(&response);

	if (result == NSS_STATUS_SUCCESS) {
		unsetenv("LWIDENTITY_PASSWD_EXPIRED");
		errno = 0;
		return 0;
	}

	*message = "Error, unable to change password";
	errno = EINVAL;
	return -1;
}

/*
  don't do any password strength testing for now
*/
static int __wb_aix_passwdrestrictions(struct mem_list **list, char *user, char *newpass, char *oldpass, 
				     char **message)
{
	struct passwd *pwd;

	logit_debug(LOG_DEBUG, "passwdresrictions called for '%s'\n", user);
	*message = "";
	pwd = __wb_aix_getpwnam(list, user);
	if(pwd == NULL)
	{
		errno = ENOENT;
		return -1;
	}

	return 0;
}

static struct mem_list* passwdrestrictions_memlist;
static int wb_aix_passwdrestrictions(char *user, char *newpass, char *oldpass, 
				     char **message)
{
	list_destroy(&passwdrestrictions_memlist);
	return __wb_aix_passwdrestrictions(&passwdrestrictions_memlist, user, newpass, oldpass, message);
}

static int wb_aix_passwdexpired(char *user, char **message)
{
	char *r_user = NULL;
	int mustChange;

	logit_debug(LOG_DEBUG, "Entering wb_aix_passwdexpired");
	
	logit_debug(LOG_DEBUG, "passwdexpired '%s'\n", user);
	/* we should check the account bits here */

	*message = "";

	if (*user == WB_AIX_ENCODED)
		r_user = decode_user(user);
	else
		r_user = strdup(user);

	mustChange = IsPamToLamBoolSet("LWIDENTITY_PASSWD_EXPIRED", r_user);

	if(r_user != NULL)
		free(r_user);

	return mustChange;
}

/*
  we can't return a crypt() password
*/
static char *wb_aix_getpasswd(char *user)
{
	logit_debug(LOG_DEBUG, "getpasswd '%s'\n", user);
	errno = ENOSYS;
	return NULL;
}

/*
  this is called to update things like the last login time. We don't 
  currently pass this onto the DC
*/
static int wb_aix_putentry(char *key, char *table, char *attributes[], 
			   attrval_t values[], int size)
{
	logit_debug(LOG_DEBUG, "putentry key='%s' table='%s' attrib='%s'\n", 
		    key, table, size>=1?attributes[0]:"<null>");
	errno = ENOSYS;
	return -1;
}

static int wb_aix_commit(char *key, char *table)
{
	logit_debug(LOG_DEBUG, "commit key='%s' table='%s'\n", key, table);
	errno = ENOSYS;
	return -1;
}

static int wb_aix_getgrusers(char *group, void *result, int type, int *size)
{
	logit_debug(LOG_DEBUG, "getgrusers group='%s'\n", group);
	errno = ENOSYS;
	return -1;
}


#define DECL_METHOD(x) \
int method_ ## x(void) \
{ \
	logit_debug(LOG_DEBUG, "UNIMPLEMENTED METHOD '%s'\n", #x); \
	errno = EINVAL; \
	return -1; \
}

#if LOG_UNIMPLEMENTED_CALLS
DECL_METHOD(delgroup);
DECL_METHOD(deluser);
DECL_METHOD(newgroup);
DECL_METHOD(newuser);
DECL_METHOD(putgrent);
DECL_METHOD(putgrusers);
DECL_METHOD(putpwent);
DECL_METHOD(lock);
DECL_METHOD(unlock);
DECL_METHOD(getcred);
DECL_METHOD(setcred);
DECL_METHOD(deletecred);
#endif

int wb_aix_init(struct secmethod_table *methods)
{
	ZERO_STRUCTP(methods);

#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_VERSION
	methods->method_version = SECMETHOD_VERSION_520;
#endif

	methods->method_getgrgid           = wb_aix_getgrgid;
	methods->method_getgrnam           = wb_aix_getgrnam;
	methods->method_getgrset           = wb_aix_getgrset;
	methods->method_getpwnam           = wb_aix_getpwnam;
	methods->method_getpwuid           = wb_aix_getpwuid;
	methods->method_getentry           = wb_aix_getentry;
	methods->method_open               = wb_aix_open;
	methods->method_close              = wb_aix_close;
	methods->method_normalize          = wb_aix_normalize;
	methods->method_passwdexpired      = wb_aix_passwdexpired;
	methods->method_putentry           = wb_aix_putentry;
	methods->method_getpasswd          = wb_aix_getpasswd;
	methods->method_authenticate       = wb_aix_authenticate;	
	methods->method_commit             = wb_aix_commit;
	methods->method_chpass             = wb_aix_chpass;
	methods->method_passwdrestrictions = wb_aix_passwdrestrictions;
	methods->method_getgracct          = wb_aix_getgracct;
	methods->method_getgrusers         = wb_aix_getgrusers;
#ifdef HAVE_STRUCT_SECMETHOD_TABLE_METHOD_ATTRLIST
	methods->method_attrlist           = wb_aix_attrlist;
#endif

#if LOG_UNIMPLEMENTED_CALLS
	methods->method_delgroup      = method_delgroup;
	methods->method_deluser       = method_deluser;
	methods->method_newgroup      = method_newgroup;
	methods->method_newuser       = method_newuser;
	methods->method_putgrent      = method_putgrent;
	methods->method_putgrusers    = method_putgrusers;
	methods->method_putpwent      = method_putpwent;
	methods->method_lock          = method_lock;
	methods->method_unlock        = method_unlock;
	methods->method_getcred       = method_getcred;
	methods->method_setcred       = method_setcred;
	methods->method_deletecred    = method_deletecred;
#endif

	// Initialize memory lists
	getgrgid_memlist = NULL;
	getgrnam_memlist = NULL;
	getgracct_memlist = NULL;
	getgrset_memlist = NULL;
	getpwuid_memlist = NULL;
	getpwnam_memlist = NULL;
	lsuser_memlist = NULL;
	lsgroup_memlist = NULL;
	user_attrib_memlist = NULL;
	group_attrib_memlist = NULL;
	memset(getentry_memlist, 0, sizeof(getentry_memlist));
	getentry_memlist_index = 0;
	normalize_memlist = NULL;
	passwdrestrictions_memlist = NULL;
	attrlist_memlist = NULL;
	wb_aix_attrlist_result = NULL;

	return AUTH_SUCCESS;
}
