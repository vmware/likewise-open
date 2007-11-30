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

#include <syslog.h>

void sys_vsyslog(int priority, const char *format, va_list ap)
{
#if defined(HAVE_VSYSLOG)
	vsyslog(priority, format, ap);
#else
	CENTERROR ceError;
	PSTR buffer = NULL;

	ceError = CTAllocateStringPrintfV(&buffer, format, ap);
	if (!ceError) {
		syslog(priority, "%s", buffer);
	}

	CT_SAFE_FREE_STRING(buffer);
#endif				/* ! HAVE_VSYSLOG */
}

#if !defined(HAVE_RPL_MALLOC)
#undef malloc

//See http://wiki.buici.com/wiki/Autoconf_and_RPL_MALLOC
void *rpl_malloc(size_t n)
{
	if (n == 0)
		n = 1;
	return malloc(n);
}

#endif				/* ! HAVE_RPL_MALLOC */

#if !defined(HAVE_RPL_REALLOC)
#undef realloc

void *rpl_realloc(void *buf, size_t n)
{
	return realloc(buf, n);
}

#endif				/* ! HAVE_RPL_REALLOC */

BOOLEAN IsRoot()
{
	return (getuid() == 0);
}
