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

#ifndef __DJ_AUTHINFO_H__
#define __DJ_AUTHINFO_H__

CENTERROR
JoinDomain(
    PCSTR pszDomainName,
    PCSTR pszUserName,
    PCSTR pszPassword,
    PCSTR pszOU,
    BOOLEAN bNoHosts
    );

CENTERROR
JoinWorkgroup(
    PSTR pszWorkgroupName,
    PSTR pszUserName,
    PSTR pszPassword
    );

CENTERROR
DJGetConfiguredDescription(
    PSTR* ppszDescription
    );

CENTERROR
DJSetConfiguredDescription(
    PSTR pszDescription
    );

CENTERROR
DJGetConfiguredDomain(
    PSTR* ppszDomain
    );

CENTERROR
DJGetConfiguredWorkgroup(
    PSTR* ppszWorkgroup
    );

CENTERROR
DJRemoveCacheFiles();

//The answer is non-authoritative
CENTERROR
DJGetDomainDC(PCSTR domain, PSTR *dc);

void
DJGetComputerDN(PSTR *dn, LWException **exc);

void DJNetInitialize(LWException **exc);

void DJNetShutdown(LWException **exc);

void DJCreateComputerAccount(PCSTR hostname,
                PCSTR domainName,
                PCSTR ou,
                PCSTR username,
                PCSTR password,
                PSTR *shortDomainName,
                JoinProcessOptions *options,
                LWException **exc);

void DJDisableComputerAccount(PCSTR username,
                PCSTR password,
                LWException **exc);

//The answer is non-authoritative
void DJGuessShortDomainName(PCSTR longName,
                PSTR *shortName,
                LWException **exc);

extern const JoinModule DJDoJoinModule;
extern const JoinModule DJLwiConfModule;
extern const JoinModule DJDoLeaveModule;

#endif /* __DJ_AUTHINFO_H__ */
