/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

/*
 *  DomainJoinException.h
 *  DomainJoin
 *
 *  Created by Chuck Mount on 8/8/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#ifndef __DOMAINJOINEXCEPTION_H__
#define __DOMAINJOINEXCEPTION_H__

#include <Carbon/Carbon.h>
#include <string>
#include <exception>

class DomainJoinException : public std::exception
{
    public:
    
        DomainJoinException();
        DomainJoinException(const std::string& errMsg);
        DomainJoinException(const std::string& errMsg, int errCode);
        DomainJoinException(int errCode);
        virtual ~DomainJoinException() throw() {}
    
    public:
    
        inline int GetErrorCode() { return _errCode; }
        inline void SetErrorCode(int code) { _errCode = code; }
        
        virtual const char* what() const throw() { return _errMsg.c_str(); }
        
        static void MapErrorCode(int errCode);
                
    public:
    
        static const int CENTERROR_DOMAINJOIN_NON_ROOT_USER;
		static const int CENTERROR_DOMAINJOIN_INVALID_HOSTNAME;
		static const int CENTERROR_DOMAINJOIN_GPAGENT_INCOMMUNICADO;
		static const int CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME;
		static const int CENTERROR_DOMAINJOIN_INVALID_USERID;
		static const int CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME;
		static const int CENTERROR_DOMAINJOIN_INVALID_OU;
		static const int CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS;

    private:
    
        std::string _errMsg;
        int _errCode;

};

class NonRootUserException : public DomainJoinException
{
    public:
        NonRootUserException()
        : DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_NON_ROOT_USER)
        {
        }
};

class InvalidHostnameException : public DomainJoinException
{
    public:
	    InvalidHostnameException()
		: DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_HOSTNAME)
		{
		}
};

class InvalidDomainnameException : public DomainJoinException
{
    public:
	    InvalidDomainnameException()
		: DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_DOMAIN_NAME)
	    {
		}
};

class InvalidUsernameException : public DomainJoinException
{
    public:
	    InvalidUsernameException()
		: DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_USERID)
		{
		}
};

class InvalidOUPathException : public DomainJoinException
{
    public:
	    InvalidOUPathException()
		: DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_INVALID_OU)
		{
		}
};

class FailedAdminPrivilegeException : public DomainJoinException
{
    public:
	    FailedAdminPrivilegeException()
		: DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS)
		{
		}
	    FailedAdminPrivilegeException(const std::string& errMsg)
		: DomainJoinException(errMsg, DomainJoinException::CENTERROR_DOMAINJOIN_FAILED_ADMIN_PRIVS)
		{
		}		
};

class UnresolvedDomainNameException : public DomainJoinException
{
    public:
	    UnresolvedDomainNameException()
		: DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_UNRESOLVED_DOMAIN_NAME)
		{
		}
};

class CannotTalkToGPAgentException : public DomainJoinException
{
    public:
	    CannotTalkToGPAgentException()
		: DomainJoinException(DomainJoinException::CENTERROR_DOMAINJOIN_GPAGENT_INCOMMUNICADO)
		{
		}
};

#endif /* __DOMAINJOINEXCEPTION_H__ */


