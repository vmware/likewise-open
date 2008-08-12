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
#include "djdaemonmgr.h"
#include <libxml/xpath.h>
#include <libxml/parser.h>

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

// aka: CENTERROR_LICENSE_INCORRECT
// static DWORD GPAGENT_LICENSE_ERROR = 0x00002001;

// CENTERROR_LICENSE_EXPIRED
// static DWORD GPAGENT_LICENSE_EXPIRED_ERROR = 0x00002002;

// static PSTR pszAuthdStartPriority = "90";
// static PSTR pszAuthdStopPriority = "10";
// static PSTR pszGPAgentdStartPriority = "91";
// static PSTR pszGPAgentdStopPriority = "9";

// Runs an xpath expression on an xml file. If the resultant nodeset contains
// exactly one text node, it is returned through result, otherwise
// CENTERROR_CFG_VALUE_NOT_FOUND or CENTERROR_INVALID_VALUE is returned.
static CENTERROR GetXPathString(PCSTR file, PSTR *result, PCSTR expression)
{
    xmlDocPtr xmlDoc = NULL;
    xmlXPathContextPtr xpathCtx = NULL; 
    xmlXPathObjectPtr xpathObj = NULL; 
    CENTERROR ceError = CENTERROR_SUCCESS;

    *result = NULL;

    xmlDoc = xmlReadFile(file, NULL, XML_PARSE_NONET | XML_PARSE_NOERROR);
    if(xmlDoc == NULL)
        GCE(ceError = CENTERROR_DOMAINJOIN_INVALID_FORMAT);

    xpathCtx = xmlXPathNewContext(xmlDoc);
    if(xpathCtx == NULL)
        GCE(ceError = CENTERROR_OUT_OF_MEMORY);

    xpathObj = xmlXPathEvalExpression((xmlChar*)expression, xpathCtx);

    if(xpathObj->type != XPATH_NODESET)
        GCE(ceError = CENTERROR_INVALID_VALUE);
    if(xpathObj->nodesetval->nodeNr < 1)
        GCE(ceError = CENTERROR_CFG_VALUE_NOT_FOUND);
    if(xpathObj->nodesetval->nodeNr > 1 ||
            xpathObj->nodesetval->nodeTab[0]->type != XML_TEXT_NODE)
    {
        GCE(ceError = CENTERROR_INVALID_VALUE);
    }
    GCE(ceError = CTStrdup((PCSTR)xpathObj->nodesetval->nodeTab[0]->content, result));

cleanup:
    if(xpathObj != NULL)
        xmlXPathFreeObject(xpathObj);
    if(xpathCtx != NULL)
        xmlXPathFreeContext(xpathCtx);
    if(xmlDoc != NULL)
        xmlFreeDoc(xmlDoc);
    return ceError;
}

static CENTERROR DJDaemonLabelToConfigFile(PSTR *configFile, PCSTR dirName, PCSTR label)
{
    DIR *dir = NULL;
    PSTR filePath = NULL;
    struct dirent *dirEntry = NULL;
    PSTR fileLabel = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;

    *configFile = NULL;

    if ((dir = opendir(dirName)) == NULL) {
        GCE(ceError = CTMapSystemError(errno));
    }

    while(1)
    {
        errno = 0;
        dirEntry = readdir(dir);
        if(dirEntry == NULL)
        {
            if(errno != 0)
                GCE(ceError = CTMapSystemError(errno));
            else
            {
                //No error here. We simply read the last entry
                break;
            }
        }
        if(dirEntry->d_name[0] == '.')
            continue;

        CT_SAFE_FREE_STRING(filePath);
        GCE(ceError = CTAllocateStringPrintf(&filePath, "%s/%s",
                    dirName, dirEntry->d_name));

        CT_SAFE_FREE_STRING(fileLabel);
        ceError = GetXPathString(filePath, &fileLabel,
            "/plist/dict/key[text()='Label']/following-sibling::string[position()=1]/text()");
        if(!CENTERROR_IS_OK(ceError))
        {
            DJ_LOG_INFO("Cannot read daemon label from '%s' file. Error code %X. Ignoring it.", filePath, ceError);
            ceError = CENTERROR_SUCCESS;
            continue;
        }
        if(!strcmp(fileLabel, label))
        {
            //This is a match
            *configFile = filePath;
            filePath = NULL;
            goto cleanup;
        }
    }

    GCE(ceError = CENTERROR_DOMAINJOIN_MISSING_DAEMON);

cleanup:
    CT_SAFE_FREE_STRING(fileLabel);
    CT_SAFE_FREE_STRING(filePath);
    if(dir != NULL)
    {
        closedir(dir);
    }
    return ceError;
}

