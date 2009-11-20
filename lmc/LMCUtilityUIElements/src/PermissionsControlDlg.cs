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

        public PermissionsControlDlg(SecurityDescriptor securityDescriptor)
            : this()
        {
            permissionsControl.securityDescriptor = securityDescriptor;
        }

        #endregion

        private void OKBtn_Click(object sender, EventArgs e)
        {
            //TODO; Need to get back the Security descrupotro edited values.

            this.DialogResult = DialogResult.OK;
            Close();
        }

        private void CancelBtn_Click(object sender, EventArgs e)
        {
            Close();
        }

        #region Event Handlers



        #endregion
    }
}