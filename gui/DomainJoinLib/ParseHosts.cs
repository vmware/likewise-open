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
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace Centeris.DomainJoinLib
{
#if NET_20
    static
#endif
    class ParseHosts
    {
        private static string HostsFile = "/etc/hosts";

        private class HostsLine
        {
            string originalLine;
            string ipAddress;
            string canonicalName;
            ArrayList aliases;
            string comment;

            public bool IsModified
            {
                get
                {
                    // logger.debug((originalLine == null) + ": " + GetLine());
                    return originalLine == null;
                }
            }

            private void MarkModified()
            {
                // logger.debug(new StackTrace(true).ToString());
                originalLine = null;
            }

            private string GetLine()
            {
                string result = "";
                if (ipAddress != null)
                {
                    result += ipAddress + " ";
                    if (canonicalName != null)
                    {
                        result += canonicalName + " ";
                        foreach (string alias in aliases)
                        {
                            result += alias + " ";
                        }
                    }
                }
                if (comment != null)
                {
                    result += "#" + comment;
                }
                return result.Trim();
            }

            public string Line
            {
                get
                {
                    string result;
                    if (IsModified)
                    {
                        result = GetLine();
                    }
                    else
                    {
                        result = originalLine;
                    }
                    return result;
                }
            }

            public string Comment
            {
                get
                {
                    return comment;
                }
                set
                {
                    MarkModified();
                    if (value != null)
                    {
                        comment = "# " + value;
                    }
                    else
                    {
                        comment = null;
                    }
                }
            }

            public string IPAddress
            {
                get
                {
                    return ipAddress;
                }
                set
                {
                    if (SupportClass.IsNullOrEmpty(value))
                    {
                        throw new ArgumentException("must be non-empty", "ipAddress");
                    }
                    MarkModified();
                    ipAddress = value;
                }
            }

            public string CanonicalName
            {
                get
                {
                    return canonicalName;
                }
                set
                {
                    if (SupportClass.IsNullOrEmpty(value))
                    {
                        throw new ArgumentException("must be non-empty", "canonicalName");
                    }
                    if (SupportClass.IsNullOrEmpty(ipAddress))
                    {
                        throw new ArgumentException("ipAddress must be set first", "canonicalName");
                    }
                    if (canonicalName != value)
                    {
                        MarkModified();
                        canonicalName = value;
                    }
                }
            }

            public string[] AliasesToArray()
            {
                if (aliases != null)
                {
                    return (string[])aliases.ToArray(typeof(string));
                }
                return null;
            }

            public void ReplaceAliases(string[] newAliases)
            {
                if (SupportClass.IsNullOrEmpty(canonicalName))
                {
                    throw new ArgumentException("canonicalName must be set first", "newAliases");
                }
                if (newAliases != null)
                {
                    aliases = new ArrayList(newAliases.Length);
                    aliases.AddRange(newAliases);
                }
                else
                {
                    aliases = new ArrayList(2);
                }
                MarkModified();
            }

            public HostsLine(string ipAddress, string canonicalName, params string[] aliases)
            {
                IPAddress = ipAddress;
                CanonicalName = canonicalName;
                ReplaceAliases(aliases);
            }

#if DISABLED
            public HostsLine(string comment, string ipAddress, string canonicalName, params string[] aliases)
            {
                Comment = comment;
                IPAddress = ipAddress;
                CanonicalName = canonicalName;
                ReplaceAliases(aliases);
            }

            public HostsLine(string comment)
            {
                Comment = comment;
            }
#endif

            private HostsLine()
            {
            }

            public static HostsLine Parse(string line)
            {
                HostsLine entry = new HostsLine();
                entry.originalLine = line;
                int index = line.IndexOf('#');
                if (index != -1)
                {
                    entry.comment = line.Substring(index + 1);
                    line = line.Substring(0, index);
                }
                string[] tokens = Regex.Split(line.Trim(), @"\s+");
                if (tokens.Length == 0 || (tokens.Length == 1 && tokens[0].Length == 0))
                {
                    // blank line -- note that we get a blank token from Split.
                }
                else
                {
                    entry.ipAddress = tokens[0];
                    if (tokens.Length > 1)
                    {
                        entry.canonicalName = tokens[1];
                        string[] aliases = new string[tokens.Length - 2];
                        Array.Copy(tokens, 2, aliases, 0, tokens.Length - 2);
                        entry.aliases = new ArrayList(tokens.Length - 2);
                        entry.aliases.AddRange(aliases);
                    }
                    else
                    {
                        entry.canonicalName = "";
                        entry.aliases = new ArrayList(2);
                    }
                }
                return entry;
            }

            public bool HasAlias(string alias)
            {
                for (int index = 0; index < aliases.Count; index++)
                {
                    if (SupportClass.IsEqualIgnoreCase(aliases[index] as string, alias))
                    {
                        return true;
                    }
                }
                return false;
            }

            public bool AddAliasFirst(string newAlias)
            {
                if (!HasAlias(newAlias))
                {
                    aliases.Insert(0, newAlias);
                    MarkModified();
                    return true;
                }
                return false;
            }

            public bool AddAlias(string newAlias)
            {
                if (!HasAlias(newAlias))
                {
                    aliases.Add(newAlias);
                    MarkModified();
                    return true;
                }
                return false;
            }

            public bool ReplaceAlias(string oldAlias, string newAlias)
            {
                bool modified = false;
                for (int index = 0; index < aliases.Count; index++)
                {
                    if (SupportClass.IsEqualIgnoreCase(aliases[index] as string, oldAlias) &&
                        !newAlias.Equals(aliases[index]))
                    {
                        aliases[index] = newAlias;
                        MarkModified();
                        modified = true;
                    }
                }
                return modified;
            }

            public bool RemoveAlias(string alias)
            {
                bool modified = false;
                for (int index = aliases.Count; index > 0; index --)
                {
                    if (SupportClass.IsEqualIgnoreCase(aliases[index] as string, alias))
                    {
                        aliases.RemoveAt(index);
                        MarkModified();
                        modified = true;
                    }
                }
                return modified;
            }

            public bool RemoveDuplicateAliases()
            {
                if (null == aliases || aliases.Count < 2)
                {
                    return false;
                }
                ArrayList unique = new ArrayList(aliases.Count);
                unique.Add(aliases[0]);
                for (int i = 1; i < aliases.Count; i++)
                {
                    if (!SupportClass.ContainsStringIgnoreCase(unique, (string)aliases[i]))
                    {
                        unique.Add(aliases[i]);
                    }
                }
                if (unique.Count != aliases.Count)
                {
                    aliases = unique;
                    MarkModified();
                    return true;
                }
                return false;
            }

            public bool NormalizeCase()
            {
                bool modified = false;
                if (ipAddress == null)
                {
                    goto cleanup;
                }
                if (!ipAddress.Equals(ipAddress.ToLower()))
                {
                    ipAddress = ipAddress.ToLower();
                    modified = true;
                }
                if (canonicalName == null)
                {
                    goto cleanup;
                }
                if (!canonicalName.Equals(canonicalName.ToLower()))
                {
                    canonicalName = canonicalName.ToLower();
                    modified = true;
                }
                if (aliases == null)
                {
                    goto cleanup;
                }
                for (int index = 0; index < aliases.Count; index++)
                {
                    string lowerAlias = ( aliases[index] as string ).ToLower();
                    if (!aliases[index].Equals(lowerAlias))
                    {
                        aliases[index] = lowerAlias;
                        modified = true;
                    }
                }
            cleanup:
                if (modified)
                {
                    MarkModified();
                }
                return modified;
            }
        }

        private static ArrayList ReadHostsFile(string path)
        {
            logger.debug("Reading hosts file " + path);
            StreamReader stream = new StreamReader(path, Encoding.Default);
            ArrayList entries = new ArrayList(10);
            string line;
            while ((line = stream.ReadLine()) != null)
            {
                HostsLine entry = HostsLine.Parse(line);
                entries.Add(entry);
            }
            stream.Close();
            // logger.debug("Finished reading hosts file " + path);
            return entries;

        }

        private static void Normalize(ArrayList entries)
        {
            foreach (HostsLine entry in entries)
            {
                entry.RemoveDuplicateAliases();
                entry.NormalizeCase();
            }
        }

        private static bool IsModified(ArrayList entries)
        {
            foreach (HostsLine entry in entries)
            {
                if (entry.IsModified)
                {
                    return true;
                }
            }
            return false;
        }

        private static void WriteHostsFile(string path, ArrayList entries)
        {
            logger.debug("Writing hosts file " + path);
            StreamWriter stream = new StreamWriter(path, false, Encoding.ASCII);
            foreach (HostsLine entry in entries)
            {
                stream.WriteLine(entry.Line);
            }
            stream.Close();
            // logger.debug("Finished writing hosts file " + path);
        }

        private static bool WriteHostsFileIfNeeded(string path, ArrayList entries)
        {
            Normalize(entries);
            if (IsModified(entries))
            {
                WriteHostsFile(path, entries);
                return true;
            }
            return false;
        }

        private static string GetHostPart(string name)
        {
            int index = name.IndexOf('.');
            if (index != -1)
            {
                return name.Substring(0, index);
            }
            else
            {
                return name;
            }
        }

        private static string GetDnsDomainPart(string name)
        {
            int index = name.IndexOf('.');
            if (index != -1)
            {
                return name.Substring(index + 1);
            }
            else
            {
                return null;
            }
        }

        private static HostsLine FindEntryByIPAddress(ArrayList entries, string IPAddress)
        {
            foreach (HostsLine entry in entries)
            {
                if (SupportClass.IsEqualIgnoreCase(entry.IPAddress, IPAddress))
                {
                    return entry;
                }
            }
            return null;
        }

        private static string ReplaceHostPart(string name, string hostName)
        {
            string result = hostName;
            string dnsDomainName = GetDnsDomainPart(name);
            if (dnsDomainName != null)
            {
                result += '.' + dnsDomainName;
            }
            return result;
        }

        public static void SetDnsDomainNameForHostName(string hostName, string dnsDomainName)
        {
            //
            // Ideal algorithm:
            //
            // 1) Find any lines with hostname.
            // 2) Make sure the the FQDN is present as the first
            //    name in each of those lines.
            // 3) If no lines were found, then add hostname to 127.0.0.1
            //    and put FQDN first.
            // 4) If 127.0.0.2 line is present, edit that to just have our info.
            //

            if (hostName.IndexOf('.') != -1)
            {
                throw new ArgumentException("host name cannot contain dots", "hostName");
            }

            dnsDomainName = dnsDomainName.Trim().ToLower();
            hostName = hostName.Trim().ToLower();

            string canonicalName = hostName + '.' + dnsDomainName;

            ArrayList entries = ReadHostsFile(HostsFile);
            bool found = false;
            bool weirdLoopbackIsOk = false;
            foreach (HostsLine entry in entries)
            {
                if (entry.CanonicalName != null)
                {
                    if (SupportClass.IsEqualIgnoreCase(entry.CanonicalName, hostName) ||
                        SupportClass.IsEqualIgnoreCase(entry.CanonicalName, canonicalName))
                    {
                        entry.CanonicalName = canonicalName;
                        // move old hostname to alias position so we do the processing below
                        // and to make sure we have an alias.
                        entry.AddAliasFirst(hostName);
                    }
                    if (entry.HasAlias(hostName))
                    {
                        found = true;
                        if (!SupportClass.IsEqualIgnoreCase(entry.CanonicalName, canonicalName))
                        {
                            entry.CanonicalName = canonicalName;
                        }
                        if (SupportClass.IsEqualIgnoreCase(entry.IPAddress, "127.0.0.2"))
                        {
                            weirdLoopbackIsOk = true;
                        }
                    }
                }
            }
            if (!found)
            {
                HostsLine entry = FindEntryByIPAddress(entries, "127.0.0.1");
                if (entry != null)
                {
                    // Preserve or add localhost, if missing
                    if (!SupportClass.IsEqualIgnoreCase(hostName, "localhost"))
                    {
                        entry.AddAliasFirst("localhost");
                    }
                    entry.CanonicalName = canonicalName;
                    entry.AddAlias(hostName);
                }
                else
                {
                    entry = new HostsLine("127.0.0.1", canonicalName, "localhost", hostName);
                    entries.Add(entry);
                }
            }

            if (!weirdLoopbackIsOk)
            {
                HostsLine entry = FindEntryByIPAddress(entries, "127.0.0.2");
                if (entry != null)
                {
                    entry.CanonicalName = canonicalName;
                    entry.ReplaceAliases(new string[] { hostName });
                }
            }

            WriteHostsFileIfNeeded(HostsFile, entries);
        }

        /// <summary> <code>SetHostName</code>
        ///
        /// Update the hosts file with the new hostname.
        ///
        /// </summary>
        /// <param name="oldHostName">a <code>String</code> value containing the old hostname
        /// </param>
        /// <param name="newHostName">a <code>String</code> value containing the new hostname
        /// </param>
        /// <exception cref="FileNotFoundException">if an error occurs finding the hosts file
        /// </exception>
        /// <exception cref="IOException">if an error occurs reading the hosts file
        /// </exception>

        public static void ReplaceHostName(string oldHostName, string newHostName)
        {
            //
            // Ideal algorithm:
            //
            // 1) Foreach line with old hostname as an alias
            //    a) Substitute hostname alias
            //    b) Substitute canonical if substituted alias (TODO: or check matching host part?)
            // 2) If no aliases were found...then add us to w/127.0.0.1
            // 3) If 127.0.0.2 line is present, edit that to just have our info.
            //

            newHostName = newHostName.Trim().ToLower();
            oldHostName = oldHostName.Trim().ToLower();

            logger.debug("Changing Host Name from " + oldHostName + " to " + newHostName);

#if DISABLED
            //
            // The name already exists; do nothing and exit
            //

            if (newHostName.Equals(oldHostName))
            {
                return;
            }
#endif

            ArrayList entries = ReadHostsFile(HostsFile);
            bool found = false;
            foreach (HostsLine entry in entries)
            {
                if (entry.CanonicalName != null)
                {
                    if (SupportClass.IsEqualIgnoreCase(entry.CanonicalName, oldHostName))
                    {
                        entry.CanonicalName = newHostName;
                        // move old hostname to alias position so we do the processing below
                        // and to make sure we have an alias.
                        if (!SupportClass.IsEqualIgnoreCase(entry.CanonicalName, oldHostName))
                        {
                            entry.AddAliasFirst(oldHostName);
                        }
                    }
                    if (entry.HasAlias(oldHostName))
                    {
                        bool modified;
                        if (SupportClass.IsEqualIgnoreCase(oldHostName, "localhost"))
                        {
                            modified = entry.AddAlias(newHostName);
                        }
                        else
                        {
                            modified = entry.ReplaceAlias(oldHostName, newHostName);
                        }
                        if (modified)
                        {
                            // TODO: always replace, or check the host part?
                            // Note that we must always replace the first found
                            if (!found || SupportClass.IsEqualIgnoreCase(GetHostPart(entry.CanonicalName), oldHostName))
                            {
                                entry.CanonicalName = ReplaceHostPart(entry.CanonicalName, newHostName);
                            }
                        }
                        found = true;
                    }
                }
            }
            if (!found)
            {
                HostsLine entry = FindEntryByIPAddress(entries, "127.0.0.1");
                if (entry != null)
                {
                    // Preserve or add localhost, if missing
                    if (!SupportClass.IsEqualIgnoreCase(newHostName, "localhost"))
                    {
                        entry.AddAliasFirst("localhost");
                    }
                    if (GetDnsDomainPart(entry.CanonicalName) != null)
                    {
                        entry.CanonicalName = ReplaceHostPart(entry.CanonicalName, newHostName);
                    }
                    entry.AddAliasFirst(newHostName);
                }
                else
                {
                    entry = new HostsLine("127.0.0.1", newHostName + ".localdomain", "localhost", newHostName);
                    entries.Add(entry);
                }
            }

            WriteHostsFileIfNeeded(HostsFile, entries);
        }

        private static void Usage()
        {
            Console.WriteLine("Invalid sub-command.  Valid sucommands:");
            Console.WriteLine("  hostname <hostsfile> <oldHostName> <newHostName>");
            Console.WriteLine("  dnsdomainname <hostsfile> <hostName> <dnsDomainName>");
            Environment.Exit(1);
        }

        [STAThread]
        public static void Main(string[] args)
        {
            try
            {
                if (args.Length < 1)
                {
                    Usage();
                }
                string command = args[0];
                switch (command)
                {
                    case "hostname":
                        if (args.Length != 4)
                        {
                            Usage();
                        }
                        Console.WriteLine("Setting hostname from {1} to {2} (in {0})", args[1], args[2], args[3]);
                        HostsFile = args[1];
                        ReplaceHostName(args[2], args[3]);
                        break;
                    case "dnsdomainname":
                        if (args.Length != 4)
                        {
                            Usage();
                        }
                        Console.WriteLine("Setting full name to {1}.{2} (in {0})", args[1], args[2], args[3]);
                        HostsFile = args[1];
                        SetDnsDomainNameForHostName(args[2], args[3]);
                        break;
                    default:
                        Usage();
                        break;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                Environment.Exit(1);
            }
        }
    }
}
