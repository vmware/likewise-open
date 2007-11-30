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
	public class SysConfigNetworkConfigFixer
	{
		private const bool enableTrace = true;
        private static readonly string className = typeof(SysConfigNetworkConfigFixer).FullName;
		
		static private void trace(string message)
		{
			if (enableTrace)
			{
				logger.debug(className + ": " + message);
			}
		}
		
		static private bool isNetworkManagerEnabled(string input)
		{
			string patternString = "^\\s*NETWORKMANAGER\\s*=\\s*(\\S+)";

            Match match = Regex.Match(input, patternString, RegexOptions.Multiline);
		    if ( match.Success )
		    {
                trace("FOUND: [" + match.Value + "]");
                if (0 == String.Compare(match.Groups[1].Value, "yes", true))
                {
                    return true;
                }
		    }
			
			return false;
		}
		
		static private int parseNetworkManagerOnlineTimeout(string input)
		{
			string patternString = "^\\s*NM_ONLINE_TIMEOUT\\s*=\\s*(\\S+)";

            Match match = Regex.Match(input, patternString, RegexOptions.Multiline);
		    if ( match.Success )
		    {
                trace("FOUND: [" + match.Value + "]");
                try
                {
                    return Int32.Parse(match.Groups[1].Value);
                }
                catch (FormatException)
                {
                    return 0;
                }
            }
			
			return 0;
		}
		
		static private void setNetworkManagerOnlineTimeout(string fileName, int minimumTimeout)
		{
			trace("Setting timeout to " + minimumTimeout);

		    string contents = FileUtils.ReadFile(fileName);

		    Match match = Regex.Match(contents, "^\\s*NM_ONLINE_TIMEOUT\\s*=.*$", RegexOptions.Multiline);
		    			
			int start = - 1;
			int end = - 1;
            while (match.Success)
            {
                start = match.Index;
                end = start + match.Length;

                trace("FOUND(" + start + "," + end + "): [" + match.Value + "]");
                int preStart = Math.Max(start - 10, 0);
                trace("PRE: [" + contents.Substring(preStart, start - preStart) + "]");
                trace("POST: [" + contents.Substring(end, Math.Min(10, contents.Length - end)) + "]");

                match = match.NextMatch();
            }
			
			//
			// We use quotes around the value to make the line look like
			// what is in there by default (at least on SLES10).
			//
			
			string configLine = "NM_ONLINE_TIMEOUT=\"" + minimumTimeout + "\"";
			
			string newChars;
			if (start < 0)
			{
				newChars = contents + "\n" + configLine + "\n";
			}
			else
			{
                newChars = contents.Substring(0, start) + configLine + contents.Substring(end, contents.Length - end);
			}
			
			FileUtils.ReplaceFile(fileName, newChars);
		}
		
		static public bool fixNetworkManagerOnlineTimeout(string fileName, int minimumTimeout)
		{
			//
			// We are using a bad try-catch pattern bellow because, in the Java
			// language, we have to catch each exception type individually so
			// that we can rethrow without falling back to adding Exception to the
			// throws clause...  This is all so that we can capture the exception
			// information in the log.  There must be a better way.
			//
			// Also, we do the logging here because the callers do not do the
			// proper logging.  When that is fixed, we can remove this logging
			// and the catch statement.
			//
			
			try
			{
				if (!File.Exists(new FileInfo(fileName).FullName))
				{
					trace(fileName + " does not exist");
					return false;
				}
				
				//
				// Use bash to do the parsing to guarantee that we are using
				// the same parsing rules as the system.
				//
				// We believe that this is safe on any system as the filename
				// passed into this function is /etc/sysconfig/network/config
				// (unless calling this on another file for testing),
				// which has the bash-sourced semantics.  The file is sourced
				// by /etc/init.d/network on SuSE systems.  On RedHat,
				// the file will not exist as /etc/sysconfig/network is a file.
				//
				
				string[] cmd = new string[]{"/bin/bash", "-c", "source \"" + fileName + "\" ; echo NM_ONLINE_TIMEOUT=$NM_ONLINE_TIMEOUT ; echo NETWORKMANAGER=$NETWORKMANAGER"};
                string output = ExecUtils.RunCommandWithOutput(cmd);
				
				trace("OUTPUT: [" + output + "]");
				
				if (!isNetworkManagerEnabled(output))
				{
					trace("NetworkManager is not enabled");
					return false;
				}
				
				int timeout = parseNetworkManagerOnlineTimeout(output);
				trace("Current timeout is " + timeout);
				if (timeout >= minimumTimeout)
				{
					return false;
				}
				
				setNetworkManagerOnlineTimeout(fileName, minimumTimeout);
				logger.debug("Set NetworkManager timeout to " + minimumTimeout);
				return true;
			}
			catch (ExecException e)
			{
				logger.debug("Abnormal exit:\n" + e);
				
				//
				// Propagate exception.  Note that this is a "rethrow" because we
				// do not instantiate a new exception, so the original stack is
				// preserved.
				//
				
				throw e;
			}
			catch (IOException e)
			{
				logger.debug("Abnormal exit:\n" + e);
				
				//
				// Propagate exception.  Note that this is a "rethrow" because we
				// do not instantiate a new exception, so the original stack is
				// preserved.
				//
				
				throw e;
			}
		}
		
		static private void usage()
		{
			Console.Error.WriteLine("usage: " + className + " fileName minimumTimeout");
			Console.Error.WriteLine();
			Console.Error.WriteLine("    Set the minimum timeout for NetworkManager (if enabled)");
			Console.Error.WriteLine();
		}
		
		static public void Main(string[] args)
		{
			if (args.Length != 2)
			{
				Console.Error.WriteLine("Incorrect number of arguments.");
				usage();
				Environment.Exit(1);
			}
			
			string fileName = args[0];
			string timeoutString = args[1];
			
			int timeout = 0;
			try
			{
				timeout = Int32.Parse(timeoutString);
			}
			catch (FormatException)
			{
				Console.Error.WriteLine("Invalid number format for timeout argument: " + timeoutString);
				usage();
				Environment.Exit(1);
			}
			
			if (timeout < 0)
			{
				Console.Error.WriteLine("timeout argument must be non-negative.");
				Environment.Exit(1);
			}
			
			try
			{
				if (fixNetworkManagerOnlineTimeout(fileName, timeout))
				{
					Console.Out.WriteLine("Set NetworkManager timeout to " + timeout);
				}
				else
				{
					Console.Out.WriteLine("NetworkManager timeout did not need to be set");
				}
				Environment.Exit(0);
			}
			catch (Exception e)
			{
				Console.Error.WriteLine("Failed to set timeout:\n" + e);
				Environment.Exit(1);
			}
		}
	}
}