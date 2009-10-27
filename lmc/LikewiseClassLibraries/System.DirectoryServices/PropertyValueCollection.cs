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
using Likewise.LMC.LDAP;

namespace System.DirectoryServices
{
    public class PropertyValueCollection: List<object>
    {
        private List<object> valueCollection;
        private bool modified = false;

        public bool Modified
        {
            get
            {
                return modified;
            }
        }

        public List<object> ValueCollection
        {
            get
            {
                return valueCollection;
            }
        }

        public object Value
        {
            get
            {
                if (valueCollection == null) return null;                
                else if (valueCollection.Count == 0) return null;
                else if (valueCollection.Count == 1) return valueCollection[0];
                else
                {
                    object[] valueObjs = new object[valueCollection.Count];
                    valueCollection.CopyTo(valueObjs);
                    return valueObjs as object;
                }
            }
            set
            {   
                modified = true;

                if (value == null)
                {
                    valueCollection = null; //value can be null
                    return;
                }

                if ((value is string) || (value is int))
                {
                    valueCollection = new List<object>();
                    valueCollection.Add(value as object);
                    return;
                }

                if (value is object[])
                {
                    object[] values = value as object[];
                    valueCollection = new List<object>();
                    if (values != null && values.Length > 0)
                        foreach (object val in values)                 
                            valueCollection.Add(val);
                    return;
                }           
            }
        }

        public PropertyValueCollection()
        {
            modified = false;
            valueCollection = null;
        }

        public PropertyValueCollection(LdapValue[] values)
        {
            if (values == null)
                valueCollection = null;

            if (values != null && values.Length > 0)
            {
                valueCollection = new List<object>();

                foreach (LdapValue ldapvalue in values)
                {
                    if (ldapvalue.AdsType == ADSType.ADSTYPE_OCTET_STRING
                       || ldapvalue.AdsType == ADSType.ADSTYPE_NT_SECURITY_DESCRIPTOR)
                        valueCollection.Add(ldapvalue.byteData as object);
                    else if (ldapvalue.AdsType == ADSType.ADSTYPE_LARGE_INTEGER)
                        valueCollection.Add(ldapvalue.longData as object);
                    else if (ldapvalue.AdsType == ADSType.ADSTYPE_INTEGER)
                        valueCollection.Add(ldapvalue.intData as object);
                    else if (ldapvalue.stringData.Equals("TRUE", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bool val = true;
                        valueCollection.Add(val as object);                        
                    }
                    else if (ldapvalue.stringData.Equals("FALSE", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bool val = false;
                        valueCollection.Add(val as object);
                    }
                    else valueCollection.Add(ldapvalue.stringData as object);                    
                }
            }
        }

        public new bool Contains(object o)
        {
            //private List<object> valueCollection;
            if (valueCollection == null || valueCollection.Count == 0)
                return false;

            foreach (object obj in valueCollection)
            {
                if (obj.GetType() == o.GetType())
                {
                    Type t = obj.GetType();
                    
                    if (obj is string)
                    {
                        string obj_str = obj as string;
                        string o_str = o as string;
                        if (obj_str.Equals(o_str,StringComparison.InvariantCultureIgnoreCase))
                            return true;
                        else continue;
                    }
                    else if (obj is byte[])
                    {
                        byte[] a = obj as byte[];
                        byte[] b = o as byte[];
                        if (a.Length == b.Length)
                        {
                            for (int i = 0; i < a.Length; i++)
                                if (a[i] != b[i]) continue;
                            return true;
                        }
                        else continue;
                    }
                    else 
                    {
                        if (t == typeof(int))
                            if ((int)obj == (int)o) return true;
                            else continue;
                        if (t == typeof(long))
                            if ((long)obj == (long)o) return true;
                            else continue;
                        if (t == typeof(bool))
                            if ((bool)obj == (bool)o) return true;
                            else continue;
                    }                       
                }                   
            }

            return false;
        }  

        public new void Add(object o)
        {
          //  Console.WriteLine("add an object " + o.ToString());
            
            if (valueCollection == null)
            {
                valueCollection = new List<object>();           
            }                                                             

            valueCollection.Add(o);

            modified = true;
            
        }

        public new int Count
        {
            get
            {
                if (valueCollection == null || (valueCollection != null && valueCollection.Count == 0))
                    return 0;
                else return valueCollection.Count;
            }    
              
        }
    }
}
