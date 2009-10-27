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

using System.Collections.Generic;


namespace Likewise.LMC.LDAP
{
    public enum ADSType
    {
        ADSTYPE_INVALID	= 0,
	    ADSTYPE_DN_STRING,
	    ADSTYPE_CASE_EXACT_STRING,
	    ADSTYPE_CASE_IGNORE_STRING,
	    ADSTYPE_PRINTABLE_STRING,
	    ADSTYPE_NUMERIC_STRING,
	    ADSTYPE_BOOLEAN,
	    ADSTYPE_INTEGER,
	    ADSTYPE_OCTET_STRING,
	    ADSTYPE_UTC_TIME,
	    ADSTYPE_LARGE_INTEGER,
	    ADSTYPE_PROV_SPECIFIC,
	    ADSTYPE_OBJECT_CLASS,
	    ADSTYPE_CASEIGNORE_LIST,
	    ADSTYPE_OCTET_LIST,
	    ADSTYPE_PATH,
	    ADSTYPE_POSTALADDRESS,
	    ADSTYPE_TIMESTAMP,
	    ADSTYPE_BACKLINK,
	    ADSTYPE_TYPEDNAME,
	    ADSTYPE_HOLD,
	    ADSTYPE_NETADDRESS,
	    ADSTYPE_REPLICAPOINTER,
	    ADSTYPE_FAXNUMBER,
	    ADSTYPE_EMAIL,
	    ADSTYPE_NT_SECURITY_DESCRIPTOR,
	    ADSTYPE_UNKNOWN,
	    ADSTYPE_DN_WITH_BINARY,
	    ADSTYPE_DN_WITH_STRING,
        ADSTYPE_DERIVED
    }
   

    /// <summary>
    /// Summary description for ADType.
    /// </summary>
    public class ADTypeLookup
    {
        #region Class data
        private static readonly Dictionary<string, ADSType> _adtypeLookup;
        #endregion

        #region Constructor
        static ADTypeLookup()
        {
            _adtypeLookup = new Dictionary<string, ADSType>();

            _adtypeLookup.Add("2.5.5.1", ADSType.ADSTYPE_DN_STRING);
            _adtypeLookup.Add("2.5.5.2", ADSType.ADSTYPE_CASE_IGNORE_STRING);
            _adtypeLookup.Add("2.5.5.3", ADSType.ADSTYPE_CASE_EXACT_STRING);
            _adtypeLookup.Add("2.5.5.4", ADSType.ADSTYPE_CASE_IGNORE_STRING);
            _adtypeLookup.Add("2.5.5.5", ADSType.ADSTYPE_PRINTABLE_STRING);
            _adtypeLookup.Add("2.5.5.6", ADSType.ADSTYPE_NUMERIC_STRING);
            _adtypeLookup.Add("2.5.5.7", ADSType.ADSTYPE_DN_WITH_STRING);
            _adtypeLookup.Add("2.5.5.8", ADSType.ADSTYPE_BOOLEAN);
            _adtypeLookup.Add("2.5.5.9", ADSType.ADSTYPE_INTEGER);
            _adtypeLookup.Add("2.5.5.10", ADSType.ADSTYPE_OCTET_STRING);
            _adtypeLookup.Add("2.5.5.11", ADSType.ADSTYPE_UTC_TIME);
            _adtypeLookup.Add("2.5.5.12", ADSType.ADSTYPE_CASE_IGNORE_STRING);//Unicode String
            _adtypeLookup.Add("2.5.5.13",ADSType.ADSTYPE_CASE_IGNORE_STRING);
            _adtypeLookup.Add("2.5.5.14",ADSType.ADSTYPE_DN_WITH_STRING);
            _adtypeLookup.Add("2.5.5.15",ADSType.ADSTYPE_NT_SECURITY_DESCRIPTOR);
            _adtypeLookup.Add("2.5.5.16",ADSType.ADSTYPE_LARGE_INTEGER);
            _adtypeLookup.Add("2.5.5.17", ADSType.ADSTYPE_OCTET_STRING);
        }
        #endregion

        #region Public method
        public static ADSType GetADSType(string syntaxID)
        {
            return (_adtypeLookup.ContainsKey(syntaxID) ? _adtypeLookup[syntaxID] : ADSType.ADSTYPE_UNKNOWN);
        }
        #endregion
    }
}
