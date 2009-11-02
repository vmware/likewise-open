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
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.LSAMgmt
{
    public class LSAMgmtMetrics
    {
        #region data

        public UInt64 successfulAuthentications = 0;
        public UInt64 failedAuthentications = 0;
        public UInt64 rootUserAuthentications = 0;
        public UInt64 successfulUserLookupsByName = 0;
        public UInt64 failedUserLookupsByName = 0;
        public UInt64 successfulUserLookupsById = 0;
        public UInt64 failedUserLookupsById = 0;
        public UInt64 successfulGroupLookupsByName = 0;
        public UInt64 failedGroupLookupsByName = 0;
        public UInt64 successfulGroupLookupsById = 0;
        public UInt64 failedGroupLookupsById = 0;
        public UInt64 successfulOpenSession = 0;
        public UInt64 failedOpenSession = 0;
        public UInt64 successfulCloseSession = 0;
        public UInt64 failedCloseSession = 0;
        public UInt64 successfulChangePassword = 0;
        public UInt64 failedChangePassword = 0;
        public UInt64 unauthorizedAccesses = 0;

        #endregion

        public LSAMgmtMetrics()
        {
        }


        public LSAMgmtMetrics(string hostname, int infolevel)
            : this()
        {
            int ret = GetLSAMgmtMetrics(hostname, infolevel);

        }

        public int GetLSAMgmtMetrics(string hostname, int infoLevel)
        {
            int ret = 0;

            if (infoLevel < 0)
            {
                infoLevel = 0;
            }
            if (infoLevel > 1)
            {
                infoLevel = 1;
            }

            if (infoLevel == 0)
            {
                LSAMgmtAPI.LsaMetricPack_0 metrics;
                ret = (int)LSAMgmtAPI.LSAMgmtQueryLsaMetrics_0(hostname, out metrics);

                Logger.Log(String.Format(
                    "LSAMgmtAdapter.GetLSAMgmtMetrics: ret={0}, metrics:\n{0}", ret,
                    LSAMgmtAPI.MetricsToString_0(metrics)));

                /*
                if (ret != 0)
                {
                    Logger.Log("GetLSAMgmtMetrics failed.  Clearing Data.");
                    ClearData();
                    return ret;
                }
                 */

                failedAuthentications = metrics.failedAuthentications;
                failedUserLookupsByName = metrics.failedUserLookupsByName;
                failedUserLookupsById = metrics.failedUserLookupsById;
                failedGroupLookupsByName = metrics.failedGroupLookupsByName;
                failedGroupLookupsById = metrics.failedGroupLookupsById;
                failedOpenSession = metrics.failedOpenSession;
                failedCloseSession = metrics.failedCloseSession;
                failedChangePassword = metrics.failedChangePassword;
                unauthorizedAccesses = metrics.unauthorizedAccesses;


            }
            else if (infoLevel == 1)
            {
                LSAMgmtAPI.LsaMetricPack_1 metrics;
                ret = (int)LSAMgmtAPI.LSAMgmtQueryLsaMetrics_1(hostname, out metrics);

                /*
                if (ret != 0)
                {
                    Logger.Log("GetLSAMgmtMetrics failed.  Clearing Data.");
                    ClearData();
                    return ret;
                }
                */
                 
                successfulAuthentications = metrics.successfulAuthentications;
                failedAuthentications = metrics.failedAuthentications;
                rootUserAuthentications = metrics.rootUserAuthentications;
                successfulUserLookupsByName = metrics.successfulUserLookupsByName;
                failedUserLookupsByName = metrics.failedUserLookupsByName;
                successfulUserLookupsById = metrics.successfulUserLookupsById;
                failedUserLookupsById = metrics.failedUserLookupsById;
                successfulGroupLookupsByName = metrics.successfulGroupLookupsByName;
                failedGroupLookupsByName = metrics.failedGroupLookupsByName;
                successfulGroupLookupsById = metrics.successfulGroupLookupsById;
                failedGroupLookupsById = metrics.failedGroupLookupsById;
                successfulOpenSession = metrics.successfulOpenSession;
                failedOpenSession = metrics.failedOpenSession;
                successfulCloseSession = metrics.successfulCloseSession;
                failedCloseSession = metrics.failedCloseSession;
                successfulChangePassword = metrics.successfulChangePassword;
                failedChangePassword = metrics.failedChangePassword;
                unauthorizedAccesses = metrics.unauthorizedAccesses;


            }

            return ret;
        }

        public void ClearData()
        {
            successfulAuthentications = 0;
            failedAuthentications = 0;
            rootUserAuthentications = 0;
            successfulUserLookupsByName = 0;
            failedUserLookupsByName = 0;
            successfulUserLookupsById = 0;
            failedUserLookupsById = 0;
            successfulGroupLookupsByName = 0;
            failedGroupLookupsByName = 0;
            successfulGroupLookupsById = 0;
            failedGroupLookupsById = 0;
            successfulOpenSession = 0;
            failedOpenSession = 0;
            successfulCloseSession = 0;
            failedCloseSession = 0;
            successfulChangePassword = 0;
            failedChangePassword = 0;
            unauthorizedAccesses = 0;
        }

        public override string ToString()
        {
            string[] components = new string[] {
                "\nsuccessfulAuthentications:          ", successfulAuthentications.ToString(), 
                "\nfailedAuthentications:              ", failedAuthentications.ToString(), 
                "\nrootUserAuthentications:            ", rootUserAuthentications.ToString(), 
                "\nsuccessfulUserLookupsByName:        ", successfulUserLookupsByName.ToString(), 
                "\nfailedUserLookupsByName:            ", failedUserLookupsByName.ToString(), 
                "\nsuccessfulUserLookupsById:          ", successfulUserLookupsById.ToString(),
                "\nfailedUserLookupsById:              ", failedUserLookupsById.ToString(), 
                "\nsuccessfulGroupLookupsByName:       ", successfulGroupLookupsByName.ToString(),
                "\nfailedGroupLookupsByName:           ", failedGroupLookupsByName.ToString(),
                "\nsuccessfulGroupLookupsById:         ", successfulGroupLookupsById.ToString(), 
                "\nfailedGroupLookupsById:             ", failedGroupLookupsById.ToString(),
                "\nsuccessfulOpenSession:              ", successfulOpenSession.ToString(),
                "\nfailedOpenSession:                  ", failedOpenSession.ToString(), 
                "\nsuccessfulCloseSession:             ", successfulCloseSession.ToString(),
                "\nfailedCloseSession:                 ", failedCloseSession.ToString(), 
                "\nsuccessfulChangePassword:           ", successfulChangePassword.ToString(),
                "\nfailedChangePassword:               ", failedChangePassword.ToString(),
                "\nunauthorizedAccesses:               ", unauthorizedAccesses.ToString()
            };

            string result = String.Concat(components);
            return result;
        }

    }

    public class LSAMgmtStatus
    {
        #region Helper functions

        public static LsaAgentStatus GetLSAMgmtStatus(string hostname)
        {
            LsaAgentStatus agentStatus = null;
            IntPtr pLsaStatus = IntPtr.Zero;

            try
            {
                LSAMgmtAPI.LSA_STATUS LsaStatus = new LSAMgmtAPI.LSA_STATUS();

                int ret = (int)LSAMgmtAPI.LWMGMTQueryLsaStatus(hostname, out pLsaStatus);
                if (ret != 0)
                {
                    //return null;
                    throw new ApplicationException("Failed to query LSA Agent status");
                }                

                Marshal.PtrToStructure(pLsaStatus, LsaStatus);               

                agentStatus = new LsaAgentStatus();

                agentStatus.MajorVersion = LsaStatus.version.majorVersion;
                agentStatus.MinorVersion = LsaStatus.version.minorVersion;
                agentStatus.BuildVersion = LsaStatus.version.buildVersion;

                agentStatus.Uptime = LsaStatus.dwUptime;

                if (LsaStatus.dwCount > 0)
                {
                    IntPtr pCur = LsaStatus.pAuthProviderStatusArray;

                    for (int index = 0; index < LsaStatus.dwCount; index++)
                    {
                        LsaAuthProviderStatus authProviderStatus = new LsaAuthProviderStatus();
                        LSAMgmtAPI.LSA_AUTH_PROVIDER_STATUS pProviderStatus = new LSAMgmtAPI.LSA_AUTH_PROVIDER_STATUS();

                        Marshal.PtrToStructure(pCur, pProviderStatus);

                        if (!IntPtr.Zero.Equals(pProviderStatus.pszId))
                        {
                            authProviderStatus.Id = Marshal.PtrToStringAuto(pProviderStatus.pszId);
                        }

                        if (!IntPtr.Zero.Equals(pProviderStatus.pszDomain))
                        {
                            authProviderStatus.Domain = Marshal.PtrToStringAuto(pProviderStatus.pszDomain);
                        }

                        if (!IntPtr.Zero.Equals(pProviderStatus.pszForest))
                        {
                            authProviderStatus.Forest = Marshal.PtrToStringAuto(pProviderStatus.pszForest);
                        }

                        if (!IntPtr.Zero.Equals(pProviderStatus.pszSite))
                        {
                            authProviderStatus.Site = Marshal.PtrToStringAuto(pProviderStatus.pszSite);
                        }

                        if (!IntPtr.Zero.Equals(pProviderStatus.pszCell))
                        {
                            authProviderStatus.Cell = Marshal.PtrToStringAuto(pProviderStatus.pszCell);
                        }

                        authProviderStatus.Mode = (LsaAuthProviderMode)Enum.Parse(typeof(LsaAuthProviderMode), pProviderStatus.mode.ToString());
                        authProviderStatus.Submode = (LsaAuthProviderSubMode)Enum.Parse(typeof(LsaAuthProviderSubMode), pProviderStatus.subMode.ToString());
                        authProviderStatus.State = (LsaAuthProviderState)Enum.Parse(typeof(LsaAuthProviderState), pProviderStatus.status.ToString());

                        GetLSAMgmtTrustedDomainInfo(pProviderStatus, ref authProviderStatus); 

                        agentStatus.AuthProviderList.Add(authProviderStatus);

                        pCur = (IntPtr)((int)pCur + Marshal.SizeOf(pProviderStatus));
                    }
                }
            }
            catch (Exception e)
            {               
                //Logger.ShowUserError(e.Message.ToString());
                Logger.LogException("LSAMgmtStatus.GetLSAMgmtStatus", e);
                agentStatus = null;
            }
            finally
            {
                if (!IntPtr.Zero.Equals(pLsaStatus))
                {
                    LSAMgmtAPI.LWMGMTFreeLsaStatus(pLsaStatus);
                }
            }

            return agentStatus;
        }

        public static void GetLSAMgmtTrustedDomainInfo(LSAMgmtAPI.LSA_AUTH_PROVIDER_STATUS pProviderStatus, ref LsaAuthProviderStatus authProviderStatus)
        {
            authProviderStatus.NetworkCheckInterval = pProviderStatus.dwNetworkCheckInterval;
            authProviderStatus.NumTrustedDomains = pProviderStatus.dwNumTrustedDomains;

            IntPtr pTrustedDomainInfoCur = pProviderStatus.pTrustedDomainInfoArray;

            for (int index = 0; index < authProviderStatus.NumTrustedDomains; index++)
            {
                LsaTrustedDomainInfo lsaTrustedDomainInfo = new LsaTrustedDomainInfo();

                LSAMgmtAPI.LSA_TRUSTED_DOMAIN_INFO pTrustedDomainInfoArray = new LSAMgmtAPI.LSA_TRUSTED_DOMAIN_INFO();

                Marshal.PtrToStructure(pTrustedDomainInfoCur, pTrustedDomainInfoArray);                

                if (pTrustedDomainInfoArray != null)
                {
                    if (!IntPtr.Zero.Equals(pTrustedDomainInfoArray.pszDnsDomain))
                    {
                        lsaTrustedDomainInfo.DnsDomain = Marshal.PtrToStringAuto(pTrustedDomainInfoArray.pszDnsDomain);
                    }

                    if (!IntPtr.Zero.Equals(pTrustedDomainInfoArray.pszNetbiosDomain))
                    {
                        lsaTrustedDomainInfo.NetbiosDomain = Marshal.PtrToStringAuto(pTrustedDomainInfoArray.pszNetbiosDomain);
                    }

                    if (!IntPtr.Zero.Equals(pTrustedDomainInfoArray.pszTrusteeDnsDomain))
                    {
                        lsaTrustedDomainInfo.TrusteeDnsDomain = Marshal.PtrToStringAuto(pTrustedDomainInfoArray.pszTrusteeDnsDomain);
                    }

                    if (!IntPtr.Zero.Equals(pTrustedDomainInfoArray.pszDomainSID))
                    {
                        lsaTrustedDomainInfo.DomainSID = Marshal.PtrToStringAuto(pTrustedDomainInfoArray.pszDomainSID);
                    }

                    if (!IntPtr.Zero.Equals(pTrustedDomainInfoArray.pszDomainGUID))
                    {
                        lsaTrustedDomainInfo.DomainGUID = Marshal.PtrToStringAuto(pTrustedDomainInfoArray.pszDomainGUID);
                    }

                    if (!IntPtr.Zero.Equals(pTrustedDomainInfoArray.pszForestName))
                    {
                        lsaTrustedDomainInfo.ForestName = Marshal.PtrToStringAuto(pTrustedDomainInfoArray.pszForestName);
                    }

                    if (!IntPtr.Zero.Equals(pTrustedDomainInfoArray.pszClientSiteName))
                    {
                        lsaTrustedDomainInfo.ClientSiteName = Marshal.PtrToStringAuto(pTrustedDomainInfoArray.pszClientSiteName);
                    }

                    lsaTrustedDomainInfo.TrustFlags = (LSA_TRUST_FLAG)Enum.Parse(typeof(LSA_TRUST_FLAG), pTrustedDomainInfoArray.dwTrustFlags.ToString());
                    lsaTrustedDomainInfo.TrustType = (LSA_TRUST_TYPE)Enum.Parse(typeof(LSA_TRUST_TYPE), pTrustedDomainInfoArray.dwTrustType.ToString());
                    lsaTrustedDomainInfo.TrustAttributes = (LSA_TRUST_ATTRIBUTE)Enum.Parse(typeof(LSA_TRUST_ATTRIBUTE), pTrustedDomainInfoArray.dwTrustAttributes.ToString());
                    lsaTrustedDomainInfo.DomainFlags = (LSA_DM_DOMAIN_FLAGS)Enum.Parse(typeof(LSA_DM_DOMAIN_FLAGS), pTrustedDomainInfoArray.dwDomainFlags.ToString());

                    if (pTrustedDomainInfoArray.pDCInfo != IntPtr.Zero)
                    {
                        LSAMgmtAPI.LSA_DC_INFO pDCInfo = new LSAMgmtAPI.LSA_DC_INFO();

                        Marshal.PtrToStructure(pTrustedDomainInfoArray.pDCInfo, pDCInfo);

                        lsaTrustedDomainInfo.pDCInfo = new LsaDCInfo();

                        if (!IntPtr.Zero.Equals(pDCInfo.pszName))
                        {
                            lsaTrustedDomainInfo.pDCInfo.Name = Marshal.PtrToStringAuto(pDCInfo.pszName);
                        }

                        if (!IntPtr.Zero.Equals(pDCInfo.pszAddress))
                        {
                            lsaTrustedDomainInfo.pDCInfo.Address = Marshal.PtrToStringAuto(pDCInfo.pszAddress);
                        }

                        if (!IntPtr.Zero.Equals(pDCInfo.pszSiteName))
                        {
                            lsaTrustedDomainInfo.pDCInfo.SiteName = Marshal.PtrToStringAuto(pDCInfo.pszSiteName);
                        }

                        lsaTrustedDomainInfo.pDCInfo.DSflags = (LSA_DS_FLAGS)Enum.Parse(typeof(LSA_DS_FLAGS), pDCInfo.dwFlags.ToString());
                    }

                    if (pTrustedDomainInfoArray.pGCInfo != IntPtr.Zero)
                    {
                        LSAMgmtAPI.LSA_DC_INFO pGCInfo = new LSAMgmtAPI.LSA_DC_INFO();

                        Marshal.PtrToStructure(pTrustedDomainInfoArray.pGCInfo, pGCInfo);

                        lsaTrustedDomainInfo.pGCInfo = new LsaDCInfo();

                        if (!IntPtr.Zero.Equals(pGCInfo.pszName))
                        {
                            lsaTrustedDomainInfo.pGCInfo.Name = Marshal.PtrToStringAuto(pGCInfo.pszName);
                        }

                        if (!IntPtr.Zero.Equals(pGCInfo.pszAddress))
                        {
                            lsaTrustedDomainInfo.pGCInfo.Address = Marshal.PtrToStringAuto(pGCInfo.pszAddress);
                        }

                        if (!IntPtr.Zero.Equals(pGCInfo.pszSiteName))
                        {
                            lsaTrustedDomainInfo.pGCInfo.SiteName = Marshal.PtrToStringAuto(pGCInfo.pszSiteName);
                        }

                        lsaTrustedDomainInfo.pGCInfo.DSflags = (LSA_DS_FLAGS)Enum.Parse(typeof(LSA_DS_FLAGS), pGCInfo.dwFlags.ToString());
                    }

                    authProviderStatus.TrustedDomainInfo.Add(lsaTrustedDomainInfo);
                }

                pTrustedDomainInfoCur = (IntPtr)((int)pTrustedDomainInfoCur + Marshal.SizeOf(pTrustedDomainInfoArray));
            }
        }

        #endregion
    }

    public class LSAKerberosKeyTableInfo
    {

        #region Helper functions

        public static UInt32 GetKeyTabEntriesCount(string hostname, string keyTabPath)
        {
            UInt32 dwCount = 0;

            try
            {

                int ret = (int)LSAMgmtAPI.LWMGMTCountKeyTabEntries(hostname, keyTabPath, out dwCount);
                if (ret != 0)
                {
                    throw new ApplicationException("Failed to get the key tab entries count");
                }

            }
            catch (Exception e)
            {
                Logger.ShowUserError(e.Message.ToString());
                dwCount = 0;
            }

            return dwCount;
        }

        public static KeyTabEntries GetKeyTabEntries(string hostname,
                                                string keyTabPath,
                                                UInt32 lastRecordId,
                                                UInt32 recordsPerPage)
        {
            KeyTabEntries keyTabEntries = null;
            IntPtr pKeyTabEntries = IntPtr.Zero;

            try
            {

                LSAMgmtAPI.LWMGMT_LSA_KEYTAB_ENTRIES KeyTabEntries = new LSAMgmtAPI.LWMGMT_LSA_KEYTAB_ENTRIES();

                int ret = (int)LSAMgmtAPI.LWMGMTReadKeyTab(hostname,
                                                           keyTabPath,
                                                           lastRecordId,
                                                           recordsPerPage,
                                                           out pKeyTabEntries);
                if (ret != 0)
                {
                    throw new ApplicationException("Failed to get the key tab entries information");
                }

                Marshal.PtrToStructure(pKeyTabEntries, KeyTabEntries);

                if (KeyTabEntries.dwCount != 0)
                {
                    keyTabEntries = new KeyTabEntries();
                    keyTabEntries.Count = KeyTabEntries.dwCount;

                    IntPtr pCur = KeyTabEntries.pLsaKeyTabEntryArray;

                    for (int index = 0; index < KeyTabEntries.dwCount; index++)
                    {
                        KerberosKeyTabEntry keytabEntry = new KerberosKeyTabEntry();
                        LSAMgmtAPI.LWMGMT_LSA_KEYTAB_ENTRY pKeytabEntry = new LSAMgmtAPI.LWMGMT_LSA_KEYTAB_ENTRY();

                        Marshal.PtrToStructure(pCur, pKeytabEntry);

                        keytabEntry.EncriptionType = pKeytabEntry.enctype;

                        keytabEntry.keyVersionNumber = pKeytabEntry.kvno;

                        keytabEntry.Timestamp = pKeytabEntry.timestamp;

                        if (!IntPtr.Zero.Equals(pKeytabEntry.pszPrincipal))
                        {
                            keytabEntry.PrincipalName = Marshal.PtrToStringAuto(pKeytabEntry.pszPrincipal);
                        }

                        if (!IntPtr.Zero.Equals(pKeytabEntry.pszPassword))
                        {
                            keytabEntry.Password = Marshal.PtrToStringAuto(pKeytabEntry.pszPassword);
                        }

                        keyTabEntries.KeyTabEntriesList.Add(keytabEntry);

                        pCur = (IntPtr)((int)pCur + Marshal.SizeOf(pKeytabEntry));
                    }
                }
            }
            catch (Exception e)
            {
                Logger.ShowUserError(e.Message.ToString());
                keyTabEntries = null;
            }
            finally
            {
                if (!IntPtr.Zero.Equals(pKeyTabEntries))
                {
                    LSAMgmtAPI.LWMGMTFreeKeyTabEntries(pKeyTabEntries);
                }
            }

            return keyTabEntries;
        }

        #endregion
    }

}
