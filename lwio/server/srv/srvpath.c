#include "includes.h"

NTSTATUS
SrvBuildFilePath(
    PWSTR  pwszPrefix,
    PWSTR  pwszSuffix,
    PWSTR* ppwszFilename
    )
{
    NTSTATUS ntStatus = 0;
    size_t              len_prefix = 0;
    size_t              len_suffix = 0;
    size_t              len_separator = 0;
    PWSTR               pDataCursor = NULL;
    wchar16_t           wszFwdSlash;
    wchar16_t           wszBackSlash;
    PWSTR               pwszFilename = NULL;

    if (!pwszPrefix)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
    }
    if (!pwszSuffix)
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (!ppwszFilename)
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    wcstowc16s(&wszFwdSlash, L"/", 1);
    wcstowc16s(&wszBackSlash, L"\\", 1);

    len_prefix = wc16slen(pwszPrefix);
    len_suffix = wc16slen(pwszSuffix);

    if (len_suffix && *pwszSuffix && (*pwszSuffix != wszFwdSlash) && (*pwszSuffix != wszBackSlash))
    {
        len_separator = sizeof(wszBackSlash);
    }

    ntStatus = SMBAllocateMemory(
                    (len_prefix + len_suffix + len_separator + 1 ) * sizeof(wchar16_t),
                    (PVOID*)&pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pwszFilename;
    while (pwszPrefix && *pwszPrefix)
    {
        *pDataCursor++ = *pwszPrefix++;
    }

    if (len_separator)
    {
        *pDataCursor++ = wszBackSlash;
    }

    while (pwszSuffix && *pwszSuffix)
    {
        *pDataCursor++ = *pwszSuffix++;
    }

    pDataCursor = pwszFilename;
    while (pDataCursor && *pDataCursor)
    {
        if (*pDataCursor == wszFwdSlash)
        {
            *pDataCursor = wszBackSlash;
        }
        pDataCursor++;
    }

    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    *ppwszFilename = NULL;

    SMB_SAFE_FREE_MEMORY(pwszFilename);

    goto cleanup;
}
