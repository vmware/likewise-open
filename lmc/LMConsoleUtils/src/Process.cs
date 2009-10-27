/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;

namespace Likewise.LMC.LMConsoleUtils
{
    public static class ProcessUtil
    {
        #region Class data
        public delegate void ProcessDoneCallback(int nReturnCode);
        private static Dictionary<Process, ProcessDoneCallback> alertTable = new Dictionary<Process, ProcessDoneCallback>();
        #endregion

        #region Static Interface
        static public Process ShellExec(string sDir, string sFilename, string sArgs, string sVerb)
        {
            // shell exec the agent installer
            ProcessStartInfo psi = new ProcessStartInfo();
            if (sDir == null || sDir == "")
                psi.WorkingDirectory = Environment.CurrentDirectory;
            else
                psi.WorkingDirectory = sDir;
            psi.UseShellExecute = true;
            psi.FileName = sFilename;
            if (psi.Arguments != null)
                psi.Arguments = sArgs;
            else
                psi.Arguments = "";
            if (sVerb != null && sVerb != "")
                psi.Verb = sVerb;
            else
                psi.Verb = "open";

            return Process.Start(psi);
        }

        static public Process ShellExec(string sFilename)
        {
            return ShellExec(null, sFilename, null, null);
        }

        static public Process Exec(string sDir, string sFilename, string sArgs)
        {
            // shell exec the agent installer
            ProcessStartInfo psi = new ProcessStartInfo();
            if (sDir == null || sDir == "")
                psi.WorkingDirectory = Environment.CurrentDirectory;
            else
                psi.WorkingDirectory = sDir;
            psi.UseShellExecute = false;
            psi.FileName = sFilename;
            psi.Arguments = sArgs;

            return Process.Start(psi);
        }

        static public void AlertWhenDone(Process p, ProcessDoneCallback cb)
        {
            // see if the process is already in the hashtable
            lock (alertTable)
            {
                if (alertTable.ContainsKey(p))
                    alertTable[p] += cb;
                else
                    alertTable[p] = cb;
            }

            // create a background worker thread to watch the process
            BackgroundWorker bg = new BackgroundWorker();
            bg.DoWork += new DoWorkEventHandler(DoWorkProc);
            bg.RunWorkerCompleted += new RunWorkerCompletedEventHandler(WorkerCompletedHandler);
            bg.RunWorkerAsync(p);
            
        }

        static public void RemoveAlerts(ProcessDoneCallback cb)
        {
            lock (alertTable)
            {
                Process[] arp = new Process[alertTable.Keys.Count];
                alertTable.Keys.CopyTo(arp, 0);
                foreach (Process p in arp)
                {
                    alertTable[p] -= cb;
                }
            }
        }

        static private void WorkerCompletedHandler(object sender, RunWorkerCompletedEventArgs e)
        {
            Process p = e.Result as Process;

            if (p != null)
            {
                // call the delegates
                ProcessDoneCallback cb = alertTable[p];
                if (cb!=null)
                    cb(p.ExitCode);

                // remove the item from the hashtable
                lock (alertTable)
                {
                    alertTable.Remove(p);
                }
            }
        }

        static private void DoWorkProc(object sender, DoWorkEventArgs e)
        {
            // get the process to watch
            Process p = e.Argument as Process;
            if (p == null)
                return;

            // wait for the process to finish
            p.WaitForExit();

            // set the return "value"
            e.Result = p;
        }
        #endregion
    }
}
