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
using System.Collections;
using System.IO;
using System.Runtime.Remoting;
using System.Text.RegularExpressions;
using System.Threading;

namespace Centeris.DomainJoinLib
{
#if NET_20
    static
#endif
    class AuthDaemonManager
    {
        public static string GetConfiguredDomain()
        {
            return GetSetting("realm");
        }

        public static string GetConfiguredWorkgroup()
        {
            return GetSetting("workgroup");
        }

        public static string GetConfiguredDescription()
        {
            return GetSetting("server string");
        }
        
        public static void SetConfiguredDescription(string value)
        {
            IniPreserve ini = new IniPreserve(Config.SmbConfPath);
            IniSection section = ini.getSection("global");
            section.putKey("server string", value);
            ini.replaceSection(section);
            ini.commit();

            // TODO: Determine full semantics for this call.
            // For instance, do we poke LWA?  Do we poke AD?
            // Do we poke smbd (killall -SIGUP smbd)?
            return;
        }

        private static void configurePamForADLogin(string shortDomainName)
        {
            ExecUtils.RunCommand(Config.ScriptDir + "/ConfigureLogin.sh " + ((null == shortDomainName) ? "disable" : "enable"));
        }

        private static string configureNameServiceMatchEvaluator(Match match)
        {
            string result = Regex.Replace(match.Value, "\\bwinbind\\b", "");
            // need to handle an upgrade from 3.0.0 
            result = Regex.Replace(result, "\\bcenteris\\b", "");
            result = result.TrimEnd();
            if (Regex.IsMatch(result, "\\blwidentity\\b"))
            {
                return result;
            }
            else
            {
                return result + " lwidentity";
            }
        }

        /// <summary>
        /// Configure name service switch configuration to support our
        /// NSS plug-in.  This include configuring /etc/nsswitch.conf
        /// and disabling the use of nscd (name service cache daemon).
        /// The reason for disabling the cache is because our NSS plugin
        /// does its own caching.
        /// </summary>
        private static void ConfigureNameServiceSwitch()
        {
            TextFileUtils.FakeSed("/etc/nsswitch.conf", "^\\s*(passwd|group)\\s*:(.*)$", new MatchEvaluator(configureNameServiceMatchEvaluator), false, null);
            TextFileUtils.FakeSed("/etc/nsswitch.conf", "^\\s*(passwd|group)\\s*:(.*)$", new MatchEvaluator(configureNameServiceMatchEvaluator), false, null);
            //ExecUtils.RunCommand(new String[] { "/bin/sed", "-i.bak", "/centeris/!s/^\\(passwd\\|group\\):.*$/& centeris/", "/etc/nsswitch.conf" });

            // if we have an nscd daemon, make sure that we dont ever run it!
            DaemonManager.ManageDaemon("nscd", false);
        }

        /// <summary> <code>configurePostJoin</code>.
        /// 
        /// Aggregate method that wraps all system configuration commands
        /// required after joining the domain.
        /// 
        /// </summary>
        /// <param name="domainName">should be null for leave</param>
        /// <param name="shortDomainName">should be null for leave</param>
        private static void configurePostJoin(string domainName, string shortDomainName)
        {
            //
            // If not already there, turn on winbind support for nsswitch.
            //

            ConfigureNameServiceSwitch();
            modifyKrb5Conf(domainName, shortDomainName);
            configurePamForADLogin(shortDomainName);
        }

        private static void ManageDaemons(bool bTrueOn, string shortDomainName)
        {
            if (bTrueOn)
            {
                DaemonManager.ManageDaemonAlways(Config.AuthDaemonName, bTrueOn, null, Config.AuthDaemonStartPriority, Config.AuthDaemonStopPriority);
            }
            else
            {
                DaemonManager.ManageDaemonAlways(Config.AuthDaemonName, bTrueOn, null, Config.AuthDaemonStartPriority, Config.AuthDaemonStopPriority);
            }
            
        }

