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

        private Dictionary<string, object> _editedObjects = new Dictionary<string, object>();

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

            List<string> possiblePermissions = new List<string>();

            if (_securityDescriptor != null && _securityDescriptor.Descretionary_Access_Control_List != null)
            {
                Dictionary<string, List<LwAccessControlEntry>> SdDacls =
                                            _securityDescriptor.Descretionary_Access_Control_List as
                                            Dictionary<string, List<LwAccessControlEntry>>;

                if (SdDacls != null && SdDacls.Count != 0)
                {
                    foreach (string key in SdDacls.Keys)
                    {
                        List<LwAccessControlEntry> daclInfo = SdDacls[key];
                        ListViewItem lvItem = new ListViewItem(new string[] { key });
                        lvItem.Tag = daclInfo;
                        lvGroupOrUserNames.Items.Add(lvItem);
                    }
                }

                possiblePermissions = _securityDescriptor.GetObjectPermissionSet();
                if (possiblePermissions.Count != 0)
                {
                    foreach (string permission in possiblePermissions)
                    {
                        DgPermissions.Rows.Add(new object[] { permission, false, false });
                    }
                }
                if (lvGroupOrUserNames.Items.Count != 0)
                {
                    lvGroupOrUserNames.Items[0].Selected = true;
                    lvGroupOrUserNames.SelectedIndexChanged += new EventHandler(lvGroupOrUserNames_SelectedIndexChanged);
                }
            }
        }

        public bool OnApply()
        {
            _securityDescriptor.EditAce(_editedObjects, null, null, ref _securityDescriptor.pSecurityDescriptorOut);
            if (_securityDescriptor.pSecurityDescriptorOut == IntPtr.Zero)
                return false;

            return true;
        }

        private void CheckPemissions(object sender, DataGridViewCellMouseEventArgs e)
        {
            if (e.RowIndex == 0 && e.ColumnIndex != 0)
            {
                DataGridViewRow Row = DgPermissions.Rows[e.RowIndex];
                if (Row.Cells[e.ColumnIndex].Value.ToString().Equals("True"))
                {
                    DataGridViewRowCollection dgRows = DgPermissions.Rows;
                    foreach (DataGridViewRow dgRow in dgRows) {
                        dgRow.Cells[e.ColumnIndex].Value = true;
                    }
                }
                else
                {
                    foreach (DataGridViewRow dgRow in DgPermissions.Rows) {
                        dgRow.Cells[e.ColumnIndex].ReadOnly = false;
                    }
                }
            }
            else
            {
                foreach (DataGridViewRow dgRow in DgPermissions.Rows)
                    dgRow.Cells[e.ColumnIndex].ReadOnly = false;
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
            AdvancedPermissionsControlDlg advancedPermissionsControlDlg = new AdvancedPermissionsControlDlg(_securityDescriptor, _objectPath);
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
                        lblPermissions.Text = string.Format("Permissions for {0}", lvItem.Text);
                        List<LwAccessControlEntry> daclInfo = lvItem.Tag as List<LwAccessControlEntry>;
                        List<string> AllowedPermissions = new List<string>();
                        List<string> DeniedPermissions = new List<string>();

                        if (daclInfo != null)
                        {
                            foreach (LwAccessControlEntry ace in daclInfo)
                            {
                                if (ace.AceType == 0 && ace.AceFlags == 16) {
                                    AllowedPermissions = _securityDescriptor.GetUserOrGroupPermissions(ace.AccessMask);
                                }
                                else if (ace.AceType == 1) {
                                    DeniedPermissions = _securityDescriptor.GetUserOrGroupPermissions(ace.AccessMask);
                                }
                            }
                            DataGridViewRowCollection dgRows = DgPermissions.Rows;
                            foreach (DataGridViewRow dgRow in dgRows)
                            {
                                if (AllowedPermissions.Count == 0 && dgRow.Cells[0].Value.ToString().Equals("Special Permissions")) {
                                    dgRow.Cells[1].Value = true;
                                    continue;
                                }
                                if (AllowedPermissions.Contains(dgRow.Cells[0].Value.ToString())) {
                                    dgRow.Cells[1].Value = true;
                                }
                                else
                                    dgRow.Cells[1].Value = false;

                                if (DeniedPermissions.Contains(dgRow.Cells[0].Value.ToString())) {
                                    dgRow.Cells[2].Value = true;
                                }
                                else
                                    dgRow.Cells[2].Value = false;
                            }

                        }
                    }
                }
            }
        }

        private void DgPermissions_CellMouseUp(object sender, DataGridViewCellMouseEventArgs e)
        {
            DgPermissions.EndEdit();
            DataGridViewRow dgRow = DgPermissions.Rows[e.RowIndex];
            if (dgRow != null)
            {
                ListViewItem lvItem = lvGroupOrUserNames.SelectedItems[0];
                List<LwAccessControlEntry> daclInfo = lvItem.Tag as List<LwAccessControlEntry>;
                string sobjectname = lvItem.Text.Substring(0, lvItem.Text.IndexOf('('));

                foreach (LwAccessControlEntry ace in daclInfo)
                {
                    long iAceMask = Convert.ToInt64(ace.AccessMask);

                    //Validation for the AceType = Allow
                    //Update the the AceType object with modified access modes
                    if (ace.AceType == 0) {
                        if (dgRow.Cells[1].Value.ToString().Equals("True"))
                        {
                            _securityDescriptor.GetIntAccessMaskFromStringAceMask(dgRow.Cells[0].Value.ToString(), ref iAceMask);
                        }
                    }

                    //Validation for the AceType = Deny
                    if (ace.AceType == 1) {
                        if (dgRow.Cells[2].Value.ToString().Equals("True"))
                        {
                            _securityDescriptor.GetIntAccessMaskFromStringAceMask(dgRow.Cells[0].Value.ToString(), ref iAceMask);
                        }
                    }

                    //Need to calculate the access mask for the Allow and deny permission sets.
                    ace.AccessMask = iAceMask.ToString();
                }
                if (_editedObjects.ContainsKey(sobjectname))
                    _editedObjects[sobjectname] = daclInfo;
                else
                    _editedObjects.Add(sobjectname, daclInfo);
            }
            CheckPemissions(sender, e);
        }

        private void DgPermissions_CellMouseDown(object sender, DataGridViewCellMouseEventArgs e)
        {
            DgPermissions.BeginEdit(false);
        }

        #endregion
    }
}
