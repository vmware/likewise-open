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
using System.Security.Principal;
using System.Net;
using System.Diagnostics;
using System.IO;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl.Properties;

namespace Likewise.LMC.ServerControl
{
    /// <summary>
    /// This class represents a machine to which the user
    /// is trying/has connected to. It bundles a variety of
    /// useful data:
    ///     
    ///     It's host name and domain name
    ///     A set of credentials that can be used with the machine
    /// 
    /// </summary>
    public class DbConnInfo : IContext
    {
        #region Class data

        //This is Db provider name
        public string sDbProvider = string.Empty;

        // Db connection string
        public string sConnString = string.Empty;

        private bool bIsConnectionSuccess = false;

        private object tag = null;

        #endregion

        #region Constructor

        /// <summary>
        /// Constructor will attempt to populate all fields using the command klist (kerberos)
        /// </summary>
        public DbConnInfo()
        {
            sDbProvider = string.Empty;
            sConnString = string.Empty;
        }

        /// <summary>
        /// Simple constructor given only a name. All 
        /// other properties must be set later as their
        /// values become available.
        /// </summary>
        /// <param name="sHostname">The machine name entered/picked by the user</param>
        public DbConnInfo(string sDbProvider, string sConnString)
        {
           this.sDbProvider = sDbProvider.Trim().ToLower();
           this.sConnString = sConnString.Trim().ToLower();               
        }

        #endregion

        public bool IsConnectionSuccess
        {
            set
            {
                bIsConnectionSuccess = value;
            }
            get
            {
                return bIsConnectionSuccess;
            }
        }

        public object Tag
        {
            set
            {
                tag = value;
            }
            get
            {
                return tag;
            }
        }

        #region operator overloads

        public static bool operator ==(DbConnInfo lhs, DbConnInfo rhs)
        {
            Object lhsObj = (Object)lhs;
            Object rhsObj = (Object)rhs;


            if (lhsObj == null && rhsObj == null) return true;
            else if (lhsObj == null && rhsObj != null) return false;
            else if (lhsObj != null && rhsObj == null) return false;

            else if (lhs.sDbProvider != rhs.sDbProvider) return false;
            else if (lhs.sConnString != rhs.sConnString) return false;            
            else return true;      
        }

        public static bool operator !=(DbConnInfo lhs, DbConnInfo rhs)
        {
            if (lhs == rhs) return false;
            else return true;
        }

        public override bool Equals(object obj)
        {
            if (obj is DbConnInfo)
            {
                return this == (DbConnInfo)obj;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        #endregion

        #region overload methods

        public override string ToString()
        {
            string[] parts = new string[]
            {
                "\nDbConnInfo:\n",
                "=========\n",
                String.Format("DbProviderName: {0}\n", sDbProvider==null ? "<null>" : sDbProvider),
                String.Format("DbConnString: {0}\n", sConnString==null ? "<null>" : sConnString),
            };

            string result = String.Concat(parts);

            return result;
        }

        #endregion

        #region Public Methods

        public DbConnInfo Clone()
        {
            DbConnInfo result = new DbConnInfo(this.sDbProvider, this.sConnString);

            result.sDbProvider = this.sDbProvider;
            result.sConnString = this.sConnString;           

            return result;
        }

        public void Reset(string sDbProvider, string sDbConnString)
        {
            if (!String.IsNullOrEmpty(sDbProvider))
                this.sDbProvider = sDbProvider;
            if (!String.IsNullOrEmpty(sDbConnString))   
                this.sConnString = sDbConnString;
        }

        #endregion     
    }
}
