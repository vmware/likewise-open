/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compat/rpcstatus.h>
#include <dce/dce_error.h>

#include <types.h>
#include <security.h>
#include <wc16str.h>
#include <srvsvc.h>
#include <srvsvcbinding.h>

#include "../client/SrvSvcUtil.h"
#include "TestSrvSvc.h"
#include "Params.h"

static const char *Win32ErrorToName(NET_API_STATUS err)
{
    static char buf[64];

    switch (err) {
    case ERROR_SUCCESS:
        return "ERROR_SUCCESS";
    case ERROR_FILE_NOT_FOUND:
        return "ERROR_FILE_NOT_FOUND";
    case ERROR_NOT_ENOUGH_MEMORY:
        return "ERROR_NOT_ENOUGH_MEMORY";
    case ERROR_BAD_NET_RESP:
        return "ERROR_BAD_NET_RESP";
    case ERROR_INVALID_PARAMETER:
        return "ERROR_INVALID_PARAMETER";
    case ERROR_INVALID_LEVEL:
        return "ERROR_INVALID_LEVEL";
    case ERROR_INTERNAL_ERROR:
        return "ERROR_INTERNAL_ERROR";
    case ERROR_BAD_DESCRIPTOR_FORMAT:
        return "ERROR_BAD_DESCRIPTOR_FORMAT";
    case ERROR_INVALID_SUB_AUTHORITY:
        return "ERROR_INVALID_SUB_AUTHORITY";
    case ERROR_INVALID_ACL:
        return "ERROR_INVALID_ACL";
    case ERROR_INVALID_SID:
        return "ERROR_INVALID_SID";
    case ERROR_INVALID_SECURITY_DESCR:
        return "ERROR_INVALID_SECURITY_DESCR";
    case NERR_DuplicateShare:
        return "NERR_DuplicateShare";
    case NERR_BufTooSmall:
        return "NERR_BufTooSmall";
    case NERR_NetNameNotFound:
        return "NERR_NetNameNotFound";
    case NERR_FileIdNotFound:
        return "NERR_FileIdNotFound";
    }

    snprintf(buf, sizeof(buf)-1,
             "Win32Error[0x%08X/%u]",
             err, err);

    return buf;
}

