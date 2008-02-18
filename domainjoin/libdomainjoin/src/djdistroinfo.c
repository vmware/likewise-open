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
#include "domainjoin.h"
#include "djdistroinfo.h"
#include <sys/utsname.h>
#include <sys/types.h>
#include <regex.h>
#ifdef HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

CENTERROR DJGetDistroInfo(const char *testPrefix, DistroInfo *info)
{
    BOOLEAN exists;
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct utsname unameStruct;
    char *path = NULL;
    PSTR fileContents = NULL;
    PSTR distroString = NULL;
    BOOLEAN rxAlloced = FALSE;
    regex_t rx;
    memset(info, 0, sizeof(*info));
#if defined(HAVE_SYSINFO) && defined(SI_ARCHITECTURE)
    char archBuffer[100];
#endif

    //According to the Solaris man page, any non-negative return value
    //indicates success. In fact, Solaris 8 returns 1, while Linux returns
    //0.
    if(uname(&unameStruct) < 0)
        return CTMapSystemError(errno);

    //Check for os override file
    if(testPrefix == NULL)
        testPrefix = "";
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/ostype", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, &fileContents, NULL));
    }
    if(fileContents != NULL)
    {
        CTStripWhitespace(fileContents);
        info->os = DJGetOSFromString(fileContents);
    }
    else
        info->os = DJGetOSFromString(unameStruct.sysname);
    CT_SAFE_FREE_STRING(fileContents);
    CT_SAFE_FREE_STRING(path);

    //Check for distro override file
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/osdistro", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, &distroString, NULL));
    }
    CT_SAFE_FREE_STRING(path);

    if(distroString != NULL)
    {
        CTStripWhitespace(distroString);
        info->distro = DJGetDistroFromString(distroString);
        CT_SAFE_FREE_STRING(distroString);
        GCE(ceError = CTStrdup("unknown", &info->version));
    }
    else if(info->os == OS_LINUX)
    {
        struct
        {
            DistroType matchDistro;
            const char *searchFile;
            const char *matchRegex;
            int versionMatchNum;
            BOOLEAN compareCase;
        } const distroSearch[] = {
            {
                DISTRO_RHEL,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   Red Hat Enterprise Linux ES release 4 (Nahant Update 1)
                #   Red Hat Advanced Server release 2.1AS (Pensacola)
                # In addition, Oracle Linux reports itself as:
                #   Enterprise Linux Enterprise Linux AS release 4 (October Update 5)
                */
                //Find a matching distro name
                "^\\s*((Red Hat)|(Enterprise Linux)) ((Enterprise Linux)|(Linux (Advanced|Enterprise) Server))\\s+(AS |ES )?"
                //Get the version number, but strip the minor version if it is
                //present (RHEL 2 has one). Also remove the AS or ES
                //suffix if it is present.
                "release ([[:digit:]]+)(\\.[[:digit:]]+)?(AS|ES)?(\\s+\\(.*\\))?\\s*$",
                9,
                1
            },
            {
                DISTRO_REDHAT,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   Red Hat Linux release 7.3 (Valhala)
                */
                "^\\s*Red Hat Linux release ([[:digit:]]+(\\.[[:digit:]]+)?)",
                1,
                1
            },
            {
                DISTRO_FEDORA,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   Fedora Core release 4 (Stentz)
                */
                "^\\s*Fedora (Core )?release (\\S+)",
                2,
                1
            },
            {
                DISTRO_CENTOS,
                "/etc/redhat-release",
                /*
                # The format of the line is something like:
                #   CentOS release 4.x (Final)
                */
                "^\\s*CentOS release ([[:digit:]]+)"
                //Trim off the minor version number
                "(\\.[[:digit:]]+)?",
                1,
                1
            },
            {
                DISTRO_SUSE,
                "/etc/SuSE-release",
                "^\\s*SUSE LINUX ([[:digit:]]+\\.[[:digit:]]+)\\s+",
                1,
                0
            },
            {
                DISTRO_OPENSUSE,
                "/etc/SuSE-release",
                "^\\s*openSUSE ([[:digit:]]+\\.[[:digit:]]+)\\s+",
                1,
                0
            },
            {
                DISTRO_SLES,
                "/etc/SuSE-release",
                "^\\s*SUSE LINUX Enterprise Server ([[:digit:]]+)\\s+",
                1,
                0
            },
            {
                DISTRO_SLED,
                "/etc/SuSE-release",
                "^\\s*SUSE LINUX Enterprise Desktop ([[:digit:]]+)\\s+",
                1,
                0
            },
            {
                DISTRO_UBUNTU,
                "/etc/lsb-release",
                /*
                # The file will have lines that include:
                #   DISTRIB_ID=Ubuntu
                #   DISTRIB_RELEASE=6.06
                */
                "^\\s*DISTRIB_ID\\s*=\\s*Ubuntu\\s*\n"
                "(.*\n)?DISTRIB_RELEASE\\s*=\\s*(\\S+)\\s*(\n.*)?$",
                2,
                1
            },
            {
                DISTRO_DEBIAN,
                "/etc/debian_version",
                /*
                # The format of the entire file is a single line like:
                # 3.1
                # and nothing else, so that is the version
                */
                "^\\s*(\\S+)\\s*$",
                1,
                1
            },
        };
        int i;
        regmatch_t matches[10];
        info->distro = DISTRO_UNKNOWN;
        for(i = 0; info->distro == DISTRO_UNKNOWN; i++)
        {
            if(i == sizeof(distroSearch)/sizeof(distroSearch[0]))
            {
                //We past the last item in DistroSearch
                break;
            }
            GCE(ceError = CTAllocateStringPrintf(
                    &path, "%s%s", testPrefix, distroSearch[i].searchFile));
            GCE(ceError = CTCheckFileOrLinkExists(path, &exists));
            if(exists)
            {
                int flags = REG_EXTENDED;
                if(!distroSearch[i].compareCase)
                    flags |= REG_ICASE;

                GCE(ceError = CTReadFile(path, &fileContents, NULL));
                if(regcomp(&rx, distroSearch[i].matchRegex, flags) != 0)
                {
                    GCE(ceError = CENTERROR_REGEX_COMPILE_FAILED);
                }
                rxAlloced = TRUE;
                if(regexec(&rx, fileContents,
                        sizeof(matches)/sizeof(matches[0]), matches, 0) == 0 &&
                        matches[distroSearch[i].versionMatchNum].rm_so != -1)
                {
                    //This is the correct distro
                    regmatch_t *ver = &matches[distroSearch[i].versionMatchNum];
                    info->distro = distroSearch[i].matchDistro;
                    GCE(ceError = CTStrndup(fileContents + ver->rm_so,
                                ver->rm_eo - ver->rm_so,
                                &info->version));
                }
                regfree(&rx);
                rxAlloced = FALSE;
                CT_SAFE_FREE_STRING(fileContents);
            }
            CT_SAFE_FREE_STRING(path);
        }
        if(info->distro == DISTRO_DEBIAN)
        {
            /*
            #
            # Debian and Ubuntu both have /etc/debian_version,
            # but only Ubuntu has an /etc/lsb-release
            #
            */
            GCE(ceError = CTAllocateStringPrintf(
                    &path, "%s/etc/lsb-release", testPrefix));
            GCE(ceError = CTCheckFileOrLinkExists(path, &exists));
            if(exists)
            {
                DJ_LOG_ERROR("Unexpected file: %s", path);
                info->distro = DISTRO_UNKNOWN;
            }
            CT_SAFE_FREE_STRING(path);
        }
    }
    else
    {
        //It's a UNIX system
        switch(info->os)
        {
        case OS_AIX:
            info->distro = DISTRO_AIX;
            /*Uname output from AIX 5.3:
            $ uname -v
            5
            $ uname -r
            3
            */
            GCE(ceError = CTAllocateStringPrintf(&info->version,
                        "%s.%s", unameStruct.version, unameStruct.release));
            break;
        case OS_SUNOS:
            info->distro = DISTRO_SUNOS;
            /*Uname output from Solaris 8:
            $ uname -r
            5.8
            */
            GCE(ceError = CTAllocateStringPrintf(&info->version,
                        "%s", unameStruct.release));
            break;
        case OS_DARWIN:
            info->distro = DISTRO_DARWIN;
            GCE(ceError = CTCaptureOutput("sw_vers -productVersion",
                    &info->version));
            break;
        case OS_HPUX:
            info->distro = DISTRO_HPUX;
            {
                const char *temp = unameStruct.release;
                while(!isdigit((int)*temp)) temp++;
                GCE(ceError = CTStrdup(temp, &info->version));
            }
            break;
        default:
            info->distro = DISTRO_UNKNOWN;
        }
    }

    if(info->distro == DISTRO_UNKNOWN)
    {
        CT_SAFE_FREE_STRING(info->version);
        GCE(ceError = CTStrdup("unknown", &info->version));
    }

    //Check for version override file
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/osver", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, &fileContents, NULL));
    }
    if(fileContents != NULL)
    {
        CTStripWhitespace(fileContents);
        CT_SAFE_FREE_STRING(info->version);
        info->version = fileContents;
        fileContents = NULL;
    }
    CT_SAFE_FREE_STRING(path);

    /*
    uname -m output:
    Linux: x86_64
    Linux: i386
    Linux: i686
    AIX: 00CBE1DD4C00
    Solaris: sun4u
    Solaris: i86pc
    Darwin: i386
    Darwin: Power Macintosh
    HPUX: 9000/785

    uname -i output:
    Linux: x86_64
    Linux: i386
    RHEL21: not recogn
    AIX: not recogn
    Darwin: not recogn
    Solaris: SUNW,Ultra-4
    Solaris: i86pc
    HPUX: 2000365584

    uname -p output:
    Linux reads /proc/cpuinfo
    Linux: x86_64
    Linux: i686
    Linux: athlon
    Darwin: i386
    Darwin: powerpc
    AIX has the value hard coded in uname
    AIX: powerpc
    Solaris uses sysinfo(SI_ARCHITECTURE, buff, sizeof(buff)
    Solaris: sparc
    Solaris: i386
    HPUX: not recogn
    */
    info->arch = ARCH_UNKNOWN;
