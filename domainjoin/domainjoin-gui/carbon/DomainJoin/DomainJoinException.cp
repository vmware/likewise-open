/*
 *  DomainJoinException.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "DomainJoinException.h"

const int DomainJoinException::CENTERROR_DOMAINJOIN_NON_ROOT_USER          = 524289;
const int DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_HOSTNAME       = 524290;
const int DomainJoinException::CENTERROR_DOMAINJOIN_GPAGENT_INCOMMUNICADO  = 524320;
const int DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME    = 524323;
const int DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_USERID         = 524322;
const int DomainJoinException::CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME = 524326;
const int DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_OU             = 524334;
const int DomainJoinException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS     = 524343;

DomainJoinException::DomainJoinException()
: _errCode(0)
{
}

DomainJoinException::DomainJoinException(int errCode)
: _errCode(errCode)
{
}

DomainJoinException::DomainJoinException(const std::string& errMsg)
: _errCode(0),
  _errMsg(errMsg)
{
}

DomainJoinException::DomainJoinException(const std::string& errMsg, int errCode)
: _errCode(errCode),
  _errMsg(errMsg)
{
}

void
DomainJoinException::MapErrorCode(int errCode)
{
    if (!errCode)
	   return;
	   
	switch(errCode)
	{
		case CENTERROR_DOMAINJOIN_NON_ROOT_USER:
		{
			throw NonRootUserException();
		}
		case CENTERROR_DOMAINJOIN_INVALID_HOSTNAME:
		{
		    throw InvalidHostnameException();
		}
		case CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME:
		{
		    throw InvalidDomainnameException();
		}
		case CENTERROR_DOMAINJOIN_INVALID_USERID:
		{
		    throw InvalidUsernameException();
		}
		case CENTERROR_DOMAINJOIN_INVALID_OU:
		{
		    throw InvalidOUPathException();
		}
		case CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS:
		{
		    throw FailedAdminPrivilegeException();
		}
		case CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME:
		{
		    throw UnresolvedDomainNameException();
		}
		case CENTERROR_DOMAINJOIN_GPAGENT_INCOMMUNICADO:
		{
		    throw CannotTalkToGPAgentException();
		}
		default:
		{
			throw DomainJoinException(errCode);
		}
	}
}

