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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsaerror.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */

#include "includes.h"

static
const char* gLsaErrorMessages[] =
{
    // LSA_ERROR_INVALID_CACHE_PATH                              : 32768
    "An invalid cache path was specified",
    // LSA_ERROR_INVALID_CONFIG_PATH                             : 32769
    "The path to the configuration file is invalid",
    // LSA_ERROR_INVALID_PREFIX_PATH                             : 32770
    "The product installation folder could not be determined",
    // LSA_ERROR_INSUFFICIENT_BUFFER                             : 32771
    "The provided buffer is insufficient",
    // LSA_ERROR_OUT_OF_MEMORY                                   : 32772
    "Out of memory",
    // LSA_ERROR_INVALID_MESSAGE                                 : 32773
    "The Inter Process message is invalid",
    // LSA_ERROR_UNEXPECTED_MESSAGE                              : 32774
    "An unexpected Inter Process message was received",
    // LSA_ERROR_NO_SUCH_USER                                    : 32775
    "No such user",
    // LSA_ERROR_DATA_ERROR                                      : 32776
    "The cached data is incorrect",
    // LSA_ERROR_NOT_IMPLEMENTED                                 : 32777
    "The requested feature has not been implemented yet",
    // LSA_ERROR_NO_CONTEXT_ITEM                                 : 32778
    "The requested item was not found in context",
    // LSA_ERROR_NO_SUCH_GROUP                                   : 32779
    "No such group",
    // LSA_ERROR_REGEX_COMPILE_FAILED                            : 32780
    "Failed to compile regular expression",
    // LSA_ERROR_NSS_EDIT_FAILED                                 : 32781
    "Failed to edit nsswitch configuration",
    // LSA_ERROR_NO_HANDLER                                      : 32782
    "No authentication Provider was found",
    // LSA_ERROR_INTERNAL                                        : 32783
    "Internal error",
    // LSA_ERROR_NOT_HANDLED                                     : 32784
    "The authentication request could not be handled",
    // LSA_ERROR_INVALID_DNS_RESPONSE                            : 32785
    "Response from DNS is invalid",
    // LSA_ERROR_DNS_RESOLUTION_FAILED                           : 32786
    "Failed to resolve query using DNS",
    // LSA_ERROR_FAILED_TIME_CONVERSION                          : 32787
    "Failed to convert the time",
    // LSA_ERROR_INVALID_SID                                     : 32788
    "The security descriptor (SID) is invalid",
    // LSA_ERROR_PASSWORD_MISMATCH                               : 32789
    "The password is incorrect for the given username",
    // LSA_ERROR_UNEXPECTED_DB_RESULT                            : 32790
    "Unexpected cached data found",
    // LSA_ERROR_PASSWORD_EXPIRED                                : 32791
    "Password expired",
    // LSA_ERROR_ACCOUNT_EXPIRED                                 : 32792
    "Account expired",
    // LSA_ERROR_USER_EXISTS                                     : 32793
    "A user by this name already exists",
    // LSA_ERROR_GROUP_EXISTS                                    : 32794
    "A group by this name already exists",
    // LSA_ERROR_INVALID_GROUP_INFO_LEVEL                        : 32795
    "An invalid group info level was specified",
    // LSA_ERROR_INVALID_USER_INFO_LEVEL                         : 32796
    "An invalid user info level was specified",
    // LSA_ERROR_UNSUPPORTED_USER_LEVEL                          : 32797
    "This interface does not support the user info level specified",
    // LSA_ERROR_UNSUPPORTED_GROUP_LEVEL                         : 32798
    "This interface does not support the group info level specified",
    // LSA_ERROR_INVALID_LOGIN_ID                                : 32799
    "The login id is invalid",
    // LSA_ERROR_INVALID_HOMEDIR                                 : 32800
    "The home directory is invalid",
    // LSA_ERROR_INVALID_GROUP_NAME                              : 32801
    "The group name is invalid",
    // LSA_ERROR_NO_MORE_GROUPS                                  : 32802
    "No more groups",
    // LSA_ERROR_NO_MORE_USERS                                   : 32803
    "No more users",
    // LSA_ERROR_FAILED_ADD_USER                                 : 32804
    "Failed to add user",
    // LSA_ERROR_FAILED_ADD_GROUP                                : 32805
    "Failed to add group",
    // LSA_ERROR_INVALID_LSA_CONNECTION                          : 32806
    "The connection to the authentication service is invalid",
    // LSA_ERROR_INVALID_AUTH_PROVIDER                           : 32807
    "The authentication provider is invalid",
    // LSA_ERROR_INVALID_PARAMETER                               : 32808
    "Invalid parameter",
    // LSA_ERROR_LDAP_NO_PARENT_DN                               : 32809
    "No distinguished name found in LDAP for parent of this object",
    // LSA_ERROR_LDAP_ERROR                                      : 32810
    "An Error was encountered when negotiating with LDAP",
    // LSA_ERROR_NO_SUCH_DOMAIN                                  : 32811
    "Unknown Active Directory domain",
    // LSA_ERROR_LDAP_FAILED_GETDN                               : 32812
    "Failed to find distinguished name using LDAP",
    // LSA_ERROR_DUPLICATE_DOMAINNAME                            : 32813
    "A duplicate Active Directory domain name was found",
    // LSA_ERROR_KRB5_CALL_FAILED                                : 32814
    "The call to Kerberos 5 failed",
    // LSA_ERROR_GSS_CALL_FAILED                                 : 32815
    "The GSS call failed",
    // LSA_ERROR_FAILED_FIND_DC                                  : 32816
    "Failed to find the domain controller",
    // LSA_ERROR_NO_SUCH_CELL                                    : 32817
    "Failed to find the Cell in Active Direcotry",
    // LSA_ERROR_GROUP_IN_USE                                    : 32818
    "The specified group is currently being used",
    // LSA_ERROR_FAILED_CREATE_HOMEDIR                           : 32819
    "Failed to create home directory for user",
    // LSA_ERROR_PASSWORD_TOO_WEAK                               : 32820
    "The specified password is not strong enough",
    // LSA_ERROR_INVALID_SID_REVISION                            : 32821
    "The security descriptor (SID) has an invalid revision",
    // LSA_ERROR_ACCOUNT_LOCKED                                  : 32822
    "The user account is locked",
    // LSA_ERROR_ACCOUNT_DISABLED                                : 32823
    "The user account is disabled",
    // LSA_ERROR_USER_CANNOT_CHANGE_PASSWD                       : 32824
    "The user is not allowed to change his/her password",
    // LSA_ERROR_LOAD_LIBRARY_FAILED                             : 32825
    "Failed to dynamically load a library",
    // LSA_ERROR_LOOKUP_SYMBOL_FAILED                            : 32826
    "Failed to lookup a symbol in a dynamic library",
    // LSA_ERROR_INVALID_EVENTLOG                                : 32827
    "The Eventlog interface is invalid",
    // LSA_ERROR_INVALID_CONFIG                                  : 32828
    "The specified configuration (file) is invalid",
    // LSA_ERROR_UNEXPECTED_TOKEN                                : 32829
    "An unexpected token was encountered in the configuration",
    // LSA_ERROR_LDAP_NO_RECORDS_FOUND                           : 32830
    "No records were found in the cache",
    // LSA_ERROR_DUPLICATE_USERNAME                              : 32831
    "A duplicate user record was found",
    // LSA_ERROR_DUPLICATE_GROUPNAME                             : 32832
    "A duplicate group record was found",
    // LSA_ERROR_DUPLICATE_CELLNAME                              : 32833
    "A duplicate cell was found",
    // LSA_ERROR_STRING_CONV_FAILED                              : 32834
    "Failed to convert string format (wide/ansi)",
    // LSA_ERROR_INVALID_ACCOUNT                                 : 32835
    "The user account is invalid",
    // LSA_ERROR_INVALID_PASSWORD                                : 32836
    "The password is invalid",
    // LSA_ERROR_QUERY_CREATION_FAILED                           : 32837
    "Failed to create query to examine cache",
    // LSA_ERROR_NO_SUCH_USER_OR_GROUP                           : 32838
    "No such user or group",
    // LSA_ERROR_DUPLICATE_USER_OR_GROUP                         : 32839
    "A duplicate user or group was found",
    // LSA_ERROR_INVALID_KRB5_CACHE_TYPE                         : 32840
    "An invalid kerberos 5 cache type was specified",
    // LSA_ERROR_NOT_JOINED_TO_AD                                : 32841
    "This machine is not currently joined to Active Directory",
    // LSA_ERROR_FAILED_TO_SET_TIME                              : 32842
    "The system time could not be set",
    // LSA_ERROR_NO_NETBIOS_NAME                                 : 32843
    "Failed to find NetBIOS name for the domain",
    // LSA_ERROR_INVALID_NETLOGON_RESPONSE                       : 32844
    "The Netlogon response buffer is invalid",
    // LSA_ERROR_INVALID_OBJECTGUID                              : 32845
    "The specified Globally Unique Identifier (GUID) is invalid",
    // LSA_ERROR_INVALID_DOMAIN                                  : 32846
    "The domain name is invalid",
    // LSA_ERROR_NO_DEFAULT_REALM                                : 32847
    "The kerberos default realm is not set",
    //  LSA_ERROR_NOT_SUPPORTED                                  : 32848
    "The request is not supported",
    // LSA_ERROR_LOGON_FAILURE                                   : 32849
    "The logon request failed",
    // LSA_ERROR_NO_SITE_INFORMATION                             : 32850
    "No site information was found for the active directory domain",
    // LSA_ERROR_INVALID_LDAP_STRING                             : 32851
    "The LDAP string value is invalid",
    // LSA_ERROR_INVALID_LDAP_ATTR_VALUE                         : 32852
    "The LDAP attribute value is NULL or invalid",
    // LSA_ERROR_NULL_BUFFER                                     : 32853
    "An invalid buffer was provided",
    // LSA_ERROR_CLOCK_SKEW                                      : 32854
    "Clock skew detected with active directory server",
    // LSA_ERROR_KRB5_NO_KEYS_FOUND                              : 32855
    "No kerberos keys found",
    // LSA_ERROR_SERVICE_NOT_AVAILABLE                           : 32856
    "The authentication service is not available",
    // LSA_ERROR_INVALID_SERVICE_RESPONSE                        : 32857
    "Invalid response from the authentication service",
    // LSA_ERROR_NSS_ERROR                                       : 32858
    "Name Service Switch Error",
    // LSA_ERROR_AUTH_ERROR                                      : 32859
    "Authentication Error",
    // LSA_ERROR_INVALID_LDAP_DN                                 : 32860
    "Invalid Ldap distinguished name (DN)",
    // LSA_ERROR_NOT_MAPPED                                      : 32861
    "Account Name or SID not mapped",
    // LSA_ERROR_RPC_NETLOGON_FAILED                             : 32862
    "Calling librpc/NetLogon API failed",
    // LSA_ERROR_ENUM_DOMAIN_TRUSTS_FAILED                       : 32863
    "Enumerating domain trusts failed",
    // LSA_ERROR_RPC_LSABINDING_FAILED                           : 32864
    "Calling librpc/Lsa Binding failed",
    // LSA_ERROR_RPC_OPENPOLICY_FAILED                           : 32865
    "Calling librpc/Lsa Open Policy failed",
    // LSA_ERROR_RPC_LSA_LOOKUPNAME2_FAILED                      : 32866
    "Calling librpc/Lsa LookupName2 failed",
    // LSA_ERROR_RPC_SET_SESS_CREDS_FAILED                       : 32867
    "RPC Set Session Creds failed",
    // LSA_ERROR_RPC_REL_SESS_CREDS_FAILED                       : 32868
    "RPC Release Session Creds failed",
    // LSA_ERROR_RPC_CLOSEPOLICY_FAILED                          : 32869
    "Calling librpc/Lsa Close Handle/policy failed",
    // LSA_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND                   : 32870
    "No user/group was found using RPC lookup name to objectSid",
    // LSA_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES            : 32871
    "Abnormal duplicates were found using RPC lookup name to objectSid",
    // LSA_ERROR_NO_TRUSTED_DOMAIN_FOUND                         : 32872
    "No such trusted domain found",
   //  LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS       : 32873
    "Trusted domain is executing in an incompatiable modes",
    // LSA_ERROR_DCE_CALL_FAILED                                 : 32874
    "A call to DCE/RPC failed",
    // LSA_ERROR_FAILED_TO_LOOKUP_DC                             : 32875
    "Failed to lookup the domain controller for given domain",
    // LSA_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL                 : 32876
    "An invalid nss artefact info level was specified",
    // LSA_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL                  : 32877
    "The NSS artefact info level specified is not supported",
    // LSA_ERROR_INVALID_USER_NAME                               : 32878
    "An invalid user name was specified",
    // LSA_ERROR_INVALID_LOG_LEVEL                               : 32879
    "An invalid log level was specified",
    // LSA_ERROR_INVALID_METRIC_TYPE                             : 32880
    "An invalid metric type was specified",
    // LSA_ERROR_INVALID_METRIC_PACK                             : 32881
    "An invalid metric pack was specified",
    // LSA_ERROR_INVALID_METRIC_INFO_LEVEL                       : 32882
    "An invalid info level was specified when querying metrics",
    // LSA_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK               : 32883
    "The system hostname configuration is incorrect",
    // LSA_ERROR_MAC_FLUSH_DS_CACHE_FAILED                       : 32884
    "Could not find DirectoryService cache utility to call -flushcache with",
    // LSA_ERROR_LSA_SERVER_UNREACHABLE                          : 32885
    "The LSASS server is not responding.",
    // LSA_ERROR_INVALID_NSS_ARTEFACT_TYPE                       : 32886
    "An invalid NSS Artefact type was specified",
    // LSA_ERROR_INVALID_AGENT_VERSION                           : 32887
    "The LSASS Server version is invalid",
    // LSA_ERROR_DOMAIN_IS_OFFLINE                               : 32888
    "The domain is offline",
    // LSA_ERROR_INVALID_HOMEDIR_TEMPLATE                        : 32889
    "An invalid home directory template was specified",
    // LSA_ERROR_RPC_PARSE_SID_STRING                            : 32890
    "Failed to use NetAPI to parse SID string",
    // LSA_ERROR_RPC_LSA_LOOKUPSIDS_FAILED                       : 32891
    "Failed to use NetAPI to lookup names for SIDs",
    // LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND                    : 32892
    "Failed to find any names for SIDs using NetAPI",
    // LSA_ERROR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES             : 32893
    "Found duplicates using RPC Call to lookup SIDs",
    // LSA_ERROR_PASSWORD_RESTRICTION                            : 32894
    "Password does not meet requirements",
    // LSA_ERROR_OBJECT_NOT_ENABLED                              : 32895
    "The user/group is not enabled in the cell",
    // LSA_ERROR_NO_MORE_NSS_ARTEFACTS                           : 32896
    "No more NSS Artefacts",
    // LSA_ERROR_INVALID_NSS_MAP_NAME                            : 32897
    "An invalid name was specified for the NIS map",
    // LSA_ERROR_INVALID_NSS_KEY_NAME                            : 32898
    "An invalid name was specified for the NIS key",
    // LSA_ERROR_NO_SUCH_NSS_KEY                                 : 32899
    "No such NIS Key is defined",
    // LSA_ERROR_NO_SUCH_NSS_MAP                                 : 32900
    "No such NIS Map is defined",
    // LSA_ERROR_RPC_ERROR                                       : 32901
    "An Error was encountered when negotiating with RPC",
    // LSA_ERROR_LDAP_SERVER_UNAVAILABLE                         : 32902
    "The LDAP server is unavailable",
    // LSA_ERROR_CREATE_KEY_FAILED                               : 32903
    "Could not create random key",
    // LSA_ERROR_CANNOT_DETECT_USER_PROCESSES                    : 32904
    "Cannot detect user processes",
    // LSA_ERROR_TRACE_NOT_INITIALIZED                           : 32905
    "Tracing has not been initialized",
    // LSA_ERROR_NO_SUCH_TRACE_FLAG                              : 32906
    "The trace flag is not defined"
};