#if defined(HAVE_SYSINFO) && defined(SI_ARCHITECTURE)
    //Solaris has this
    if(info->arch == ARCH_UNKNOWN &&
            sysinfo(SI_ARCHITECTURE, archBuffer, sizeof(archBuffer)) != -1)
    {
        info->arch = DJGetArchFromString(archBuffer);
    }
#endif
#if defined(HAVE_SYSCONF) && defined(_SC_CPU_VERSION)
    //HPUX uses this
    if(info->arch == ARCH_UNKNOWN)
    {
        switch(sysconf(_SC_CPU_VERSION))
        {
            case CPU_PA_RISC1_0:
            case CPU_PA_RISC1_1:
            case CPU_PA_RISC1_2:
            case CPU_PA_RISC2_0:
            case CPU_PA_RISC_MAX:
                info->arch = ARCH_HPPA;
                break;
#ifdef CPU_HP_INTEL_EM_1_0
            case CPU_HP_INTEL_EM_1_0:
#endif
#ifdef CPU_IA64_ARCHREV_0
            case CPU_IA64_ARCHREV_0:
#endif
                info->arch = ARCH_IA64;
                break;
            //If it's not any of the previous values, let another test figure
            //it out.
        }
    }
#endif
    if(info->arch == ARCH_UNKNOWN)
    {
        //Linux uses this, and sometimes Darwin does too. If 'uname -m' doesn't
        //return something meaningful on this platform, then the arch will stay
        //as unknown.
        info->arch = DJGetArchFromString(unameStruct.machine);
    }
    if(info->arch == ARCH_UNKNOWN)
    {
        //AIX and sometimes Darwin use this
        GCE(ceError = CTCaptureOutput("uname -p", &distroString));
        CTStripWhitespace(distroString);
        info->arch = DJGetArchFromString(distroString);
        CT_SAFE_FREE_STRING(distroString);
    }

    //Check for arch override file
    GCE(ceError = CTAllocateStringPrintf(
            &path, "%s/osarch", testPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(
            path, &exists));
    if(exists)
    {
        GCE(ceError = CTReadFile(
                path, &fileContents, NULL));
        info->arch = DJGetArchFromString(fileContents);
        CT_SAFE_FREE_STRING(fileContents);
    }
    CT_SAFE_FREE_STRING(path);

cleanup:
    CT_SAFE_FREE_STRING(path);
    CT_SAFE_FREE_STRING(fileContents);
    CT_SAFE_FREE_STRING(distroString);
    if(rxAlloced)
        regfree(&rx);
    if(!CENTERROR_IS_OK(ceError))
    {
        DJFreeDistroInfo(info);
    }
    return ceError;
}

