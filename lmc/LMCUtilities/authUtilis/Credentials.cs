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

        private string sUsername = string.Empty;
        private string sPassword = string.Empty;
        private bool bUseDefaultCreds=false;

        #endregion

        #region Constructors

        public Credentials()
        {
            SetDefaultCredentails();
        }

        public Credentials(string username, string password)
        {
            if (String.IsNullOrEmpty(username) || String.IsNullOrEmpty(password)) {
                SetDefaultCredentails();
            }
            else {
                this.sUsername = username;
                this.sPassword = password;
            }
        }

        #endregion

        #region Public functions

        public void SetDefaultCredentails()
        {
            if (String.IsNullOrEmpty(Username) && !String.IsNullOrEmpty(System.Environment.UserName)) {
                Username = System.Environment.UserName;
                bUseDefaultCreds = true;
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

        public bool UseDefaultUserCreds
        {
            get {
                return bUseDefaultCreds;
            }
            set {
                bUseDefaultCreds = value;
            }
        }

        #endregion
    }
}
