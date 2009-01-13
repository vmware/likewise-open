#ifndef __DEFS_H__
#define __DEFS_H__

#define ERROR_NOT_SUPPORTED 50

#define SRV_IOCTL_ADD_SHARE 0
#define SRV_IOCTL_SET_SHARE 1

#define BAIL_ON_ERROR(dwError) \
    if (dwError) goto error;

#define BAIL_ON_NT_STATUS(ntStatus) \
    if (ntStatus) goto error;

#endif /* __DEFS_H__ */

