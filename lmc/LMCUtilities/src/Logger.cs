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
using System.Windows.Forms;

namespace Likewise.LMC.Utilities
{
    public class Logger
    {
        #region Constants

        // maximum size of a log file before we decide to truncate it
        private const int cchMaxLogFileSize = 1024 * 1024;

        #endregion

        #region Class data

        // stream where Log output should go
        private static string sLogFilename = null;
        private static TextWriter twLog = null;

        private static bool use_timestamp = false;
        private static bool use_tag = false;

        public enum LogLevel
        {
            Silent=0,
            Panic=1,
            Error=2,
            Normal=3,
            Verbose=4,
            Debug=5,
            DebugTracing=6  //almost as verbose as tracing
        }

#if DEBUG
        public static LogLevel currentLogLevel = LogLevel.Debug;
#else
        public static LogLevel currentLogLevel = LogLevel.Error;
#endif

        public static LogLevel manageLogLevel = LogLevel.Verbose;

        public static LogLevel authLogLevel = LogLevel.Verbose;

        public static LogLevel ldapLogLevel = LogLevel.DebugTracing;

        public static LogLevel ldapTracingLevel = LogLevel.DebugTracing;

        public static LogLevel netAPILogLevel = LogLevel.Verbose;

        public static LogLevel timingLogLevel = LogLevel.DebugTracing;

        public static LogLevel eventLogLogLevel = LogLevel.Verbose;

        public static LogLevel LSAMgmtLogLevel = LogLevel.Verbose;

        public static LogLevel KerberosMgmtLogLevel = LogLevel.Verbose;

        public static LogLevel SMBLogLevel = LogLevel.Verbose;

        public static LogLevel GPMCLogLevel = LogLevel.Verbose;

        public static LogLevel GPOELogLevel = LogLevel.Verbose;

        public static LogLevel ADUCLogLevel = LogLevel.Verbose;

        public static LogLevel NetlogonLogLevel = LogLevel.Verbose;

        public static LogLevel FileBrowserLogLevel = LogLevel.Verbose;

		public static LogLevel FileShareManagerLogLevel = LogLevel.Verbose;

        public static LogLevel RegistryViewerLoglevel = LogLevel.Verbose;

        public static LogLevel ServiceManagerLoglevel = LogLevel.Verbose;

        public static LogLevel LWIOCopy = LogLevel.Verbose;

        public static LogLevel Krb5LogLevel = LogLevel.Verbose;

        #endregion

        #region Public methods

        public static string Filename
        {
            get {
                return sLogFilename;
            }
        }

        public static void LogMsgBox(string sMessage)
        {
            Log(sMessage, TraceEventType.Error, LogLevel.Error);
        }