        /// <summary> <code>testDomain</code>
        /// Determine if our host is on good relations with it's current domain
        /// </summary>
        /// <returns> a <code>boolean</code> value; true is AD join is ok, false if not
        /// </returns>
        /// <exception cref="ExecException">if an error occurs testing the domain join state
        /// </exception>
        private static bool testdomain()
        {
            try
            {
                string output = ExecUtils.RunCommandWithOutput(Config.NetCommand + " ads testjoin -P");
                return ( -1 != output.IndexOf("Join is OK") );
            }
            catch (ExecException e)
            {

                // TODO - it's possible that we might want to be more
                // discriminating in this catch block. currently, it's possible
                // for 'net ads testjoin' to block waiting for stdin, so rudec
                // coded runCommand to close stdin thus anything that we spawn
                // that actually wants it will fail. in the case of net ads
                // testjoin, it fails with a -1 in this case. we'll test and
                // return false for that, because the only time that net ads
                // testjoin ever cares about stdin is if it has a domain name in
                // smb.conf for which it cant find a machine
                // password. pragmatically, that implies that we arent currently
                // a member of a domain.

                if (-1 == e.RetCode)
                    return false;
                if (255 == e.RetCode)
                    return false;

                throw;
            }
        }



        //
        // <code>runLeaveDomainCommand</code>
        //
        // Does the low-level unjoin domain operation.  The caller should
        // disable Samba before calling this in case the leave causes weird
        // races wrt Samba.  This call does its best to leave the domain.
        // It should not fail as it masks out exceptions.
        //

        private static void runLeaveDomainCommand()
        {
            //
            // sRun is used to try to capture what is happening when an exception
            // is thrown.
            //

            String sRun = Config.NetCommand + " ads leave -P";

            //
            // Do the leave.  If this fails, that is ok.
            //

            try
            {
                ExecUtils.RunCommand(sRun);
            }
            catch (Exception e)
            {
                //
                // This is where we need to make sure we write out
                // all of the relevant exception information.
                // Ideally, we might want to fire off some event.
                //

                //UPGRADE_TODO: The equivalent in .NET for method 'java.lang.Throwable.toString' may return a different value. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1043'"
                logger.debug(sRun + " failed:\n" + e);
            }
        }

        /// <summary>
        /// Run the domain join command and extract (screenscrape) the
        /// short domain name that is output by the command.
        /// </summary>
        /// <param name="sNameDomain">Name of domain to join (FQDN).</param>
        /// <param name="sNameAdmin">Name of account that can join.</param>
        /// <param name="sPassAdmin">Password of account that can join.</param>
        /// <param name="organizationalUnit">OU to create computer account in (optional).</param>
        /// <returns>short domain name</returns>
        private static string RunJoinDomainCommand(string sNameDomain, string sNameAdmin, string sPassAdmin, string organizationalUnit)
        {
            // Make sure that we do the right thing with both old workgroup
            // and new style AD domain account names, ie:
            //
            //   DOMAIN\bob
            //   bob@domain.example.com
            //
            // If we are just passed 'bob', Delay lookup of group sids (domain
            // admins, centeris admins, enterprise admins) until absolutely
            // necessary. This is to reduce the need to call winbind
            // immediately after a domain join (something known to be
            // risky).  Then we use the domain name to make a UPN
            // bob@DomainToJoin.

            if (-1 == sNameAdmin.IndexOf("@") && -1 == sNameAdmin.IndexOf("\\"))
            {
                sNameAdmin += ("@" + sNameDomain);
            }

            // The realm has to be upper-case.  However, the name of
            // the user is case-insensitive (to AD).
            sNameAdmin = sNameAdmin.ToUpper();

            logger.debug("Attempting to join with administrative user: " + sNameAdmin);

            // get the OS name and version
            string sOSName = ExecUtils.RunCommandWithOutput("uname -s");
            string sOSVer = ExecUtils.RunCommandWithOutput("uname -r");
            logger.debug("OS name: " + sOSName + " OS version: " + sOSVer);

            //
            // Pass arguments using an array so that we handle user names
            // with spaces.
            //

            string[] command = new string[] { Config.NetCommand, "ads", "join", "-U" + sNameAdmin, "osName=" + sOSName, "osVer=" + sOSVer };
            if (organizationalUnit != null)
            {
                ArrayList al = new ArrayList(command);
                al.Add("createcomputer=" + organizationalUnit);
                command = (string[]) al.ToArray(typeof(string));
            }

            // give the command 5 minutes to run, then terminate it

            int timeoutSeconds = 5 * 60;

            Hashtable environmentVariables = new Hashtable();
            environmentVariables.Add("PASSWD", sPassAdmin);

            string output = ExecUtils.RunCommandWithOutput(command, timeoutSeconds, environmentVariables);

            logger.debug("Results from Domain Join: " + output);

            // Now do the screenscrape to get the domain short name.
            logger.debug("Searching for short domain name");
            Match match = Regex.Match(output, "-- (\\w|-)*");
            string matchString = match.Value;
            string shortDomainName = matchString.Substring(3);

            logger.debug("Found short domain name: " + shortDomainName);

            return shortDomainName;
        }

