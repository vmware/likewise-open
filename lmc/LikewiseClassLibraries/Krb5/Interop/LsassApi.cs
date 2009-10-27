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
using System.Text;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Likewise.LMC.LMConsoleUtils;

namespace Likewise.LMC.Krb5
{
    public class PrivateLsassApi
    {
        private const string LSASS_DLL_PATH = "liblsakrb5.so";   

        //12 hours in seconds
        public static double KRB5_DEFAULT_TKT_LIFE = (12 * 60 * 60);
        public static string ADMIN_CACHE_DIR = "/var/lib/likewise/lac";
        public static string KRB5_CACHEPATH = "krb5_cc_lac";

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5GetServiceTicketForUser(
                                 int uid,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszUserPrincipal,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszServername,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszDomain,
                                 [MarshalAs(UnmanagedType.LPWStr)] string cacheType
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5GetTgs(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszCliPrincipal,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszPassword,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszCcPath                                 
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5GetTgt(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszCliPrincipal,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszPassword,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszCcPath,
                                 out int pdwGoodUntilTime
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5RealmTransitionOffline(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszRealm
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5RealmIsOffline(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszRealm
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5CopyFromUserCache(
                                 IntPtr ctx,
                                 IntPtr destCC,
                                 int uid
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5MoveCCacheToUserPath(
                                 IntPtr ctx,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszNewCacheName,
                                 int uid,
                                 int gid
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5GetDefaultRealm(
                                 [MarshalAs(UnmanagedType.LPWStr)]
                                 out string pszRealm
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5GetSystemCachePath(
                                 int cacheType,
                                 [MarshalAs(UnmanagedType.LPWStr)] out string ppszCachePath                                
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5GetUserCachePath(
                                 int uid,
                                 int cacheType,
                                 [MarshalAs(UnmanagedType.LPWStr)] out string ppszCachePath
                                 );
        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5SetDefaultCachePath(                                 
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszCachePath,
                                 [MarshalAs(UnmanagedType.LPWStr)] out string ppszOriginalCachePath
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaKrb5SetProcessDefaultCachePath(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszCachePath                                 
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaSetupMachineSession(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszMachname,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszSamAccountName,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszPassword,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszRealm,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszDomain,
                                 int pdwGoodUntilTime
                                 );

        [DllImport(LSASS_DLL_PATH)]
        public static extern int LsaAllocateString(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszUserPrincipal,
                                 [MarshalAs(UnmanagedType.LPWStr)] out string pszUPN
                                 );

    }

    public class PrivateLwioApi
    {
        private const string LWIO_DLL_PATH = "liblwioclient.so";

        [DllImport(LWIO_DLL_PATH)]
        public static extern int LwIoCreateKrb5AccessTokenA(
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszPrincipalName,
                                 [MarshalAs(UnmanagedType.LPWStr)] string pszCachePath,
                                 out IntPtr pAccessToken
                                 );
    }

    public enum Krb5CacheType
    {
        KRB5_InMemory_Cache = 0,
        KRB5_File_Cache
    }

    public struct CRED_CONTEXT
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        public string pszPrincipalName;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string pszCachePath;
        public bool bDestroyCachePath;
        public IntPtr pAccessToken;
    }
}
