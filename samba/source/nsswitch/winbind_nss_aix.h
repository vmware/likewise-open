/* LAM implementayion header file
	Robert Amenn 2007
*/

#ifndef WINBIND_NSS_AIX_H
#define WINBIND_NSS_AIX_H

#include "winbind_nss.h"
#include <iniparser.h>

#ifndef PAM_WINBIND_CONFIG_FILE
#define PAM_WINBIND_CONFIG_FILE "/etc/security/pam_lwidentity.conf"
#endif

#define MODULE_NAME "lam_lwidentity"

#define AUTH_PASSWORD_MUST_CHANGE 1001

#define WINBIND_DEBUG_ARG (1<<0)
/* #define WINBIND_USE_AUTHTOK_ARG (1<<1)  - Not need by LAM */
/* #define WINBIND_UNKNOWN_OK_ARG (1<<2) - Not need by LAM */
#define WINBIND_TRY_FIRST_PASS_ARG (1<<3)
/* #define WINBIND_USE_FIRST_PASS_ARG (1<<4) - Not need by LAM */
/* #define WINBIND__OLD_PASSWORD (1<<5) - Not need by LAM */
#define WINBIND_REQUIRED_MEMBERSHIP (1<<6)
#define WINBIND_KRB5_AUTH (1<<7)
#define WINBIND_KRB5_CCACHE_TYPE (1<<8)
#define WINBIND_CACHED_LOGIN (1<<9)
/* #define WINBIND_CONFIG_FILE (1<<10) - Not need by LAM */ 
#define WINBIND_SILENT (1<<11)
#define WINBIND_DEBUG_STATE (1<<12)
#define WINBIND_CREATE_HOMEDIR (1<<13)
#define WINBIND_SKEL_DIR (1<<14)
#define WINBIND_HOMEDIR_UMASK (1<<15)
#define WINBIND_CREATE_K5LOGIN (1<<16)

#define IS_SID_STRING(name) (strncmp("S-", name, 2) == 0)

#endif // #ifdef WINBIND_NSS_AIX_H