void
DJGetDaemonStatus(
    PCSTR pszDaemonPath,
    PBOOLEAN pbStarted,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 7;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    CHAR  szBuf[1024+1];
    FILE* fp = NULL;
    CENTERROR ceError;
    PSTR configFile = NULL;
    PSTR command = NULL;
    int argNum = 0;
    PSTR whitePos;

    /* Translate the Unix daemon names into the mac daemon names */
    if(!strcmp(pszDaemonPath, "centeris.com-gpagentd"))
        pszDaemonPath = "com.centeris.gpagentd";
    else if(!strcmp(pszDaemonPath, "likewise-open"))
        pszDaemonPath = "com.likewise.open";

    /* Find the .plist file for the daemon */
    ceError = DJDaemonLabelToConfigFile(&configFile, "/System/Library/LaunchDaemons", pszDaemonPath);
    if(ceError == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
        ceError = DJDaemonLabelToConfigFile(&configFile, "/etc/centeris/LaunchDaemons", pszDaemonPath);
    if(ceError == CENTERROR_DOMAINJOIN_MISSING_DAEMON)
    {
        DJ_LOG_ERROR("Checking status of daemon [%s] failed: Missing", pszDaemonPath);
        LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_MISSING_DAEMON, "Unable to find daemon plist file", "The plist file for the '%s' daemon could not be found in /System/Library/LaunchDaemons or /etc/centeris/LaunchDaemons .", pszDaemonPath);
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    DJ_LOG_INFO("Found config file [%s] for daemon [%s]", configFile, pszDaemonPath);

    /* Figure out the daemon binary by reading the command from the plist file
     */
    LW_CLEANUP_CTERR(exc, GetXPathString(configFile, &command,
        "/plist/dict/key[text()='ProgramArguments']/following-sibling::array[position()=1]/string[position()=1]/text()"));

    if(!strcmp(command, "/opt/centeris/sbin/winbindd-wrap"))
    {
        CT_SAFE_FREE_STRING(command);
        LW_CLEANUP_CTERR(exc, CTStrdup("/opt/centeris/sbin/likewise-winbindd", &command));
    }

    DJ_LOG_INFO("Found daemon binary [%s] for daemon [%s]", command, pszDaemonPath);

    DJ_LOG_INFO("Checking status of daemon [%s]", pszDaemonPath);

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));

    LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/ps", ppszArgs + argNum++));

    LW_CLEANUP_CTERR(exc, CTAllocateString("-U", ppszArgs + argNum++));

    LW_CLEANUP_CTERR(exc, CTAllocateString("root", ppszArgs + argNum++));

    LW_CLEANUP_CTERR(exc, CTAllocateString("-o", ppszArgs + argNum++));

    LW_CLEANUP_CTERR(exc, CTAllocateString("command=", ppszArgs + argNum++));

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));

    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    *pbStarted = FALSE;
    if (!status) {

        fp = fdopen(pProcInfo->fdout, "r");
        if (!fp) {
            LW_CLEANUP_CTERR(exc, CTMapSystemError(errno));
        }

        while (1) {

            if (fgets(szBuf, 1024, fp) == NULL) {
                if (!feof(fp)) {
                    LW_CLEANUP_CTERR(exc, CTMapSystemError(errno));
                }
                else
                    break;
            }

            CTStripWhitespace(szBuf);

            if (IsNullOrEmptyString(szBuf))
                continue;

            whitePos = strchr(szBuf, ' ');
            if(whitePos != NULL)
                *whitePos = '\0';

            if (!strcmp(szBuf, command)) {
                *pbStarted = TRUE;
                break;
            }
        }
    }

