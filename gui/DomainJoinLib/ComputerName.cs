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
using System.Text;
using System.Text.RegularExpressions;

namespace Centeris.DomainJoinLib
{

#if NET_20
    static
#endif
    class ComputerNameManager
    {
        private static string GetMachineSid()
        {
            string sOut = ExecUtils.RunCommandWithOutput(Config.NetCommand + " getlocalsid");
            // parse out the sid
            int ich = sOut.IndexOf(": ");
            if (ich > 0)
            {
                // zap the trailing cruft
                return sOut.Substring(ich + 2).Trim();
            }
            else
            {
                throw new GenericException("Improperly formatted result [" + sOut + "] from \'net getlocalsid\'");
            }
        }
        
        private static void SetMachineSid(string machineSid)
        {
            logger.debug("attempting to set machine SID after rename to [" + machineSid + "]");
            ExecUtils.RunCommand(Config.NetCommand + " setlocalsid " + machineSid);
        }
        
        private static string SafeHostName
        {
            get
            {
                try
                {
                    return GetComputerName().Trim();
                }
                catch (Exception)
                {
                    return "localhost";
                }
            }
        }
        
        public static string GetComputerName()
        {
            string result = ExecUtils.RunCommandWithOutput("hostname");
            
            if (null != result)
            {
                // we need to return the short name. however, hostname -s
                // 'works' in a somewhat feeble fashion. :-( All it looks for is
                // the first dot, and if the dot doesnt exist, then the command
                // fails ie:
                //
                // hostname -s bob
                //
                // fails because it has no dots!
                //
                // So we need to get the full name and strip it ourselves.

                int dotLocation = result.IndexOf('.');
                if (-1 != dotLocation)
                {
                    return result.Substring(0, dotLocation);
                }

                // need to remove any carriage returns if we didnt need to trim the string.
                result = result.Trim();
            }

            if (SupportClass.IsNullOrEmpty(result))
            {
                throw new ComputerNameNotFoundException();
            }

            return result;
        }

        public static bool IsValidComputerName(string inputHostname)
        {
            //
            // Rules:
            // - length must be between 1 and 15
            // - must not be "localhost" nor "linux"
            // - must not start or end with '-'
            // - chars must be alphanumeric or '-'
            //

            bool success = true;

            if (null == inputHostname ||
                inputHostname.Length < 1 ||
                inputHostname.Length > 15 ||
                "localhost".Equals(inputHostname.ToLower()) ||
                "linux".Equals(inputHostname.ToLower()) ||
                inputHostname.IndexOf('.') != -1 ||
                Regex.IsMatch(inputHostname, "^-.*") ||
                Regex.IsMatch(inputHostname, ".*-$") ||
                !Regex.IsMatch(inputHostname, "^[\\-a-zA-Z0-9]+$"))
            {
                success = false;
            }

            return success;
        }

