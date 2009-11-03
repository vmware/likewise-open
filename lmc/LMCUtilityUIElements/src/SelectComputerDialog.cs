using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class SelectComputerDialog : Form
    {
        #region Class Data

        private string sUsername = "";
        private CredentialsDialog credsDialog = null;
        private Hostinfo _hn = null;

        #endregion

        #region Constructors

        public SelectComputerDialog()
        {
            InitializeComponent();

            _hn = new Hostinfo();
        }

        public SelectComputerDialog(string hostname, string username)
            : this()
        {
            if (String.IsNullOrEmpty(username))
                rbLocalComputer.Enabled = true;
            else
                this.sUsername = username;

            this.tbComputer.Text = hostname;
        }

        public SelectComputerDialog(string hostname)
            : this()
        {
            this.tbComputer.Text = hostname;
            rbLocalComputer.Checked = true;
            rbRemoteComputer.Checked = false;
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
            if (credsDialog != null)
            {
                if (credsDialog.UseDefaultUserCreds() != true)
                {
                    _hn.creds.UserName = credsDialog.GetUsername();
                    _hn.creds.Password = credsDialog.GetPassword();
                }
            }

            if (rbLocalComputer.Checked)
            {
                _hn.hostName = System.Environment.MachineName;
            }
            else
            {
                _hn.hostName = tbComputer.Text;
            }

            if (!rbLocalComputer.Checked  && credsDialog != null && credsDialog.UseDefaultUserCreds() == true)
            {
                MessageBox.Show(
                   "You have selected a remote host that requires credentials.\n Please click on 'alternate' to set the credentails",
                   "Likewise Administrative Console",
                   MessageBoxButtons.OK,
                   MessageBoxIcon.Exclamation);
                return;
            }

            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            Close();
        }

        #endregion

        #region Access Specifiers

        public Hostinfo hostInfo
        {
            get { return _hn; }
        }

        #endregion
    }
}