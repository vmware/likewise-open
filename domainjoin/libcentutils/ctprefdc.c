/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007.  
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ctbase.h"

#include <sys/types.h>
#include <sys/wait.h>

static const DWORD BUFSIZE = 4096;

CENTERROR CTGetShortDomainName(PSTR pszDomainName, PSTR * ppszShortDomainName)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	char szCommand[PATH_MAX + 1];
	FILE *pFile = NULL;
	char szBuf[BUFSIZE + 1];
	pid_t child = 0;
	int filedes[2] = { -1, -1 };

	if (!pszDomainName || !ppszShortDomainName) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	sprintf(szCommand,
		"%s/bin/lwiinfo -D %s | grep \"^Name\" | awk '{print $3}'",
		PREFIXDIR, pszDomainName);

	/* popen fails for some reason here on Solaris. Instead we
	   emulate it. */
	if (pipe(filedes) != 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	child = fork();
	if (child == (pid_t) - 1) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	if (child == 0) {
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
	} else {
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
	if (child != 0) {
		int status;
		waitpid(child, &status, 0);
	}

	return ceError;
}

CENTERROR CTGetPreferredDCAddress(PSTR pszShortDomainName, PSTR * ppszDCAddress)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	char szCommand[PATH_MAX + 1];
	char *newline;
	FILE *pFile = NULL;

	if (!pszShortDomainName || !ppszDCAddress) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	sprintf(szCommand,
		"cat /var/lib/lwidentity/smb_krb5/krb5.conf.%s | grep \"kdc\" | awk '{print $3}'",
		pszShortDomainName);

	ceError = CTCaptureOutput(szCommand, ppszDCAddress);
	BAIL_ON_CENTERIS_ERROR(ceError);

	newline = strchr(*ppszDCAddress, '\n');

	if (newline)
		*newline = 0;

      error:

	if (pFile != NULL) {
		pclose(pFile);
	}

	return ceError;
}
