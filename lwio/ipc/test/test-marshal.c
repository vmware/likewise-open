/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"

static LWMsgProtocol* pProtocol = NULL;
static LWMsgContext* pContext = NULL;

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

static inline PWSTR
wstr(const char* str)
{
    PWSTR wstr = NULL;

    TRY ( SMBMbsToWc16s(str, &wstr) );

    return wstr;
}

MU_FIXTURE_SETUP(marshal)
{
    NTSTATUS ntStatus = 0;
    PCSTR pszError = NULL;

    TRY( lwmsg_protocol_new(NULL, &pProtocol) );

    ntStatus = NtIpcAddProtocolSpecEx(pProtocol, &pszError);
    if (ntStatus)
    {
        MU_FAILURE("Failed to compile protocol: %s", pszError);
    }

    TRY( lwmsg_context_new(&pContext) );
}

MU_FIXTURE_TEARDOWN(marshal)
{
    lwmsg_protocol_delete(pProtocol);
    lwmsg_context_delete(pContext);
}

MU_TEST(marshal, SMB_CREATE_FILE)
{
    IO_ACCESS_TOKEN token;
    NT_IPC_MESSAGE_CREATE_FILE request = { 0 };
    NT_IPC_MESSAGE_CREATE_FILE* pCopy;
    void* buffer;
    size_t length;
    LWMsgTypeSpec* type = NULL;

    token.type = IO_ACCESS_TOKEN_TYPE_PLAIN;
    token.payload.plain.pwszUsername = wstr("FOODOMAIN\\foouser");
    token.payload.plain.pwszPassword = wstr("foopassword");

    request.pSecurityToken = &token;
    request.FileName.FileName = wstr("\\\\foohost\\$IPC\\endpoint");
    request.DesiredAccess = GENERIC_READ | GENERIC_WRITE;
    request.CreateDisposition = OPEN_EXISTING;

    TRY ( lwmsg_protocol_get_message_type(pProtocol, NT_IPC_MESSAGE_TYPE_CREATE_FILE, &type) );
    TRY ( lwmsg_marshal_alloc(pContext, type, &request, &buffer, &length) );
    TRY ( lwmsg_unmarshal_simple(pContext, type, buffer, length, (void*) &pCopy) );

    MU_ASSERT_EQUAL_DWORD(request.pSecurityToken->type, pCopy->pSecurityToken->type);
    MU_ASSERT_EQUAL_PWSTR(request.pSecurityToken->payload.plain.pwszUsername, 
                          pCopy->pSecurityToken->payload.plain.pwszUsername);
    MU_ASSERT_EQUAL_PWSTR(request.pSecurityToken->payload.plain.pwszPassword,
                          pCopy->pSecurityToken->payload.plain.pwszPassword);
    MU_ASSERT_EQUAL_PWSTR(request.FileName.FileName, pCopy->FileName.FileName);
    MU_ASSERT_EQUAL_DWORD(request.DesiredAccess, pCopy->DesiredAccess);
    MU_ASSERT_EQUAL_DWORD(request.FileAttributes, pCopy->FileAttributes);
    MU_ASSERT_EQUAL_DWORD(request.ShareAccess, pCopy->ShareAccess);
    MU_ASSERT_EQUAL_DWORD(request.CreateDisposition, pCopy->CreateDisposition);
    MU_ASSERT_EQUAL_DWORD(request.CreateOptions, pCopy->CreateOptions);
}
