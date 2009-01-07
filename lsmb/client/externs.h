#ifndef __EXTERNS_H__
#define __EXTERNS_H__

extern LWMsgProtocol* gpSMBProtocol;

DWORD
SMBGetClientContext(
    PSMB_CLIENT_CONTEXT* ppContext
    );

DWORD
SMBAcquireState(
    PSMB_SERVER_CONNECTION pConnection,
    PSMB_CLIENT_CONTEXT* ppContext
    );

VOID
SMBReleaseState(
    PSMB_SERVER_CONNECTION pConnection,
    PSMB_CLIENT_CONTEXT pContext
    );

DWORD
SMBAPIHandleGetSecurityToken(
    HANDLE hHandle,
    PSMB_SECURITY_TOKEN_REP* ppSecurityToken
    );

DWORD
SMBAPIHandleFreeSecurityToken(
    HANDLE hHandle
    );

#endif /* __EXTERNS_H__ */
