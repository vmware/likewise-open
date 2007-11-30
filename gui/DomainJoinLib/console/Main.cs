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
using System.Diagnostics;
using System.Text.RegularExpressions;
using Centeris.DomainJoinLib;
using CommonOptions = Centeris.DomainJoinLib.Test.CommonOptions;

public class Program
{
    private static void Usage(string message)
    {
        if (message != null && message.Length != 0)
        {
            Console.WriteLine(message);
        }
        string programName = Test.GetProgramName();
        Console.WriteLine("usage: " + programName + " [options] command [args...]");
        Console.WriteLine();
        Console.WriteLine("  where options are:");
        Console.WriteLine();
        Console.WriteLine("    --help           Display this help information.");
        Console.WriteLine("    --test           Do not actually change anything.");
        Console.WriteLine("    --log {.|path}   Log to a file (or \".\" to log to console).");
        Console.WriteLine();
        Console.WriteLine("  and commands are:");
        Console.WriteLine();
        Console.WriteLine("    query");
        Console.WriteLine("    fixfqdn");
        Console.WriteLine("    setname <computerName>");
#if DISABLED
        Console.WriteLine("    setdesc <computerDescription>");
#endif
        Console.WriteLine("    join [--ou <organizationalUnit>] <domainName> <userName> [<password>]");
        Console.WriteLine("    leave");
        Console.WriteLine();
        Console.WriteLine("  Example:");
        Console.WriteLine();
        Console.WriteLine("    " + programName + " query");
        Console.WriteLine();
    }

    #region Commands

    private static bool Query(IDomainJoin joinInterface)
    {
        try
        {
            DomainJoinInfo info = joinInterface.QueryInformation();
            Console.WriteLine("Name = {0}", info.Name);
#if DISABLED
            Console.WriteLine("DnsDomain = {0}", info.DnsDomain);
#endif
            Console.WriteLine("Domain = {0}", info.DomainName);
#if DISABLED
            Console.WriteLine("Workgroup = {0}", info.WorkgroupName);
            Console.WriteLine("Description = {0}", info.Description);
#endif

            return true;
        }
        catch (Exception e)
        {
            Console.WriteLine("FAILURE");
            Console.WriteLine(e);
            return false;
        }
    }

    private static bool SetComputerName(IDomainJoin joinInterface, string computerName)
    {
        try
        {
            joinInterface.SetComputerName(computerName);
            Console.WriteLine("SUCCESS");
            return true;
        }
        catch (Exception e)
        {
            Console.WriteLine("FAILURE");
            Console.WriteLine(e);
            return false;
        }
    }

    private static bool SetFQDN(IDomainJoin joinInterface)
    {
        try
        {
            DomainJoinInfo info = joinInterface.QueryInformation();
            joinInterface.SetFQDN(info.Name, info.DomainName);
            Console.WriteLine("SUCCESS");
        }
        catch (Exception e)
        {
            Console.WriteLine("FAILURE");
            Console.WriteLine(e);
        }
        return true;
    }

#if DISABLED
    private static bool SetComputerDescription(IDomainJoin joinInterface, string description)
    {
        try
        {
            joinInterface.SetComputerDescription(description);
            Console.WriteLine("SUCCESS");
            return true;
        }
        catch (Exception e)
        {
            Console.WriteLine("FAILURE");
            Console.WriteLine(e);
            return false;
        }
    }
#endif

    private static bool JoinDomain(IDomainJoin joinInterface, string domainName, string userName, string password, string organizationalUnit, bool doNotChangeHostsFile)
    {
        try
        {
            joinInterface.JoinDomain(domainName, userName, password, organizationalUnit, doNotChangeHostsFile);
            Console.WriteLine("SUCCESS");
            return true;
        }
        catch (InvalidOrganizationalUnitException)
        {
            Console.WriteLine("FAILURE");
            Console.WriteLine("The join operation failed because the OU argument is invalid.");
            return false;
        }
        catch (Exception e)
        {
            Console.WriteLine("FAILURE");
            Console.WriteLine(e);
            return false;
        }
    }

    private static bool JoinWorkgroup(IDomainJoin joinInterface, string workgroupName, string userName, string password)
    {
        try
        {
            joinInterface.JoinWorkgroup(workgroupName, userName, password);
            Console.WriteLine("SUCCESS");
            return true;
        }
        catch (Exception e)
        {
            Console.WriteLine("FAILURE");
            Console.WriteLine(e);
            return false;
        }
    }

    #endregion

    private class UsageException : Exception
    {
        public UsageException() : base()
        {
        }
        public UsageException(string message) : base(message)
        {
        }
    }

