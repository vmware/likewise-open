/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

/*
 * Abstract: drsuapi interface (rpc server library)
 *
 * Authors: Adam Bernstein (abernstein@vmware.com)
 */

#include "includes.h"


#if 1 /* TBD:Adam-debugging */
#include <stdio.h>
#endif

// opnum 0
ULONG
srv_IDL_DRSBind(
    /* [in] */         handle_t rpc_handle,
    /* [in, unique] */ UUID *puuidClientDsa,
    /* [in, unique] */ DRS_EXTENSIONS *pextClient,
    /* [out] */        DRS_EXTENSIONS **ppextServer,
    /* [out, ref] */   DRS_HANDLE *phDrs)
{
    ULONG ntStatus = 0;
    ULONG drsuHandleValue = 31415;
    void *phDrsRetHandle = NULL;
    DRS_EXTENSIONS *pextServer = NULL;

    
    phDrsRetHandle = calloc(1, sizeof(drsuHandleValue));
    if (!phDrsRetHandle )
    {
        ntStatus = STATUS_NO_MEMORY;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);
    memset(phDrsRetHandle, 0, sizeof(*phDrsRetHandle));

    ntStatus = DrsuapiSrvAllocateMemory(
                   (void **) &pextServer,
                   sizeof(pextServer) + 48);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);
    pextServer->cb = 48;
    memset(pextServer->rgb, 0, 48);


    *phDrs = (DRS_HANDLE) phDrsRetHandle;
    *ppextServer = pextServer;

cleanup:
    return ntStatus;

error:

    free(phDrsRetHandle);
    goto cleanup;
}
 