        public static void SetComputerName(string computerName)
        {
            string sid = GetMachineSid();

            try
            {
                bool isDhcpHost = false;
                
                logger.debug("Changing hostname to : " + computerName);
                // Check the validity of the hostname.
                
                if (!IsValidComputerName(computerName))
                {
                    throw new InvalidComputerNameException("Error: Invalid Hostname: " + computerName);
                }

                computerName = computerName.ToLower();

                // Start spelunking for various hostname holding things. Rather
                // than trying to worry about what flavor of linux we are
                // running, we look for various files and fix them up if they
                // exist. That way we dont end up with a huge wad of repeated
                // code for each linux flavor.

                // change the repositories of the 'HOSTNAME' variable.
                // it's a string in /etc/HOSTNAME for some dists, it's a variable in
                // /etc/sysconfig/network for others

                // fixup HOSTNAME file if it exists
                // Ubuntu/Debian have /etc/hostname, so add that...

                string[] fileNames = new string[]{ "/etc/hostname", "/etc/HOSTNAME" };
                foreach (string fileName in fileNames)
                {
                    logger.debug("Checking for " + fileName);

                    if (File.Exists(fileName))
                    {
                        logger.debug("Changing " + fileName);
                        StreamWriter bw = new StreamWriter(fileName, false, Encoding.ASCII);
                        bw.WriteLine(computerName);
                        bw.Close();
                        logger.debug("Finished Changing " + fileName);
                    }
                }

                string uname = ExecUtils.RunCommandWithOutput("uname");
                if (uname.Trim() == "SunOS")
                {
                    FileUtils.WriteFile("/etc/nodename", computerName + "\n");
                    FileInfo[] files = new DirectoryInfo("/etc").GetFiles("hostname.*");
                    foreach (FileInfo file in files)
                    {
                        FileUtils.WriteFile(file.FullName, computerName + "\n");
                    }
                }

                // fixup HOSTNAME variable in /etc/sysconfig/network file if it exists
                // note that 'network' is a *directory* on some dists (ie SuSE),
                // is a *file* on others (ie Redhat). weird.

                logger.debug("Checking for /etc/sysconfig/network");

                if (File.Exists("/etc/sysconfig/network"))
                {
                    logger.debug("Changing /etc/sysconfig/network");
                    TextFileUtils.FakeSed("/etc/sysconfig/network", "^\\s*(HOSTNAME)\\s*=.*$", "$1=" + computerName, false, null);
                    //ExecUtils.RunCommand("sed -i.bak s/^\\(HOSTNAME=\\).*$/\\1" + computerName + "/ /etc/sysconfig/network");
                }

                logger.debug("Checking for /etc/sysconfig/");
                if (Directory.Exists("/etc/sysconfig"))
                {
                    // another place that needs to be updated is the ifcfg files but
                    // only if the interfaces is configured as DHCP. the ifcfg-eth
                    // file is different on different dists.

                    // we check that we have a redhat ifcfg path, if not, it's SuSE style

                    string sPathifcfg = "/etc/sysconfig/network-scripts/ifcfg-eth0";
                    string sPathifcfgBackup = "/etc/sysconfig/network-scripts/ifcfg-eth0.bak";
                    logger.debug("Checking for ifcfg-eth: " + sPathifcfg);

                    if (!File.Exists(sPathifcfg))
                        // redhat or SuSE?
                    {
                        sPathifcfg = null;
                        if (Directory.Exists("/etc/sysconfig/network"))
                        {
                            FileInfo[] files = new DirectoryInfo("/etc/sysconfig/network").GetFiles("ifcfg-eth*");
                            foreach (FileInfo file in files)
                            {
                                if (file.Name.StartsWith("ifcfg-eth-id-") ||
                                    file.Name.Equals("ifcfg-eth0") ||
                                    file.Name.StartsWith("ifcfg-eth-bus"))
                                {
                                    sPathifcfg = file.FullName;
                                    sPathifcfgBackup = "/etc/sysconfig/network/bak" + file.Name;
                                    break;
                                }
                            }
                        }

                        if (null == sPathifcfg)
                        {
                            throw new GenericException("Failed to find an ethernet interface config file");
                        }
                    }

                    logger.debug("Found ifcg-eth: " + sPathifcfg);

                    // now that we have a file, we need to check out our BOOTPROTO,
                    // if it's DHCP, we have to update the DHCP_HOSTNAME
                    // ps: the expression should be BOOTPROTO='?dhcp'? because RH uses dhcp and SuSE 'dhcp'
                    // sRun = "grep BOOTPROTO=\\'\\\\?dhcp\\'\\\\? " + sPathifcfg;
                    //sRun = "grep BOOTPROTO=\\'*dhcp\\'* " + sPathifcfg;

                    isDhcpHost = TextFileUtils.FakeGrep(sPathifcfg, "^.*BOOTPROTO.*dhcp.*$", false);
                    //isDhcpHost = ExecUtils.RunCommandBoolean("grep BOOTPROTO.*dhcp " + sPathifcfg, 1);
                    if (isDhcpHost)
                    {
                        logger.debug("IS DHCP Host");

                        // now look for DHCP_HOSTNAME, we will set it if it exists add it if it doesnt
                        // TODO: SuSE wraps vals in '', rhel doesnt, we are only doing the rhel way

                        bool haveDhcpHost = TextFileUtils.FakeGrep(sPathifcfg, "^\\s*DHCP_HOSTNAME\\s*=.*$", false);
                        //bool haveDhcpHost = ExecUtils.RunCommandBoolean("grep DHCP_HOSTNAME " + sPathifcfg, 1);
                        if (haveDhcpHost)
                        {
                            logger.debug("Updating DHCP_HOSTNAME = " + computerName);
                            TextFileUtils.FakeSed(sPathifcfg, "^\\s*(DHCP_HOSTNAME)\\s*=.*$", "$1=" + computerName, false, sPathifcfgBackup);
                            //ExecUtils.RunCommand("sed -i s/\\(DHCP_HOSTNAME=\\).*$/\\1" + computerName + "/ " + sPathifcfg);
                        }
                        else
                        {
                            logger.debug("Inserting DHCP_HOSTNAME=" + computerName);
                            TextFileUtils.Append(sPathifcfg, "\nDHCP_HOSTNAME=" + computerName + "\n", sPathifcfgBackup);
                            //ExecUtils.RunCommand("sed -i 1a\\DHCP_HOSTNAME=" + computerName + " " + sPathifcfg);
                        }
                    }
                }

                // insert/correct the new hostname in /etc/hosts - note that this
                // has to be done *before* we update the running hostname because
                // we call hostname to get the current hostname so that we can
                // find it and replace it.

                logger.debug("updating /etc/hosts to reflect new hostname: " + computerName);

                ParseHosts.ReplaceHostName(SafeHostName, computerName);

                logger.debug("calling hostname to set the current hostname to: " + computerName);

                // fix up the current state of the hostname

                ExecUtils.RunCommand("hostname " + computerName);

                // Only DHCP boxes need to restart their networks
                if (isDhcpHost)
                {
                    // fixup dhcp config file to tell the dhcp server not to give us a different one.

                    if (File.Exists(new FileInfo("/etc/sysconfig/network/dhcp").FullName))
                    {
                        logger.debug("Setting DHCLIENT_SET_HOSTNAME=no in /etc/sysconfig/network/dhcp");
                        TextFileUtils.FakeSed("/etc/sysconfig/network/dhcp", "^\\s*(DHCLIENT_SET_HOSTNAME)\\s*=.*$", "$1=\"no\"", false, null);
                        //ExecUtils.RunCommand("sed -i.bak 's/\\(DHCLIENT_SET_HOSTNAME=\\).*$/\\1\\\"no\\\"/' /etc/sysconfig/network/dhcp");
                    }

                    if (SysConfigNetworkConfigFixer.fixNetworkManagerOnlineTimeout("/etc/sysconfig/network/config", 60))
                    {
                        logger.debug("Fixed network manager online timeout");
                    }

                    // since we are a dhcp box, restart the network so that the rest of the network knows our new name
                    logger.debug("DHCP Host, restarting network");
                    ExecUtils.RunCommand("/etc/init.d/network restart");
                }
            }
            catch (Exception e)
            {
                logger.debug("Error setting hostname to " + computerName + ":\n" + e);
                throw e;
            }
            finally
            {
                //
                // This ensures that we do not change the SID after a machine name
                // change.  The issue here is that Samba implements its SAM such that
                // a machine name change changes the seeding used for the machine SID.
                // Therefore, we must re-store the old SID with the new machine name
                // seed.
                //

                SetMachineSid(sid);
            }
        }
    }

}
