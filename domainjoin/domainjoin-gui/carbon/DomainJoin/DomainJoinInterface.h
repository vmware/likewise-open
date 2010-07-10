/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 *  DomainJoinInterface.h
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/7/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __DOMAINJOININTERFACE_H__
#define __DOMAINJOININTERFACE_H__

#include <Carbon/Carbon.h>

#include <string>
#include <dlfcn.h>

#include "DomainJoinStatus.h"
#include "DomainJoinException.h"
#include "djitf.h"

class DomainJoinInterface
{  
    private:
       DomainJoinInterface();
       ~DomainJoinInterface();
          
    public:
    
        static void JoinDomain(std::string& szDomainName,
                               std::string& pszUserName,
                               std::string& pszPassword,
                               std::string& pszOU,
                               std::string& pszUserDomainPrefix,
                               bool bAssumeDefaultDomain,
                               bool bNoHosts);
        
        static void LeaveDomain();
        
        static void SetComputerName(std::string& pszComputerName,
                                    std::string& pszDomainName);
        
        static void GetDomainJoinStatus(DomainJoinStatus& joinStatus);
		
		static bool IsDomainNameResolvable(const std::string& domainName);
        
    protected:
    
        static DomainJoinInterface& getInstance();
        
        void Initialize();
        void Cleanup();
        
        static void LoadFunction(
						void*       pLibHandle,
						const char* pszFunctionName,
						void**      FunctionPointer
						);
        
    private:
    
        static DomainJoinInterface* _instance;
    
        void* _libHandle;
		PDJ_API_FUNCTION_TABLE _pDJApiFunctionTable;
        PFNShutdownJoinInterface _pfnShutdownJoinInterface;
};

#endif /* __DOMAINJOININTERFACE_H__ */
