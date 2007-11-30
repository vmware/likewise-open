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

namespace DomainJoin
{
    /// <summary>
	/// Summary description for DomainManagerAPI.
	/// </summary>
	public class DomainManagerAPI
	{   
        private static string _hostName = "localhost";
        private static string _dnsSuffix = "localdomain";
        private static string _domainName = null;
        private static string _workgroupName = "WORKGROUP";
        private static string _description = "";
        private static bool   _joinedToDomain = false;
        
	    /// <summary>
	    /// Fetches the computer name
	    /// </summary>
	    /// <returns>The current computer's hostname</returns>
	    public static string GetHostname()
	    {
	        return _hostName;
	    }
	    
	    /// <summary>
	    /// Fetches the fully qualified computer name
	    /// </summary>
	    /// <returns>Fully qualified computer name</returns>
	    public static string GetFullyQualifiedHostname()
	    {
	        return _hostName + "." + _dnsSuffix;
	    }
        
        /// <summary>
        /// Fetches the DNS Suffix
        /// </summary>
        /// <returns>DNS Suffix</returns>
        public static string GetDnsSuffix()
        {
            return _dnsSuffix;
        }
	    
	    /// <summary>
	    /// Sets the computer name.
	    /// </summary>
	    /// <param name="hostName">Hostname to be set</param>
	    /// <returns>true if hostname could be set successfully</returns>
	    public static void SetHostname(string hostName)
	    {
	        _hostName = hostName;
	    }
	    
	    /// <summary>
	    /// Fetches the current Active Directory domain for the computer.
	    /// </summary>
	    /// <returns>domain name or null if not joined to a domain</returns>
	    public static string GetDomainName()
	    {
	        return _domainName;
	    }
	    
	    /// <summary>
	    /// Fetches the current workgroup name for the computer.
	    /// </summary>
	    /// <returns>workgroup name or null if not joined to a workgroup</returns>
	    public static string GetWorkgroupName()
	    {
	        return _workgroupName;
	    }
	    
	    /// <summary>
	    /// Joins the computer to an active directory domain.
	    /// </summary>
	    /// <param name="domainName">Active Directory domain name</param>
	    /// <param name="userName">Domain Administrator User Id</param>
	    /// <param name="password">Password</param>
	    public static void JoinDomain(string domainName, string userName, string password)
	    {
	        _domainName = domainName;
	        _joinedToDomain = true;
	        _workgroupName = null;
	    }
	    
	    /// <summary>
	    /// Joins the computer to a windows workgroup.
	    /// </summary>
	    /// <param name="workgroupName">Windows workgroup name</param>
	    /// <param name="userName">Administrator User Id</param>
	    /// <param name="password">Password</param>
	    public static void JoinWorkGroup(string workgroupName, string userName, string password)
	    {
	        _workgroupName = workgroupName;
	        _joinedToDomain = false;
	        _domainName = null;
	    }
	    
	    /// <summary>
	    /// Fetches the description of the computer.
	    /// </summary>
	    /// <returns>Computer Description</returns>
	    public static string GetComputerDescription()
	    {
	        return _description;
	    }
        
        /// <summary>
        /// Sets the computer description
        /// </summary>
        /// <param name="description">Computer description</param>
        public static void SetComputerDescription(string description)
        {
            _description = description;
        }
	}
}