struct
{
    OSType value;
    const char *name;
} static const osList[] = 
{
    { OS_AIX, "AIX" },
    { OS_SUNOS, "SunOS" },
    { OS_SUNOS, "Solaris" },
    { OS_DARWIN, "Darwin"},
    { OS_DARWIN, "OsX" },
    { OS_HPUX, "HP-UX"},
    { OS_HPUX, "HPUX" },
    { OS_LINUX, "Linux" },
};

OSType DJGetOSFromString(const char *str)
{
    int i;
    for(i = 0; i < sizeof(osList)/sizeof(osList[0]); i++)
    {
        if(!strcasecmp(str, osList[i].name))
            return osList[i].value;
    }
    return OS_UNKNOWN;
}

CENTERROR DJGetOSString(OSType type, char **result)
{
    int i;
    for(i = 0; i < sizeof(osList)/sizeof(osList[0]); i++)
    {
        if(type == osList[i].value)
            return CTStrdup(osList[i].name, result);
    }
    return CTStrdup("unknown", result);
}

struct
{
    DistroType value;
    const char *name;
} static const distroList[] = 
{
    { DISTRO_AIX, "AIX" },
    { DISTRO_SUNOS, "Solaris" },
    { DISTRO_SUNOS, "SunOS" },
    { DISTRO_DARWIN, "Darwin"},
    { DISTRO_DARWIN, "OsX" },
    { DISTRO_HPUX, "HP-UX"},
    { DISTRO_HPUX, "HPUX" },
    { DISTRO_RHEL, "RHEL" },
    { DISTRO_REDHAT, "Redhat" },
    { DISTRO_FEDORA, "Fedora" },
    { DISTRO_CENTOS, "CentOS" },
    { DISTRO_SUSE, "SuSE" },
    { DISTRO_OPENSUSE, "OpenSuSE" },
    { DISTRO_SLES, "SLES" },
    { DISTRO_SLED, "SLED" },
    { DISTRO_UBUNTU, "Ubuntu" },
    { DISTRO_DEBIAN, "Debian" },
};

