using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.SecurityDesriptor;

using Likewise.LMC.Utilities;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class PermissionsControlDlg : Form
    {
        #region Constructors

        public PermissionsControlDlg()
        {
            InitializeComponent();
        }

        public PermissionsControlDlg(SecurityDescriptor securityDescriptor, string ObjectPath)
            : this()
        {
            this.Text = String.Format("Permissions for {0}", ObjectPath);

            permissionsControl.securityDescriptor = securityDescriptor;
            permissionsControl._ObjectPath = ObjectPath;
        }

        #endregion

        #region Event Handlers

        private void OKBtn_Click(object sender, EventArgs e)
        {
            //TODO; Need to get back the Security descrupotor edited values.

            this.DialogResult = DialogResult.OK;
            Close();
        }

        private void CancelBtn_Click(object sender, EventArgs e)
        {
            Close();
        }

        #endregion

        #region Helper functions

        #endregion
    }
}