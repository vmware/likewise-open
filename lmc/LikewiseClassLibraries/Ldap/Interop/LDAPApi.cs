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

namespace Likewise.LMC.LDAP.Interop
{
    public class LdapAPI
    {
#if DEBUG
        private const string LDAP_DLL_PATH = "libldap.dll";
#else
        private const string LDAP_DLL_PATH = "wldap32.dll";
#endif

        public static int morePagedResults = 1;
        public static int pageSize = 500;
        public static int entriesLeft = 0;
        public static Berval page_cookie = new Berval(0,IntPtr.Zero);
        public static IntPtr page_cookiePtr = page_cookie.ConvertToUM();

        public enum LDAPSCOPE
        {
            BASE      = 0x00,
            ONE_LEVEL = 0x01,
            SUB_TREE  = 0x02
        }


        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_set_option(IntPtr ld,
                                                 int option,
                                                 //const void *optdata );
                                                 IntPtr optdata);

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_get_option(IntPtr ld,
                                                 int option,    
                                                //const void *optdata );
                                                 IntPtr optdata);


        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_open(
                                        string HostName,
                                        int PortNumber
                                        );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_connect(
                                        IntPtr ld,
                                        IntPtr timeout
                                        );

        /*LDAP_F( int ) in cancel.c
        ldap_cancel LDAP_P(( LDAP *ld,
	    int cancelid,
	    LDAPControl		**sctrls,
	    LDAPControl		**cctrls,
	    int				*msgidp ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_cancel(
                                        IntPtr ld,
                                        int cancelid,
                                        IntPtr[] ServerControls,
                                        IntPtr[] ClientControls,
                                        out int msgidp
                                        );
          
        /*LDAP_F( int ) in cancel.c
        ldap_cancel_s LDAP_P(( LDAP *ld,
	    int msgidp,
	    LDAPControl		**sctrls,
	    LDAPControl		**cctrls));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_cancel_s(
                                        IntPtr ld,
                                        int cancelid,
                                        IntPtr[] ServerControls,
                                        IntPtr[] ClientControls                                       
                                        );   

        /*in abandon.c: 
        LDAP_F( int )
        ldap_abandon_ext LDAP_P((
        LDAP			*ld,
	    int				msgid,
	    LDAPControl		**serverctrls,
	    LDAPControl		**clientctrls ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_abandon_ext(
                                        IntPtr ld,
                                        int msgidp,
                                        IntPtr[] ServerControls,
                                        IntPtr[] ClientControls                                       
                                        );

        /*ldap_abandon LDAP_P((	/* deprecated, use ldap_abandon_ext 
	    LDAP *ld,
	    int msgid ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_abandon(
                                        IntPtr ld,                                        
                                        int msgidp
                                        );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_bind(
            IntPtr ld,
            string dn,
            string cred,
            int method
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_bind_s(
                                    IntPtr ld,
                                    string dn,
                                    string cred,
                                    ulong method
                                    );


        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_get_values(
            IntPtr ld, IntPtr entry, string attr);

        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_get_values_len(
            IntPtr ld, IntPtr entry, string attr);

        /*
        PCHAR ldap_first_attribute(
         __in          LDAP* ld,
         __in          LDAPMessage* entry,
         __out         BerElement** ptr);
        */
        [DllImport(LDAP_DLL_PATH)]
        public static extern string ldap_first_attribute(
                                        IntPtr ld,
                                        IntPtr entry,
                                        IntPtr berValStarStar
                                        );

        /*
         PCHAR ldap_next_attribute(
          __in          LDAP* ld,
          __in          LDAPMessage* entry,
          __in_out      BerElement* ptr);
         */
        [DllImport(LDAP_DLL_PATH)]
        public static extern string ldap_next_attribute(
                                        IntPtr ld,
                                        IntPtr entry,
                                        IntPtr berValStar
                                        );

