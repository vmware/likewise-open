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
using System.Reflection;

namespace Centeris.DomainJoinLib
{
    public class Test
    {
        public static string GetProgramName()
        {
            string programName = Environment.GetEnvironmentVariable("CENTERIS_SCRIPT_WRAPPER");
            if (null == programName)
            {
                programName = Assembly.GetEntryAssembly().GetName().Name;
            }
            return programName;
        }

        public class CommonOptions
        {
			public const string sDefaultLogPath = "/tmp/lwidentity.join.log";

            public string ArgError;
            public bool IsHelp;
            public bool IsTest;
            public string LogName = sDefaultLogPath;
            public bool IsUnknown;
            public int UnknownIndex;

            public static CommonOptions Parse(string[] args)
            {
                CommonOptions result = new CommonOptions();
                if (args != null)
                {
                    bool done = false;
                    for (int i = 0; i < args.Length && !done; i++)
                    {
                        switch (args[i])
                        {
                            case "--help":
                                result.IsHelp = true;
                                break;
                            case "--test":
                                result.IsTest = true;
                                break;
                            case "--log":
                                if (i + 1 >= args.Length)
                                {
                                    result.ArgError = "Missing log name.";
                                }
                                else
                                {
                                    result.LogName = args[i + 1];
                                    i++;
                                }
                                break;
							case "--nolog":
								result.LogName = null;
								break;
                            default:
                                result.IsUnknown = true;
                                result.UnknownIndex = i;
                                break;
                        }
                        if (null != result.ArgError ||
                            result.IsHelp ||
                            result.IsUnknown)
                        {
                            done = true;
                        }
                    }
                }

                return result;
            }

        }

        private static void Usage()
        {
            string programName = GetProgramName();
            Console.WriteLine("usage: " + programName + " [options] command [args...]");
            Console.WriteLine();
            Console.WriteLine("  where options are:");
            Console.WriteLine();
            Console.WriteLine("    --help");
            Console.WriteLine("    --test");
            Console.WriteLine("    --log {.|path}");
			Console.WriteLine("    --nolog");
            Console.WriteLine();
            Console.WriteLine("  and commands are:");
            Console.WriteLine();
            Console.WriteLine("    IsValidComputerName <computerName>");
            Console.WriteLine("    exec <args...>");
            Console.WriteLine("    query");
            Console.WriteLine("    setname <computerName>");
            Console.WriteLine("    setdesc <computerDescription>");
            Console.WriteLine("    joindomain [--ou {organizationalUnit}] <domainName> <userName> <password>");
            Console.WriteLine("    joinwg <workgroupName> <userName> <password>");
            Console.WriteLine("    nmtest <args...>");
            Console.WriteLine("    parsehosts <args...>");
            Console.WriteLine();
            Console.WriteLine("  Example:");
            Console.WriteLine();
            Console.WriteLine("    " + programName + " query");
            Console.WriteLine();
        }

        #region Commands

        private static void Query(IDomainJoin joinInterface)
        {
            try
            {
                DomainJoinInfo info = joinInterface.QueryInformation();
                Console.WriteLine("Name = {0}", info.Name);
                Console.WriteLine("DnsDomain = {0}", info.DnsDomain);
                Console.WriteLine("Domain = {0}", info.DomainName);
                Console.WriteLine("Workgroup = {0}", info.WorkgroupName);
                Console.WriteLine("Description = {0}", info.Description);
            }
            catch (Exception e)
            {
                Console.WriteLine("FAILURE");
                Console.WriteLine(e);
            }
        }

        private static void SetComputerName(IDomainJoin joinInterface, string computerName)
        {
            try
            {
                joinInterface.SetComputerName(computerName);
                Console.WriteLine("SUCCESS");
            }
            catch (Exception e)
            {
                Console.WriteLine("FAILURE");
                Console.WriteLine(e);
            }
        }

        private static void SetComputerDescription(IDomainJoin joinInterface, string description)
        {
            try
            {
                joinInterface.SetComputerDescription(description);
            }
            catch (Exception e)
            {
                Console.WriteLine("FAILURE");
                Console.WriteLine(e);
            }
        }

        private static void JoinDomain(IDomainJoin joinInterface, string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostsFile)
        {
            try
            {
                joinInterface.JoinDomain(domainName, userName, password, organizationalUnit, doNotChangeHostsFile);
            }
            catch (Exception e)
            {
                Console.WriteLine("FAILURE");
                Console.WriteLine(e);
            }
        }