        private static void modifyKrb5Conf(String sDomainName, String sShortDomainName)
        {
            String command = "perl " + Config.ScriptDir + "/ConfigureKrb5.pl";

            if (null == sDomainName)
            {
                command += " --leave";
            }
            else
            {
                command += (" --join " + sDomainName);
                if (null != sShortDomainName)
                {
                    command += (" --short " + sShortDomainName);
                    command += (" --trusts");
                }
            }

            ExecUtils.RunCommand(command);
        }


        private class JoinHelper
        {
            private String typeOfName;
            private String name;
            public bool isDomain;
            public String domainName;
            public String workgroupName;
            public String workgroupNameOrig = "WORKGROUP";
            public IniPreserve ini;
            public IniSection globalSection;


            public virtual void setSmbConfigParameter(String keyName, String keyValue)
            {
                globalSection.putKey(keyName, keyValue);
            }

            public virtual void writeSmbConfig()
            {
                ini.replaceSection(globalSection);
                ini.commit();

                globalSection = ini.getSection("global");
            }

            private void readSmbConfig()
            {
                ini = new IniPreserve(Config.SmbConfPath);
                globalSection = ini.getSection("global");
            }

            internal JoinHelper(String sDomainOrWorkgroupName, bool isDomain)
            {
                //
                // We dont want the caller to try to join callee to an invalid
                // argument make sure that no carriage returns are part of the
                // string, then check the name again.
                //
                // TODO what regexp would define a valid Active Directory or WORKGROUP Name?
                //

                sDomainOrWorkgroupName = sDomainOrWorkgroupName.Trim().ToUpper();
                if (0 == sDomainOrWorkgroupName.Length)
                {
                    throw new Exception("Failed: empty domain/workgroup name");
                }

                this.isDomain = isDomain;
                name = sDomainOrWorkgroupName;

                if (isDomain)
                {
                    typeOfName = "domain";
                    domainName = sDomainOrWorkgroupName;
                }
                else
                {
                    typeOfName = "workgroup";
                    workgroupName = sDomainOrWorkgroupName;
                }

                // if this blows up, it means that we didnt have an smb.conf

                IniPreserve ini = new IniPreserve(Config.SmbConfPath);

                // we can live without having an initial global section because
                // doInstall() will create one later as a side effect

                try
                {
                    IniSection gSection = ini.getSection("global");
                    if (gSection.Keys.ContainsKey("workgroup"))
                    {
                        workgroupNameOrig = gSection.lookupKey("workgroup");
                    }
                }
                catch (Exception e)
                {
                    //UPGRADE_TODO: The equivalent in .NET for method 'java.lang.Throwable.getMessage' may return a different value. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1043'"
                    logger.error("Original Workgroup Name Lookup Failed: " + e.Message);
                }
            }

            //
            // On success samba is stopped.  On failure, samba is???
            //

            internal virtual void prepare()
            {
                logger.debug("Using " + typeOfName + "name: " + name);

                //
                // The basic steps are as follows:
                //
                // 1) Prepare to leave the domain.
                // 2) Try to leave the domain.
                // 3) Set up common Samba settings.
                //
                // Note that we will try to leave any configured domain
                // regardless of whether or not we are currently joined
                // to a domain.
                //

                //
                // Turn off daemons before trying to leave the domain.  There were
                // problems when certain operations in winbind/smbd raced wrt
                // leaving the domain.  We do not recall the specifics.  However,
                // they likely had to do with race conditions while trying to do domain
                // operations while leaving.
                //

                logger.debug("stopping daemons");
                ManageDaemons(false, null);

                logger.debug("Leaving Domain");
                runLeaveDomainCommand();
                logger.debug("Left Domain");

                //
                // Do common setup of smb.conf.  Note that samba should
                // not be running.
                //

                logger.debug("Initialize Configuration");
                InitializeConfigurationFile(Config.SmbConfPath);

                readSmbConfig();

                //
                // insert include directive for an optional debug file
                // will be ignored if it doesnt exist
                //

                setSmbConfigParameter("include", "/etc/samba/debug.conf");

                //
                // Configure workgroup setting.  Note that for a domain,
                // we will look like a default workgroup until the domain
                // join code is ready to run.  (That will allow for better
                // error handling.)
                //

                setSmbConfigParameter("workgroup", isDomain ? "WORKGROUP" : workgroupName);

                writeSmbConfig();

                configurePostJoin(null, null);
            }

