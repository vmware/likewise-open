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

namespace Likewise.LMC.LSAMgmt
{
    public class KerberosKeyTabEntry
    {
        private UInt32 timestamp;
        private UInt32 keyversionNo;
        private UInt32 enctype;
        private string pszPrincipal;
        private string pszPassword;

        public KerberosKeyTabEntry()
        {
            timestamp = 0;
            keyversionNo = 0;
            enctype = 0;
        }

        public UInt32 Timestamp
        {
            get
            {
                return timestamp;
            }
            set
            {
                timestamp = value;
            }
        }

        public UInt32 keyVersionNumber
        {
            get
            {
                return keyversionNo;
            }
            set
            {
                keyversionNo = value;
            }
        }

        public UInt32 EncriptionType
        {
            get
            {
                return enctype;
            }
            set
            {
                enctype = value;
            }
        }

        public string PrincipalName
        {
            get
            {
                return pszPrincipal;
            }
            set
            {
                pszPrincipal = value;
            }
        }

        public string Password
        {
            get
            {
                return pszPassword;
            }
            set
            {
                pszPassword = value;
            }
        }
    }

    public class KeyTabEntries
    {
        private UInt32 dwCount;
        private List<KerberosKeyTabEntry> _ketTabentries;

        public KeyTabEntries()
        {
            dwCount = 0;
            _ketTabentries = new List<KerberosKeyTabEntry>();
        }

        public UInt32 Count
        {
            get
            {
                return dwCount;
            }
            set
            {
                dwCount = value;
            }
        }

        public List<KerberosKeyTabEntry> KeyTabEntriesList
        {
            get
            {
                return _ketTabentries;
            }
            set
            {
                _ketTabentries = value;
            }
        }
    }
}
