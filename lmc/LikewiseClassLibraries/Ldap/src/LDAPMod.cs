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
using System.Collections;
using System.Runtime.InteropServices;

namespace Likewise.LMC.LDAP
{
    /// <summary>
    /// Summary description for LDAPMod.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    public struct LDAPModInner
    {
        #region variables

        public int mod_op; //operationType (add, mod, ...)
        public string mod_type; //attributeName
        public IntPtr attrValues; // A pointer to a block of unmanaged memory containing an array of pointers

        #endregion
    }

    public class LDAPMod
    {
        #region variables

        IntPtr pMods = IntPtr.Zero;
        IntPtr[] strings = null;

        public enum mod_ops
        {
            LDAP_MOD_ADD = 0x00,
            LDAP_MOD_DELETE = 0x01,
            LDAP_MOD_REPLACE = 0x02
        };

        public LDAPModInner ldapmod;

        //Pointer to a null-terminated array of null-terminated strings.
        //The last element of the array must be a NULL pointer.
        public string[] modv_strvals;
        //Pointer to a NULL terminated array of berval pointers.
        //The last element of the array must be a NULL pointer.
        public ArrayList modv_bvals;

        #endregion

        #region Constructors

        //initialize ldapmod
        public LDAPMod()
        {
            ldapmod = new LDAPModInner();
            ldapmod.mod_op = 0;
            ldapmod.mod_type = null;
            ldapmod.attrValues = IntPtr.Zero;
            modv_strvals = null;
        }

        public LDAPMod(int mod_op, string mod_type, string[] modv_strvals)
        {
            ldapmod = new LDAPModInner();
            ldapmod.mod_op = mod_op; //Add, Mod, Replace
            ldapmod.mod_type = mod_type; //attributeNames
            ldapmod.attrValues = IntPtr.Zero;
            this.modv_strvals = modv_strvals; //attributeValues
        }

        #endregion

        #region Public methods

        public IntPtr ConvertToUM()
        {
            int sizeOfIntPtr;
            int counter;

            pMods = IntPtr.Zero;
            strings = null;

            strings = new IntPtr[modv_strvals.Length];
            // Convert the strings into unmanaged pointers
            for (counter = 0; counter < modv_strvals.Length - 1; counter++)
            //    strings[counter] = Marshal.StringToCoTaskMemUni(modv_strvals[counter]);
                strings[counter] = Marshal.StringToCoTaskMemAuto(modv_strvals[counter]);

            strings[counter] = IntPtr.Zero; // Null terminator

            sizeOfIntPtr = Marshal.SizeOf(typeof(IntPtr));
            // Allocate memory to store the array of pointers
            ldapmod.attrValues = Marshal.AllocCoTaskMem(sizeOfIntPtr * strings.Length);

            // Stuff the pointers into the array
            for (counter = 0; counter < strings.Length; counter++)
                Marshal.WriteIntPtr(ldapmod.attrValues, counter * sizeOfIntPtr, strings[counter]);

            // Allocate unmanaged memory to hold the LDAPMod struct
          //  pMods = Marshal.AllocCoTaskMem(Marshal.SizeOf(ldapmod));
            pMods = Marshal.AllocHGlobal(Marshal.SizeOf(ldapmod));

            // Stuff the struct into pMod
            Marshal.StructureToPtr(ldapmod, pMods, false);

            return pMods;

            //return IntPtr.Zero;
        }

        public void ldapfree()
        {
            if (ldapmod.attrValues != IntPtr.Zero)
                Marshal.FreeCoTaskMem(ldapmod.attrValues);
            if (strings != null)
            {
                for (int counter = 0; counter < strings.Length; counter++)
                {
                    if (strings[counter] != IntPtr.Zero)
                        Marshal.FreeCoTaskMem(strings[counter]);
                }
            }
            Marshal.FreeCoTaskMem(pMods);
        }

        #endregion

    }
}
