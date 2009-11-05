/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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

static
DWORD
NetAllocateUserInfo0(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    );


static
DWORD
NetAllocateUserInfo1(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    );


static
DWORD
NetAllocateUserInfo2(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    );


static
DWORD
NetAllocateUserInfo3(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    );


static
DWORD
NetAllocateUserInfo20(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    );


DWORD
NetAllocateUserInfo(
    PVOID   pInfoBuffer,
    PDWORD  pdwSpaceLeft,
    DWORD   dwLevel,
    PVOID   pSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = pInfoBuffer;

    switch (dwLevel)
    {
    case 0:
        err = NetAllocateUserInfo0(&pCursor,
                                   pdwSpaceLeft,
                                   pSource,
                                   pdwSize);
        break;

    case 1:
        err = NetAllocateUserInfo1(&pCursor,
                                   pdwSpaceLeft,
                                   pSource,
                                   pdwSize);
        break;

    case 2:
        err = NetAllocateUserInfo2(&pCursor,
                                   pdwSpaceLeft,
                                   pSource,
                                   pdwSize);
        break;

    case 3:
        err = NetAllocateUserInfo3(&pCursor,
                                   pdwSpaceLeft,
                                   pSource,
                                   pdwSize);
        break;

    case 20:
        err = NetAllocateUserInfo20(&pCursor,
                                    pdwSpaceLeft,
                                    pSource,
                                    pdwSize);
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        break;
    }
    BAIL_ON_WINERR_ERROR(err);

cleanup:
    return err;

error:
    goto cleanup;
}


