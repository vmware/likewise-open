using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Windows.Forms;

namespace Likewise.LMC.UtilityUIElements
{
    public class Credentials
    {
        #region Class Data

        private string sDomain = string.Empty;
        private string sUsername = string.Empty;
        private string sPassword = string.Empty;
        private string sHostname = string.Empty;

        #endregion

        #region Constructors

        public Credentials()
        {
            SetDefaultCredentails();
        }

        public Credentials(string username, string password, string domain, string hostname)
        {
            if (String.IsNullOrEmpty(username) || String.IsNullOrEmpty(password) ||
                String.IsNullOrEmpty(domain)) {
                SetDefaultCredentails();
            }
            else {
                this.sUsername = username;
                this.sPassword = password;
                this.sDomain = domain;
                this.sHostname = hostname;
            }
        }

        #endregion

        #region Public functions

        public void SetDefaultCredentails()
        {
            if (String.IsNullOrEmpty(Domain) && !String.IsNullOrEmpty(System.Environment.UserDomainName)) {
                Domain = System.Environment.UserDomainName;
            }

            if (String.IsNullOrEmpty(Username) && !String.IsNullOrEmpty(System.Environment.UserName)) {
                Username = System.Environment.UserName;
            }

            if (String.IsNullOrEmpty(Hostname) && !String.IsNullOrEmpty(System.Environment.MachineName)) {
                Hostname = System.Environment.MachineName;
            }
        }

        #endregion

        #region Access Specifiers

        public string Username
        {
            set {
                sUsername = value;
            }
            get {
                return sUsername;
            }
        }

        public string Password
        {
            set {
                sPassword = value;
            }
            get {
                return sPassword;
            }
        }

        public string Domain
        {
            set {
                sDomain = value;
            }
            get {
                return sDomain;
            }
        }

        public string Hostname
        {
            set {
                sHostname = value;
            }
            get {
                return sHostname;
            }
        }

        #endregion
    }
}
