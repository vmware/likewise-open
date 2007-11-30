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

#include "domainjoin.h"

CENTERROR DJGetErrorMessage(CENTERROR errCode, PSTR * ppszMsg)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszErrMsg = NULL;

	switch (errCode) {

	case CENTERROR_DOMAINJOIN_INVALID_HOSTNAME:
		ceError = CTAllocateString("An invalid hostname was specified",
					   &pszErrMsg);
		break;

	case CENTERROR_DOMAINJOIN_GPAGENTD_INCOMMUNICADO:
		ceError =
		    CTAllocateString
		    ("Can not talk to Likewise Identity group policy daemon.",
		     &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;

	case CENTERROR_DOMAINJOIN_INVALID_USERID:
		ceError = CTAllocateString("Please provide a valid user id.",
					   &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;

	case CENTERROR_DOMAINJOIN_INVALID_PASSWORD:
		ceError = CTAllocateString("Please provide a valid password.",
					   &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;

	case CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME:
		ceError =
		    CTAllocateString("Please provide a valid domain name.",
				     &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;

	case CENTERROR_DOMAINJOIN_BAD_LICENSE_KEY:
		ceError =
		    CTAllocateString("The license key provided is invalid.",
				     &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;

	case CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME:
		ceError =
		    CTAllocateString("The domain name could not be resolved.",
				     &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;

	case CENTERROR_DOMAINJOIN_INVALID_LOG_LEVEL:
		ceError =
		    CTAllocateString("An invalid log level was specified.",
				     &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;

	case CENTERROR_DOMAINJOIN_INVALID_OU:
		ceError =
		    CTAllocateString("An invalid OU argument was specified.",
				     &pszErrMsg);
		BAIL_ON_CENTERIS_ERROR(ceError);

		break;
	}

	*ppszMsg = pszErrMsg;

	return ceError;

      error:

	if (pszErrMsg)
		CTFreeString(pszErrMsg);

	return ceError;
}