cleanup:

    if (fp)
        fclose(fp);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    CT_SAFE_FREE_STRING(configFile);
    CT_SAFE_FREE_STRING(command);
}

void
DJStartStopDaemon(
    PCSTR pszDaemonPath,
    BOOLEAN bStatus,
    PSTR pszPreCommand,
    LWException **exc
    )
{
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN bStarted = FALSE;
    int count;

    if (bStatus) {
        DJ_LOG_INFO("Starting daemon [%s]", pszDaemonPath);
    } else {
        DJ_LOG_INFO("Stopping daemon [%s]", pszDaemonPath);
    }

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs));
    LW_CLEANUP_CTERR(exc, CTAllocateString("/bin/launchctl", ppszArgs));

    if (bStatus)
    {
       LW_CLEANUP_CTERR(exc, CTAllocateString("start", ppszArgs+1));
       LW_CLEANUP_CTERR(exc, CTAllocateString(pszDaemonPath, ppszArgs+2));
    }
    else
    {
       LW_CLEANUP_CTERR(exc, CTAllocateString("unload", ppszArgs+1));
       LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(ppszArgs+2, "/System/Library/LaunchDaemons/%s.plist", pszDaemonPath));
    }

    LW_CLEANUP_CTERR(exc, DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo));
    LW_CLEANUP_CTERR(exc, DJGetProcessStatus(pProcInfo, &status));

    for (count = 0; count < 5; count++) {

	LW_TRY(exc, DJGetDaemonStatus(pszDaemonPath, &bStarted, &LW_EXC));

        if (bStarted == bStatus) {
            break;
        }

        sleep(1);
    }

    if (bStarted != bStatus) {

        if(bStatus)
        {
            LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INCORRECT_STATUS, "Unable to start daemon", "An attempt was made to start the '%s' daemon, but querying its status revealed that it did not start.", pszDaemonPath);
        }
        else
        {
            LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_INCORRECT_STATUS, "Unable to start daemon", "An attempt was made to stop the '%s' daemon, but querying its status revealed that it did not stop.", pszDaemonPath);
        }
        goto cleanup;
    }

cleanup:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);
}

CENTERROR
DJConfigureForDaemonRestart(
    PCSTR pszDaemonName,
    BOOLEAN bStatus,
    PSTR pszStartPriority,
    PSTR pszStopPriority
    )
{
    return CENTERROR_SUCCESS;
}