size_t
LsaGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;

    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }

    if (!dwError)
    {
        // No error string for success
        goto cleanup;
    }

    if (LSA_ERROR_MASK(dwError) != 0)
    {
        stResult = LsaMapLsaErrorToString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    else
    {
        stResult = LsaGetSystemErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }

cleanup:

    return stResult;
}

size_t
LsaMapLsaErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    DWORD dwNMessages = sizeof(gLsaErrorMessages)/sizeof(PCSTR);

    if ((dwError >= LSA_ERROR_INVALID_CACHE_PATH) &&
        (dwError < LSA_ERROR_SENTINEL))
    {
        DWORD dwErrorOffset = dwError - 0x8000;

        if (dwErrorOffset < dwNMessages)
        {
            PCSTR pszMessage = gLsaErrorMessages[dwErrorOffset];
            DWORD dwRequiredLen = strlen(pszMessage) + 1;

            if (stBufSize >= dwRequiredLen) {
                memcpy(pszBuffer, pszMessage, dwRequiredLen);
            }

            stResult = dwRequiredLen;

            goto cleanup;
        }
    }

    stResult = LsaGetUnmappedErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);

cleanup:

    return stResult;
}

size_t
LsaGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    DWORD  dwError = LSA_ERROR_SUCCESS;
    size_t stResult = 0;
    PSTR   pszTempBuffer = NULL;

    int result = LsaStrError(dwConvertError, pszBuffer, stBufSize);
    if (result == EINVAL)
    {
        stResult = LsaGetUnmappedErrorString(
                        dwConvertError,
                        pszBuffer,
                        stBufSize);
        goto cleanup;
    }

    while (result != 0)
    {
        if (result == ERANGE)
        {
            // Guess
            stBufSize = stBufSize * 2 + 10;
        }
        else
        {
            stResult = LsaGetUnmappedErrorString(
                            dwConvertError,
                            pszBuffer,
                            stBufSize);
            goto cleanup;
        }
        LSA_SAFE_FREE_MEMORY(pszTempBuffer);

        dwError = LsaAllocateMemory(
                        stBufSize,
                        (PVOID*)&pszTempBuffer);
        BAIL_ON_LSA_ERROR(dwError);

        result = LsaStrError(dwConvertError, pszTempBuffer, stBufSize);
    }

    if (pszTempBuffer != NULL)
    {
        stResult = strlen(pszTempBuffer) + 1;
    }
    else
    {
        stResult = strlen(pszBuffer) + 1;
    }

