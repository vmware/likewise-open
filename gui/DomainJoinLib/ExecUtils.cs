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
using System.Collections;
using System.Collections.Specialized;
using System.IO;
using System.Threading;
using System.Diagnostics;
using System.Text;

namespace Centeris.DomainJoinLib
{
    /// <summary>
    /// Contains the return result of an exec:
    /// - stdout
    /// - stderr
    /// - return code
    /// </summary>
    class ProcessInfo
    {
        virtual public System.String StdOut
        {
            get
            {
                return stdOut;
            }

        }

#if DISABLED
    virtual public System.String StdErr
    {
        get
        {
            return stdErr;
        }

    }
#endif

        virtual public int ReturnCode
        {
            get
            {
                return returnCode;
            }

        }

        protected internal System.String stdOut;
        protected internal System.String stdErr;
        protected internal int returnCode;

        public ProcessInfo(System.String stdOut, System.String stdErr, int returnCode)
        {
            this.stdOut = stdOut;
            this.stdErr = stdErr;
            this.returnCode = returnCode;
        }
    }

    /// <summary>
    /// ExecException is to be thrown whenever Runtime.exec("command") returns an unexpected error code.
    /// It should be instantiated with strings representing the command used, the stdOut and stdErr of the
    /// process, as well as the return code of the process (if any of these are available) which may be
    /// accessed by whoever catches the exception, to decide what to do with this issue.
    /// </summary>
    [Serializable]
    class ExecException : ApplicationException
    {
        private System.String sCommand;
        private System.String sStdOut;
        private System.String sStdErr;
        private int iRet;

#if DISABLED
    virtual public System.String Command
    {
        get
        {
            return sCommand;
        }

    }
    virtual public System.String StdOut
    {
        get
        {
            return sStdOut;
        }

    }
    virtual public System.String StdErr
    {
        get
        {
            return sStdErr;
        }

    }
#endif

        virtual public int RetCode
        {
            get
            {
                return iRet;
            }

        }

        /// <summary> If initialized with no information, set the return value to -100 as a default.</summary>
        public ExecException()
            : base()
        {
            sCommand = "";
            sStdOut = "";
            sStdErr = "";
            iRet = -100;
        }
        /// <summary> Constructors which don't set the return value leave it at the default of -(100+numArgsInConstructor) for
        /// debugging purposes
        /// </summary>
        /// <param name="msg">is passed up the super() call, and also set to be the stdErr if none is supplied
        /// </param>
        public ExecException(System.String msg)
            : base(msg)
        {
            sCommand = "";
            sStdOut = "";
            sStdErr = msg;
            iRet = -101;
        }
        /// <summary> </summary>
        /// <param name="msg">passed up to super(), but is no longer set into stdErr
        /// </param>
        /// <param name="cause">yeilds its stackTrace().toString into our stdErr variable
        /// </param>
        //UPGRADE_NOTE: Exception 'java.lang.Throwable' was converted to 'System.Exception' which has different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1100'"
        public ExecException(System.String msg, System.Exception cause)
            : base(msg, cause)
        {
            sCommand = "";
            sStdOut = "";
            sStdErr = "";
            iRet = -102;
        }
        /// <summary> </summary>
        /// <param name="msg">passed up to super(), but is no longer set into stdErr
        /// </param>
        /// <param name="command">The command we tried to execute.
        /// </param>
        /// <param name="cause">yeilds its stackTrace().toString into our stdErr variable
        /// </param>
        //UPGRADE_NOTE: Exception 'java.lang.Throwable' was converted to 'System.Exception' which has different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1100'"
        public ExecException(System.String msg, System.String command, System.Exception cause)
            : base(msg, cause)
        {
            sCommand = command;
            sStdOut = "";
            sStdErr = "";
            iRet = -102;
        }
        public ExecException(System.String stdOut, System.String stdErr)
            : base("Exec() error:" + "STDOUT: " + stdOut + '\n' + "STDERR: " + stdErr)
        {
            sCommand = "";
            sStdOut = stdOut;
            sStdErr = stdErr;
            iRet = -103;
        }
        public ExecException(System.String command, System.String stdOut, System.String stdErr)
            : base("Exec() error:" + "STDOUT: " + stdOut + '\n' + "STDERR: " + stdErr + '\n' + "Command: " + command)
        {
            sCommand = command;
            sStdOut = stdOut;
            sStdErr = stdErr;
            iRet = -104;
        }
        //UPGRADE_TODO: The equivalent in .NET for method 'java.lang.Throwable.getMessage' may return a different value. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1043'"
        //UPGRADE_TODO: The equivalent in .NET for method 'java.lang.Class.getName' may return a different value. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1043'"
        //UPGRADE_NOTE: Exception 'java.lang.Throwable' was converted to 'System.Exception' which has different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1100'"
        public ExecException(System.String command, System.String stdOut, System.String stdErr, int retCode, System.Exception cause)
            : base("Exec() error: " + "STDOUT: " + stdOut + '\n' + "STDERR: " + stdErr + '\n' + "Command: " + command + '\n' + "Cause: " + (null != cause ? (null != cause.Message ? cause.Message : cause.GetType().FullName) : "") + "\n" + "Return Code: " + retCode, cause)
        {
            sCommand = command;
            sStdOut = stdOut;
            sStdErr = stdErr;
            iRet = retCode;
        }
        public ExecException(System.String command, System.String stdOut, System.String stdErr, int retCode)
            : this(command, stdOut, stdErr, retCode, null)
        {
        }

#if DISABLED
    public ExecException(System.String command, ProcessInfo info)
        : this(command, info.stdOut, info.stdErr, info.returnCode, info.exception)
    {
    }
#endif
    }

#if NET_20
    static
#endif
    public class ExecUtils
    {
        class ReadWholeStream
        {
            private StreamReader readFrom;
            public string result = "";
            public ReadWholeStream(StreamReader from)
            {
                readFrom=from;
            }
            public string Result
            {
                get
                {
                    return result;
                }
            }
            public void ReadToEnd()
            {
                result = readFrom.ReadToEnd();
            }
        }

