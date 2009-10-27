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
using System.Text;
using System.Runtime.InteropServices;
using Likewise.LMC.Netlogon;
using Likewise.LMC.Netlogon.Implementation;

namespace Likewise.LMC.Netlogon
{
    public class CNetlogon
    {

        #region definitions
        //putting these in a separate struct makes autocomplete work better.
        public struct Definitions
        {

            /* ERRORS */
            public const uint LWNET_ERROR_SUCCESS = 0x0000;
            public const uint LWNET_ERROR_INVALID_CACHE_PATH = 0xA000; // 40960
            public const uint LWNET_ERROR_INVALID_CONFIG_PATH = 0xA001; // 40961
            public const uint LWNET_ERROR_INVALID_PREFIX_PATH = 0xA002; // 40962
            public const uint LWNET_ERROR_INSUFFICIENT_BUFFER = 0xA003; // 40963
            public const uint LWNET_ERROR_OUT_OF_MEMORY = 0xA004; // 40964
            public const uint LWNET_ERROR_INVALID_MESSAGE = 0xA005; // 40965
            public const uint LWNET_ERROR_UNEXPECTED_MESSAGE = 0xA006; // 40966
            public const uint LWNET_ERROR_DATA_ERROR = 0xA007; // 40967
            public const uint LWNET_ERROR_NOT_IMPLEMENTED = 0xA008; // 40968
            public const uint LWNET_ERROR_NO_CONTEXT_ITEM = 0xA009; // 40969
            public const uint LWNET_ERROR_REGEX_COMPILE_FAILED = 0xA00A; // 40970
            public const uint LWNET_ERROR_INTERNAL = 0xA00B; // 40971
            public const uint LWNET_ERROR_INVALID_DNS_RESPONSE = 0xA00C; // 40972
            public const uint LWNET_ERROR_DNS_RESOLUTION_FAILED = 0xA00D; // 40973
            public const uint LWNET_ERROR_FAILED_TIME_CONVERSION = 0xA00E; // 40974
            public const uint LWNET_ERROR_INVALID_SID = 0xA00F; // 40975
            public const uint LWNET_ERROR_UNEXPECTED_DB_RESULT = 0xA010; // 40976
            public const uint LWNET_ERROR_INVALID_LWNET_CONNECTION = 0xA011; // 40977
            public const uint LWNET_ERROR_INVALID_PARAMETER = 0xA012; // 40978
            public const uint LWNET_ERROR_LDAP_NO_PARENT_DN = 0xA013; // 40979
            public const uint LWNET_ERROR_LDAP_ERROR = 0xA014; // 40980
            public const uint LWNET_ERROR_NO_SUCH_DOMAIN = 0xA015; // 40981
            public const uint LWNET_ERROR_LDAP_FAILED_GETDN = 0xA016; // 40982
            public const uint LWNET_ERROR_DUPLICATE_DOMAINNAME = 0xA017; // 40983
            public const uint LWNET_ERROR_FAILED_FIND_DC = 0xA018; // 40984
            public const uint LWNET_ERROR_LDAP_GET_DN_FAILED = 0xA019; // 40985
            public const uint LWNET_ERROR_INVALID_SID_REVISION = 0xA01A; // 40986
            public const uint LWNET_ERROR_LOAD_LIBRARY_FAILED = 0xA01B; // 40987
            public const uint LWNET_ERROR_LOOKUP_SYMBOL_FAILED = 0xA01C; // 40988
            public const uint LWNET_ERROR_INVALID_EVENTLOG = 0xA01D; // 40989
            public const uint LWNET_ERROR_INVALID_CONFIG = 0xA01E; // 40990
            public const uint LWNET_ERROR_UNEXPECTED_TOKEN = 0xA01F; // 40991
            public const uint LWNET_ERROR_LDAP_NO_RECORDS_FOUND = 0xA020; // 40992
            public const uint LWNET_ERROR_STRING_CONV_FAILED = 0xA021; // 40993
            public const uint LWNET_ERROR_QUERY_CREATION_FAILED = 0xA022; // 40994
            public const uint LWNET_ERROR_NOT_JOINED_TO_AD = 0xA023; // 40995
            public const uint LWNET_ERROR_FAILED_TO_SET_TIME = 0xA024; // 40996
            public const uint LWNET_ERROR_NO_NETBIOS_NAME = 0xA025; // 40997
            public const uint LWNET_ERROR_INVALID_NETLOGON_RESPONSE = 0xA026; // 40998
            public const uint LWNET_ERROR_INVALID_OBJECTGUID = 0xA027; // 40999
            public const uint LWNET_ERROR_INVALID_DOMAIN = 0xA028; // 41000
            public const uint LWNET_ERROR_NO_DEFAULT_REALM = 0xA029; // 41001
            public const uint LWNET_ERROR_NOT_SUPPORTED = 0xA02A; // 41002
            public const uint LWNET_ERROR_NO_LWNET_INFORMATION = 0xA02B; // 41003
            public const uint LWNET_ERROR_NO_HANDLER = 0xA02C; // 41004
            public const uint LWNET_ERROR_NO_MATCHING_CACHE_ENTRY = 0xA02D; // 41005
            public const uint LWNET_ERROR_KRB5_CONF_FILE_OPEN_FAILED = 0xA02E; // 41006
            public const uint LWNET_ERROR_KRB5_CONF_FILE_WRITE_FAILED = 0xA02F; // 41007
            public const uint LWNET_ERROR_DOMAIN_NOT_FOUND = 0xA030; // 41008
            public const uint LWNET_ERROR_SENTINEL = 0xA031; // 41009

