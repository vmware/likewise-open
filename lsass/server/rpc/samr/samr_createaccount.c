#include "includes.h"


NTSTATUS
SamrSrvCreateAccount(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UnicodeStringEx *account_name,
    /* [in  */ PSTR pszObjectClass,
    /* [in] */ uint32 account_flags,
    /* [in] */ uint32 access_mask,
    /* [out] */ ACCOUNT_HANDLE *hAccount,
    /* [out] */ uint32 *access_granted,
    /* [out] */ uint32 *rid
    )
{
    const CHAR szAttrNameObjectClass[] = "objectclass";
    const CHAR szAttrNameAliasname[] = "group-name";
    const CHAR szAttrNameUsername[] = "user-name";
    const CHAR szAttrNameAccountFlags[] = "user-info-flags";
    const wchar_t wszAccountDnFmt[] = L"CN=%ws,%ws";

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAccCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszAttrNameObjectClass = NULL;
    PWSTR pwszAttrNameAccountName = NULL;
    PWSTR pwszAttrNameAccountFlags = NULL;
    PWSTR pwszAccountName = NULL;
    PWSTR pwszDn = NULL;
    PWSTR pwszAccountDn = NULL;
    DWORD dwAccountDnLen = 0;
    DIRECTORY_MOD DirMod[4];
    ATTRIBUTE_VALUE AttrValObjectClass;
    ATTRIBUTE_VALUE AttrValAccountName;
    ATTRIBUTE_VALUE AttrValAccountFlags;
    DWORD i = 0;
    UnicodeString AccountName;
    Ids Rids, Types;
    DWORD dwRid = 0;
    DWORD dwAccountType = 0;

    memset(DirMod, 0, sizeof(DirMod));
    memset(&AttrValObjectClass, 0, sizeof(AttrValObjectClass));
    memset(&AttrValAccountName, 0, sizeof(AttrValAccountName));
    memset(&AccountName, 0, sizeof(AccountName));
    memset(&Rids, 0, sizeof(Rids));
    memset(&Types, 0, sizeof(Types));

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    dwError = LsaMbsToWc16s(szAttrNameObjectClass,
                            &pwszAttrNameObjectClass);
    BAIL_ON_LSA_ERROR(dwError);

    if (!strcmp(pszObjectClass, "user")) {
        dwError = LsaMbsToWc16s(szAttrNameUsername,
                                &pwszAttrNameAccountName);

    } else if (!strcmp(pszObjectClass, "group")) {
        dwError = LsaMbsToWc16s(szAttrNameAliasname,
                                &pwszAttrNameAccountName);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(szAttrNameAccountFlags,
                            &pwszAttrNameAccountFlags);
    BAIL_ON_LSA_ERROR(dwError);


    status = SamrSrvAllocateMemory((void**)&pAccCtx,
                                   sizeof(*pAccCtx));
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvGetFromUnicodeStringEx(&pwszAccountName,
                                           account_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    hDirectory = pDomCtx->pConnCtx->hDirectory;
    pwszDn     = pDomCtx->pwszDn;

    dwAccountDnLen = wcslen(wszAccountDnFmt) +
                     wc16slen(pwszDn) +
                     wc16slen(pwszAccountName);

    status = SamrSrvAllocateMemory((void**)pwszAccountDn,
                                   dwAccountDnLen * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(status);

    AttrValObjectClass.Type                = DIRECTORY_ATTR_TYPE_ANSI_STRING;
    AttrValObjectClass.data.pszStringValue = pszObjectClass;

    AttrValAccountName.Type              = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    AttrValAccountName.data.pwszStringValue   = pwszAccountName;

    if (!strcmp(pszObjectClass, "user")) {
        AttrValAccountFlags.Type         = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
        AttrValAccountFlags.data.ulValue = account_flags;
        AttrValAccountFlags.data.ulValue |= ACB_NORMAL | ACB_DISABLED;
    }

    DirMod[i].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    DirMod[i].pwszAttrName     = pwszAttrNameObjectClass;
    DirMod[i].ulNumValues      = 1;
    DirMod[i++].pAttrValues    = &AttrValObjectClass;

    DirMod[i].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    DirMod[i].pwszAttrName     = pwszAttrNameAccountName;
    DirMod[i].ulNumValues      = 1;
    DirMod[i++].pAttrValues    = &AttrValAccountName;

    if (!strcmp(pszObjectClass, "user")) {
        DirMod[i].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        DirMod[i].pwszAttrName     = pwszAttrNameAccountFlags;
        DirMod[i].ulNumValues      = 1;
        DirMod[i++].pAttrValues    = &AttrValAccountFlags;
    }

    DirMod[i].pwszAttrName     = NULL;
    DirMod[i].pAttrValues      = NULL;

    dwError = DirectoryAddObject(hDirectory,
                                 pwszAccountDn,
                                 DirMod);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&AccountName,
                                      pwszAccountName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvLookupNames(hBinding,
                                hDomain,
                                1,
                                &AccountName,
                                &Rids,
                                &Types);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwRid         = Rids.ids[0];
    dwAccountType = Types.ids[0];

    pAccCtx->refcount      = 1;
    pAccCtx->pwszDn        = pwszAccountDn;
    pAccCtx->pwszName      = pwszAccountName;
    pAccCtx->dwRid         = dwRid;
    pAccCtx->dwAccountType = dwAccountType;
    pAccCtx->pDomCtx       = pDomCtx;

    /* Increase ref count because DCE/RPC runtime is about to use this
       pointer as well */
    InterlockedIncrement(&pAccCtx->refcount);

    *hAccount = (ACCOUNT_HANDLE)pAccCtx;
    *rid      = dwRid;

    /* TODO: this has be changed accordingly when security descriptors
       are enabled */
    *access_granted = MAXIMUM_ALLOWED;

cleanup:
    if (pwszAttrNameObjectClass) {
        LW_SAFE_FREE_MEMORY(pwszAttrNameObjectClass);
    }

    if (pwszAttrNameAccountName) {
        LW_SAFE_FREE_MEMORY(pwszAttrNameAccountName);
    }

    SamrSrvFreeUnicodeString(&AccountName);

    if (Rids.ids) {
        SamrSrvFreeMemory(Rids.ids);
    }

    if (Types.ids) {
        SamrSrvFreeMemory(Types.ids);
    }

    return status;

error:
    if (pAccCtx) {
        SamrSrvFreeMemory(pAccCtx);
    }

    *hAccount       = NULL;
    *access_granted = 0;
    *rid            = 0;
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
