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
using System.Runtime.InteropServices;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC.NETAPI.APIMarshal
{
    class NETAPIMarshal
    {

        public class NETAPIException : Exception
        {
            public int errorCode = 0;

            public NETAPIException(string message, int error)
                :
            base(message)
            {
                errorCode = error;
            }
        }

        #region PInvoke Stuff

        #region Definitions

        private const string netAPIDllPath = "netapi32.dll";

        #endregion

        #region Methods
        [DllImport(netAPIDllPath)]
        private extern static int NetUserDel([MarshalAs(UnmanagedType.LPWStr)]
                                             string serverName,
                                             [MarshalAs(UnmanagedType.LPWStr)]
                                             string userName);
        public static int apiNetUserDel(string serverName, string userName)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetUserDel(serverName={0}, userName={1}) called",
              serverName, userName), Logger.netAPILogLevel);

            int ret = NetUserDel(serverName, userName);
            ThrowExceptionIfError(ret, serverName);
            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetUserGetLocalGroups([MarshalAs(UnmanagedType.LPWStr)]
                                                        string serverName,
                                                        [MarshalAs(UnmanagedType.LPWStr)]
                                                        string userName,
                                                        int level,
                                                        uint flags,
                                                        out IntPtr bufPtr,
                                                        int prefMaxLen,
                                                        out int entriesRead,
                                                        out int totalEntries);
        public static int apiNetUserGetLocalGroups(string serverName, string userName, int level, uint flags, out IntPtr bufPtr,
            int prefMaxLen, out int entriesRead, out int totalEntries)
        {

            Logger.Log(String.Format("NETAPIMarshal.apiNetUserGetLocalGroups(serverName={0}, userName={1}, level={2}, flags={3}, prefMaxLen={4}) called",
                serverName, userName, level, flags, prefMaxLen), Logger.netAPILogLevel);

            int ret = NetUserGetLocalGroups(serverName, userName, level, flags, out bufPtr, 
                prefMaxLen, out entriesRead, out totalEntries);

            Logger.Log(String.Format("NETAPIMarshal.NetUserGetLocalGroups(), ret={0}, bufPtr{1}, entriesRead={2}, totalEntries={3}",
                ret, ptrStr(bufPtr), entriesRead, totalEntries), Logger.netAPILogLevel);

            ThrowExceptionIfError(ret, serverName);

            validateEntriesRead("apiNetUserGetLocalGroups", bufPtr, ref entriesRead, ref totalEntries);

            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetUserSetInfo([MarshalAs(UnmanagedType.LPWStr)]
                                                 string serverName,
                                                 [MarshalAs(UnmanagedType.LPWStr)]
                                                 string userName,
                                                 int level,
                                                 IntPtr buf,
                                                 IntPtr parmErr);
        public static int apiNetUserSetInfo(string serverName, string userName, int level, IntPtr buf, IntPtr parmErr)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetUserSetInfo(serverName={0}, userName={1}, level={2}, buf{3}, parmerr{4}) called",
              serverName, userName, level, ptrStr(buf), ptrStr(parmErr)), Logger.netAPILogLevel);


            int ret = NetUserSetInfo(serverName, userName, level, buf, parmErr);

            Logger.Log(String.Format("NETAPIMarshal.NetUserSetInfo(), ret={0}",
                ret), Logger.netAPILogLevel);           

            ThrowExceptionIfError(ret, serverName);
            return ret;
        }


        [DllImport(netAPIDllPath)]
        private extern static int NetUserGetInfo([MarshalAs(UnmanagedType.LPWStr)]
                                                 string serverName,
                                                 [MarshalAs(UnmanagedType.LPWStr)]
                                                 string userName,
                                                 int level,
                                                 out IntPtr buf);
        public static int apiNetUserGetInfo(string serverName, string userName, int level, out IntPtr buf)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetUserGetInfo(serverName={0}, userName={1}, level={2}) called",
                serverName, userName, level), Logger.netAPILogLevel);

            int ret = NetUserGetInfo(serverName, userName, level, out buf);

            Logger.Log(String.Format("NETAPIMarshal.NetUserGetInfo(), ret={0}, buf{1}",
                ret, ptrStr(buf)), Logger.netAPILogLevel);

            ThrowExceptionIfError(ret, serverName);
            return ret;
        } 


        [DllImport(netAPIDllPath)]
        private extern static int NetApiBufferFree(IntPtr buffer);
        public static int apiNetApiBufferFree(IntPtr buffer)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetApiBufferFree(buffer{0}) called",
                                     ptrStr(buffer)), Logger.netAPILogLevel);

            int ret = NetApiBufferFree(buffer);

            Logger.Log(String.Format("NETAPIMarshal.NetApiBufferFree(), ret={0}",
                ret), Logger.netAPILogLevel);


            //HACK: ignoring return codes, as NetApiBufferFree doesn't actually free any memory yet.
            if(ret != 0)
            {
                Logger.Log(String.Format(
                    "WARNING: Ignoring non-zero return code ({0}) from NETAPIMarshal.NetApiBufferFree()", 
                    ret), Logger.netAPILogLevel);
            }
            //TODO: uncomment this back.
            //ThrowExceptionIfError(ret, serverName);
           

            return ret;
        }


        [DllImport(netAPIDllPath)]
        private extern static int NetUserAdd([MarshalAs(UnmanagedType.LPWStr)]
                                             string serverName,
                                             UInt32 level,
                                             IntPtr userInfo,
                                             IntPtr parmErr);
        public static int apiNetUserAdd(string serverName, UInt32 level, IntPtr userInfo, IntPtr parmErr)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetUserAdd(serverName={0}, level={1}, userInfo{2}, parmErr{3}) called",
                    serverName, level, ptrStr(userInfo), ptrStr(parmErr)), Logger.netAPILogLevel);

            int ret = NetUserAdd(serverName, level, userInfo, parmErr);

            Logger.Log(String.Format("NETAPIMarshal.NetUserAdd(), ret={0}",
                ret), Logger.netAPILogLevel);

            ThrowExceptionIfError(ret, serverName);
            return ret;
        }

        [DllImport(netAPIDllPath, CharSet = CharSet.Unicode)]
        private extern static int NetUserEnum([MarshalAs(UnmanagedType.LPWStr)] 
                                              string serverName,
                                              int level,
                                              int filter,
                                              out IntPtr bufPtr,
                                              int prefMaxLen,
                                              out int entriesRead,
                                              out int totalEntries,
                                              out int resumeHandle);
        public static int apiNetUserEnum(string serverName, int level, int filter, out IntPtr bufPtr, int prefMaxLen,
            ref int entriesRead, ref int totalEntries, ref int resumeHandle)
        {
            int ret;
            bufPtr = IntPtr.Zero;

            Logger.Log(String.Format("NETAPIMarshal.apiNetUserEnum(serverName={0}, level={1}, filter={2}, prefMaxLen={3}, resumeHandle={4}) called",
                serverName, level, filter, prefMaxLen, resumeHandle), Logger.netAPILogLevel);

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                ret = WindowsSession.NetUserEnum(serverName, level, filter, out bufPtr, prefMaxLen,
                    out entriesRead, out totalEntries, ref resumeHandle);
            }
            else
            {
                filter = 1;
                ret = NetUserEnum(serverName, level, filter, out bufPtr, prefMaxLen,
                    out entriesRead, out totalEntries, out resumeHandle);
            }

            Logger.Log(String.Format("NETAPIMarshal.NetUserEnum(), ret={0}, bufPtr{1}, entriesRead={2}, totalEntries={3}, resumeHandle={4}",
                ret, ptrStr(bufPtr), entriesRead, totalEntries, resumeHandle), Logger.netAPILogLevel);

            //allow non-zero error code if indicates more data must be read.
            if (ret != (int)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
            {
                ThrowExceptionIfError(ret, serverName);
            }        

            //HACK: the following code should NOT be necessary.  This is a likely bug in Plumb
            if (totalEntries < entriesRead)
            {
                Logger.Log(String.Format(
                    "NETAPIMarshal.apiNetUserEnum(): HACK: assigned totalEntries({0}) = entriesRead({1})",
                    totalEntries, entriesRead), Logger.netAPILogLevel);

                totalEntries = entriesRead;
            }
            //END HACK

            validateEntriesRead("apiNetUserEnum", bufPtr, ref entriesRead, ref totalEntries);
            
            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupAdd([MarshalAs(UnmanagedType.LPWStr)]
                                                   string serverName,
                                                   UInt32 level,
                                                   IntPtr groupInfo,
                                                   IntPtr parmErr);
        public static int apiNetLocalGroupAdd(string serverName, UInt32 level, IntPtr groupInfo, IntPtr parmErr)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupAdd(serverName={0}, level={1}, groupInfo{2}, parmErr{3}) called",
                serverName, level, ptrStr(groupInfo), ptrStr(parmErr)), Logger.netAPILogLevel);

            int ret = NetLocalGroupAdd(serverName, level, groupInfo, parmErr);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupAdd(), ret={0}",
                ret), Logger.netAPILogLevel);


            ThrowExceptionIfError(ret, serverName);
            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupDel([MarshalAs(UnmanagedType.LPWStr)]
                                                    string serverName,
                                                    [MarshalAs(UnmanagedType.LPWStr)]
                                                    string groupName);
        public static int apiNetLocalGroupDel(string serverName, string groupName)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupDel(serverName={0}, groupName={1}) called",
                serverName, groupName), Logger.netAPILogLevel);

            int ret = NetLocalGroupDel(serverName, groupName);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupDel(), ret={0}",
                ret), Logger.netAPILogLevel);

            ThrowExceptionIfError(ret, serverName);
            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupEnum([MarshalAs(UnmanagedType.LPWStr)] 
                                                    string serverName,
                                                    int level,
                                                    out IntPtr bufPtr,
                                                    int prefMaxLen,
                                                    out int entriesRead,
                                                    out int totalEntries,
                                                    ref int resumeHandle);
        public static int apiNetLocalGroupEnum(string serverName, int level, out IntPtr bufPtr, int prefMaxLen,
            out int entriesRead, out int totalEntries, ref int resumeHandle)
        {

            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupEnum(serverName={0}, level={1}, prefMaxLen={2}, resumeHandle={3}) called",
                serverName, level, prefMaxLen, resumeHandle), Logger.netAPILogLevel);

            int ret = NetLocalGroupEnum(serverName, level, out bufPtr, prefMaxLen, 
                out entriesRead, out totalEntries, ref resumeHandle);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupEnum(), ret={0}, bufPtr{1}, entriesRead={2}, totalEntries={3}, resumeHandle={4}",
                ret, ptrStr(bufPtr), entriesRead, totalEntries, resumeHandle), Logger.netAPILogLevel);

            ThrowExceptionIfError(ret, serverName);

            validateEntriesRead("apiNetLocalGroupEnum", bufPtr, ref entriesRead, ref totalEntries);

            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupAddMembers([MarshalAs(UnmanagedType.LPWStr)]
                                                          string serverName,
                                                          [MarshalAs(UnmanagedType.LPWStr)]
                                                          string groupName,
                                                          int level,
                                                          IntPtr buf,
                                                          int totalEntries);
        public static int apiNetLocalGroupAddMembers(string serverName, string groupName, 
            int level, IntPtr buf, int totalEntries)
        {

            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupAddMembers(serverName={0}, groupName={1}, level={2}, buf{3}, totalEntries={4}) called",
                serverName, groupName, level, ptrStr(buf), totalEntries), Logger.netAPILogLevel);

            int ret = NetLocalGroupAddMembers(serverName, groupName, level, buf, totalEntries);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupAddMembers(), ret={0}",
                 ret), Logger.netAPILogLevel);


            ThrowExceptionIfError(ret, serverName);
            return ret;
        }


        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupDelMembers([MarshalAs(UnmanagedType.LPWStr)]
                                                          string serverName,
                                                          [MarshalAs(UnmanagedType.LPWStr)]
                                                          string groupName,
                                                          int level,
                                                          IntPtr buf,
                                                          int totalEntries);
        public static int apiNetLocalGroupDelMembers(string serverName, string groupName, 
            int level, IntPtr buf, int totalEntries)
        {

            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupDelMembers(serverName={0}, groupName={1}, level={2}, buf{3}, totalEntries={4}) called",
                serverName, groupName, level, ptrStr(buf), totalEntries), Logger.netAPILogLevel);

            int ret = NetLocalGroupDelMembers(serverName, groupName, level, buf, totalEntries);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupDelMembers(), ret={0}",
                ret), Logger.netAPILogLevel);


            ThrowExceptionIfError(ret, serverName);
            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupSetInfo([MarshalAs(UnmanagedType.LPWStr)]
                                                       string serverName,
                                                       [MarshalAs(UnmanagedType.LPWStr)]
                                                       string groupName,
                                                       int level,
                                                       IntPtr buf,
                                                       IntPtr parmErr);
        public static int apiNetLocalGroupSetInfo(string serverName, string groupName, int level, IntPtr buf, IntPtr parmErr)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupSetInfo(serverName={0}, groupName={1}, level={2}, buf{3}, parmErr{4}) called",
                serverName, groupName, level, ptrStr(buf), ptrStr(parmErr)), Logger.netAPILogLevel);

            int ret = NetLocalGroupSetInfo(serverName, groupName, level, buf, parmErr);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupSetInfo(), ret={0}",
                    ret), Logger.netAPILogLevel);


            ThrowExceptionIfError(ret, serverName);
            return ret;
        }


        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupGetInfo([MarshalAs(UnmanagedType.LPWStr)]
                                                       string serverName,
                                                       [MarshalAs(UnmanagedType.LPWStr)]
                                                       string groupName,
                                                       int level,
                                                       out IntPtr buf);
        public static int apiNetLocalGroupGetInfo(string serverName, string groupName, int level, out IntPtr buf)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupGetInfo(serverName={0}, groupName={1}, level={2}) called",
                serverName, groupName, level), Logger.netAPILogLevel);


            int ret = NetLocalGroupGetInfo(serverName, groupName, level, out buf);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupGetInfo(), ret={0}, buf{1}",
                ret, ptrStr(buf)), Logger.netAPILogLevel);


            ThrowExceptionIfError(ret, serverName);
            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupGetMembers([MarshalAs(UnmanagedType.LPWStr)]
                                                          string serverName,
                                                          [MarshalAs(UnmanagedType.LPWStr)]
                                                          string localGroupName,
                                                          int level,
                                                          out IntPtr bufPtr,
                                                          int prefMaxLen,
                                                          out int entriesRead,
                                                          out int totalEntries,
                                                          IntPtr resumeHandle);
        public static int apiNetLocalGroupGetMembers(string serverName, string localGroupName, int level, out IntPtr bufPtr,
            int prefMaxLen, out int entriesRead, out int totalEntries, IntPtr resumeHandle)
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetLocalGroupGetMembers(serverName={0}, localGroupName={1}, level={2}, prefMaxLen={3}, resumeHandle{4}) called",
                serverName, localGroupName, level, prefMaxLen, ptrStr(resumeHandle)), Logger.netAPILogLevel);

            int ret = NetLocalGroupGetMembers(serverName, localGroupName, level, out bufPtr, prefMaxLen,
                                           out entriesRead, out totalEntries, resumeHandle);

            Logger.Log(String.Format("NETAPIMarshal.NetLocalGroupGetMembers(), ret={0}, bufPtr{1}, entriesRead={2}, totalEntries={3}",
                ret, ptrStr(bufPtr), entriesRead, totalEntries), Logger.netAPILogLevel);

            ThrowExceptionIfError(ret, serverName);

            validateEntriesRead("apiNetLocalGroupGetMembers", bufPtr, ref entriesRead, ref totalEntries);

            return ret;
        }

        [DllImport(netAPIDllPath)]
        private extern static int NetInitMemory();
        public static int apiNetInitMemory()
        {
            Logger.Log(String.Format("NETAPIMarshal.apiNetInitMemory() called", Logger.netAPILogLevel));

            int ret = NetInitMemory();

            Logger.Log(String.Format("NETAPIMarshal.NetInitMemory()", Logger.netAPILogLevel));

            ThrowExceptionIfError(ret, "");            

            return ret;
        }

        #endregion

        #endregion

        #region helper_functions

        private static string ptrStr(IntPtr ip)
        {
            if(ip == IntPtr.Zero)
            {
                return "==null";
            }
            else
            {
                return "!=null";
            }
        }

        private static void ThrowExceptionIfError(int NET_API_STATUS, string serverName)
        {
            if (NET_API_STATUS != 0)
            {
                string description = ErrorCodes.WIN32String(NET_API_STATUS);

                Logger.Log(String.Format("NETAPIMarshal.ThrowExceptionIfError: {0}  description: {1}", 
                    NET_API_STATUS, description));

                throw new NETAPIException(description, NET_API_STATUS);

            }
        }

        public static bool validateEntriesRead(string location,
            IntPtr bufPtr, ref int entriesRead, ref int totalEntries)
        {
            if (totalEntries < entriesRead)
            {
                Logger.Log(String.Format("WARNING ({0}): totalEntries ({1}) < entriesRead ({2}).  Proessing only totalEntries",
                 location, totalEntries, entriesRead), Logger.netAPILogLevel);
                entriesRead = totalEntries;
            }

            if (totalEntries > entriesRead)
            {
                Logger.Log(String.Format("WARNING ({0}): totalEntries ({1}) > entriesRead ({2}).  Proessing only entriesRead",
                    location, totalEntries, entriesRead), Logger.netAPILogLevel);
            }

            if (totalEntries == 0)
            {
                Logger.Log(String.Format("WARNING ({0}): entriesRead=0", location),
                        Logger.netAPILogLevel);
                return false;
            }

            else if (bufPtr == IntPtr.Zero)
            {
                Logger.Log(String.Format("WARNING ({0}): bufPtr=null", location),
                     Logger.netAPILogLevel);
                entriesRead = 0;
                return false;
            }

            return true;
        }

        #endregion



    }
}
