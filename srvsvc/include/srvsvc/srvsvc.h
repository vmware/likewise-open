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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvsvc.h
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        Client API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SRVSVC_H_
#define _SRVSVC_H_

#include <lwrpc/types.h>
#include <srvsvc/srvsvcbinding.h>
#include <srvsvc/srvsvcdefs.h>
#include <srvsvc/wkssvcdefs.h>
#include <srvsvc/winregdefs.h>


/*
 * Error codes
 */
#define SRVSVC_ERROR_SUCCESS                   0x0000
#define SRVSVC_ERROR_INVALID_CONFIG_PATH       0x9400 // 37888
#define SRVSVC_ERROR_INVALID_PREFIX_PATH       0x9401 // 37889
#define SRVSVC_ERROR_INSUFFICIENT_BUFFER       0x9402 // 37890
#define SRVSVC_ERROR_OUT_OF_MEMORY             0x9403 // 37891
#define SRVSVC_ERROR_INVALID_MESSAGE           0x9404 // 37892
#define SRVSVC_ERROR_UNEXPECTED_MESSAGE        0x9405 // 37893
#define SRVSVC_ERROR_NO_SUCH_USER              0x9406 // 37894
#define SRVSVC_ERROR_DATA_ERROR                0x9407 // 37895
#define SRVSVC_ERROR_NOT_IMPLEMENTED           0x9408 // 37896
#define SRVSVC_ERROR_NO_CONTEXT_ITEM           0x9409 // 37897
#define SRVSVC_ERROR_NO_SUCH_GROUP             0x940A // 37898
#define SRVSVC_ERROR_REGEX_COMPILE_FAILED      0x940B // 37899
#define SRVSVC_ERROR_NSS_EDIT_FAILED           0x940C // 37900
#define SRVSVC_ERROR_NO_HANDLER                0x940D // 37901
#define SRVSVC_ERROR_INTERNAL                  0x940E // 37902
#define SRVSVC_ERROR_NOT_HANDLED               0x940F // 37903
#define SRVSVC_ERROR_UNEXPECTED_DB_RESULT      0x9410 // 37904
#define SRVSVC_ERROR_INVALID_PARAMETER         0x9411 // 37905
#define SRVSVC_ERROR_LOAD_LIBRARY_FAILED       0x9412 // 37906
#define SRVSVC_ERROR_LOOKUP_SYMBOL_FAILED      0x9413 // 37907
#define SRVSVC_ERROR_INVALID_EVENTLOG          0x9414 // 37908
#define SRVSVC_ERROR_INVALID_CONFIG            0x9415 // 37909
#define SRVSVC_ERROR_STRING_CONV_FAILED        0x9416 // 37910
#define SRVSVC_ERROR_INVALID_DB_HANDLE         0x9417 // 37911
#define SRVSVC_ERROR_FAILED_CONVERT_TIME       0x9418 // 37912
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING 0x9419 // 37913
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_OPEN   0x941A // 37914
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_CLOSE  0x941B // 37915
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_COUNT  0x941C // 37916
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_READ   0x941D // 37917
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_WRITE  0x941E // 37918
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_CLEAR  0x941F // 37919
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_DELETE 0x9420 // 37920
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_REGISTER 0x9421 // 37921
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_UNREGISTER 0x9422 // 37922
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_LISTEN 0x9423 // 37923
#define SRVSVC_ERROR_RPC_EXCEPTION             0x9424 // 37924
#define SRVSVC_ERROR_ACCESS_DENIED             0x9425 // 37925
#define SRVSVC_ERROR_SENTINEL                  0x9426 // 37926

#ifndef NET_API_STATUS_DEFINED
typedef WINERR NET_API_STATUS;

#define NET_API_STATUS_DEFINED
#endif


NET_API_STATUS
NetConnectionEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *qualifier,
    UINT32 level,
    UINT8 **bufptr,
    UINT32 prefmaxlen,
    UINT32 *entriesread,
    UINT32 *totalentries,
    UINT32 *resume_handle
    );


NET_API_STATUS
NetFileEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *basepath,
    const wchar16_t *username,
    UINT32 level,
    UINT8 **bufptr,
    UINT32 prefmaxlen,
    UINT32 *entriesread,
    UINT32 *totalentries,
    UINT32 *resume_handle
    );


