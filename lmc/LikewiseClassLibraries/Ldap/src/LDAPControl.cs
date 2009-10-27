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

namespace Likewise.LMC.LDAP
{
	/// <summary>rol
	/// Summary description for LDAPControl.
	/// </summary>
    
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]        
	public struct LDAPControlInner
    {
        #region Variables

        public string oid;       
        //public bool isCritical;
        public char ldctl_iscritical;
        public IntPtr ptrberVal;
       // public Berval berVal;

        #endregion
    }

    public class LDAPControl
    {
        #region Class data

        public LDAPControlInner ldapControl;

        #endregion

        #region Constructors

        public LDAPControl(LDAPControlInner ldapControl)
        {
            this.ldapControl = ldapControl;
        }        

        public LDAPControl(Berval berVal, string oid, char isCritical)
        {
            ldapControl.oid = oid;
            ldapControl.ldctl_iscritical = isCritical;
            ldapControl.ptrberVal = IntPtr.Zero;
          //  ldapControl.berVal = berVal;

            ldapControl.ptrberVal = Marshal.AllocHGlobal(Marshal.SizeOf(berVal));
            Marshal.StructureToPtr(berVal, ldapControl.ptrberVal, false);
        }

        #endregion

        #region Public methods

        public IntPtr ConvertToUM()
        {
            IntPtr ptrC = IntPtr.Zero;
            ptrC = Marshal.AllocHGlobal(Marshal.SizeOf(ldapControl));
            Marshal.StructureToPtr(ldapControl, ptrC, false);            
            
            return ptrC;
        }

        #endregion
    }
}

#region Commented Code
        /*  
        public LDAPControl(LDAPControlInner ldapControl)
        {

            this.ldapControl = ldapControl;
            
            if (ldapControl.ptrberVal != IntPtr.Zero)
            {
                this._value = new Berval();
                this._value = (Berval)Marshal.PtrToStructure(ldapControl.ptrberVal, typeof(Berval));
            }            
        }			 

	    public Berval Value
	    {
	        get
	        {
	            return _value;
	        }
            set
            {
                _value = value;
            }
	    }


		public IntPtr ConvertToUM()
		{          
            
            return IntPtr.Zero;
		} 
        */
#endregion