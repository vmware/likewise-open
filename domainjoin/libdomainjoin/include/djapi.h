/*
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

#ifndef __DJ_API_H__
#define __DJ_API_H__

#include <lwexc.h>

typedef struct
{
    BOOLEAN noModifyHosts;
    char* logFile;

/* Callbacks */
    void (*warn)(
	const char* shortMsg,
	const char* longMsg,
	LWException** exc);
    void (*progress)(
	double ratio,
	const char* operationMsg,
	LWException** exc);
} DJOptions;

void
DJQuery(
    char **computer,
    char **domain,
    DJOptions* options,
    LWException** exc
    );

void
DJRenameComputer(
    const char* computer,
    const char* domain,
    DJOptions* options,
    LWException** exc
    );

void
DJJoinDomain(
    const char* domain,
    const char* ou,
    const char* user,
    const char* password,
    DJOptions* options,
    LWException** exc
    );

void
DJLeaveDomain(
    DJOptions* options,
    LWException** exc
    );

#endif
