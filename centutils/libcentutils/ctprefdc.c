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

#include "ctbase.h"
#include "ctshell.h"

#include <sys/types.h>
#include <sys/wait.h>

CENTERROR
CTGetShortDomainName(
    PCSTR pszDomainName,
    PSTR * ppszShortDomainName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszOut = NULL;

    if (!pszDomainName || !ppszShortDomainName)
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTShell(PREFIXDIR "/bin/lwiinfo -D %domain | grep \"^Name\" | awk '{print $3}' >%out",
                      CTSHELL_STRING(domain, pszDomainName),
                      CTSHELL_BUFFER(out, &pszOut));
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!pszOut)
    {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
    CTStripWhitespace(pszOut);
    if (!*pszOut)
    {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (!CENTERROR_IS_OK(ceError))
    {
        CT_SAFE_FREE_STRING(pszOut);
    }
    *ppszShortDomainName = pszOut;

    return ceError;
}

CENTERROR
CTGetFullDomainName(
    PCSTR pszShortDomainName,
    PSTR* ppszDomainFQDN
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszOut = NULL;

    if (!pszShortDomainName || !ppszDomainFQDN)
    {
        ceError = CENTERROR_INVALID_PARAMETER;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTShell(PREFIXDIR "/bin/lwiinfo -D %domain | grep \"^Alt_Name\" | awk '{print $3}' >%out",
                      CTSHELL_STRING(domain, pszShortDomainName),
                      CTSHELL_BUFFER(out, &pszOut));
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (!pszOut)
    {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
 
    CTStripWhitespace(pszOut);
    if (!*pszOut)
    {
        ceError = CENTERROR_NO_SUCH_DOMAIN;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:

    if (!CENTERROR_IS_OK(ceError))
    {
        CT_SAFE_FREE_STRING(pszOut);
    }
    *ppszDomainFQDN = pszOut;

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
    PCSTR pszShortDomainName,
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

