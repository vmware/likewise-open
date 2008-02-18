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

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#ifndef __DJDISTROINFO_H__
#define __DJDISTROINFO_H__

typedef enum
{
    OS_UNKNOWN,
    OS_AIX,
    OS_SUNOS,
    OS_DARWIN,
    OS_HPUX,
    OS_LINUX,
} OSType;

typedef enum
{
    DISTRO_UNKNOWN = OS_UNKNOWN,
    DISTRO_AIX = OS_AIX,
    DISTRO_SUNOS = OS_SUNOS,
    DISTRO_DARWIN = OS_DARWIN,
    DISTRO_HPUX = OS_HPUX,
    DISTRO_RHEL,
    DISTRO_REDHAT,
    DISTRO_FEDORA,
    DISTRO_CENTOS,
    DISTRO_SUSE,
    DISTRO_OPENSUSE,
    DISTRO_SLES,
    DISTRO_SLED,
    DISTRO_UBUNTU,
    DISTRO_DEBIAN,
} DistroType;

typedef enum
{
    ARCH_UNKNOWN,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_HPPA,
    ARCH_IA64,
    ARCH_SPARC,
    ARCH_POWERPC,
} ArchType;

typedef struct
{
    OSType os;
    DistroType distro;
    ArchType arch;
    char *version;
} DistroInfo;

OSType DJGetOSFromString(const char *str);
DistroType DJGetDistroFromString(const char *str);
ArchType DJGetArchFromString(const char * str);

CENTERROR DJGetOSString(OSType type, char **result);
CENTERROR DJGetDistroString(DistroType type, char **result);
CENTERROR DJGetArchString(ArchType type, char **result);

//Fills in fields with correct values
CENTERROR
DJGetDistroInfo(const char *testPrefix, DistroInfo *info);

//Safe to call after DJGetDistroInfo has been called, or the structure has
//been zeroed out.
void DJFreeDistroInfo(DistroInfo *info);

#endif // __DJDISTROINFO_H__