        private static void JoinWorkgroup(IDomainJoin joinInterface, string workgroupName, string userName, string password)
        {
            try
            {
                joinInterface.JoinWorkgroup(workgroupName, userName, password);
            }
            catch (Exception e)
            {
                Console.WriteLine("FAILURE");
                Console.WriteLine(e);
            }
        }

        #endregion

        public static int Main(string[] args)
        {
            if (args.Length < 1)
            {
                Usage();
                return 1;
            }

            CommonOptions options = CommonOptions.Parse(args);
            if (options.IsHelp)
            {
                Usage();
                return 1;
            }
            else if (options.ArgError != null)
            {
                Console.WriteLine(options.ArgError);
                Usage();
                return 1;
            }
            else if (!options.IsUnknown)
            {
                Console.WriteLine("Missing command.");
                Usage();
                return 1;
            }
            else if (args[options.UnknownIndex].StartsWith("-"))
            {
                Console.WriteLine("Unknown option: " + args[options.UnknownIndex]);
                Usage();
                return 1;
            }

            if (options.LogName != null)
            {
                logger.SetLogFile(options.LogName);
            }

            IDomainJoin domainJoinInterface = DomainJoinFactory.Create(options.IsTest ? DomainJoinClassType.Test : DomainJoinClassType.Real);

            if (options.UnknownIndex > 0)
            {
                string[] commandAndArgs = new string[args.Length - options.UnknownIndex];
                Array.Copy(args, options.UnknownIndex, commandAndArgs, 0, args.Length - options.UnknownIndex);
                args = commandAndArgs;
            }

            string command = args[0];
            switch (command)
            {
                case "nulltest":
                    string s = null;
                    Console.WriteLine("Hello " + s);
                    return 0;
                case "IsValidComputerName":
                    if (args.Length != 2)
                    {
                        Usage();
                        return 1;
                    }
                    bool result = ComputerNameManager.IsValidComputerName(args[1]);
                    Console.WriteLine(result);
                    return result ? 0 : 1;
                case "exec":
                {
                    if (args.Length < 2)
                    {
                        Usage();
                        return 1;
                    }
                    string[] restOfArgs = new string[args.Length - 1];
                    Array.Copy(args, 1, restOfArgs, 0, args.Length - 1);
                    string output = ExecUtils.RunCommandWithOutput(restOfArgs);
                    Console.WriteLine("OUTPUT:\n" + output);
                    return 0;
                }
                case "query":
                    if (args.Length != 1)
                    {
                        Usage();
                        return 1;
                    }
                    Query(domainJoinInterface);
                    return 0;
                case "setname":
                    if (args.Length != 2)
                    {
                        Usage();
                        return 1;
                    }
                    SetComputerName(domainJoinInterface, args[1]);
                    return 0;
                case "setdesc":
                    if (args.Length != 2)
                    {
                        Usage();
                        return 1;
                    }
                    SetComputerDescription(domainJoinInterface, args[1]);
                    return 0;
                case "joindomain":
                    switch (args.Length)
                    {
                        case 6:
                            if (args[1] != "--ou")
                            {
                                Usage();
                                return 1;
                            }
                            JoinDomain(domainJoinInterface, args[2], args[3], args[4], args[1], false);
                            break;
                        case 4:
                            JoinDomain(domainJoinInterface, args[1], args[2], args[3], null, false);
                            break;
                        default:
                            Usage();
                            return 1;
                    }
                    return 0;
                case "joinwg":
                    if (args.Length != 4)
                    {
                        Usage();
                        return 1;
                    }
                    JoinWorkgroup(domainJoinInterface, args[1], args[2], args[3]);
                    return 0;
                case "nmtest":
                {
                    string[] restOfArgs = new string[args.Length - 1];
                    Array.Copy(args, 1, restOfArgs, 0, args.Length - 1);
                    SysConfigNetworkConfigFixer.Main(restOfArgs);
                    return 0;
                }
                case "parsehosts":
                {
                    string[] restOfArgs = new string[args.Length - 1];
                    Array.Copy(args, 1, restOfArgs, 0, args.Length - 1);
                    ParseHosts.Main(restOfArgs);
                    return 0;
                }
                default:
                    Console.WriteLine("Unknown command: " + command);
                    Usage();
                    return 1;
            }
        }
    }
}
