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

#include "ctbase.h"

#include <sys/types.h>
#include <sys/wait.h>

static const DWORD BUFSIZE = 4096;

CENTERROR
CTGetShortDomainName(
    PSTR pszDomainName,
    PSTR * ppszShortDomainName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};

    if (!pszDomainName || !ppszShortDomainName) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf(szCommand,
            "%s/bin/lwiinfo -D %s | grep \"^Name\" | awk '{print $3}'",
            PREFIXDIR,
            pszDomainName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    if(child == 0)
    {
        /* The child */
        close(filedes[0]);
        dup2(filedes[1], 1);
        close(filedes[1]);
        execl("/bin/sh", "sh", "-c", szCommand, (char *)0);
        /* This exit call should not be reached */
        exit(-1);
    }

    /* The parent */
    close(filedes[1]);
    filedes[1] = -1;
    pFile = fdopen(filedes[0], "r");
    if (pFile == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        CTStripWhitespace(szBuf);
        ceError = CTAllocateString(szBuf, ppszShortDomainName);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pFile != NULL) {
        pclose(pFile);
    }
    if (filedes[0] != -1)
        close(filedes[0]);
    if (filedes[1] != -1)
        close(filedes[1]);
    if (child != 0)
    {
        int status;
        waitpid(child, &status, 0);
    }

    return ceError;
}

CENTERROR
CTGetFullDomainName(
    PCSTR pszShortDomainName,
    PSTR* ppszDomainFQDN
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];
    FILE * pFile = NULL;
    char  szBuf[BUFSIZE+1];
    pid_t child = 0;
    int filedes[2] = {-1, -1};

    if (!pszShortDomainName || !ppszDomainFQDN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf(szCommand,
            "%s/bin/lwiinfo -D %s | grep \"^Alt_Name\" | awk '{print $3}'",
            PREFIXDIR,
            pszShortDomainName);

    /* popen fails for some reason here on Solaris. Instead we
       emulate it. */
    if (pipe(filedes) != 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    child = fork();
    if(child == (pid_t)-1)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    if(child == 0)
    {
        /* The child */
        close(filedes[0]);
        dup2(filedes[1], 1);
        close(filedes[1]);
        execl("/bin/sh", "sh", "-c", szCommand, (char *)0);
        /* This exit call should not be reached */
        exit(-1);
    }

    /* The parent */
    close(filedes[1]);
    filedes[1] = -1;
    pFile = fdopen(filedes[0], "r");
    if (pFile == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    /* pFile now owns the descriptor */
    filedes[0] = -1;

    if (!feof(pFile) && (NULL != fgets(szBuf, BUFSIZE, pFile))) {
        CTStripWhitespace(szBuf);
        ceError = CTAllocateString(szBuf, ppszDomainFQDN);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    else
    {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (pFile != NULL) {
        pclose(pFile);
    }
    if (filedes[0] != -1)
        close(filedes[0]);
    if (filedes[1] != -1)
        close(filedes[1]);
    if (child != 0)
    {
        int status;
        waitpid(child, &status, 0);
    }

    return ceError;
}

CENTERROR
CTGetDomainFQDN(
		PCSTR pszShortDomainName,
		PSTR* ppszDomainFQDN
		)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szCommand[PATH_MAX + 1];
    char* newline;
    FILE * pFile = NULL;
    PSTR pszAllUpperDomainName = NULL;

    if (!pszShortDomainName || !ppszDomainFQDN) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateString(pszShortDomainName, &pszAllUpperDomainName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    CTStrToUpper(pszAllUpperDomainName);

    sprintf(szCommand,
            "cat /var/lib/lwidentity/smb_krb5/krb5.conf.%s | grep default_realm | awk '{print $3}'",
            pszAllUpperDomainName);

    ceError = CTCaptureOutput(szCommand, ppszDomainFQDN);
    BAIL_ON_CENTERIS_ERROR(ceError);

    newline = strchr(*ppszDomainFQDN, '\n');
    
    if (newline)
        *newline = 0;
    
    if (*ppszDomainFQDN)
    	CTStrToLower(*ppszDomainFQDN);

error:

    if (pFile != NULL) {
        pclose(pFile);
    }

    CT_SAFE_FREE_STRING(pszAllUpperDomainName);

    return ceError;	
}

CENTERROR
CTGetPreferredDCAddress(
    PSTR pszShortDomainName,
    PSTR * ppszDCAddress
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char szPath[PATH_MAX + 1];
    char szCommand[PATH_MAX + 1];
    char* newline;
    FILE * pFile = NULL;
    BOOLEAN fExists = FALSE;
    PSTR pszAddress = NULL;

    if (!pszShortDomainName || !ppszDCAddress) {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    strcpy(szPath, "/var/lib/lwidentity/smb_krb5");
    ceError = CTCheckDirectoryExists(szPath, &fExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!fExists) {
        ceError = CENTERROR_GP_NO_SMB_KRB5_INFO;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    strcat(szPath, "/krb5.conf.");
    strcat(szPath, pszShortDomainName);

    ceError = CTCheckFileExists(szPath, &fExists);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!fExists) {
        ceError = CENTERROR_GP_NO_SMB_KRB5_SITE_INFO;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    sprintf(szCommand,
            "cat %s | grep \"kdc\" | awk '{print $3}'",
            szPath);

    ceError = CTCaptureOutput(szCommand, &pszAddress);
    BAIL_ON_CENTERIS_ERROR(ceError);

    newline = strchr(pszAddress, '\n');
    
    if (newline)
        *newline = 0;

    /* What is 7? strlen("w.x.y.z"); a min length IP address. */
    if ( pszAddress && strlen(pszAddress) >= 7 ) {
        *ppszDCAddress = pszAddress;
        pszAddress = NULL;
    } else {
        ceError = CENTERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    CT_SAFE_FREE_STRING(pszAddress);

    if (pFile != NULL) {
        pclose(pFile);
    }

    return ceError;
}

