using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.ServerControl
{
    public partial class CredentialsDialog : Form
    {
        string username = null;
        string password = null;

        public CredentialsDialog()
        {
            InitializeComponent();
        }

        public CredentialsDialog(string username)
            : this()
        {
            if (string.IsNullOrEmpty(username))
            {
                this.rbUseCurrentUserCreds.Checked = true;
                this.rbUseTheseCreds.Checked = false;
            }
            else
            {
                this.rbUseCurrentUserCreds.Checked = false;
                this.rbUseTheseCreds.Checked = true;
                this.tbUsername.Text = username;
                this.tbPassword.Text = "";
            }
        }

        public string GetUsername()
        {
            return username;
        }

        public string GetPassword()
        {
            return password;
        }

        public bool UseDefaultUserCreds()
        {
            return rbUseCurrentUserCreds.Checked;
        }

        private void CancelBtn_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void OKBtn_Click(object sender, EventArgs e)
        {
            bool okayToExit = true;

            // Copy the values from the edit fields.
            this.username = tbUsername.Text;
            this.password = tbPassword.Text;

            if (!rbUseCurrentUserCreds.Checked)
            {
                if (username.Length == 0)
                    okayToExit = false;
            }

            if (okayToExit == true)
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        private void rbUseCurrentUserCreds_CheckedChanged(object sender, EventArgs e)
        {
            tbUsername.Enabled = false;
            tbUsername.Text = "";
            tbPassword.Enabled = false;
            tbPassword.Text = "";
        }

        private void rbUseTheseCreds_CheckedChanged(object sender, EventArgs e)
        {
            tbUsername.Enabled = true;
            tbPassword.Enabled = true;
        }
    }
}