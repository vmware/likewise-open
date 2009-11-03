using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class SelectComputerDialog : Form
    {
        #region Class Data

        private string sUsername = "";
        public CredentialsDialog credsDialog = null;

        #endregion

        #region Constructors

        public SelectComputerDialog()
        {
            InitializeComponent();
        }

        public SelectComputerDialog(string hostname, string username)
            : this()
        {
            if (String.IsNullOrEmpty(username))
                rbLocalComputer.Checked = true;
            else
            {
                this.sUsername = username;
                this.tbComputer.Text = hostname;
                rbRemoteComputer.Checked = true;
            }
        }

        #endregion

        #region Events

        private void linkCreds_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            credsDialog = new CredentialsDialog(sUsername);
            credsDialog.ShowDialog(this);
        }

        private void rbLocalComputer_CheckedChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = true;
        }

        private void rbRemoteComputer_CheckedChanged(object sender, EventArgs e)
        {
            tbComputer.Enabled = rbRemoteComputer.Checked;
            btnOk.Enabled = false;

            if (tbComputer.Enabled && tbComputer.TextLength != 0)
            {
                btnOk.Enabled = true;
            }
        }

        private void tbComputer_TextChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = false;

            if (tbComputer.Enabled && tbComputer.TextLength != 0)
            {
                btnOk.Enabled = true;
            }
        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            bool okayToExit = true;

            if (credsDialog != null)
            {
                if (credsDialog.UseDefaultUserCreds() != true) {
                    okayToExit = !String.IsNullOrEmpty(credsDialog.GetUsername()) && !String.IsNullOrEmpty(credsDialog.GetPassword());
                }
            }

            if (!rbLocalComputer.Checked && credsDialog != null && credsDialog.UseDefaultUserCreds() == true)
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

        public string GetHostname()
        {
            if (rbLocalComputer.Checked) {
                return System.Environment.MachineName;
            }
            else
            {
                return tbComputer.Text;
            }
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