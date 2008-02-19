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
#include "config/config.h"
#if HAVE_UTMPS_H
#include <utmps.h>
#elif HAVE_UTMPX_H
#include <utmpx.h>
#elif HAVE_UTMP_H
#include <utmpx.h>
#endif

CENTERROR
CTVerifyUID(
    uid_t uid
    )
{
  CENTERROR ceError = CENTERROR_SUCCESS;
  CHAR szBuf[1024];
  struct passwd user;
  struct passwd* pResult = NULL;
  
  memset(&user, 0, sizeof(struct passwd));

#if defined(__LWI_SOLARIS__)
  if ((pResult = getpwuid_r(uid, &user, szBuf, sizeof(szBuf))) == NULL) {
    ceError = CENTERROR_INVALID_UID;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }
#else
  if (getpwuid_r(uid, &user, szBuf, sizeof(szBuf), &pResult) < 0) {
    ceError = CTMapSystemError(errno);
    BAIL_ON_CENTERIS_ERROR(ceError);
  }
#endif

  if (!pResult) {
     ceError = CENTERROR_INVALID_UID;
     BAIL_ON_CENTERIS_ERROR(ceError);
  }
  
 error:

  return ceError;
}

CENTERROR
CTGetLoginId(
    uid_t uid,
    PSTR* ppszLoginId
    )
{
  CENTERROR ceError = CENTERROR_SUCCESS;
  CHAR szBuf[1024];
  struct passwd user;
  struct passwd* pResult = NULL;
  
  memset(&user, 0, sizeof(struct passwd));

#if defined(__LWI_SOLARIS__)
  if ((pResult = getpwuid_r(uid, &user, szBuf, sizeof(szBuf))) == NULL) {
    ceError = CENTERROR_INVALID_UID;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }
#else
  if (getpwuid_r(uid, &user, szBuf, sizeof(szBuf), &pResult) < 0) {
    ceError = CTMapSystemError(errno);
    BAIL_ON_CENTERIS_ERROR(ceError);
  }
#endif

  if (!pResult) {
     ceError = CENTERROR_INVALID_UID;
     BAIL_ON_CENTERIS_ERROR(ceError);
  }

  ceError = CTAllocateString(user.pw_name, ppszLoginId);
  BAIL_ON_CENTERIS_ERROR(ceError);

  return ceError;

error:

  *ppszLoginId = NULL;

  return ceError;
}

CENTERROR
CTGetUID(
    PCSTR pszUID,
    uid_t* pUID
    )
{
  CENTERROR ceError = CENTERROR_SUCCESS;

  if (IsNullOrEmptyString(pszUID)) {
    ceError = CENTERROR_INVALID_UID;
    BAIL_ON_CENTERIS_ERROR(ceError);
  }

  if (CTIsAllDigit(pszUID)) {
    uid_t uid = atoi(pszUID);

    ceError = CTVerifyUID(uid);
    BAIL_ON_CENTERIS_ERROR(ceError);

    *pUID = uid;
  }
  else {
    CHAR szBuf[1024];
    struct passwd user;
    struct passwd* pResult = NULL;

    memset(&user, 0, sizeof(struct passwd));

#if defined(__LWI_SOLARIS__)
    if ((pResult = getpwnam_r(pszUID, &user, szBuf, sizeof(szBuf))) == NULL) {
      ceError = CENTERROR_INVALID_UID;
      BAIL_ON_CENTERIS_ERROR(ceError);
    }
#else
    if (getpwnam_r(pszUID, &user, szBuf, sizeof(szBuf), &pResult) < 0) {
      ceError = CTMapSystemError(errno);
      BAIL_ON_CENTERIS_ERROR(ceError);
    }
#endif

    if (!pResult) {
       ceError = CENTERROR_INVALID_UID;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }
    
    *pUID = user.pw_uid;
  }

 error:

  return ceError;
}

CENTERROR
CTGetHomeDirectory(
		uid_t uid,
		PSTR* ppszHomeDir
		)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszHomeDir = NULL;
	CHAR szBuf[1024];
	struct passwd user;
	struct passwd* pResult = NULL;

        memset(&user, 0, sizeof(struct passwd));
	  
	#if defined(__LWI_SOLARIS__)
	  if ((pResult = getpwuid_r(uid, &user, szBuf, sizeof(szBuf))) == NULL) {
	     ceError = CENTERROR_INVALID_UID;
	     BAIL_ON_CENTERIS_ERROR(ceError);
	  }
	#else
	  if (getpwuid_r(uid, &user, szBuf, sizeof(szBuf), &pResult) < 0) {
	     ceError = CTMapSystemError(errno);
	     BAIL_ON_CENTERIS_ERROR(ceError);
	  }
	#endif

	  if (!pResult) {
	     ceError = CENTERROR_INVALID_UID;
	     BAIL_ON_CENTERIS_ERROR(ceError);
	  }
	  
	  ceError = CTAllocateString(pResult->pw_dir, &pszHomeDir);
	  BAIL_ON_CENTERIS_ERROR(ceError);
	  
	  *ppszHomeDir = pszHomeDir;
	  pszHomeDir = NULL;
	  
error:
	 
	 CT_SAFE_FREE_STRING(pszHomeDir);

	 return ceError;
}

CENTERROR
CTIsUserInX(BOOLEAN *inX)
{
#if HAVE_UTMPS_H
    struct utmps *ent;
#elif HAVE_UTMPX_H
    struct utmpx *ent;
#elif HAVE_UTMP_H
    struct utmp *ent;
#else
#error no utmp support found
#endif

    *inX = FALSE;

#if HAVE_UTMPS_H
    setutsent();
#elif HAVE_UTMPX_H
    setutxent();
#elif HAVE_UTMP_H
    setutent();
#endif
    while(1)
    {
#if HAVE_UTMPS_H
        ent = GETUTSENT();
#elif HAVE_UTMPX_H
        ent = getutxent();
#elif HAVE_UTMP_H
        ent = getutent();
#endif
        if(ent == NULL)
            break;
        //HP-UX uses console when someone is graphically logged in
        if(ent->ut_line[0] == ':' || ent->ut_id[0] == ':' ||
                !strcmp(ent->ut_line, "console"))
        {
            *inX = TRUE;
            goto cleanup;
        }
    }

cleanup:
#if HAVE_UTMPS_H
    endutsent();
#elif HAVE_UTMPX_H
    endutxent();
#elif HAVE_UTMP_H
    endutent();
#endif

    return CENTERROR_SUCCESS;
}