NET_API_STATUS
NetFileGetInfo(
    handle_t b,
    const wchar16_t *servername,
    UINT32 fileid,
    UINT32 level,
    UINT8 **bufptr
    );


NET_API_STATUS
NetFileClose(
    handle_t b,
    const wchar16_t *servername,
    UINT32 fileid
    );


NET_API_STATUS
NetSessionEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *unc_client_name,
    const wchar16_t *username,
    UINT32 level,
    UINT8 **bufptr,
    UINT32 prefmaxlen,
    UINT32 *entriesread,
    UINT32 *totalentries,
    UINT32 *resume_handle
    );


NET_API_STATUS
NetrShareAdd(
    IN  handle_t hBinding,
    IN  PCWSTR   pwszServername,
    IN  DWORD    dwLevel,
    IN  PVOID    pBuffer,
    OUT PDWORD   pdwParmErr
    );


NET_API_STATUS
NetShareAdd(
    IN  PCWSTR  pwszServername,
    IN  DWORD   dwLevel,
    IN  PVOID   pBuffer,
    OUT PDWORD  pdwParmErr
    );


NET_API_STATUS
NetrShareEnum(
    IN  handle_t hBinding,
    IN  PCWSTR   pwszServername,
    IN  DWORD    dwLevel,
    OUT PVOID   *ppBuffer,
    IN  DWORD    dwMaxLen,
    OUT PDWORD   pdwNumEntries,
    OUT PDWORD   pdwTotalEntries,
    OUT PDWORD   pdwResume
    );


NET_API_STATUS
NetShareEnum(
    IN  PCWSTR   pwszServername,
    IN  DWORD    dwLevel,
    OUT PVOID   *ppBuffer,
    IN  DWORD    dwMaxLen,
    OUT PDWORD   pdwNumEntries,
    OUT PDWORD   pdwTotalEntries,
    OUT PDWORD   pdwResume
    );


NET_API_STATUS
NetrShareGetInfo(
    IN  handle_t  hBinding,
    IN  PWSTR     pwszServername,
    IN  PWSTR     pwszNetname,
    IN  DWORD     dwLevel,
    OUT PVOID    *ppBuffer
    );


NET_API_STATUS
NetShareGetInfo(
    IN  PCWSTR    pwszServername,
    IN  PCWSTR    pwszNetname,
    IN  DWORD     dwLevel,
    OUT PVOID    *ppBuffer
    );


NET_API_STATUS
NetrShareSetInfo(
    IN  handle_t  hBinding,
    IN  PWSTR     pwszServername,
    IN  PWSTR     pwszNetname,
    IN  DWORD     dwLevel,
    IN  PVOID     pBuffer,
    OUT PDWORD    pdwParmErr
    );


NET_API_STATUS
NetShareSetInfo(
    IN  PCWSTR    pwszServername,
    IN  PCWSTR    pwszNetname,
    IN  DWORD     dwLevel,
    IN  PVOID     pBuffer,
    OUT PDWORD    pdwParmErr
    );


NET_API_STATUS
NetrShareDel(
    IN  handle_t hBinding,
    IN  PCWSTR   pwszServername,
    IN  PCWSTR   pwszSharename,
    IN  DWORD    dwReserved
    );


NET_API_STATUS
NetShareDel(
    IN  PCWSTR  pwszServername,
    IN  PCWSTR  pwszSharename,
    IN  DWORD   dwReserved
    );


NET_API_STATUS
NetServerGetInfo(
    handle_t b,
    const wchar16_t *servername,
    UINT32 level,
    UINT8 **bufptr
    );


NET_API_STATUS
NetServerSetInfo(
    handle_t b,
    const wchar16_t *servername,
    UINT32 level,
    UINT8 *bufptr,
    UINT32 *parm_err
    );


NET_API_STATUS
NetRemoteTOD(
    handle_t b,
    const wchar16_t *servername,
    UINT8 **bufptr
    );


NET_API_STATUS
SrvSvcInitMemory(
    void
    );


NET_API_STATUS
SrvSvcDestroyMemory(
    void
    );


NET_API_STATUS
SrvSvcFreeMemory(
    void *ptr
    );


#endif /* _SRVSVC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
