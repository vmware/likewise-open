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

#ifndef __DJAIXPARSER_H__
#define __DJAIXPARSER_H__

ssize_t
DJFindStanza(const DynamicArray *lines, PCSTR name);

ssize_t DJFindLine(const DynamicArray *lines, const char *stanza, const char *name);

CENTERROR DJGetOptionValue(const DynamicArray *lines, PCSTR stanza, PCSTR name, PSTR *value);

CENTERROR
DJSetOptionValue(DynamicArray *lines, PCSTR stanza, PCSTR name, PCSTR value);

#endif /* __DJAIXPARSER_H__ */
