#ifndef __CREATEFILE_H__
#define __CREATEFILE_H__

DWORD
RdrCreateFileEx(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszFileName,
    DWORD   dwDesiredAccess,
    DWORD   dwSharedMode,
    DWORD   dwCreationDisposition,
    DWORD   dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE phFile
    );

#endif /* __CREATEFILE_H__ */
