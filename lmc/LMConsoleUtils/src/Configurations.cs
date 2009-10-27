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
using System.Diagnostics;
using System.IO;

namespace Likewise.LMC.LMConsoleUtils
{
    public enum LikewiseTargetPlatform
    {
        Unknown = 0,
        UnixOrLinux = 1,
        Windows = 2,
        Darwin = 3,   //Mac OS X
        BSD = 4
    }

    public class Configurations
    {
        public static bool useActionBox = true;
        public static bool useWaitForm = false;
        public static bool useListScrolling = true;

        //On Mono 1.9 on the mac, if you select first row X then row Y, rows X+1,X+2,...Y-1 will become bold.
        public static bool resetListViewFonts = false;
        public static bool resetLWTreeViewFonts = false;

        //On Mono 1.9 on the mac, there are frequent nullreference execeptions coming from MouseUpHandler.
        //Some Windows.Forms classes have been wrapped to handle this.
        public static bool provideDummyMouseHandlers = false;

        public static LikewiseTargetPlatform currentPlatform = LikewiseTargetPlatform.Unknown;
        public static string tempDirectory = null;

        public static int lwmgmtPerformanceInfoLevel = 0;
        public static bool SSOFailed = false;
        public static string sLIBRARY_PATH = string.Empty;
        private static string UserId = "";

        //Set the LIB_PATH if the LD_LIBRARY_PATH is empty.
        //Used when LAC rurring with IDEs Options: /opt/likewise/lib or /opt/likewise/lib64
        private static string sLIB_LIBRARY_PATH = "/opt/likewise/lib";
        private static string sLIB64_LIBRARY_PATH = "/opt/likewise/lib64";

        public static void verifyEnvironment()
        {
            string separator = "=============";

            Logger.Log(String.Format(
                "Checking Environment:\n{0}\nMONO_CONFIG = {1}",
                separator,
                Environment.GetEnvironmentVariable("MONO_CONFIG")));

            if (currentPlatform == LikewiseTargetPlatform.Darwin)
            {
                sLIBRARY_PATH = Environment.GetEnvironmentVariable("DYLD_LIBRARY_PATH");

                Logger.Log(String.Format(
                    "DYLD_LIBRARY_PATH = {0}",
                    sLIBRARY_PATH));

                if (!String.IsNullOrEmpty(sLIBRARY_PATH))
                {
                    if (sLIBRARY_PATH.Split(':').Length >= 1)
                        sLIBRARY_PATH = sLIBRARY_PATH.Split(':')[0];
                }
            }
            else
            {
                sLIBRARY_PATH = Environment.GetEnvironmentVariable("LD_LIBRARY_PATH");

                Logger.Log(String.Format(
                    "LD_LIBRARY_PATH = {0}",
                    sLIBRARY_PATH));

                if (!String.IsNullOrEmpty(sLIBRARY_PATH))
                {
                    if (sLIBRARY_PATH.Split(':').Length >= 1)
                        sLIBRARY_PATH = sLIBRARY_PATH.Split(':')[0];
                }
            }

            if (String.IsNullOrEmpty(sLIBRARY_PATH))
            {
                if (Directory.Exists(sLIB64_LIBRARY_PATH))
                    sLIBRARY_PATH = sLIB64_LIBRARY_PATH;
                else
                    sLIBRARY_PATH = sLIB_LIBRARY_PATH;
            }

            Logger.Log(separator);

            verifyLibraryExists("libsrvsvc");
            verifyLibraryExists("libnetapi");
            verifyLibraryExists("libldap");
            //verifyLibraryExists("liblwmgmtclient");

            Logger.Log(separator, Logger.manageLogLevel);
        }

        public static string GetUID
        {
            get
            {
                if (string.IsNullOrEmpty(UserId))
                {
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
                        UserId = output.ReadToEnd();
                    }
                    catch (IOException)
                    {
                        UserId = "0";
                    }
                }

                return UserId;
            }
        }

