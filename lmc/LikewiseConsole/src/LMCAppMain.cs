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
using System.Threading;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.Properties;
using Likewise.LMC.ServerControl;
using System.Net;
using System.IO;
using System.Xml;
using System.Reflection;

namespace Likewise.LMC
{
    public class LMCAppMain
    {
        #region Enum Variables
        private enum CmdLineParseMode
        {
            OPEN = 0,
            LOGLEVEL = 1,
            PLUGIN,
            PLATFORM,
            ERROR
        };

        private enum PluginTypes
        {
            ADUC = 0,
            CellManager = 1,
            EventViewer = 2,
            GPOE = 3,
            GPMC = 4,
            Kerberos = 5,
            Auth = 6,
            FileAndPrint = 7,
            LSA = 8,
            LUG = 9
        };

        #endregion

        #region Methods
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        private static void Main(string[] args)
        {
           // LdapTest.testing();

            string sConsoleFile = null;

            if (args.Length == 1 && !args[0].StartsWith("--"))
                sConsoleFile = args[0];

            // Enable themes
            Application.EnableVisualStyles();

            // Set the user interface to display in the
            // same culture as that set in Control Panel.
            Thread.CurrentThread.CurrentUICulture =
                Thread.CurrentThread.CurrentCulture;

            try
            {
                // process command line args
                if (args.Length>0 && args[0].StartsWith("--"))
                    parseCommandLineArgs(args, out sConsoleFile);

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Unknown) {
                    Configurations.determineTargetPlatform();
                }
                Configurations.setPlatformFeatures();
                Configurations.verifyEnvironment();

                Logger.Log("Likewise Management Console started");
                Logger.Log("Current LogLevel = " + Logger.currentLogLevel);

                //make sure a local, persistent, user-writeable data path exists.
                if (! System.IO.Directory.Exists(Application.UserAppDataPath))
                {
                    System.IO.Directory.CreateDirectory(Application.UserAppDataPath);
                }

                Hostinfo hnTemp = new Hostinfo();

                // create a mainform, show it and peg it
                LMCMainForm mf = new LMCMainForm(sConsoleFile);

                // Display the real form
                mf.TopLevel = true;
                mf.Show();

                // hide the splash window since we're getting ready to show a real one
                mf.Activate();

                // start running
                Application.Run(mf);      
            }
            catch (ShowUsageException)
            {                  
            }
            catch (DllNotFoundException ex)
            {
                Logger.LogMsgBox(String.Format(
                    "Likewise Management Console encountered a fatal error: Could not find DLL: {0}",
                    ex.Message));
                Logger.Log(String.Format("EXCEPTION (caught in LMCAppMain.cs): \n{0}",
                     ex), Logger.LogLevel.Panic);
            }
            catch (Exception ex)
            {
                int ex_id = 0;
                MessageBox.Show("Likewise Management Console encountered a fatal error.",
                    CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                Logger.Log(String.Format("EXCEPTION #{0} (caught in LMCAppMain.cs): \n{1}",
                    ex_id++, ex), Logger.LogLevel.Panic);

                for (Exception in_ex = ex.InnerException; in_ex != null; in_ex = in_ex.InnerException)
                {
                    Logger.Log(String.Format(
                        "\n\nEXCEPTION #{0}: \n{1}", ex_id++, in_ex), Logger.LogLevel.Panic);
                }
            }

            //Only in the debug build, make sure all the unmanaged objects have been freed
#if DEBUG
            Likewise.LMC.Utilities.DebugMarshal.CheckAllFreed();
#endif

            return;
        }
        

