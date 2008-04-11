/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Guenther Deschner 2006

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "includes.h"

static void smb_nscd_flush_cache(const char *service)
{
#if defined(HAVE_NSCD_FLUSH_CACHE)
	if (!nscd_flush_cache(service)) {
		DEBUG(10,("failed to flush nscd cache for '%s' service: %s. "
			  "Is nscd running?\n",
			  service, strerror(errno)));
	}
#elif defined(DARWINOS)
	/* Darwin has a simple way to purge the Directory Service
	   cache, but no API for it.  So we have to run "dscacheutil 
	   -flushcache" */
	{
		int ret;
		SMB_STRUCT_STAT sbuf;

		if (sys_stat("/usr/sbin/lookupd", &sbuf) == 0) {
		  ret = smbrun("/usr/sbin/lookupd -flushcache", NULL);
		  if (ret != 0 ) {
		    DEBUG(2,("smb_nscd_flush_cache: Failed to flush "
			     "Apple Directory Service cache (%s)\n",
			     strerror(errno)));			
		  }
		} else if (sys_stat("/usr/bin/dscacheutil", &sbuf) == 0) {
		  ret = smbrun("/usr/bin/dscacheutil -flushcache", NULL);
		  if (ret != 0 ) {
		    DEBUG(2,("smb_nscd_flush_cache: Failed to flush "
			     "Apple Directory Service cache (%s)\n",
			     strerror(errno)));			
		  }
		} else {
		  DEBUG(1,("smb_nscd_flush_cache: Failed to find lookupd or dscacheutil tools.\n"));
		}

		return;		
	}	
	
#endif
}

void smb_nscd_flush_user_cache(void)
{
	smb_nscd_flush_cache("passwd");
}

void smb_nscd_flush_group_cache(void)
{
	smb_nscd_flush_cache("group");
}