static
DWORD
NetAllocateUserInfo0(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    PWSTR pwszName = (PWSTR)pSource;

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pdwSize)
    {
        dwSize = *pdwSize;
    }

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    /* usri0_name */
    err = NetAllocBufferWC16String(&pCursor,
                                   &dwSpaceLeft,
                                   pwszName,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


static
DWORD
NetAllocateUserInfo1(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    UserInfo21 *pSamrInfo21 = (UserInfo21*)pSource;
    NtTime CurrentTime = 0;
    NtTime PasswordAge = 0;

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pdwSize)
    {
        dwSize = *pdwSize;
    }

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    /* usri1_name */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->account_name,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri1_password: SKIP */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   NULL,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri1_password_age */
    err = LwGetNtTime((PULONG64)&CurrentTime);
    BAIL_ON_WINERR_ERROR(err);

    PasswordAge = CurrentTime - pSamrInfo21->last_password_change;
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              0,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri1_priv: SKIP (it is set outside this function */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              0,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri1_home_dir */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->home_directory,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri1_comment */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->comment,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri1_flags */
    err = NetAllocBufferUserFlagsFromAcbFlags(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   pSamrInfo21->account_flags,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri1_script_path */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->logon_script,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

    if (ppCursor)
    {
        *ppCursor = pCursor;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


static
DWORD
NetAllocateUserInfo2(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    UserInfo21 *pSamrInfo21 = (UserInfo21*)pSource;

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pdwSize)
    {
        dwSize = *pdwSize;
    }

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    err = NetAllocateUserInfo1(&pCursor,
                               &dwSpaceLeft,
                               pSource,
                               &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_auth_flags: SKIP */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              0,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_full_name */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->full_name,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_usr_comment: SKIP */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   NULL,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_parms */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->parameters,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_workstations */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->workstations,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_last_logon */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              0,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_last_logoff: SKIP */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              0,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_account_expires */
    err = NetAllocBufferWinTimeFromNtTime(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   pSamrInfo21->account_expiry,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_max_storage: SKIP */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              0,
                              &dwSize);

    /* usri2_units_per_week: SKIP */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              0,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_logon_hours: SKIP */
    err = NetAllocBufferLogonHours(&pCursor,
                                   &dwSpaceLeft,
                                   NULL,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_bad_pw_count */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              pSamrInfo21->bad_password_count,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_num_logons */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              pSamrInfo21->logon_count,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_logon_server */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   NULL,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_country_code */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              pSamrInfo21->country_code,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri2_code_page */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              pSamrInfo21->code_page,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

    if (ppCursor)
    {
        *ppCursor = pCursor;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


static
DWORD
NetAllocateUserInfo3(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    UserInfo21 *pSamrInfo21 = (UserInfo21*)pSource;

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pdwSize)
    {
        dwSize = *pdwSize;
    }

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    err = NetAllocateUserInfo2(&pCursor,
                               &dwSpaceLeft,
                               pSource,
                               &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri3_user_id */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              pSamrInfo21->rid,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri3_profile */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->profile_path,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri3_home_dir_drive */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->home_drive,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri3_password_expired */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              pSamrInfo21->password_expired,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

    if (ppCursor)
    {
        *ppCursor = pCursor;
    }

cleanup:
    return err;

error:
    goto cleanup;
}


static
DWORD
NetAllocateUserInfo20(
    PVOID  *ppCursor,
    PDWORD  pdwSpaceLeft,
    PVOID   pSource,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    PVOID pCursor = NULL;
    DWORD dwSpaceLeft = 0;
    DWORD dwSize = 0;
    UserInfo21 *pSamrInfo21 = (UserInfo21*)pSource;

    if (pdwSpaceLeft)
    {
        dwSpaceLeft = *pdwSpaceLeft;
    }

    if (pdwSize)
    {
        dwSize = *pdwSize;
    }

    if (ppCursor)
    {
        pCursor = *ppCursor;
    }

    /* usri20_name */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->account_name,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri20_full_name */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->full_name,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri20_comment */
    err = NetAllocBufferWC16StringFromUnicodeString(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   &pSamrInfo21->comment,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri20_flags */
    err = NetAllocBufferUserFlagsFromAcbFlags(
                                   &pCursor,
                                   &dwSpaceLeft,
                                   pSamrInfo21->account_flags,
                                   &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    /* usri20_user_id */
    err = NetAllocBufferDword(&pCursor,
                              &dwSpaceLeft,
                              pSamrInfo21->rid,
                              &dwSize);
    BAIL_ON_WINERR_ERROR(err);

    if (pdwSpaceLeft)
    {
        *pdwSpaceLeft = dwSpaceLeft;
    }

    if (pdwSize)
    {
        *pdwSize = dwSize;
    }

    if (ppCursor)
    {
        *ppCursor = pCursor;
    }

cleanup:
    return err;

error:
    goto cleanup;

}


/*
 * push functions/macros transfer: net userinfo -> samr userinfo
 * pull functions/macros transfer: net userinfo <- samr userinfo
 */


NTSTATUS
PullUserInfo0(
    void **buffer,
    wchar16_t **names,
    uint32 num
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_0 *info = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(buffer);
    BAIL_ON_INVALID_PTR(names);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_1) * num,
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < num; i++) {
        if (names[i]) {
            info[i].usri0_name = wc16sdup(names[i]);

        } else {
            info[i].usri0_name = wc16sdup(null_string);
        }
        BAIL_ON_NO_MEMORY(info[i].usri0_name);

        status = NetAddDepMemory(info[i].usri0_name,
                                 info);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    *buffer = info;

cleanup:
    if (status == STATUS_SUCCESS &&
        err != ERROR_SUCCESS) {
        status = Win32ErrorToNtStatus(err);
    }

    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS
PullUserInfo1(
    void **buffer,
    UserInfo21 *ui,
    uint32 num
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_1 *info = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(buffer);
    BAIL_ON_INVALID_PTR(ui);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_1) * num,
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].usri1_name, ui[i].account_name, info);
        info[i].usri1_password = NULL;
        /* info[i].usri1_password_age */
        /* info[i].usri1_priv */
        PULL_UNICODE_STRING(info[i].usri1_home_dir, ui[i].home_directory, info);
        PULL_UNICODE_STRING(info[i].usri1_comment, ui[i].comment, info);
        PULL_ACCOUNT_FLAGS(info[i].usri1_flags, ui[i].account_flags);
        PULL_UNICODE_STRING(info[i].usri1_script_path, ui[i].logon_script, info);
    }

    *buffer = info;

cleanup:
    if (status == STATUS_SUCCESS &&
        err != ERROR_SUCCESS) {
        status = Win32ErrorToNtStatus(err);
    }

    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS PullUserInfo2(void **buffer, UserInfo21 *ui, uint32 num)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_2 *info = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(buffer);
    BAIL_ON_INVALID_PTR(ui);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_2) * num,
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].usri2_name, ui[i].account_name, info);
        info[i].usri2_password = NULL;
        /* info[i].usri1_password_age */
        /* info[i].usri1_priv */
        PULL_UNICODE_STRING(info[i].usri2_home_dir, ui[i].home_directory, info);
        PULL_UNICODE_STRING(info[i].usri2_comment, ui[i].comment, info);
        PULL_ACCOUNT_FLAGS(info[i].usri2_flags, ui[i].account_flags);
        PULL_UNICODE_STRING(info[i].usri2_script_path, ui[i].logon_script, info);

        /* info[i].usri2_auth_flags */
        PULL_UNICODE_STRING(info[i].usri2_full_name, ui[i].full_name, info);
        /* info[i].usri2_usr_comment */
        /* info[i].usri2_parms */
        PULL_UNICODE_STRING(info[i].usri2_workstations, ui[i].workstations, info);
        info[i].usri2_last_logon = ui[i].last_logon;
        info[i].usri2_last_logoff = ui[i].last_logoff;
        info[i].usri2_acct_expires = ui[i].account_expiry;
        /* info[i].usri2_max_storage */
        info[i].usri2_units_per_week = ui[i].logon_hours.units_per_week;
        /* info[i].usri2_logon_hours */
        info[i].usri2_bad_pw_count = ui[i].bad_password_count;
        info[i].usri2_num_logons = ui[i].logon_count;
        /* info[i].usri2_logon_server */
        info[i].usri2_country_code = ui[i].country_code;
        info[i].usri2_code_page = ui[i].code_page;
    }

    *buffer = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS PullUserInfo20(void **buffer, UserInfo21 *ui, uint32 num)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_20 *info = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(buffer);
    BAIL_ON_INVALID_PTR(ui);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_20) * num,
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].usri20_name, ui[i].account_name, info);
        PULL_UNICODE_STRING(info[i].usri20_full_name, ui[i].full_name, info);
        PULL_UNICODE_STRING(info[i].usri20_comment, ui[i].comment, info);
        PULL_ACCOUNT_FLAGS(info[i].usri20_flags, ui[i].account_flags);
        info[i].usri20_user_id = ui[i].rid;
    }

    *buffer = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS EncPasswordEx(uint8 pwbuf[532], wchar16_t *password,
                       uint32 password_len, NetConn *conn)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    MD5_CTX ctx;
    RC4_KEY rc4_key;
    uint8 initval[16], digested_sess_key[16];

    BAIL_ON_INVALID_PTR(pwbuf);
    BAIL_ON_INVALID_PTR(password);
    BAIL_ON_INVALID_PTR(conn);

    memset(&ctx, 0, sizeof(ctx));
    memset(initval, 0, sizeof(initval));
    memset(digested_sess_key, 0, sizeof(digested_sess_key));

    EncodePassBufferW16(pwbuf, password);

    get_random_buffer((unsigned char*)initval, sizeof(initval));

    MD5_Init(&ctx);
    MD5_Update(&ctx, initval, 16);
    MD5_Update(&ctx, conn->sess_key, conn->sess_key_len);
    MD5_Final(digested_sess_key, &ctx);

    RC4_set_key(&rc4_key, 16, (unsigned char*)digested_sess_key);
    RC4(&rc4_key, 516, (unsigned char*)pwbuf, (unsigned char*)pwbuf);
    memcpy((void*)&pwbuf[516], initval, 16);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS PushUserInfoAdd(UserInfo **sinfo, uint32 *slevel, void *ptr,
                         uint32 nlevel, uint32 *parm_err)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;
    USER_INFO_X *ninfo = NULL;

    BAIL_ON_INVALID_PTR(sinfo);
    BAIL_ON_INVALID_PTR(slevel);
    BAIL_ON_INVALID_PTR(ptr);
    /* parm_err is optional and can be NULL */

    if (nlevel < 1 || nlevel > 4) {
        status = STATUS_INVALID_LEVEL;
        goto error;
    }

    ninfo = ptr;

    if (parm_err) {
        *parm_err = 0;
    }


    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    *slevel = 21;
    info21  = &info->info21;

    /* privileges field must be set to USER_PRIV_USER when
       calling NetUserAdd function */
    if (ninfo->priv != USER_PRIV_USER) {
        if (parm_err) {
            *parm_err = (void*)&ninfo->priv - (void*)ninfo;
        }

        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->home_dir, home_directory,
                                 SAMR_FIELD_HOME_DIRECTORY, info);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->comment, comment,
                                 SAMR_FIELD_COMMENT, info);

    PUSH_ACCOUNT_FLAGS(info21, ninfo->flags, account_flags,
                       SAMR_FIELD_ACCT_FLAGS);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->script_path, logon_script,
                                 SAMR_FIELD_LOGON_SCRIPT, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo0(UserInfo **sinfo, uint32 *slevel, USER_INFO_0 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    BAIL_ON_INVALID_PTR(sinfo);
    BAIL_ON_INVALID_PTR(slevel);
    BAIL_ON_INVALID_PTR(ninfo);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri0_name, account_name,
                                 SAMR_FIELD_ACCOUNT_NAME, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1(UserInfo **sinfo, uint32 *slevel, USER_INFO_1 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    BAIL_ON_INVALID_PTR(sinfo);
    BAIL_ON_INVALID_PTR(slevel);
    BAIL_ON_INVALID_PTR(ninfo);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1_home_dir, home_directory,
                                 SAMR_FIELD_HOME_DIRECTORY, info);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1_comment, comment,
                                 SAMR_FIELD_COMMENT, info);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1_script_path, logon_script,
                                 SAMR_FIELD_LOGON_SCRIPT, info);

    PUSH_ACCOUNT_FLAGS(info21, ninfo->usri1_flags, account_flags,
                       SAMR_FIELD_ACCT_FLAGS);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1003(UserInfo **sinfo, uint32 *slevel, USER_INFO_1003 *ninfo,
                          NetConn *conn)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo25 *info25 = NULL;
    wchar16_t *password = NULL;
    size_t password_len = 0;

    BAIL_ON_INVALID_PTR(sinfo);
    BAIL_ON_INVALID_PTR(slevel);
    BAIL_ON_INVALID_PTR(ninfo);
    BAIL_ON_INVALID_PTR(conn);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    *slevel = 25;
    info25 = &info->info25;

    password = wc16sdup(ninfo->usri1003_password);
    BAIL_ON_NO_MEMORY(password);

    password_len = wc16slen(password);
    status = EncPasswordEx(info25->password.data, password, password_len, conn);
    BAIL_ON_NTSTATUS_ERROR(status);

    info25->info.fields_present = SAMR_FIELD_PASSWORD;

    *sinfo = info;

cleanup:
    SAFE_FREE(password);
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1007(UserInfo **sinfo, uint32 *slevel, USER_INFO_1007 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    BAIL_ON_INVALID_PTR(sinfo);
    BAIL_ON_INVALID_PTR(slevel);
    BAIL_ON_INVALID_PTR(ninfo);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1007_comment, comment,
                                 SAMR_FIELD_COMMENT, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1008(UserInfo **sinfo, uint32 *slevel, USER_INFO_1008 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    BAIL_ON_INVALID_PTR(sinfo);
    BAIL_ON_INVALID_PTR(slevel);
    BAIL_ON_INVALID_PTR(ninfo);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_ACCOUNT_FLAGS(info21, ninfo->usri1008_flags, account_flags,
                       SAMR_FIELD_ACCT_FLAGS);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1011(UserInfo **sinfo, uint32 *slevel, USER_INFO_1011 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    BAIL_ON_INVALID_PTR(sinfo);
    BAIL_ON_INVALID_PTR(slevel);
    BAIL_ON_INVALID_PTR(ninfo);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1011_full_name, full_name,
                                 SAMR_FIELD_FULL_NAME, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
