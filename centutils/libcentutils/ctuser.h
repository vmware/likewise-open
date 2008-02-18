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

#ifndef CTUSER_H_
#define CTUSER_H_

CENTERROR
CTVerifyUID(
    uid_t uid
    );

CENTERROR
CTGetLoginId(
    uid_t uid,
    PSTR* ppszLoginId
    );

CENTERROR
CTGetUID(
    PCSTR pszUID,
    uid_t* pUID
    );

CENTERROR
CTGetHomeDirectory(
		uid_t uid,
		PSTR* ppszHomeDir
		);

//This function is not thread safe. Use your own locking before calling this function if you need it to be thread safe.
//This function sets inX to true if a user is logged into X windows.
CENTERROR
CTIsUserInX(BOOLEAN *inX);

#endif /*CTUSER_H_*/
