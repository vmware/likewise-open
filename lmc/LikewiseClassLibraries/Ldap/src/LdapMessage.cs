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
using Likewise.LMC.LDAP.Interop;
using System.Threading;

namespace Likewise.LMC.LDAP
{
	/// <summary>
	/// Summary description for LdapMessage.
	/// </summary>
	///
    public class LdapMessage
    {

        #region data

        public static string LDAP_CONTROL_PAGEDRESULTS = "1.2.840.113556.1.4.319";  /*RFC 2696*/
        public static Int32 LBER_ERROR = (Int32) (- 1);
        private const int LDAP_RES_SEARCH_ENTRY = 100; //(Int32)0x64U;
        private const int LDAP_RES_SEARCH_RESULT = 101;// (Int32)0x65U;

        private IntPtr     _ldapMessage;
		private LdapHandle _ldapHandle;
	    private DirectoryContext _dircontext;

        private System.Object Ldap_ApiCallSync = new System.Object();

        #endregion

        #region constructors

        public LdapMessage(LdapHandle ldapHandle, IntPtr ldapMessage)
		{
			_ldapHandle = ldapHandle;
			_ldapMessage = ldapMessage;
		    _dircontext = null;
        }

        #endregion

        #region public methods

        public string GetDN(IntPtr ldapEntry)
        {
            return LdapAPI.ldap_get_dn(_ldapHandle.Handle, ldapEntry);
        }