        private static string QuoteAsNeeded(string argument)
        {
            //
            // We should escape the quoting character with backslashes.
            // Note that quoting can be done with " or ' (the latter at
            // least when using Mono), but we just always use ".
            // Therefore, we just need to escape the " with a backslash.
            //
            if (argument.IndexOf(" ") != -1 ||
                argument.IndexOf("'") != -1 ||
                argument.IndexOf("\"") != -1)
            {
                return "\"" + argument.Replace("\"", "\\\"") + "\"";
            }
            return argument;
        }

        private static string QuoteAsNeeded(string[] argumentList, int startIndex)
        {
            string quotedArguments = "";
            if (argumentList.Length > startIndex)
            {
                quotedArguments = QuoteAsNeeded(argumentList[startIndex]);
                for (int i = startIndex + 1; i < argumentList.Length; i++)
                {
                    quotedArguments = quotedArguments + " " + QuoteAsNeeded(argumentList[i]);
                }
            }
            return quotedArguments;
        }

        public static int RunInConsole(string[] commandAndArgs)
        {
            string command = commandAndArgs[0];
            string arguments = QuoteAsNeeded(commandAndArgs, 1);

            Process p = new Process();
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardOutput = false;
            p.StartInfo.RedirectStandardError = false;
            p.StartInfo.FileName = command;
            p.StartInfo.Arguments = arguments;
            p.Start();
            p.WaitForExit();
            return p.ExitCode;
        }

        /// <summary>
        /// Run a command, optionally overriding the environment.
        /// </summary>
        /// <param name="command">Program to run.</param>
        /// <param name="arguments">Rest of command line arguments.</param>
        /// <param name="timeoutSeconds">How many seconds to wait before timing out.  Use 0 for infinite wait.</param>
        /// <param name="modifyEnvironment">This is an IDictionary because StringDictionary always goes lowercase.</param>
        /// <returns>Process information, including output.</returns>
        /// <remarks>We use IDictionary for the environment modification instead of StringDictionary so
        /// as to preserve case in the environment variable names.  Otherwise we have trouble on Unix.</remarks>
        private static ProcessInfo RunCommandInternal(string command, string arguments, int timeoutSeconds, IDictionary modifyEnvironment)
        {
            string fullCommand = command;
            if (arguments != null && arguments.Length > 1)
            {
                fullCommand = fullCommand + " " + arguments;
            }

            Process p = new Process();
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;
            p.StartInfo.FileName = command;
            p.StartInfo.Arguments = arguments;
            if (modifyEnvironment != null)
            {
                StringDictionary variables = p.StartInfo.EnvironmentVariables;
                foreach (DictionaryEntry entry in modifyEnvironment)
                {
                    variables.Remove(entry.Key as string);
                    if (entry.Value != null)
                    {
                        variables.Add(entry.Key as string, entry.Value as string);
                    }
                }
            }
            try
            {
                p.Start();
            }
            catch (Exception e)
            {
                throw new ExecException("unexpected error while trying to execute " + command, e);
            }
            string output = "unable to retrieve output";
            string error = "unable to retrieve error";
            Thread thread1 = null;
            Thread thread2 = null;
            try
            {
                ReadWholeStream outputReader = new ReadWholeStream(p.StandardOutput);
                ReadWholeStream errorReader = new ReadWholeStream(p.StandardError);
                thread1 = new Thread(new ThreadStart(outputReader.ReadToEnd));
                thread2 = new Thread(new ThreadStart(errorReader.ReadToEnd));
                thread1.Start();
                thread2.Start();

                int timeoutMilliseconds = timeoutSeconds * 1000;
                if (timeoutSeconds == 0)
                {
                    timeoutMilliseconds = Int32.MaxValue;
                }
                bool done = p.WaitForExit(timeoutMilliseconds);
                if (!done)
                {
                    throw new ExecException("command timeout after " + timeoutSeconds + " seconds: " + fullCommand);
                }

                if ( thread1.Join(2000) )
                {
                    output = outputReader.Result;
                }
                else
                {
                    output = "output thread timed out";
                }
                if ( thread2.Join(2000) )
                {
                    error = errorReader.Result;
                }
                else
                {
                    error = "error thread timed out";
                }
            }
            finally
            {
                if (thread1 != null && thread1.IsAlive)
                {
                    thread1.Abort();
                }
                if (thread2 != null && thread2.IsAlive)
                {
                    thread2.Abort();
                }
            }

            if (p.ExitCode != 0)
            {
                throw new ExecException(fullCommand, output, error, p.ExitCode);
            }

            return new ProcessInfo(output, null, p.ExitCode);
        }

