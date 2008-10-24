#include "includes.h"

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    LWDNSLogLevel* pMaxLogLevel,
    PFN_LWDNS_LOG_MESSAGE* ppFnLogMessage
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

    if (geteuid() != 0)
    {
        fprintf(stderr, "Please retry with super-user privileges\n");
        exit(EACCES);
    }

    dwError = ParseArgs(
                    argc,
                    argv,
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

    dwError = LWNetGetCurrentDomain(&pszDomain);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    dwError = DNSGetNameServers(
                    pszDomain,
                    &pNameServerInfos,
                    &dwNumNSInfos);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    if (!dwNumNSInfos)
    {
        dwError = LWDNS_ERROR_NO_NAMESERVER;
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    
    dwError = DNSGetNetworkInterfaces(
                    &pInterfaceInfos,
                    &dwNumItfInfos);
    BAIL_ON_LWDNS_ERROR(dwError);
    
    if (!dwNumItfInfos)
    {
        dwError = LWDNS_ERROR_NO_INTERFACES;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = GetHostname(&pszHostname);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSKrb5Init(pszHostname, pszDomain);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSAllocateMemory(
                    strlen(pszHostname) + strlen(pszDomain) + 2,
                    (PVOID*)&pszHostnameFQDN);
    BAIL_ON_LWDNS_ERROR(dwError);

    DNSStrToLower(pszHostname);
    DNSStrToLower(pszDomain);

    sprintf(pszHostnameFQDN, "%s.%s", pszHostname, pszDomain);

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
                        pszDomain,
                        pszHostnameFQDN,
                        pInterfaceInfos,
                        dwNumItfInfos);
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

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
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
    LWDNSLogLevel* pMaxLogLevel,
    PFN_LWDNS_LOG_MESSAGE* ppfnLogger
    )
{
    typedef enum
    {
        LWDNS_PARSE_MODE_OPEN = 0,
        LWDNS_PARSE_MODE_LOGLEVEL
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = LWDNS_PARSE_MODE_OPEN;
    DWORD iArg = 1;
    LWDNSLogLevel logLevel = LWDNS_LOG_LEVEL_ERROR;
    PFN_LWDNS_LOG_MESSAGE pfnLogger = NULL;

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
                else
                {
                    ShowUsage();
                    exit(1);
                }

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
    *ppfnLogger = pfnLogger;

    return dwError;
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
    fprintf(stdout, "Usage: lw-update-dns [ --loglevel {error, warning, info, verbose} ]\n");
}