static
CENTERROR
DJExistsInLaunchCTL(
    PCSTR pszName,
    PBOOLEAN pbExists
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    LONG  status = 0;
    DWORD nArgs = 3;
    FILE* fp = NULL;
    CHAR szBuf[1024+1];
    BOOLEAN bExists = FALSE;

    ceError = CTAllocateMemory(nArgs*sizeof(PSTR), (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/launchctl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("list", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_LISTSVCS_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fp = fdopen(pProcInfo->fdout, "r");
    if (fp == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    for(;;)
    {
        if (fgets(szBuf, 1024, fp) == NULL)
        {
            if (feof(fp))
            {
                break;
            }
            else
            {
                ceError = CTMapSystemError(errno);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }

        CTStripWhitespace(szBuf);

        if (!strcmp(szBuf, pszName))
        {
            bExists = TRUE;
            break;
        }
    }

error:

    *pbExists = bExists;

    if (fp)
        fclose(fp);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    return ceError;
}

static
CENTERROR
DJPrepareServiceLaunchScript(
    PCSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    LONG  status = 0;
    DWORD nArgs = 5;
    CHAR szBuf[1024+1];
    BOOLEAN bFileExists = FALSE;

    ceError = CTAllocateMemory(nArgs*sizeof(PSTR), (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/cp", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("-f", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "/etc/centeris/LaunchDaemons/%s.plist", pszName);
    ceError = CTCheckFileExists(szBuf, &bFileExists);  
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists)
	BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_DOMAINJOIN_MISSING_DAEMON);

    ceError = CTAllocateString(szBuf, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);
    ceError = CTAllocateString(szBuf, ppszArgs+3);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_PREPSVC_FAILED;
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
DJRemoveFromLaunchCTL(
    PCSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExistsInLaunchCTL = FALSE;
    CHAR    szBuf[1024+1];
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG  status = 0;

    ceError = DJExistsInLaunchCTL(pszName, &bExistsInLaunchCTL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExistsInLaunchCTL) {

        sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);

        ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("/bin/launchctl", ppszArgs);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString("unload", ppszArgs+1);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = CTAllocateString(szBuf, ppszArgs+2);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
        BAIL_ON_CENTERIS_ERROR(ceError);

        ceError = DJGetProcessStatus(pProcInfo, &status);
        BAIL_ON_CENTERIS_ERROR(ceError);

        if (status != 0) {
            ceError = CENTERROR_DOMAINJOIN_SVC_UNLOAD_FAILED;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);
        ceError = CTRemoveFile(szBuf);
        if (!CENTERROR_IS_OK(ceError)) {
            DJ_LOG_WARNING("Failed to remove file [%s]", szBuf);
            ceError = CENTERROR_SUCCESS;
        }
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
DJAddToLaunchCTL(
    PCSTR pszName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bExistsInLaunchCTL = FALSE;
    BOOLEAN bFileExists = FALSE;
    CHAR    szBuf[1024+1];
    PPROCINFO pProcInfo = NULL;
    PSTR* ppszArgs = NULL;
    DWORD nArgs = 4;
    LONG  status = 0;

    ceError = DJExistsInLaunchCTL(pszName, &bExistsInLaunchCTL);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (bExistsInLaunchCTL) {

        ceError = DJRemoveFromLaunchCTL(pszName);
        BAIL_ON_CENTERIS_ERROR(ceError);

        bExistsInLaunchCTL = FALSE;
    }

    ceError = DJPrepareServiceLaunchScript(pszName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);

    ceError = CTCheckFileExists(szBuf, &bFileExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!bFileExists) {
        ceError = CENTERROR_DOMAINJOIN_MISSING_DAEMON;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf(szBuf, "/System/Library/LaunchDaemons/%s.plist", pszName);

    ceError = CTAllocateMemory(sizeof(PSTR)*nArgs, (PVOID*)&ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("/bin/launchctl", ppszArgs);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString("load", ppszArgs+1);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTAllocateString(szBuf, ppszArgs+2);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJSpawnProcess(ppszArgs[0], ppszArgs, &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        ceError = CENTERROR_DOMAINJOIN_SVC_LOAD_FAILED;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (ppszArgs)
        CTFreeStringArray(ppszArgs, nArgs);

    if (pProcInfo)
        FreeProcInfo(pProcInfo);

    return ceError;
}

void
DJManageDaemon(
    PCSTR pszName,
    BOOLEAN bStatus,
    PSTR pszPreCommand,
    PSTR pszStartPriority,
    PSTR pszStopPriority,
    LWException **exc
    )
{
    BOOLEAN bStarted = FALSE;

    if (bStatus)
    {
        LW_CLEANUP_CTERR(exc, DJAddToLaunchCTL(pszName));
    }

    // check our current state prior to doing anything.  notice that
    // we are using the private version so that if we fail, our inner
    // exception will be the one that was tossed due to the failure.
    LW_TRY(exc, DJGetDaemonStatus(pszName, &bStarted, &LW_EXC));

    // if we got this far, we have validated the existence of the
    // daemon and we have figured out if its started or stopped

    // if we are already in the desired state, do nothing.
    if (bStarted != bStatus) {

        LW_TRY(exc, DJStartStopDaemon(pszName, bStatus, pszPreCommand, &LW_EXC));

    }
    else
    {
        DJ_LOG_INFO("daemon '%s' is already %s", pszName, bStarted ? "started" : "stopped");
    }

    LW_CLEANUP_CTERR(exc, DJConfigureForDaemonRestart(pszName, bStatus, pszStartPriority, pszStopPriority));

cleanup:
    ;
}

struct _DaemonList daemonList[] = {
    { "com.likewise.open", {NULL}, TRUE, 92, 10 },
    { "com.centeris.gpagentd", {NULL}, FALSE, 92, 9 },
    { NULL, {NULL}, FALSE, 0, 0 }
};
