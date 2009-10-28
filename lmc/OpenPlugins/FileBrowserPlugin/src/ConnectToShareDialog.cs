using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public partial class ConnectToShareDialog : Form
    {
        string path = null;

        public ConnectToShareDialog()
        {
            InitializeComponent();
        }

        public ConnectToShareDialog(string path, bool showPathError)
            : this()
        {
            this.cbUseAlternateCreds.Checked = false;
            this.tbPath.Text = path;
            this.lblErrorMessage.Hide();

            if (showPathError)
            {
                this.lblErrorMessage.Show();
                this.tbPath.SelectAll();
            }
        }

        public string GetPath()
        {
            return path;
        }

        public bool UseAlternateUserCreds()
        {
            return this.cbUseAlternateCreds.Checked;
        }

        private void CancelBtn_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void OKBtn_Click(object sender, EventArgs e)
        {
            bool okayToExit = true;

            // Copy the values from the edit field.
            this.path = tbPath.Text;

            if (string.IsNullOrEmpty(path))
            {
                okayToExit = false;
            }

            if (okayToExit == true)
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        private void tbPath_TextChanged(object sender, EventArgs e)
        {
            lblErrorMessage.Hide();
        }
    }
}