using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.ServerControl
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
        }

        private void tbComputer_TextChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = true;
        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            if (rbLocalComputer.Checked) {
                _hn.hostName = System.Environment.MachineName;
            }
            else
            {
                if (credsDialog != null) {
                    if (!String.IsNullOrEmpty(credsDialog.CredentialsControl.Username))
                    {
                        _hn.hostName = tbComputer.Text;
                        _hn.creds.UserName = credsDialog.GetUsername();
                        _hn.creds.Password = credsDialog.GetPassword();
                    }
                    else
                    {
                        MessageBox.Show(
                           "You have selected the Remost host that needs the credentials cache.\n Please click on 'alternate' to set the credentails",
                           "Likewise Administrative Console",
                           MessageBoxButtons.OK,
                           MessageBoxIcon.Exclamation);
                        return;
                    }
                }
            }

            Close();
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