            private static void InitGlobalSection(IniPreserve ini)
            {
                // see if we already have a global section
                IniSection section;
                try
                {
                    section = ini.getSection("global");
                }
                catch (IniSectionNotFound e)
                {
                    throw new ApplicationException("[global] section not found", e);
                }

                section.removeKey("realm");
                section.removeKey("workgroup");
                section.putKey("workgroup", "WORKGROUP");
                section.removeKey("security");
                section.putKey("security", "user");

                ini.replaceSection(section);
            }

            private static void InitializeConfigurationFile(string path)
            {
                //SambaShareInstaller.doInstall(Config.SmbConfPath, Config.PrefixDir + "/bin/");
                IniPreserve ini = new IniPreserve(path);
                InitGlobalSection(ini);
                ini.commit();
            }

            /// <summary>
            /// Bailout code used to patch up a failed domain join.
            /// Attemps to set config files to reasonable workgroup settings.
            /// We will set the workgroup to be the previous workgroup.
            /// Note that we only do the minimal repair needed to make us
            /// work in a workgroup so that the user can try a join again
            /// later.
            /// </summary>
            /// <exception cref="FileNotFoundException">if an error occurs finding the smb.conf
            /// </exception>
            /// <exception cref="IOException">if an error occurs reading the smb.conf
            /// </exception>
            /// <exception cref="IniSectionNotFound">if an error occurs reading global section from smb.conf
            /// </exception>
            /// <exception cref="ExecException">if an error occurs fixing up Krb5.conf
            /// </exception>
            public virtual void revertToOriginalWorkgroup()
            {
                IniPreserve ini = new IniPreserve(Config.SmbConfPath);
                IniSection globalSection = ini.getSection("global");
                if (globalSection.Keys.ContainsKey("realm"))
                {
                    globalSection.removeKey("realm");
                }
                globalSection.putKey("workgroup", workgroupNameOrig);
                globalSection.putKey("security", "user");
                ini.replaceSection(globalSection);
                ini.commit();
                modifyKrb5Conf(null, null); // change kr5 to not refer to a domain
            }
        }


        private static void finishJoin(string shortDomainName)
        {
            FileInfo cache = new FileInfo(Config.CacheTdb);
            logger.debug("deleting " + cache.Name + " if it is present");
            if (File.Exists(cache.FullName))
            {
                File.Delete(cache.FullName);
            }

            logger.debug("Turning on daemons post " + ((shortDomainName != null) ? "join" : "leave"));
            ManageDaemons(true, shortDomainName);

            try
            {
                refreshLwa();
            }
            catch (Exception e)
            {
                // ignore but log
                logger.debug("failed to refresh LWA:\n" + e);
            }
        }

        public static string JoinWorkgroup(String sNameWorkgroup)
        {
            logger.debug("Joining workgroup named: " + sNameWorkgroup);
            string trimmedWorkgroup = sNameWorkgroup;
            Exception error = null;

            try
            {
                JoinHelper helper = new JoinHelper(sNameWorkgroup, false);
                trimmedWorkgroup = helper.workgroupName;
                helper.prepare();
            }
            catch (Exception e)
            {
                error = e;
                throw;
            }
            finally
            {
                if (error != null)
                {
                    logger.debug("Abnormal termination while joining: " + trimmedWorkgroup + "\n" + error.StackTrace);
                }

                //
                // TODO-What if finishJoin throws?
                //

                finishJoin(null);
            }

            return trimmedWorkgroup;
        }

