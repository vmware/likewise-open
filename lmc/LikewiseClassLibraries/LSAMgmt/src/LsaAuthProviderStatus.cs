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
    public enum LsaAuthProviderMode
    {
        LSA_PROVIDER_MODE_UNKNOWN = 0,
        LSA_PROVIDER_MODE_UNPROVISIONED,
        LSA_PROVIDER_MODE_DEFAULT_CELL,
        LSA_PROVIDER_MODE_NON_DEFAULT_CELL,
        LSA_PROVIDER_MODE_LOCAL_SYSTEM
    }

    public enum LsaAuthProviderSubMode
    {
        LSA_AUTH_PROVIDER_SUBMODE_UNKNOWN = 0,
        LSA_AUTH_PROVIDER_SUBMODE_SCHEMA,
        LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA
    }

    public enum LsaAuthProviderState
    {
        LSA_AUTH_PROVIDER_STATUS_UNKNOWN = 0,
        LSA_AUTH_PROVIDER_STATUS_ONLINE,
        LSA_AUTH_PROVIDER_STATUS_OFFLINE
    }

    public class LsaAuthProviderStatus
    {
        private string _id;
        private string _domain;
        private string _forest;
        private string _site;
        private string _cell;
        private LsaAuthProviderMode    _mode;
        private LsaAuthProviderSubMode _subMode;
        private LsaAuthProviderState _state;

        private UInt32 _networkCheckInterval;
        private UInt32 _numTrustedDomains;

        private List<LsaTrustedDomainInfo> _trustedDomainInfo;

        public LsaAuthProviderStatus()
        {
            _id = string.Empty;
            _domain = string.Empty;
            _forest = string.Empty;
            _site = string.Empty;
            _cell = string.Empty;

            _mode = LsaAuthProviderMode.LSA_PROVIDER_MODE_UNKNOWN;
            _subMode = LsaAuthProviderSubMode.LSA_AUTH_PROVIDER_SUBMODE_UNKNOWN;
            _state = LsaAuthProviderState.LSA_AUTH_PROVIDER_STATUS_UNKNOWN;

            _networkCheckInterval = 0;
            _numTrustedDomains = 0;
            _trustedDomainInfo = new List<LsaTrustedDomainInfo>();
        }

        public string Id
        {
            get
            {
                return _id;
            }
            set
            {
                _id = value;
            }
        }

        public string Domain
        {
            get
            {
                return _domain;
            }
            set
            {
                _domain = value;
            }
        }

        public string Forest
        {
            get
            {
                return _forest;
            }
            set
            {
                _forest = value;
            }
        }

        public string Site
        {
            get
            {
                return _site;
            }
            set
            {
                _site = value;
            }
        }

        public string Cell
        {
            get
            {
                return _cell;
            }
            set
            {
                _cell = value;
            }
        }

        public LsaAuthProviderMode Mode
        {
            get
            {
                return _mode;
            }
            set
            {
                _mode = value;
            }
        }

        public LsaAuthProviderSubMode Submode
        {
            get
            {
                return _subMode;
            }
            set
            {
                _subMode = value;
            }
        }

        public LsaAuthProviderState State
        {
            get
            {
                return _state;
            }
            set
            {
                _state = value;
            }
        }

        public UInt32 NetworkCheckInterval
        {
            get
            {
                return _networkCheckInterval;
            }
            set
            {
                _networkCheckInterval = value;
            }
        }

        public UInt32 NumTrustedDomains
        {
            get
            {
                return _numTrustedDomains;
            }
            set
            {
                _numTrustedDomains = value;
            }
        }

        public List<LsaTrustedDomainInfo> TrustedDomainInfo
        {
            get
            {
                return _trustedDomainInfo;
            }
        }
    }
}