            //only used in .NET DLL
            public const uint LWNET_ERROR_UNSUPPORTED_PLATFORM = 0xAFFE;   //45055
            public const uint LWNET_ERROR_INTEROP_EXCEPTION = 0xAFFF;    //45056

            //
            // Note: When you add errors here, please remember to update
            //       the corresponding error strings in lwnet-error.c
            //
            public const uint LWNET_ERROR_PREFIX = 0xA000; // 40960

            public const string LWNET_KRB5_CONF_DIRNAME = "/var/lib/likewise";
            public const string LWNET_KRB5_CONF_BASENAME = "krb5-affinity.conf";
            public const string LWNET_KRB5_CONF_PATH = LWNET_KRB5_CONF_DIRNAME + "/" + LWNET_KRB5_CONF_BASENAME;

            //Standard GUID's are 16 bytes long.
            public const uint LWNET_GUID_SIZE = 16;

            //used in LWNET_DC_INFO::ulDomainControllerAddressType
            public const uint DS_INET_ADDRESS = 23;
            public const uint DS_NETBIOS_ADDRESS = 24;

            //used in LWNET_DC_INFO::Flags
            public const uint DS_PDC_FLAG = 0x00000001; //DC is a PDC of a domain
            public const uint DS_BIT1_RESERVED_FLAG = 0x00000002; //reserved: should always be 0
            public const uint DS_GC_FLAG = 0x00000004; //DC contains GC of a forest
            public const uint DS_LDAP_FLAG = 0x00000008; //DC supports an LDAP server
            public const uint DS_DS_FLAG = 0x00000010; //DC supports a DS
            public const uint DS_KDC_FLAG = 0x00000020; //DC is running a KDC
            public const uint DS_TIMESERV_FLAG = 0x00000040; //DC is running the time service
            public const uint DS_CLOSEST_FLAG = 0x00000080; //DC is the closest one to the client.
            public const uint DS_WRITEABLE_FLAG = 0x00000100; //DC has a writable DS
            public const uint DS_GOOD_TIMESERV_FLAG = 0x00000200; //DC is running time service and has clock hardware 
            public const uint DS_NDNC_FLAG = 0x00000400; //Non-Domain NC
            public const uint DS_PING_FLAGS = 0x0000FFFF; //bitmask of flags returned on ping

            public const uint DS_DNS_CONTROLLER_FLAG = 0x20000000; //DomainControllerName is a DNS name
            public const uint DS_DOMAIN_FLAG = 0x40000000; //DomainName is a DNS name
            public const uint DS_DNS_FOREST_FLAG = 0x80000000; //DnsForestName is a DNS name

            public const uint LWNET_SUPPORTED_DS_OUTPUT_FLAGS = (
                                DS_PDC_FLAG |
                                DS_GC_FLAG |
                                DS_DS_FLAG |
                                DS_KDC_FLAG |
                                DS_TIMESERV_FLAG |
                                DS_CLOSEST_FLAG |
                                DS_WRITEABLE_FLAG |
                                DS_GOOD_TIMESERV_FLAG);

            public const uint LWNET_UNSUPPORTED_DS_OUTPUT_FLAGS = (
                                DS_NDNC_FLAG |
                                DS_DNS_CONTROLLER_FLAG |
                                DS_DOMAIN_FLAG |
                                DS_DNS_FOREST_FLAG);


            //used in DsGetDcName 'Flags' input parameter
            public const uint DS_FORCE_REDISCOVERY = 0x00000001;