        /// <summary>
        /// Get the canonical "/" format of an OU name.
        /// </summary>
        /// <param name="organizationalUnit">fully-qualified OU path in
        /// "/" format (Foo/Bar) or LDAP format
        /// (ou=Bar,ou=Foo,dc=example,dc=com).</param>
        /// <param name="domainName">Name of domain we expect.</param>
        /// <returns>OU name in "/" format</returns>
        private static string CanonicalizeOrganizationalUnit(string organizationalUnit, string domainName)
        {
            string dnsDomain = null;
            string ouPath = null;
            string[] parts = organizationalUnit.Split(',');
            if (parts.Length < 2)
            {
                return organizationalUnit;
            }
            foreach (string part in parts)
            {
                part.Trim();
                Match match = Regex.Match(part, "^(dc|cn|ou)=(.+)$", RegexOptions.IgnoreCase);
                if (!match.Success)
                {
                    throw new InvalidOrganizationalUnitException(organizationalUnit);
                }
                string type = match.Groups[1].Value.ToLower();
                string component = match.Groups[2].Value;
                if (!type.Equals("dc"))
                {
                    if (dnsDomain != null)
                    {
                        throw new InvalidOrganizationalUnitException(organizationalUnit, "\"" + type + "\"= cannot occur after \"dc=\"");
                    }
                    if (ouPath != null)
                    {
                        ouPath = "/" + ouPath;
                    }
                    ouPath = component + ouPath;
                }
                else
                {
                    if (ouPath == null)
                    {
                        throw new InvalidOrganizationalUnitException(organizationalUnit, "missing OU path before \"dc=\"");
                    }
                    if (dnsDomain != null)
                    {
                        dnsDomain = dnsDomain + ".";
                    }
                    dnsDomain += component;
                }
            }
            if (SupportClass.IsNullOrEmpty(ouPath))
            {
                throw new InvalidOrganizationalUnitException(organizationalUnit, "missing OU");
            }
            if (SupportClass.IsNullOrEmpty(dnsDomain))
            {
                throw new InvalidOrganizationalUnitException(organizationalUnit, "missing domain name");
            }
            if (!domainName.ToLower().Equals(dnsDomain.ToLower()))
            {
                throw new InvalidOrganizationalUnitException(organizationalUnit, "\"" + dnsDomain + "\" does not match domain name \"" + domainName + "\"");
            }
            return ouPath;
        }

        /// <summary>
        /// Join a domain.
        /// </summary>
        /// <param name="domainName">Name of domain to join (FQDN).</param>
        /// <param name="userName">Name of account that can join.</param>
        /// <param name="password">Password of account that can join.</param>
        /// <param name="organizationalUnit">OU to create computer account in (optional).</param>
        /// <param name="doNotChangeHostFile">Do not change the /etc/hosts file on join.</param>
        /// <returns>short domain name</returns>
        public static string JoinDomain(string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostFile)
        {
            Exception error = null;

            logger.debug("Joining domain named: " + domainName);

            if (organizationalUnit != null)
            {
                organizationalUnit = CanonicalizeOrganizationalUnit(organizationalUnit, domainName);
            }

            //
            // We dont want the caller to try and join with a bogus hostname
            // like linux.site or a non RFC-compliant name.
            //         
            string hostName = ComputerNameManager.GetComputerName();
            if (!ComputerNameManager.IsValidComputerName(hostName))
            {
                throw new InvalidComputerNameException("Failed: hostname is invalid: " + hostName);
            }

            if (!doNotChangeHostFile)
            {
                //
                // Make sure that the hostname is fully and properly configured in the OS.
                //
                ComputerNameManager.SetComputerName(hostName);

                //
                // Before joining the domain, we need to fix up the /etc/hosts file
                //

                ParseHosts.SetDnsDomainNameForHostName(hostName, domainName);
            }

            string shortDomainName = null;
            JoinHelper helper = null;
            try
            {
                helper = new JoinHelper(domainName, true);
                domainName = helper.domainName;
                helper.prepare();

                //
                // insert the name of the AD into the realm property. samba's
                // net command doesnt take an argument for the realm or
                // workgroup properties, so we have to patch the smb.conf.
                //

                helper.setSmbConfigParameter("realm", domainName);
                helper.setSmbConfigParameter("security", "ads");
                helper.setSmbConfigParameter("use kerberos keytab", "yes");
                helper.writeSmbConfig();

                //
                // Set up krb5.conf with the domain as the Kerberos realm.
                // We do this before doing the join.  (We should verify whether
                // it is necessary to do so before trying to join, however.)
                //

                logger.debug("Modifying krb5.conf");
                modifyKrb5Conf(domainName, null);

                logger.debug("syncing system time to " + domainName);
                try
                {
                    TimeManager.SyncSystemTime(domainName);
                }
                catch (Exception e)
                {
                    //UPGRADE_TODO: The equivalent in .NET for method 'java.lang.Throwable.toString' may return a different value. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1043'"
                    logger.debug("Failed to sync system time: " + e);
                }

                logger.debug("Attempting to join domain");
                string workgroupName = RunJoinDomainCommand(domainName, userName, password, organizationalUnit);

                //
                // Now that we have joined the domain and collected the
                // workgroup information, we need to put it in smb.conf
                //

                logger.debug("Insert workgroup name in Global Section");

                helper.setSmbConfigParameter("workgroup", workgroupName);
                helper.writeSmbConfig();

#if DISABLED
                //
                // Change the system time to point into our new domain.
                //

                try
                {
                    sRun = "Enslaving System Time to " + sNameDomain + " Post Join";
                    logger.debug(sRun);
                    TimeManager.EnslaveSystemTimeNTP(sNameDomain);
                }
                catch (Exception e)
                {
                    logger.debug("Failed while " + sRun);
                    logger.debug("Exception: " + e);
                }
#endif

                //
                // Now that we have joined the domain successfully, we can point
                // our configuration files to reference winbind and start the
                // samba daemons and turn them in the boot process.
                //

                logger.debug("Configuring Naming and Auth Post Join");
                configurePostJoin(domainName, workgroupName);

                //
                // If we get this far, we want to propagate the workgroup
                // name out to the finally block.
                //

                shortDomainName = workgroupName;
            }
            catch (Exception e)
            {
                error = e;
                if (null != helper)
                {
                    try
                    {
                        // this code can fail, but we want the caller to get the original error
                        // not the one caused by this revert failing. note that if this fails
                        // the user will probably have to repair the smb.conf by hand before
                        // trying to join again
                        helper.revertToOriginalWorkgroup();
                    }
                    catch (Exception revisionException)
                    {
                        logger.error("Revert to Workgroup Failed: " + revisionException);
                    }
                }
                throw;
            }
            finally
            {
                if (null == shortDomainName)
                {
                    logger.debug("Cleaning up from abnormal termination");
                }
                if (null != error)
                {
                    logger.debug(error.ToString());
                }

                //
                // TODO-What if finishJoin throws?
                //

                finishJoin(shortDomainName);
            }

            return shortDomainName;
        }

