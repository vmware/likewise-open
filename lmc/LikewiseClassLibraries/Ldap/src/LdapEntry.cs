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

namespace Likewise.LMC.LDAP
{
    /// <summary>
    /// Summary description for LdapEntry.
    /// </summary>
    public class LdapEntry
    {
        #region variables

        private LdapMessage _ldapMessage;
        private IntPtr _ldapEntry;

        #endregion

        #region Constructor

        public LdapEntry(LdapMessage ldapMessage, IntPtr ldapEntry)
        {
            _ldapEntry = ldapEntry;
            _ldapMessage = ldapMessage;
        }

        #endregion

        #region public methods

        //ldap_get_values returns char**; this must be manually marshaled
        public LdapValue[] GetAttributeValues(string attrName, DirectoryContext dirContext)
        {
            return _ldapMessage.GetAttributeValues(_ldapEntry, attrName, dirContext);
        }

        public string GetDN()
        {
            return _ldapMessage.GetDN(_ldapEntry);
        }

        public string[] GetAttributeNames()
        {
            return _ldapMessage.GetAttributeNames(_ldapEntry);
        }

        #endregion
    }
}

