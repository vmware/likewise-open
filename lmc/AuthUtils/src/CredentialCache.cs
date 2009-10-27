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
using System.Net;

namespace Likewise.LMC.AuthUtils
{
    public class CredentialEntry: NetworkCredential
    {
        #region data

        private bool _bInvalidated;
        private bool _bDefaultCreds;
        private string _sMachineName;

        #endregion

        #region constructors

        public CredentialEntry()
        {
            _bInvalidated = false;
            _bDefaultCreds = true;
        }

        protected CredentialEntry(bool invalidated, bool defaultCreds, string machineName) : base()
        {
            _bInvalidated = invalidated;
            _bDefaultCreds = defaultCreds;
            _sMachineName = machineName;
        }

        public CredentialEntry(NetworkCredential creds) : base(creds.UserName, creds.Password, creds.Domain)
        {
            _bInvalidated = false;
            _bDefaultCreds = false;
        }

        public CredentialEntry(string userName, string password, string domain)
            : base(userName, password, domain)
        {
            _bInvalidated = false;
            _bDefaultCreds = false;
        }

        #endregion

        #region operator overloads

        public static bool operator ==(CredentialEntry lhs, CredentialEntry rhs)
        {
            Object lhsObj = (Object)lhs;
            Object rhsObj = (Object)rhs;


            if (lhsObj == null && rhsObj == null) return true;
            else if (lhsObj == null && rhsObj != null) return false;
            else if (lhsObj != null && rhsObj == null) return false;
            else if (lhs.DefaultCreds != rhs.DefaultCreds) return false;
            else if (lhs.Domain != rhs.Domain) return false;
            else if (lhs.Invalidated != rhs.Invalidated) return false;
            else if (lhs.MachineName != rhs.MachineName) return false;
            else if (lhs.Password != rhs.Password) return false;
            else if (lhs.UserName != rhs.UserName) return false;
            else return true;
        }

        public static bool operator !=(CredentialEntry lhs, CredentialEntry rhs)
        {
            if (lhs == rhs) return false;
            else return true;
        }

        public override bool Equals(object obj)
        {
            if (obj is CredentialEntry)
            {
                return this == (CredentialEntry)obj;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        #endregion

        #region accessors

        public bool Invalidated
        {
            get
            {
                return _bInvalidated;
            }            
        }

        public bool DefaultCreds
        {
            get
            {
                return _bDefaultCreds;
            }
        }

        public string MachineName
        {
            get
            {
                return _sMachineName;
            }
            set
            {
                _sMachineName = value;
            }
        }

        #endregion

        #region Helper functions

        public void Invalidate(String sServer)
        {
            _bInvalidated = true;
            if (sServer != null)
                Session.DeleteNullSession(sServer);
        }

        public static bool IsNullOrEmpty(CredentialEntry ce)
        {
            if (ce == null ||
                String.IsNullOrEmpty(ce.Domain) ||
                String.IsNullOrEmpty(ce.Password) ||
                String.IsNullOrEmpty(ce.UserName))
            {
                return true;
            }

            return false;
        }

        public CredentialEntry Clone()
        {

            string resultMachineName = null;


            if (!String.IsNullOrEmpty(this.MachineName))
            {
                resultMachineName = (string)this.MachineName.Clone();
            }


            CredentialEntry result = new CredentialEntry(this.Invalidated, this.DefaultCreds, resultMachineName);

            result.Domain = (this.Domain == null) ? null : (string) this.Domain.Clone();
            result.Password = (this.Password == null) ? null : (string)this.Password.Clone();
            result.UserName = (this.UserName == null) ? null : (string)this.UserName.Clone();



            return result;
        }

        #endregion

        #region override method

        public override string ToString()
        {
            return String.Format(
                "[[CREDS: Domain={0}, PasswordLength={1}, Username={2}, Invalid={3}, DefaultCreds={4}, MachineName={5}]]",
                this.Domain==null ? "<null>" : this.Domain,
                this.Password == null ? 0 : this.Password.Length,
                this.UserName == null ? "<null>" : this.UserName,
                _bInvalidated, 
                _bDefaultCreds,
                _sMachineName == null ? "<null>" : _sMachineName);

        }

        #endregion
    }
}
