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
using System.Text;
using System.Diagnostics;
using System.IO;

namespace Likewise.LMC.Utilities
{
    //TODO: this needs to be kerberized; the current method exposes the password to all
    //users via the ps command, for the duration of the upload/download.
    public class SMBClient
    {
        private static string smbclientPath = "/opt/likewise/bin/lwismbclient";

        private static string argFormatSSOFailed =
            "{0} -U {1}\\\\{2}%{3} -k -c \"lcd {4}; cd {5}; prompt; recurse; {6} {7}; exit;\" &>/dev/null";

        private static string argFormat =
            "{0} -k -c \"lcd {1}; cd {2}; prompt; recurse; {3} {4}; exit;\" &>/dev/null";

        private string localPath = null;
        private string remotePath = null;
        private string changeDirectoryPath = null;
        private CredentialEntry creds = null;

        public SMBClient(CredentialEntry creds, string localPath, string remotePath, string changeDirectoryPath)
        {
            Logger.Log(String.Format(
                "domain={0}, user={1}, localPath={2}, remotePath={3}, changeDirectoryPath={4}",
                creds.Domain,
                creds.UserName,
                localPath,
                remotePath,
                changeDirectoryPath),
                Logger.SMBLogLevel);

            if (!Configurations.SSOFailed)
            {
                string UserID = DetermineUserID();

                string sCredsPath = string.Format("/tmp/krb5cc_{0}", UserID);

                Environment.SetEnvironmentVariable("KRB5CCNAME", sCredsPath.Trim());
            }

            this.creds = creds;

            if (!String.IsNullOrEmpty(this.creds.UserName) && this.creds.UserName.IndexOf(@"\") >= 0)
            {
                int ibackwhack = this.creds.UserName.LastIndexOf(@"\");
                this.creds.UserName = this.creds.UserName.Substring(ibackwhack + 1);
            }

            this.localPath = localPath;
            this.remotePath = remotePath;
            this.changeDirectoryPath = changeDirectoryPath;
        }

        public string DetermineUserID()
        {
            string UserIdResult = "";
            ProcessStartInfo startInfo = new ProcessStartInfo("id", "-u");

            startInfo.RedirectStandardOutput = true;
            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;
            startInfo.WindowStyle = ProcessWindowStyle.Hidden;

            Process process = Process.Start(startInfo);

            StreamReader output = process.StandardOutput;

            process.WaitForExit(5000);

            if (!process.HasExited)
            {
                throw new ApplicationException("Timed out in effort to determine UserID");
            }

            try
            {
                UserIdResult = output.ReadToEnd();
            }
            catch (IOException)
            {
                UserIdResult = "0";
            }

            return UserIdResult;
        }

        public string LocalPath
        {
            set
            {
                Logger.Log(String.Format(
                    "SMBClient.LocalPath = {0}",
                    value), Logger.SMBLogLevel);
                localPath = value;
            }
        }

        public string ChangeDirectoryPath
        {
            set
            {
                Logger.Log(String.Format(
                    "SMBClient.ChangeDirectoryPath = {0}",
                    value), Logger.SMBLogLevel);
                changeDirectoryPath = value;
            }
        }

        public string RemotePath
        {
            set
            {
                Logger.Log(String.Format(
                    "SMBClient.RemotePath = {0}",
                    value), Logger.SMBLogLevel);
                remotePath = value;
            }
        }

        public void UploadFile(string fileName)
        {
            Logger.Log(String.Format(
                "SMBClient.UploadFile({0})",
                fileName), Logger.SMBLogLevel);

            string arguments = string.Empty;

            if (Configurations.SSOFailed)
            {
                arguments = String.Format(argFormatSSOFailed,
                    remotePath,
                    creds.Domain,
                    creds.UserName,
                    creds.Password,
                    localPath,
                    changeDirectoryPath,
                    "mput",
                    fileName);
            }
            else
            {
                arguments = String.Format(argFormat,
                        remotePath,
                        localPath,
                        changeDirectoryPath,
                        "mput",
                        fileName);
            }

            Logger.Log(arguments, Logger.SMBLogLevel);

            Process proc = new Process();

            proc.StartInfo.FileName = smbclientPath;
            proc.StartInfo.Arguments = arguments;
            proc.Start();
            proc.WaitForExit(10000);
        }

        public void DownloadFile(string fileName)
        {
            Logger.Log(String.Format(
                "SMBClient.DownloadFile({0})",
                fileName), Logger.SMBLogLevel);

            string arguments = string.Empty;

            if (Configurations.SSOFailed)
            {
                arguments = String.Format(argFormatSSOFailed,
                    remotePath,
                    creds.Domain,
                    creds.UserName,
                    creds.Password,
                    localPath,
                    changeDirectoryPath,
                    "mget",
                    fileName);
            }
            else
            {
                arguments = String.Format(argFormat,
                    remotePath,
                    localPath,
                    changeDirectoryPath,
                    "mget",
                    fileName);
            }

            Logger.Log(arguments, Logger.SMBLogLevel);

            Process proc = new Process();

            proc.StartInfo.FileName = smbclientPath;
            proc.StartInfo.Arguments = arguments;
            proc.Start();
            proc.WaitForExit(10000);
        }

        public void DeleteDirectoryRecursively(string path)
        {
            try
            {
                string arguments = String.Format("-rf ", path);

                ProcessStartInfo startInfo = new ProcessStartInfo("rm", arguments);

                startInfo.RedirectStandardOutput = true;
                startInfo.UseShellExecute = false;
                startInfo.CreateNoWindow = true;
                startInfo.WindowStyle = ProcessWindowStyle.Hidden;

                System.Diagnostics.Process proc = Process.Start(startInfo);
                proc.WaitForExit();
            }
            catch (Exception ex)
            {
                Logger.LogException("SMBClient().DeleteDirectoryRecursively", ex);
            }
        }
    }
}