		/// <summary>
		/// Returns TRUE if the given domain name can be "resolved" (by LDAP) to a particular domain
		/// controller
		/// </summary>
        public static bool IsDomainNameResolvable(string domainName)
		{
			//
			// sRun is used to try to capture what is happening when an exception
			// is thrown.
			//

            string command = Config.NetCommand + " lookup ldap " + domainName;

			//
			// Do the lookup.  If this fails, that is ok.
			//

			try
			{
                ExecUtils.RunCommand(command);
				return true;
			}
			catch (Exception e)
			{
                logger.debug(command + " failed:\n" + e);
				return false;
			}
		}

		/// <summary>
		/// Returns TRUE if the current user is the root user
		/// </summary>
		/// <returns></returns>
		public static bool IsRootUser()
		{
			// the program to figure out what user we are
			String sRun = "id -u";

			// different on Solaris
			string uname = ExecUtils.RunCommandWithOutput("uname");
			if (uname.Trim() == "SunOS")
				sRun = "/usr/xpg4/bin/id -u";

			//
			// Do the lookup.  If this fails, that is ok.
			//
			try
			{
				string sOut = ExecUtils.RunCommandWithOutput(sRun);
				return sOut.Trim()=="0";
			}
			catch (Exception e)
			{
			    // Must at least log error.
			    // TODO: Probably best to fail hard.
                logger.error("Could not determine uid\n" + e);
			}
		    return false;
		}

        private static string GetSetting(string settingName)
        {
            try
            {
                if (!File.Exists(new FileInfo(Config.SmbConfPath).FullName))
                {
                    throw new ApplicationException(Config.SmbConfPath + " missing");
                }

                //UPGRADE_TODO: Method 'java.util.HashMap.get' was converted to 'System.Collections.Hashtable.Item' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javautilHashMapget_javalangObject'"
                return (String) new IniPreserve(Config.SmbConfPath).getSection("global").Keys[settingName];
            }
            catch (Exception e)
            {
                throw new ApplicationException("Error getting: " + settingName + " from " + Config.SmbConfPath, e);
            }
        }

        private static void refreshLwa()
        {
            if (isLwaInstalled())
            {
		// do nothing
                ;
            }
        }

        private static bool isLwaInstalled()
        {
            return false;
        }

    }
}
