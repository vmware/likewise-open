using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.SecurityDesriptor;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class SecuritySettingsControl : UserControl
    {
        #region Class Data

        private SecurityDescriptor _securityDescriptor = null;
        private string _objectPath = string.Empty;

        #endregion

        #region Constructors

        public SecuritySettingsControl()
        {
            InitializeComponent();
        }

        public SecuritySettingsControl(SecurityDescriptor securityDescriptor, string objectPath)
            : this()
        {
            this._securityDescriptor = securityDescriptor;
            this._objectPath = objectPath;

            InitailizeData();
        }

        #endregion

        #region Helepr functions

        private void InitailizeData()
        {
            this.lblObjectName.Text = string.Format(lblObjectName.Text, _objectPath);

            if (_securityDescriptor != null)
            {
                if (_securityDescriptor.Descretionary_Access_Control_List != null)
                {

                }

                if (_securityDescriptor.System_Access_Control_List != null)
                {

                }
            }
        }

        #endregion

        #region Events

        private void btnEdit_Click(object sender, EventArgs e)
        {
            PermissionsControlDlg permissionsDlg = new PermissionsControlDlg(_securityDescriptor);
            permissionsDlg.ShowDialog(this);
        }

        private void btnAdvanced_Click(object sender, EventArgs e)
        {
            AdvancedPermissionsControlDlg advancedPermissionsControlDlg = new AdvancedPermissionsControlDlg();
            advancedPermissionsControlDlg.ShowDialog(this);
        }

        #endregion
    }
}