        public string[] GetAttributeNames(IntPtr ldapEntry)
        {
            List<string> attrs = new List<string>();

            IntPtr pBerval = new IntPtr(0);

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    if (_ldapHandle != null && _ldapHandle.Handle != IntPtr.Zero)
                    {
                        IntPtr ppBerval = Marshal.AllocHGlobal(IntPtr.Size);

                        Marshal.StructureToPtr(pBerval, ppBerval, false);

                        Logger.Log(string.Format(
                            "In GetAttributeNames calling ldap_first_attribute(_ldapHandle{0},ldapEntry{1},ppBerval {2})",
                            _ldapHandle, ldapEntry, ppBerval), Logger.ldapTracingLevel);

                        string strTmp = LdapAPI.ldap_first_attribute(_ldapHandle.Handle, ldapEntry, ppBerval);

                        Logger.Log(string.Format("Result of ldap_first_attribute(strTmp)", strTmp), Logger.ldapTracingLevel);

                        string attrName = new string(strTmp.ToCharArray());

                        if (!IntPtr.Zero.Equals(ppBerval))
                        {
                            pBerval = Marshal.ReadIntPtr(ppBerval);

                            if (!IntPtr.Zero.Equals(pBerval))
                            {
                                while (!string.IsNullOrEmpty(attrName))
                                {
                                    Logger.Log(string.Format(
                                        "In GetAttributeNames calling ldap_next_attribute(_ldapHandle{0},ldapEntry{1},berValStarStar{2})",
                                        _ldapHandle, ldapEntry, ppBerval), Logger.ldapTracingLevel);

                                    strTmp = LdapAPI.ldap_next_attribute(_ldapHandle.Handle, ldapEntry, pBerval);

                                    Logger.Log(string.Format("Result of ldap_next_attribute(strTmp)", strTmp), Logger.ldapTracingLevel);

                                    if (string.IsNullOrEmpty(strTmp))
                                    {
                                        attrName = null;
                                    }
                                    else
                                    {
                                        attrName = new string(strTmp.ToCharArray());
                                        attrs.Add(attrName);
                                    }
                                }
                                LdapAPI.ber_free(pBerval, 0);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapMessage.GetAttributeNames", ex);
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
            return attrs.ToArray();
        }

        // C# to convert a string to a byte array.
        public static byte[] StrToByteArray(string str)
        {
            System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
            return encoding.GetBytes(str);
        }

        public LdapValue[] GetBervals(IntPtr ldapEntry, string attrName)
        {
            Logger.Log(string.Format("Calling GetBervals(ldapEntry{0},attrName{1})", ldapEntry, attrName), Logger.ldapTracingLevel);
             Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    if (_ldapHandle != null && _ldapHandle.Handle != IntPtr.Zero)
                    {
                        IntPtr ppBervals = LdapAPI.ldap_get_values_len(_ldapHandle.Handle, ldapEntry, attrName);

                        if (IntPtr.Zero.Equals(ppBervals))
                        {
                            return null;
                        }

                        int berCount =
                            LdapAPI.ldap_count_values_len(LdapAPI.ldap_get_values_len(_ldapHandle.Handle, ldapEntry, attrName));
                        Logger.Log(String.Format("Result of ldap_get_values_len(berCount)", berCount), Logger.ldapTracingLevel);

                        if (berCount == 0)
                        {
                            return null;
                        }

                        LdapValue[] ldapValues = new LdapValue[berCount];
                        for (int i = 0; i < berCount; i++)
                        {
                            IntPtr pBerval = Marshal.ReadIntPtr(ppBervals, i * IntPtr.Size);
                            if (IntPtr.Zero.Equals(pBerval))
                            {
                                ldapValues[i] = null;
                            }
                            else
                            {
                                Berval tmpBer = (Berval)Marshal.PtrToStructure(pBerval, typeof(Berval));

                                if (tmpBer.bv_len == 0)
                                {
                                    ldapValues[i] = null;
                                }
                                else
                                {
                                    byte[] tmpBytes = new byte[tmpBer.bv_len];

                                    for (int j = 0; j < (int)tmpBer.bv_len; j++)
                                    {
                                        tmpBytes[j] = Marshal.ReadByte(tmpBer.bv_val, j);
                                    }
                                    ldapValues[i] = new LdapValue(ADSType.ADSTYPE_DN_STRING, tmpBytes);

                                    ldapValues[i].stringData = BitConverter.ToString(tmpBytes);
                                }
                            }
                        }
                        return ldapValues;
                    }
                    else
                        return null;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapMessage.GetAttributeNames", ex);
                return null;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public LdapValue[] GetNonBervals(IntPtr ldapEntry, string attrName)
        {
            Logger.Log(string.Format("Calling GetNonBervals(ldapEntry{0},attrName{1})",
                ldapEntry, attrName), Logger.ldapTracingLevel);

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    if (_ldapHandle != null && _ldapHandle.Handle != IntPtr.Zero)
                    {
                        IntPtr ppValues = LdapAPI.ldap_get_values(_ldapHandle.Handle, ldapEntry, attrName);

                        if (IntPtr.Zero.Equals(ppValues))
                        {
                            Logger.Log(String.Format(
                                "GetNonBervals({0}): values = null", attrName),
                                Logger.ldapLogLevel);
                            return null;
                        }

                        int valueCount = LdapAPI.ldap_count_values(ppValues);

                        if (valueCount == 0)
                        {
                            Logger.Log(String.Format(
                                "GetNonBervals({0}): values.Count == 0", attrName),
                                Logger.ldapLogLevel);
                            return null;
                        }
                        string[] resultStr = new string[valueCount];
                        LdapValue[] ldapValues = new LdapValue[valueCount];

                        for (int i = 0; i < valueCount; i++)
                        {
                            IntPtr pValue = Marshal.ReadIntPtr(ppValues, i * IntPtr.Size);
                            if (IntPtr.Zero.Equals(pValue))
                            {
                                resultStr[i] = null;
                            }
                            else
                            {
                                string tmpString = Marshal.PtrToStringAnsi(pValue);

                                if (String.IsNullOrEmpty(tmpString))
                                {
                                    resultStr[i] = null;
                                }
                                else
                                {
                                    resultStr[i] = new string(tmpString.ToCharArray());
                                }
                            }
                            ldapValues[i] = new LdapValue(ADSType.ADSTYPE_DN_STRING, resultStr[i]);
                        }
                        return ldapValues;
                    }
                    else
                        return null;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapMessage.GetNonBervals", ex);
                return null;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }


        public LdapValue[] GetAttributeValues(IntPtr ldapEntry, string attrName, DirectoryContext dirContext)
        {

            if(dirContext == null ||
                String.IsNullOrEmpty(attrName))
            {
                return null;
            }

            if (dirContext.SchemaCache == null)
            {
                return GetNonBervals(ldapEntry, attrName);
            }

            LDAPSchemaCache schema = dirContext.SchemaCache;
            SchemaType foundType = schema.GetSchemaTypeByDisplayName(attrName);

            // if cannot find type in schemaCache, get byte[] first
            //(sometimes schemaCache doesn't contain objectGUID,...type information)
            if (foundType == null)
            {
                if (String.Equals(attrName, "objectGUID", StringComparison.InvariantCultureIgnoreCase) ||
                    String.Equals(attrName, "objectSid", StringComparison.InvariantCultureIgnoreCase) ||
                    String.Equals(attrName, "nTsecurityDescriptor", StringComparison.InvariantCultureIgnoreCase))
                {
                    return ReformatBervalsInHex(ldapEntry, attrName);
                }
                else
                {
                    return GetNonBervals(ldapEntry, attrName);
                }
            }
            ADSType adstype = foundType.DataType;

            if (adstype == ADSType.ADSTYPE_OCTET_STRING ||
                adstype == ADSType.ADSTYPE_NT_SECURITY_DESCRIPTOR)
            {
                return ReformatBervalsInHex(ldapEntry, attrName);
            }
            else if (adstype == ADSType.ADSTYPE_INTEGER ||
                    adstype == ADSType.ADSTYPE_LARGE_INTEGER)
            {
                LdapValue[] values = GetBervals(ldapEntry, attrName);

                if (values == null || values.Length == 0)
                {
                    return values;
                }
                foreach (LdapValue value in values)
                {

                    string replace = value.stringData.Replace("-", "");
                    char[] intValuechrs = new char[replace.Length / 2];

                    for (int i = 0; i < replace.Length / 2; i++)
                    {
                        intValuechrs[i] = replace[i * 2 + 1];
                    }

                    for (int i = 0; i < intValuechrs.Length; i++)
                    {
                        if (intValuechrs[i] < '0' || intValuechrs[i] > '9')
                        {
                            intValuechrs[i] = '-';
                        }
                    }
                    value.stringData = new string(intValuechrs);
                    value.stringData.Replace("-", "");

                    if (adstype == ADSType.ADSTYPE_INTEGER)
                    {
                        value.intData = Convert.ToInt32(value.stringData);
                        value.AdsType = ADSType.ADSTYPE_INTEGER;
                    }
                    else if (adstype == ADSType.ADSTYPE_LARGE_INTEGER)
                    {
                        value.longData = Convert.ToInt64(value.stringData);
                        value.AdsType = ADSType.ADSTYPE_LARGE_INTEGER;
                    }
                }
            }
            return GetNonBervals(ldapEntry, attrName);

        }

	    public void FreeMessage()
		{
            Logger.Log(string.Format("Calling FreeMessage()"), Logger.ldapTracingLevel);
            try
            {
                Logger.Log(String.Format("FreeMessage :{0}"), Logger.ldapTracingLevel);
                int ret = LdapAPI.ldap_msgfree(_ldapMessage);
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapTracingLevel);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapMessage.FreeMessage", ex);
            }
		}

		public int Ldap_Count_Entries()
		{
            //return (LdapAPI.ldap_count_entries(_ldapHandle.Handle, _ldapMessage));
            Logger.Log(string.Format(
                "Calling Ldap_Count_Entries"),
                Logger.ldapTracingLevel);
            int ret = -1;
            try
            {
                Logger.Log(string.Format(
                    "Calling ldap_count_entries(_ldapHandle{0},_ldapMessage{1})",
                    _ldapHandle, _ldapMessage), Logger.ldapTracingLevel);

                if (_ldapHandle != null && _ldapHandle.Handle != IntPtr.Zero)
                {
                    ret = LdapAPI.ldap_count_entries(_ldapHandle.Handle, _ldapMessage);

                    if (ret == 0)
                    {
                        Logger.Log(string.Format(
                            "Calling ldap_count_entries :ret={0})", ret), Logger.ldapTracingLevel);
                    }
                }
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapMessage.Ldap_Count_Entries", ex);
                return ret;
            }
		}

        public List<LdapEntry> Ldap_Get_Entries()
        {
            List<LdapEntry> entries = new List<LdapEntry>();

            Logger.Log(string.Format("Calling Ldap_Get_Entries()"), Logger.ldapTracingLevel);
            try
            {
                Logger.Log(string.Format(
                    "Calling ldap_first_entry(_ldapHandle{0},_ldapMessage{1}",
                    _ldapHandle, _ldapMessage), Logger.ldapTracingLevel);

                if (_ldapHandle != null && _ldapHandle.Handle != IntPtr.Zero)
                {
                    IntPtr currentEntry = LdapAPI.ldap_first_entry(_ldapHandle.Handle, _ldapMessage);

                    Logger.Log(String.Format(
                        "Result of ldap_first_entry():{0}",
                        currentEntry), Logger.ldapTracingLevel);

                    if (currentEntry == IntPtr.Zero)
                    {
                        return entries;
                    }
                    do
                    {
                        entries.Add(new LdapEntry(this, currentEntry));

                        Logger.Log(string.Format(
                            "Calling ldap_next_entry(_ldapHandle{0},_ldapMessage{1}",
                            _ldapHandle, _ldapMessage),
                            Logger.ldapTracingLevel);

                        currentEntry = LdapAPI.ldap_next_entry(_ldapHandle.Handle, currentEntry);

                        Logger.Log(String.Format(
                            "Result of ldap_next_entry():{0}",
                            currentEntry), Logger.ldapTracingLevel);

                    } while (currentEntry != IntPtr.Zero);
                }

                return entries;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapMessage.Ldap_Get_Entries", ex);
                return null;
            }
        }

        #endregion

        #region helper methods

        private LdapValue[] ReformatBervalsInHex(IntPtr ldapEntry, string attrName)
        {
            LdapValue[] hexValues = GetBervals(ldapEntry, attrName);
            if (hexValues != null && hexValues.Length > 0)
            {
                int i = 0;
                foreach (LdapValue value in hexValues)
                {
                    string replace = value.stringData.Replace("-", " 0x");
                    replace = string.Concat("0x", replace);
                    value.stringData = replace;
                    value.AdsType = ADSType.ADSTYPE_OCTET_STRING;

                    Logger.Log(String.Format("LdapMessage.ReformatBervalsInHex: hexValues[{0}]={1}", i++, value.stringData),
                        Logger.LogLevel.DebugTracing);
                }
            }
            return hexValues;
        }
        #endregion

        #region accessors

        public DirectoryContext DirContext
        {
            get
            {
                return _dircontext;
            }
            set
            {
                _dircontext = value;
            }
        }

        public IntPtr MessagePtr
        {
            get
            {
                return _ldapMessage;
            }
        }

        #endregion

    }

}
