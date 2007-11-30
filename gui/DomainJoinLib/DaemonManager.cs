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

namespace Centeris.DomainJoinLib
{
    class NoDaemonsFoundException : Exception
    {
    }
#if NET_20
    static
#endif
    class DaemonManager
    {
        const string _chkconfigFile = "/sbin/chkconfig";
        const string _updateRcDFile = "/usr/sbin/update-rc.d";
//        string _startPriority = "20";
//        string _stopPriority = "20";
        
        public static void ManageDaemonArray(bool isStart, bool failOnMissing, string[] daemons)
        {
            bool bTrueHasFile = false;

            // we expect to find something to work on. so if we didnt find
            // something that implies an error. but sometimes the caller may
            // not care, so they are compelled to tell us if they care by the
            // bTrueFailOnMissing var.

            // loop thru the wad 'o daemon names and start them if we can find
            // them.  if the caller has told us that they care, toss an
            // exception if we cant find them.
            //
            // NB: yes, i know there are no {} here.

            for (int i = 0; i < daemons.Length; i++)
            {
                if (File.Exists(new FileInfo("/etc/init.d/" + daemons[i]).FullName))
                {
                    ManageDaemon(daemons[i], isStart);
                    bTrueHasFile = true;
                }
                else if (failOnMissing)
                {
                    throw new ExecException("Can't find daemon: " + daemons[i]);
                }
            }

            // If we dont find *any* daemons whatsoever, warn the user via the return string but dont toss an
            // exception, let the caller decide if they care.

            if (!bTrueHasFile)
            {
                throw new NoDaemonsFoundException();
            }
        }

        public static void ManageDaemonAlways(string daemon, bool isStart, string preCommand, string startPriority, string stopPriority)
        {
            if (!ManageDaemon(daemon, isStart, preCommand, startPriority, stopPriority))
            {
                throw new ExecException("Can't find daemon: " + daemon);
            }
        }

        public static bool ManageDaemon(string daemon, bool isStart)
        {
            return ManageDaemon(daemon, isStart, null);
        }

	public static bool ManageDaemon(string daemon, bool isStart, string preCommand)
        {
            return ManageDaemon(daemon, isStart, preCommand, "20", "20");
        }

        public static bool ManageDaemon(string daemon, bool isStart, string preCommand, string startPriority, string stopPriority)
        {
            string sNameDaemon = daemon;
            string sPathDaemon = "/etc/init.d/" + sNameDaemon;

            if (!File.Exists("/etc/init.d/" + daemon))
            {
                logger.debug("Daemon " + daemon + " does not exist.");
                return false;
            }

            // check our current state prior to doing anything.  notice that
            // we are using the private version so that if we fail, our inner
            // exception will be the one that was tossed due to the failure.

            bool bTrueStarted = GetDaemonStatus(sNameDaemon);

            // if we got this far, we have validated the existence of the
            // daemon and we have figured out if its started or stopped

            // if we are already in the desired state, do nothing.
            if (bTrueStarted != isStart)
            {
                StartStopDaemon(sNameDaemon, isStart, preCommand);
            }

            if (File.Exists(_chkconfigFile))
            {
                // if we got this far, we have set the daemon to the running state
                // that the caller has requested.  now we need to set it's init
                // state (whether or not it get's ran on startup) to what the
                // caller has requested. we can do this unconditionally because
                // chkconfig wont complain if we do an --add to something that is
                // already in the --add state.

                ExecUtils.RunCommand(_chkconfigFile + " " + (isStart ? "--add" : "--del") + " " + sNameDaemon);

                // this step is needed in some circumstances, but not all. i
                // *think* that this step is needed for LSB-1.2 only. furthermore
                // this step appears to turn off LSB-2.0 daemons, so we have to
                // make sure that we dont try to run it on them. so we check for
                // BEGIN INIT INFO. It's slower than i would like due to the need
                // to read the file in and do stringops on it. shelling out and
                // using grep *might* be faster, but it might not be, and it has
                // all the complexity associated with running native apps.

                string sRd = null;

                StreamReader br = new StreamReader(sPathDaemon, Encoding.Default);
                while (null != (sRd = br.ReadLine()))
                {
                    if (-1 != sRd.IndexOf("chkconfig:"))
                    {
                        ExecUtils.RunCommand(_chkconfigFile + " " + sNameDaemon + (isStart ? " on" : " off"));
                        break;
                    }
                    if (-1 != sRd.IndexOf("BEGIN INIT INFO"))
                        break;
                }
            }
            else if (File.Exists(_updateRcDFile))
            {
                //
                // TODO-2006/10/31-dalmeida -- Should not hardcode runlevels.
                //
                if (isStart)
                {
                    ExecUtils.RunCommand(_updateRcDFile + " " + sNameDaemon + " start " + startPriority + " 2 3 4 5 . stop " + stopPriority + " 0 1 6 .");
                }
                else
                {
                    ExecUtils.RunCommand(_updateRcDFile + " -f " + sNameDaemon + " remove");
                }
            }
            else
            {
                logger.debug("Could not find chkconfig-equivalent (" + daemon + ", " + isStart + ")");
                string uname = ExecUtils.RunCommandWithOutput("uname");
                if (uname.Trim() != "SunOS")
                {
                    throw new ExecException("Could not find chkconfig-equivalent (" + daemon + ", " + isStart + ")");
                }
            }

            logger.debug("Daemon: " + sNameDaemon + "  State:: New: " + isStart + "  Was: " + bTrueStarted);
            
            return true;
        }

        public static bool GetDaemonStatus(string sNameDaemon)
        {
            try
            {
                ExecUtils.RunCommand("/etc/init.d/" + sNameDaemon + " status");
                return true;
            }
            catch (ExecException e)
            {
                // see http://forgeftp.novell.com/library/SUSE%20Package%20Conventions/spc_init_scripts.html
                // and http://www.linuxbase.org/~gk4/wip-sys-init.html
                //
                // note further that some other error codes might be thrown, so
                // we choose to only pay attention to the ones that lsb says are
                // valid return codes for status that indicate that a progam
                // isnt running, otherwise, we are gonna throw it.

                if (1 == e.RetCode || 2 == e.RetCode || 3 == e.RetCode || 4 == e.RetCode)
                    return false;
                else
                    throw e;
            }
        }

        private static void StartStopDaemon(string sNameDaemon, bool isStart, string preCommand)
        {
            string command = "/etc/init.d/" + sNameDaemon + " " + (isStart ? "start" : "stop");
            if (preCommand != null)
            {
                command = "/bin/bash -c '" + preCommand + " ; " + command + "'";
            }
            ExecUtils.RunCommand(command);

            // recheck status to confirm that our goal was met.  this is
            // important because some daemons report a successful start, but
            // dont actually run (irda on rhel4 comes to mind)

            bool isStarted = GetDaemonStatus(sNameDaemon);
            if (isStarted != isStart)
            {
                throw new ExecException("Incorrect status after state change: " + sNameDaemon + " is " + isStarted + " instead of " + isStart);
            }
        }

    }

}
