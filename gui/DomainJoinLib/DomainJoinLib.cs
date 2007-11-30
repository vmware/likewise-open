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
#if NET_20
    static
#endif
    public class DomainJoinLogControl
    {
        public static void SetLogFile(string logFile)
        {
            logger.SetLogFile(logFile);
        }
    }

    public struct DomainJoinInfo : ICloneable
    {
        public string Name;
        public string Description;
        public string DnsDomain;		
        public string DomainName; // null if not joined
        public string DomainShortName; // null if not joined
        public string WorkgroupName; // null if joined
        public string LogFilePath; // null if not logging
		public string OrganizationalUnit; // null unless being set

        public object Clone()
        {
            return MemberwiseClone();
        }

        public override bool Equals(object obj)
        {
            if (obj is DomainJoinInfo)
            {
                DomainJoinInfo other = (DomainJoinInfo) obj;
                return ((0 == string.Compare(Name, other.Name)) &&
                        (0 == string.Compare(Description, other.Description)) &&
                        (0 == string.Compare(DnsDomain, other.DnsDomain, true)) &&
                        (0 == string.Compare(DomainName, other.DomainName, true)) &&
                        (0 == string.Compare(DomainShortName, other.DomainShortName, true)) &&
                        (0 == string.Compare(WorkgroupName, other.WorkgroupName, true)));
            }
            return false;
        }

        public override int GetHashCode()
        {
            return ToString().GetHashCode();
        }

        public override string ToString()
        {
            return
                string.Format("Name:{0};Description:{1};DnsDomain:{2};DomainName:{3};ShortName{4};WorkgroupName:{5}", Name,
                              Description, DnsDomain, DomainName, DomainShortName, WorkgroupName);
        }

    }

    public interface IDomainJoin
    {
        DomainJoinInfo QueryInformation();
        void JoinDomain(string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostFile);
        void JoinWorkgroup(string workgroupName, string userName, string password);
        void SetComputerName(string computerName);
        void SetFQDN(string computerName, string domainName);
        void SetComputerDescription(string description);
        bool IsDomainNameResolvable(string domainName);
        bool IsRootUser();
    }

    public enum DomainJoinClassType
    {
        Test,
        Real,
    }

#if NET_20
    static