            public const uint DS_DIRECTORY_SERVICE_REQUIRED = 0x00000010;
            public const uint DS_DIRECTORY_SERVICE_PREFERRED = 0x00000020;
            public const uint DS_GC_SERVER_REQUIRED = 0x00000040;
            public const uint DS_PDC_REQUIRED = 0x00000080;
            public const uint DS_BACKGROUND_ONLY = 0x00000100;
            public const uint DS_IP_REQUIRED = 0x00000200;
            public const uint DS_KDC_REQUIRED = 0x00000400;
            public const uint DS_TIMESERV_REQUIRED = 0x00000800;
            public const uint DS_WRITABLE_REQUIRED = 0x00001000;
            public const uint DS_GOOD_TIMESERV_REQUIRED = 0x00002000;
            public const uint DS_AVOID_SELF = 0x00004000;
            public const uint DS_ONLY_LDAP_NEEDED = 0x00008000;

            public const uint DS_IS_FLAT_NAME = 0x00010000;
            public const uint DS_IS_DNS_NAME = 0x00020000;

            public const uint DS_RETURN_DNS_NAME = 0x40000000;
            public const uint DS_RETURN_FLAT_NAME = 0x80000000;

            public const uint LWNET_SUPPORTED_DS_INPUT_FLAGS = (
                                DS_FORCE_REDISCOVERY |
                                DS_DIRECTORY_SERVICE_REQUIRED |
                                DS_GC_SERVER_REQUIRED |
                                DS_PDC_REQUIRED |
                                DS_BACKGROUND_ONLY |
                                DS_KDC_REQUIRED |
                                DS_TIMESERV_REQUIRED |
                                DS_WRITABLE_REQUIRED |
                                DS_GOOD_TIMESERV_REQUIRED |
                                DS_AVOID_SELF);



            public const uint LWNET_UNSUPPORTED_DS_INPUT_FLAGS = (
                                DS_DIRECTORY_SERVICE_PREFERRED |
                                DS_IP_REQUIRED |
                                DS_ONLY_LDAP_NEEDED |
                                DS_IS_FLAT_NAME |
                                DS_IS_DNS_NAME |
                                DS_RETURN_DNS_NAME |
                                DS_RETURN_FLAT_NAME);
        }
        #endregion

