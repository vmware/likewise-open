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
    public class Hostinfo : IContext
    {
        #region types

        public struct FieldBitmaskBits
        {
            //list of fields to be used in a bitmask.
            public static uint SHORT_HOSTNAME = 0x01; //short hostname or alias
            public static uint FQ_HOSTNAME = 0x02; //Fully qualified host name
            public static uint FQDN = 0x04; //Fully-qualified domain name
            public static uint FQ_DCNAME = 0x08; //Fully-qualified domain controller name
            public static uint CREDS_NT4DOMAIN = 0x10;
            public static uint CREDS_USERNAME = 0x20;
            public static uint CREDS_PASSWORD = 0x40;
            public static uint FORCE_USER_PROMPT = 0x80;

        }

        #endregion

        #region Class data

        //This is the short hostname
        public string shortName = null;

        //e.g., 'person1.sales.mycompany.com', where FQDN = person1.sales.mycompany.com
        public string hostName = null;

        //e.g., 'sales.mycompany.com', where FQDN = person1.sales.mycompany.com
        public string domainName = null;

        public string domainControllerName = null;

        // store creds here, too, for good measure
        public CredentialEntry creds = null;

        public bool IsSmbAvailable = false;
        public bool IsSSHAvailable = false;
        public bool IsNetBiosAvailable = false;
        private bool bIsConnectionSuccess = false;

        private object tag = null;

        #endregion

        #region Constructor

        /// <summary>
        /// Constructor will attempt to populate all fields using the command klist (kerberos)
        /// </summary>
        public Hostinfo()
        {
            creds = new CredentialEntry();

            if (Environment.OSVersion.Platform == PlatformID.Unix)
            {
                //this is commented out, since spinning off a new process
                //will under some circumstances result in an unhandled exception
                //within libdcerpc::rpc__timer_fork_handler which will bring down
                //the application in dcerpc/exc_handling::dce_ptdexc_abort
                //see bug# 4882
                //TODO: this should be re-written to use Kerberos API calls.
                //getUnixKerberosInfoFromKlist();
            }
        }

        /// <summary>
        /// Simple constructor given only a name. All
        /// other properties must be set later as their
        /// values become available.
        /// </summary>
        /// <param name="sHostname">The machine name entered/picked by the user</param>
        public Hostinfo(string sHostname)
        {
            Canonize(sHostname);

            creds = new CredentialEntry();
        }

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

        #endregion

        #region operator overloads

        public static bool operator==(Hostinfo lhs, Hostinfo rhs)
        {
            Object lhsObj = (Object)lhs;
            Object rhsObj = (Object)rhs;


            if (lhsObj == null && rhsObj == null) return true;
            else if (lhsObj == null && rhsObj != null) return false;
            else if (lhsObj != null && rhsObj == null) return false;

            else if (lhs.hostName != rhs.hostName) return false;
            else if (lhs.domainName != rhs.domainName) return false;
            else if (lhs.creds != rhs.creds) return false;
            else return true;

        }

        public static bool operator!=(Hostinfo lhs, Hostinfo rhs)
        {
            if (lhs == rhs) return false;
            else return true;
        }

        public override bool Equals(object obj)
        {
            if (obj is Hostinfo)
            {
                return this == (Hostinfo)obj;
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
            string[] parts = new string[] {
                "\nHostinfo:\n",
                "=========\n",
                String.Format("hostName: {0}\n", hostName==null ? "<null>" : hostName),
                String.Format("domainName: {0}\n", domainName==null ? "<null>" : domainName),
                String.Format("domainControllerName: {0}\n", domainControllerName==null ? "<null>" : domainControllerName),
                String.Format("creds: {0}", creds==null ? "<null>" : creds.ToString())
            };

            string result = String.Concat(parts);

            return result;
        }

        #endregion

        #region Public Methods

        public Hostinfo Clone()
        {
            Hostinfo result = new Hostinfo(this.hostName);

            result.domainName = this.domainName;

            result.IsSmbAvailable = this.IsSmbAvailable;
            result.IsSSHAvailable = this.IsSSHAvailable;
            result.IsNetBiosAvailable = this.IsNetBiosAvailable;

            if (this.creds != null)
            {
                result.creds = this.creds.Clone();
            }

            return result;
        }

        public static bool HasCreds(Hostinfo hn)
        {

            if (hn == null ||
                hn.creds == null ||
                CredentialEntry.IsNullOrEmpty(hn.creds))
            {
                return false;
            }
            else
            {
                return true;
            }

        }

        private void Canonize(string inputHostName)
        {
            if (inputHostName == null)
                return;
            // clean up and store
            hostName = inputHostName.Trim().ToLower();

            // if "localhost", translate since windows has trouble with this
            if (hostName == Resources.Localhost)
            {
                hostName = Environment.MachineName;
            }

            // split the provided name into parts
            string[] aparts = hostName.Split(new char[] { '.' });

            // set the shortname
            shortName = aparts[0];

            // set the rest
            if (aparts.Length > 1)
            {
                domainName = "";
                for (int i = 1; i < aparts.Length; i++)
                {
                    if (i != 1)
                    {
                        domainName += ".";
                    }
                    domainName += aparts[i];
                }
            }

            Logger.Log(String.Format("Hostinfo.Canonize({0}) = host:{1}, short:{2}, domain:{3}",
                inputHostName, hostName, shortName, domainName), Logger.manageLogLevel);

        }

        #endregion

        #region Validation Methods

        public static bool ValidateHostName(string hostName, out string errorMessage)
        {
            errorMessage = null;

            if (string.IsNullOrEmpty(hostName))
            {
                errorMessage = "host name is null or empty";
                return false;
            }
            foreach (char c in hostName)
            {
                if (Char.IsWhiteSpace(c) || Char.IsControl(c))
                {
                    errorMessage = "Warning: Hostname contains whitespace or control character.";
                    return false;
                }
            }

            char[] arr = hostName.ToCharArray();
            foreach (char c in arr)
            {
                if (!(Char.IsNumber(c) || Char.IsLetter(c) || Char.IsWhiteSpace(c) || Char.IsControl(c) || (c == '_') || (c == '-') || (c == '.') || (c == ' ')))
                {
                    errorMessage = "When connecting to a remote host, please use an qualified host name composed of letters, numbers, '.', '_' or '-'.";
                    return false;
                }
            }

            try
            {
                IPAddress ipAddr = null;
                if (IPAddress.TryParse(hostName, out ipAddr))
                {
                    errorMessage = "When connecting to a remote host, please use an qualified host name instead of IP addresses.";
                    return false;
                }
            }
            catch (Exception)
            {
                errorMessage = "When connecting to a remote host, please use an qualified host name composed of letters, numbers, '.', '_' or '-'.";
                return false;
            }

            return true;
        }

        public static bool ValidateFullyQualifiedHostName(string fullyQualifiedHostName, out string errorMessage)
        {

            errorMessage = null;
            IPAddress[] addrs = null;
            string advice = "Please enter a valid Qualified host name composed of letters, numbers, or hyphens.";

            if (!ValidateHostName(fullyQualifiedHostName, out errorMessage))
            {
                return false;
            }

            try
            {
                addrs = Dns.GetHostAddresses(fullyQualifiedHostName);
            }
            catch (System.Net.Sockets.SocketException se)
            {
                errorMessage = String.Format(
                        "Could not resolve IP address of {0}: {1}.  {2}",
                        fullyQualifiedHostName, se.SocketErrorCode, advice);
                return false;
            }

            if (addrs == null || addrs.Length == 0)
            {
                errorMessage = String.Format(
                    "Could not resolve IP address of {0}.  {1}",
                    fullyQualifiedHostName, advice);
                return false;
            }

            return true;
        }

        public static bool ValidateIPAddress(string ipAddr, out string errorMessage)
        {
            errorMessage = string.Empty;

            string[] sPortArr = ipAddr.Split('.');

            if (sPortArr.Length == 4)
            {
                foreach (string strCopy in sPortArr)
                {
                    try
                    {
                        int iPort = Convert.ToInt16(strCopy);
                        if (iPort > 255)
                        {
                            errorMessage = "Invalid IP Address";
                            return false;
                        }
                    }
                    catch
                    {
                        errorMessage = "Invalid IP Address";
                        return false;
                    }
                }
            }
            else
            {
                errorMessage = "Invalid IP Address";
                return false;
            }
            return true;
        }

        public static bool ValidateFQDN(string FQDN, out string errorMessage)
        {
            errorMessage = null;

            if (string.IsNullOrEmpty(FQDN))
            {
                errorMessage = "DomainNameValidate() called on null or empty hostName";
                return false;
            }

            if (FQDN.Length > 255)
            {
                errorMessage = "Please enter a domain name with no more than 255 characters";
                return false;
            }

            char[] arr = FQDN.ToCharArray();

            foreach (char c in arr)
            {
                if (!(Char.IsNumber(c) || Char.IsLetter(c) || (c == '_') || (c == '-') || (c == '.')))
                {
                    errorMessage = "When connecting to a domain, please use a domain name composed of letters, numbers, '_', '-', or '.'.";
                    return false;
                }
            }

            return true;
        }

        public static bool ValidateFullyQualifiedDCName(string fullyQualifiedDCName, out string errorMessage)
        {
            return ValidateFullyQualifiedHostName(fullyQualifiedDCName, out errorMessage);
        }

        public static bool ValidateShortDomainName(string shortDomainName, out string errorMessage)
        {
            return ValidateString(shortDomainName, 15, out errorMessage);
        }

        public static bool ValidateUserName(string userName, out string errorMessage)
        {
            return ValidateString(userName, out errorMessage);
        }

        public static bool ValidatePassword(string pw1, string pw2, out string errorMessage)
        {
            errorMessage = null;

            if (pw1.Length > 255)
            {
                errorMessage = "Password length is greater than 255.";
                return false;
            }
            else if (pw1 != pw2)
            {
                errorMessage = "Passwords do not match.  Please try again.";
                return false;
            }
            else
            {
                foreach (char c in pw1)
                {
                    if (Char.IsWhiteSpace(c) || Char.IsControl(c))
                    {
                        errorMessage = "Password contains whitespace or control characters.  Please try again.";
                        return false;
                    }
                }
            }
            return true;
        }

        public static bool ValidateString(string strInput, int maxLength, out string errorMessage)
        {
            errorMessage = null;
            if (strInput.Length > maxLength)
            {
                errorMessage = String.Format("Error: input length is greater than {0}.", maxLength);
                return false;
            }
            else
            {
                foreach (char c in strInput)
                {
                    if (!(Char.IsLetterOrDigit(c) || Char.IsPunctuation(c) || Char.IsWhiteSpace(c)))
                    {
                        errorMessage = "input contains characters other than letters, numbers, whitespace, and punctuation.  Please try again.";
                        return false;
                    }
                }
            }
            return true;
        }

        public static bool ValidateString(string strInput, out string errorMessage)
        {
            errorMessage = null;
            return ValidateString(strInput, 255, out errorMessage);
        }

        #endregion

    }
}
