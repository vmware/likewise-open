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
using Likewise.LMC.Utilities;
using System.Windows.Forms;

namespace Likewise.LMC.LSAMgmt
{
    public class LSAMgmtAPI
    {

        #region data types

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsaMetricPack_0
        {
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedAuthentications;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedUserLookupsByName;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedUserLookupsById;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedGroupLookupsByName;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedGroupLookupsById;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedOpenSession;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedCloseSession;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedChangePassword;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 unauthorizedAccesses;

        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsaMetricPack_1
        {
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulAuthentications;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedAuthentications;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 rootUserAuthentications;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulUserLookupsByName;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedUserLookupsByName;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulUserLookupsById;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedUserLookupsById;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulGroupLookupsByName;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedGroupLookupsByName;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulGroupLookupsById;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedGroupLookupsById;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulOpenSession;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedOpenSession;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulCloseSession;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedCloseSession;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 successfulChangePassword;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 failedChangePassword;
            [MarshalAs(UnmanagedType.U8)]
            public UInt64 unauthorizedAccesses;

        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct LsaAgentVersion
        {
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 majorVersion;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 minorVersion;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 buildVersion;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class LSA_STATUS
        {
            public LsaAgentVersion version;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 dwUptime;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 dwCount;
            public IntPtr pAuthProviderStatusArray;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class LSA_AUTH_PROVIDER_STATUS
        {
            public IntPtr pszId;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 mode;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 subMode;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 status;
            public IntPtr pszDomain;
            public IntPtr pszForest;
            public IntPtr pszSite;
            public IntPtr pszCell;

            [MarshalAs(UnmanagedType.U4)]
            public UInt32 dwNetworkCheckInterval;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 dwNumTrustedDomains;
            public IntPtr pTrustedDomainInfoArray;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class LSA_TRUSTED_DOMAIN_INFO
        {
            public IntPtr pszDnsDomain;
            public IntPtr pszNetbiosDomain;
            public IntPtr pszTrusteeDnsDomain;
            public IntPtr pszDomainSID;
            public IntPtr pszDomainGUID;
            public IntPtr pszForestName;
            public IntPtr pszClientSiteName;
            public UInt32 dwTrustFlags;
            public UInt32 dwTrustType;
            public UInt32 dwTrustAttributes;
            public UInt32 dwDomainFlags;
            public IntPtr pDCInfo;
            public IntPtr pGCInfo;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class LSA_DC_INFO
        {
            public IntPtr pszName;
            public IntPtr pszAddress;
            public IntPtr pszSiteName;
            public UInt32 dwFlags;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class LWMGMT_LSA_KEYTAB_ENTRY
        {
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 timestamp;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 kvno;
            [MarshalAs(UnmanagedType.U4)]
            public UInt32 enctype;
            public IntPtr pszPrincipal;
            public IntPtr pszPassword;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class LWMGMT_LSA_KEYTAB_ENTRIES
        {
            public UInt32 dwCount;
            public IntPtr pLsaKeyTabEntryArray;
        }

        #endregion

        #region definitions

        private const string clientLibPath = "lwmgmtclient.dll";

        #endregion

        #region public interface


        public static UInt32 LSAMgmtQueryLsaMetrics_0(string serverName, out LsaMetricPack_0 metrics)
        {
            UInt32 result = 0;
            string functionName = "LSAMgmtAPI.LSAMgmtQueryLsaMetrics_0";
            IntPtr pMetrics = IntPtr.Zero;
            metrics = new LsaMetricPack_0();

            try
            {
                Logger.Log(String.Format(
                    "{0}(serverName={1}) called",
                    functionName, serverName), Logger.LSAMgmtLogLevel);

                result = PrivateLSAMgmtAPI.LWMGMTQueryLsaMetrics_0(
                    serverName,
                    out pMetrics);

                if (result != 0)
                {
                    return result;
                }

                if (pMetrics.Equals(IntPtr.Zero))
                {
                    throw new Exception("Failed to read metrics; null handle returned");
                }

                metrics = (LsaMetricPack_0)Marshal.PtrToStructure(pMetrics, typeof(LsaMetricPack_0));

                Logger.Log(String.Format(
                        "{0}: result={1} metrics:\n{2}",
                        functionName,
                        result,
                        MetricsToString_0(metrics)),
                        Logger.LSAMgmtLogLevel);

                int freeSuccess = (int)PrivateLSAMgmtAPI.LWMGMTFreeLsaMetrics_0(pMetrics);
                if (freeSuccess != 0)
                {
                    Logger.Log(String.Format(
                        "WARNING: PrivateLSAMgmtAPI.LWMGMTFreeLsaMetrics_0 reports failure! [code={0}]",
                        freeSuccess),
                        Logger.LogLevel.Panic);
                }

            }
            catch (Exception ex)
            {
                Logger.LogException(functionName, ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }

        public static UInt32 LSAMgmtQueryLsaMetrics_1(string serverName, out LsaMetricPack_1 metrics)
        {

            UInt32 result = 0;
            string functionName = "LSAMgmtAPI.LSAMgmtQueryLsaMetrics_1";

            IntPtr pMetrics = IntPtr.Zero;
            metrics = new LsaMetricPack_1();

            try
            {
                Logger.Log(String.Format(
                    "{0}(serverName={1}) called",
                    functionName, serverName), Logger.LSAMgmtLogLevel);

                result = PrivateLSAMgmtAPI.LWMGMTQueryLsaMetrics_1(
                    serverName, out pMetrics);

                if (result != 0)
                {
                    return result;
                }

                if (pMetrics.Equals(IntPtr.Zero))
                {
                    throw new Exception("Failed to read metrics; null handle returned");
                }

                metrics = (LsaMetricPack_1)Marshal.PtrToStructure(pMetrics, typeof(LsaMetricPack_1));

                Logger.Log(String.Format(
                        "{0}: result={1}",
                        functionName,
                        result),
                        Logger.LSAMgmtLogLevel);


                int freeSuccess = (int)PrivateLSAMgmtAPI.LWMGMTFreeLsaMetrics_1(pMetrics);
                if (freeSuccess != 0)
                {
                    Logger.Log(String.Format(
                        "WARNING: PrivateLSAMgmtAPI.LWMGMTFreeLsaMetrics_1 reports failure! [code={0}]",
                        freeSuccess),
                        Logger.LogLevel.Panic);
                }

            }
            catch (Exception ex)
            {
                Logger.LogException(functionName, ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }
            return result;

        }

        public static UInt32 LWMGMTQueryLsaStatus(string serverName, out IntPtr pLsaStatus)
        {
            UInt32 result = 0;
            string functionName = "LSAMgmtAPI.LWMGMTQueryLsaStatus";

            IntPtr lsa_status = IntPtr.Zero;

            try
            {
                Logger.Log(String.Format(
                    "{0}(serverName={1}) called",
                    functionName, serverName), Logger.LSAMgmtLogLevel);

                result = PrivateLSAMgmtAPI.LWMGMTQueryLsaStatus(
                                serverName,
                                out lsa_status);

                if (result != 0)
                {
                    pLsaStatus = IntPtr.Zero;
                    return result;
                }

                if (lsa_status.Equals(IntPtr.Zero))
                {
                    throw new Exception("Failed to read status information; null handle returned");
                }

                Logger.Log(String.Format(
                   "LWMGMTQueryLsaStatus returns non zero value for pLsaStatus {0}", lsa_status),
                   Logger.LSAMgmtLogLevel);

                Logger.Log(String.Format(
                     "{0}: result={1}",
                     functionName,
                     result),
                     Logger.LSAMgmtLogLevel);

                pLsaStatus = lsa_status;
            }
            catch (Exception ex)
            {
                Logger.LogException(functionName, ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }

                pLsaStatus = IntPtr.Zero;
            }

            return result;

        }

        public static UInt32 LWMGMTFreeLsaStatus(IntPtr pLsaStatus)
        {
            UInt32 result = 0;
            string functionName = "LSAMgmtAPI.LWMGMTFreeLsaStatus";

            try
            {
                Logger.Log(String.Format(
                            "{0} called",
                            functionName), Logger.LSAMgmtLogLevel);

                if (!IntPtr.Zero.Equals(pLsaStatus))
                {
                    result = PrivateLSAMgmtAPI.LWMGMTFreeLsaStatus(pLsaStatus);
                    if (result != 0)
                    {
                        Logger.Log(String.Format(
                            "WARNING: PrivateLSAMgmtAPI.LWMGMTFreeLsaStatus reports failure! [code={0}]",
                            result),
                        Logger.LogLevel.Panic);
                    }
                }

                Logger.Log(String.Format(
                            "{0}: result={1}",
                            functionName,
                            result),
                            Logger.LSAMgmtLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException(functionName, ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }

            return result;
        }

        public static UInt32 LWMGMTReadKeyTab(string serverName,
                                              string KeyTabPath,
                                              UInt32 LastRecordId,
                                              UInt32 RecordsPerPage,
                                              out IntPtr pKeyTabEntries)
        {
            UInt32 result = 0;
            string functionName = "LSAMgmtAPI.LWMGMTReadKeyTab";

            IntPtr kaytab_entries = IntPtr.Zero;

            try
            {
                Logger.Log(String.Format(
                    "{0}(serverName={1}) called",
                    functionName, serverName), Logger.LSAMgmtLogLevel);

                result = PrivateLSAMgmtAPI.LWMGMTReadKeyTab(
                                 serverName,
                                 KeyTabPath,
                                 LastRecordId,
                                 RecordsPerPage,
                                 out kaytab_entries);

                if (result != 0)
                {
                    pKeyTabEntries = IntPtr.Zero;
                    return result;
                }

                if (kaytab_entries.Equals(IntPtr.Zero))
                {
                    throw new Exception("Failed to read kay tab entry information; null handle returned");
                }

                Logger.Log(String.Format(
                   "LWMGMTReadKeyTab returns non zero value for kaytab_entries {0}", kaytab_entries),
                   Logger.LSAMgmtLogLevel);

                Logger.Log(String.Format(
                     "{0}: result={1}",
                     functionName,
                     result),
                     Logger.LSAMgmtLogLevel);

                pKeyTabEntries = kaytab_entries;
            }
            catch (Exception ex)
            {
                Logger.LogException(functionName, ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }

                pKeyTabEntries = IntPtr.Zero;
            }

            return result;

        }

        public static UInt32 LWMGMTFreeKeyTabEntries(IntPtr pKeyTabEntries)
        {
            UInt32 result = 0;
            string functionName = "LSAMgmtAPI.LWMGMTFreeKeyTabEntries";

            try
            {
                Logger.Log(String.Format(
                            "{0} called",
                            functionName), Logger.LSAMgmtLogLevel);

                if (!IntPtr.Zero.Equals(pKeyTabEntries))
                {
                    result = PrivateLSAMgmtAPI.LWMGMTFreeKeyTabEntries(pKeyTabEntries);
                    if (result != 0)
                    {
                        Logger.Log(String.Format(
                            "WARNING: PrivateLSAMgmtAPI.LWMGMTFreeKeyTabEntries reports failure! [code={0}]",
                            result),
                        Logger.LogLevel.Panic);
                    }
                }

                Logger.Log(String.Format(
                            "{0}: result={1}",
                            functionName,
                            result),
                            Logger.LSAMgmtLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException(functionName, ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }
            }

            return result;

        }

        public static UInt32 LWMGMTCountKeyTabEntries(string serverName, string keyTabPath, out UInt32 pdwCount)
        {
            UInt32 result = 0;
            string functionName = "LSAMgmtAPI.LWMGMTCountKeyTabEntries";
            UInt32 count = 0;

            try
            {
                Logger.Log(String.Format(
                    "{0}(serverName={1}) called",
                    functionName, serverName), Logger.LSAMgmtLogLevel);

                result = PrivateLSAMgmtAPI.LWMGMTCountKeyTabEntries(
                                serverName,
                                keyTabPath,
                                out count);

                if (result != 0)
                {
                    pdwCount = 0;
                    return result;
                }

                Logger.Log(String.Format(
                   "LWMGMTCountKeyTabEntries returns non zero value for pdwCount {0}", count),
                   Logger.LSAMgmtLogLevel);

                Logger.Log(String.Format(
                     "{0}: result={1}",
                     functionName,
                     result),
                     Logger.LSAMgmtLogLevel);

                pdwCount = count;
            }
            catch (Exception ex)
            {
                Logger.LogException(functionName, ex);
                if (result == 0)
                {
                    result = 0xFFFFFFFF;
                }

                pdwCount = 0;
            }

            return result;

        }

        public static string MetricsToString_0(LsaMetricPack_0 metrics)
        {
            string[] parts = new string[] {
                "\nfailedAuthentications: ",
                metrics.failedAuthentications.ToString(),
                "\nfailedUserLookupsByName:  ",
                metrics.failedUserLookupsByName.ToString(),
                "\nfailedUserLookupsById:  ",
                metrics.failedUserLookupsById.ToString(),
                "\nfailedGroupLookupsByName:  ",
                metrics.failedGroupLookupsByName.ToString(),
                "\nfailedGroupLookupsById:  ",
                metrics.failedGroupLookupsById.ToString(),
                "\nfailedOpenSession:  ",
                metrics.failedOpenSession.ToString(),
                "\nfailedCloseSession:  ",
                metrics.failedCloseSession.ToString(),
                "\nfailedChangePassword:  ",
                metrics.failedChangePassword.ToString(),
                "\nunauthorizedAccesses:  ",
                metrics.unauthorizedAccesses.ToString(),
                "\n"
            };

            string result = String.Concat(parts);
            return result;
        }

        #endregion

        #region API functions
        private class PrivateLSAMgmtAPI
        {

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTQueryLsaMetrics_0(
                [MarshalAs(UnmanagedType.LPStr)] string pszHostname,
                out IntPtr pPack
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTFreeLsaMetrics_0(
                IntPtr pPack
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTQueryLsaMetrics_1(
                [MarshalAs(UnmanagedType.LPStr)] string pszHostname,
                out IntPtr pPack
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTFreeLsaMetrics_1(
                IntPtr pPack
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTQueryLsaStatus(
                [MarshalAs(UnmanagedType.LPStr)] string pszHostname,
                out IntPtr pPack
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTFreeLsaStatus(
                IntPtr pPack
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTReadKeyTab(
                [MarshalAs(UnmanagedType.LPStr)] string pszHostname,
                [MarshalAs(UnmanagedType.LPStr)] string pszKeyTabPath,
                [MarshalAs(UnmanagedType.U4)] UInt32 dwLastRecordId,
                [MarshalAs(UnmanagedType.U4)] UInt32 dwRecordsPerPage,
                out IntPtr ppKeyTabEntries
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTFreeKeyTabEntries(
                IntPtr pKeyTabEntries
                );

            [DllImport(clientLibPath)]
            public extern static UInt32 LWMGMTCountKeyTabEntries(
                [MarshalAs(UnmanagedType.LPStr)] string pszHostname,
                [MarshalAs(UnmanagedType.LPStr)] string pszKeyTabPath,
                [MarshalAs(UnmanagedType.U4)] out UInt32 pdwCount
                );

        }
        #endregion

    }
}