        #region structures
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct LWNET_DC_INFO
        {
            public uint pingTime;
            public uint DomainControllerAddressType;
            public uint Flags;
            public uint Version;
            public byte LMToken;
            public byte NTToken;
            [MarshalAs(UnmanagedType.LPStr)]
            public string DomainControllerName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string DomainControllerAddress;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = (int)Definitions.LWNET_GUID_SIZE)]
            public byte[] pucDomainGUID;
            [MarshalAs(UnmanagedType.LPStr)]
            public string NetBIOSDomainName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string FullyQualifiedDomainName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string DnsForestName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string DCSiteName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string ClientSiteName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string NetBIOSHostName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string UserName;
        }




        #endregion

        #region data

        private static INetlogon _netlogonImplementation = null;

        #endregion

        #region constructors

        static CNetlogon()
        {
            if (System.Environment.OSVersion.Platform == PlatformID.Unix)
            {
                _netlogonImplementation = new UnixNetlogon() as INetlogon;
            }
            else
            {
                _netlogonImplementation = new WindowsNetlogon() as INetlogon;
            }        
        }

        #endregion

        #region public members

        public static uint GetDCName(
            string domainFQDN,
            uint flags,
            out LWNET_DC_INFO DCInfo
            )
        {
            return _netlogonImplementation.GetDCName(domainFQDN, flags, out DCInfo);
        }

        public static uint GetDomainController(
            string domainFQDN,
            out string domainControllerFQDN
            )
        {
            return _netlogonImplementation.GetDomainController(domainFQDN, out domainControllerFQDN);
        }

        public static uint GetDCTime(
            string domainFQDN,
            out DateTime DCTime
            )
        {
            return _netlogonImplementation.GetDCTime(domainFQDN, out DCTime);
        }

        public static uint GetCurrentDomain(
            out string domainFQDN
            )
        {
            return _netlogonImplementation.GetCurrentDomain(out domainFQDN);
        }


        #endregion

        #region helper functions

        public static string LWNETDCInfoToString(LWNET_DC_INFO DCInfo)
        {

            if (DCInfo.pucDomainGUID == null)
            {
                return null;
            }

            int i = 0;

            string[] domainGUIDStrings = new string[Definitions.LWNET_GUID_SIZE];
            string domainGUIDString = null;
            string sFlags = GetFlagString(DCInfo.Flags);

            StringBuilder sb = new StringBuilder();

            for (i = 0; i < Definitions.LWNET_GUID_SIZE; i++)
            {
                domainGUIDStrings[i] = String.Format("{0:x2} ", DCInfo.pucDomainGUID[i]);
            }

            domainGUIDString = String.Concat(domainGUIDStrings);

            sb.Append("pingTime:");
            sb.Append(DCInfo.pingTime);
            sb.Append(";");

            sb.Append("DomainControllerAddressType:");
            sb.Append(DCInfo.DomainControllerAddressType);
            sb.Append(";");

            sb.Append("Flags(hex unsigned integer):");
            sb.Append(String.Format("0x{0:x}\n",DCInfo.Flags));
            sb.Append(";");

            sb.Append("Flags(string):");
            sb.Append(sFlags);
            sb.Append(";");

            sb.Append("Version:");
            sb.Append(DCInfo.Version);
            sb.Append(";");

            sb.Append("LMToken:");
            sb.Append(DCInfo.LMToken);
            sb.Append(";");

            sb.Append("NTToken:");
            sb.Append(DCInfo.NTToken);
            sb.Append(";");

            sb.Append("DomainControllerName:");
            sb.Append(DCInfo.DomainControllerName);
            sb.Append(";");

            sb.Append("DomainControllerAddress:");
            sb.Append(DCInfo.DomainControllerAddress);
            sb.Append(";");

            sb.Append("domainGUID:");
            sb.Append(domainGUIDString);
            sb.Append(";");

            sb.Append("NetBIOSDomainName:");
            sb.Append(DCInfo.NetBIOSDomainName);
            sb.Append(";");

            sb.Append("FullyQualifiedDomainName:");
            sb.Append(DCInfo.FullyQualifiedDomainName);
            sb.Append(";");

            sb.Append("DnsForestName:");
            sb.Append(DCInfo.DnsForestName);
            sb.Append(";");

            sb.Append("DCSiteName:");
            sb.Append(DCInfo.DCSiteName);
            sb.Append(";");

            sb.Append("ClientSiteName:");
            sb.Append(DCInfo.ClientSiteName);
            sb.Append(";");

            sb.Append("NetBIOSHostName:");
            sb.Append(DCInfo.NetBIOSHostName);
            sb.Append(";");

            sb.Append("UserName:");
            sb.Append(DCInfo.UserName);
            sb.Append(";");

            return sb.ToString();

        }

        public static string GetFlagString(uint Flags)
        {

            StringBuilder sb = new StringBuilder();

            if(((uint) Definitions.DS_PDC_FLAG & Flags) > 0)  
            {
                sb.Append("DS_PDC_FLAG,");
            }

            if(((uint) Definitions.DS_BIT1_RESERVED_FLAG & Flags) > 0)  
            {
                sb.Append("DS_BIT1_RESERVED_FLAG,");
            }

            if(((uint) Definitions.DS_GC_FLAG & Flags) > 0)  
            {
                sb.Append("DS_GC_FLAG,");
            }

            if(((uint) Definitions.DS_LDAP_FLAG & Flags) > 0)  
            {
                sb.Append("DS_LDAP_FLAG,");
            }

            if(((uint) Definitions.DS_DS_FLAG & Flags) > 0)  
            {
                sb.Append("DS_DS_FLAG,");
            }

            if(((uint) Definitions.DS_KDC_FLAG & Flags) > 0) 
            {
                sb.Append("DS_KDC_FLAG,");
            }

            if(((uint) Definitions.DS_TIMESERV_FLAG & Flags) > 0)  
            {
                sb.Append("DS_TIMESERV_FLAG,");
            }

            if(((uint) Definitions.DS_CLOSEST_FLAG & Flags) > 0)  
            {
                sb.Append("DS_CLOSEST_FLAG,");
            }

            if(((uint) Definitions.DS_WRITEABLE_FLAG & Flags) > 0)  
            {
                sb.Append("DS_WRITEABLE_FLAG,");
            }

            if(((uint) Definitions.DS_GOOD_TIMESERV_FLAG & Flags) > 0)  
            {
                sb.Append("DS_GOOD_TIMESERV_FLAG,");
            }

            if(((uint) Definitions.DS_NDNC_FLAG & Flags) > 0)  
            {
                sb.Append("DS_NDNC_FLAG,");
            }

            if(((uint) Definitions.DS_PING_FLAGS & Flags) > 0)  
            {
                sb.Append("DS_PING_FLAGS,");
            }

            if(((uint) Definitions.DS_DNS_CONTROLLER_FLAG & Flags) > 0) 
            {
                sb.Append("DS_DNS_CONTROLLER_FLAG,");
            }

            if(((uint) Definitions.DS_DOMAIN_FLAG & Flags) > 0)  
            {
                sb.Append("DS_DOMAIN_FLAG,");
            }

            if(((uint) Definitions.DS_DNS_FOREST_FLAG & Flags) > 0)  
            {
                sb.Append("DS_DNS_FOREST_FLAG,");
            }

            return sb.ToString();
        }

        #endregion
    }
}
