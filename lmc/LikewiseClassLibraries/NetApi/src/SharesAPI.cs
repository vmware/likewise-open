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
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;
using System.Collections.Generic;

//Use the DebugMarshal for Debug builds, and the standard Marshal in release builds
#if DEBUG
using Marshal = Likewise.LMC.Utilities.DebugMarshal;
#endif

namespace Likewise.LMC.NETAPI
{
    public class SharesAPI
    {

        private const string netAPIDllPath = "netapi32.dll";
        private const string libsrvsvcDllPath = "libsrvsvc.dll";

        private const uint STYPE_DISKTREE = 0;
        private const uint STYPE_PRINTQ = 1;
        private const uint STYPE_DEVICE = 2;
        private const uint STYPE_IPC = 3;
        private const uint STYPE_TEMPORARY = 0x40000000;
        private const uint STYPE_SPECIAL = 0x80000000;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        struct SHARE_INFO_2
        {
            public string shi2_netname;
            [MarshalAs(UnmanagedType.U4)]
            public uint shi2_type;
            public string shi2_remark;
            [MarshalAs(UnmanagedType.U4)]
            public int shi2_permissions;
            [MarshalAs(UnmanagedType.U4)]
            public int shi2_max_uses;
            [MarshalAs(UnmanagedType.U4)]
            public int shi2_current_uses;
            public string shi2_path;
            public string shi2_passwd;
        };