        /// <summary>
        /// Assimilates the command line arguments.
        /// </summary>
        /// <param name="args">command line arguments</param>
        private static void parseCommandLineArgs(string[] args, out string sConsoleFile)
        {            
            if (args.Length == 0)
            {
                sConsoleFile = null;
                // Nothing to do
                return;
            }
            sConsoleFile = null;
            string sOption = string.Empty;

            CmdLineParseMode parseMode = CmdLineParseMode.OPEN;
            foreach (string arg in args)
            {
                try
                {
                    switch (parseMode)
                    {
                        case CmdLineParseMode.OPEN:
                            {
                                if (arg.Equals("--loglevel"))
                                {
                                    parseMode = CmdLineParseMode.LOGLEVEL;
                                }
                                else if (arg.Equals("--plugin"))
                                {
                                    parseMode = CmdLineParseMode.PLUGIN;
                                }
                                else if (arg.Equals("--platform"))
                                {
                                    parseMode = CmdLineParseMode.PLATFORM;
                                }
                                else if (arg.Equals("--help") || arg.Equals("help") || arg.Equals("--h"))
                                {
                                    Console.WriteLine(Properties.Resources.HelpMsg);
                                    throw new ShowUsageException();
                                }
                                else
                                {
                                    string str = string.Format(Properties.Resources.Usage, arg);
                                    Console.WriteLine(str);
                                    throw new ShowUsageException();
                                }
                            }
                            break;
                        case CmdLineParseMode.LOGLEVEL:
                            {
                                if (arg.Equals("Silent") ||
                                    arg.Equals("Panic") ||
                                    arg.Equals("Error") ||
                                    arg.Equals("Normal") ||
                                    arg.Equals("Verbose") ||
                                    arg.Equals("Debug") ||
                                    arg.Equals("DebugTracing"))
                                {
                                    Logger.LogLevel x = (Logger.LogLevel)Enum.Parse(typeof(Logger.LogLevel), arg, true /* ignore case */);
                                    //
                                    // The logger is shared to all important parts of the application
                                    //
                                    Logger.currentLogLevel = x;
                                    parseMode = CmdLineParseMode.OPEN;
                                }
                                else
                                    throw new ShowWrongArgException();
                            }
                            break;
                        case CmdLineParseMode.PLUGIN:
                            {
                                if (arg.Equals("aduc") ||
                                    arg.Equals("cellmanager") ||
                                    arg.Equals("eventviewer") ||
                                    arg.Equals("gpmc") ||
                                    arg.Equals("fileandprint") ||
                                    arg.Equals("lsa") ||
                                    arg.Equals("auth") ||
                                    arg.Equals("kerberos") ||
                                    arg.Equals("lug"))
                                {
                                    sConsoleFile = UpdatePluginToConsole(arg);
                                    parseMode = CmdLineParseMode.OPEN;
                                }
                                else
                                    throw new ShowWrongArgException();
                            }
                            break;
                        case CmdLineParseMode.PLATFORM:
                            {
                                switch (arg)
                                {
                                    case "linux":
                                        parseMode = CmdLineParseMode.OPEN;
                                        Configurations.currentPlatform = LikewiseTargetPlatform.UnixOrLinux;
                                        break;

                                    case "mac":
                                        parseMode = CmdLineParseMode.OPEN;
                                        Configurations.currentPlatform = LikewiseTargetPlatform.Darwin;
                                        break;

                                    case "bsd":
                                        parseMode = CmdLineParseMode.OPEN;
                                        Configurations.currentPlatform = LikewiseTargetPlatform.BSD;
                                        break;

                                    case "windows":
                                        parseMode = CmdLineParseMode.OPEN;
                                        Configurations.currentPlatform = LikewiseTargetPlatform.Windows;
                                        break;

                                    default:
                                        throw new ShowWrongArgException();
                                }
                            }
                            break;

                        default:
                            {
                                string str = string.Format(Properties.Resources.Usage, arg);
                                Console.WriteLine(str);
                                throw new ShowUsageException();
                            }                            
                    }
                }
                catch (ShowWrongArgException)
                {
                    string str = string.Format(Properties.Resources.Usage, arg);
                    Console.WriteLine(str);
                    throw new ShowUsageException();
                }
                catch (ArgumentNullException)
                {
                    Logger.Log(
                        "Error: No log level was specified. Expecting one of {Silent,Panic,Error,Normal,Verbose,Debug}",
                        Logger.LogLevel.Error);
                    return;
                }
                catch (ArgumentException)
                {
                    Logger.Log(
                        "Error: An invalid log level was specified. Expecting one of {Silent,Panic,Error,Normal,Verbose,Debug}",
                        Logger.LogLevel.Error);
                    return;
                }               
                sOption = arg;
            }

            if (parseMode != CmdLineParseMode.OPEN)
            {
                string str = string.Format(Properties.Resources.Usage, sOption);
                Console.WriteLine(str);
                throw new ShowUsageException();
            }
        }

        private static string UpdatePluginToConsole(string args)
        {

            string sConsolePath = string.Empty;

#if QUARTZ
            sConsolePath = Application.StartupPath;
            sConsolePath = Path.Combine(sConsolePath, "iConsole.lmc");

            XmlDocument XmlDoc = new XmlDocument();
            try
            {
                XmlDoc.LoadXml(CommonResources.GetString("ConsoleSettings"));
                if (XmlDoc != null)
                {
                    XmlElement ViewsNode = (XmlElement)XmlDoc.GetElementsByTagName("Views")[0];
                    if (ViewsNode != null)
                    {
                        string dllsPath = "Likewise.LMC.Plugins";
                        string name = string.Empty;

                        XmlElement viewNode = ViewsNode.OwnerDocument.CreateElement("View");
                        viewNode.SetAttribute("NodeID", "0");
                        switch (args)
                        {
                            case "aduc":
                                name = "ADUCPlugin";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.dll", name));
                                break;

                            case "cellmanager":
                                name = "CellManagerPlugin";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.dll", name));
                                break;

                            //Commented since GPOE plugin is not an independent plugin to open it directly.
                            //Since it has it own requirement to connect to the domain before plugin to be added
                            //case "gpoe":
                            //    name = "GPOEPlugin";
                            //    dllsPath = string.Concat(dllsPath, string.Format(".{0}.dll", name));
                            //    break;

                            case "gpmc":
                                name = "GPMCPlugin";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.dll", name));
                                break;

                            case "fileandprint":
                                name = "FileAndPrint";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.dll", name));
                                break;

                            case "lug":
                                name = "LUGPlugin";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.unix.dll", name));
                                break;

                            case "lsa":
                                name = "LSAMgmtPlugin";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.unix.dll", name));
                                break;

                            case "kerberos":
                                name = "KerberosKeyTableMgmtPlugin";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.unix.dll", name));
                                break;

                            case "auth":
                                name = "AuthPlugIn";
                                dllsPath = string.Format("{0}.unix.dll", name);
                                break;

                            case "eventviewer":
                                name = "EventlogPlugin";
                                dllsPath = string.Concat(dllsPath, string.Format(".{0}.dll", name));
                                break;

                            default:
                                sConsolePath = null;
                                break;

                        }

                        XmlElement hostInfo = viewNode.OwnerDocument.CreateElement("HostInfo");

                        viewNode.SetAttribute("NodeName", name);
                        viewNode.SetAttribute("NodeDll", dllsPath);

                        viewNode.AppendChild(hostInfo);
                        ViewsNode.AppendChild(viewNode);
                    }
                }
                XmlDoc.Save(sConsolePath);
            }
            catch (Exception e)
            {
                Logger.LogException("LMCAppMain.cs: UpdatePluginToConsole", e);
            }
#endif
            return sConsolePath;
        }

        #endregion
    }
}