        private static ProcessInfo RunCommandEx(string[] commandAndArgs, int timeoutSeconds, IDictionary modifyEnvironment)
        {
            string command = commandAndArgs[0];
            string arguments = QuoteAsNeeded(commandAndArgs, 1);
            return RunCommandInternal(command, arguments, timeoutSeconds, modifyEnvironment);
        }

        private static ProcessInfo RunCommandEx(string commandLine, int timeoutSeconds, IDictionary modifyEnvironment)
        {
            string command = commandLine.Split(new char[] { ' ' })[0].Trim();
            string arguments = commandLine.Substring(command.Length).Trim();
            return RunCommandInternal(command, arguments, timeoutSeconds, modifyEnvironment);
        }

        public static string RunCommandWithOutput(string command)
        {
            return RunCommandEx(command, 0, null).StdOut;
        }

        public static string RunCommandWithOutput(string[] commandAndArgs, int timeoutSeconds, IDictionary modifyEnvironment)
        {
            return RunCommandEx(commandAndArgs, timeoutSeconds, modifyEnvironment).StdOut;
        }

        public static string RunCommandWithOutput(string[] commandAndArgs)
        {
            return RunCommandWithOutput(commandAndArgs, 0, null);
        }

        public static void RunCommand(string command)
        {
            RunCommandEx(command, 0, null);
        }

        public static void RunCommand(string[] commandAndArgs)
        {
            RunCommandEx(commandAndArgs, 0, null);
        }

        public static bool RunCommandBoolean(string command, int falseExitCode)
        {
            //
            // returns true if ok (zero exit code), false if exit code matches, exception otherwise
            //

            try
            {
                RunCommand(command);
                return true;
            }
            catch (ExecException e)
            {
                if (falseExitCode == e.RetCode)
                {
                    return false;
                }
                throw e;
            }
        }
    }

#if NET_20
    static
#endif
    class logger
    {
        private static TextWriter _writer = null;
        private static Type _lock = typeof(logger);
		private static string _logFile = null;

        public static void SetLogFile(string logFile)
        {
            lock (_lock)
            {
                if (null != _writer)
                {
                    if (Console.Out != _writer)
                    {
                        _writer.Close();
                    }
                    _writer = null;
                }
                switch (logFile)
                {
                    case null:
                        break;
                    case ".":
                        _writer = Console.Out;
                        break;
                    default:
                        //
                        // Set AutoFlush to true (which can only be done via
                        // StreamWriter and not TextWriter) so that _writer
                        // behaves like Console.Out.  This ensures that
                        // we do not lose any logging if the process
                        // terminates abnormally.
                        //
                        _writer = new StreamWriter(logFile, false, Encoding.ASCII);
                        (_writer as StreamWriter).AutoFlush = true;
						_logFile = logFile;
                        break;
                }
            }
        }

        private static void WriteLog(TraceLevel level, string message)
        {
            lock (_lock)
            {
                if (null != _writer)
                {
                    _writer.WriteLine("{1}: [{0}] {2}", level, DateTime.Now, message);
                }
            }
        }

        public static void debug(string message)
        {
            WriteLog(TraceLevel.Verbose, message);
        }

        public static void error(string message)
        {
            WriteLog(TraceLevel.Error, message);
        }

		public static string GetLogFilePath()
		{
			return _logFile;
		}
    }
}
