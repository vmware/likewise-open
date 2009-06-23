#include "includes.h"

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszIPAddress,
    LWDNSLogLevel* pMaxLogLevel,
    PFN_LWDNS_LOG_MESSAGE* ppFnLogMessage
    );

static
BOOLEAN
IsValidIPAddress(
    PCSTR pszIPAddress
    );

static
DWORD
GetHostname(
    PSTR* ppszHostname
    );

static
VOID
LogMessage(
    LWDNSLogLevel logLevel,
    PCSTR         pszFormat,
    va_list       msgList
    );

static
VOID
ShowUsage(
    VOID
    );

int
main(
    int   argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hDNSServer = (HANDLE)NULL;
    PSTR   pszDomain = NULL;
    PSTR   pszDnsDomain = NULL;
    PSTR   pszZone = NULL;
    PSTR   pszMachAcctName = NULL;
    PSTR   pszHostname = NULL;
    PSTR   pszHostnameFQDN = NULL;
    LWDNSLogLevel logLevel = LWDNS_LOG_LEVEL_ERROR;
    PFN_LWDNS_LOG_MESSAGE pfnLogger = NULL;
    DWORD  dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PLW_NS_INFO pNameServerInfos = NULL;
    DWORD   dwNumNSInfos = 0;
    PLW_INTERFACE_INFO pInterfaceInfos = NULL;
    DWORD   dwNumItfInfos = 0;
    DWORD   iNS = 0;
    BOOLEAN bDNSUpdated = FALSE;
    PSTR         pszIPAddress = NULL;
    PSOCKADDR_IN pAddrArray = NULL;
    DWORD        dwNumAddrs = 0;
    PLWPS_PASSWORD_INFO pMachineAcctInfo = NULL;
    HANDLE hPasswordStore = (HANDLE)NULL;

    if (geteuid() != 0)
    {
        fprintf(stderr, "Please retry with super-user privileges\n");
        exit(EACCES);
    }

    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszIPAddress,
                    &logLevel,
                    &pfnLogger);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = LWNetExtendEnvironmentForKrb5Affinity(FALSE);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSInitialize();
    BAIL_ON_LWDNS_ERROR(dwError);

    if (pfnLogger)
    {
        DNSSetLogParameters(logLevel, pfnLogger);
    }

    dwError = GetHostname(&pszHostname);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = LwpsOpenPasswordStore(
                  LWPS_PASSWORD_STORE_SQLDB,
                  &hPasswordStore);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = LwpsGetPasswordByHostName(
                  hPasswordStore,
                  pszHostname,
                  &pMachineAcctInfo);
    BAIL_ON_LWDNS_ERROR(dwError);

    pszMachAcctName = awc16stombs(pMachineAcctInfo->pwszMachineAccount);
    if ( !pszMachAcctName )
    {
        dwError = LWDNS_ERROR_STRING_CONV_FAILED;
    }
    BAIL_ON_LWDNS_ERROR(dwError);

    pszDomain = awc16stombs(pMachineAcctInfo->pwszDnsDomainName);
    if ( !pszDomain )
    {
        dwError = LWDNS_ERROR_STRING_CONV_FAILED;
    }
    BAIL_ON_LWDNS_ERROR(dwError);

    pszDnsDomain = awc16stombs(pMachineAcctInfo->pwszHostDnsDomain);
    if ( !pszDnsDomain )
    {
        dwError = LWDNS_ERROR_STRING_CONV_FAILED;
    }
    BAIL_ON_LWDNS_ERROR(dwError);

    DNSStrToUpper(pszDnsDomain);

    dwError = DNSGetNameServers(
                    pszDnsDomain,
                    &pszZone,
                    &pNameServerInfos,
                    &dwNumNSInfos);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (!dwNumNSInfos)
    {
        dwError = LWDNS_ERROR_NO_NAMESERVER;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    if (!IsNullOrEmptyString(pszIPAddress))
    {
        PSOCKADDR_IN pSockAddr = NULL;

        dwError = DNSAllocateMemory(
                        sizeof(SOCKADDR_IN),
                        (PVOID*)&pAddrArray);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwNumAddrs = 1;

        pSockAddr = &pAddrArray[0];
        pSockAddr->sin_family = AF_INET;

        if (!inet_aton(pszIPAddress, &pSockAddr->sin_addr))
        {
            dwError = LWDNS_ERROR_INVALID_IP_ADDRESS;
            BAIL_ON_LWDNS_ERROR(dwError);
        }
    }
    else
    {
        DWORD iAddr = 0;

        dwError = DNSGetNetworkInterfaces(
                        &pInterfaceInfos,
                        &dwNumItfInfos);
        BAIL_ON_LWDNS_ERROR(dwError);

        if (!dwNumItfInfos)
        {
            dwError = LWDNS_ERROR_NO_INTERFACES;
            BAIL_ON_LWDNS_ERROR(dwError);
        }

        dwError = DNSAllocateMemory(
                        sizeof(SOCKADDR_IN) * dwNumItfInfos,
                        (PVOID*)&pAddrArray);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwNumAddrs = dwNumItfInfos;

        for (; iAddr < dwNumAddrs; iAddr++)
        {
            PSOCKADDR_IN pSockAddr = NULL;
            PLW_INTERFACE_INFO pInterfaceInfo = NULL;

            pSockAddr = &pAddrArray[iAddr];
            pInterfaceInfo = &pInterfaceInfos[iAddr];

            pSockAddr->sin_family = pInterfaceInfo->ipAddr.sa_family;

            pSockAddr->sin_addr = ((PSOCKADDR_IN)&pInterfaceInfo->ipAddr)->sin_addr;
        }
    }

    dwError = DNSKrb5Init(pszMachAcctName, pszDomain);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    strlen(pszHostname) + strlen(pszDnsDomain) + 2,
                    (PVOID*)&pszHostnameFQDN);
    BAIL_ON_LWDNS_ERROR(dwError);

    DNSStrToLower(pszHostname);
    DNSStrToLower(pszDnsDomain);

    sprintf(pszHostnameFQDN, "%s.%s", pszHostname, pszDnsDomain);

    for (; !bDNSUpdated && (iNS < dwNumNSInfos); iNS++)
    {
        PSTR   pszNameServer = NULL;
        PLW_NS_INFO pNSInfo = NULL;

        pNSInfo = &pNameServerInfos[iNS];
        pszNameServer = pNSInfo->pszNSHostName;

        if (hDNSServer != (HANDLE)NULL)
        {
            DNSClose(hDNSServer);
        }

        LWDNS_LOG_INFO("Attempting to update name server [%s]", pszNameServer);

        dwError = DNSOpen(
                        pszNameServer,
                        DNS_TCP,
                        &hDNSServer);
        if (dwError)
        {
            LWDNS_LOG_ERROR(
                    "Failed to open connection to Name Server [%s]. [Error code:%d]",
                    pszNameServer,
                    dwError);
            dwError = 0;

            continue;
        }

        dwError = DNSUpdateSecure(
                        hDNSServer,
                        pszNameServer,
                        pszZone,
                        pszHostnameFQDN,
                        dwNumAddrs,
                        pAddrArray);
        if (dwError)
        {
            LWDNS_LOG_ERROR(
                    "Failed to update Name Server [%s]. [Error code:%d]",
                    pszNameServer,
                    dwError);
            dwError = 0;
            
            continue;
        }

        bDNSUpdated = TRUE;
    }

    if (!bDNSUpdated)
    {
        dwError = LWDNS_ERROR_UPDATE_FAILED;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    printf("DNS was updated successfully\n");

cleanup:

    if (hDNSServer) {
        DNSClose(hDNSServer);
    }

    LWDNS_SAFE_FREE_STRING(pszHostname);
    LWDNS_SAFE_FREE_STRING(pszHostnameFQDN);
    LWDNS_SAFE_FREE_STRING(pszIPAddress);
    LWDNS_SAFE_FREE_STRING(pszZone);
    LWDNS_SAFE_FREE_STRING(pszMachAcctName);
    LWDNS_SAFE_FREE_STRING(pszDomain);
    LWDNS_SAFE_FREE_STRING(pszDnsDomain);

    if (pNameServerInfos)
    {
        DNSFreeNameServerInfoArray(
                pNameServerInfos,
                dwNumNSInfos);
    }

    if (pInterfaceInfos)
    {
        DNSFreeNetworkInterfaces(
                pInterfaceInfos,
                dwNumItfInfos);
    }

    if (pAddrArray)
    {
        DNSFreeMemory(pAddrArray);
    }

    if (pMachineAcctInfo)
    {
        LwpsFreePasswordInfo(hPasswordStore, pMachineAcctInfo);
    }

    if (hPasswordStore)
    {
       LwpsClosePasswordStore(hPasswordStore);
    }


    DNSShutdown();

    DNSKrb5Shutdown();

    return(dwError);

error:

    dwErrorBufferSize = DNSGetErrorString(dwError, NULL, 0);

    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;

        dwError2 = DNSAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);

        if (!dwError2)
        {
            DWORD dwLen = DNSGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);

            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to update DNS.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LWDNS_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to update DNS. Error code [%d]\n", dwError);
    }

    goto cleanup;
}

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszIPAddress,
    LWDNSLogLevel* pMaxLogLevel,
    PFN_LWDNS_LOG_MESSAGE* ppfnLogger
    )
{
    typedef enum
    {
        LWDNS_PARSE_MODE_OPEN = 0,
        LWDNS_PARSE_MODE_IP_ADDRESS,
        LWDNS_PARSE_MODE_LOGLEVEL
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = LWDNS_PARSE_MODE_OPEN;
    DWORD iArg = 1;
    LWDNSLogLevel logLevel = LWDNS_LOG_LEVEL_ERROR;
    PFN_LWDNS_LOG_MESSAGE pfnLogger = NULL;
    PSTR pszIPAddress = NULL;

    for (; iArg < argc; iArg++)
    {
        PCSTR pszArg = argv[iArg];

        switch(parseMode)
        {
            case LWDNS_PARSE_MODE_OPEN:

                if (!strcasecmp(pszArg, "-h") ||
                    !strcasecmp(pszArg, "--help"))
                {
                    ShowUsage();
                    exit(0);
                }
                else if (!strcasecmp(pszArg, "--loglevel"))
                {
                    parseMode = LWDNS_PARSE_MODE_LOGLEVEL;
                }
                else if (!strcasecmp(pszArg, "--ipaddress"))
                {
                    parseMode = LWDNS_PARSE_MODE_IP_ADDRESS;
                }
                else
                {
                    ShowUsage();
                    exit(1);
                }

                break;
                
            case LWDNS_PARSE_MODE_IP_ADDRESS:
                
                if (IsValidIPAddress(pszArg))
                {
                    dwError = DNSAllocateString(
                                    pszArg,
                                    &pszIPAddress);
                    BAIL_ON_LWDNS_ERROR(dwError);
                }

                parseMode = LWDNS_PARSE_MODE_OPEN;
                
                break;

            case LWDNS_PARSE_MODE_LOGLEVEL:

                if (!strcasecmp(pszArg, "error"))
                {
                    logLevel = LWDNS_LOG_LEVEL_ERROR;
                    pfnLogger = &LogMessage;
                }
                else if (!strcasecmp(pszArg, "warning"))
                {
                    logLevel = LWDNS_LOG_LEVEL_WARNING;
                    pfnLogger = &LogMessage;
                }
                else if (!strcasecmp(pszArg, "info"))
                {
                    logLevel = LWDNS_LOG_LEVEL_INFO;
                    pfnLogger = &LogMessage;
                }
                else if (!strcasecmp(pszArg, "verbose"))
                {
                    logLevel = LWDNS_LOG_LEVEL_VERBOSE;
                    pfnLogger = &LogMessage;
                }
                else if (!strcasecmp(pszArg, "debug"))
                {
                    logLevel = LWDNS_LOG_LEVEL_DEBUG;
                    pfnLogger = &LogMessage;
                }

                parseMode = LWDNS_PARSE_MODE_OPEN;

                break;

            default:

                break;
        }
    }

    *pMaxLogLevel = logLevel;
    *ppszIPAddress = pszIPAddress;
    *ppfnLogger = pfnLogger;

cleanup:

    return dwError;
    
error:

    *pMaxLogLevel = LWDNS_LOG_LEVEL_ERROR;
    *ppfnLogger = NULL;
    *ppszIPAddress = NULL;
    
    LWDNS_SAFE_FREE_STRING(pszIPAddress);

    goto cleanup;
}

static
BOOLEAN
IsValidIPAddress(
    PCSTR pszIPAddress
    )
{
    BOOLEAN bResult = FALSE;
    
    if (!IsNullOrEmptyString(pszIPAddress))
    {
        struct in_addr ipAddr;
        
        if (inet_aton(pszIPAddress, &ipAddr) != 0)
        {
            bResult = TRUE;
        }
    }
    
    return bResult;
}

static
DWORD
GetHostname(
    PSTR* ppszHostname
    )
{
    DWORD dwError = 0;
    CHAR szBuffer[256];
    PSTR pszLocal = NULL;
    PSTR pszDot = NULL;
    int len = 0;
    PSTR pszHostname = NULL;

    if ( gethostname(szBuffer, sizeof(szBuffer)) != 0 )
    {
        dwError = errno;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    len = strlen(szBuffer);
    if ( len > strlen(".local") )
    {
        pszLocal = &szBuffer[len - strlen(".local")];
        if ( !strcasecmp( pszLocal, ".local" ) )
        {
            pszLocal[0] = '\0';
        }
    }

    /* Test to see if the name is still dotted.
     * If so we will chop it down to just the
     * hostname field.
     */
    pszDot = strchr(szBuffer, '.');
    if ( pszDot )
    {
        pszDot[0] = '\0';
    }

    dwError = DNSAllocateString(
                    szBuffer,
                    &pszHostname);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppszHostname = pszHostname;

cleanup:

    return dwError;

error:

    LWDNS_SAFE_FREE_STRING(pszHostname);

    *ppszHostname = NULL;

    goto cleanup;
}

static
VOID
LogMessage(
    LWDNSLogLevel logLevel,
    PCSTR         pszFormat,
    va_list       msgList
    )
{
    PCSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];

    switch (logLevel)
    {
        case LWDNS_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = LWDNS_INFO_TAG;
            break;
        }
        case LWDNS_LOG_LEVEL_ERROR:
        {
            pszEntryType = LWDNS_ERROR_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_WARNING:
        {
            pszEntryType = LWDNS_WARN_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_INFO:
        {
            pszEntryType = LWDNS_INFO_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_VERBOSE:
        {
            pszEntryType = LWDNS_VERBOSE_TAG;
            break;
        }

        case LWDNS_LOG_LEVEL_DEBUG:
        {
            pszEntryType = LWDNS_DEBUG_TAG;
            break;
        }

        default:
        {
            pszEntryType = LWDNS_VERBOSE_TAG;
            break;
        }
    }

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LWDNS_LOG_TIME_FORMAT, &tmp);

    fprintf(stdout, "%s:%s:", timeBuf, pszEntryType);
    vfprintf(stdout, pszFormat, msgList);
    fprintf(stdout, "\n");
    fflush(stdout);
}

static
VOID
ShowUsage(
    VOID
    )
{
    fprintf(stdout, "Usage: lw-update-dns [ --loglevel {error, warning, info, verbose} ] [ --ipaddress {IP address to register for computer} ]\n");
}
