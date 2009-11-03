using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class CredentialsDialog : Form
    {
        #region Class Data

        #endregion

        #region Constructors

        public CredentialsDialog()
        {
            InitializeComponent();
        }

        public CredentialsDialog(string username)
            : this()
        {
            credentialsControl.Username = username;
        }

        #endregion

        #region Helper functions

        public string GetUsername()
        {
            return credentialsControl.Username;
        }

        public string GetPassword()
        {
            return credentialsControl.Password;
        }

        public bool UseDefaultUserCreds()
        {
            return credentialsControl.UseDefaultCreds;
        }

        #endregion

        #region Events

        private void CancelBtn_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void OKBtn_Click(object sender, EventArgs e)
        {
            bool okayToExit = true;

            if (!UseDefaultUserCreds())
            {
                if (GetUsername().Length == 0 || GetPassword().Length == 0)
                    okayToExit = false;
            }

            if (okayToExit == true)
            {
                if (UseDefaultUserCreds())
                {
                    credentialsControl.Username = "";
                    credentialsControl.Password = "";
                }

                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        #endregion
    }
}