#include "includes.h"

#define CN_PREFIX    "CN="
#define CN_PREFIX_L L"CN="

#define DC_PREFIX    "DC="
#define DC_PREFIX_L L"DC="

#define USERS_TOKEN    "Users"
#define USERS_TOKEN_L L"Users"

#define GROUPS_TOKEN    "Groups"
#define GROUPS_TOKEN_L L"Groups"

typedef enum
{
    SAMDB_DN_TOKEN_TYPE_UNKNOWN = 0,
    SAMDB_DN_TOKEN_TYPE_DC,
    SAMDB_DN_TOKEN_TYPE_CN

} SAMDB_DN_TOKEN_TYPE;

typedef enum
{
    SAMDB_DN_PARSE_STATE_INITIAL = 0,
    SAMDB_DN_PARSE_STATE_USER_OR_GROUP,
    SAMDB_DN_PARSE_STATE_DOMAIN

} SAMDB_DN_PARSE_STATE;

typedef struct
{
    SAMDB_DN_TOKEN_TYPE tokenType;
    PWSTR               pwszToken;
    DWORD               dwLen;
} SAMDB_DN_TOKEN, *PSAMDB_DN_TOKEN;

static
DWORD
SamDbGetDnToken(
    PWSTR           pwszObjectNameCursor,
    DWORD           dwAvailableLen,
    PSAMDB_DN_TOKEN pDnToken,
    PDWORD          pdwLenUsed
    );

