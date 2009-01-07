#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __SMB_SERVER_CONNECTION
{
    LWMsgAssoc* pAssoc;
} SMB_SERVER_CONNECTION, *PSMB_SERVER_CONNECTION;

typedef enum __SMB_API_HANDLE_TYPE
{
    SMB_API_HANDLE_FILE,
    SMB_API_HANDLE_ACCESS
} SMB_API_HANDLE_TYPE;

typedef struct __SMB_API_HANDLE
{
    SMB_API_HANDLE_TYPE type;
    union
    {
        HANDLE hIPCHandle;
        SMB_SECURITY_TOKEN_REP securityToken;
    } variant;
} SMB_API_HANDLE, *PSMB_API_HANDLE;

typedef struct __SMB_CLIENT_CONTEXT
{
    HANDLE hAccessToken;
    DWORD dwLastError;
    unsigned bSecurityTokenIsPrivate:1;
} SMB_CLIENT_CONTEXT, *PSMB_CLIENT_CONTEXT;

#define BAIL_IF_NOT_FILE_HANDLE(h)                                      \
    do                                                                  \
    {                                                                   \
        if (((PSMB_API_HANDLE)(h))->type != SMB_API_HANDLE_FILE)        \
        {                                                               \
            dwError = SMB_ERROR_INVALID_HANDLE;                         \
            goto error;                                                 \
        }                                                               \
    } while (0)

#endif /* __STRUCTS_H__ */
