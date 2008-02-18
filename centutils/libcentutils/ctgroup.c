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

CENTERROR
CTVerifyGID(
    gid_t gid
    )
{
  CENTERROR ceError = CENTERROR_SUCCESS;
  CHAR szBuf[1024];
  struct group group;
  struct group* pResult = NULL;
  
#if defined(__LWI_SOLARIS__)
  if ((pResult = getgrgid_r(gid, &group, szBuf, sizeof(szBuf))) == NULL) {
    ceError = CENTERROR_INVALID_GID;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }
#else
  if (getgrgid_r(gid, &group, szBuf, sizeof(szBuf), &pResult) < 0) {
    ceError = CTMapSystemError(errno);
    BAIL_ON_CENTERIS_ERROR(ceError);
  }
#endif

  if (!pResult) {
     ceError = CENTERROR_INVALID_GID;
     BAIL_ON_CENTERIS_ERROR(ceError);
  }
  
 error:
  
  return ceError;
}

CENTERROR
CTGetGID(
    PCSTR pszGID,
    gid_t* pGID
    )
{
  CENTERROR ceError = CENTERROR_SUCCESS;

  if (IsNullOrEmptyString(pszGID)) {
    ceError = CENTERROR_INVALID_GID;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }

  if (CTIsAllDigit(pszGID)) {

    gid_t gid = atoi(pszGID);

    ceError = CTVerifyGID(gid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pGID = gid;

  }
  else {

    CHAR szBuf[1024];
    struct group group;
    struct group* pResult = NULL;

    memset(&group, 0, sizeof(struct group));

#if defined(__LWI_SOLARIS__)
    if ((pResult = getgrnam_r(pszGID, &group, szBuf, sizeof(szBuf))) == NULL) {
      ceError = CENTERROR_INVALID_GID;
      BAIL_ON_CENTERIS_ERROR(ceError);
    }
#else
    if (getgrnam_r(pszGID, &group, szBuf, sizeof(szBuf), &pResult) < 0) {
      ceError = CTMapSystemError(errno);
      BAIL_ON_CENTERIS_ERROR(ceError);
    }
#endif
    
    if (!pResult) {
       ceError = CENTERROR_INVALID_GID;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    *pGID = group.gr_gid;

  }

 error:
   
  return ceError;
}
