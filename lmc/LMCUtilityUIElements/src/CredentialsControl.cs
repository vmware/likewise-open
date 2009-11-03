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

        public Credentials credentials = null;

        #endregion

        #region Constructors

        public CredentialsControl()
        {
            InitializeComponent();
        }

        public CredentialsControl(string username, string password)
            : this()
        {
            credentials = new Credentials(username, password);
        }

        #endregion

        #region Events

        private void rbUseCurrentUserCreds_CheckedChanged(object sender, EventArgs e)
        {
            tbUsername.Enabled = false;
            tbUsername.Text = "";
            tbPassword.Enabled = false;
            tbPassword.Text = "";

            credentials.UseDefaultUserCreds = rbUseCurrentUserCreds.Checked;

            CredentialsDialog credDlg = (CredentialsDialog)this.Parent;
            credDlg.OKBtn.Enabled = true;
        }

        private void rbUseTheseCreds_CheckedChanged(object sender, EventArgs e)
        {
            groupBox.Enabled = rbUseTheseCreds.Checked;
            tbUsername.Enabled = groupBox.Enabled;
            tbPassword.Enabled = groupBox.Enabled;

            CredentialsDialog credDlg = (CredentialsDialog)this.Parent;

            credDlg.OKBtn.Enabled = false;
            if (tbUsername.Enabled == true && !string.IsNullOrEmpty(credentials.Username))
            {
                credDlg.OKBtn.Enabled = true;
            }
        }

        private void tbUsername_TextChanged(object sender, EventArgs e)
        {
            credentials.Username = tbUsername.Text;
            CredentialsDialog credDlg = (CredentialsDialog)this.Parent;

            credDlg.OKBtn.Enabled = false;

            if (!string.IsNullOrEmpty(credentials.Username))
            {
                credDlg.OKBtn.Enabled = true;
            }
        }

        private void tbPassword_TextChanged(object sender, EventArgs e)
        {
            credentials.Password = tbPassword.Text;
        }

        private void CredentialsControl_Load(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(credentials.Username))
            {
                this.rbUseCurrentUserCreds.Checked = true;
                this.rbUseTheseCreds.Checked = false;
            }
            else
            {
                this.rbUseCurrentUserCreds.Checked = false;
                this.rbUseTheseCreds.Checked = true;
                this.tbUsername.Text = credentials.Username;
                this.tbPassword.Text = "";
            }
        }

        #endregion

        #region Access Specifiers

        public string Username
        {
            set {
                credentials.Username = value;
            }
            get {
                return credentials.Username;
            }
        }

        public string Password
        {
            set {
                credentials.Password = value;
            }
            get {
                return credentials.Password;
            }
        }

        public bool UseDefaultCreds
        {
            set {
                credentials.UseDefaultUserCreds = value;
            }
            get {
                return credentials.UseDefaultUserCreds;
            }
        }

        #endregion
    }
}
