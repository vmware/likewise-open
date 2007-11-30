//
// Copyright (C) Centeris Corporation 2004-2007
// Copyright (C) Likewise Software 2007.  
// All rights reserved.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

using System;

namespace Centeris.DomainJoinLib
{
    class GenericException : ApplicationException
    {
        public GenericException(string message)
            : base(message)
        {
        }
    }

    class ComputerNameNotFoundException : ApplicationException
    {
        public ComputerNameNotFoundException()
        {
        }
    }

    public class InvalidComputerNameException : ApplicationException
    {
        public InvalidComputerNameException(string message)
            : base(message)
        {
        }
    }

    public class InvalidOrganizationalUnitException : ApplicationException
    {
        protected string ouPath;
        protected string message;

        public InvalidOrganizationalUnitException(string ouPath)
        {
            this.ouPath = ouPath;
            message = "Invalid OU path \"" + ouPath + "\"";
        }

        public InvalidOrganizationalUnitException(string ouPath, string reason)
            : this(ouPath)
        {
            if (!SupportClass.IsNullOrEmpty(reason))
            {
                message += " (reason: " + reason + ")";
            }
        }

        public string OrganizationalUnit
        {
            get {
                return ouPath;
            }
        }

        public override string Message
        {
            get
            {
                return message;
            }
        }
    }
}
