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
using System.Collections;
using System.Data;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace Likewise.LMC.Utilities
{
    public class WindowsSession
    {
        private const string mprDllPath = "mpr.dll";

        [DllImport(mprDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int WNetAddConnection2(
            Session.NETRESOURCE netResource,
            [MarshalAs(UnmanagedType.LPWStr)] string sPassword,
            [MarshalAs(UnmanagedType.LPWStr)] string sUsername,
            uint dwFlags
			);

        [DllImport(mprDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int WNetCancelConnection2(
            [MarshalAs(UnmanagedType.LPWStr)] string sName,
            uint dwFlags,
            bool bForce
			);
    }

    public class Session
    {
        #region Constants
        private const string netAPIDllPath = "netapi32.dll";
        private const string mprDllPath = "mpr.dll";
        #endregion

        #region PInvoke Stuff

        private const int SESS_GUEST = 1;

        /*
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public struct NETRESOURCE
        {
            public int dwScope;
            public int dwType;
            public int dwDisplayType;
            public int dwUsage;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string LocalName;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string RemoteName;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Comment;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Provider;
        }
        */

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public class NETRESOURCE
        {
            public int dwScope;
            public int dwType;
            public int dwDisplayType;
            public int dwUsage;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string LocalName;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string RemoteName;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Comment;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Provider;
        }


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct SESSION_INFO_1
        {
            public string sesi1_cname;
            public string sesi1_username;
            public int sesi1_num_opens;
            public int sesi1_time;
            public int sesi1_idle_time;
            public int sesi1_user_flags;
        }

        [DllImport(mprDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int WNetAddConnection2(
            NETRESOURCE netResource,
            [MarshalAs(UnmanagedType.LPWStr)] string sPassword,
            [MarshalAs(UnmanagedType.LPWStr)] string sUsername,
            uint dwFlags
            );

        [DllImport(mprDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int WNetCancelConnection2(
            [MarshalAs(UnmanagedType.LPWStr)] string sName,
            uint dwFlags,
            bool bForce
            );

        [DllImport(netAPIDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int NetSessionDel(string sServer, string sUNCClient, string sUsername);

        [DllImport(netAPIDllPath, SetLastError = true, CharSet=CharSet.Unicode)]
        private static extern int NetSessionEnum(
            string ServerName,
            string UncClientName,
            string UserName,
            int Level,
            out IntPtr bufptr,
            int prefmaxlen,
            ref int entriesread,
            ref int totalentries,
            ref int hResumeHandle);

        [DllImport(netAPIDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int NetApiBufferFree(IntPtr Buffer);

        #endregion

        #region Private classes

        // private class to force a clean up call (C# needs static destructors!)
        private class CleanUp
        {
            ~CleanUp()
            {
                DeleteAllCreatedNullSessions();
            }
        }

        #endregion

        #region Class data

        // static data to keep track of global sessions
        private static ArrayList alSessions = new ArrayList();

#if __MonoCS__ // error CS0414: The private field `Likewise.Auth.Session.cleanup' is assigned but its value is never used
#pragma warning disable 414
#endif
        // force C# to call some static clean up code
        private static CleanUp cleanup = new CleanUp();
#if __MonoCS__
#pragma warning restore 414
#endif

        #endregion

        #region Static Interface
        public static bool CreateNullSession(string sServer, string sShortname, CredentialEntry ce)
        {
            Logger.Log(
                String.Format("Session.CreateNullSession({0}, {1}, {2}) called", sServer, sShortname, ce),
                Logger.netAPILogLevel);

            int nErr;

            // set up the user name and password; map "root" as needed
            string sUsername;
            string sPassword;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                if (ce != null && !ce.DefaultCreds && ce.Password != new String('*', 16))
                {
                    if (ce.UserName.IndexOf('\\') < 0 && !String.IsNullOrEmpty(ce.Domain))
                    {
                        sUsername = String.Format("{0}\\{1}", ce.Domain, ce.UserName);
                    }
                    else
                    {
                        sUsername = ce.UserName;
                    }
                    sPassword = ce.Password;
                }
                else
                {
                    // setup for default creds
                    sUsername = null;
                    sPassword = null;
                    Logger.Log("CreateNullSession(): Using default creds", Logger.netAPILogLevel);
                }
            }
            else
            {
                if (ce.UserName.IndexOf(@"\") < 0 && !String.IsNullOrEmpty(ce.Domain))
                {
                    sUsername = String.Format("{0}\\{1}", ce.Domain, ce.UserName);
                }
                else
                {
                    sUsername = ce.UserName;
                }
                sPassword = ce.Password;
            }

            // set up a NETRESOURCE structure
            NETRESOURCE nr = new NETRESOURCE();
            nr.dwScope       = 0;
            nr.dwType        = 0;
            nr.dwDisplayType = 0;
            nr.dwUsage       = 0;
            nr.LocalName     = null;
            nr.RemoteName    = @"\\" + sServer + @"\IPC$";
            nr.Comment       = null;
            nr.Provider      = null;

            // try the operation. Throws exception if it fails.
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                nErr = WindowsSession.WNetAddConnection2(nr, sPassword, sUsername, 0);
            }
            else
            {
                nErr = WNetAddConnection2(nr, sPassword, sUsername, 0);
            }

            //HACK: WNetAddConnection2 appears to return 'access denied' even when it has just granted access!
            //this is a workaround with the side-effect that the user will never be warned when their password
            //is invalid.
            if (nErr == (int) ErrorCodes.WIN32Enum.ERROR_ACCESS_DENIED)
            {
                Logger.Log(String.Format("CreateNullSession(): Ignoring error!  nErr={0}: {1}",
                    nErr, ErrorCodes.WIN32String(nErr)), Logger.LogLevel.Error);

                System.Windows.Forms.MessageBox.Show("WNetAddConnection2 success but ERROR_ACCESS_DENIED");

                return true;
            }
            else if (nErr != 0)
            {
                ce.Invalidate(sServer);

                Logger.Log(String.Format("CreateNullSession(): nErr={0}: {1}",
                    nErr, ErrorCodes.WIN32String(nErr)), Logger.LogLevel.Error);
                return false;
            }

            Logger.Log("CreateNullSession() successful", Logger.netAPILogLevel);

            return true;
        }

        public static bool EnsureNullSession(string sServer, CredentialEntry ce)
        {
            if (String.IsNullOrEmpty(sServer))
            {
                return false;
            }

            Logger.Log(String.Format("Session.EnsureNullSession({0}, {1}) called",
                sServer, ce),
                Logger.netAPILogLevel);

            if(ce == null)
            {
                Logger.Log("Session.EnsureNullSession() called with CredentialEntry ce=null, returning false",
                    Logger.authLogLevel);
                return false;
            }

            sServer = Canon(sServer);

            try
            {
                // if we already have a session, don't recreate it
                if (alSessions.Contains(sServer))
                {
                    return true;
                }
                else
                {
                    try
                    {
                        DeleteNullSession(sServer);
                    }
                    catch(AuthSessionException aex)
                    {
                        Logger.Log("EnsureNullSession: DeleteNullSession failed (as expected) beacuse " + aex.Message, Logger.netAPILogLevel);
                    }

                    try
                    {
                        if (CreateNullSession(sServer, ce.MachineName, ce))
                        {
                            alSessions.Add(sServer);
                            return true;
                        }
                    }
                    catch (AuthSessionException aex)
                    {
                        Logger.Log("EnsureNullSession: CreateNullSession failed unexpectedly. beacuse " + aex.Message, Logger.netAPILogLevel);
                    }
                }
                return false;
            }
            catch (DllNotFoundException ex)
            {
                Logger.ShowUserError(String.Format(
                    "DllNotFound Exception caught in SharesAPI.EnumShares: Could not load library: {0}",
                    ex.Message));
                ce.Invalidate(sServer);
                return false;
            }
            catch (Exception ex)
            {
                Logger.LogException("Session.EnsureNullSession", ex);
                ce.Invalidate(sServer);
                return false;
            }
        }


        public static bool DeleteNullSession(string sServer)
        {
            Logger.Log(String.Format("Session::DeleteNullSession({0}) called", sServer), Logger.netAPILogLevel);

            bool result = false;
            int nRet = 0;

            sServer = Canon(sServer);
            try
            {

                // if there is no session, don't try to delete
                //HACK: this is removed due to issue 5006: bad creds can persist in the muxer after Quartz exist
                //and therefore may be present in a new instance of Quartz before creds are entered.
                //if (!alSessions.Contains(sServer))
                 //   return;
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    nRet = WindowsSession.WNetCancelConnection2(@"\\" + sServer + @"\IPC$", 1, true);
                else
                    nRet = WNetCancelConnection2(@"\\" + sServer + @"\IPC$", 1, true);

                Logger.Log(String.Format(
                    "{0}(sServer={1}): nRet={2}",
                    "Session.DeleteNullSession(): WNetCancelConnection2",
                    sServer, nRet), Logger.netAPILogLevel);

                if (nRet == 0)
                {
                    result = true;
                }


                else
                {
                    result = true;
                }

                if (alSessions.Contains(sServer))
                {
                    alSessions.Remove(sServer);
                }

            }
            catch (Exception ex)
            {
                Logger.LogException("Session.DeleteNullSession", ex);
            }

            if (nRet != 0)
            {
                throw new AuthSessionException(ErrorCodes.WIN32String(nRet), null);
            }

            return result;
        }

        public static void DeleteAllCreatedNullSessions()
        {
            Logger.Log("Session::DeleteAllCreatedNullSessions() called", Logger.netAPILogLevel);


            foreach (string s in alSessions)
            {
                string sServer = Canon(s);
                try
                {
                    int nRet;
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        nRet = WindowsSession.WNetCancelConnection2(@"\\" + sServer + @"\IPC$", 1, true);
                    else
                        nRet = WNetCancelConnection2(@"\\" + sServer + @"\IPC$", 1, true);

                    Logger.Log(String.Format(
                        "{0}(sServer={1}): nRet={2}",
                        "Session.DeleteAllCreatedNullSessions(): WNetCancelConnection2",
                        sServer, nRet), Logger.netAPILogLevel);

                    if (nRet != 0)
                    {
                        throw new AuthSessionException(ErrorCodes.WIN32String(nRet), null);
                    }
                }
                catch (Exception ex)
                {
                    Logger.LogException("Session.DeleteAllCreatedNullSessions", ex);
                }
            }
            alSessions.Clear();

        }

        public static void DeleteSession(CredentialEntry ce, string sServer, string sSessionMachine, string sSessionUser)
        {

            sServer = Canon(sServer);

            try
            {
                int nRet = NetSessionDel(sServer, sSessionMachine, sSessionUser);

                Logger.Log(String.Format(
                    "{0}(sServer={1}, sSessionMachine={2}, sSessionUser={3}): nRet={4}",
                    "Session.DeleteSession(): NetSessionDel",
                    sServer, sSessionMachine, sSessionUser, nRet), Logger.netAPILogLevel);

                if (nRet != 0)
                {
                    throw new AuthSessionException(ErrorCodes.WIN32String(nRet), null);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Session.DeleteSession", ex);
            }

        }

        public static Dictionary<int, string[]> EnumSessions(CredentialEntry ce, string sServer)
        {
            int entriesread = 0;
            int totalentries = 0;
            int hResumeHandle = 0;
            IntPtr lpBuf = IntPtr.Zero;

            Dictionary<int, string[]> SessionList = new Dictionary<int, string[]>();

            if (!EnsureNullSession(sServer, ce))
            {
                return null;
            }

            try
            {
                int nRet = NetSessionEnum(sServer,
                                null,
                                null,
                                1,
                                out lpBuf,
                                -1,
                                ref entriesread,
                                ref totalentries,
                                ref hResumeHandle);

                Logger.Log(String.Format("{0}: nRet={1}, entriesread={2}, totalentries={3}",
                    "Session.EnumSessions(): NetSessionEnum()",
                    nRet, entriesread, totalentries), Logger.netAPILogLevel);

                if (nRet != 0)
                {
                    throw new AuthSessionException(ErrorCodes.WIN32String(nRet), null);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Session.EnumSessions", ex);
                return null;
            }

            // iterate through the entries
            IntPtr lpTemp = lpBuf;
            for (int i = 0; i < entriesread; i++)
            {
                SESSION_INFO_1 s1 = (SESSION_INFO_1)Marshal.PtrToStructure(lpTemp, typeof(SESSION_INFO_1));

                bool bIsGuest = (s1.sesi1_user_flags & SESS_GUEST) == SESS_GUEST;
                string IsGuest = "";
                if (bIsGuest)
                    IsGuest = "Yes";
                else
                    IsGuest = "No";

                string[] sSessionInfo = { s1.sesi1_username, s1.sesi1_cname, s1.sesi1_num_opens.ToString(), TimeSpan.FromSeconds((double)s1.sesi1_time).ToString(), TimeSpan.FromSeconds((double)s1.sesi1_idle_time).ToString(), IsGuest };

                SessionList.Add(i, sSessionInfo);

                lpTemp = (IntPtr)((int)lpTemp + Marshal.SizeOf(s1));
            }
            NetApiBufferFree(lpBuf);

            return SessionList;
        }

        private static string Canon(string sServer)
        {
            return sServer.Trim().ToLower();
        }
        #endregion
    }
}