DWORD
SamDbParseDN(
    PWSTR             pwszObjectDN,
    PWSTR*            ppwszObjectName,
    PWSTR*            ppwszDomain,
    PSAMDB_ENTRY_TYPE pEntryType
    )
{
    DWORD dwError = 0;
    wchar16_t wszUsersToken[sizeof(USERS_TOKEN)];
    wchar16_t wszGroupsToken[sizeof(GROUPS_TOKEN)];
    wchar16_t wszDot[2];
    PWSTR pwszObjectNameCursor = NULL;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszDomainCursor = NULL;
#if 0
    /* ifdef-ed to fix errors caused by unused variables */
    DWORD dwLenGroupsToken = sizeof(GROUPS_TOKEN) - 1;
    DWORD dwLenUsersToken = sizeof(USERS_TOKEN) - 1;
    DWORD dwDomainLen = 0;
#endif
    DWORD dwDomainOffset = 0;
    DWORD dwDomainLenAllocated = 0;
    DWORD dwDomainLenAvailable = 0;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    DWORD dwAvailableLen = 0;
    DWORD dwParseState = SAMDB_DN_PARSE_STATE_INITIAL;

    if (!pwszObjectDN || !*pwszObjectDN)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    wcstowc16s(wszUsersToken, USERS_TOKEN_L, sizeof(wszUsersToken));
    wcstowc16s(wszGroupsToken, GROUPS_TOKEN_L, sizeof(wszGroupsToken));
    wcstowc16s(wszDot, L".", 1);

    dwAvailableLen = wc16slen(pwszObjectDN);
    pwszObjectNameCursor = pwszObjectDN;

    do
    {
        SAMDB_DN_TOKEN token = {0};
        DWORD dwLenUsed;

        dwError = SamDbGetDnToken(
                        pwszObjectNameCursor,
                        dwAvailableLen,
                        &token,
                        &dwLenUsed);
        BAIL_ON_SAMDB_ERROR(dwError);

        switch(dwParseState)
        {
            case SAMDB_DN_PARSE_STATE_INITIAL:

                if (token.tokenType == SAMDB_DN_TOKEN_TYPE_CN)
                {
                    if (!wc16scmp(token.pwszToken, wszUsersToken) ||
                        !wc16scmp(token.pwszToken, wszGroupsToken))
                    {
                        dwError = LSA_ERROR_INVALID_LDAP_DN;
                        BAIL_ON_SAMDB_ERROR(dwError);
                    }

                    dwError = DirectoryAllocateMemory(
                                    (token.dwLen + 1) * sizeof(wchar16_t),
                                    (PVOID*)&pwszObjectName);
                    BAIL_ON_SAMDB_ERROR(dwError);

                    memcpy((PBYTE)pwszObjectName,
                           (PBYTE)token.pwszToken,
                           token.dwLen * sizeof(wchar16_t));

                    // Cannot tell if it is a user or group yet
                    dwParseState = SAMDB_DN_PARSE_STATE_USER_OR_GROUP;

                }
                else if (token.tokenType == SAMDB_DN_TOKEN_TYPE_DC)
                {
                    DWORD dwLenIncrement = 0;
                    DWORD dwNewLen = 0;

                    if (entryType == SAMDB_ENTRY_TYPE_UNKNOWN)
                    {
                        entryType = SAMDB_ENTRY_TYPE_DOMAIN;
                    }

                    dwLenIncrement = LSA_MAX(token.dwLen + 2, 256);
                    dwNewLen = dwDomainLenAllocated + dwLenIncrement;

                    dwError = LsaAllocateMemory(
                                    dwNewLen * sizeof(wchar16_t),
                                    (PVOID*)&pwszDomain);
                    BAIL_ON_SAMDB_ERROR(dwError);

                    pwszDomainCursor = pwszDomain;

                    dwDomainLenAllocated = dwNewLen;
                    dwDomainLenAvailable += dwLenIncrement;

                    memset((PBYTE)pwszDomainCursor, 0,
                           dwDomainLenAvailable * sizeof(wchar16_t));

                    memcpy((PBYTE)pwszDomainCursor,
                           (PBYTE)token.pwszToken,
                           token.dwLen * sizeof(wchar16_t));

                    pwszDomainCursor += token.dwLen;
                    dwDomainOffset += token.dwLen;
                    dwDomainLenAvailable -= token.dwLen;
                }
                break;

            case SAMDB_DN_PARSE_STATE_USER_OR_GROUP:

                if (token.tokenType != SAMDB_DN_TOKEN_TYPE_CN)
                {
                    dwError = LSA_ERROR_INVALID_LDAP_DN;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                if (!wc16scmp(token.pwszToken, wszUsersToken))
                {
                    entryType = SAMDB_ENTRY_TYPE_USER;
                }
                else if (!wc16scmp(token.pwszToken, wszGroupsToken))
                {
                    entryType = SAMDB_ENTRY_TYPE_GROUP;
                }
                else
                {
                    dwError = LSA_ERROR_INVALID_LDAP_DN;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                // Next token has to be a domain
                dwParseState = SAMDB_DN_PARSE_STATE_DOMAIN;

                break;

            case SAMDB_DN_PARSE_STATE_DOMAIN:

                if (token.tokenType != SAMDB_DN_TOKEN_TYPE_DC)
                {
                    dwError = LSA_ERROR_INVALID_LDAP_DN;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                if (entryType == SAMDB_ENTRY_TYPE_UNKNOWN)
                {
                    entryType = SAMDB_ENTRY_TYPE_DOMAIN;
                }

                if ((token.dwLen + 2) > dwDomainLenAvailable)
                {
                    DWORD dwLenIncrement = LSA_MAX(token.dwLen + 2, 256);
                    DWORD dwNewLen = dwDomainLenAllocated + dwLenIncrement;

                    dwError = LsaReallocMemory(
                                    pwszDomain,
                                    (PVOID*)&pwszDomain,
                                    dwNewLen * sizeof(wchar16_t));
                    BAIL_ON_SAMDB_ERROR(dwError);

                    dwDomainLenAllocated = dwNewLen;

                    pwszDomainCursor = pwszDomain + dwDomainOffset;

                    dwDomainLenAvailable += dwLenIncrement;

                    memset((PBYTE)pwszDomainCursor, 0, dwDomainLenAvailable * sizeof(wchar16_t));
                }

                if (dwDomainOffset)
                {
                    *pwszDomainCursor++ = wszDot[0];
                    dwDomainOffset++;
                    dwDomainLenAvailable--;
                }

                memcpy((PBYTE)pwszDomainCursor,
                       (PBYTE)token.pwszToken,
                       token.dwLen * sizeof(wchar16_t));

                pwszDomainCursor += token.dwLen;
                dwDomainOffset += token.dwLen;
                dwDomainLenAvailable -= token.dwLen;

                break;
        }

        pwszObjectNameCursor += dwLenUsed;
        dwAvailableLen -= dwLenUsed;

    } while (dwAvailableLen);

    if (entryType == SAMDB_ENTRY_TYPE_UNKNOWN)
    {
        dwError = LSA_ERROR_INVALID_LDAP_DN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppwszObjectName = pwszObjectName;
    *ppwszDomain = pwszDomain;
    *pEntryType = entryType;

cleanup:

    return dwError;

error:

    *ppwszObjectName = pwszObjectName;
    *ppwszDomain = pwszDomain;
    *pEntryType = entryType;

    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }

    if (pwszDomain)
    {
        DirectoryFreeMemory(pwszDomain);
    }

    goto cleanup;
}

static
DWORD
SamDbGetDnToken(
    PWSTR           pwszObjectNameCursor,
    DWORD           dwAvailableLen,
    PSAMDB_DN_TOKEN pDnToken,
    PDWORD          pdwLenUsed
    )
{
    DWORD  dwError = 0;
    wchar16_t wszCNPrefix[sizeof(CN_PREFIX)];
    DWORD dwLenCNPrefix = sizeof(CN_PREFIX) - 1;
    wchar16_t wszDCPrefix[sizeof(DC_PREFIX)];
    DWORD dwLenDCPrefix = sizeof(DC_PREFIX) - 1;
    wchar16_t wszComma[2];
    DWORD dwLenUsed = 0;
    SAMDB_DN_TOKEN token = {0};

    wcstowc16s(wszCNPrefix, CN_PREFIX_L, sizeof(wszCNPrefix));
    wcstowc16s(wszDCPrefix, DC_PREFIX_L, sizeof(wszDCPrefix));
    wcstowc16s(wszComma,    L",",  1);

    if ((dwAvailableLen > dwLenCNPrefix) &&
        !memcmp(pwszObjectNameCursor, wszCNPrefix, dwLenCNPrefix))
    {
        token.tokenType = SAMDB_DN_TOKEN_TYPE_CN;

        pwszObjectNameCursor += dwLenCNPrefix;
        dwAvailableLen -= dwLenCNPrefix;
        dwLenUsed += dwLenCNPrefix;
    }
    else
    if ((dwAvailableLen > dwLenDCPrefix) &&
        !memcmp(pwszObjectNameCursor, wszDCPrefix, dwLenDCPrefix))
    {
        token.tokenType = SAMDB_DN_TOKEN_TYPE_DC;

        pwszObjectNameCursor += dwLenDCPrefix;
        dwAvailableLen -= dwLenDCPrefix;
        dwLenUsed += dwLenDCPrefix;
    }
    else
    {
        dwError = LSA_ERROR_INVALID_LDAP_DN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!dwAvailableLen)
    {
        dwError = LSA_ERROR_INVALID_LDAP_DN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    token.pwszToken = pwszObjectNameCursor;
    while (dwAvailableLen && (*pwszObjectNameCursor++ != wszComma[0]))
    {
        dwAvailableLen--;
        token.dwLen++;
        dwLenUsed++;
    }

    if (dwAvailableLen && (*pwszObjectNameCursor) == wszComma[0])
    {
        dwLenUsed++;
    }

    *pDnToken = token;
    *pdwLenUsed = dwLenUsed;

cleanup:

    return dwError;

error:

    memset(pDnToken, 0, sizeof(*pDnToken));
    *pdwLenUsed = 0;

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
