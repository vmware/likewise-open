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
using System.Runtime.InteropServices;
using System.IO;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Krb5
{
    //TODO: this needs to be kerberized; the current method exposes the password to all
    //users via the ps command, for the duration of the upload/download.
    public class LwioCopy
    {
        private static string smbclientPath = "/opt/likewise/bin/lwio-copy";

        //Sample : "-r -u administrator@PARENT4.LIKEWISEQA.COM /root/lactest/ /tmp/lactest/"
        private static string argFormatSSOFailed =
            "-r -u {0} {1} {2}";

        //Sample : @"-r -k /tmp/krb5cc_901776502 /tmp/PARENT4\\luser1/parent4.likewiseqa.com/SysVol/Policies/\{596B7C4F-3FA5-41F3-9D0A-D99B83A159CD\} //centerisone.parent4.likewiseqa.com/sysvol/parent4.likewiseqa.com/policies";
        private static string argFormat =
            "-r -k {0} {1} {2}";   

        private string localPath = null;
        private string remotePath = null;
        private CredentialEntry cCreds = null;
        private string sCredsPath = null;
        private string sSourcePath = null;
        private string sTargetPath = null;
        private IntPtr CrdesCache = IntPtr.Zero;
        private string sUserUPN = string.Empty;        

        public LwioCopy(CredentialEntry creds, string localPath, string remotePath)
        {            
            string sRealm=string.Empty;

            Logger.Log(String.Format(
                "domain={0}, user={1}, localPath={2}, remotePath={3}",
                creds.Domain,
                creds.UserName,
                localPath,
                remotePath),
                Logger.LWIOCopy);

            if (!Configurations.SSOFailed)
            {
                string UserID = DetermineUserID();

                sCredsPath = string.Format("/tmp/krb5cc_{0}", UserID);                

                Environment.SetEnvironmentVariable("KRB5CCNAME", sCredsPath.Trim());
            }
            else
            {
                //int ret = Krb5CredsCache.BuildCredsContext(creds.UserName, 
                //                                           creds.Password,
                //                                           creds.Domain, 
                //                                           out CrdesCache);
                uint ret = Krb5CredsCache.Krb5GetDomainPrincipalRealm(creds.Domain, out sRealm);
                if (ret != 0)
                {
                    Logger.Log("Krb5CredsCache.Krb5GetDomainPrincipalRealm failed" + ret.ToString());                    
                }
                sUserUPN = string.Concat(creds.UserName.ToLower(), "@", String.IsNullOrEmpty(sRealm) ? creds.Domain.ToUpper() : sRealm.ToUpper());
            }

            this.cCreds = creds;

            if (!String.IsNullOrEmpty(this.cCreds.UserName) && this.cCreds.UserName.IndexOf(@"\") >= 0)
            {
                int ibackwhack = this.cCreds.UserName.LastIndexOf(@"\");
                this.cCreds.UserName = this.cCreds.UserName.Substring(ibackwhack + 1);
            }  

            this.localPath = localPath;
            this.remotePath = remotePath;           
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
                    value), Logger.LWIOCopy);
                localPath = value;
            }
        }
        
        public string RemotePath
        {
            set
            {
                Logger.Log(String.Format(
                    "SMBClient.RemotePath = {0}",
                    value), Logger.LWIOCopy);
                remotePath = value;
            }
        }

        public string SourcePath
        {
            set
            {
                Logger.Log(String.Format(
                    "SMBClient.SourcePath = {0}",
                    value), Logger.LWIOCopy);
                sSourcePath = value;
            }
        }

        public string TargetPath
        {
            set
            {
                Logger.Log(String.Format(
                    "SMBClient.TargetPath = {0}",
                    value), Logger.LWIOCopy);
                sTargetPath = value;
            }
        }

        public void UploadFile(string fileName)
        {
            Logger.Log(String.Format(
                "SMBClient.UploadFile({0})",
                fileName), Logger.LWIOCopy);

            string arguments = string.Empty;

            SourcePath = localPath;

            //Adding the GPO GIUD directiry name to the Remote path since Lwio-copy tool works one level down from the base
            TargetPath = string.Concat(remotePath, @"/", fileName);

            if (Configurations.SSOFailed)
            {
                arguments = String.Format(argFormatSSOFailed,
                    sUserUPN,                    
                    sSourcePath,
                    sTargetPath);
            }
            else
            {
                arguments = String.Format(argFormat,
                         sCredsPath.Trim(),
                         sSourcePath,
                         sTargetPath);
            }

            Logger.Log(arguments, Logger.LWIOCopy);

            Process proc = new Process();

            proc.StartInfo.FileName = smbclientPath;
            proc.StartInfo.Arguments = arguments;

            Logger.Log(string.Concat(
                       proc.StartInfo.Domain,
                       proc.StartInfo.FileName,
                       proc.StartInfo.UserName,
                       proc.StartInfo.WorkingDirectory,
                       proc.StartInfo.ToString()),
                       Logger.LWIOCopy);

            if (Configurations.SSOFailed)
            {
                proc.StartInfo.UseShellExecute = false;
                proc.StartInfo.RedirectStandardInput = true;
                proc.StartInfo.RedirectStandardOutput = true;

                proc.Start();
                proc.WaitForExit(5000);
                if (!proc.HasExited)
                {
                    Logger.Log("Timed out in effort to procerss the command");
                }
                try
                {
                    proc.StandardInput.WriteLine(cCreds.Password);
                    proc.StandardInput.Flush();
                }
                catch (IOException ioe)
                {
                    Logger.LogException("LwioCopy :DownloadFile()", ioe);
                }
            }
            else
            {
                proc.Start();
                proc.WaitForExit(10000);
            }
        }

        public void DownloadFile(string fileName)
        {
            Logger.Log(String.Format(
                "SMBClient.DownloadFile({0})",
                fileName), Logger.LWIOCopy);

            string arguments = string.Empty;
           
            SourcePath = string.Concat(remotePath, @"\\", fileName);

            if (Configurations.SSOFailed)
            {
                arguments = String.Format(argFormatSSOFailed,
                    sUserUPN,
                    sSourcePath,
                    localPath);
            }
            else
            {
                arguments = String.Format(argFormat,
                         sCredsPath.Trim(),
                         sSourcePath,
                         localPath);
            }

            Logger.Log(arguments, Logger.LWIOCopy);

            Process proc = new Process();           
         
            proc.StartInfo.FileName = smbclientPath;
            proc.StartInfo.Arguments = arguments;

            if (Configurations.SSOFailed)
            {
                proc.StartInfo.UseShellExecute = false;
                proc.StartInfo.RedirectStandardInput = true;
                proc.StartInfo.RedirectStandardOutput = true;

                proc.Start();
                proc.WaitForExit(5000);
                if (!proc.HasExited)
                {
                    Logger.Log("Timed out in effort to procerss the command");
                }
                try
                {
                    proc.StandardInput.WriteLine(cCreds.Password);
                    proc.StandardInput.Flush();
                }
                catch (IOException ioe)
                {
                    Logger.LogException("LwioCopy :DownloadFile()", ioe);
                }
            }
            else
            {
                proc.Start();
                proc.WaitForExit(10000);
            }
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
