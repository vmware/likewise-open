using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.LMCCredentials
{
    public partial class SelectDomainDialog : Form
    {
        #region Class Data

        private Hostinfo _hn = null;
        private CredentialsDialog credsDialog = null;
        private string sUsername = "";

        #endregion

        #region Constructors

        public SelectDomainDialog()
        {
            InitializeComponent();
        }

        public SelectDomainDialog(IPlugIn plugin, string domainname)
            : this()
        {
            tbDomain.Text = domainname;
            rbDefaultDomain.Checked = true;

            _hn.domainName = domainname;
            LMCCredentials.GetPluginCredentials(plugin, _hn);
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
                tbDomain.Text = domain;
                rbOtherDomain.Checked = true;
            }
        }

        public SelectDomainDialog(LMCCredentials lmcCredentials)
            : this()
        {
            if (lmcCredentials != null)
            {
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

        private void btnOk_Click(object sender, EventArgs e)
        {
            if (rbDefaultDomain.Checked) {
                _hn.domainName = System.Environment.UserDomainName;
            }
            else
            {
                if (credsDialog != null)
                {
                    if (!String.IsNullOrEmpty(credsDialog.CredentialsControl.Username))
                    {
                        _hn.domainName = tbDomain.Text;
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

        private void tbDomain_TextChanged(object sender, EventArgs e)
        {
            btnOk.Enabled = true;
        }

        #endregion
    }
}