        public static void MakeDirectoryRecursive(string sPath)
        {
            try
            {               
                string arguments = String.Format("-p {0}", sPath);

                ProcessStartInfo startInfo = new ProcessStartInfo("mkdir", arguments);

                startInfo.RedirectStandardOutput = true;
                startInfo.UseShellExecute = false;
                startInfo.CreateNoWindow = true;
                startInfo.WindowStyle = ProcessWindowStyle.Hidden;

                System.Diagnostics.Process proc = Process.Start(startInfo);
                System.IO.StreamReader output = proc.StandardOutput;

                proc.WaitForExit(10000);
            }
            catch (Exception ex)
            {
                Logger.LogException("Configurations().MakeDirectoryRecursive", ex);
            }
        }        

        public static void determineTargetPlatform()
        {            
            if (Environment.OSVersion.Platform.Equals(PlatformID.Unix))
            {               
                string unameResult = "";
                ProcessStartInfo startInfo = new ProcessStartInfo("uname", "-s");

                startInfo.RedirectStandardOutput = true;
                startInfo.UseShellExecute = false;
                startInfo.CreateNoWindow = true;
                startInfo.WindowStyle = ProcessWindowStyle.Hidden;

                System.Diagnostics.Process proc = Process.Start(startInfo);

                try
                {
                    System.IO.StreamReader output = proc.StandardOutput;

                    proc.WaitForExit();

                    if (!proc.HasExited)
                    {
                        throw new ApplicationException("Timed out in effort to determine Operating System Information");
                    }

                    unameResult = output.ReadToEnd();                    
                }
                catch
                {
                    currentPlatform = LikewiseTargetPlatform.UnixOrLinux;                    
                }                

                if (unameResult.Contains("Darwin"))
                {
                    currentPlatform = LikewiseTargetPlatform.Darwin;
                }
                else
                {
                    currentPlatform = LikewiseTargetPlatform.UnixOrLinux;
                }
            }
            else
            {
                currentPlatform = LikewiseTargetPlatform.Windows;
            }
        }

        public static void setPlatformFeatures()
        {
            if (currentPlatform == LikewiseTargetPlatform.Darwin)
            {
                useActionBox = false;
                useWaitForm = false;
                useListScrolling = true;
                resetListViewFonts = true;
                resetLWTreeViewFonts = true;
                provideDummyMouseHandlers = true;
            }

            tempDirectory = Path.Combine(System.IO.Path.GetTempPath(), System.Environment.UserName);
        }

        public static void verifyLibraryExists(string basicLibraryName)
        {
            string libraryDirectory = sLIBRARY_PATH;//"/opt/likewise/lib";
            string fullLibraryName = null;
            string fullLibraryPath = null;

            switch (currentPlatform)
            {
                case LikewiseTargetPlatform.Windows:
                    return; //not implemented

                case LikewiseTargetPlatform.UnixOrLinux:
                    fullLibraryName = basicLibraryName + ".so";
                    break;

                case LikewiseTargetPlatform.Darwin:
                    fullLibraryName = basicLibraryName + ".dylib";
                    break;

                default:
                    fullLibraryName = basicLibraryName + ".so";
                    break;
            }

            fullLibraryPath = Path.Combine(libraryDirectory, fullLibraryName);

            verifyFileExists(fullLibraryPath);
        }

        public static void verifyFileExists(string path)
        {
            verifyFileExists(path, Logger.LogLevel.Normal);
        }

        public static void verifyFileExists(string path, Logger.LogLevel level)
        {
            FileInfo fi = new FileInfo(path);

            string status = "";

            if (fi != null && fi.Exists && fi.Length > 0)
            {
                status = "OK";
            }
            else
            {
                Logger.ShowUserError(String.Format("Warning: File Not Found: {0}", path));
                status = "MISSING";
            }

            Logger.Log(
                String.Format("Checking {0} ...... {1}", path, status),
                Logger.manageLogLevel);
        }

        public static bool StringsEqual(string str1, string str2)
        {
            bool result = true;

            if (str1 == null && str2 == null)
            {
                result = true;
            }
            else if (str1 == null && str2 != null)
            {
                result = false;
            }
            else if (str1 != null && str2 == null)
            {
                result = false;
            }
            else
            {
                result = str1.Equals(str2, StringComparison.InvariantCultureIgnoreCase);
            }
            return result;
        }

        public static bool StringStartsWith(string str1, string str2)
        {
            bool result = true;

            if (str1 == null || str2 == null)
            {
                result = false;
            }
            else
            {
                result = str1.StartsWith(str2, StringComparison.InvariantCultureIgnoreCase);
            }
            return result;
        }

    }
}
