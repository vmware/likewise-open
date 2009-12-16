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
using Microsoft.Win32;
using Likewise.LMC.Utilities;
using System.DirectoryServices;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using System.Collections.Generic;
using Likewise.LMC.Netlogon;
using Likewise.LMC.Netlogon.Interop;

namespace Likewise.LMC.Eventlog
{
    /// <summary>
    /// This class provides additional functions to retrieve information about events. The EventLog
    /// class provided in .NET does not provide everything we need
    /// </summary>
    public class EventAPI
    {
        #region data types

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct EventLogRecord
        {
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 dwEventRecordId;
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszEventTableCategoryId;
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszEventType;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 dwEventDateTime;  //seconds since Jan. 1 1970
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszEventSource;
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszEventCategory;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 dwEventSourceId;
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszUser;
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszComputer;
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszDescription;
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszData;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct EVENT_LOG_CATEGORY
        {
            [MarshalAs(UnmanagedType.LPStr)]
            public string pszCategory;
        }

        public enum TableCategoryType
        {
            Application = 0,
            WebBrowser = 1,
            Security = 2,
            System = 3
        }

        #endregion

        #region definitions

        private const string clientLibPath = "eventlog.dll";

        #endregion

        #region public interface

        //.NET appears to lack these very elementary interoperability
        //functions for reading UNIX timestamps, even though
        //this capability is available in C/C++ in Windows.
        public static DateTime ConvertFromUnixTimestamp(UInt32 unixTimestamp)
        {
            DateTime origin = new DateTime(1970, 1, 1, 0, 0, 0, 0);

            Logger.Log(String.Format("ConvertFromUnixTimeStamp: origin={0}",
                origin), Logger.eventLogLogLevel);

            DateTime present = origin.AddSeconds((double) unixTimestamp);

            Logger.Log(String.Format("ConvertFromUnixTimeStamp: {0} --> {1}",
                unixTimestamp, present), Logger.eventLogLogLevel);

            return present;

        }

        public static UInt32 ConvertToUnixTimestamp(DateTime date)
        {
            DateTime origin = new DateTime(1970, 1, 1, 0, 0, 0, 0);
            TimeSpan diff = date - origin;

            Logger.Log(String.Format("ConvertFromUnixTimeStamp: origin={0}, diff={1}",
                origin, diff), Logger.eventLogLogLevel);

            UInt32 unixTimestamp = (UInt32) Math.Floor(diff.TotalSeconds);

            Logger.Log(String.Format("ConvertToUnixTimestamp: {0} --> {1}",
                date, unixTimestamp), Logger.eventLogLogLevel);

            return unixTimestamp;
        }

        public static UInt32 OpenEventLog(string serverName, out IntPtr hEventLog)
        {
            UInt32 result = 0;
            hEventLog = IntPtr.Zero;
            string hostFQDN = serverName;

            try
            {
                ReadRemoteHostFQDN(serverName, out hostFQDN);

                if (!String.IsNullOrEmpty(hostFQDN))
                {
                    Logger.Log(String.Format("OpenEventLog(serverName={0}) called",
                    hostFQDN), Logger.eventLogLogLevel);

                    result = PrivateEventAPI.LWIOpenEventLog(
                        hostFQDN.ToLower(), out hEventLog);
                }
                else
                {
                   Logger.Log(String.Format("OpenEventLog(serverName={0}) called",
                   serverName), Logger.eventLogLogLevel);

                    result = PrivateEventAPI.LWIOpenEventLog(
                        serverName, out hEventLog);
                }

                if (result != 0)
                {
                    return result;
                }

                if (hEventLog == IntPtr.Zero)
                {
                    throw new Exception("Failed to open eventlog; null handle returned");
                }

                Logger.Log(String.Format(
                        "OpenEventLog(); result={0}, hEventLog={1:X}",
                    result, hEventLog.ToInt32()));
            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.OpenEventLog", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }

        public static void ReadRemoteHostFQDN(string hostname, out string hostFQDN)
        {
            hostFQDN = string.Empty; string domain = string.Empty;

            uint error = CNetlogon.GetCurrentDomain(out domain);

            if (error != 0 && String.IsNullOrEmpty(domain))
            {
                return;
            }

            string[] rootDNcom = domain.Split('.');

            string rootDN = ""; string errorMessage = "";
            foreach (string str in rootDNcom)
            {
                string temp = string.Concat("dc=", str, ",");
                rootDN = string.Concat(rootDN, temp);
            }
            rootDN = rootDN.Substring(0, rootDN.Length - 1);

            try
            {
                DirectoryContext dirContext = DirectoryContext.CreateDirectoryContext
                                            (domain,
                                            rootDN,
                                            null,
                                            null,
                                            389,
                                            false,
                                            out errorMessage);

                if (!String.IsNullOrEmpty(errorMessage))
                {
                    Logger.ShowUserError(errorMessage);
                }

                if (dirContext == null)
                {
                    return;
                }

                List<LdapEntry> ldapEntries = new List<LdapEntry>();

                string[] attrs = { "name", "dNSHostName", null };

                int ret = dirContext.ListChildEntriesSynchronous
                (rootDN,
                 LdapAPI.LDAPSCOPE.SUB_TREE,
                string.Format("(&(objectClass=computer)(cn={0}))", hostname),
                attrs,
                false,
                out ldapEntries);

                if (ldapEntries == null)
                {
                    return;
                }

                LdapEntry ldapNextEntry = ldapEntries[0];

                string[] attrsList = ldapNextEntry.GetAttributeNames();

                Logger.Log("The number of attributes are " + attrsList.Length, Logger.ldapLogLevel);

                if (attrsList != null)
                {
                    foreach (string attr in attrsList)
                    {
                        if (attr.Trim().Equals("dNSHostName"))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirContext);
                            if (attrValues != null && attrValues.Length > 0)
                            {
                                hostFQDN = attrValues[0].stringData;
                                break;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                hostFQDN = string.Empty;
                Logger.LogException("EventAPI.ReadRemoteHostFQDN", ex);
            }
        }


        private static void GetUserDomain(out string domain)
        {
            domain = string.Empty;
            try
            {
                string domainName = string.Empty;
                CNetlogon.LWNET_DC_INFO DCInfo;
                uint netlogonError = CNetlogon.GetDCName(domainName, 0, out DCInfo);
                if (netlogonError == 0 && !String.IsNullOrEmpty(DCInfo.DomainControllerName))
                {
                    domain = DCInfo.FullyQualifiedDomainName;
                }
            }
            catch (Exception ex)
            {
                Logger.Log("Exception occured while getting DCInfo ," + ex.Message);
                return;
            }
        }


        public static UInt32 ReadEventLog(IntPtr hEventLog, UInt32 dwLastRecordId,
            UInt32 nRecordsPerPage, string sqlQuery, out UInt32 pdwNumReturned, out EventLogRecord[] records)
        {

            UInt32 result = 0;
            records = null;
            pdwNumReturned = 0;
            IntPtr bufPtr = IntPtr.Zero;

            try
            {

                Logger.Log(String.Format(
                 "ReadEventLog(hEventLog={0:X}, dwLastRecordId={1}, nRecordsPerPage={2}) called",
                 hEventLog.ToInt32(), dwLastRecordId, nRecordsPerPage), Logger.eventLogLogLevel);

                result = PrivateEventAPI.LWIReadEventLog(hEventLog, dwLastRecordId,
                    nRecordsPerPage, sqlQuery, out pdwNumReturned, out bufPtr);

                Logger.Log(String.Format(
                 "ReadEventLog_after(result={0}, pdwNumReturned={1}, bufPtr={2:X})",
                 result, pdwNumReturned, bufPtr.ToInt32()), Logger.eventLogLogLevel);


                if (pdwNumReturned > 0)
                {
                    records = new EventLogRecord[pdwNumReturned];

                    IntPtr iter = bufPtr;

                    for (int i = 0; i < pdwNumReturned && iter != IntPtr.Zero; i++)
                    {
                        records[i] = (EventLogRecord)Marshal.PtrToStructure(iter, typeof(EventLogRecord));
                        iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(EventLogRecord)));
                    }
                }

                //this function produces a lot of output, so only run it at Debug level.
                LogEventLogRecords(records, Logger.eventLogLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.OpenEventLog", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            //Marshal.FreeHGlobal(bufPtr);
            return result;

        }

        public static UInt32 CountEventLog(IntPtr hEventLog,
            string sqlQuery, out UInt32 pdwNumMatched)
        {

            UInt32 result = 0;
            pdwNumMatched = 0;

            try
            {

                Logger.Log(String.Format(
                 "CountEventLog(hEventLog={0}) called",
                 hEventLog), Logger.eventLogLogLevel);

                result = PrivateEventAPI.LWICountEventLog(hEventLog,
                    sqlQuery, out pdwNumMatched);

                Logger.Log(String.Format(
                 "CountEventLog_after(result={0}, pdwNumMatched={1})",
                 result, pdwNumMatched), Logger.eventLogLogLevel);

            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.CountEventLog", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }

        public static UInt32 CloseEventLog(IntPtr hEventLog)
        {
            UInt32 result = 0;

            try
            {
                Logger.Log(String.Format("CloseEventLog(hEventLog={0:X}) called",
                    hEventLog.ToInt32(), Logger.eventLogLogLevel));

                result = PrivateEventAPI.LWICloseEventLog(hEventLog);

                Logger.Log(String.Format("CloseEventLog(); result={0}", result), Logger.eventLogLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.CloseEventLog", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }

            return result;

        }

        public static UInt32 GetCategoryCount(IntPtr hEventLog,
                                              out UInt32 pdwNumMatched)
        {

            UInt32 result = 0;
            pdwNumMatched = 0;

            try
            {
                Logger.Log(String.Format(
                 "GetCategoryCount(hEventLog={0}, pdwNumMatched={1}) called",
                 hEventLog, pdwNumMatched), Logger.eventLogLogLevel);

                result = PrivateEventAPI.LWIGetCategoryCount(hEventLog,
                                                             out pdwNumMatched);

                Logger.Log(String.Format(
                 "GetCategoryCount_after(result={0}, pdwNumMatched={1})",
                 result, pdwNumMatched), Logger.eventLogLogLevel);

            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.GetCategoryCount", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }


        public static UInt32 GetDistinctCategories(IntPtr hEventLog,
                                                   UInt32 pdwNumMatched,
                                                   out string[] EventCategoryArry)
        {

            UInt32 result = 0;
            IntPtr pEVENT_LOG_CATEGORY = IntPtr.Zero;
            EventCategoryArry = null;

            try
            {
                Logger.Log(String.Format(
                 "GetDistinctCategories(hEventLog={0}, pdwNumMatched={1}) called",
                 hEventLog, pdwNumMatched), Logger.eventLogLogLevel);

                result = PrivateEventAPI.LWIGetDistinctCategories(hEventLog,
                                                                  pdwNumMatched,
                                                                  out pEVENT_LOG_CATEGORY);

                Logger.Log(String.Format(
                 "GetDistinctCategories_after(result={0}, pEVENT_LOG_CATEGORY={1})",
                 result, pEVENT_LOG_CATEGORY), Logger.eventLogLogLevel);

                if (pdwNumMatched > 0 && pEVENT_LOG_CATEGORY != IntPtr.Zero)
                {
                    EventCategoryArry = new string[pdwNumMatched];

                    IntPtr iter = pEVENT_LOG_CATEGORY;

                    for (int i = 0; i < pdwNumMatched && iter != IntPtr.Zero; i++)
                    {
                        EVENT_LOG_CATEGORY sEVENT_LOG_CATEGORY = (EVENT_LOG_CATEGORY)Marshal.PtrToStructure(iter, typeof(EVENT_LOG_CATEGORY));
                        EventCategoryArry[i] = sEVENT_LOG_CATEGORY.pszCategory;
                        iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(EVENT_LOG_CATEGORY)));
                    }
                }

                //this function produces a lot of output, so only run it at Debug level.
                LogEventLogCategories(EventCategoryArry, Logger.eventLogLogLevel);

            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.GetDistinctCategories", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }

        public static UInt32 DeleteFromEventLog(IntPtr hEventLog,
                                                string sqlfilter)
        {
            UInt32 result = 0;

            try
            {
                Logger.Log(String.Format(
                 "DeleteFromEventLog(hEventLog={0}, sqlfilter={1}) called",
                 hEventLog, sqlfilter), Logger.eventLogLogLevel);

                result = PrivateEventAPI.LWIDeleteFromEventLog(hEventLog,
                                                               sqlfilter);

                Logger.Log(String.Format(
                 "DeleteFromEventLog(result={0}",
                 result), Logger.eventLogLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.DeleteFromEventLog", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }

        public static UInt32 ClearEventLog(IntPtr hEventLog)
        {
            UInt32 result = 0;

            try
            {
                Logger.Log(String.Format(
                 "ClearEventLog(hEventLog={0}) called",
                 hEventLog), Logger.eventLogLogLevel);

                result = PrivateEventAPI.LWIClearEventLog(hEventLog);

                Logger.Log(String.Format(
                 "ClearEventLog(result={0}",
                 result), Logger.eventLogLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.ClearEventLog", ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }

        /// <summary>
        /// Returns the available sources for a particular log on a given machine
        /// </summary>
        /// <param name="sMachine">The machine who logs are being retrieved</param>
        /// <param name="sLogName">The name of the particular log whose sources are to be enumerated</param>
        /// <returns>An array of strings containing the source names</returns>
        static public string[] GetEventSources(string sMachine, string sLogName)
        {
            try
            {
                // get the information from the registry
                RegistryKey rklm = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, sMachine);
                RegistryKey rk = rklm.OpenSubKey(@"SYSTEM\CurrentControlSet\Services\Eventlog\" + sLogName);

                // to get the sources, get the "sources" key value
                object o = rk.GetValue("Sources");
                rk.Close();
                rklm.Close();

                if (o is string[])
                    return (string[])o;
            }
            catch (Exception ex)
            {
                Logger.LogException("EventAPI.GetEventSources", ex);
            }

            return null;
        }

        /// <summary>
        /// Replaces environment strings (of form %foo%) with their values as set on the indicated machine.
        /// Currently, this function only supports %systemroot%
        /// </summary>
        /// <param name="sMachineName">The machine used for resolving environment variables</param>
        /// <param name="s">The string whose variables are to be replaced</param>
        /// <returns>The string, with replacements</returns>
        static public string ReplaceEnvironmentVariables(string sMachineName, string s)
        {
            string sSysRoot = null;

            // see if %systemroot% is present
            for (; ; )
            {
                int ich = s.ToLower().IndexOf("%systemroot%");
                if (ich < 0)
                    break;

                // get the system root value if we don't already have it
                if (sSysRoot == null)
                {
                    RegistryKey rklm = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, sMachineName);
                    RegistryKey rk = rklm.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion");
                    if (rk == null)
                        break;

                    sSysRoot = (string) rk.GetValue("SystemRoot");
                    if (sSysRoot == null)
                        break;
                }

                // replace (note, "12" is the length of %systemroot%)
                s = s.Substring(0, ich) + sSysRoot + s.Substring(ich + 12);

                // keep going to replace other occurrences
            }
            return s;

        }

        /// <summary>
        /// Returns the filename associated with a log
        /// </summary>
        /// <param name="sMachine">The machine who logs are being retrieved</param>
        /// <param name="sLogName">The name of the particular log whose sources are to be enumerated</param>
        /// <returns>The filename.</returns>
        static public string GetLogFilename(string sMachineName, string sLogName)
        {
            try
            {
                // get the information from the registry
                // get the information from the registry
                RegistryKey rklm = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, sMachineName);
                RegistryKey rk = rklm.OpenSubKey(@"SYSTEM\CurrentControlSet\Services\Eventlog\" + sLogName);

                // get the "File" value
                string sFile = rk.GetValue("File", null, RegistryValueOptions.DoNotExpandEnvironmentNames) as string;
                if (sFile == null)
                    return sFile;

                // replace any environment variables
                sFile = ReplaceEnvironmentVariables(sMachineName, sFile);

                if (sFile!=null && sFile.Length>3)
                {
                    // convert x:\ into \\machinename\x$
                    char ch0 = sFile[0];
                    if (char.IsLetter(ch0) && sFile[1]==':' && sFile[2]=='\\')
                        return @"\\" + sMachineName + @"\" + ch0.ToString() + "$" + sFile.Substring(2);
                }
            }
            catch(Exception ex)
            {
                Logger.LogException("EventAPI.GetLogFilename", ex);
            }
            return null;

        }

        #endregion

        #region API functions
        private class PrivateEventAPI
        {
            [DllImport(clientLibPath)]
            public extern static UInt32 LWIOpenEventLog(
                                            [MarshalAs(UnmanagedType.LPStr)] string serverName,
                                            out IntPtr hEventLog);



            [DllImport(clientLibPath)]
            public extern static UInt32 LWIReadEventLog(
                                            IntPtr hEventLog,
                                            UInt32 dwLastRecordId,
                                            UInt32 nRecordsPerPage,
                                            [MarshalAs(UnmanagedType.LPWStr)] string sqlFilter,
                                            out UInt32 pdwNumReturned,
                                            out IntPtr bufPtr
                                            );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWICountEventLog(
                                            IntPtr hEventLog,
                                            [MarshalAs(UnmanagedType.LPWStr)] string sqlFilter,
                                            out UInt32 pdwNumMatched
                                            );


            [DllImport(clientLibPath)]
            public extern static UInt32 LWICloseEventLog(
                                            IntPtr hEventLog
                                            );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWIGetCategoryCount(
                                            IntPtr hEventLog,
                                            out UInt32 pdwNumMatched
                                            );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWIGetDistinctCategories(
                                            IntPtr hEventLog,
                                            UInt32 dwCatCount,
                                            out IntPtr ppCategory
                                            );
            [DllImport(clientLibPath)]
            public extern static UInt32 LWIDeleteFromEventLog(
                                            IntPtr hEventLog,
                                            [MarshalAs(UnmanagedType.LPWStr)] string sqlFilter
                                            );
            [DllImport(clientLibPath)]
            public extern static UInt32 LWIClearEventLog(
                                            IntPtr hEventLog
                                            );

        }
        #endregion

        #region helper functions

        private static void LogEventLogRecords(EventLogRecord[] records, Logger.LogLevel level)
        {
            //save a bit of CPU if the eventlog log has been silenced
            if (Logger.currentLogLevel < level)
            {
                return;
            }

            string result = "EventLogRecords[]: ";

            if (records == null)
            {
                result += "null";
            }

            else if (records.Length == 0)
            {
                result += " Count=0";
            }

            else
            {

                int i = 0;

                foreach (EventLogRecord record in records)
                {
                    result += String.Format("\n\t{0}: recordID={1}, eventID={2}, eventType={3}, eventTime={4}",
                        i, record.dwEventRecordId, record.pszEventType, record.dwEventDateTime, record.pszEventCategory);
                    result += String.Format("\n\t\tsource: {0}, user: {1}, computer: {2}",
                        record.pszEventSource, record.pszUser, record.pszComputer);
                    result += String.Format("\n\t\tdescription: {0}", record.pszDescription);
                    result += String.Format("\n\t\tdata: {0}", record.pszData);

                    i++;
                }
            }

            Logger.Log(result, level);
        }

        private static void LogEventLogCategories(string[] categories,Logger.LogLevel level)
        {
            //save a bit of CPU if the eventlog log has been silenced
            if (Logger.currentLogLevel < level)
            {
                return;
            }

            string result = "EventLogCategories[]: ";

            if (categories == null)
            {
                result += "null";
            }

            else if (categories.Length == 0)
            {
                result += " Count = 0 ";
            }

            else
            {
                int i = 0;
                foreach (string category in categories)
                {
                    result += String.Format("\n\t{0}: categoryID={1}", i, category);
                    i++;
                }
            }

            Logger.Log(result, level);
        }

        #endregion

    }
}