        /*(LDAP_F( int )
        ldap_msgfree LDAP_P((
        LDAPMessage *lm ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_msgfree(
            IntPtr ldapMessage
            );

        /*LDAP_F( int )
        ldap_count_entries LDAP_P((
	    LDAP *ld,
	    LDAPMessage *chain ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_count_entries(
            IntPtr ld,
            IntPtr ldapMessage
            );

        /*
        *in getentry.c: 
        LDAP_F( LDAPMessage * )
        ldap_first_entry LDAP_P((
        LDAP *ld,
        LDAPMessage *chain ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_first_entry(
            IntPtr ld, IntPtr ldapMsg);

        /*LDAP_F( LDAPMessage * )
        ldap_next_entry LDAP_P((
	    LDAP *ld,
	    LDAPMessage *entry ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_next_entry(
            IntPtr ld, IntPtr entry);


        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_first_message(
            IntPtr ld, IntPtr ldapMsg);

        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_next_message(
            IntPtr ld, IntPtr entry);

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_msgtype(
            IntPtr ldapMsg);

        [DllImport(LDAP_DLL_PATH)]
        public static extern string ldap_err2string(
            int error);

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_simple_bind(
            IntPtr ld,
            string dn,
            string password
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern string ldap_get_dn(
            IntPtr ld, IntPtr entry);

        [DllImport(LDAP_DLL_PATH)]
        /* public static extern int ldap_count_values(
             IntPtr ld,
             IntPtr ldapEntry
             );*/
        public static extern int ldap_count_values(
            IntPtr charStarStar
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_count_values_len(
            IntPtr vals);

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_simple_bind_s(
            IntPtr ld,
            string dn,
            string password
            );

        /*LDAP_F( int )
        ldap_unbind_s LDAP_P(( /* deprecated, use ldap_unbind_ext_s 
	    LDAP *ld ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_unbind_s(
            IntPtr ld
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_search_st(
            IntPtr ld,
            string basedn,
            int scope,
            string filter,
            string[] attrs,
            int attrsonly,
            IntPtr timeout,
            out IntPtr res
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_search_s(
            IntPtr ld,
            string basedn,
            int scope,
            string filter,
            string[] attrs,
            int attrsonly,
            out IntPtr umldapMessage
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_search_ext(
            IntPtr ld,
            string basedn,
            int scope,
            string filter,
            string[] attrs,
            int attrsonly,
            IntPtr[] ServerControls,
            IntPtr[] ClientControls,
            int TimeLimit,
            int SizeLimit,
            out IntPtr ldapMessage
            );

        /*int
        ldap_search_ext_s(
        LDAP *ld,
        LDAP_CONST char *base,
        int scope,
        LDAP_CONST char *filter,
        char **attrs,
        int attrsonly,
        LDAPControl **sctrls,
        LDAPControl **cctrls,
        struct timeval *timeout,
        int sizelimit,
        LDAPMessage **res )  */
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_search_ext_s(
            IntPtr ld,
            string basedn,
            int scope,
            string filter,
            string[] attrs,
            int attrsonly,
            IntPtr[] ServerControls,
            IntPtr[] ClientControls,
            IntPtr timeout,
            int SizeLimit,
            out IntPtr ldapMessage
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_add_s(
            IntPtr ld,
            string basedn,
            IntPtr[] attrs
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_add(
            IntPtr ld,
            string basedn,
            IntPtr[] attrs
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_add_ext(
            IntPtr ld,
            string dn,
            IntPtr[] attrs,
            IntPtr[] ServerControls,
            IntPtr[] ClientControls,
            out int MessageNumber
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_add_ext_s(
            IntPtr ld,
            string dn,
            IntPtr[] attrs,
            IntPtr[] umServerControls,
            IntPtr[] umClientControls,
            out IntPtr umldapMessage
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_modify_s(
            IntPtr ld,
            string basedn,
            IntPtr[] attrs
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_modify(
            IntPtr ld,
            string basedn,
            IntPtr[] attrs
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_delete_s(
            IntPtr ld,
            string dn
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_delete(
            IntPtr ld,
            string basedn,
            IntPtr[] attrs
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_rename_s(
            IntPtr ld,
            string basedn,
            string newdn,
            string newSuperior,
            int deleteoldrdn,
            IntPtr[] ServerControls,
            IntPtr[] ClientControls
            );

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_rename(
            IntPtr ld,
            string basedn,
            string newdn,
            string newSuperior,
            int deleteoldrdn,
            IntPtr[] ServerControls,
            IntPtr[] ClientControls,
            IntPtr msgId
            );

        /*[DllImport(LDAP_DLL_PATH)]
       public static extern string ldap_err2string(
                                       int err
                                       );*/


        /*
         * in error.c: 
         LDAP_F( int )
         ldap_parse_result LDAP_P((
         LDAP			*ld,
         LDAPMessage		*res,
         int				*errcodep,
         char			**matcheddnp,
         char			**errmsgp,
         char			***referralsp,
         LDAPControl		***serverctrls,
         int				freeit ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_parse_result(
             IntPtr ld,
             IntPtr ldapMsgRes,
             out int errcodep,
             IntPtr matcheddnp,
             IntPtr errmsgp,
             IntPtr referralsp,
             out IntPtr serverLdapControl,
             int freeit);


        /*ldap_parse_page_control LDAP_P((
        /* deprecated, use ldap_parse_pageresponse_control 
        LDAP *ld,
        LDAPControl **ctrls,
        ber_int_t *count,
        struct berval **cookie ));*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_parse_page_control(
            IntPtr ld,
            IntPtr ctrls,
            out ulong countp,
            out IntPtr cookie);

        [DllImport(LDAP_DLL_PATH)]
        public static extern void ldap_control_free(
            IntPtr ctrl);

        /**
         * LDAPControl * ldap_find_control( LDAP_CONST char *void,
         * LDAPControl ** ctrls)
         * *********************************/
        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ldap_find_control(
            string ldap_control_val,
            IntPtr ctrls);


        /*BerElement * ber_init (struct berval *bv)*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ber_init(
            out IntPtr bv);        


        /*ber_tag_t ber_scanf(
         * BerElement *ber, 
         * LDAP_CONST char *fmt, 
         * ber_int_t* entriesLeft, 
         * struct berVal *cookie)***/
        [DllImport(LDAP_DLL_PATH)]
        public static extern Int32 ber_scanf(
            IntPtr ber,
            string fmt,
            out IntPtr entriesLeft,
            out IntPtr cookie);


        /*void ber_free(
         * BerElement *ber,
         * int freeBuf))*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern void ber_free(
              IntPtr ber,
              Int32 freeBuf);

        /*void ldap_controls_free(
         * LDAPControl **ctrls)*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern void ldap_controls_free(
            IntPtr ctrls);

        /*
        *int ldap_create_page_control(
        LDAP          *ld,
        unsigned long pageSize,
        struct berval *cookie,
        const char    isCritical,
        LDAPControl   **control)
        This API is not available in openldap release, need to implement by the client side*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_create_page_control(
            IntPtr ld,
            int pageSize,
            IntPtr cookie,
            char isCritical,
            out IntPtr control);

        /*int ldap_create_control(
         * char *requestOID,
         * BerElement *ber,
         * int iscritical,
         * LDAPControl **ctrlp);
         * ******************/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ldap_create_control(
            string requestOID,
            IntPtr ber,
            int iscritical,
            out IntPtr ctrlp);

        /*BerElement *ber_alloc_t(
         *    int beroptions);
         * ***************/
        [DllImport(LDAP_DLL_PATH)]
        public static extern IntPtr ber_alloc_t(
            int beroptions);

        [DllImport(LDAP_DLL_PATH)]
        public static extern int ber_printf(
            IntPtr ber,
            string fmt,
            ulong pagesize,
            IntPtr cookie);
           // string str,
          // int num);
            //params object[] argsRest);

        /*int ber_flatten(BerElement *ber, struct berval **bvPtr)*/
        [DllImport(LDAP_DLL_PATH)]
        public static extern int ber_flatten(
            IntPtr berPtr,                                         
            IntPtr bvPtrStarStar);  

    }
}
