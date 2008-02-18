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

#ifndef __CTCFGPARSER_H__
#define __CTCFGPARSER_H__

typedef struct __NVPAIR {

    PSTR pszName;
    PSTR pszValue;

    struct __NVPAIR *pNext;

} NVPAIR, *PNVPAIR;

typedef struct __CFGSECTION {

    PSTR pszName;

    PNVPAIR pNVPairList;

    struct __CFGSECTION *pNext;

} CFGSECTION, *PCFGSECTION;

CENTERROR
CTParseConfigFile(
    PCSTR pszFilePath,
    PCFGSECTION* ppSectionList,
    BOOLEAN bWindowsDoubleByteFormat
    );

void
CTFreeConfigSectionList(
    PCFGSECTION pSectionList
    );

CENTERROR
CTSaveConfigSectionList(
    PCSTR pszConfigFilePath,
    PCFGSECTION pSectionList
    );

CENTERROR
CTSaveConfigSectionListToFile(
    FILE* fp,
    PCFGSECTION pSectionList
    );

CENTERROR
CTCreateConfigSection(
    PCFGSECTION* ppSectionList,
    PCFGSECTION* pCreatedSection,
    PCSTR pszSectionName
    );

CENTERROR
CTGetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PSTR* ppszValue
    );

CENTERROR
CTSetConfigValueBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue
    );

CENTERROR
CTGetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PSTR* ppszValue
    );

CENTERROR
CTSetConfigValueBySection(
    PCFGSECTION pSection,
    PCSTR pszName,
    PCSTR pszValue
    );

CENTERROR
CTDeleteConfigSection(
    PCFGSECTION* ppSectionList,
    PCSTR pszSectionName
    );

CENTERROR
CTDeleteNameValuePairBySection(
    PCFGSECTION pSection,
    PCSTR pszName
    );

CENTERROR
CTDeleteNameValuePairBySectionName(
    PCFGSECTION pSectionList,
    PCSTR pszSectionName,
    PCSTR pszName
    );
#endif /* __CTCFGPARSER_H__ */
