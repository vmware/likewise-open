#ifndef __DEFS_H__
#define __DEFS_H__

#define DEFAULT_LSALPC_SOCKET_PATH CACHEDIR "/rpc/lsass"

#define SRV_DRIVER_NAME_W { '\\', 's', 'r', 'v', 0 }

#define SERVER_COMMENT_STRING "Likewise CIFS"

#define SECURITY_UNMAPPED_UNIX_AUTHORITY    { 0, 0, 0, 0, 0, 22 }
#define SECURITY_UNMAPPED_UNIX_UID_RID      1
#define SECURITY_UNMAPPED_UNIX_RID_COUNT    2

/*
 * Internal access flags for doing security checks
 */
#define SRVSVC_ACCESS_GET_INFO_AUTHENTICATED_USERS (0x00000001)
#define SRVSVC_ACCESS_GET_INFO_ADMINS              (0x00000002)
#define SRVSVC_ACCESS_DELETE                       (0x00000020)

#endif /* __DEFS_H__ */