        [DllImport(libsrvsvcDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        private extern static int NetShareEnum(
             IntPtr handle_b,
             [MarshalAs(UnmanagedType.LPStr)] string ServerName,
             int level,
             ref IntPtr bufPtr,
             int prefmaxlen,
             ref int entriesread,
             ref int totalentries,
             ref int resume_handle
             );

        [DllImport(netAPIDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        private extern static int NetShareEnum(
             string ServerName,
             int level,
             ref IntPtr bufPtr,
             int prefmaxlen,
             ref int entriesread,
             ref int totalentries,
             ref int resume_handle
             );

        // public constants (to interpret the Permissions prop)
        private const int PERM_FILE_READ = 1;
        private const int PERM_FILE_WRITE = 2;
        private const int PERM_FILE_CREATE = 4;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        struct FILE_INFO_3
        {
            public int fi3_id;
            public int fi3_permission;
            public int fi3_num_locks;
            public string fi3_pathname;
            public string fi3_username;
        }

        [DllImport(netAPIDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        static extern int NetFileEnum(
             string ServerName,
             string basepath,
             string username,
             int level,
             ref IntPtr bufPtr,
             int prefmaxlen,
             ref int entriesread,
             ref int totalentries,
             ref int resume_handle
        );

        [DllImport(libsrvsvcDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        static extern int NetFileEnum(
             IntPtr handle_b,
             [MarshalAs(UnmanagedType.LPStr)] string ServerName,
             [MarshalAs(UnmanagedType.LPStr)] string basepath,
             [MarshalAs(UnmanagedType.LPStr)] string username,
             int level,
             ref IntPtr bufPtr,
             int prefmaxlen,
             ref int entriesread,
             ref int totalentries,
             ref int resume_handle
        );

        [DllImport(libsrvsvcDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        static extern int NetFileClose(
                                       IntPtr handle_b, 
                                       string strServer,
                                       int nFileId
                                      );

        [DllImport(netAPIDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        static extern int NetFileClose(string strServer, int nFileId);

        [DllImport(netAPIDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int NetApiBufferFree(IntPtr Buffer);

        [DllImport(libsrvsvcDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int SrvSvcFreeMemory(IntPtr Buffer);

        [DllImport(libsrvsvcDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        static extern int NetShareDel(
                                       IntPtr handle_b,
                                       string strServer,
                                       string strNetName,
                                       int reserved
                                     );

        [DllImport(netAPIDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int NetShareDel(string strServer,
                                        string strNetName,
                                        int reserved);

        [DllImport(libsrvsvcDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int NetShareAdd(
                                       IntPtr handle_b,
                                       string strServer,
                                       int level,
                                       ref IntPtr bufPtr,
                                       ref int param_err
                                      );

        [DllImport(netAPIDllPath, SetLastError = true)]
        public static extern int NetShareGetInfo(
            [MarshalAs(UnmanagedType.LPWStr)] string serverName,
            [MarshalAs(UnmanagedType.LPWStr)] string netName,
            Int32 level,
            out IntPtr bufPtr);

        [DllImport(libsrvsvcDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int NetShareGetInfo(
                                       IntPtr handle_b,
                                       [MarshalAs(UnmanagedType.LPStr)] string servername,
                                       [MarshalAs(UnmanagedType.LPStr)] string netname,
                                       int level,
                                       ref IntPtr bufPtr,
                                       ref int param_err
                                      );

        [DllImport(libsrvsvcDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        static extern int InitSrvSvcBindingDefault(
                                                    out IntPtr handle_b,
                                                    [MarshalAs(UnmanagedType.LPStr)] string Servername
                                                   );

        #region Static Interface
        public static Dictionary<int, string[]> EnumShares(IntPtr handle_b, CredentialEntry ce, string sHostname)
        {
            //// establish null session
            //if(!Session.EnsureNullSession(sHostname, ce))
            //{
            //    return null;
            //}

            try
            {
                Dictionary<int, string[]> ShareList = new Dictionary<int, string[]>();

                IntPtr pBuf = IntPtr.Zero;                
                int cRead = 0;
                int cTotal = 0;
                int hResume = 0;
                int maxlen = -1;
                int nret = -1;

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    Logger.Log(String.Format("NetShareEnum(sHostname={0}) called",
                    sHostname), Logger.FileShareManagerLogLevel);

                    nret = NetShareEnum(sHostname,
                                                2,
                                                ref pBuf,
                                                maxlen,
                                                ref cRead,
                                                ref cTotal,
                                                ref hResume);
                }
                else
                {
                    Logger.Log(String.Format("NetShareEnum(handle_b={0:x},sHostname={1}) called",
                    handle_b.ToInt32(), sHostname), Logger.FileShareManagerLogLevel);

                    maxlen = 20;

                    nret = NetShareEnum(handle_b,
                                            sHostname,
                                            2,
                                            ref pBuf,
                                            maxlen,
                                            ref cRead,
                                            ref cTotal,
                                            ref hResume);
                }
                
                Logger.Log(String.Format(
                        "NetShareEnum(); result={0}, pBuf={1}, cRead={2}, cTotal={3}, hResume={4}",
                    nret, pBuf, cRead, cTotal, hResume));

                // now, iterate through the data in pBuf
                IntPtr pCur = pBuf;
                for (int i = 0; i < cRead; i++)
                {
                    // marshal the entry into 
                    SHARE_INFO_2 si2 = (SHARE_INFO_2)Marshal.PtrToStructure(pCur, typeof(SHARE_INFO_2));

                    // only allow regular diskshares
                    // TODO: Review this. Should we not display admin shares?
                    if (si2.shi2_type == STYPE_DISKTREE)
                    {
                        string[] sShareInfo = { si2.shi2_netname, si2.shi2_path, si2.shi2_remark, si2.shi2_current_uses.ToString() };
                        ShareList.Add(i, sShareInfo);
                    }

                    // advance to the next entry
                    pCur = (IntPtr)((int)pCur + Marshal.SizeOf(si2));
                }
                if (pBuf != IntPtr.Zero)
                {
                    // free the data
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    {
                        NetApiBufferFree(pBuf);
                    }
                    else
                    {
                        SrvSvcFreeMemory(pBuf);
                    }
                }
                // all done
                return ShareList;
            }
            catch (Win32Exception we)
            {
                throw new LikewiseAPIException(we);
            }
        }

        public static Dictionary<int, string[]> EnumFiles(IntPtr pHandle, CredentialEntry ce, string sHostname)
        {
            // establish null session
            //if (!Session.EnsureNullSession(sHostname, ce))
            //{
            //    return null;
            //}

            try
            {
                Dictionary<int, string[]> UserList = new Dictionary<int, string[]>();

                IntPtr pBuf = IntPtr.Zero;                                
                int cRead = 0;
                int cTotal = 0;
                int hResume = 0;
                int maxlen = -1;
                int nret = -1;

                Logger.Log(String.Format("NetFileEnum(handle_b={0:x}, sHostname={1}) called",
                   pHandle.ToInt32(), sHostname), Logger.FileShareManagerLogLevel);

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    nret = NetFileEnum(sHostname,
                                       null,
                                       null,
                                       3,
                                       ref pBuf,
                                       maxlen,
                                       ref cRead,
                                       ref cTotal,
                                       ref hResume);
                }
                else
                {
                    nret = NetFileEnum(pHandle,
                                           sHostname,
                                           null,
                                           null,
                                           3,
                                           ref pBuf,
                                           maxlen,
                                           ref cRead,
                                           ref cTotal,
                                           ref hResume);
                }

                Logger.Log(String.Format(
                       "NetFileEnum(); result={0}, pBuf={1}, cRead={2}, cTotal={3}, hResume={4}",
                   nret, pBuf, cRead, cTotal, hResume));

                // now, iterate through the data in pBuf
                IntPtr pCur = pBuf;
                for (int i = 0; i < cRead; i++)
                {
                    // marshal the entry into 
                    FILE_INFO_3 fi3 = (FILE_INFO_3)Marshal.PtrToStructure(pCur, typeof(FILE_INFO_3));

                    // create a row
                    // set the mode
                    int iMode = fi3.fi3_permission;
                    string sMode = "";
                    if ((iMode & PERM_FILE_READ) == PERM_FILE_READ)
                        sMode = "Read";
                    if ((iMode & PERM_FILE_WRITE) == PERM_FILE_WRITE)
                    {
                        if (sMode.Length > 0)
                            sMode += "+Write";
                        else
                            sMode = "Write";
                    }
                    if ((iMode & PERM_FILE_CREATE) == PERM_FILE_CREATE)
                    {
                        if (sMode.Length > 0)
                            sMode += "+Create";
                        else
                            sMode = "Create";
                    }
                    string[] sFileInfo = { fi3.fi3_pathname, fi3.fi3_username, fi3.fi3_num_locks.ToString(), sMode, fi3.fi3_id.ToString() };

                    UserList.Add(i, sFileInfo);


                    // advance to the next entry
                    pCur = (IntPtr)((int)pCur + Marshal.SizeOf(fi3));
                }
                if (pBuf != IntPtr.Zero)
                {
                    // free the data
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    {
                        NetApiBufferFree(pBuf);
                    }
                    else
                    {
                        SrvSvcFreeMemory(pBuf);
                    }
                }

                // all done
                return UserList;

            }
            catch (Win32Exception we)
            {
                throw new LikewiseAPIException(we);
            }
        }

        public static void CloseFile(IntPtr handle_b, CredentialEntry ce, string sHostname, int nFileId)
        {
            // establish null session
            Session.EnsureNullSession(sHostname, ce);

            try
            {
                int nret = -1;

                Logger.Log(String.Format("NetFileClose(sHostname={1}) called",
                   sHostname), Logger.FileShareManagerLogLevel);

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    nret = NetFileClose(sHostname, nFileId);                    
                }
                else
                {                    
                    nret = NetFileClose(handle_b, sHostname, nFileId);
                }

                Logger.Log(String.Format(
                      "NetFileClose(); result={0}",
                      nret));
            }
            catch (Win32Exception we)
            {
                throw new LikewiseAPIException(we);
            }
        }

        public static void DeleteShare(IntPtr handle_b, CredentialEntry ce, string sHostname, string sShareName)
        {
            // establish null session
            Session.EnsureNullSession(sHostname, ce);

            try
            {
                int nret = -1;

                Logger.Log(String.Format("NetShareDel(sHostname={0} ,sShareName={1}) called",
                   sHostname, sShareName), Logger.FileShareManagerLogLevel);

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    nret = NetShareDel(sHostname, sShareName, 0);
                }
                else
                {                    
                    nret = NetShareDel(handle_b, sHostname, sShareName, 0);
                }

                Logger.Log(String.Format(
                      "NetShareDel(); result={0}",
                      nret));
            }
            catch (Win32Exception we)
            {
                throw new LikewiseAPIException(we);
            }
        }

        public static void AddShare(IntPtr handle_b, CredentialEntry ce, string sHostname, string sShareName)
        {
            // establish null session
            //Session.EnsureNullSession(sHostname, ce);

            try
            {
                int nret = -1;
                IntPtr pBuf = IntPtr.Zero;
                int param_err = 0;

                Logger.Log(String.Format("NetShareAdd(sHostname={0} ,sShareName={1}) called",
                   sHostname, sShareName), Logger.FileShareManagerLogLevel);

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    nret = NetShareAdd(
                                        handle_b,
                                        sHostname,
                                        0,
                                        ref pBuf,
                                        ref param_err
                                      );
                }
                else
                {
                    nret = NetShareDel(handle_b, sHostname, sShareName, 0);
                }

                Logger.Log(String.Format(
                      "NetShareDel(); result={0}",
                      nret));
            }
            catch (Win32Exception we)
            {
                throw new LikewiseAPIException(we);
            }
        }

        public static int OpenHandle(string sHostname, out IntPtr handle_b)
        {
            // establish null session
            //Session.EnsureNullSession(sHostname, ce);
            int nret = -1;

            try
            {                
                handle_b = IntPtr.Zero;

                Logger.Log(String.Format("OpenHandle(sHostname={0} called",
                   sHostname), Logger.FileShareManagerLogLevel);

                if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
                {
                    nret = InitSrvSvcBindingDefault(out handle_b, sHostname);
                }                     

                Logger.Log(String.Format(
                      "OpenHandle(); result={0}, handle={1:x}",
                      nret, handle_b.ToInt32()));
            }
            catch (Win32Exception we)
            {
                throw new LikewiseAPIException(we);
            }

            return nret;
        }

        public static string[] GetShareInfo(IntPtr handle_b, string sharename, string sHostname)
        {
            //// establish null session
            //if(!Session.EnsureNullSession(sHostname, ce))
            //{
            //    return null;
            //}

            try
            {
                string[] sShareInfo = null;

                IntPtr pBuf = IntPtr.Zero;
                int nret = -1;
                int param_err = 0;

                Logger.Log(String.Format("NetShareGetInfo(handle_b={0:x},sHostname={1}, sharename={2}) called",
                    handle_b.ToInt32(), sHostname, sharename), Logger.FileShareManagerLogLevel);

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    nret = NetShareGetInfo(                                           
                                           sHostname,
                                           sharename,
                                           2,
                                           out pBuf                                           
                                          );
                }
                else
                {
                    nret = NetShareGetInfo(
                                            handle_b,
                                            sHostname,
                                            sharename,
                                            2,
                                            ref pBuf,
                                            ref param_err
                                           );
                }

                Logger.Log(String.Format(
                        "NetShareGetInfo(); result={0}, pBuf={1}, param_err={2}",
                    nret, pBuf, param_err));

                // now, iterate through the data in pBuf
                IntPtr pCur = pBuf;

                // marshal the entry into 
                SHARE_INFO_2 si2 = (SHARE_INFO_2)Marshal.PtrToStructure(pCur, typeof(SHARE_INFO_2));

                // only allow regular diskshares
                // TODO: Review this. Should we not display admin shares?
                if (si2.shi2_type == STYPE_DISKTREE)
                {
                    sShareInfo = new string[] { "share", si2.shi2_netname, si2.shi2_path, si2.shi2_remark, si2.shi2_current_uses.ToString(), si2.shi2_max_uses.ToString() };
                }
                else
                {
                    sShareInfo = new string[] { "adminshare", si2.shi2_netname, si2.shi2_path, si2.shi2_remark, si2.shi2_current_uses.ToString(), si2.shi2_max_uses.ToString() };
                }

                if (pBuf != IntPtr.Zero)
                {
                    // free the data
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows) {
                        NetApiBufferFree(pBuf);
                    }
                    else {
                        SrvSvcFreeMemory(pBuf);
                    }
                }
                // all done
                return sShareInfo;
            }
            catch (Win32Exception we)
            {
                throw new LikewiseAPIException(we);
            }
        }

        public static Process RunAddAShareWizard(CredentialEntry ce, string sHostname)
        {
            Process proc;
            try
            {
                Session.EnsureNullSession(sHostname, ce);
                proc = ProcessUtil.Exec(Environment.SystemDirectory, "shrpubw.exe", "/s " + sHostname);
            }
            catch (Exception ex)
            {
                Logger.LogException("SharesAPI.RunAddAShareWizard()", ex);
                proc = null;
            }

            return proc;
        }

        public static Process RunMMC(CredentialEntry ce, string sHostname)
        {
            Session.EnsureNullSession(sHostname, ce);
            return ProcessUtil.ShellExec(Environment.SystemDirectory, "fsmgmt.msc", "/s /computer=" + sHostname, "open");
        }

        public static Process ViewShare(CredentialEntry ce, string sHostname, string sShare)
        {
            Session.EnsureNullSession(sHostname, ce);
            return ProcessUtil.Exec(Environment.SystemDirectory, "explorer.exe", @"\\" + sHostname + @"\" + sShare);
        }
        #endregion
    }
}
