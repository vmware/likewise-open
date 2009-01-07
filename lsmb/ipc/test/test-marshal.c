#include "includes.h"

static LWMsgProtocol* pProtocol = NULL;
static LWMsgBuffer buffer;

#define TRY(_e_) ((_e_) && (MU_FAILURE("%s", #_e_), 0))

#define MU_ASSERT_EQUAL_PWSTR(_a_, _b_)         \
    do                                          \
    {                                           \
        PCWSTR a = (_a_);                       \
        PCWSTR b = (_b_);                       \
                                                \
        if ((a == NULL && b != NULL) ||         \
            (a != NULL && b == NULL) ||         \
            (SMBWc16sCmp(a, b)))                \
        {                                       \
            Mu_Interface_Result(                \
            __FILE__,                           \
            __LINE__,                           \
            MU_STATUS_ASSERTION,                \
            "Assertion failed: %s == %s",       \
            #_a_,                               \
            #_b_);                              \
        }                                       \
    } while (0)

#define MU_ASSERT_EQUAL_DWORD(_a_, _b_) MU_ASSERT_EQUAL(MU_TYPE_INTEGER, _a_, _b_)

static inline LPWSTR
wstr(const char* str)
{
    LPWSTR wstr = NULL;

    TRY ( SMBMbsToWc16s(str, &wstr) );

    return wstr;
}

static void
BufferInit(void)
{
    buffer.length = 8192;
    buffer.memory = malloc(buffer.length);
    buffer.cursor = buffer.memory;
    buffer.full = NULL;
    buffer.data = NULL;
}

static void
BufferRewind(void)
{
    buffer.cursor = buffer.memory;
}

MU_FIXTURE_SETUP(marshal)
{
    LWMsgProtocolSpec* pSpec = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    TRY( SMBIPCGetProtocolSpec(&pSpec) );
    TRY( lwmsg_protocol_new(NULL, &pProtocol) );
    status = lwmsg_protocol_add_protocol_spec(pProtocol, pSpec);

    if (status)
    {
        MU_FAILURE("Failed to compile protocol: %s", lwmsg_protocol_get_error_message(pProtocol, status));
    }

    BufferInit();
}

MU_FIXTURE_TEARDOWN(marshal)
{
    lwmsg_protocol_delete(pProtocol);
}

MU_TEST(marshal, SMB_CREATE_FILE)
{
    SMB_SECURITY_TOKEN_REP token;
    SMB_CREATE_FILE_REQUEST request, *pCopy;

    token.type = SMB_SECURITY_TOKEN_TYPE_PLAIN;
    token.payload.plain.pwszUsername = wstr("FOODOMAIN\\foouser");
    token.payload.plain.pwszPassword = wstr("foopassword");

    request.pSecurityToken = &token;
    request.pwszFileName = wstr("\\\\foohost\\$IPC\\endpoint");
    request.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    request.dwSharedMode = 0;
    request.dwCreationDisposition = OPEN_EXISTING;
    request.dwFlagsAndAttributes = 0;
    request.pSecurityAttributes = NULL;

    TRY ( lwmsg_protocol_marshal(pProtocol, SMB_CREATE_FILE, &request, &buffer) );

    BufferRewind();

    TRY ( lwmsg_protocol_unmarshal(pProtocol, SMB_CREATE_FILE, &buffer, (void*) &pCopy) );

    MU_ASSERT_EQUAL_DWORD(request.pSecurityToken->type, pCopy->pSecurityToken->type);
    MU_ASSERT_EQUAL_PWSTR(request.pSecurityToken->payload.plain.pwszUsername, 
                          pCopy->pSecurityToken->payload.plain.pwszUsername);
    MU_ASSERT_EQUAL_PWSTR(request.pSecurityToken->payload.plain.pwszPassword,
                          pCopy->pSecurityToken->payload.plain.pwszPassword);
    MU_ASSERT_EQUAL_PWSTR(request.pwszFileName, pCopy->pwszFileName);
    MU_ASSERT_EQUAL_DWORD(request.dwDesiredAccess, pCopy->dwDesiredAccess);
    MU_ASSERT_EQUAL_DWORD(request.dwSharedMode, pCopy->dwSharedMode);
    MU_ASSERT_EQUAL_DWORD(request.dwCreationDisposition, pCopy->dwCreationDisposition);
    MU_ASSERT_EQUAL_DWORD(request.dwFlagsAndAttributes, pCopy->dwFlagsAndAttributes);
    MU_ASSERT_EQUAL(MU_TYPE_POINTER, request.pSecurityAttributes, pCopy->pSecurityAttributes);
}
