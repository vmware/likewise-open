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
#if NET_20
    static
#endif
    class TimeManager
    {
        // use our command line exe to test if we are running on vmware.
        // because if we are running on vmware, we have to worry about time!

        private static bool testVMWareHost()
        {
            return ExecUtils.RunCommandBoolean(Config.DetectVmWareCommand, 3);
        }


        // test to see if vmware-tools have been installed and are currently
        // running, if they are installed but not running, they are still of
        // no use to us.
        //
        // you would think that we would just do an /etc/init.d/vmware-tools
        // status, but that doesnt work properly in all cases; ie it returns
        // 0 when it's unconfigured and broken on SuSE.
        //
        // so, we just search for the process directly.

        private static bool testVMWareTools()
        {
            return ExecUtils.RunCommandBoolean("ps -C vmware-guestd", 1);
        }

        // we have 2 disjoint places where we have to know the name of the
        // ntp daemon, thus a function.  note that we return the name of the
        // daemon instead of being helpful and returning the whole path
        // because we have to deal with daemon management utilities that
        // expect to get only the daemonname.

        private static string NtpdDiscover()
        {
            if (File.Exists(new FileInfo("/etc/init.d/ntp").FullName))
                return "ntp";

            if (File.Exists(new FileInfo("/etc/init.d/ntpd").FullName))
                return "ntpd";

            if (File.Exists(new FileInfo("/etc/init.d/xntpd").FullName))
                return "xntpd";

            return "";
        }


        /// <summary> <code>SyncSystemTime</code>
        /// 
        /// Provides a mechanism to execute an ntpdate call with
        /// <code>sNameNTPHost</code> as the argument. Prior to execution the
        /// existence and running state of the ntp daemon is checked, and if
        /// it's running, it is stopped prior to calling ntpdate and then
        /// restarted.
        /// 
        /// </summary>
        /// <param name="sNameNTPHost">a <code>String</code> value of ntp server
        /// </param>
        /// <returns> a <code>String</code> value "ntp daemon left unstarted" or the output from the ntpd start.
        /// </returns>
        /// <exception cref="RemoteException">if an error occurs
        /// </exception>

        public static void SyncSystemTime(string sNameNTPHost)
        {
            String sNameNTPD = "";
            bool bTrueStoppedNTPD = false;

            // figure out if we actually *have* an ntpdate before we do
            // anything!  if NTPD exists and is running we will stop it.

            if (!"".Equals(sNameNTPD = NtpdDiscover()) && DaemonManager.GetDaemonStatus(sNameNTPD))
            {
                ExecUtils.RunCommand("/etc/init.d/" + sNameNTPD + " stop");
                bTrueStoppedNTPD = true;
            }

            // TODO figure out why ntpdate doesnt always work against AD
            ExecUtils.RunCommand("ntpdate " + sNameNTPHost);
            // TODO figure out why net time set -S doesnt always work against AD
            // ExecUtils.runCommand("net time set -S " + sNameNTPHost);

            //    logger.debug("About to call getADTime with sNameNTPHost:\n" + sNameNTPHost);
            //    Date dAD = ADLookup.getADTime(sNameNTPHost);
            //    logger.debug("AD Time from the Server is:\n  " + dAD);

            // 100712362005 time format that the 'date' command will accept
            //    String sDateTime =  new SimpleDateFormat("MMddHHmmyyyy").format(dAD);

            //    ProcessInfo PInfo1 = ExecUtils.runCommand("/bin/date ");
            //    logger.debug("Date output before the set is:\n " + PInfo1.getStdOut());

            //    ProcessInfo PInfo2 = ExecUtils.runCommand("/bin/date " + sDateTime);
            //    logger.debug("Date output after the set is:\n " + PInfo2.getStdOut());


            // having made it this far, we will be well behaved and restart
            // the ntpd daemon if it was started when we came in. note that it
            // wont get restarted if the ntpdate command throws an exception.

            if (bTrueStoppedNTPD)
            {
                ExecUtils.RunCommandWithOutput("/etc/init.d/" + sNameNTPD + " start");
            }
        }


        // point the system time at the Domain so that the machine stays in
        // sync with the domain using ntp

        public static void EnslaveSystemTimeNTP(String sNameHostNTP)
        {
            // only turn on centeris.com-vmtimefix if we are running on vmware
            // and vmware tools isnt running
            //
            // the daemon will exit with a nonzero error code if there isnt a
            // valid, reachable ad domain listed in smb.conf

            if (testVMWareHost() && !testVMWareTools())
            {
                DaemonManager.ManageDaemon("centeris.com-vmwtimefix", true);
            }

            string sPathConfNTP = "/etc/ntp.conf";
            string sNameNTPD = "";

            if ("".Equals(sNameNTPD = NtpdDiscover()))
            {
                throw new IOException("No NTP Daemon Found");
            }

            StreamWriter bw = new StreamWriter(sPathConfNTP, false, Encoding.ASCII);
            bw.Write("server " + sNameHostNTP);
            //UPGRADE_TODO: Method 'java.io.BufferedWriter.newLine' was converted to 'System.IO.TextWriter.WriteLine' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073'"
            bw.WriteLine();
            bw.Close();

            DaemonManager.ManageDaemon(sNameNTPD, true);
        }


        // point the system time at the Domain so that the machine stays in
        // sync with the domain using a cron job.
        // THIS IS NOT THE DESIRABLE WAY TO SYNC TIME!

        public static void EnslaveSystemTimeCron(String sNameDomain)
        {
            // first check to see that we are a member in good standing in the domain.
            // we will bail if we arent because we cant do a 'net time' without it

            // TODO: With 2 domain controllers, it's possible to join a domain
            // and have the test *fail* if it's run shortly after a join.
            // if(false == testDomain()) throw new Exception("Error: Domain Time Enslave: Domain relationship is not OK");

            // if we get this far, we should be good to go.

            // if it exists, stop ntp daemon and remove it from the boot sequence

            String sNameNTPD = "";

            if (!"".Equals(sNameNTPD = NtpdDiscover()))
            {
                DaemonManager.ManageDaemon(sNameNTPD, false);
            }

            // start to create or update the crontab file of the user we are
            // running as ( ie: root )

            StreamWriter bw = new StreamWriter("/tmp/cronadd", false, Encoding.ASCII);

            bw.Write("* * * * * " + Config.NetCommand + " time set -S " + sNameDomain + ">/dev/null 2>&1");

            // without the newline crontab will fail to add the entry(premature EOF error)

            //UPGRADE_TODO: Method 'java.io.BufferedWriter.newLine' was converted to 'System.IO.TextWriter.WriteLine' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073'"
            bw.WriteLine();
            bw.Close();

            // check for an existing crontab and if one exists, combine it
            // with our new crontab entry. otherwise, create a new one

            // Here we will catch the ExecException that can be thrown by runCommand(),
            // because it may just be signalling a nonzero return code.
            //
            // Because in this case we are asking crontab to print out a
            // crontab that *might* not exist and if it doesnt exist, we are
            // cool with that, because we can then just insert our own
            // crontab.
            //
            // we check out getRetCode() from the thrown exception to see if this is the case

            try
            {
                ExecUtils.RunCommand("crontab -l >/tmp/crontab.orig");
            }
            catch (ExecException ee)
            {
                if (ee.RetCode == 1)
                {
                    ExecUtils.RunCommand("crontab /tmp/cronadd");
                }
                else
                {
                    throw ee;
                } // if it wasn't that it was already there, there's some other problem...
            }

            // Since we got this far, we know we need to merge in our crontab entry, we
            // are back to re-throwing exceptions instead of using return codes

            ExecUtils.RunCommand("cat /tmp/crontab.orig /tmp/cronadd | uniq > /tmp/crontab.new");
            ExecUtils.RunCommand("crontab /tmp/crontab.new");
        }

    }

}