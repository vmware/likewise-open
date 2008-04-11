/* 
   Unix SMB/CIFS implementation.

   Winbind daemon for ntdom nss module

   Copyright (C) Tim Potter 2000
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _WINBIND_NSS_LINUX_H
#define _WINBIND_NSS_LINUX_H

#if HAVE_NSS_H
#include <nss.h>
#endif

#ifndef NSS_STATUS_DEFINED
#define NSS_STATUS_DEFINED
typedef enum nss_status NSS_STATUS;
#endif

NSS_STATUS _nss_lwidentity_setpwent(void);
NSS_STATUS _nss_lwidentity_endpwent(void);
NSS_STATUS _nss_lwidentity_getpwent_r(struct passwd *result, char *buffer, 
				   size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_getpwuid_r(uid_t uid, struct passwd *result, 
				   char *buffer, size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_getpwnam_r(const char *name, struct passwd *result, 
				   char *buffer, size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_setgrent(void);
NSS_STATUS _nss_lwidentity_endgrent(void);
NSS_STATUS _nss_lwidentity_getgrent_r(struct group *result, char *buffer, 
				   size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_getgrlst_r(struct group *result, char *buffer, 
				   size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_getgrnam_r(const char *name, struct group *result, 
				   char *buffer, size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_getgrgid_r(gid_t gid, struct group *result, char *buffer, 
				   size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_initgroups_dyn(const char *user, gid_t group, long int *start, 
				       long int *size, gid_t **groups, 
				       long int limit, int *errnop);
NSS_STATUS _nss_lwidentity_getusersids(const char *user_sid, char **group_sids, 
				    int *num_groups, char *buffer, size_t buf_size, 
				    int *errnop);
NSS_STATUS _nss_lwidentity_nametosid(const char *name, char **sid, char *buffer,
				  size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_sidtoname(const char *sid, char **name, char *buffer, 
				  size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_sidtouid(const char *sid, uid_t *uid, int *errnop);
NSS_STATUS _nss_lwidentity_sidtogid(const char *sid, gid_t *gid, int *errnop);
NSS_STATUS _nss_lwidentity_uidtosid(uid_t uid, char **sid, char *buffer, 
				 size_t buflen, int *errnop);
NSS_STATUS _nss_lwidentity_gidtosid(gid_t gid, char **sid, char *buffer, 
				 size_t buflen, int *errnop);

#endif /* _WINBIND_NSS_LINUX_H */
