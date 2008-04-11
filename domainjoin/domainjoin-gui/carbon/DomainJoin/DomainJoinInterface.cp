/*
 *  DomainJoinInterface.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/7/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainJoinInterface.h"

#define CT_SAFE_FREE_MEMORY(mem) \
    do { if (mem) { free(mem); (mem) = NULL; } } while (0)
    
#define LIBDOMAINJOIN "/opt/centeris/lib/libdomainjoin.so"

DomainJoinInterface* DomainJoinInterface::_instance = NULL;

DomainJoinInterface::DomainJoinInterface()
: _libHandle(NULL),
  _joinDomain(NULL),
  _joinWorkgroup(NULL),
  _setComputerName(NULL),
  _queryInformation(NULL),
  _isDomainNameResolvable(NULL)
{
}

DomainJoinInterface::~DomainJoinInterface()
{
    Cleanup();
}

DomainJoinInterface&
DomainJoinInterface::getInstance()
{
    if (_instance == NULL)
    {
       _instance = new DomainJoinInterface();
       _instance->Initialize();
    }
    
    return *_instance;
}

void
DomainJoinInterface::Initialize()
{
    dlerror();
    _libHandle = dlopen(LIBDOMAINJOIN, RTLD_LAZY);
    if (!_libHandle)
    {
       std::string errMsg = dlerror();
       throw DomainJoinException(errMsg);
    }
    
    LoadFunction("JoinDomain", (void**)&_joinDomain);
       
    LoadFunction("JoinWorkgroup", (void**)&_joinWorkgroup);
       
    LoadFunction("DJSetComputerName", (void**)&_setComputerName);
       
    LoadFunction("QueryInformation", (void**)&_queryInformation);
	
	LoadFunction("DJIsDomainNameResolvable", (void**)&_isDomainNameResolvable);
}

void
DomainJoinInterface::Cleanup()
{
  if (_libHandle)
  {
    _joinDomain = NULL;
    _joinWorkgroup = NULL;
    _setComputerName = NULL;
    _queryInformation = NULL;
	_isDomainNameResolvable = NULL;
  
    dlclose(_libHandle);
    _libHandle = NULL;
  }
}

void
DomainJoinInterface::LoadFunction(const char* pszFunctionName, void** functionPointer)
{
    void* function;
    
    dlerror();
    
    function = dlsym(_libHandle, pszFunctionName);
    if (!function)
    {
       std::string errMsg = dlerror();
       throw DomainJoinException(errMsg);
    }
    
    *functionPointer = function;
}

void
DomainJoinInterface::JoinDomain(std::string& pszDomainName,
                                std::string& pszUserName,
                                std::string& pszPassword,
                                std::string& pszOU,
                                bool bNoHosts)
{
     DomainJoinException::MapErrorCode(getInstance()._joinDomain(const_cast<char*>(pszDomainName.c_str()),
                                                                 const_cast<char*>(pszUserName.c_str()), 
                                                                 const_cast<char*>(pszPassword.c_str()), 
                                                                 const_cast<char*>(pszOU.c_str()), 
                                                                 bNoHosts));
}
                                        
void
DomainJoinInterface::LeaveDomain()
{
    DomainJoinException::MapErrorCode(getInstance()._joinWorkgroup("WORKGROUP", "empty", ""));
}

bool
DomainJoinInterface::IsDomainNameResolvable(const std::string& domainName)
{
    long result = 0;
	DomainJoinException::MapErrorCode(getInstance()._isDomainNameResolvable(const_cast<char*>(domainName.c_str()), &result));
	return (result ? true : false);
}
        
void
DomainJoinInterface::SetComputerName(std::string& pszComputerName,
                                     std::string& pszDomainName)
{
    DomainJoinException::MapErrorCode(getInstance()._setComputerName(const_cast<char*>(pszComputerName.c_str()),
                                                                     const_cast<char*>(pszDomainName.c_str())));
}

void
DomainJoinInterface::GetDomainJoinStatus(DomainJoinStatus& joinStatus)
{
    PDOMAINJOININFO pInfo = NULL;
    
    DomainJoinException::MapErrorCode(getInstance()._queryInformation(&pInfo));
    
    joinStatus.Name = (pInfo->pszName ? pInfo->pszName : "");
    joinStatus.Description = (pInfo->pszDescription ? pInfo->pszDescription : "");
    joinStatus.DnsDomain = (pInfo->pszDnsDomain ? pInfo->pszDnsDomain : "");
    joinStatus.DomainName = (pInfo->pszDomainName ? pInfo->pszDomainName : "");
    joinStatus.ShortDomainName = (pInfo->pszDomainShortName ? pInfo->pszDomainShortName : "");
    joinStatus.WorkgroupName = (pInfo->pszWorkgroupName ? pInfo->pszWorkgroupName : "");
    joinStatus.LogFilePath = (pInfo->pszLogFilePath ? pInfo->pszLogFilePath : "");
    
    CT_SAFE_FREE_MEMORY(pInfo->pszName);
    CT_SAFE_FREE_MEMORY(pInfo->pszDescription);
    CT_SAFE_FREE_MEMORY(pInfo->pszDnsDomain);
    CT_SAFE_FREE_MEMORY(pInfo->pszDomainName);
    CT_SAFE_FREE_MEMORY(pInfo->pszDomainShortName);
    CT_SAFE_FREE_MEMORY(pInfo->pszWorkgroupName);
    CT_SAFE_FREE_MEMORY(pInfo->pszLogFilePath);
    CT_SAFE_FREE_MEMORY(pInfo);
}
