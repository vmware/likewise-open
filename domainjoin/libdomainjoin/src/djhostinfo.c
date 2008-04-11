/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "domainjoin.h"
#include "ctshell.h"

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

CENTERROR
DJGetComputerName(
    PSTR* ppszComputerName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    CHAR szBuf[256+1];
    PSTR pszTmp = NULL;

    if (geteuid() != 0) {
       ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (gethostname(szBuf, 256) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    pszTmp = szBuf;
    while (*pszTmp != '\0') {
        if (*pszTmp == '.') {
            *pszTmp = '\0';
            break;
        }
        pszTmp++;
    }

    if (IsNullOrEmptyString(szBuf)) {
        ceError = CENTERROR_DOMAINJOIN_INVALID_HOSTNAME;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateString(szBuf, ppszComputerName);
    BAIL_ON_CENTERIS_ERROR(ceError);

error:

    return ceError;
}

static
CENTERROR
WriteHostnameToFiles(
    PSTR pszComputerName,
    PSTR* ppszHostfilePaths
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFilePath = (ppszHostfilePaths ? *ppszHostfilePaths : NULL);
    BOOLEAN bFileExists = FALSE;
    FILE* fp = NULL;

    while (pszFilePath != NULL && *pszFilePath != '\0') {

        ceError = CTCheckFileExists(pszFilePath, &bFileExists);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (bFileExists) {
            fp = fopen(pszFilePath, "w");
            if (fp == NULL) {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
            fprintf(fp, "%s\n", pszComputerName);
            fclose(fp);
            fp = NULL;
        }

        pszFilePath = *(++ppszHostfilePaths);
    }

error:

    if (fp) {
        fclose(fp);
    }

    return ceError;
}

#if defined(_HPUX_SOURCE)
#define NETCONF "/etc/rc.config.d/netconf"
static CENTERROR SetHPUXHostname(PSTR pszComputerName)
{
  CENTERROR ceError = CENTERROR_SUCCESS;
  PPROCINFO pProcInfo = NULL;
  PSTR *ppszArgs = NULL;
  DWORD nArgs = 6;
  CHAR szBuf[512];
  LONG status = 0;

  DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

  ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("/bin/sh", ppszArgs);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("-c", ppszArgs+1);
  BAIL_ON_CENTERIS_ERROR(ceError);

  memset(szBuf, 0, sizeof(szBuf));
  snprintf(szBuf, sizeof(szBuf), "/usr/bin/sed s/HOSTNAME=\\\"[a-zA-Z0-9].*\\\"/HOSTNAME=\\\"%s\\\"/ %s > %s.new", pszComputerName, NETCONF, NETCONF);
  ceError = CTAllocateString(szBuf, ppszArgs+2);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = DJGetProcessStatus(pProcInfo, &status);
  BAIL_ON_CENTERIS_ERROR(ceError);

  if (status != 0) {
    ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }

  memset(szBuf, 0, sizeof(szBuf));
  snprintf(szBuf, sizeof(szBuf), "%s.new", NETCONF);

  ceError = CTMoveFile(szBuf, NETCONF);
  BAIL_ON_CENTERIS_ERROR(ceError);

  CTFreeStringArray(ppszArgs, nArgs);
  ppszArgs = NULL;
  FreeProcInfo(pProcInfo);
  pProcInfo = NULL;

  /* After updating the file, HP-UX wants us to "start" the hostname */
  ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("/sbin/init.d/hostname", ppszArgs);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = CTAllocateString("start", ppszArgs + 1);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
  BAIL_ON_CENTERIS_ERROR(ceError);

  ceError = DJGetProcessStatus(pProcInfo, &status);
  BAIL_ON_CENTERIS_ERROR(ceError);

  if (status != 0) {
    ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }

 error:
  if(ppszArgs)
    CTFreeStringArray(ppszArgs, nArgs);

  if(pProcInfo)
    FreeProcInfo(pProcInfo);

  return ceError;
}
#endif /* _HPUX_SOURCE */

#if defined(_AIX)
static
CENTERROR
SetAIXHostname(
    PSTR pszComputerName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 6;
    CHAR  szBuf[512];
    LONG  status = 0;

    DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("chdev", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-a", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "hostname=%s", pszComputerName);
    ceError = CTAllocateString(szBuf, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-l", ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("inet0", ppszArgs+4);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

#endif

#if defined(__LWI_SOLARIS__)
static
CENTERROR
WriteHostnameToSunFiles(
    PSTR pszComputerName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* fp = NULL;
    PSTR* ppszHostfilePaths = NULL;
    DWORD nPaths = 0;
    DWORD iPath = 0;
    PSTR contents = NULL;
    long fileLen;

    DJ_LOG_INFO("Setting hostname to [%s]", pszComputerName);

    fp = fopen("/etc/nodename", "w");
    if (fp == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    fprintf(fp, "%s\n", pszComputerName);
    fclose(fp);
    fp = NULL;

    ceError = CTGetMatchingFilePathsInFolder("/etc",
                                             "hostname.*",
                                             &ppszHostfilePaths,
                                             &nPaths);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (iPath = 0; iPath < nPaths; iPath++) {

        CTReadFile(*(ppszHostfilePaths+iPath), &contents, &fileLen);
        CTStripWhitespace(contents);
        if(!strcasecmp(contents, pszComputerName))
        {
            //The machine hostname is already set
            goto done;
        }
    }

    // Only write the hostname in 1 file
    for (iPath = 0; iPath < nPaths && iPath < 1; iPath++) {

        fp = fopen(*(ppszHostfilePaths+iPath), "w");
        if (fp == NULL) {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        fprintf(fp, "%s\n", pszComputerName);
        fclose(fp);
        fp = NULL;
    }

error:
done:

    if (ppszHostfilePaths)
        CTFreeStringArray(ppszHostfilePaths, nPaths);

    if (fp) {
        fclose(fp);
    }

    CT_SAFE_FREE_STRING(contents);

    return ceError;
}
#endif

#if defined(__LWI_MACOSX__)
static
CENTERROR
SetMacOsXHostName(
    PCSTR HostName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int EE = 0;
    char command[] = "scutil";
    /* ISSUE-2007/08/01-dalmeida -- Fix const-ness of arg array in procutils */
    PSTR args[5] = { command, "--set", "HostName", (char*)HostName };
    PPROCINFO procInfo = NULL;
    LONG status = 0;

    ceError = DJSpawnProcessSilent(command, args, &procInfo);
    GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = DJGetProcessStatus(procInfo, &status);
    GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);

    if (status != 0) {
        DJ_LOG_ERROR("%s failed [Status code: %d]", command, status);
        ceError = CENTERROR_DOMAINJOIN_FAILED_SET_HOSTNAME;
        GOTO_CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

cleanup:
    if (procInfo)
    {
        FreeProcInfo(procInfo);
    }

    DJ_LOG_VERBOSE("SetMacOsXHostName LEAVE -> 0x%08x (EE = %d)", ceError, EE);

    return ceError;
}
#endif

static
CENTERROR
DJCheckIfDHCPHost(
    PSTR pszPathifcfg,
    PBOOLEAN pbDHCPHost
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFilter = "^[[:space:]]*BOOTPROTO.*dhcp.*$";
    BOOLEAN bDHCPHost = FALSE;

    DJ_LOG_INFO("Checking if DHCP Host...");

    // now that we have a file, we need to check out our BOOTPROTO,
    // if it's DHCP, we have to update the DHCP_HOSTNAME
    // ps: the expression should be BOOTPROTO='?dhcp'? because RH uses dhcp and SuSE 'dhcp'
    // sRun = "grep BOOTPROTO=\\'\\\\?dhcp\\'\\\\? " + sPathifcfg;
    //sRun = "grep BOOTPROTO=\\'*dhcp\\'* " + sPathifcfg;

    ceError = CTCheckFileHoldsPattern(pszPathifcfg, pszFilter, &bDHCPHost);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pbDHCPHost = bDHCPHost;

    return ceError;

error:

    *pbDHCPHost = FALSE;

    return ceError;
}

static
CENTERROR
GetTmpPath(
    PCSTR pszOriginalPath,
    PSTR* ppszTmpPath
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR pszSuffix = ".domainjoin";
    PSTR pszTmpPath = NULL;

    ceError = CTAllocateMemory(strlen(pszOriginalPath)+strlen(pszSuffix)+1,
                               PPCAST(&pszTmpPath));
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pszTmpPath, pszOriginalPath);
    strcat(pszTmpPath, pszSuffix);

    *ppszTmpPath = pszTmpPath;

    return ceError;

error:

    if (pszTmpPath)
        CTFreeString(pszTmpPath);

    return ceError;
}

static
BOOLEAN
IsComment(
    PSTR pszLine
    )
{
    PSTR pszTmp = pszLine;

    if (IsNullOrEmptyString(pszLine))
        return TRUE;

    while (*pszTmp != '\0' && isspace((int) *pszTmp))
        pszTmp++;

    return *pszTmp == '#' || *pszTmp == '\0';
}

static
CENTERROR
DJReplaceNameValuePair(
    PSTR pszFilePath,
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszTmpPath = NULL;
    FILE* fpSrc = NULL;
    FILE* fpDst = NULL;
    regex_t rx;
    CHAR szRegExp[256];
    CHAR szBuf[1024+1];
    BOOLEAN bRemoveFile = FALSE;

    memset(&rx, 0, sizeof(rx));

    ceError = GetTmpPath(pszFilePath, &pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szRegExp, "^[[:space:]]*%s[[:space:]]*=.*$", pszName);

    if (regcomp(&rx, szRegExp, REG_EXTENDED) < 0) {
        ceError = CENTERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((fpSrc = fopen(pszFilePath, "r")) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if ((fpDst = fopen(pszTmpPath, "w")) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    bRemoveFile = TRUE;

    while (1) {

        if (fgets(szBuf, 1024, fpSrc) == NULL) {
            if (feof(fpSrc)) {
                break;
            } else {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        if (!IsComment(szBuf) &&
            !regexec(&rx, szBuf, (size_t)0, NULL, 0)) {

            if (fprintf(fpDst, "%s=%s\n", pszName, pszValue) < 0) {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        } else {

            if (fputs(szBuf, fpDst) == EOF) {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }

        }

    }

    fclose(fpSrc); fpSrc = NULL;
    fclose(fpDst); fpDst = NULL;

    ceError = CTBackupFile(pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTMoveFile(pszTmpPath, pszFilePath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

error:

    if (fpSrc)
        fclose(fpSrc);

    if (fpDst)
        fclose(fpDst);

    regfree(&rx);

    if (bRemoveFile)
        CTRemoveFile(pszTmpPath);

    if (pszTmpPath)
        CTFreeString(pszTmpPath);

    return ceError;
}

static
CENTERROR
DJAppendNameValuePair(
    PSTR pszFilePath,
    PSTR pszName,
    PSTR pszValue
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE* fp = NULL;

    if ((fp = fopen(pszFilePath, "a")) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (fprintf(fp, "\n%s=%s\n", pszName, pszValue) < 0) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fclose(fp); fp = NULL;

error:

    if (fp)
        fclose(fp);

    return ceError;
}

CENTERROR
DJFixDHCPHost(
    PSTR pszPathifcfg,
    PSTR pszComputerName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bPatternExists = FALSE;

    ceError = CTCheckFileHoldsPattern(pszPathifcfg,
                                      "^[[:space:]]*DHCP_HOSTNAME[[:space:]]*=.*$",
                                      &bPatternExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bPatternExists) {

        ceError = DJReplaceNameValuePair(pszPathifcfg,
                                         "DHCP_HOSTNAME",
                                         pszComputerName);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        ceError = DJAppendNameValuePair(pszPathifcfg,
                                        "DHCP_HOSTNAME",
                                        pszComputerName);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

error:

    return ceError;
}

static
CENTERROR
DJFixNetworkManagerOnlineTimeout(
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszFilePath = "/etc/sysconfig/network/config";
    DWORD dwTimeout = 60;
    int EE = 0;
    BOOLEAN bFileExists = FALSE;
    char *isEnabled = NULL;
    char *currentTimeout = NULL;
    char *sedExpression = NULL;
    long currentTimeoutLong;
    char *conversionEnd;

    ceError = CTCheckFileExists(pszFilePath, &bFileExists);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);
    if(!bFileExists)
        goto cleanup;

    ceError = CTShell(". %pszFilePath; echo \"$NETWORKMANAGER\" >%enabled; echo \"$NM_ONLINE_TIMEOUT\" >%timeout",
            CTSHELL_STRING (pszFilePath, pszFilePath),
            CTSHELL_BUFFER (enabled, &isEnabled),
            CTSHELL_BUFFER (timeout, &currentTimeout));
    CLEANUP_ON_CENTERROR_EE(ceError, EE);
    CTStripTrailingWhitespace(isEnabled);
    CTStripTrailingWhitespace(currentTimeout);

    DJ_LOG_VERBOSE("Network manager enabled [%s] network manager timeout [%s]", isEnabled, currentTimeout);

    if(strcasecmp(isEnabled, "yes"))
    {
        DJ_LOG_INFO("Network manager is not enabled");
        goto cleanup;
    }

    currentTimeoutLong = strtol(currentTimeout, &conversionEnd, 10);
    if(*conversionEnd != '\0')
    {
        DJ_LOG_INFO("Unable to convert network manager timeout to long");
        currentTimeoutLong = 0;
    }

    if(currentTimeoutLong < dwTimeout)
    {
        DJ_LOG_INFO("Setting network manager timeout to %d", dwTimeout);
        ceError = CTAllocateStringPrintf(&sedExpression,
                "s/^\\([ \t]*NM_ONLINE_TIMEOUT[ \t]*=[ \t]*\\).*$/\\1%d/",
                dwTimeout);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);
        ceError = CTRunSedOnFile(pszFilePath, pszFilePath, FALSE, sedExpression);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

cleanup:
    DJ_LOG_VERBOSE("DJFixNetworkManagerOnlineTimeout LEAVE -> 0x%08x (EE = %d)", ceError, EE);

    CT_SAFE_FREE_STRING(isEnabled);
    CT_SAFE_FREE_STRING(currentTimeout);
    CT_SAFE_FREE_STRING(sedExpression);

    return ceError;
}

static
CENTERROR
DJRestartDHCPService(
    PSTR pszComputerName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int EE = 0;
    BOOLEAN bFileExists = FALSE;
    PSTR dhcpFilePath = "/etc/sysconfig/network/dhcp";
    PSTR dhcpFilePathNew = "/etc/sysconfig/network/dhcp.new";
    PSTR  ppszArgs[] =
        { "/bin/sed",
          "s/^.*\\(DHCLIENT_SET_HOSTNAME\\).*=.*$/\\1=\\\"no\\\"/",
          dhcpFilePath,
          NULL
        };
    PSTR  ppszNetArgs[] =
        {
#if defined(_AIX)
            "/etc/rc.d/init.d/network",
#else
            "/etc/init.d/network",
#endif
            "restart",
            NULL
        };
    PPROCINFO pProcInfo = NULL;
    LONG status = 0;

    ceError = CTCheckFileExists(dhcpFilePath, &bFileExists);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    if (bFileExists) {

        ceError = CTBackupFile(dhcpFilePath);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);

        ceError = DJSpawnProcessOutputToFile(ppszArgs[0], ppszArgs, dhcpFilePathNew, &pProcInfo);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);

        ceError = DJGetProcessStatus(pProcInfo, &status);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);

        if (status != 0) {
            ceError = CENTERROR_DOMAINJOIN_DHCPRESTART_SET_FAIL;
            CLEANUP_ON_CENTERROR_EE(ceError, EE);
        }

        // Now move temp file into place
        ceError = CTMoveFile(dhcpFilePathNew, dhcpFilePath);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

    if (pProcInfo) {
        FreeProcInfo(pProcInfo);
        pProcInfo = NULL;
    }

    ceError = DJFixNetworkManagerOnlineTimeout();
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    /* Restart network */

    ceError = DJSpawnProcess(ppszNetArgs[0], ppszNetArgs, &pProcInfo);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_DHCPRESTART_FAIL;
        CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

cleanup:

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    DJ_LOG_VERBOSE("DJRestartDHCPService LEAVE -> 0x%08x (EE = %d)", ceError, EE);

    return ceError;
}

static
CENTERROR
DJGetMachineSID(
    PSTR* ppszMachineSID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 0;
    CHAR  szCmd[PATH_MAX+1];
    LONG  status = 0;
    PSTR  pszBuffer = NULL;
    PSTR  pszMachineSID = NULL;
    PSTR  pszTmp = NULL;
    PROCBUFFER procBuffer;
    DWORD iBufIdx = 0;
    DWORD dwBufLen = 0;
    DWORD dwBufAvailable = 0;

    sprintf(szCmd, "%s/bin/lwinet", PREFIXDIR);

    nArgs = 3;

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(szCmd, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("getlocalsid", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(szCmd, ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    do {

        ceError = DJReadData(pProcInfo, &procBuffer);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (procBuffer.dwOutBytesRead) {

            while (1) {

                if (procBuffer.dwOutBytesRead < dwBufAvailable) {

                    memcpy(pszBuffer+iBufIdx,
                           procBuffer.szOutBuf,
                           procBuffer.dwOutBytesRead
                        );

                    iBufIdx+= procBuffer.dwOutBytesRead;
                    dwBufAvailable -= procBuffer.dwOutBytesRead;

                    *(pszBuffer+iBufIdx+1) = '\0';

                    break;

                } else {

                    /*
                     * TODO: Limit the amount of memory acquired
                     */

                    ceError = CTReallocMemory(pszBuffer,
                                              (PVOID*)&pszBuffer,
                                              dwBufLen+1024);
                    BAIL_ON_CENTERIS_ERROR(ceError);

                    dwBufLen += 1024;
                    dwBufAvailable += 1024;
                }
            }
        }
    } while (!procBuffer.bEndOfFile);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_SET_MACHINESID_FAIL;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (pszBuffer == NULL ||
        (pszTmp = strstr(pszBuffer, ": ")) == NULL ||
        *(pszTmp+2) == '\0') {
        ceError = CENTERROR_DOMAINJOIN_NETCONFIGCMD_FAIL;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    CTStripTrailingWhitespace(pszBuffer);

    ceError = CTAllocateString(pszTmp+2, &pszMachineSID);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *ppszMachineSID = pszMachineSID;
    pszMachineSID = NULL;

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pszBuffer)
        CTFreeMemory(pszBuffer);

    if (pszMachineSID)
        CTFreeString(pszMachineSID);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

static
CENTERROR
DJSetMachineSID(
    PSTR pszMachineSID
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 0;
    CHAR  szCmd[PATH_MAX+1];
    LONG  status = 0;

    sprintf(szCmd, "%s/bin/lwinet", PREFIXDIR);

    nArgs = 4;

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(szCmd, ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("setlocalsid", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(pszMachineSID, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(szCmd, ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_SET_MACHINESID_FAIL;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

static
CENTERROR
FixNetworkInterfaces(
    PSTR pszComputerName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int EE = 0;
    BOOLEAN bFileExists = FALSE;
    BOOLEAN bDirExists = FALSE;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 0;
    PSTR* ppszPaths = NULL;
    DWORD nPaths = 0;
    DWORD iPath = 0;
    CHAR szBuf[1024];
    LONG status = 0;
    PSTR pszPathifcfg = NULL;
    BOOLEAN bDHCPHost = FALSE;
    PSTR pszMachineSID = NULL;
    PCSTR networkConfigPath = "/etc/sysconfig/network";

    ceError = DJGetMachineSID(&pszMachineSID);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    /*
     * fixup HOSTNAME variable in /etc/sysconfig/network file if it exists
     * note that 'network' is a *directory* on some dists (ie SUSE),
     * is a *file* on others (ie Redhat). weird.
     */
    ceError = CTCheckFileExists(networkConfigPath, &bFileExists);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    if (bFileExists) {
        sprintf(szBuf, "s/^.*\\(HOSTNAME\\).*=.*$/\\1=%s/", pszComputerName);
        ceError = CTRunSedOnFile(networkConfigPath, networkConfigPath,
                FALSE, szBuf);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

    ceError = CTCheckDirectoryExists("/etc/sysconfig", &bDirExists);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    if (bDirExists) {

        struct
        {
            PCSTR dir;
            PCSTR glob;
        } const searchPaths[] = {
            {"/etc/sysconfig/network", "ifcfg-eth-id-[^.]*$"},
            {"/etc/sysconfig/network", "ifcfg-eth0[^.]*$"},
            {"/etc/sysconfig/network", "ifcfg-eth-bus[^.]*$"},
            //SLES 10.1 on zSeries uses /etc/sysconfig/network/ifcfg-qeth-bus-ccw-0.0.0500
            {"/etc/sysconfig/network", "ifcfg-qeth-bus[^.]*$"},
            // Redhat uses /etc/sysconfig/network-scripts/ifcfg-eth<number>
            {"/etc/sysconfig/network-scripts", "ifcfg-eth[^.]*$"},
            {NULL, NULL}
        };

        // Find the ifcfg file
        pszPathifcfg = NULL;

        for(iPath = 0; searchPaths[iPath].dir != NULL && pszPathifcfg == NULL; iPath++)
        {
            if (ppszPaths)
            {
                CTFreeStringArray(ppszPaths, nPaths);
                ppszPaths = NULL;
            }

            ceError = CTGetMatchingFilePathsInFolder(searchPaths[iPath].dir,
                                                         searchPaths[iPath].glob,
                                                         &ppszPaths,
                                                         &nPaths);
            if(ceError == CENTERROR_INVALID_DIRECTORY)
            {
                ceError = CENTERROR_SUCCESS;
                continue;
            }
            CLEANUP_ON_CENTERROR_EE(ceError, EE);

            if(nPaths > 0)
            {
                ceError = CTAllocateString(ppszPaths[0], &pszPathifcfg);
                CLEANUP_ON_CENTERROR_EE(ceError, EE);
            }
        }

        if (IsNullOrEmptyString(pszPathifcfg)) {
            ceError = CENTERROR_DOMAINJOIN_NO_ETH_ITF_CFG_FILE;
            CLEANUP_ON_CENTERROR_EE(ceError, EE);
        }

        DJ_LOG_INFO("Found ifcfg file at %s", pszPathifcfg);

        ceError = DJCheckIfDHCPHost(pszPathifcfg, &bDHCPHost);
        CLEANUP_ON_CENTERROR_EE(ceError, EE);

        if (bDHCPHost) {
            ceError = DJFixDHCPHost(pszPathifcfg, pszComputerName);
            CLEANUP_ON_CENTERROR_EE(ceError, EE);
        }

        if (pProcInfo) {
            FreeProcInfo(pProcInfo);
            pProcInfo = NULL;
        }

        if (ppszArgs) {
            CTFreeStringArray(ppszArgs, nArgs);
            ppszArgs = NULL;
        }
    }

    nArgs = 3;
    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, PPCAST(&ppszArgs));
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = CTAllocateString("/bin/hostname", ppszArgs);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = CTAllocateString(pszComputerName, ppszArgs+1);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    if (status != 0) {
       ceError = CENTERROR_DOMAINJOIN_HOSTS_EDIT_FAIL;
       CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

    // Only DHCP boxes need to restart their networks
    if (bDHCPHost) {
       ceError = DJRestartDHCPService(pszComputerName);
       CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

cleanup:

    // This ensures that we do not change the SID after a machine name
    // change.  The issue here is that Samba implements its SAM such that
    // a machine name change changes the seeding used for the machine SID.
    // Therefore, we must re-store the old SID with the new machine name
    // seed.
    if (pszMachineSID) {
        if (*pszMachineSID != '\0')
            DJSetMachineSID(pszMachineSID);
        CTFreeString(pszMachineSID);
    }

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (ppszPaths)
        CTFreeStringArray(ppszPaths, nPaths);

    if (pszPathifcfg)
        CTFreeString(pszPathifcfg);

    DJ_LOG_VERBOSE("FixNetworkInterfaces LEAVE -> 0x%08x (EE = %d)", ceError, EE);

    return ceError;
}

static QueryResult QueryDescriptionSetHostname(const JoinProcessOptions *options, PSTR *changeDescription, LWException **exc)
{
    PSTR required = NULL;
    PSTR optional = NULL;
    PSTR both = NULL;
    PSTR newFqdn = NULL;
    PSTR oldShortHostname = NULL;
    PSTR oldFqdnHostname = NULL;
    PSTR newValue;
    PHOSTSFILELINE pHostsFileLineList = NULL;
    BOOLEAN describedFqdn = FALSE;
    QueryResult result = CannotConfigure;
    BOOLEAN modified = FALSE;
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(!options->joiningDomain)
    {
        if(changeDescription != NULL)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(changeDescription,
                "The hostname is fully set"));
        }
        result = NotApplicable;
        goto cleanup;
    }

    //Throws an exception if the hostname is not valid
    LW_TRY(exc, DJCheckValidComputerName(options->computerName, &LW_EXC));

    LW_CLEANUP_CTERR(exc, CTStrdup("", &required));
    LW_CLEANUP_CTERR(exc, CTStrdup("", &optional));
    LW_CLEANUP_CTERR(exc, CTStrdup("", &both));

    LW_CLEANUP_CTERR(exc, DJGetFQDN(&oldShortHostname, &oldFqdnHostname));
    CTStrToLower(oldShortHostname);
    CTStrToLower(oldFqdnHostname);
    if(strcmp(oldShortHostname, options->computerName))
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tSet the computer short hostname (currently %s) to the requested hostname (%s) by setting:\n"
"\t\t* The kernel hostname\n"
"\t\t* The hostname in files for statically configured computers\n"
"\t\t* The hostname in files for DHCP configured computers",
            required, oldShortHostname, options->computerName));
        CT_SAFE_FREE_STRING(required);
        required = newValue;
    }
    
    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newFqdn,
                "%s.%s", options->computerName,
            options->domainName));
    CTStrToLower(newFqdn);
    if(oldFqdnHostname == NULL)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tGive the machine a fully-qualified domain name. If performed automatically, the fqdn will be set through /etc/hosts to '%s', but it is possible to use a different fqdn and/or set it through dns instead of /etc/hosts. However in all cases, the fqdn must follow standard DNS naming conventions, and have a period in the name. The following steps will be used if the fqdn is set automatically:\n"
"\t\t* Make sure local comes before bind in nsswitch\n"
"\t\t* Add a loopback entry in /etc/hosts and put the fqdn as the primary name\n",
            required, newFqdn));
        CT_SAFE_FREE_STRING(required);
        required = newValue;
        describedFqdn = TRUE;
    }
    else if(strcmp(newFqdn, oldFqdnHostname))
    {
        if(strchr(oldFqdnHostname, '.') == NULL)
        {
            //The current fqdn does not match out minimal requirements
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tChange the fqdn from '%s' to '%s'. The current fqdn is invalid because it does not contain a dot in the name. Changing the fqdn could be done via DNS, but this program will change it with the following steps:\n"
"\t\t* Making sure local comes before bind in nsswitch\n"
"\t\t* Adding the fqdn before all entries in /etc/hosts that contain the short hostname and removing the old fqdn if it appears on the line\n"
"\t\t* Restart nscd (if running) to flush the DNS cache",
                required, oldFqdnHostname, newFqdn));
            CT_SAFE_FREE_STRING(required);
            required = newValue;
        }
        else
        {
            //The current fqdn does not match our ideal fqdn
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tChange the fqdn from '%s' to '%s'. This could be done via DNS, but this program will do it with the following steps:\n"
"\t\t* Making sure local comes before bind in nsswitch\n"
"\t\t* Adding the fqdn before all entries in /etc/hosts that contain the short hostname and removing the old fqdn if it appears on the line\n"
"\t\t* Restart nscd (if running) to flush the DNS cache",
                optional, oldFqdnHostname, newFqdn));
            CT_SAFE_FREE_STRING(optional);
            optional = newValue;
        }
        describedFqdn = TRUE;
    }

    //Find out if the fqdn is stored in /etc/hosts
    if(oldFqdnHostname != NULL && !strcmp(oldFqdnHostname, "localhost"))
    {
        CT_SAFE_FREE_STRING(oldFqdnHostname);
    }
    if(oldShortHostname != NULL && !strcmp(oldShortHostname, "localhost"))
    {
        CT_SAFE_FREE_STRING(oldShortHostname);
    }

    LW_CLEANUP_CTERR(exc, DJParseHostsFile("/etc/hosts", &pHostsFileLineList));
    LW_CLEANUP_CTERR(exc, DJReplaceHostnameInMemory(
                pHostsFileLineList,
                oldShortHostname, oldFqdnHostname,
                options->computerName, options->domainName));
    modified |= DJHostsFileWasModified(pHostsFileLineList);
    if (pHostsFileLineList)
    {
        DJFreeHostsFileLineList(pHostsFileLineList);
        pHostsFileLineList = NULL;
    }
    ceError = DJParseHostsFile("/etc/inet/ipnodes", &pHostsFileLineList);
    if(ceError == CENTERROR_INVALID_FILENAME)
    {
        ceError = CENTERROR_SUCCESS;
    }
    else if(CENTERROR_IS_OK(ceError))
    {
        LW_CLEANUP_CTERR(exc, DJReplaceHostnameInMemory(
                    pHostsFileLineList,
                    oldShortHostname, oldFqdnHostname,
                    options->computerName, options->domainName));
        modified |= DJHostsFileWasModified(pHostsFileLineList);
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if (modified && !describedFqdn) {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&newValue,
"%s\n"
"\tSet the fqdn in /etc/hosts and /etc/inet/ipnodes. This will preserve the fqdn even when the DNS server is unreachable. This will be performed with these steps:\n"
"\t\t* Making sure local comes before bind in nsswitch\n"
"\t\t* Adding the fqdn before all entries in /etc/hosts and /etc/inet/ipnodes that contain the short hostname and removing the old fqdn if it appears on the line\n"
"\t\t* Restart nscd (if running) to flush the DNS cache"
        , optional, oldFqdnHostname, newFqdn));
        CT_SAFE_FREE_STRING(optional);
        optional = newValue;
        describedFqdn = TRUE;
    }

    if(strlen(required) > 1)
    {
        if(strlen(optional) > 1)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The following step(s) are required:%s\n\nThe following step(s) are optional (but will performed automatically when this step is run):%s", required, optional));
        }
        else
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The following step(s) are required:%s", required));
        }
        result = NotConfigured;
    }
    else if(strlen(optional) > 1)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The following step(s) are optional:%s", optional));
        result = SufficientlyConfigured;
    }
    else
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&both,
"The hostname is fully set"));
        result = FullyConfigured;
    }

    if(changeDescription != NULL)
    {
        *changeDescription = both;
        both = NULL;
    }

cleanup:
    CT_SAFE_FREE_STRING(required);
    CT_SAFE_FREE_STRING(optional);
    CT_SAFE_FREE_STRING(both);
    CT_SAFE_FREE_STRING(oldShortHostname);
    CT_SAFE_FREE_STRING(oldFqdnHostname);
    CT_SAFE_FREE_STRING(newFqdn);
    if (pHostsFileLineList)
        DJFreeHostsFileLineList(pHostsFileLineList);

    return result;
}

static QueryResult QuerySetHostname(const JoinProcessOptions *options, LWException **exc)
{
    return QueryDescriptionSetHostname(options, NULL, exc);
}

static void DoSetHostname(JoinProcessOptions *options, LWException **exc)
{
    LWException *inner = NULL;
    CENTERROR ceError;

    LW_CLEANUP_CTERR(exc,
            DJSetComputerName(options->computerName, options->domainName));

    ceError = DJConfigureHostsEntry(NULL);
    if(ceError == CENTERROR_INVALID_FILENAME)
    {
        ceError = CENTERROR_SUCCESS;
        DJ_LOG_WARNING("Warning: Could not find nsswitch file");
    }
    LW_CLEANUP_CTERR(exc, ceError);

    DJRestartIfRunning("nscd", &inner);
    if(!LW_IS_OK(inner) && inner->code == CENTERROR_FILE_NOT_FOUND)
        LW_HANDLE(&inner);
    LW_CLEANUP(exc, inner);

cleanup:
    LW_HANDLE(&inner);
}

static PSTR GetSetHostnameDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    QueryDescriptionSetHostname(options, &ret, exc);
    return ret;
}

const JoinModule DJSetHostname = { TRUE, "hostname", "set computer hostname", QuerySetHostname, DoSetHostname, GetSetHostnameDescription };

CENTERROR
DJGetFQDN(
    PSTR *shortName,
    PSTR *fqdn
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR _shortName = NULL;
    PSTR _fqdn = NULL;
    size_t i;
    struct hostent* pHostent = NULL;

    if(shortName != NULL)
        *shortName = NULL;
    if(fqdn != NULL)
        *fqdn = NULL;

    ceError = DJGetComputerName(&_shortName);
    CLEANUP_ON_CENTERROR(ceError);

    //We have the short hostname that the hostname command returns, now we're
    //going to get the long hostname. This is the same as 'hostname -f' on
    //systems which support it.
    //Try to look it up upto 3 times
    for(i = 0; i < 3; i++)
    {
        pHostent = gethostbyname(_shortName);
        if (pHostent == NULL) {
            if (h_errno == TRY_AGAIN) {
                sleep(1);
                continue;
            }
            break;
        }
        ceError = CTAllocateString(pHostent->h_name, &_fqdn);
        CLEANUP_ON_CENTERROR(ceError);
        break;
    }

    if(shortName != NULL)
    {
        *shortName = _shortName;
        _shortName = NULL;
    }
    if(fqdn != NULL)
    {
        *fqdn = _fqdn;
        _fqdn = NULL;
    }

cleanup:
    CT_SAFE_FREE_STRING(_fqdn);
    CT_SAFE_FREE_STRING(_shortName);
    return ceError;
}

CENTERROR
DJSetComputerName(
    PCSTR pszComputerName,
    PCSTR pszDnsDomainName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int EE = 0;
    BOOLEAN bValidComputerName = FALSE;
    PSTR oldShortHostname = NULL;
    PSTR oldFqdnHostname = NULL;
    PSTR pszComputerName_lower = NULL;
    PSTR ppszHostfilePaths[] = { "/etc/hostname", "/etc/HOSTNAME", NULL };

    if (geteuid() != 0) {
       ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
       CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

    ceError = DJIsValidComputerName(pszComputerName, &bValidComputerName);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    if (!bValidComputerName) {
        ceError = CENTERROR_DOMAINJOIN_INVALID_HOSTNAME;
        CLEANUP_ON_CENTERROR_EE(ceError, EE);
    }

    ceError = CTAllocateString(pszComputerName, &pszComputerName_lower);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    CTStrToLower(pszComputerName_lower);

    /* Start spelunking for various hostname holding things. Rather
       than trying to worry about what flavor of linux we are
       running, we look for various files and fix them up if they
       exist. That way we dont end up with a huge wad of repeated
       code for each linux flavor.

       change the repositories of the 'HOSTNAME' variable.
       it's a string in /etc/HOSTNAME for some dists, it's a variable in
       /etc/sysconfig/network for others

       fixup HOSTNAME file if it exists
       Ubuntu/Debian have /etc/hostname, so add that...
    */

    ceError = WriteHostnameToFiles(pszComputerName_lower,
                                   ppszHostfilePaths);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    // insert/correct the new hostname in /etc/hosts - note that this
    // has to be done *before* we update the running hostname because
    // we call hostname to get the current hostname so that we can
    // find it and replace it.
    ceError = DJGetFQDN(&oldShortHostname, &oldFqdnHostname);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    //Don't replace localhost in /etc/hosts, always add our new hostname instead
    if(oldFqdnHostname != NULL && !strcmp(oldFqdnHostname, "localhost"))
    {
        CTFreeString(oldFqdnHostname);
        oldFqdnHostname = NULL;
    }
    if(oldShortHostname != NULL && !strcmp(oldShortHostname, "localhost"))
    {
        CTFreeString(oldShortHostname);
        oldShortHostname = NULL;
    }

    ceError = DJCopyMissingHostsEntry("/etc/inet/ipnodes", "/etc/hosts",
            pszComputerName_lower, oldShortHostname);
    if(ceError == CENTERROR_INVALID_FILENAME)
        ceError = CENTERROR_SUCCESS;
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = DJReplaceNameInHostsFile("/etc/hosts",
            oldShortHostname, oldFqdnHostname,
            pszComputerName_lower, pszDnsDomainName);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

    ceError = DJReplaceNameInHostsFile("/etc/inet/ipnodes",
            oldShortHostname, oldFqdnHostname,
            pszComputerName_lower, pszDnsDomainName);
    if(ceError == CENTERROR_INVALID_FILENAME)
        ceError = CENTERROR_SUCCESS;
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

#if defined(__LWI_SOLARIS__)
    ceError = WriteHostnameToSunFiles(pszComputerName_lower);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

#if defined(_AIX)
    ceError = SetAIXHostname(pszComputerName_lower);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

#if defined(_HPUX_SOURCE)
    ceError = SetHPUXHostname(pszComputerName_lower);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

#if defined(__LWI_MACOSX__)
    ceError = SetMacOsXHostName(pszComputerName_lower);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);
#endif

    ceError = FixNetworkInterfaces(pszComputerName_lower);
    CLEANUP_ON_CENTERROR_EE(ceError, EE);

cleanup:
    CT_SAFE_FREE_STRING(oldShortHostname);
    CT_SAFE_FREE_STRING(oldFqdnHostname);
    CT_SAFE_FREE_STRING(pszComputerName_lower);

    DJ_LOG_VERBOSE("DJSetComputerName LEAVE -> 0x%08x (EE = %d)", ceError, EE);
    return ceError;
}

void DJCheckValidComputerName(
    PCSTR pszComputerName,
    LWException **exc)
{
    size_t dwLen;
    size_t i;

    if (IsNullOrEmptyString(pszComputerName))
    {
        LW_RAISE_EX(exc, CENTERROR_INVALID_COMPUTERNAME, "Invalid hostname", "Hostname is empty");
        goto cleanup;
    }

    dwLen = strlen(pszComputerName);

    //Zero length hostnames are already handled above
    if (dwLen > 15)
    {
        LW_RAISE_EX(exc, CENTERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The name '%s' is %d characters long. Hostnames may only be up to 15 characters long.", pszComputerName, dwLen);
        goto cleanup;
    }

    if (!strcasecmp(pszComputerName, "linux") ||
        !strcasecmp(pszComputerName, "localhost"))
    {
        LW_RAISE_EX(exc, CENTERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The hostname may not be 'linux' or 'localhost'.");
        goto cleanup;
    }

    if (pszComputerName[0] == '-' || pszComputerName[dwLen - 1] == '-')
    {
        LW_RAISE_EX(exc, CENTERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The hostname may not start or end with a hyphen.");
        goto cleanup;
    }

    for(i = 0; i < dwLen; i++)
    {
        char c = pszComputerName[i];
        if (!(c == '-' ||
              (c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9'))) {
            LW_RAISE_EX(exc, CENTERROR_INVALID_COMPUTERNAME, "Invalid hostname", "The given hostname, '%s', contains a '%c'. Valid hostnames may only contain hyphens, letters, and digits.", pszComputerName, c);
            goto cleanup;
        }
    }

cleanup:
    ;
}

CENTERROR
DJIsValidComputerName(
    PCSTR pszComputerName,
    PBOOLEAN pbIsValid
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LWException *exc = NULL;

    *pbIsValid = FALSE;
    DJCheckValidComputerName(pszComputerName, &exc);
    if(LW_IS_OK(exc))
    {
        *pbIsValid = TRUE;
    }
    else
    {
        ceError = exc->code;
        LWHandle(&exc);
    }

    if(ceError == CENTERROR_DOMAINJOIN_INVALID_HOSTNAME)
    {
        ceError = CENTERROR_SUCCESS;
    }
    return ceError;
}

CENTERROR
DJIsDomainNameResolvable(
    PCSTR pszDomainName,
    PBOOLEAN pbIsResolvable
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct hostent* pHostent = NULL;
    int i = 0;

    if (geteuid() != 0) {
       ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pbIsResolvable = FALSE;

    if (IsNullOrEmptyString(pszDomainName)) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    for(i = 0; i < 3; i++){
        pHostent = gethostbyname(pszDomainName);
        if (pHostent == NULL) {
            if (h_errno == TRY_AGAIN) {
                continue;
            } else {
                *pbIsResolvable = FALSE;
                break;
            }
        } else {
            *pbIsResolvable = !IsNullOrEmptyString(pHostent->h_name);
            break;
        }
    }

    return ceError;

error:

    *pbIsResolvable = FALSE;

    return ceError;
}

CENTERROR
DJGetFinalFqdn(
    const JoinProcessOptions *options,
    PSTR *fqdn
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    const ModuleState *state = DJGetModuleStateByName((JoinProcessOptions *)options, "hostname");

    *fqdn = NULL;

    if(state != NULL && state->runModule)
    {
        //The fqdn will be set
        ceError = CTAllocateStringPrintf(fqdn, "%s.%s", options->computerName, options->domainName);
        GCE(ceError);
    }
    else
    {
        //The fqdn will not be set
        GCE(ceError = DJGetFQDN(NULL, fqdn));
    }
cleanup:
    return ceError;
}
