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
    public enum LSA_TRUST_FLAG
    {
        LSA_TRUST_FLAG_IN_FOREST = 0x00000001,
        LSA_TRUST_FLAG_OUTBOUND = 0x00000002,
        LSA_TRUST_FLAG_TREEROOT = 0x00000004,
        LSA_TRUST_FLAG_PRIMARY = 0x00000008,
        LSA_TRUST_FLAG_NATIVE = 0x000000010,
        LSA_TRUST_FLAG_INBOUND = 0x000000020
    }

    public enum LSA_TRUST_TYPE
    {
        LSA_TRUST_TYPE_DOWNLEVEL = 0x00000001,
        LSA_TRUST_TYPE_UPLEVEL = 0x00000002,
        LSA_TRUST_TYPE_MIT = 0x00000003,
        LSA_TRUST_TYPE_DCE = 0x00000004
    }

    public enum LSA_TRUST_ATTRIBUTE
    {
        LSA_TRUST_ATTRIBUTE_NON_TRANSITIVE = 0x00000001,
        LSA_TRUST_ATTRIBUTE_UPLEVEL_ONLY = 0x00000002,
        LSA_TRUST_ATTRIBUTE_FILTER_SIDS = 0x00000004,
        LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE = 0x00000008,
        LSA_TRUST_ATTRIBUTE_CROSS_ORGANIZATION = 0x00000010,
        LSA_TRUST_ATTRIBUTE_WITHIN_FOREST = 0x00000020
    }

    public enum LSA_DM_DOMAIN_FLAGS
    {
        LSA_DM_DOMAIN_FLAG_PRIMARY = 0x00000001,
        LSA_DM_DOMAIN_FLAG_OFFLINE = 0x00000002,
        LSA_DM_DOMAIN_FLAG_FORCE_OFFLINE = 0x00000004,
        LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD = 0x00000008,
        LSA_DM_DOMAIN_FLAG_FOREST_ROOT = 0x00000010
    }

    public enum LSA_DS_FLAGS : uint
    {
        LSA_DS_DNS_CONTROLLER_FLAG = 0x20000000,
        LSA_DS_DNS_DOMAIN_FLAG = 0x40000000,
        LSA_DS_DNS_FOREST_FLAG = 0x80000000,
        LSA_DS_DS_FLAG = 0x00000010,
        LSA_DS_GC_FLAG = 0x00000004,
        LSA_DS_KDC_FLAG = 0x00000020,
        LSA_DS_PDC_FLAG = 0x00000001,
        LSA_DS_TIMESERV_FLAG = 0x00000040,
        LSA_DS_WRITABLE_FLAG = 0x00000100
    }

    public class LsaTrustedDomainInfo
    {
        private string _dnsDomain;
        private string _netbiosDomain;
        private string _trusteeDnsDomain;
        private string _domainSID;
        private string _domainGUID;
        private string _forestName;
        private string _clientSiteName;

        private LSA_TRUST_FLAG _trustFlags;
        private LSA_TRUST_TYPE _trustType;
        private LSA_TRUST_ATTRIBUTE _trustAttributes;
        private LSA_DM_DOMAIN_FLAGS _domainFlags;
        public LsaDCInfo pDCInfo = null;
        public LsaDCInfo pGCInfo = null;

        public string DnsDomain
        {
            get
            {
                return _dnsDomain;
            }
            set
            {
                _dnsDomain = value;
            }
        }

        public string NetbiosDomain
        {
            get
            {
                return _netbiosDomain;
            }
            set
            {
                _netbiosDomain = value;
            }
        }

        public string TrusteeDnsDomain
        {
            get
            {
                return _trusteeDnsDomain;
            }
            set
            {
                _trusteeDnsDomain = value;
            }
        }

        public string DomainSID
        {
            get
            {
                return _domainSID;
            }
            set
            {
                _domainSID = value;
            }
        }

        public string DomainGUID
        {
            get
            {
                return _domainGUID;
            }
            set
            {
                _domainGUID = value;
            }
        }

        public string ForestName
        {
            get
            {
                return _forestName;
            }
            set
            {
                _forestName = value;
            }
        }

        public string ClientSiteName
        {
            get
            {
                return _clientSiteName;
            }
            set
            {
                _clientSiteName = value;
            }
        }

        public LSA_TRUST_FLAG TrustFlags
        {
            get
            {
                return _trustFlags;
            }
            set
            {
                _trustFlags = value;
            }
        }

        public LSA_TRUST_TYPE TrustType
        {
            get
            {
                return _trustType;
            }
            set
            {
                _trustType = value;
            }
        }

        public LSA_TRUST_ATTRIBUTE TrustAttributes
        {
            get
            {
                return _trustAttributes;
            }
            set
            {
                _trustAttributes = value;
            }
        }

        public LSA_DM_DOMAIN_FLAGS DomainFlags
        {
            get
            {
                return _domainFlags;
            }
            set
            {
                _domainFlags = value;
            }
        }

    }

    public class LsaDCInfo
    {
        private string _name;
        private string _address;
        private string _siteName;
        private LSA_DS_FLAGS _dsflags;

        public string Name
        {
            get
            {
                return _name;
            }
            set
            {
                _name = value;
            }
        }

        public string Address
        {
            get
            {
                return _address;
            }
            set
            {
                _address = value;
            }
        }

        public string SiteName
        {
            get
            {
                return _siteName;
            }
            set
            {
                _siteName = value;
            }
        }

        public LSA_DS_FLAGS DSflags
        {
            get
            {
                return _dsflags;
            }
            set
            {
                _dsflags = value;
            }
        }
    }
}
