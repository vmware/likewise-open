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
 *        lwnet-error.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */

#include "includes.h"

static
const char* gLWNetErrorMessages[] = 
{
    // LWNET_ERROR_INVALID_CACHE_PATH                              : 40960
    "An invalid cache path was specified",
    // LWNET_ERROR_INVALID_CONFIG_PATH                             : 40961
    "The path to the configuration file is invalid",
    // LWNET_ERROR_INVALID_PREFIX_PATH                             : 40962
    "The product installation folder could not be determined",
    // LWNET_ERROR_INSUFFICIENT_BUFFER                             : 40963
    "The provided buffer is insufficient",
    // LWNET_ERROR_OUT_OF_MEMORY                                   : 40964
    "Out of memory",
    // LWNET_ERROR_INVALID_MESSAGE                                 : 40965
    "The Inter Process message is invalid",
    // LWNET_ERROR_UNEXPECTED_MESSAGE                              : 40966
    "An unexpected Inter Process message was received",
    // LWNET_ERROR_DATA_ERROR                                      : 40967
    "The cached data is incorrect",
    // LWNET_ERROR_NOT_IMPLEMENTED                                 : 40968
    "The requested feature has not been implemented yet",
    // LWNET_ERROR_NO_CONTEXT_ITEM                                 : 40969
    "The requested item was not found in context",
    // LWNET_ERROR_REGEX_COMPILE_FAILED                            : 40970
    "Failed to compile regular expression",
    // LWNET_ERROR_INTERNAL                                        : 40971
    "Internal error",
    // LWNET_ERROR_INVALID_DNS_RESPONSE                            : 40972
    "Response from DNS is invalid",
    // LWNET_ERROR_DNS_RESOLUTION_FAILED                           : 40973
    "Failed to resolve query using DNS",
    // LWNET_ERROR_FAILED_TIME_CONVERSION                          : 40974
    "Failed to convert the time",
    // LWNET_ERROR_INVALID_SID                                     : 40975
    "The security descriptor (SID) is invalid",
    // LWNET_ERROR_UNEXPECTED_DB_RESULT                            : 40976
    "Unexpected cached data found",
    // LWNET_ERROR_INVALID_LWNET_CONNECTION                         : 40977
    "The connection to the authentication service is invalid",
    // LWNET_ERROR_INVALID_PARAMETER                               : 40978
    "Invalid parameter",
    // LWNET_ERROR_LDAP_NO_PARENT_DN                               : 40979
    "No distinguished name found in LDAP for parent of this object",
    // LWNET_ERROR_LDAP_ERROR                                      : 40980
    "An Error was encountered when negotiating with LDAP",
    // LWNET_ERROR_NO_SUCH_DOMAIN                                  : 40981
    "Unknown Active Directory domain",
    // LWNET_ERROR_LDAP_FAILED_GETDN                               : 40982
    "Failed to find distinguished name using LDAP",
    // LWNET_ERROR_DUPLICATE_DOMAINNAME                            : 40983
    "A duplicate Active Directory domain name was found",
    // LWNET_ERROR_FAILED_FIND_DC                                  : 40984
    "Failed to find the domain controller",
    // LWNET_ERROR_LDAP_GET_DN_FAILED                              : 40985
    "Failed to find distinguished name using LDAP",
    // LWNET_ERROR_INVALID_SID_REVISION                            : 40986
    "The security descriptor (SID) has an invalid revision",
    // LWNET_ERROR_LOAD_LIBRARY_FAILED                             : 40987
    "Failed to dynamically load a library",
    // LWNET_ERROR_LOOKUP_SYMBOL_FAILED                            : 40988
    "Failed to lookup a symbol in a dynamic library",
    // LWNET_ERROR_INVALID_EVENTLOG                                : 40989
    "The Eventlog interface is invalid",
    // LWNET_ERROR_INVALID_CONFIG                                  : 40990
    "The specified configuration (file) is invalid",
    // LWNET_ERROR_UNEXPECTED_TOKEN                                : 40991
    "An unexpected token was encountered in the configuration",
    // LWNET_ERROR_LDAP_NO_RECORDS_FOUND                           : 40992
    "No records were found in the cache",
    // LWNET_ERROR_STRING_CONV_FAILED                              : 40993
    "Failed to convert string format (wide/ansi)",
    // LWNET_ERROR_QUERY_CREATION_FAILED                           : 40994
    "Failed to create query to examine cache",
    // LWNET_ERROR_NOT_JOINED_TO_AD                                : 40995
    "This machine is not currently joined to Active Directory",
    // LWNET_ERROR_FAILED_TO_SET_TIME                              : 40996
    "The system time could not be set",
    // LWNET_ERROR_NO_NETBIOS_NAME                                 : 40997
    "Failed to find NetBIOS name for the domain",
    // LWNET_ERROR_INVALID_NETLOGON_RESPONSE                       : 40998
    "The Netlogon response buffer is invalid",
    // LWNET_ERROR_INVALID_OBJECTGUID                              : 40999
    "The specified Globally Unique Identifier (GUID) is invalid",
    // LWNET_ERROR_INVALID_DOMAIN                                  : 41000
    "The domain name is invalid",
    // LWNET_ERROR_NO_DEFAULT_REALM                                : 41001
    "The kerberos default realm is not set",
    //  LWNET_ERROR_NOT_SUPPORTED                                  : 41002
    "The request is not supported",   
    // LWNET_ERROR_NO_LWNET_INFORMATION                             : 41003
    "No lwnet information was found for the active directory domain",
    // LWNET_ERROR_NO_HANDLER                                       : 41004
    "No handler could be found for this message type",
    // LWNET_ERROR_NO_MATCHING_CACHE_ENTRY                          : 41005
    "No matching entry in the Domain Controller Cache could be found",
    // LWNET_ERROR_KRB5_CONF_FILE_OPEN_FAILED                       : 41006
    "LW Netlogon was unable to open a krb5.conf file for writing",
    // LWNET_ERROR_KRB5_CONF_FILE_WRITE_FAILED                      : 41007
    "LW Netlogon was unable to write to a krb5.conf file",
    // LWNET_ERROR_DOMAIN_NOT_FOUND                                 : 41008
    "LW Netlogon reports an invalid domain or domain not found.",
    // LWNET_ERROR_CONNECTION_CLOSED                                : 41009
    "LW Netlogon reports that the peer closed the socket connection",
    // LWNET_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK                : 41010
    "The system hostname configuration is incorrect, netlogond stopping",
    // LWNET_ERROR_MAC_FLUSH_DS_CACHE_FAILED                       :  41011
    "Could not find DirectoryService cache utility to call -flushcache with"
};