        public static void ShowUserError(string sMessage)
        {
            Log(String.Format("ShowUserError: {0}", sMessage), LogLevel.Error);
            MessageBox.Show(limitLineLength(sMessage),
                    "Likewise Management Console",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
        }

        public static void Log(string sMessage)
        {
            Log(sMessage, TraceEventType.Information, LogLevel.Normal);
        }

        public static void Log(string sMessage, LogLevel level)
        {
            Log(sMessage, TraceEventType.Information, level);
        }

        public static void Log(string sMessage, TraceEventType tet, LogLevel level)
        {
            if(level > currentLogLevel || currentLogLevel == LogLevel.Silent)
            {
                return;
            }

            if (twLog == null)
            {
                try
                {
                    CreateLogFile();
                }
                catch (Exception e)
                {
                    Console.WriteLine(String.Format("Exception from CreateLogFile(): {0}", e.Message));
                    Console.WriteLine(String.Format(
                        "Failed to create log file in {0}.  Continuing with logging disabled.",
                        Configurations.tempDirectory == null ? "<null>" : Configurations.tempDirectory));
                    currentLogLevel = LogLevel.Silent;
                    return;
                }
                if (currentLogLevel == LogLevel.Silent) {
                    return;
                }
            }
            string logString = String.Format("{0}{1}{2}",
                                             (use_timestamp ? DateTime.Now.ToString("[yyyy-MM-dd hh:mm:ss,fff] ") : ""),
                                             (use_tag ? (" [" + tet.ToString() + "] ") : ""),
                                             sMessage);
            twLog.WriteLine(logString);
            twLog.Flush();

        }


        public static void Log(string sMessage, System.Diagnostics.TraceEventType tet, int nLevel)
        {
            Log(sMessage, tet, Logger.LogLevel.Panic);
        }

        public static void LogException(string methodName, Exception e)
        {
            Log(String.Format("Exception: Location={0} Type={1} \nMessage={2}",
                methodName, e.GetType(), e.Message));

            Log(e.StackTrace, Logger.LogLevel.Error);
        }

        public static void LogStackTrace(string message)
        {
            LogStackTrace(message, Logger.LogLevel.Error);
        }

        public static void LogStackTrace(string message, LogLevel level)
        {
            Log(String.Format("LogStackTrace: {0}", message), level);

            Log(System.Environment.StackTrace, level);
        }

        public static DateTime StartTimer()
        {
            return DateTime.Now;
        }

        public static string TimerMsg(ref DateTime timer, string msg)
        {
            return TimerMsg(ref timer, msg, Logger.timingLogLevel);
        }

        public static string TimerMsg(ref DateTime timer, string msg, LogLevel level)
        {

            DateTime currentTime = DateTime.Now;

            string timerMessage = String.Format("{0}: Elapsed Time: {1}", msg, currentTime - timer);

            Log(timerMessage, level);

            timer = DateTime.Now;

            return timerMessage;

        }

        public static string limitLineLength(string message)
        {
            const int MAX_CHARS = 80;

            //make sure the message shown is no longer, and no less than 80 characters per line.
            //This is a HACK to get around a Mono display bug.
            int msgLen = message.Length;
            int i = 0;
            int currentLineLength = 0;
            int lastWhiteSpace = -1;

            //first replace any existing newline-based formatting
            message.Replace('\n', ' ');

            Char[] charArr = message.ToCharArray();

            //add newlines to prevents lines from being longer than the screen size.
            for (; i < msgLen; i++, currentLineLength++)
            {
                if (Char.IsWhiteSpace(charArr[i]))
                {
                    lastWhiteSpace = i;
                }

                if (currentLineLength > MAX_CHARS)
                {
                    if (lastWhiteSpace > 0)
                    {
                        charArr[lastWhiteSpace] = '\n';
                        currentLineLength = 0;
                        lastWhiteSpace = -1;
                    }
                    else
                    {
                        //don't break up words
                    }
                }
            }
            return new string(charArr);
        }

        public static void Close()
        {
            if (twLog != null)
            {
                twLog.Close();
                twLog = null;
            }
        }

        #endregion

        #region Helpers

        /// <summary>
        /// Creates/opens the log file. Sets up twLog member.
        /// </summary>
        private static void CreateLogFile()
        {
            try
            {
                if (String.IsNullOrEmpty(Configurations.tempDirectory)) {
                    Configurations.tempDirectory = Path.Combine(System.IO.Path.GetTempPath(), System.Environment.UserName);
                }

                string tempDirectory = Configurations.tempDirectory;

                Directory.CreateDirectory(tempDirectory);

                string sLogFilename = Path.Combine(Configurations.tempDirectory, @"Likewise.LMC.log");

                System.Console.WriteLine(String.Format("Opening file {0} for logging.", sLogFilename));

                // see if the file needs to be truncated
                if (File.Exists(sLogFilename))
                {
                    // get the size
                    FileInfo fi = new FileInfo(sLogFilename);

                    if (fi.Length > cchMaxLogFileSize) {
                        // shave stuff off the top
                        FileStream fs = new FileStream(sLogFilename, FileMode.Open, FileAccess.ReadWrite);

                        // start at the end less 1/2 the allowable file size
                        int cHalf = cchMaxLogFileSize / 2;
                        fs.Seek(-cHalf, SeekOrigin.End);

                        // try to find a line break, but give up if we don't (TODO: unicode issues?)
                        for (int i = 0; i < 128; i++)
                        {
                            if (char.IsControl((char)fs.ReadByte()))
                            {
                                break;
                            }
                        }

                        // read the rest of the file into a big buffo
                        byte[] arb = new byte[cHalf];
                        int cchRead = fs.Read(arb, 0, cHalf);
                        if (cchRead > 0)
                        {
                            // now, rewind the file and write this stuff back out
                            fs.Seek(0, SeekOrigin.Begin);
                            fs.Write(arb, 0, cchRead);

                            fs.SetLength(cchRead);
                        }

                        // all done
                        fs.Close();
                    }
                }
                // open the stream for append mode
                twLog = new StreamWriter(sLogFilename, true);
            }
            catch (Exception e)
            {
                Console.WriteLine(String.Format("Exception from CreateLogFile(): {0}", e.Message));
                Console.WriteLine(String.Format(
                    "Failed to create log file in {0}.  Continuing with logging disabled.",
                    Configurations.tempDirectory == null ? "<null>" : Configurations.tempDirectory));
                currentLogLevel = LogLevel.Silent;
                return;
            }
        }
        #endregion
    }
}
