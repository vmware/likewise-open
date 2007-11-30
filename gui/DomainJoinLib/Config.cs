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
using System.IO;
using System.Reflection;

namespace Centeris.DomainJoinLib
{
#if NET_20
    static
#endif
    class Config
    {
        #region TimeManager
        public static readonly string DetectVmWareCommand;
        #endregion
        
        #region AuthDaemonManager
        public const string SmbConfPath = "/etc/samba/lwiauthd.conf";
        public const string AuthDaemonName = "likewise-open";
        public const string AuthDaemonCommand = "/etc/init.d/" + AuthDaemonName;
        public static readonly string[] DaemonNames = new string[] { AuthDaemonName };
        public static readonly string PrefixDir;
        public static readonly string ScriptDir;
        public const string CacheTdb = "/var/lib/likewise-open/winbindd_cache.tdb";
        public const string AuthDaemonStartPriority = "90";
	public const string AuthDaemonStopPriority = "10";
        #endregion

        #region Multiple
        public static readonly string NetCommand;
        #endregion
        
#if DISABLED
        private static readonly string[] sambaRestartCommands = new string[] { "/etc/init.d/smb", "/etc/init.d/nmb", "/etc/init.d/winbind" };
        private static readonly string[] sambaDaemons = new string[] { "smb", "nmb", "winbind" };
        private static readonly string sPAMSMB = "/etc/pam.d/samba";
        private static readonly string sSMBLIB = "pam_winbind.so";
#endif

        static Config()
        {
            //
            // This code assumes that the assembly is in <PrefixDir>/subdir/Assembly.ext
            //

            //
            // Note that Location does not return proper case on Windows.
            // CodeBase returns correct case but is URI format.
            //

            UriBuilder uri = new UriBuilder(Assembly.GetExecutingAssembly().CodeBase);
            string path = uri.Path;
            if (!File.Exists(path))
            {
                throw new FileNotFoundException("could not locate executing assembly", path);
            }
            string dirPath = Path.GetDirectoryName(path);
            PrefixDir = Path.GetDirectoryName(dirPath);

            ScriptDir = PrefixDir + "/bin";
            NetCommand = PrefixDir + "/bin/lwinet";
            DetectVmWareCommand = PrefixDir + "/bin/detectVMWare";
        }
    }
}