size_t
LWNetGetErrorString(
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
        
    if (LWNET_ERROR_MASK(dwError) != 0)
    {
        stResult = LWNetMapLWNetErrorToString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    else
    {
        stResult = LWNetGetSystemErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    
cleanup:
    
    return stResult;
}

size_t
LWNetMapLWNetErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    DWORD dwNMessages = sizeof(gLWNetErrorMessages)/sizeof(PCSTR);
    
    if ((dwError >= LWNET_ERROR_PREFIX) &&
        (dwError < LWNET_ERROR_SENTINEL))
    {
        DWORD dwErrorOffset = dwError - LWNET_ERROR_PREFIX;
        
        if (dwErrorOffset < dwNMessages)
        {
            PCSTR pszMessage = gLWNetErrorMessages[dwErrorOffset];
            DWORD dwRequiredLen = strlen(pszMessage) + 1;
            
            if (stBufSize >= dwRequiredLen) {
                memcpy(pszBuffer, pszMessage, dwRequiredLen);
            }
            
            stResult = dwRequiredLen;
           
            goto cleanup;
        }
    }              
    
    stResult = LWNetGetUnmappedErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);

cleanup:

    return stResult;
}

size_t
LWNetGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    DWORD  dwError = LWNET_ERROR_SUCCESS;
    size_t stResult = 0;
    PSTR   pszTempBuffer = NULL;

    int result = LWNetStrError(dwConvertError, pszBuffer, stBufSize);

    while (result < 0)
    {
        if(errno == ERANGE)
        {
            // Guess
            stBufSize = stBufSize * 2 + 10;
        }
        else
        {
            stResult = LWNetGetUnmappedErrorString(
                            dwConvertError,
                            pszBuffer,
                            stBufSize);
            goto cleanup;
        }
        LWNET_SAFE_FREE_MEMORY(pszTempBuffer);

        dwError = LWNetAllocateMemory(
                        stBufSize,
                        (PVOID*)&pszTempBuffer);
        BAIL_ON_LWNET_ERROR(dwError);

        result = LWNetStrError(dwConvertError, pszTempBuffer, stBufSize);
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

    LWNET_SAFE_FREE_MEMORY(pszTempBuffer);
    
    return stResult;

error:

    stResult = 0;
    
    goto cleanup;
}

size_t
LWNetGetUnmappedErrorString(
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
