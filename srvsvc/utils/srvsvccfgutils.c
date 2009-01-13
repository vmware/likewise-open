#include "includes.h"

DWORD
SRVSVCParseDays(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    )
{
    DWORD  dwError = 0;
    PSTR   pszTimeIntervalLocal = 0;
    DWORD  dwTimeIntervalLocalLen = 0;
    BOOLEAN  bConvert = FALSE;
    PSTR   pszUnitCode = NULL;

    SRVSVCStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);

    dwError = SRVSVCAllocateString(
                    pszTimeInterval,
                    &pszTimeIntervalLocal
                    );
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwTimeIntervalLocalLen = strlen(pszTimeIntervalLocal);

    pszUnitCode = pszTimeIntervalLocal + dwTimeIntervalLocalLen - 1;

    if (isdigit((int)(*pszUnitCode)))
    {
        bConvert = TRUE;
    }
    else if(*pszUnitCode == 'd' || *pszUnitCode == 'D')
    {
        bConvert = TRUE;
    }

    if(bConvert) {
        SRVSVCStripWhitespace(pszTimeIntervalLocal, TRUE, TRUE);
	*pdwTimeInterval = (DWORD) atoi(pszTimeIntervalLocal);
    }
    else
    {
	*pdwTimeInterval = 0;
        dwError = SRVSVC_ERROR_INVALID_PARAMETER;
    }

cleanup:

    SRVSVC_SAFE_FREE_STRING(pszTimeIntervalLocal);

    return dwError;

error:

    goto cleanup;
}

DWORD
SRVSVCParseDiskUsage(
    PCSTR  pszDiskUsage,
    PDWORD pdwDiskUsage
    )
{
    DWORD  dwError = 0;
    PSTR   pszDiskUsageLocal = 0;
    DWORD  dwLen = 0;
    DWORD  dwUnitMultiplier = 0;
    PSTR   pszUnitCode = NULL;

    SRVSVCStripWhitespace(pszDiskUsageLocal, TRUE, TRUE);

    dwError = SRVSVCAllocateString(
                    pszDiskUsage,
                    &pszDiskUsageLocal
                    );
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwLen = strlen(pszDiskUsageLocal);

    pszUnitCode = pszDiskUsageLocal + dwLen - 1;

    if (isdigit((int)(*pszUnitCode)))
    {
        dwUnitMultiplier = 1;
    }
    else if(*pszUnitCode == 'k' || *pszUnitCode == 'K')
    {
        dwUnitMultiplier = SRVSVC_BYTES_IN_KB;
    }
    else if(*pszUnitCode == 'm' || *pszUnitCode == 'M')
    {
        dwUnitMultiplier = SRVSVC_BYTES_IN_MB;
    }
    else if(*pszUnitCode == 'g' || *pszUnitCode == 'G')
    {
        dwUnitMultiplier = SRVSVC_BYTES_IN_GB;
    }
    else
    {
        dwError = SRVSVC_ERROR_INVALID_PARAMETER;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    *pszUnitCode = ' ';

    SRVSVCStripWhitespace(pszDiskUsageLocal, TRUE, TRUE);

    *pdwDiskUsage = (DWORD) atoi(pszDiskUsageLocal) * dwUnitMultiplier;

cleanup:

    SRVSVC_SAFE_FREE_STRING(pszDiskUsageLocal);

    return dwError;

error:

    goto cleanup;
}

DWORD
SRVSVCParseMaxEntries(
    PCSTR  pszMaxEntries,
    PDWORD pdwMaxEntries
    )
{
    DWORD  dwError = 0;
    PSTR   pszMaxEntriesLocal = 0;
    DWORD  dwLen = 0;
    DWORD  dwUnitMultiplier = 0;
    PSTR   pszUnitCode = NULL;

    SRVSVCStripWhitespace(pszMaxEntriesLocal, TRUE, TRUE);

    dwError = SRVSVCAllocateString(
                    pszMaxEntries,
                    &pszMaxEntriesLocal
                    );
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwLen = strlen(pszMaxEntriesLocal);

    pszUnitCode = pszMaxEntriesLocal + dwLen - 1;

    if (isdigit((int)(*pszUnitCode)))
    {
        dwUnitMultiplier = 1;
    }
    else if(*pszUnitCode == 'k' || *pszUnitCode == 'K')
    {
        dwUnitMultiplier = SRVSVC_RECS_IN_K;
    }
    else if(*pszUnitCode == 'm' || *pszUnitCode == 'M')
    {
        dwUnitMultiplier = SRVSVC_RECS_IN_M;
    }
    else
    {
        dwError = SRVSVC_ERROR_INVALID_PARAMETER;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    *pszUnitCode = ' ';

    SRVSVCStripWhitespace(pszMaxEntriesLocal, TRUE, TRUE);

    *pdwMaxEntries = (DWORD) atoi(pszMaxEntriesLocal) * dwUnitMultiplier;

cleanup:

    SRVSVC_SAFE_FREE_STRING(pszMaxEntriesLocal);

    return dwError;

error:

    goto cleanup;
}