handle_t CreateSrvSvcBinding(handle_t *binding, const wchar16_t *host)
{
    RPCSTATUS status;
    size_t hostname_size;
    unsigned char *hostname;

    if (binding == NULL || host == NULL) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (unsigned char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;
    wc16stombs(hostname, host, hostname_size);

    status = InitSrvSvcBindingDefault(binding, hostname);
    if (status != RPC_S_OK) {
        int result;
        CHAR_T errmsg[dce_c_error_string_len];
	
        dce_error_inq_text(status, errmsg, &result);
        if (result == 0) {
            printf("Error: %s\n", errmsg);
        } else {
            printf("Unknown error: %08x\n", status);
        }

        return NULL;
    }

    free(hostname);
    return *binding;
}


int TestNetConnectionEnum(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 resume_handle = 0;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetConnectionEnum(srvsvc_binding,
					hostname,/*servername*/
					ambstowc16s("IPC$"),/*qualifier*/
					0,/*level*/
					&bufptr,/*bufptr*/
					0,/*prefmaxlen*/
					&entriesread,/*entriesread*/
					&totalentries,/*totalentries*/
					NULL/*resume_handle*/
					));
    if (err != NERR_BufTooSmall) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetConnectionEnum(srvsvc_binding,
					hostname,/*servername*/
					ambstowc16s("IPC$"),/*qualifier*/
					0,/*level*/
					&bufptr,/*bufptr*/
					0xFFFFFFFF,/*prefmaxlen*/
					&entriesread,/*entriesread*/
					&totalentries,/*totalentries*/
					&resume_handle/*resume_handle*/
					));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetConnectionEnum(srvsvc_binding,
					hostname,/*servername*/
					ambstowc16s("IPC$"),/*qualifier*/
					0,/*level*/
					&bufptr,/*bufptr*/
					0xFFFFFFFF,/*prefmaxlen*/
					&entriesread,/*entriesread*/
					&totalentries,/*totalentries*/
					&resume_handle/*resume_handle*/
					));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetConnectionEnum(srvsvc_binding,
					hostname,/*servername*/
					ambstowc16s("IPC$"),/*qualifier*/
					1,/*level*/
					&bufptr,/*bufptr*/
					0,/*prefmaxlen*/
					&entriesread,/*entriesread*/
					&totalentries,/*totalentries*/
					NULL/*resume_handle*/
					));
    if (err != NERR_BufTooSmall) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetConnectionEnum(srvsvc_binding,
					hostname,/*servername*/
					ambstowc16s("IPC$"),/*qualifier*/
					1,/*level*/
					&bufptr,/*bufptr*/
					0xFFFFFFFF,/*prefmaxlen*/
					&entriesread,/*entriesread*/
					&totalentries,/*totalentries*/
					&resume_handle/*resume_handle*/
					));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetConnectionEnum(srvsvc_binding,
					hostname,/*servername*/
					ambstowc16s("IPC$"),/*qualifier*/
					1,/*level*/
					&bufptr,/*bufptr*/
					0xFFFFFFFF,/*prefmaxlen*/
					&entriesread,/*entriesread*/
					&totalentries,/*totalentries*/
					&resume_handle/*resume_handle*/
					));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetConnectionEnum(srvsvc_binding,
					hostname,/*servername*/
					ambstowc16s("IPC$"),/*qualifier*/
					2,/*level*/
					&bufptr,/*bufptr*/
					0xFFFFFFFF,/*prefmaxlen*/
					&entriesread,/*entriesread*/
					&totalentries,/*totalentries*/
					&resume_handle/*resume_handle*/
					));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetFileEnum(struct test *t, const wchar16_t *hostname,
                    const wchar16_t *user, const wchar16_t *pass,
                    struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 resume_handle = 0;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  2,/*level*/
                                  &bufptr,/*bufptr*/
                                  0,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  NULL/*resume_handle*/
                                  ));
    if (err != NERR_BufTooSmall) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  2,/*level*/
                                  &bufptr,/*bufptr*/
                                  0xFFFFFFFF,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  &resume_handle/*resume_handle*/
                                  ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  2,/*level*/
                                  &bufptr,/*bufptr*/
                                  0xFFFFFFFF,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  &resume_handle/*resume_handle*/
                                  ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  3,/*level*/
                                  &bufptr,/*bufptr*/
                                  0,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  NULL/*resume_handle*/
                                  ));
    if (err != NERR_BufTooSmall) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  3,/*level*/
                                  &bufptr,/*bufptr*/
                                  0xFFFFFFFF,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  &resume_handle/*resume_handle*/
                                  ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  3,/*level*/
                                  &bufptr,/*bufptr*/
                                  0xFFFFFFFF,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  &resume_handle/*resume_handle*/
                                  ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  0,/*level*/
                                  &bufptr,/*bufptr*/
                                  0xFFFFFFFF,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  &resume_handle/*resume_handle*/
                                  ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  123,/*level*/
                                  &bufptr,/*bufptr*/
                                  0xFFFFFFFF,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  NULL//&resume_handle/*resume_handle*/
                                  ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetFileGetInfo(struct test *t, const wchar16_t *hostname,
                       const wchar16_t *user, const wchar16_t *pass,
                       struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    NET_API_STATUS unknownlevel_err = ERROR_INVALID_LEVEL;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 resume_handle = 0;
    FILE_INFO_2 *fi2_enum = NULL;
    uint32 i;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    CALL_NETAPI(err = NetFileGetInfo(srvsvc_binding,
                                     hostname,/*servername*/
                                     0,/*fileid*/
                                     2,/*level*/
                                     &bufptr/*bufptr*/
                                     ));
    if (err != ERROR_FILE_NOT_FOUND) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);

    bufptr = NULL;
    CALL_NETAPI(err = NetFileGetInfo(srvsvc_binding,
                                     hostname,/*servername*/
                                     0,/*fileid*/
                                     3,/*level*/
                                     &bufptr/*bufptr*/
                                     ));
    if (err != ERROR_FILE_NOT_FOUND) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);

    bufptr = NULL;
    CALL_NETAPI(err = NetFileGetInfo(srvsvc_binding,
                                     hostname,/*servername*/
                                     0,/*fileid*/
                                     123,/*level*/
                                     &bufptr/*bufptr*/
                                     ));
    if (err != ERROR_INVALID_LEVEL) {
        /* w2k returnd FILE_NOT_FOUND for all unknown levels */
        if (err != ERROR_FILE_NOT_FOUND) netapi_fail(err);
    }
    unknownlevel_err = err;
    printf("bufptr[%p]\n", bufptr);

    bufptr = NULL;
    CALL_NETAPI(err = NetFileGetInfo(srvsvc_binding,
                                     hostname,/*servername*/
                                     0,/*fileid*/
                                     1,/*level*/
                                     &bufptr/*bufptr*/
                                     ));
    if (err != unknownlevel_err) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);

    bufptr = NULL;
    CALL_NETAPI(err = NetFileGetInfo(srvsvc_binding,
                                     hostname,/*servername*/
                                     0,/*fileid*/
                                     0,/*level*/
                                     &bufptr/*bufptr*/
                                     ));
    if (err != unknownlevel_err) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetFileEnum(srvsvc_binding,
                                  hostname,/*servername*/
                                  NULL,/*basepath*/
                                  NULL,/*username*/
                                  2,/*level*/
                                  &bufptr,/*bufptr*/
                                  0xFFFFFFFF,/*prefmaxlen*/
                                  &entriesread,/*entriesread*/
                                  &totalentries,/*totalentries*/
                                  &resume_handle/*resume_handle*/
                                  ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    fi2_enum = (PFILE_INFO_2)bufptr;
    for (i=0; i< entriesread; i++) {

        bufptr = NULL;
        CALL_NETAPI(err = NetFileGetInfo(srvsvc_binding,
                                         hostname,/*servername*/
                                         fi2_enum[i].fi2_id,/*fileid*/
                                         2,/*level*/
                                         &bufptr/*bufptr*/
                                         ));
        if (err != ERROR_SUCCESS) netapi_fail(err);
        printf("bufptr[%p]\n", bufptr);

        bufptr = NULL;
        CALL_NETAPI(err = NetFileGetInfo(srvsvc_binding,
                                         hostname,/*servername*/
                                         fi2_enum[i].fi2_id,/*fileid*/
                                         3,/*level*/
                                         &bufptr/*bufptr*/
                                         ));
        if (err != ERROR_SUCCESS) netapi_fail(err);
        printf("bufptr[%p]\n", bufptr);
    }

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetFileClose(struct test *t, const wchar16_t *hostname,
                     const wchar16_t *user, const wchar16_t *pass,
                     struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    CALL_NETAPI(err = NetFileClose(srvsvc_binding,
                                   hostname,/*servername*/
                                   0/*fileid*/
                                   ));
    if (err != NERR_FileIdNotFound) netapi_fail(err);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetSessionEnum(struct test *t, const wchar16_t *hostname,
                       const wchar16_t *user, const wchar16_t *pass,
                       struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 resume_handle = 0;
    const uint32 levels[5] = { 0, 1, 2, 10, 502 };
    uint32 i;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    for (i=0; i < 5; i++) {

        bufptr = NULL;
        entriesread = 0;
        totalentries = 0;
        resume_handle = 0;
        CALL_NETAPI(err = NetSessionEnum(srvsvc_binding,
                                         hostname,/*servername*/
                                         NULL, /*unc client name */
                                         NULL, /*username */
                                         levels[i],/*level*/
                                         &bufptr,/*bufptr*/
                                         0,/*prefmaxlen*/
                                         &entriesread,/*entriesread*/
                                         &totalentries,/*totalentries*/
                                         NULL/*resume_handle*/
                                         ));
        if (err != NERR_BufTooSmall) netapi_fail(err);
        printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
               bufptr, entriesread, totalentries, resume_handle);

        bufptr = NULL;
        entriesread = 0;
        totalentries = 0;
        resume_handle = 0;
        CALL_NETAPI(err = NetSessionEnum(srvsvc_binding,
                                         hostname,/*servername*/
                                         NULL, /*unc client name */
                                         NULL, /*username */
                                         levels[i],/*level*/
                                         &bufptr,/*bufptr*/
                                         0xFFFFFFFF,/*prefmaxlen*/
                                         &entriesread,/*entriesread*/
                                         &totalentries,/*totalentries*/
                                         &resume_handle/*resume_handle*/
                                         ));
        if (err != ERROR_SUCCESS) netapi_fail(err);
        printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
               bufptr, entriesread, totalentries, resume_handle);

        bufptr = NULL;
        entriesread = 0;
        totalentries = 0;
        CALL_NETAPI(err = NetSessionEnum(srvsvc_binding,
                                         hostname,/*servername*/
                                         NULL, /*unc client name */
                                         NULL, /*username */
                                         levels[i],/*level*/
                                         &bufptr,/*bufptr*/
                                         0xFFFFFFFF,/*prefmaxlen*/
                                         &entriesread,/*entriesread*/
                                         &totalentries,/*totalentries*/
                                         &resume_handle/*resume_handle*/
                                         ));
        if (err != ERROR_SUCCESS) netapi_fail(err);
        printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
               bufptr, entriesread, totalentries, resume_handle);

    }

    /* test invalid level 123 */
    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetSessionEnum(srvsvc_binding,
                                     hostname,/*servername*/
                                     NULL, /*unc client name */
                                     NULL, /*username */
                                     123,/*level*/
                                     &bufptr,/*bufptr*/
                                     0xFFFFFFFF,/*prefmaxlen*/
                                     &entriesread,/*entriesread*/
                                     &totalentries,/*totalentries*/
                                     &resume_handle/*resume_handle*/
                                     ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetShareAdd(struct test *t, const wchar16_t *hostname,
                    const wchar16_t *user, const wchar16_t *pass,
                    struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    SHARE_INFO_0 info0;
    SHARE_INFO_2 info2;
    SHARE_INFO_502 info502;
    PSHARE_INFO_502 shi502_enum;
    PSHARE_INFO_502 ginfo502 = NULL;
    uint8 *bufptr = NULL;
    uint32 parm_err = 0;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 i;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    parm_err = 0;
    CALL_NETAPI(err = NetShareAdd(srvsvc_binding,
                                  hostname,/*servername*/
                                  0,/*level*/
                                  bufptr,/*bufptr*/
                                  NULL/*parm_err*/
                                  ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = NULL;
    parm_err = 0;
    CALL_NETAPI(err = NetShareAdd(srvsvc_binding,
                                  hostname,/*servername*/
                                  0,/*level*/
                                  bufptr,/*bufptr*/
                                  &parm_err/*parm_err*/
                                  ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info2;
    parm_err = 0;
    info2.shi2_netname      = ambstowc16s("C$");
    info2.shi2_type         = 0;/*STYPE_DISKTREE*/
    info2.shi2_remark       = NULL;
    info2.shi2_permissions  = 0;
    info2.shi2_max_uses     = 10;
    info2.shi2_current_uses = 0;
    info2.shi2_path         = ambstowc16s("C:\\");
    info2.shi2_password     = NULL;
    CALL_NETAPI(err = NetShareAdd(srvsvc_binding,
                                  hostname,/*servername*/
                                  2,/*level*/
                                  bufptr,/*bufptr*/
                                  &parm_err/*parm_err*/
                                  ));
    if (err != NERR_DuplicateShare) netapi_fail(err);

    bufptr = &info502;
    parm_err = 0;
    info502.shi502_netname             = ambstowc16s("C$");
    info502.shi502_type                = 0;/*STYPE_DISKTREE*/
    info502.shi502_remark              = info502.shi502_netname;
    info502.shi502_permissions         = 0;
    info502.shi502_max_uses            = 10;
    info502.shi502_current_uses        = 0;
    info502.shi502_path                = ambstowc16s("C:\\");
    info502.shi502_password            = NULL;
    info502.shi502_reserved            = 0;
    info502.shi502_security_descriptor = NULL;
    CALL_NETAPI(err = NetShareAdd(srvsvc_binding,
                                  hostname,/*servername*/
                                  502,/*level*/
                                  bufptr,/*bufptr*/
                                  &parm_err/*parm_err*/
                                  ));
    if (err != NERR_DuplicateShare) netapi_fail(err);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                   hostname,/*servername*/
                                   502,/*level*/
                                   &bufptr,/*bufptr*/
                                   0xFFFFFFFF,/*prefmaxlen*/
                                   &entriesread,/*entriesread*/
                                   &totalentries,/*totalentries*/
                                   NULL/*resume_handle*/
                                   ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u]\n",
           bufptr, entriesread, totalentries);

    shi502_enum = (PSHARE_INFO_502)bufptr;
    for (i=0; i < entriesread; i++) {
         if (shi502_enum[i].shi502_security_descriptor) {
             ginfo502 = &shi502_enum[i];
             break;
         }
    }

    printf("returned info502[%p]\n", ginfo502);
    if (ginfo502) {
        printf("found [%s] security descriptor[%p]\n",
               awc16stombs(ginfo502->shi502_netname),
               ginfo502->shi502_security_descriptor);

        bufptr = &info502;
        parm_err = 0;
        info502.shi502_netname             = ambstowc16s("C$");
        info502.shi502_type                = 0;/*STYPE_DISKTREE*/
        info502.shi502_remark              = info502.shi502_netname;
        info502.shi502_permissions         = 0;
        info502.shi502_max_uses            = 10;
        info502.shi502_current_uses        = 0;
        info502.shi502_path                = ambstowc16s("C:\\");
        info502.shi502_password            = NULL;
        info502.shi502_reserved            = 0;
        info502.shi502_security_descriptor = ginfo502->shi502_security_descriptor;
        CALL_NETAPI(err = NetShareAdd(srvsvc_binding,
                                      hostname,/*servername*/
                                      502,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
        if (err != NERR_DuplicateShare) netapi_fail(err);
    }

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetShareEnum(struct test *t, const wchar16_t *hostname,
                     const wchar16_t *user, const wchar16_t *pass,
                     struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 resume_handle = 0;
    const uint32 levels[5] = { 0, 1, 2, 501, 502 };
    uint32 i;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    for (i=0; i < 5; i++) {

        bufptr = NULL;
        entriesread = 0;
        totalentries = 0;
        resume_handle = 0;
        CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                       hostname,/*servername*/
                                       levels[i],/*level*/
                                       &bufptr,/*bufptr*/
                                       0,/*prefmaxlen*/
                                       &entriesread,/*entriesread*/
                                       &totalentries,/*totalentries*/
                                       NULL/*resume_handle*/
                                       ));
        if (err != NERR_BufTooSmall) netapi_fail(err);
        printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
               bufptr, entriesread, totalentries, resume_handle);

        bufptr = NULL;
        entriesread = 0;
        totalentries = 0;
        resume_handle = 0;
        CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                       hostname,/*servername*/
                                       levels[i],/*level*/
                                       &bufptr,/*bufptr*/
                                       0xFFFFFFFF,/*prefmaxlen*/
                                       &entriesread,/*entriesread*/
                                       &totalentries,/*totalentries*/
                                       &resume_handle/*resume_handle*/
                                       ));
        if (err != ERROR_SUCCESS) netapi_fail(err);
        printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
               bufptr, entriesread, totalentries, resume_handle);

        bufptr = NULL;
        entriesread = 0;
        totalentries = 0;
        CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                       hostname,/*servername*/
                                       levels[i],/*level*/
                                       &bufptr,/*bufptr*/
                                       0xFFFFFFFF,/*prefmaxlen*/
                                       &entriesread,/*entriesread*/
                                       &totalentries,/*totalentries*/
                                       &resume_handle/*resume_handle*/
                                       ));
        if (err != ERROR_SUCCESS) netapi_fail(err);
        printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
               bufptr, entriesread, totalentries, resume_handle);

    }

    /* test invalid level 123 */
    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                   hostname,/*servername*/
                                   123,/*level*/
                                   &bufptr,/*bufptr*/
                                   0xFFFFFFFF,/*prefmaxlen*/
                                   &entriesread,/*entriesread*/
                                   &totalentries,/*totalentries*/
                                   &resume_handle/*resume_handle*/
                                   ));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    /* test invalid level 1502 */
    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    resume_handle = 0;
    CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                   hostname,/*servername*/
                                   1502,/*level*/
                                   &bufptr,/*bufptr*/
                                   0xFFFFFFFF,/*prefmaxlen*/
                                   &entriesread,/*entriesread*/
                                   &totalentries,/*totalentries*/
                                   &resume_handle/*resume_handle*/
                                   ));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);


    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetShareGetInfo(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 resume_handle = 0;
    SHARE_INFO_0 *shi0_enum = NULL;
    uint32 i;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                   hostname,/*servername*/
                                   0,/*level*/
                                   &bufptr,/*bufptr*/
                                   0xFFFFFFFF,/*prefmaxlen*/
                                   &entriesread,/*entriesread*/
                                   &totalentries,/*totalentries*/
                                   &resume_handle/*resume_handle*/
                                   ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u] resume_handle[%u]\n",
           bufptr, entriesread, totalentries, resume_handle);

    shi0_enum = (PSHARE_INFO_0)bufptr;
    for (i=0; i< entriesread; i++) {
        uint32 levels[5] = { 0, 1, 2, 501, 502, 1005 };
        uint32 y;

        for (y=0; y < 5; y++) {
           bufptr = NULL;
           CALL_NETAPI(err = NetShareGetInfo(srvsvc_binding,
                                             hostname,/*servername*/
                                             shi0_enum[i].shi0_netname,/*netname*/
                                             levels[y],/*level*/
                                             &bufptr/*bufptr*/
                                             ));
           if (err != ERROR_SUCCESS) netapi_fail(err);
           printf("bufptr[%p]\n", bufptr);
        }

        bufptr = NULL;
        CALL_NETAPI(err = NetShareGetInfo(srvsvc_binding,
                                          hostname,/*servername*/
                                          shi0_enum[i].shi0_netname,/*netname*/
                                          1004,/*level*/
                                          &bufptr/*bufptr*/
                                          ));
        if (err != ERROR_INVALID_LEVEL) netapi_fail(err);
        printf("bufptr[%p]\n", bufptr);
    }

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetShareSetInfo(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    SHARE_INFO_1 info1;
    SHARE_INFO_2 info2;
    SHARE_INFO_502 info502;
    SHARE_INFO_1004 info1004;
    SHARE_INFO_1005 info1005;
    SHARE_INFO_1006 info1006;
    SHARE_INFO_1501 info1501;
    PSHARE_INFO_502 shi502_enum;
    PSHARE_INFO_502 ginfo502 = NULL;
    uint8 *bufptr = NULL;
    uint32 parm_err = 0;
    uint32 entriesread = 0;
    uint32 totalentries = 0;
    uint32 i;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    parm_err = 0;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      ambstowc16s("C$"),/*netname*/
                                      0,/*level*/
                                      bufptr,/*bufptr*/
                                      NULL/*parm_err*/
                                      ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = NULL;
    parm_err = 0;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      ambstowc16s("C$"),/*netname*/
                                      0,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = NULL;
    entriesread = 0;
    totalentries = 0;
    CALL_NETAPI(err = NetShareEnum(srvsvc_binding,
                                   hostname,/*servername*/
                                   502,/*level*/
                                   &bufptr,/*bufptr*/
                                   0xFFFFFFFF,/*prefmaxlen*/
                                   &entriesread,/*entriesread*/
                                   &totalentries,/*totalentries*/
                                   NULL/*resume_handle*/
                                   ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p] entriesread[%u] totalentries[%u]\n",
           bufptr, entriesread, totalentries);

    shi502_enum = (PSHARE_INFO_502)bufptr;
    for (i=0; i < entriesread; i++) {
         if (shi502_enum[i].shi502_security_descriptor) {
             ginfo502 = &shi502_enum[i];
             break;
         }
    }

    printf("returned info502[%p]\n", ginfo502);
    if (ginfo502) {
        printf("found [%s] security descriptor[%p]\n",
               awc16stombs(ginfo502->shi502_netname),
               ginfo502->shi502_security_descriptor);

        bufptr = ginfo502;
        parm_err = 0;
        CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                          hostname,/*servername*/
                                          ginfo502->shi502_netname,/*netname*/
                                          502,/*level*/
                                          bufptr,/*bufptr*/
                                          &parm_err/*parm_err*/
                                          ));
        if (err != ERROR_SUCCESS) netapi_fail(err);

        bufptr = &info1501;
        parm_err = 0;
        info1501.shi1501_reserved            = ginfo502->shi502_reserved;
        info1501.shi1501_security_descriptor = ginfo502->shi502_security_descriptor;
        CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                          hostname,/*servername*/
                                          ginfo502->shi502_netname,/*netname*/
                                          1501,/*level*/
                                          bufptr,/*bufptr*/
                                          &parm_err/*parm_err*/
                                          ));
        if (err != ERROR_SUCCESS) netapi_fail(err);

        bufptr = NULL;
        parm_err = 0;
        CALL_NETAPI(err = NetShareGetInfo(srvsvc_binding,
                                          hostname,/*servername*/
                                          ginfo502->shi502_netname,/*netname*/
                                          502,/*level*/
                                          &bufptr/*bufptr*/
                                          ));
        if (err != ERROR_SUCCESS) netapi_fail(err);
    }

    bufptr = &info1;
    parm_err = 0;
    info1.shi1_netname      = ambstowc16s("C$D$");
    info1.shi1_type         = 0;/*STYPE_DISKTREE*/
    info1.shi1_remark       = info1.shi1_netname;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      info1.shi1_netname,/*netname*/
                                      1,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    bufptr = &info2;
    parm_err = 0;
    info2.shi2_netname      = ambstowc16s("C$D$");
    info2.shi2_type         = 0;/*STYPE_DISKTREE*/
    info2.shi2_remark       = NULL;
    info2.shi2_permissions  = 0;
    info2.shi2_max_uses     = 10;
    info2.shi2_current_uses = 0;
    info2.shi2_path         = ambstowc16s("C:\\");
    info2.shi2_password     = NULL;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      info2.shi2_netname,/*netname*/
                                      2,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    bufptr = &info502;
    parm_err = 0;
    info502.shi502_netname             = ambstowc16s("C$D$");
    info502.shi502_type                = 0;/*STYPE_DISKTREE*/
    info502.shi502_remark              = NULL;
    info502.shi502_permissions         = 0;
    info502.shi502_max_uses            = 10;
    info502.shi502_current_uses        = 0;
    info502.shi502_path                = ambstowc16s("C:\\");
    info502.shi502_password            = NULL;
    info502.shi502_reserved            = 0;
    info502.shi502_security_descriptor = NULL;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      info502.shi502_netname,/*netname*/
                                      502,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    bufptr = &info1004;
    parm_err = 0;
    info1004.shi1004_remark = ambstowc16s("C-REMARK");
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      ambstowc16s("C$D$"),/*netname*/
                                      1004,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    bufptr = &info1005;
    parm_err = 0;
    info1005.shi1005_flags = 0;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      ambstowc16s("C$D$"),/*netname*/
                                      1005,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    bufptr = &info1006;
    parm_err = 0;
    info1006.shi1006_max_uses = 20;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      ambstowc16s("C$D$"),/*netname*/
                                      1006,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    bufptr = &info1501;
    parm_err = 0;
    info1501.shi1501_reserved            = 0;
    info1501.shi1501_security_descriptor = NULL;
    CALL_NETAPI(err = NetShareSetInfo(srvsvc_binding,
                                      hostname,/*servername*/
                                      ambstowc16s("C$D$"),/*netname*/
                                      1501,/*level*/
                                      bufptr,/*bufptr*/
                                      &parm_err/*parm_err*/
                                      ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetShareDel(struct test *t, const wchar16_t *hostname,
                    const wchar16_t *user, const wchar16_t *pass,
                    struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    CALL_NETAPI(err = NetShareDel(srvsvc_binding,
                                  hostname,/*servername*/
                                  ambstowc16s("C$D$"),/*netname*/
                                  0/*reserved*/
                                  ));
    if (err != NERR_NetNameNotFound) netapi_fail(err);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetServerGetInfo(struct test *t, const wchar16_t *hostname,
                         const wchar16_t *user, const wchar16_t *pass,
                         struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 i;
    uint32 levels[] = { 100, 101, 102, 502, 503 };
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    for (i=0; i < 5; i++) {
       bufptr = NULL;
       CALL_NETAPI(err = NetServerGetInfo(srvsvc_binding,
                                          hostname,/*servername*/
                                          levels[i],/*level*/
                                          &bufptr/*bufptr*/
                                          ));
       if (err != ERROR_SUCCESS) netapi_fail(err);
       printf("bufptr[%p]\n", bufptr);
    }

    bufptr = NULL;
    CALL_NETAPI(err = NetServerGetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       403,/*level*/
                                       &bufptr/*bufptr*/
                                       ));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);

    bufptr = NULL;
    CALL_NETAPI(err = NetServerGetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       789,/*level*/
                                       &bufptr/*bufptr*/
                                       ));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetServerSetInfo(struct test *t, const wchar16_t *hostname,
                         const wchar16_t *user, const wchar16_t *pass,
                         struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
    uint32 parm_err = 0;
    SERVER_INFO_102 *orig102;
    SERVER_INFO_503 *orig503;
    SERVER_INFO_100 info100;
    SERVER_INFO_101 info101;
    SERVER_INFO_102 info102;
    SERVER_INFO_402 info402;
    SERVER_INFO_403 info403;
    SERVER_INFO_502 info502;
    SERVER_INFO_503 info503;
    SERVER_INFO_599 info599;
    SERVER_INFO_1005 info1005;
    SERVER_INFO_1010 info1010;
    SERVER_INFO_1016 info1016;
    SERVER_INFO_1017 info1017;
    SERVER_INFO_1018 info1018;
    SERVER_INFO_1107 info1107;
    SERVER_INFO_1501 info1501;
    SERVER_INFO_1502 info1502;
    SERVER_INFO_1503 info1503;
    SERVER_INFO_1506 info1506;
    SERVER_INFO_1509 info1509;
    SERVER_INFO_1510 info1510;
    SERVER_INFO_1511 info1511;
    SERVER_INFO_1512 info1512;
    SERVER_INFO_1513 info1513;
    SERVER_INFO_1514 info1514;
    SERVER_INFO_1515 info1515;
    SERVER_INFO_1516 info1516;
    SERVER_INFO_1518 info1518;
    SERVER_INFO_1520 info1520;
    SERVER_INFO_1521 info1521;
    SERVER_INFO_1522 info1522;
    SERVER_INFO_1523 info1523;
    SERVER_INFO_1524 info1524;
    SERVER_INFO_1525 info1525;
    SERVER_INFO_1528 info1528;
    SERVER_INFO_1529 info1529;
    SERVER_INFO_1530 info1530;
    SERVER_INFO_1533 info1533;
    SERVER_INFO_1534 info1534;
    SERVER_INFO_1535 info1535;
    SERVER_INFO_1536 info1536;
    SERVER_INFO_1537 info1537;
    SERVER_INFO_1538 info1538;
    SERVER_INFO_1539 info1539;
    SERVER_INFO_1540 info1540;
    SERVER_INFO_1541 info1541;
    SERVER_INFO_1542 info1542;
    SERVER_INFO_1543 info1543;
    SERVER_INFO_1544 info1544;
    SERVER_INFO_1545 info1545;
    SERVER_INFO_1546 info1546;
    SERVER_INFO_1547 info1547;
    SERVER_INFO_1548 info1548;
    SERVER_INFO_1549 info1549;
    SERVER_INFO_1550 info1550;
    SERVER_INFO_1552 info1552;
    SERVER_INFO_1553 info1553;
    SERVER_INFO_1554 info1554;
    SERVER_INFO_1555 info1555;
    SERVER_INFO_1556 info1556;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    CALL_NETAPI(err = NetServerGetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       102,/*level*/
                                       &bufptr/*bufptr*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);
    orig102 = (PSERVER_INFO_102)bufptr;

    bufptr = NULL;
    CALL_NETAPI(err = NetServerGetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       503,/*level*/
                                       &bufptr/*bufptr*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);
    orig503 = (PSERVER_INFO_503)bufptr;

    bufptr = NULL;
    parm_err = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       0,/*level*/
                                       bufptr,/*bufptr*/
                                       NULL/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info100;
    parm_err = 0;
    info100.sv100_platform_id = orig102->sv102_platform_id;
    info100.sv100_name        = orig102->sv102_name;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       100,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);

    bufptr = &info101;
    parm_err = 0;
    info101.sv101_platform_id   = orig102->sv102_platform_id;
    info101.sv101_name          = orig102->sv102_name;
    info101.sv101_version_major = orig102->sv102_version_major;
    info101.sv101_version_minor = orig102->sv102_version_minor;
    info101.sv101_type          = orig102->sv102_type;
    info101.sv101_comment       = orig102->sv102_comment;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       101,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info102;
    parm_err = 0;
    info102.sv102_platform_id   = orig102->sv102_platform_id;
    info102.sv102_name          = orig102->sv102_name;
    info102.sv102_version_major = orig102->sv102_version_major;
    info102.sv102_version_minor = orig102->sv102_version_minor;
    info102.sv102_type          = orig102->sv102_type;
    info102.sv102_comment       = orig102->sv102_comment;
    info102.sv102_users         = orig102->sv102_users;
    info102.sv102_disc          = orig102->sv102_disc;
    info102.sv102_hidden        = orig102->sv102_hidden;
    info102.sv102_announce      = orig102->sv102_announce;
    info102.sv102_anndelta      = orig102->sv102_anndelta;
    info102.sv102_licenses      = orig102->sv102_licenses;
    info102.sv102_userpath      = orig102->sv102_userpath;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       102,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info402;
    parm_err = 0;
    memset(&info402, 0, sizeof(info402));
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       402,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);

    bufptr = &info403;
    parm_err = 0;
    memset(&info403, 0, sizeof(info403));
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       403,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_LEVEL) netapi_fail(err);

    bufptr = &info502;
    parm_err = 0;
    info502.sv502_sessopens              = orig503->sv503_sessopens;
    info502.sv502_sessvcs                = orig503->sv503_sessvcs;
    info502.sv502_opensearch             = orig503->sv503_opensearch;
    info502.sv502_sizreqbuf              = orig503->sv503_sizreqbuf;
    info502.sv502_initworkitems          = orig503->sv503_initworkitems;
    info502.sv502_maxworkitems           = orig503->sv503_maxworkitems;
    info502.sv502_rawworkitems           = orig503->sv503_rawworkitems;
    info502.sv502_irpstacksize           = orig503->sv503_irpstacksize;
    info502.sv502_maxrawbuflen           = orig503->sv503_maxrawbuflen;
    info502.sv502_sessusers              = orig503->sv503_sessusers;
    info502.sv502_sessconns              = orig503->sv503_sessconns;
    info502.sv502_maxpagedmemoryusage    = orig503->sv503_maxpagedmemoryusage;
    info502.sv502_maxnonpagedmemoryusage = orig503->sv503_maxnonpagedmemoryusage;
    info502.sv502_enablesoftcompat       = orig503->sv503_enablesoftcompat;
    info502.sv502_enableforcedlogoff     = orig503->sv503_enableforcedlogoff;
    info502.sv502_timesource             = orig503->sv503_timesource;
    info502.sv502_acceptdownlevelapis    = orig503->sv503_acceptdownlevelapis;
    info502.sv502_lmannounce             = orig503->sv503_lmannounce;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       502,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info503;
    parm_err = 0;
    info503.sv503_sessopens               = orig503->sv503_sessopens;
    info503.sv503_sessvcs                 = orig503->sv503_sessvcs;
    info503.sv503_opensearch              = orig503->sv503_opensearch;
    info503.sv503_sizreqbuf               = orig503->sv503_sizreqbuf;
    info503.sv503_initworkitems           = orig503->sv503_initworkitems;
    info503.sv503_maxworkitems            = orig503->sv503_maxworkitems;
    info503.sv503_rawworkitems            = orig503->sv503_rawworkitems;
    info503.sv503_irpstacksize            = orig503->sv503_irpstacksize;
    info503.sv503_maxrawbuflen            = orig503->sv503_maxrawbuflen;
    info503.sv503_sessusers               = orig503->sv503_sessusers;
    info503.sv503_sessconns               = orig503->sv503_sessconns;
    info503.sv503_maxpagedmemoryusage     = orig503->sv503_maxpagedmemoryusage;
    info503.sv503_maxnonpagedmemoryusage  = orig503->sv503_maxnonpagedmemoryusage;
    info503.sv503_enablesoftcompat        = orig503->sv503_enablesoftcompat;
    info503.sv503_enableforcedlogoff      = orig503->sv503_enableforcedlogoff;
    info503.sv503_timesource              = orig503->sv503_timesource;
    info503.sv503_acceptdownlevelapis     = orig503->sv503_acceptdownlevelapis;
    info503.sv503_lmannounce              = orig503->sv503_lmannounce;
    info503.sv503_domain                  = orig503->sv503_domain;
    info503.sv503_maxcopyreadlen          = orig503->sv503_maxcopyreadlen;
    info503.sv503_maxcopywritelen         = orig503->sv503_maxcopywritelen;
    info503.sv503_minkeepsearch           = orig503->sv503_minkeepsearch;
    info503.sv503_maxkeepsearch           = orig503->sv503_maxkeepsearch;
    info503.sv503_minkeepcomplsearch      = orig503->sv503_minkeepcomplsearch;
    info503.sv503_maxkeepcomplsearch      = orig503->sv503_maxkeepcomplsearch;
    info503.sv503_threadcountadd          = orig503->sv503_threadcountadd;
    info503.sv503_numblockthreads         = orig503->sv503_numblockthreads;
    info503.sv503_scavtimeout             = orig503->sv503_scavtimeout;
    info503.sv503_minrcvqueue             = orig503->sv503_minrcvqueue;
    info503.sv503_minfreeworkitems        = orig503->sv503_minfreeworkitems;
    info503.sv503_xactmemsize             = orig503->sv503_xactmemsize;
    info503.sv503_threadpriority          = orig503->sv503_threadpriority;
    info503.sv503_maxmpxct                = orig503->sv503_maxmpxct;
    info503.sv503_oplockbreakwait         = orig503->sv503_oplockbreakwait;
    info503.sv503_oplockbreakresponsewait = orig503->sv503_oplockbreakresponsewait;
    info503.sv503_enableoplocks           = orig503->sv503_enableoplocks;
    info503.sv503_enableoplockforceclose  = orig503->sv503_enableoplockforceclose;
    info503.sv503_enablefcbopens          = orig503->sv503_enablefcbopens;
    info503.sv503_enableraw               = orig503->sv503_enableraw;
    info503.sv503_enablesharednetdrives   = orig503->sv503_enablesharednetdrives;
    info503.sv503_minfreeconnections      = orig503->sv503_minfreeconnections;
    info503.sv503_maxfreeconnections      = orig503->sv503_maxfreeconnections;

    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       503,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info599;
    parm_err = 0;
    info599.sv599_sessopens               = orig503->sv503_sessopens;
    info599.sv599_sessvcs                 = orig503->sv503_sessvcs;
    info599.sv599_opensearch              = orig503->sv503_opensearch;
    info599.sv599_sizreqbuf               = orig503->sv503_sizreqbuf;
    info599.sv599_initworkitems           = orig503->sv503_initworkitems;
    info599.sv599_maxworkitems            = orig503->sv503_maxworkitems;
    info599.sv599_rawworkitems            = orig503->sv503_rawworkitems;
    info599.sv599_irpstacksize            = orig503->sv503_irpstacksize;
    info599.sv599_maxrawbuflen            = orig503->sv503_maxrawbuflen;
    info599.sv599_sessusers               = orig503->sv503_sessusers;
    info599.sv599_sessconns               = orig503->sv503_sessconns;
    info599.sv599_maxpagedmemoryusage     = orig503->sv503_maxpagedmemoryusage;
    info599.sv599_maxnonpagedmemoryusage  = orig503->sv503_maxnonpagedmemoryusage;
    info599.sv599_enablesoftcompat        = orig503->sv503_enablesoftcompat;
    info599.sv599_enableforcedlogoff      = orig503->sv503_enableforcedlogoff;
    info599.sv599_timesource              = orig503->sv503_timesource;
    info599.sv599_acceptdownlevelapis     = orig503->sv503_acceptdownlevelapis;
    info599.sv599_lmannounce              = orig503->sv503_lmannounce;
    info599.sv599_domain                  = orig503->sv503_domain;
    info599.sv599_maxcopyreadlen          = orig503->sv503_maxcopyreadlen;
    info599.sv599_maxcopywritelen         = orig503->sv503_maxcopywritelen;
    info599.sv599_minkeepsearch           = orig503->sv503_minkeepsearch;
    info599.sv599_maxkeepsearch           = orig503->sv503_maxkeepsearch;
    info599.sv599_minkeepcomplsearch      = orig503->sv503_minkeepcomplsearch;
    info599.sv599_maxkeepcomplsearch      = orig503->sv503_maxkeepcomplsearch;
    info599.sv599_threadcountadd          = orig503->sv503_threadcountadd;
    info599.sv599_numblockthreads         = orig503->sv503_numblockthreads;
    info599.sv599_scavtimeout             = orig503->sv503_scavtimeout;
    info599.sv599_minrcvqueue             = orig503->sv503_minrcvqueue;
    info599.sv599_minfreeworkitems        = orig503->sv503_minfreeworkitems;
    info599.sv599_xactmemsize             = orig503->sv503_xactmemsize;
    info599.sv599_threadpriority          = orig503->sv503_threadpriority;
    info599.sv599_maxmpxct                = orig503->sv503_maxmpxct;
    info599.sv599_oplockbreakwait         = orig503->sv503_oplockbreakwait;
    info599.sv599_oplockbreakresponsewait = orig503->sv503_oplockbreakresponsewait;
    info599.sv599_enableoplocks           = orig503->sv503_enableoplocks;
    info599.sv599_enableoplockforceclose  = orig503->sv503_enableoplockforceclose;
    info599.sv599_enablefcbopens          = orig503->sv503_enablefcbopens;
    info599.sv599_enableraw               = orig503->sv503_enableraw;
    info599.sv599_enablesharednetdrives   = orig503->sv503_enablesharednetdrives;
    info599.sv599_minfreeconnections      = orig503->sv503_minfreeconnections;
    info599.sv599_maxfreeconnections      = orig503->sv503_maxfreeconnections;
    info599.sv599_initsesstable           = 0;
    info599.sv599_initconntable           = 0;
    info599.sv599_initfiletable           = 0;
    info599.sv599_initsearchtable         = 0;
    info599.sv599_alertschedule           = 0;
    info599.sv599_errorthreshold          = 0;
    info599.sv599_networkerrorthreshold   = 0;
    info599.sv599_diskspacethreshold      = 0;
    info599.sv599_reserved                = 0;
    info599.sv599_maxlinkdelay            = 0;
    info599.sv599_minlinkthroughput       = 0;
    info599.sv599_linkinfovalidtime       = 0;
    info599.sv599_scavqosinfoupdatetime   = 0;
    info599.sv599_maxworkitemidletime     = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       599,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1005;
    parm_err = 0;
    info1005.sv1005_comment = orig102->sv102_comment;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1005,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1010;
    parm_err = 0;
    info1010.sv1010_disc = orig102->sv102_disc;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1010,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1016;
    parm_err = 0;
    info1016.sv1016_hidden = orig102->sv102_hidden;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1016,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1017;
    parm_err = 0;
    info1017.sv1017_announce = orig102->sv102_announce;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1017,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1018;
    parm_err = 0;
    info1018.sv1018_anndelta = orig102->sv102_anndelta;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1018,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1107;
    parm_err = 0;
    info1107.sv1107_users = orig102->sv102_users;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1107,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1501;
    parm_err = 0;
    info1501.sv1501_sessopens = orig503->sv503_sessopens;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1501,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1502;
    parm_err = 0;
    info1502.sv1502_sessvcs = orig503->sv503_sessvcs;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1502,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1503;
    parm_err = 0;
    info1503.sv1503_opensearch = orig503->sv503_opensearch;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1503,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1506;
    parm_err = 0;
    info1506.sv1506_maxworkitems = orig503->sv503_maxworkitems;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1506,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1509;
    parm_err = 0;
    info1509.sv1509_maxrawbuflen = orig503->sv503_maxrawbuflen;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1509,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1510;
    parm_err = 0;
    info1510.sv1510_sessusers = orig503->sv503_sessusers;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1510,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1511;
    parm_err = 0;
    info1511.sv1511_sessconns = orig503->sv503_sessconns;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1511,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1512;
    parm_err = 0;
    info1512.sv1512_maxnonpagedmemoryusage = orig503->sv503_maxnonpagedmemoryusage;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1512,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1513;
    parm_err = 0;
    info1513.sv1513_maxpagedmemoryusage = orig503->sv503_maxpagedmemoryusage;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1513,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1514;
    parm_err = 0;
    info1514.sv1514_enablesoftcompat = orig503->sv503_enablesoftcompat;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1514,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1515;
    parm_err = 0;
    info1515.sv1515_enableforcedlogoff = orig503->sv503_enableforcedlogoff;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1515,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1516;
    parm_err = 0;
    info1516.sv1516_timesource = orig503->sv503_timesource;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1516,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1518;
    parm_err = 0;
    info1518.sv1518_lmannounce = orig503->sv503_lmannounce;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1518,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1520;
    parm_err = 0;
    info1520.sv1520_maxcopyreadlen = orig503->sv503_maxcopyreadlen;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1520,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1521;
    parm_err = 0;
    info1521.sv1521_maxcopywritelen = orig503->sv503_maxcopywritelen;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1521,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1522;
    parm_err = 0;
    info1522.sv1522_minkeepsearch = orig503->sv503_minkeepsearch;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1522,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1523;
    parm_err = 0;
    info1523.sv1523_maxkeepsearch = orig503->sv503_maxkeepsearch;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1523,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1524;
    parm_err = 0;
    info1524.sv1524_minkeepcomplsearch = orig503->sv503_minkeepcomplsearch;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1524,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1525;
    parm_err = 0;
    info1525.sv1525_maxkeepcomplsearch = orig503->sv503_maxkeepcomplsearch;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1525,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1528;
    parm_err = 0;
    info1528.sv1528_scavtimeout = orig503->sv503_scavtimeout;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1528,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1529;
    parm_err = 0;
    info1529.sv1529_minrcvqueue = orig503->sv503_minrcvqueue;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1529,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1530;
    parm_err = 0;
    info1530.sv1530_minfreeworkitems = orig503->sv503_minfreeworkitems;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1530,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1533;
    parm_err = 0;
    info1533.sv1533_maxmpxct = orig503->sv503_maxmpxct;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1533,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1534;
    parm_err = 0;
    info1534.sv1534_oplockbreakwait = orig503->sv503_oplockbreakwait;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1534,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1535;
    parm_err = 0;
    info1535.sv1535_oplockbreakresponsewait = orig503->sv503_oplockbreakresponsewait;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1535,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1536;
    parm_err = 0;
    info1536.sv1536_enableoplocks = orig503->sv503_enableoplocks;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1536,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1537;
    parm_err = 0;
    info1537.sv1537_enableoplockforceclose = orig503->sv503_enableoplockforceclose;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1537,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1538;
    parm_err = 0;
    info1538.sv1538_enablefcbopens = orig503->sv503_enablefcbopens;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1538,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1539;
    parm_err = 0;
    info1539.sv1539_enableraw = orig503->sv503_enableraw;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1539,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1540;
    parm_err = 0;
    info1540.sv1540_enablesharednetdrives = orig503->sv503_enablesharednetdrives;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1540,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1541;
    parm_err = 0;
    info1541.sv1541_minfreeconnections = orig503->sv503_minfreeconnections;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1541,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1542;
    parm_err = 0;
    info1542.sv1542_maxfreeconnections = orig503->sv503_maxfreeconnections;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1542,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1543;
    parm_err = 0;
    info1543.sv1543_initsesstable = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1543,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1544;
    parm_err = 0;
    info1544.sv1544_initconntable = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1544,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1545;
    parm_err = 0;
    info1545.sv1545_initfiletable = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1545,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1546;
    parm_err = 0;
    info1546.sv1546_initsearchtable = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1546,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1547;
    parm_err = 0;
    info1547.sv1547_alertsched = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1547,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1548;
    parm_err = 0;
    info1548.sv1548_errorthreshold = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1548,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1549;
    parm_err = 0;
    info1549.sv1549_networkerrorthreshold = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1549,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1550;
    parm_err = 0;
    info1550.sv1550_diskspacethreshold = 0xFFFFFFFF;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1550,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1552;
    parm_err = 0;
    info1552.sv1552_maxlinkdelay = 0xFFFFFFFF;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1552,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1553;
    parm_err = 0;
    info1553.sv1553_minlinkthroughput = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1553,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_SUCCESS) netapi_fail(err);

    bufptr = &info1554;
    parm_err = 0;
    info1554.sv1554_linkinfovalidtime = 0xFFFFFFFF;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1554,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1555;
    parm_err = 0;
    info1555.sv1555_scavqosinfoupdatetime = 0xFFFFFFFF;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1555,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    bufptr = &info1556;
    parm_err = 0;
    info1556.sv1556_maxworkitemidletime = 0;
    CALL_NETAPI(err = NetServerSetInfo(srvsvc_binding,
                                       hostname,/*servername*/
                                       1556,/*level*/
                                       bufptr,/*bufptr*/
                                       &parm_err/*parm_err*/
                                       ));
    if (err != ERROR_INVALID_PARAMETER) netapi_fail(err);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

