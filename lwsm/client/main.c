/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Client utility program
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static struct
{
    BOOLEAN bQuiet;
} gState =
{
    .bQuiet = FALSE
};

static
PCSTR
LwSmStatusToString(
    LW_SERVICE_STATUS status
    )
{
    switch (status)
    {
    case LW_SERVICE_STOPPED:
        return "stopped";
    case LW_SERVICE_STARTING:
        return "starting";
    case LW_SERVICE_RUNNING:
        return "running";
    case LW_SERVICE_STOPPING:
        return "stopping";
    case LW_SERVICE_PAUSED:
        return "paused";
    case LW_SERVICE_DEAD:
        return "dead";
    default:
        return "unknown";
    }
}

static
PCSTR
LwSmTypeToString(
    LW_SERVICE_TYPE type
    )
{
    switch (type)
    {
    case LW_SERVICE_EXECUTABLE:
        return "legacy executable";
    case LW_SERVICE_SM_EXECUTABLE:
        return "service manager executable";
    case LW_SERVICE_MODULE:
        return "module";
    case LW_SERVICE_DRIVER:
        return "driver";
    default:
        return "unknown";
    }
}

static
DWORD
LwSmList(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceNames = NULL;
    PSTR pszServiceName = NULL;
    LW_SERVICE_STATUS status = LW_SERVICE_DEAD;
    LW_SERVICE_HANDLE hHandle = NULL;
    size_t i = 0;

    dwError = LwSmEnumerateServices(&ppwszServiceNames);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszServiceNames[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszServiceNames[i], &hHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmGetServiceStatus(hHandle, &status);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmReleaseServiceHandle(hHandle);
        hHandle = NULL;
        BAIL_ON_ERROR(dwError);

        dwError = LwWc16sToMbs(ppwszServiceNames[i], &pszServiceName);
        BAIL_ON_ERROR(dwError);
        
        if (!gState.bQuiet)
        {
            printf("%s: %s\n", pszServiceName, LwSmStatusToString(status));
        }

        LW_SAFE_FREE_MEMORY(pszServiceName);    
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (ppwszServiceNames)
    {
        LwSmFreeServiceNameList(ppwszServiceNames);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStart(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    
    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        printf("Starting service: %s\n", pArgv[1]);
    }

    dwError = LwSmStartService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}


static
DWORD
LwSmStartAll(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status = LW_SERVICE_DEAD;
    PWSTR* ppwszDependencies = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmGetServiceDependencyClosure(hHandle, &ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmGetServiceStatus(hDepHandle, &status);
        BAIL_ON_ERROR(dwError);

        if (status != LW_SERVICE_RUNNING)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszDependencies[i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                
                printf("Starting service dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }

            dwError = LwSmStartService(hDepHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

    if (!gState.bQuiet)
    {
        printf("Starting service: %s\n", pArgv[1]);
    }

    dwError = LwSmStartService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
    }

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStop(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    
    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        printf("Stopping service: %s\n", pArgv[1]);
    }

    dwError = LwSmStopService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStopAll(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    LW_SERVICE_STATUS status = LW_SERVICE_DEAD;
    PWSTR* ppwszDependencies = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmGetServiceReverseDependencyClosure(hHandle, &ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmGetServiceStatus(hDepHandle, &status);
        BAIL_ON_ERROR(dwError);

        if (status != LW_SERVICE_STOPPED)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszDependencies[i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                
                printf("Stopping service reverse dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }

            dwError = LwSmStopService(hDepHandle);
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

    if (!gState.bQuiet)
    {
        printf("Stopping service: %s\n", pArgv[1]);
    }

    dwError = LwSmStopService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (ppwszDependencies)
    {
        LwSmFreeServiceNameList(ppwszDependencies);
    }

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmRestartAll(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PWSTR* ppwszReverseDeps = NULL;
    PLW_SERVICE_STATUS pStatus = NULL;
    PLW_SERVICE_HANDLE phDepHandles = NULL;
    PSTR pszTemp = NULL;
    size_t count = 0;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmGetServiceReverseDependencyClosure(hHandle, &ppwszReverseDeps);
    BAIL_ON_ERROR(dwError);

    count = LwSmStringListLength(ppwszReverseDeps);

    dwError = LwAllocateMemory(sizeof(*pStatus) * count, OUT_PPVOID(&pStatus));
    BAIL_ON_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*phDepHandles) * count, OUT_PPVOID(&phDepHandles));
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < count; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszReverseDeps[i], &phDepHandles[i]);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmGetServiceStatus(phDepHandles[i], &pStatus[i]);
        BAIL_ON_ERROR(dwError);

        if (pStatus[i] != LW_SERVICE_STOPPED)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszReverseDeps[i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                printf("Stopping service reverse dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }
            dwError = LwSmStopService(phDepHandles[i]);
            BAIL_ON_ERROR(dwError);
        }
    }

    if (!gState.bQuiet)
    {
        printf("Stopping service: %s\n", pArgv[1]);
    }

    dwError = LwSmStopService(hHandle);
    BAIL_ON_ERROR(dwError);

    if (!gState.bQuiet)
    {
        printf("Starting service: %s\n", pArgv[1]);
    }

    dwError = LwSmStartService(hHandle);
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < count; i++)
    {
        if (pStatus[count - 1 - i] == LW_SERVICE_RUNNING)
        {
            if (!gState.bQuiet)
            {
                dwError = LwWc16sToMbs(ppwszReverseDeps[count - 1 - i], &pszTemp);
                BAIL_ON_ERROR(dwError);
                printf("Starting service reverse dependency: %s\n", pszTemp);
                LW_SAFE_FREE_MEMORY(pszTemp);
            }
            dwError = LwSmStartService(phDepHandles[count - 1 - i]);
            BAIL_ON_ERROR(dwError);
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pStatus);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    if (phDepHandles)
    {
        for (i = 0; i < count; i++)
        {
            if (phDepHandles[i])
            {
                LwSmReleaseServiceHandle(phDepHandles[i]);
            }
        }

        LW_SAFE_FREE_MEMORY(phDepHandles);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmRefresh(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    
    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        printf("Refreshing service: %s\n", pArgv[1]);
    }

    dwError = LwSmRefreshService(hHandle);
    BAIL_ON_ERROR(dwError);
    
cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmInfo(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    PLW_SERVICE_INFO pInfo = NULL;
    PSTR pszTemp = NULL;
    size_t i = 0;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmGetServiceInfo(hHandle, &pInfo);
    BAIL_ON_ERROR(dwError);
    
    printf("Service: %s\n", pArgv[1]);

    dwError = LwWc16sToMbs(pInfo->pwszDescription, &pszTemp);
    BAIL_ON_ERROR(dwError);

    printf("Description: %s\n", pszTemp);
    LW_SAFE_FREE_MEMORY(pszTemp);

    printf("Type: %s\n", LwSmTypeToString(pInfo->type));
    printf("Startup service: %s\n", pInfo->bStartupService ? "yes" : "no");

    dwError = LwWc16sToMbs(pInfo->pwszPath, &pszTemp);
    BAIL_ON_ERROR(dwError);

    printf("Path: %s\n", pszTemp);
    LW_SAFE_FREE_MEMORY(pszTemp);

    printf("Arguments:");

    for (i = 0; pInfo->ppwszArgs[i]; i++)
    {
         dwError = LwWc16sToMbs(pInfo->ppwszArgs[i], &pszTemp);
         BAIL_ON_ERROR(dwError);

         printf(" '%s'", pszTemp);

         LW_SAFE_FREE_MEMORY(pszTemp);
    }

    printf("\n");

    printf("Dependencies:");

    for (i = 0; pInfo->ppwszDependencies[i]; i++)
    {
         dwError = LwWc16sToMbs(pInfo->ppwszDependencies[i], &pszTemp);
         BAIL_ON_ERROR(dwError);

         printf(" %s", pszTemp);

         LW_SAFE_FREE_MEMORY(pszTemp);
    }

    printf("\n");

cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);
    LW_SAFE_FREE_MEMORY(pszTemp);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmStatus(
    int argc,
    char** pArgv,
    int* pRet
    )
{
    DWORD dwError = 0;
    PWSTR pwszServiceName = NULL;
    LW_SERVICE_HANDLE hHandle = NULL;
    LW_SERVICE_STATUS status = LW_SERVICE_DEAD;

    dwError = LwMbsToWc16s(pArgv[1], &pwszServiceName);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmAcquireServiceHandle(pwszServiceName, &hHandle);
    BAIL_ON_ERROR(dwError);
    
    dwError = LwSmGetServiceStatus(hHandle, &status);
    BAIL_ON_ERROR(dwError);
    
    if (!gState.bQuiet)
    {
        printf("%s\n", LwSmStatusToString(status));
    }

    *pRet = status;

cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszServiceName);

    if (hHandle)
    {
        LwSmReleaseServiceHandle(hHandle);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmUsage(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;

    printf("Usage: %s [ options ... ] <command> ...\n\n", pArgv[0]);
    printf("Commands:\n"
           "    list                       List all known services and their status\n"
           "    start <service>            Start a service\n"
           "    start-all <service>        Start a service and all dependencies\n"
           "    stop <service>             Stop a service\n"
           "    stop-all <service>         Stop a service and all reverse dependencies\n"
           "    restart-all <service>      Restart a service and all running reverse dependencies\n"
           "    refresh <service>          Refresh service's configuration\n"
           "    info <service>             Get information about a service\n"
           "    status <service>           Get the status of a service\n\n");
    printf("Options:\n"
           "    -q, --quiet                Suppress console output\n"
           "    -h, --help                 Show usage information\n\n");

    return dwError;
}

int
main(
    int argc,
    char** pArgv
    )
{
    DWORD dwError = 0;
    CHAR szErrorMessage[2048];
    int ret = 0;
    int i = 0;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(pArgv[i], "-h") ||
            !strcmp(pArgv[i], "--help"))
        {
            dwError = LwSmUsage(argc, pArgv);
            goto error;
        }
        if (!strcmp(pArgv[i], "-q") ||
            !strcmp(pArgv[i], "--quiet"))
        {
            gState.bQuiet = TRUE;
        }
        else if (!strcmp(pArgv[i], "list"))
        {
            dwError = LwSmList(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "start"))
        {
            dwError = LwSmStart(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "start-all"))
        {
            dwError = LwSmStartAll(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "stop"))
        {
            dwError = LwSmStop(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "stop-all"))
        {
            dwError = LwSmStopAll(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "info"))
        {
            dwError = LwSmInfo(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "status"))
        {
            dwError = LwSmStatus(argc-i, pArgv+i, &ret);
            goto error;
        }
        else if (!strcmp(pArgv[i], "refresh"))
        {
            dwError = LwSmRefresh(argc-i, pArgv+i);
            goto error;
        }
        else if (!strcmp(pArgv[i], "restart-all"))
        {
            dwError = LwSmRestartAll(argc-i, pArgv+i);
            goto error;
        }
        else
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_ERROR(dwError);
        }
    }

    dwError = LwSmUsage(argc, pArgv);
    BAIL_ON_ERROR(dwError);

error:

    if (dwError)
    {
        memset(szErrorMessage, 0, sizeof(szErrorMessage));
        LwGetErrorString(dwError, szErrorMessage, sizeof(szErrorMessage) - 1);

        if (!gState.bQuiet)
        {
            printf("Error: %s\n", szErrorMessage);
        }

        return 1;
    }
    else
    {
        return ret;
    }
}
