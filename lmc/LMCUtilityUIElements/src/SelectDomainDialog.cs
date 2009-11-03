using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;


namespace Likewise.LMC.UtilityUIElements
{
    public partial class SelectDomainDialog : Form
    {
        #region Class Data

        private string sUsername = "";
        private CredentialsDialog credsDialog = null;

        #endregion

        #region Constructors

        public SelectDomainDialog()
        {
            InitializeComponent();
        }

        public SelectDomainDialog(string domain, string username)
            : this()
        {
            if (String.IsNullOrEmpty(username))
            {
                rbDefaultDomain.Checked = true;
                tbDomain.Text = "";
            }
            else
            {
                sUsername = username;
                tbDomain.Text = domain;
                rbOtherDomain.Checked = true;
            }
        }

        #endregion

        #region Events

        private void linkCreds_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            credsDialog = new CredentialsDialog(sUsername);
            credsDialog.ShowDialog(this);
        }

        private void rbDefaultDomain_CheckedChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = true;
        }

        private void rbOtherDomain_CheckedChanged(object sender, EventArgs e)
        {
            tbDomain.Enabled = rbOtherDomain.Checked;
            btnOk.Enabled = false;
        }

        private void tbDomain_TextChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = !String.IsNullOrEmpty(tbDomain.Text.Trim());

        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            bool okayToExit = true;

            if (credsDialog != null)
            {
                if (credsDialog.UseDefaultUserCreds() != true)
                {
                    okayToExit = !String.IsNullOrEmpty(credsDialog.GetUsername()) && !String.IsNullOrEmpty(credsDialog.GetPassword());
                }
            }

            if (!rbDefaultDomain.Checked && credsDialog != null && credsDialog.UseDefaultUserCreds() == true)
            {
                MessageBox.Show(
                   "You have selected a remote host that requires credentials.\n Please click on 'alternate' to set the credentails",
                   "Likewise Administrative Console",
                   MessageBoxButtons.OK,
                   MessageBoxIcon.Exclamation);
                return;
            }

            if (okayToExit == true)
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            Close();
        }

        #endregion

        #region Helper functions

        public string GetDomain()
        {
            if (rbDefaultDomain.Checked) {
                return System.Environment.UserDomainName;
            }
            else
                return tbDomain.Text.Trim();
        }

        public string GetUsername()
        {
            return credsDialog.GetUsername();
        }

        public string GetPassword()
        {
            return credsDialog.GetPassword();
        }

        public bool UseDefaultUserCreds()
        {
            if (credsDialog != null)
                return credsDialog.UseDefaultUserCreds();
            else
                return true;
        }

        #endregion
    }
}