int TestNetRemoteTOD(struct test *t, const wchar16_t *hostname,
                     const wchar16_t *user, const wchar16_t *pass,
                     struct parameter *options, int optcount)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    handle_t srvsvc_binding;
    uint8 *bufptr = NULL;
//    NETRESOURCE nr = {0};

    TESTINFO(t, hostname, user, pass);

//    SET_SESSION_CREDS(nr, hostname, user, pass);

    srvsvc_binding = CreateSrvSvcBinding(&srvsvc_binding, hostname);
    if (srvsvc_binding == NULL) goto done;

    INPUT_ARG_PTR(srvsvc_binding);
    INPUT_ARG_WSTR(hostname);

    bufptr = NULL;
    CALL_NETAPI(err = NetRemoteTOD(srvsvc_binding,
                                   hostname,/*servername*/
                                   &bufptr/*bufptr*/
                                   ));
    if (err != ERROR_SUCCESS) netapi_fail(err);
    printf("bufptr[%p]\n", bufptr);

    OUTPUT_ARG_PTR(srvsvc_binding);

    FreeSrvSvcBinding(&srvsvc_binding);

//    RELEASE_SESSION_CREDS(nr);

    SrvSvcDestroyMemory();
    return true;
done:
    SrvSvcDestroyMemory();
    return false;
}