    public static int Main(string[] args)
    {
        try
        {
            if (args.Length < 1)
            {
                throw new UsageException();
            }

            CommonOptions options = CommonOptions.Parse(args);
            if (options.IsHelp)
            {
                throw new UsageException();
            }
            else if (options.ArgError != null)
            {
                throw new UsageException(options.ArgError);
            }
            else if (!options.IsUnknown)
            {
                throw new UsageException("Missing command.");
            }
            else if (args[options.UnknownIndex].StartsWith("-"))
            {
                throw new UsageException("Unknown option: " + args[options.UnknownIndex]);
            }

            // check to make sure we're running as root
            IDomainJoin domainJoinInterface = DomainJoinFactory.Create(options.IsTest ? DomainJoinClassType.Test : DomainJoinClassType.Real);
            if (!domainJoinInterface.IsRootUser())
            {
                Console.WriteLine("Error: This program can only be run by the root user");
                return 1;
            }

            if (options.LogName != null)
            {
                DomainJoinLogControl.SetLogFile(options.LogName);
            }

		    if (options.UnknownIndex > 0)
            {
                string[] commandAndArgs = new string[args.Length - options.UnknownIndex];
                Array.Copy(args, options.UnknownIndex, commandAndArgs, 0, args.Length - options.UnknownIndex);
                args = commandAndArgs;
            }

            string command = args[0];
            switch (command)
            {
            case "configure":
                string[] commandAndArgs = new string[1 + args.Length];
                commandAndArgs[0] = "/usr/centeris/bin/lwi-domainjoin-cli";
                Array.Copy(args, 0, commandAndArgs, 1, args.Length);
                return ExecUtils.RunInConsole(commandAndArgs) == 0 ? 0 : 1;
            case "query":
                if (args.Length != 1)
                {
                    throw new UsageException();
                }
                return Query(domainJoinInterface) ? 0 : 1;
            case "setname":
                if (args.Length != 2)
                {
                    throw new UsageException();
                }
                return SetComputerName(domainJoinInterface, args[1]) ? 0 : 1;
            case "fixfqdn":
                return SetFQDN(domainJoinInterface) ? 0 : 1;
    #if DISABLED
            case "setdesc":
                if (args.Length != 2)
                {
                    throw UsageException();
                }
                return SetComputerDescription(domainJoinInterface, args[1]) ? 0 : 1;
    #endif
            case "join":
                int index = 1;
                string organizationalUnit = null;
                bool doNotChangeHostsFile = false;
                string password;
                while (index < args.Length)
                {
                    if (args[index] == "--ou")
                    {
                        if ((index + 1) >= args.Length)
                        {
                            throw new UsageException();
                        }
                        organizationalUnit = args[index + 1];
                        index += 2;
                    }
                    if (args[index] == "--nohosts")
                    {
                        doNotChangeHostsFile = true;
                        index += 1;
                    }
                    else
                    {
                        break;
                    }
                }
                switch (args.Length - (index-1))
                {
                    case 3:
                        // password not specified, need to prompt for it
                        Console.Write("Password: ");

                        // read a line with no echo on
                        password = ReadLineNoEcho();
                        break;
                    case 4:
                        password = args[index+2];
                        break;
                    default:
                        throw new UsageException();
                }
                return JoinDomain(domainJoinInterface, args[index], args[index+1], password, organizationalUnit, doNotChangeHostsFile) ? 0 : 1;
            case "leave":
                if (args.Length != 1)
                {
                    throw new UsageException();
                }
                return JoinWorkgroup(domainJoinInterface, "WORKGROUP", "empty", "") ? 0 : 1;
            default:
                Console.WriteLine("Unknown command: " + command);
                    throw new UsageException();
            }
        }
        catch (UsageException usageException)
        {
            Usage(usageException.Message);
            return 1;
        }
    }

    public static string ReadLineNoEcho()
    {
        // first, get the initial stty state
        string sState = GetSttyState();

        // now, disable echo and other options
        SetSttyState();

        // get the input
        string s = Console.ReadLine();

        // restore the tty state
        RestoreSttyState(sState);

        // to make the display look pretty
        Console.WriteLine();

        return s;
    }

    static string GetSttyState()
    {
        try
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.WorkingDirectory = Environment.CurrentDirectory;
            psi.FileName = "stty";
            psi.Arguments = "-a";
            psi.RedirectStandardOutput = true;
            psi.UseShellExecute = false;
            Process p = Process.Start(psi);
            p.WaitForExit();
            return p.StandardOutput.ReadToEnd();
        }
        catch(Exception)
        {
            return "";
        }
    }

    /// <summary>
    /// Turns off echo and icanon processing
    /// </summary>
    static void SetSttyState()
    {
        try
        {
            Process p = Process.Start("stty -echo");
            p.WaitForExit();
        }
        catch(Exception)
        {
        }
    }

    /// <summary>
    /// Restores previous stty state
    /// </summary>
    /// <param name="sState"></param>
    static void RestoreSttyState(string sState)
    {
        try
        {
            string sOptions = "";

            if (SttyOptionEnabled(sState, "echo"))
                sOptions += "echo ";
            if (sOptions!="")
            {
                Process p = Process.Start("stty " + sOptions);
                p.WaitForExit();
            }
        }
        catch(Exception)
        {
        }

    }

    static bool SttyOptionEnabled(string sState, string sOption)
    {
        // return true if sOption is present and -sOption is not
        return SttyOptionPresent(sState, sOption) && !SttyOptionPresent(sState, "-"+sOption);
    }

    static bool SttyOptionPresent(string sState, string sOption)
    {
        Regex re = new Regex(@"(^|\s)" + sOption + @"($^|\s)", RegexOptions.Multiline);
        return re.IsMatch(sState);
    }

}
