#include "config.h"
#include "lsmbsys.h"

#include <krb5.h>
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>

#include <lsmb/lsmb.h>

#include "smbdef.h"
#include "smbutils.h"
#include "smbkrb5.h"

#include "smbwire.h"
#include "smbtransport.h"

#include "smbclient.h"

#define SMB_CONFIG_FILE_PATH CONFIGDIR "/lsmbd.conf"

static
DWORD
GetHostAndShareNames(
    PCSTR pszShareName,
    PSTR* ppszShareName,
    PSTR* ppszHostname
    );

uint32_t
SMBSrvClientTreeOpen(
    PCSTR pszHostname,
    PCSTR pszPrincipal,
    PCSTR pszSharename,
    PSMB_TREE* ppTree
    );

int
main(
    int   argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    SMB_TREE *pTree = NULL;
    PSTR pszHostname = NULL;
    PSTR pszSharename = NULL;
    BOOLEAN bCleanupSMBCore = FALSE;

    if (argc < 2)
    {
        printf("Usage: tcon <share>\n");
        exit(1);
    }

    dwError = GetHostAndShareNames(
                    argv[1],
                    &pszSharename,
                    &pszHostname);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientInit(SMB_CONFIG_FILE_PATH);
    BAIL_ON_SMB_ERROR(dwError);

    bCleanupSMBCore = TRUE;

    dwError = SMBSrvClientTreeOpen(
                    pszHostname,
                    "Administrator@KAYA-2K.CORP.CENTERIS.COM",
                    pszSharename,
                    &pTree);
    BAIL_ON_SMB_ERROR(dwError);

    SMBTreeRelease(pTree);
    pTree = NULL;

    getchar();

cleanup:

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    if (bCleanupSMBCore)
    {
        SMBSrvClientShutdown();
    }

    SMB_SAFE_FREE_STRING(pszHostname);
    SMB_SAFE_FREE_STRING(pszSharename);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
GetHostAndShareNames(
    PCSTR pszShareName,
    PSTR* ppszShareName,
    PSTR* ppszHostname
    )
{
    DWORD dwError = 0;
    PCSTR pszInput = pszShareName;
    size_t sLength = 0;
    PSTR pszHostname = NULL;
    PSTR pszShareNameLocal = NULL;

    // Optional
    sLength = strspn(pszInput, "\\");
    if (sLength)
    {
        pszInput += sLength;
    }

    // Hostname
    sLength = strcspn(pszInput, "\\");
    if (!sLength)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBStrndup(
                pszInput,
                (DWORD)sLength,
                &pszHostname);
    BAIL_ON_SMB_ERROR(dwError);

    pszInput += sLength;

    // Mandatory before share
    sLength = strspn(pszInput, "\\");
    if (!sLength)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pszInput += sLength;

    // Share
    if (IsNullOrEmptyString(pszInput))
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBAllocateString(
                pszShareName,
                &pszShareNameLocal);
    BAIL_ON_SMB_ERROR(dwError);

    *ppszHostname = pszHostname;
    *ppszShareName = pszShareNameLocal;

cleanup:

    return dwError;

error:

    SMB_SAFE_FREE_STRING(pszHostname);
    SMB_SAFE_FREE_STRING(pszShareNameLocal);

    goto cleanup;
}