cleanup:

    LSA_SAFE_FREE_MEMORY(pszTempBuffer);

    return stResult;

error:

    stResult = 0;

    goto cleanup;
}

size_t
LsaGetUnmappedErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    CHAR  szBuf[128] = "";
    DWORD dwRequiredLen = 0;

    dwRequiredLen = sprintf(szBuf, "Error [code=%d] occurred.", dwError) + 1;

    if (stBufSize >= dwRequiredLen) {
        memcpy(pszBuffer, szBuf, dwRequiredLen);
    }

    stResult = dwRequiredLen;

    return stResult;
}

DWORD
LsaGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg)
{
    DWORD dwErrorBufferSize = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszErrorMsg = NULL;
    PSTR  pszErrorBuffer = NULL;

    dwErrorBufferSize = LsaGetErrorString(dwErrCode, NULL, 0);

    if (!dwErrorBufferSize)
        goto cleanup;

    dwError = LsaAllocateMemory(
                dwErrorBufferSize,
                (PVOID*)&pszErrorBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    dwLen = LsaGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);

    if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
    {
        dwError = LsaAllocateStringPrintf(
                     &pszErrorMsg,
                     "Error: %s [error code: %d]",
                     pszErrorBuffer,
                     dwErrCode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:

    LSA_SAFE_FREE_STRING(pszErrorBuffer);

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}

DWORD
LsaTranslateLwMsgError(
        LWMsgAssoc *pAssoc,
        DWORD dwMsgError,
        const char *file,
        int line
        )
{
    DWORD dwLsaError = (DWORD)-1;

    switch (dwMsgError)
    {
        case LWMSG_STATUS_SUCCESS:
            return LSA_ERROR_SUCCESS;
        case LWMSG_STATUS_FILE_NOT_FOUND:
        case LWMSG_STATUS_CONNECTION_REFUSED:
            dwLsaError = ECONNREFUSED;
            break;
    }

    LSA_LOG_DEBUG("Lwmsg error at %s:%d [lwmsg code: %d] [lwmsg string: %s] [lsass code: %d]",
            file,
            line,
            LSA_SAFE_LOG_STRING(
                lwmsg_assoc_get_error_message(pAssoc, dwMsgError)),
            dwLsaError);

    return dwLsaError;
}

DWORD
LsaNtStatusToLsaError(
    IN NTSTATUS Status
    )
{
    switch (Status)
    {
        case STATUS_INSUFFICIENT_RESOURCES:
            return LSA_ERROR_OUT_OF_MEMORY;
        case STATUS_INVALID_SID:
            return LSA_ERROR_INVALID_SID;
        default:
            return LwNtStatusToUnixErrno(Status);
    }
}