DistroType DJGetDistroFromString(const char *str)
{
    int i;
    for(i = 0; i < sizeof(distroList)/sizeof(distroList[0]); i++)
    {
        if(!strcasecmp(str, distroList[i].name))
            return distroList[i].value;
    }
    return DISTRO_UNKNOWN;
}

CENTERROR DJGetDistroString(DistroType type, char **result)
{
    int i;
    for(i = 0; i < sizeof(distroList)/sizeof(distroList[0]); i++)
    {
        if(type == distroList[i].value)
            return CTStrdup(distroList[i].name, result);
    }
    return CTStrdup("unknown", result);
}

struct
{
    ArchType value;
    const char *name;
} static const archList[] = 
{
    { ARCH_X86_32, "x86_32" },
    { ARCH_X86_32, "i386" },
    { ARCH_X86_32, "i486" },
    { ARCH_X86_32, "i586" },
    { ARCH_X86_32, "i686" },
    { ARCH_X86_64, "x86_64" },
    { ARCH_HPPA, "hppa" },
    { ARCH_IA64, "ia64" },
    { ARCH_IA64, "itanium" },
    { ARCH_SPARC, "sparc" },
    { ARCH_POWERPC, "powerpc" },
    { ARCH_POWERPC, "ppc" },
};

ArchType DJGetArchFromString(const char * str)
{
    int i;
    for(i = 0; i < sizeof(archList)/sizeof(archList[0]); i++)
    {
        if(!strcasecmp(str, archList[i].name))
            return archList[i].value;
    }
    return ARCH_UNKNOWN;
}

CENTERROR DJGetArchString(ArchType type, char **result)
{
    int i;
    for(i = 0; i < sizeof(archList)/sizeof(archList[0]); i++)
    {
        if(type == archList[i].value)
            return CTStrdup(archList[i].name, result);
    }
    return CTStrdup("unknown", result);
}

void DJFreeDistroInfo(DistroInfo *info)
{
    if(info != NULL)
        CT_SAFE_FREE_STRING(info->version);
}
