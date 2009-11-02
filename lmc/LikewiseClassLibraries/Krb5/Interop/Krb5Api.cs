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
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Krb5
{
    public enum Krb5_flags : uint
    {
        //#define KRB5_GC_CACHED (1U << 0)
        KRB5_GC_CACHED = 1,

        //#define KRB5_GC_USER_USER (1U << 1) 
        KRB5_GC_USER_USER = 2
    }

    public class Krb5Utils
    {
        private static DateTime _dt1970 = new DateTime(1970, 1, 1, 0, 0, 0, 0);

        public static DateTime Time_T2DateTime(uint time_t)
        {
            // Should we convert to local time?
            return _dt1970.AddSeconds(Convert.ToDouble(time_t));
        }
    }

    public class PrivateKrb5Api
    {
        private const string Krb5_DLL_PATH = "libktkrb5.dll";

        //12 hours in seconds
        public static double ExpiryGraceSeconds = (12 * 60 * 60);
        public static string ADMIN_CACHE_DIR = "/var/lib/likewise/lac";
        public static string ADMIN_TEMP_CACHE_DIR = "/tmp";
        public static string KRB5_CACHEPATH = "krb5_cc_lac";

        [DllImport(Krb5_DLL_PATH)]
        public static extern UInt32 krb5_init_context(out IntPtr ctx);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_initialize(IntPtr ctx,
                             IntPtr cc,
                             IntPtr client_principal);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_default_name(IntPtr ctx);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_store_cred(IntPtr ctx,
                             IntPtr cc,
                             KRB5API.krb5_creds creds);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_resolve(IntPtr ctx,
                             [MarshalAs(UnmanagedType.LPWStr)] string pszCachePath,
                             ref IntPtr cc);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_default(IntPtr ctx,
                             ref IntPtr cc);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_close(IntPtr ctx,
                             IntPtr cc);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_destroy(IntPtr ctx,
                             IntPtr cc);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_free_context(IntPtr ctx);


        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_free_creds(IntPtr ctx, IntPtr creds);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_free_principal(IntPtr ctx,
                             IntPtr client_principal);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_cc_get_principal(IntPtr ctx,
                             IntPtr cc,
                             out IntPtr client_principal);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_unparse_name(IntPtr ctx,
                             IntPtr principal,                            
                             out string principal_name);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_free_cred_contents(IntPtr ctx,
                             ref IntPtr creds);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_parse_name(IntPtr ctx,
                             [MarshalAs(UnmanagedType.LPWStr)] string szUserName,
                          out KRB5API.krb5_principal client_principal);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_copy_principal(IntPtr ctx,
                             IntPtr client_principal,
                             [MarshalAs(UnmanagedType.LPWStr)]
                             out string in_creds);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_kt_default(IntPtr ctx,
                             out IntPtr keytab);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_kt_close(IntPtr ctx,
                             IntPtr keytab);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_get_init_creds_keytab(IntPtr ctx,
                             ref KRB5API.krb5_creds creds,
                             KRB5API.krb5_principal client_principal,
                             IntPtr keytab,
                             int start_time,
                             [MarshalAs(UnmanagedType.LPWStr)] string in_tkt_service,
                             IntPtr options);

        [DllImport(Krb5_DLL_PATH)]
        public static extern IntPtr krb5_get_credentials(IntPtr ctx,
                             int options,
                             IntPtr cc,
                             ref IntPtr inCreds,
                             out IntPtr outCreds);
    }

    public class KRB5API
    {
        #region struct definitions
        //struct krb5_creds {
        //    krb5_principal      client;
        //    krb5_principal      server;
        //    krb5_keyblock       session;
        //    krb5_times          times;
        //    krb5_data           ticket;
        //    krb5_data           second_ticket;
        //    krb5_authdata       authdata;
        //    krb5_addresses      addresses;
        //    krb5_ticket_flags   flags;
        //} krb5_creds
        //[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        //public class krb5_creds
        //{
        //    public IntPtr client;           
        //    public IntPtr server;
        //    public IntPtr session;
        //    public krb5_times times;
        //    public IntPtr ticket;
        //    public IntPtr second_ticket;
        //    public IntPtr authdata;
        //    public IntPtr addresses;
        //    public IntPtr flags;
        //}

        //typedef struct krb5_times 
        //{ 
        //   krb5_timestamp authtime;
        //   krb5_timestamp starttime; 
        //   krb5_timestamp endtime; 
        //   krb5_timestamp renew_till; 
        //}
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_times
        {
            public UInt32 authtime;
            public UInt32 starttime;
            public UInt32 endtime;
            public UInt32 renew_till;
        }

        //typedef struct time_t krb5_times; <definition from #include <time.h>

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_context
        {
            public UInt32 magic;
            public IntPtr in_tkt_ktypes;
            public UInt32 in_tkt_ktype_count;
            public IntPtr tgs_ktypes;
            public UInt32 tgs_ktype_count;
            public krb5_os_context os_context;
            public IntPtr default_realm;
            public IntPtr profile;
            public IntPtr db_context;
            public UInt32 ser_ctx_count;
            public IntPtr ser_ctx;
            public UInt32 clockskew;
            public UInt32 kdc_req_sumtype;
            public UInt32 default_ap_req_sumtype;
            public UInt32 default_safe_sumtype;
            public UInt32 kdc_default_options;
            public UInt32 library_options;
            public UInt32 profile_secure;
            public UInt32 fcc_default_format;
            public UInt32 scc_default_format;
            public IntPtr prompt_types;
            public UInt32 udp_pref_limit;
            public UInt32 use_conf_ktypes;
            public UInt32 profile_in_memory;
            public krb5_plugins libkrb5_plugins;
            public IntPtr vtbl;
            public IntPtr locate_fptrs;
            public krb5_plugins preauth_plugins;
            public IntPtr preauth_context;
            public krb5_error_code err;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_os_context
        {
            public UInt32 magic;
            public UInt32 time_offset;
            public UInt32 usec_offset;
            public UInt32 os_flags;
            public IntPtr default_ccname;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_plugins
        {
            public IntPtr files;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_error_code
        {
            public UInt32 code;
            public IntPtr msg;
            public IntPtr scratch_buf;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_ccache
        {
            public UInt32 magic;
            public IntPtr ops;
            public IntPtr data;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_creds
        {
            public UInt32 magic;
            public krb5_principal client;
            public krb5_principal server;
            public krb5_keyblock keyblock;
            public krb5_times times;
            public UInt32 is_skey;
            public UInt32 ticket_flags;
            public IntPtr addresses;
            public krb5_ticket ticket;
            public krb5_ticket second_ticket;
            public IntPtr authdata;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_keyblock
        {
            public UInt32 magic;
            public UInt32 enctype;
            public UInt32 length;
            public IntPtr contents;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_ticket
        {
            public UInt32 magic;
            public UInt32 length;
            public IntPtr data;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_keytab
        {
            public UInt32 magic;
            public IntPtr ops;
            public IntPtr data;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public class krb5_principal
        {
            public UInt32 magic;
            public krb5_ticket realm;
            public IntPtr data;
            public UInt32 length;
            public UInt32 type;
        }

        #endregion
    }
}