#endif
    public class DomainJoinFactory
    {
        private static IDomainJoin Wrap(IDomainJoin inner)
        {
            return new LoggingDomainJoinWrapper(new ArgValidatingDomainJoinWrapper(inner));
        }

        public static IDomainJoin Create(DomainJoinClassType type)
        {
            switch (type)
            {
                case DomainJoinClassType.Test:
                    return Wrap(new TrivialDomainJoin());
                case DomainJoinClassType.Real:
                    return Wrap(new DomainJoin());
            }
            throw new ArgumentOutOfRangeException("type");
        }
    }

    class DomainJoin : IDomainJoin
    {
        public static DomainJoin Create()
        {
            return new DomainJoin();
        }

        public DomainJoinInfo QueryInformation()
        {
            DomainJoinInfo info = new DomainJoinInfo();
            info.Name = ComputerNameManager.GetComputerName();
            info.Description = AuthDaemonManager.GetConfiguredDescription();
            info.DomainName = AuthDaemonManager.GetConfiguredDomain();
            info.DnsDomain = "";

            if (info.DomainName != null && info.DomainName.Length == 0)
            {
                info.DomainName = null;
            }

            if (info.DomainName == null)
            {
                info.WorkgroupName = AuthDaemonManager.GetConfiguredWorkgroup();
                info.DomainShortName = null;
            }
            else
            {
                info.WorkgroupName = null;
                info.DomainShortName = AuthDaemonManager.GetConfiguredWorkgroup();
            }

            return info;
        }

        public void JoinDomain(string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostFile)
        {
            AuthDaemonManager.JoinDomain(domainName, userName, password, organizationalUnit, doNotChangeHostFile);
        }

        public void JoinWorkgroup(string workgroupName, string userName, string password)
        {
            AuthDaemonManager.JoinWorkgroup(workgroupName);
        }

        public void SetComputerName(string computerName)
        {
            ComputerNameManager.SetComputerName(computerName);
        }

        public void SetFQDN(string computerName, string domainName)
        {
            ParseHosts.SetDnsDomainNameForHostName(computerName, domainName);
        }

        public void SetComputerDescription(string description)
        {
            AuthDaemonManager.SetConfiguredDescription(description);
        }

        public bool IsDomainNameResolvable(string domainName)
        {
            return AuthDaemonManager.IsDomainNameResolvable(domainName);
        }

        public bool IsRootUser()
        {
            return AuthDaemonManager.IsRootUser();
        }
    }

    class TrivialDomainJoin : IDomainJoin
    {
        private string _computerName = "localhost";
        private string _dnsSuffix = "localdomain";
        private string _domainName = null;
        private string _workgroupName = "WORKGROUP";
        private string _description = "";

        public DomainJoinInfo QueryInformation()
        {
            DomainJoinInfo info = new DomainJoinInfo();
            info.Name = _computerName.ToLower();
            info.DnsDomain = _domainName != null ? _domainName.ToLower() : _dnsSuffix.ToLower();
            info.Description = _description;

            if (_domainName == null)
            {
                info.DomainShortName = null;
                info.WorkgroupName = _workgroupName;
            }
            else
            {
                info.DomainShortName = _domainName.Split(new char[] {'.'})[0].ToUpper();
                info.WorkgroupName = null;
            }

            info.DomainName = _domainName;

            return info;
        }

        public void JoinDomain(string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostFile)
        {
            _domainName = domainName;
        }

        public void JoinWorkgroup(string workgroupName, string userName, string password)
        {
            _domainName = null;
            _workgroupName = workgroupName;
        }

        public void SetComputerName(string computerName)
        {
            _computerName = computerName;
        }

        public void SetFQDN(string computerName, string domainName)
        {
            _computerName = computerName;
            _domainName = domainName;
        }

        public void SetComputerDescription(string description)
        {
            _description = description;
        }

        public bool IsDomainNameResolvable(string domainName)
        {
            return true;
        }

        public bool IsRootUser()
        {
            return true;
        }
	}

    class ArgValidatingDomainJoinWrapper : IDomainJoin
    {
        private IDomainJoin _inner;

        public ArgValidatingDomainJoinWrapper(IDomainJoin inner)
        {
            _inner = inner;
        }

        public DomainJoinInfo QueryInformation()
        {
            return _inner.QueryInformation();
        }

        public void JoinDomain(string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostFile)
        {
            if (domainName == null)
            {
                throw new ArgumentNullException("domainName");
            }
            if (domainName.Length == 0)
            {
                throw new ArgumentException("domainName cannot be empty", "domainName");
            }
            if (userName == null)
            {
                throw new ArgumentNullException("userName");
            }
            if (userName.Length == 0)
            {
                throw new ArgumentException("userName cannot be empty", "userName");
            }
            if (password == null)
            {
                throw new ArgumentNullException("password");
            }
            _inner.JoinDomain(domainName, userName, password, organizationalUnit, doNotChangeHostFile);
        }

        public void JoinWorkgroup(string workgroupName, string userName, string password)
        {
            if (workgroupName == null)
            {
                throw new ArgumentNullException("workgroupName");
            }
            if (workgroupName.Length == 0)
            {
                throw new ArgumentException("workgroupName cannot be empty", "workgroupName");
            }
            if (userName == null)
            {
                throw new ArgumentNullException("userName");
            }
            if (userName.Length == 0)
            {
                throw new ArgumentException("userName cannot be empty", "userName");
            }
            if (password == null)
            {
                throw new ArgumentNullException("password");
            }
            _inner.JoinWorkgroup(workgroupName, userName, password);
        }

        public void SetComputerName(string computerName)
        {
            if (computerName == null)
            {
                throw new ArgumentNullException("computerName");
            }
            if (!ComputerNameManager.IsValidComputerName(computerName))
            {
                throw new InvalidComputerNameException("invalid computer name " + computerName);
            }
            _inner.SetComputerName(computerName);
        }

        public void SetFQDN(string computerName, string domainName)
        {
            _inner.SetFQDN(computerName, domainName);
        }

        public void SetComputerDescription(string description)
        {
            if (description == null)
            {
                throw new ArgumentNullException("description");
            }
            _inner.SetComputerDescription(description);
        }

        public bool IsDomainNameResolvable(string domainName)
        {
            if (domainName == null)
            {
                throw new ArgumentNullException("domainName");
            }
            if (domainName.Length == 0)
            {
                throw new ArgumentException("domainName cannot be empty", "domainName");
            }

            return _inner.IsDomainNameResolvable(domainName);
        }

        public bool IsRootUser()
        {
            return _inner.IsRootUser();
        }
    }

    class LoggingDomainJoinWrapper : IDomainJoin
    {
        private IDomainJoin _inner;

        public LoggingDomainJoinWrapper(IDomainJoin inner)
        {
            _inner = inner;
        }

        private static void LogCall(string function, params object[] args)
        {
            string argsLine = "";
            foreach (object arg in args)
            {
                if (argsLine.Length != 0)
                {
                    argsLine += ", ";
                }
                if (arg == null)
                {
                    argsLine += "{null}";
                }
                else
                {
                    argsLine += arg.ToString();
                }

            }
            logger.debug("ENTER: " + function + "(" + argsLine + ")");
        }

        private static void LogResult(string function, Exception error)
        {
            string prefix = "EXIT: " + function + "(): ";
            if (error != null)
            {
                logger.error(prefix + error);
            }
            else
            {
                logger.debug(prefix + "SUCCESS");
            }
        }

        public DomainJoinInfo QueryInformation()
        {
            string function = "QueryInformation";
            Exception error = null;
            try
            {
                LogCall(function);
                DomainJoinInfo info = _inner.QueryInformation();
                info.LogFilePath = logger.GetLogFilePath();
                return info;
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                LogResult(function, error);
            }
        }

        public void JoinDomain(string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostFile)
        {
            string function = "JoinDomain";
            Exception error = null;
            try
            {
                LogCall(function, domainName, userName, (password != null) ? "{*}" : null, organizationalUnit, doNotChangeHostFile);
                _inner.JoinDomain(domainName, userName, password, organizationalUnit, doNotChangeHostFile);
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                LogResult(function, error);
            }
        }

        public void JoinWorkgroup(string workgroupName, string userName, string password)
        {
            string function = "JoinWorkgroup";
            Exception error = null;
            try
            {
                LogCall(function, workgroupName, userName, (password == null) ? "{null}" : "{*}");
                _inner.JoinWorkgroup(workgroupName, userName, password);
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                LogResult(function, error);
            }
        }

        public void SetComputerName(string computerName)
        {
            string function = "SetComputerName";
            Exception error = null;
            try
            {
                LogCall(function, computerName);
                _inner.SetComputerName(computerName);
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                LogResult(function, error);
            }
        }

        public void SetFQDN(string computerName, string domainName)
        {
            string function = "SetFQDN";
            Exception error = null;
            try
            {
                LogCall(function, computerName, domainName);
                _inner.SetFQDN(computerName, domainName);
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                LogResult(function, error);
            }
        }

        public void SetComputerDescription(string description)
        {
            string function = "SetComputerDescription";
            Exception error = null;
            try
            {
                LogCall(function, description);
                _inner.SetComputerDescription(description);
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                LogResult(function, error);
            }
        }

        public bool IsDomainNameResolvable(string domainName)
        {
            string function = "IsDomainNameResolvable";
            Exception error = null;
            try
            {
                LogCall(function, domainName);
                return _inner.IsDomainNameResolvable(domainName);
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                LogResult(function, error);
            }
        }

        public bool IsRootUser()
        {
            return _inner.IsRootUser();
        }
    }
}
