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

int main(int argc, char *argv[])
{
	CENTERROR ceError = CENTERROR_SUCCESS;

#if defined (_AIX)

	if (argc < 2) {
		printf("Usage: test_login_cfg <config file path>\n");
		exit(1);
	}

	ceError = DJFixLoginConfigFile(argv[1]);
	BAIL_ON_CENTERIS_ERROR(ceError);

#else

	ceError = 1;

	printf("This test is valid only on AIX\n");

#endif

      error:

	if (CENTERROR_IS_OK(ceError))
		printf("Success\n");
	else
		printf("Failed\n");

	return (ceError);
}
