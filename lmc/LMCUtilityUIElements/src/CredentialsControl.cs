using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class CredentialsControl : UserControl
    {
        #region ClassData

        private string sUsername = string.Empty;
        private string sPassword = string.Empty;
        private bool bUseDefaultCreds = false;

        #endregion

        #region Constructors

        public CredentialsControl()
        {
            InitializeComponent();
        }

        public CredentialsControl(string username)
            : this()
        {
            this.sUsername = username;
        }

        #endregion

        #region Events

        private void rbUseCurrentUserCreds_CheckedChanged(object sender, EventArgs e)
        {
            tbUsername.Enabled = false;
            tbUsername.Text = "";
            tbPassword.Enabled = false;
            tbPassword.Text = "";

            bUseDefaultCreds = rbUseCurrentUserCreds.Checked;
        }

        private void rbUseTheseCreds_CheckedChanged(object sender, EventArgs e)
        {
            groupBox.Enabled = rbUseTheseCreds.Checked;
        }

        private void tbUsername_TextChanged(object sender, EventArgs e)
        {
            sUsername = tbUsername.Text;
        }

        private void tbPassword_TextChanged(object sender, EventArgs e)
        {
            sPassword = tbPassword.Text;
        }

        private void CredentialsControl_Load(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(sUsername))
            {
                this.rbUseCurrentUserCreds.Checked = true;
                this.rbUseTheseCreds.Checked = false;
            }
            else
            {
                this.rbUseCurrentUserCreds.Checked = false;
                this.rbUseTheseCreds.Checked = true;
                this.tbUsername.Text = sUsername;
                this.tbPassword.Text = "";
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

        public bool UseDefaultCreds
        {
            set {
                bUseDefaultCreds = value;
            }
            get {
                return bUseDefaultCreds;
            }
        }

        #endregion
    }
}
