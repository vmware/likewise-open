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
            bool bAllow = false;
            bool bDeny = false;
            //bool bAudit = false;
            string sPermissiosname = string.Empty;

            if (_securityDescriptor != null)
            {
                if (_securityDescriptor.Descretionary_Access_Control_List != null)
                {
                    Dictionary<string, Dictionary<string, string>> SdDacls = _securityDescriptor.Descretionary_Access_Control_List as
                                                                                 Dictionary<string, Dictionary<string, string>>;

                    if (SdDacls != null && SdDacls.Count != 0)
                    {
                        foreach (string key in SdDacls.Keys)
                        {
                            Dictionary<string, string> daclInfo = SdDacls[key];
                            foreach (string sKey in daclInfo.Keys)
                            {
                                switch (sKey)
                                {
                                    case "Username":
                                        ListViewItem lvItem = new ListViewItem(daclInfo[sKey]);
                                        lvItem.Tag = daclInfo["Sid"];
                                        lvItems.Add(lvItem);
                                        break;

                                    case "Sid":
                                        break;

                                    case "AceType":
                                        if (Convert.ToUInt32(daclInfo[sKey]) == 0)
                                            bAllow = true;
                                        else if (Convert.ToUInt32(daclInfo[sKey]) == 1)
                                            bDeny = true;
                                        //else
                                        //    bAudit = true;
                                        break;

                                    case "AceMask":
                                        SetListWithAccessMask(daclInfo[sKey], ref sPermissiosname);
                                        break;

                                    case "AceFlags":
                                        break;

                                    case "AceSize":
                                        break;
                                }

                                string[] dgRowsItems = new string[]{
                                                    sPermissiosname,
                                                    bAllow.ToString(),
                                                    bDeny.ToString()
                                                    };
                            }
                        }
                    }
                }
            }
        }

        public void SetListWithAccessMask(string accessMask, ref string sPermissionname)
        {
            uint iAccessMask = Convert.ToUInt32(accessMask);

            if ((iAccessMask & (uint)SecurityDescriptorApi.ACCESS_MASK.DELETE) == 0)
            {
                sPermissionname = PermissionsSet.PermissionSet[SecurityDescriptorApi.ACCESS_MASK.DELETE];
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

        private void SecuritySettingsControl_Load(object sender, EventArgs e)
        {
            InitailizeData();
        }

        #endregion
    }
}