// opnum 1
ULONG
srv_IDL_DRSUnbind(
    /* [in] */         handle_t rpc_handle,
    /* [in, out, ref] */ DRS_HANDLE *phDrs)
{
    ULONG ntStatus = 0;


    if (phDrs && *phDrs)
    {
        free(*phDrs);
        *phDrs = NULL;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 2
ULONG
srv_IDL_DRSReplicaSync(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                            DRS_HANDLE hDrs,
    /* [in] */                            DWORD dwVersion,
    /* [in, ref, switch_is(dwVersion)] */ DRS_MSG_REPSYNC *pmsgSync)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 3
ULONG
srv_IDL_DRSGetNCChanges(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_GETCHGREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_GETCHGREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 4
ULONG
srv_IDL_DRSUpdateRefs(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                            DRS_HANDLE hDrs,
    /* [in] */                            DWORD dwVersion,
    /* [in, ref, switch_is(dwVersion)] */ DRS_MSG_UPDREFS *pmsgUpdRefs)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 5
ULONG
srv_IDL_DRSReplicaAdd(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                            DRS_HANDLE hDrs,
    /* [in] */                            DWORD dwVersion,
    /* [in, ref, switch_is(dwVersion)] */ DRS_MSG_REPADD *pmsgAdd)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 6
ULONG
srv_IDL_DRSReplicaDel(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                            DRS_HANDLE hDrs,
    /* [in] */                            DWORD dwVersion,
    /* [in, ref, switch_is(dwVersion)] */ DRS_MSG_REPDEL *pmsgDel)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 7
ULONG
srv_IDL_DRSReplicaModify(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                            DRS_HANDLE hDrs,
    /* [in] */                            DWORD dwVersion,
    /* [in, ref, switch_is(dwVersion)] */ DRS_MSG_REPMOD *pmsgMod)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 8
ULONG
srv_IDL_DRSVerifyNames(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_VERIFYREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_VERIFYREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 9
ULONG
srv_IDL_DRSGetMemberships(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_REVMEMB_REQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_REVMEMB_REPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 10
ULONG
srv_IDL_DRSInterDomainMove(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_MOVEREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_MOVEREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 11
ULONG
srv_IDL_DRSGetNT4ChangeLog(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_NT4_CHGLOG_REQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_NT4_CHGLOG_REPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
#if 0 /* TBD:Adam-Documentation */

typedef struct {
    DWORD status;
    [string,unique] WCHAR *pDomain;
    [string,unique] WCHAR *pName;
} DS_NAME_RESULT_ITEMW, *PDS_NAME_RESULT_ITEMW;


typedef struct {
    DWORD cItems;
    [size_is(cItems)] PDS_NAME_RESULT_ITEMW rItems;
} DS_NAME_RESULTW, *PDS_NAME_RESULTW;

typedef struct {
    DS_NAME_RESULTW *pResult;
} DRS_MSG_CRACKREPLY_V1;

typedef [switch_type(DWORD)] union {
    [case(1)] DRS_MSG_CRACKREPLY_V1 V1;
} DRS_MSG_CRACKREPLY;


typedef enum  { 
  DS_UNKNOWN_NAME             = 0,
  DS_FQDN_1779_NAME           = 1,
  DS_NT4_ACCOUNT_NAME         = 2,
  DS_DISPLAY_NAME             = 3,
  DS_UNIQUE_ID_NAME           = 6,
  DS_CANONICAL_NAME           = 7,
  DS_USER_PRINCIPAL_NAME      = 8,
  DS_CANONICAL_NAME_EX        = 9,
  DS_SERVICE_PRINCIPAL_NAME   = 10,
  DS_SID_OR_SID_HISTORY_NAME  = 11,
  DS_DNS_DOMAIN_NAME          = 12
} DS_NAME_FORMAT;


typedef struct  {
DS_NAME_RESULTW *pResult;
} DRS_MSG_CRACKREPLY_V1;

typedef union  {
/* case(s): 1 */
DRS_MSG_CRACKREPLY_V1 V1;
} DRS_MSG_CRACKREPLY;

#endif
static
ULONG
DRSMakeCrackNamesResult(
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_CRACKREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_CRACKREPLY *pmsgOut)
{
    ULONG ntStatus = 0;
    DWORD formatOffered = 0;
    DWORD formatDesired = 0;
    DWORD cnamesCount = 0;
    DWORD cnamesMax = 0;
    PWSTR rpNamesIn = NULL;
    PDS_NAME_RESULTW pRetNameResult = NULL;
    PDS_NAME_RESULT_ITEMW pRetNameResultArray = NULL;
    // PSTR pszRpName = NULL;
    PSTR pszRpDomain = NULL;
    PWSTR pwszRpName = NULL;
    PWSTR pwszRpDomain = NULL;
    PSTR pszPtr = NULL;

    BAIL_ON_INVALID_PTR(pmsgIn);

    /* There is only a version 1 format for DRS_MSG_CRACKREQ */
    BAIL_ON_INVALID_PARAMETER(dwInVersion == 1);

// formatOffered = 7, formatDesired = 2, 
//  DS_CANONICAL_NAME           = 7,
//  DS_NT4_ACCOUNT_NAME         = 2,

    formatOffered = pmsgIn->V1.formatOffered;
    formatDesired = pmsgIn->V1.formatDesired;
    cnamesMax = pmsgIn->V1.cNames;


    /* Allocate return array of cracked names */
    ntStatus = DrsuapiSrvAllocateMemory(
                   (void *) &pRetNameResult,
                   sizeof(DS_NAME_RESULTW) + (sizeof(DS_NAME_RESULT_ITEMW) * cnamesMax));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pRetNameResult->rItems = (PDS_NAME_RESULT_ITEMW) (((CHAR *) pRetNameResult) + sizeof(*pRetNameResult));
    pRetNameResultArray = pRetNameResult->rItems;

    for (cnamesCount = 0; cnamesCount < cnamesMax; cnamesCount++)
    {
        rpNamesIn = pmsgIn->V1.rpNames[cnamesCount];
        ntStatus = LwRtlCStringAllocateFromWC16String(
                       &pszRpDomain,
                       rpNamesIn);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        /*
         * TBD:Adam-Hard code specific conversion for testing.
         * ASSUMPTION: domain is "lightwave.local"
         * Really need to perform LDAP lookup against DSE root for
         * this information.
         */
        if (formatOffered == DS_CANONICAL_NAME && formatDesired == DS_NT4_ACCOUNT_NAME &&
            LwRtlCStringCompare(
                pszRpDomain,
                "lightwave.local/",
                FALSE) == 0)
        {
            pszPtr = strchr(pszRpDomain, '.');
            if (pszPtr)
            {
                *pszPtr = '\0';
            }

            ntStatus = DrsuapiSrvWC16StringAllocateFromCString(
                           &pwszRpDomain,
                           pszRpDomain);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

/* TBD: Bad to return NULL pointer; seeing crash */
pwszRpName = rpc_ss_allocate(4);
memset(pwszRpName, 0, 4);
            
            pRetNameResultArray[cnamesCount].status = 0; /* TBD:Adam-always return success for now */
            pRetNameResultArray[cnamesCount].pName = pwszRpDomain;
            pRetNameResultArray[cnamesCount].pDomain = pwszRpName;
        }
        else if (formatOffered == DS_NT4_ACCOUNT_NAME &&
                 formatDesired == DS_FQDN_1779_NAME &&
                 LwRtlCStringCompare(
                     pszRpDomain,
                     "lightwaveADAM-WIN2K8R2-D$",
                     FALSE) == 0)
        {
            ntStatus = DrsuapiSrvWC16StringAllocateFromCString(
                           &pwszRpName,
                           "cn=ADAM-WIN2K8R2-D,dc=lightwave,dc=local");
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

/* TBD: Bad to return NULL pointer; seeing crash */
pwszRpDomain = rpc_ss_allocate(4);
memset(pwszRpDomain, 0, 4);

            pRetNameResultArray[cnamesCount].status = 0; /* TBD:Adam-always return success for now */
            pRetNameResultArray[cnamesCount].pName = pwszRpName;
            pRetNameResultArray[cnamesCount].pDomain = pwszRpDomain;
        }
    }
    pRetNameResult->cItems = cnamesMax;

    *pdwOutVersion = dwInVersion;
    // *pmsgOut = *pRetCrackReply;
    pmsgOut->V1.pResult = pRetNameResult;

cleanup:
    return ntStatus;

error:
    if (pRetNameResult)
    {
        DrsuapiSrvFreeMemory(pRetNameResult);
    }

    goto cleanup;
}


// opnum 12
ULONG
srv_IDL_DRSCrackNames(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_CRACKREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_CRACKREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DRSMakeCrackNamesResult(
                   dwInVersion,
                   pmsgIn,
                   pdwOutVersion,
                   pmsgOut);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 13
ULONG
srv_IDL_DRSWriteSPN(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_SPNREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_SPNREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 14
ULONG
srv_IDL_DRSRemoveDsServer(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_RMSVRREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_RMSVRREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 15
ULONG
srv_IDL_DRSRemoveDsDomain(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_RMDMNREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_RMDMNREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 16
ULONG
srv_IDL_DRSDomainControllerInfo(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_DCINFOREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_DCINFOREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 17
ULONG
srv_IDL_DRSAddEntry(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_ADDENTRYREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_ADDENTRYREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 18
ULONG
srv_IDL_DRSExecuteKCC(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                              DRS_HANDLE hDrs,
    /* [in] */                              DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */ DRS_MSG_KCC_EXECUTE *pmsgIn)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 19
ULONG
srv_IDL_DRSGetReplInfo(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_GETREPLINFO_REQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_GETREPLINFO_REPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 20
ULONG
srv_IDL_DRSAddSidHistory(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_ADDSIDREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_ADDSIDREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 21
ULONG
srv_IDL_DRSGetMemberships2(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_GETMEMBERSHIPS2_REQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_GETMEMBERSHIPS2_REPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 22
ULONG
srv_IDL_DRSReplicaVerifyObjects(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                            DRS_HANDLE hDrs,
    /* [in] */                            DWORD dwVersion,
    /* [in, ref, switch_is(dwVersion)] */ DRS_MSG_REPVERIFYOBJ *pmsgVerify)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 23
ULONG
srv_IDL_DRSGetObjectExistence (
    /* [in] */         handle_t rpc_handle,
    /* [in] */                              DRS_HANDLE hDrs,
    /* [in] */                              DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */ DRS_MSG_EXISTREQ *pmsgIn,
    /* [out, ref] */                        DWORD *pdwOutVersion,
    /* [out, switch_is(*pdwOutVersion)] */  DRS_MSG_EXISTREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 24
ULONG
srv_IDL_DRSQuerySitesByCost(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_QUERYSITESREQ *pmsgIn,
    /* [out, ref] */                            DWORD *pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_QUERYSITESREPLY *pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 25
ULONG
srv_IDL_DRSInitDemotion(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_INIT_DEMOTIONREQ* pmsgIn,
    /* [out, ref] */                            DWORD* pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_INIT_DEMOTIONREPLY* pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 26
ULONG
srv_IDL_DRSReplicaDemotion(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_REPLICA_DEMOTIONREQ* pmsgIn,
    /* [out, ref] */                            DWORD* pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_REPLICA_DEMOTIONREPLY* pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 27
ULONG
srv_IDL_DRSFinishDemotion(
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_FINISH_DEMOTIONREQ* pmsgIn,
    /* [out, ref] */                            DWORD* pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_FINISH_DEMOTIONREPLY* pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
 
// opnum 28
ULONG
srv_IDL_DRSAddCloneDC (
    /* [in] */         handle_t rpc_handle,
    /* [in] */                                  DRS_HANDLE hDrs,
    /* [in] */                                  DWORD dwInVersion,
    /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_ADDCLONEDCREQ* pmsgIn,
    /* [out, ref] */                            DWORD * pdwOutVersion,
    /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_ADDCLONEDCREPLY* pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 29
ULONG
srv_IDL_DRSWriteNgcKey(
    /* [in] */         handle_t rpc_handle,
   /* [in] */                                  DRS_HANDLE hDrs,
   /* [in] */                                  DWORD dwInVersion,
   /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_WRITENGCKEYREQ* pmsgIn,
   /* [out, ref] */                            DWORD* pdwOutVersion,
   /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_WRITENGCKEYREPLY* pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}
 
// opnum 30
ULONG
srv_IDL_DRSReadNgcKey(
    /* [in] */         handle_t rpc_handle,
   /* [in] */                                  DRS_HANDLE hDrs,
   /* [in] */                                  DWORD dwInVersion,
   /* [in, ref, switch_is(dwInVersion)] */     DRS_MSG_READNGCKEYREQ* pmsgIn,
   /* [out, ref] */                            DWORD* pdwOutVersion,
   /* [out, ref, switch_is(*pdwOutVersion)] */ DRS_MSG_READNGCKEYREPLY* pmsgOut)
{
    ULONG ntStatus = 0;

    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}



void
DRS_HANDLE_rundown(
    void *hContext
    )
{
   DRS_HANDLE hDrs = NULL;

   if (hContext)
   {
       hDrs = (DRS_HANDLE) hContext;
       free(hDrs); /* TBD:Adam memory leak of context contents */
   }
}
