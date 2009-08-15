#ifndef __DEFS_H__
#define __DEFS_H__

#define DEFAULT_CONFIG_FILE_PATH CONFIGDIR "/srvsvcd.conf"

#define DEFAULT_LSALPC_SOCKET_PATH CACHEDIR "/rpc/lsass"

#define ERROR_NOT_SUPPORTED 50

#define SRV_IOCTL_ADD_SHARE 0
#define SRV_IOCTL_SET_SHARE 1

#define BAIL_ON_ERROR(dwError) \
    if (dwError) goto error;

#define BAIL_ON_NT_STATUS(ntStatus) \
    if (ntStatus) goto error;

/* Work around lack of proper Win32 error codes for now */

#define WIN32_ERROR_SUCCESS                     0x00000000
#define WIN32_ERROR_INVALID_FUNCTION            0x00000001
#define WIN32_ERROR_FILE_NOT_FOUND              0x00000002
#define WIN32_ERROR_UNKNOWN_LEVEL               0x0000007c


#endif /* __DEFS_H__ */