void SetupSrvSvcTests(struct test *t)
{
    SrvSvcInitMemory();

    AddTest(t, "SRVSVC-NET-CONNECTION-ENUM", TestNetConnectionEnum);
    AddTest(t, "SRVSVC-NET-FILE-ENUM", TestNetFileEnum);
    AddTest(t, "SRVSVC-NET-FILE-GET-INFO", TestNetFileGetInfo);
    AddTest(t, "SRVSVC-NET-FILE-CLOSE", TestNetFileClose);
    AddTest(t, "SRVSVC-NET-SESSION-ENUM", TestNetSessionEnum);
    AddTest(t, "SRVSVC-NET-SHARE-ADD", TestNetShareAdd);
    AddTest(t, "SRVSVC-NET-SHARE-ENUM", TestNetShareEnum);
    AddTest(t, "SRVSVC-NET-SHARE-GET-INFO", TestNetShareGetInfo);
    AddTest(t, "SRVSVC-NET-SHARE-SET-INFO", TestNetShareSetInfo);
    AddTest(t, "SRVSVC-NET-SHARE-DEL", TestNetShareDel);
    AddTest(t, "SRVSVC-NET-SERVER-GET-INFO", TestNetServerGetInfo);
    AddTest(t, "SRVSVC-NET-SERVER-SET-INFO", TestNetServerSetInfo);
    AddTest(t, "SRVSVC-NET-REMOTE-TOD", TestNetRemoteTOD);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
