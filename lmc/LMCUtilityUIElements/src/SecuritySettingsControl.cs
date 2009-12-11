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
        }

        #endregion

        #region Helepr functions

        private void InitailizeData()
        {
            this.lblObjectName.Text = string.Format(lblObjectName.Text, _objectPath);

            List<ListViewItem> lvItems = new List<ListViewItem>();
            List<string> possiblePermissions = new List<string>();

            if (_securityDescriptor != null)
            {
                if (_securityDescriptor.Descretionary_Access_Control_List != null)
                {
                    Dictionary<string, Dictionary<string, string>> SdDacls =
                                                _securityDescriptor.Descretionary_Access_Control_List as
                                                Dictionary<string, Dictionary<string, string>>;

                    if (SdDacls != null && SdDacls.Count != 0)
                    {
                        foreach (string key in SdDacls.Keys)
                        {
                            Dictionary<string, string> daclInfo = SdDacls[key];
                            int daclCount = daclInfo.Count;

                            ListViewItem lvItem = new ListViewItem(new string[] { SdDacls[key]["Username"] });
                            lvItem.Tag = daclInfo;
                            lvItems.Add(lvItem);

                            List<object[]> permissions = _securityDescriptor.GetPermissionsFromAccessMask(
                                                         _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, "AceMask") as string);
                            foreach (object[] arry in permissions)
                            {
                                DgPermissions.Rows.Add(arry);
                            }
                        }
                    }
                }
            }
        }

        #endregion

        #region Events

        private void btnEdit_Click(object sender, EventArgs e)
        {
            PermissionsControlDlg permissionsDlg = new PermissionsControlDlg(_securityDescriptor, _objectPath);
            permissionsDlg.ShowDialog(this);
        }

        private void btnAdvanced_Click(object sender, EventArgs e)
        {
            AdvancedPermissionsControlDlg advancedPermissionsControlDlg = new AdvancedPermissionsControlDlg();
            advancedPermissionsControlDlg.ShowDialog(this);
        }

        private void SecuritySettingsControl_Load(object sender, EventArgs e)
        {
            InitailizeData();
        }

        private void lvGroupOrUserNames_SelectedIndexChanged(object sender, EventArgs e)
        {
            ListView listview = sender as ListView;
            if (listview != null)
            {
                if (listview.SelectedItems.Count != 0)
                {
                    ListViewItem lvItem = listview.SelectedItems[0];
                    if (lvItem.Tag != null)
                    {
                        Dictionary<string, string> daclInfo = lvItem.Tag as Dictionary<string, string>;
                        if (daclInfo != null)
                        {
                            List<object[]> permissions = _securityDescriptor.GetPermissionsFromAccessMask(
                                                         _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, "AceMask") as string);

                            foreach (object[] arry in permissions)
                            {
                                DgPermissions.Rows.Add(arry);
                            }
                        }
                    }
                }
            }
        }

        #endregion
    }
}
