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

            List<object> possiblePermissions = new List<object>();

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

                            ListViewItem lvItem = new ListViewItem(new string[] { SdDacls[key]["Username"] });
                            lvItem.Tag = daclInfo;
                            lvGroupOrUserNames.Items.Add(lvItem);

                            List<string> permissions = _securityDescriptor.GetUserOrGroupPermissions(
                                                       _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, "AceMask") as string);

                            foreach (string per in permissions)
                            {
                                if (!possiblePermissions.Contains(per))
                                    possiblePermissions.Add(per);
                            }
                        }
                    }

                    if (possiblePermissions.Count != 0)
                    {
                        foreach (string permission in possiblePermissions)
                        {
                            DgPermissions.Rows.Add(new object[] { permission, false, false });
                        }

                        lvGroupOrUserNames.Items[0].Selected = true;
                        lvGroupOrUserNames.SelectedIndexChanged += new EventHandler(lvGroupOrUserNames_SelectedIndexChanged);
                    }
                }
            }
        }

        public bool OnApply()
        {
            if (_securityDescriptor.EditAce(_editedObjects, null, null) != 0)
                return false;

            return true;
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
                        Dictionary<string, string> daclInfo = lvItem.Tag as Dictionary<string, string>;
                        if (daclInfo != null)
                        {
                            List<string> permissions = _securityDescriptor.GetUserOrGroupPermissions(
                                                       _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, "AceMask") as string);

                            foreach (string permission in permissions)
                            {
                                DataGridViewRowCollection dgRows = DgPermissions.Rows;
                                foreach (DataGridViewRow dgRow in dgRows)
                                {
                                    if (dgRow.Cells[0].Value.ToString().Equals(permission))
                                    {
                                        dgRow.Cells[1].Value = _securityDescriptor.CheckAccessMaskExists(daclInfo["AceType"], 0);
                                        dgRow.Cells[2].Value = _securityDescriptor.CheckAccessMaskExists(daclInfo["AceType"], 1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        private void DgPermissions_CellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            DataGridViewRow dgRow = DgPermissions.Rows[e.RowIndex];
            if (dgRow != null)
            {
                uint iAceType = 0;
                uint aceType = 0;

                ListViewItem lvItem = lvGroupOrUserNames.SelectedItems[0];
                Dictionary<string, string> daclInfo = lvItem.Tag as Dictionary<string, string>;
                string sobjectname = lvItem.Text.Substring(0, lvItem.Text.IndexOf('('));

                iAceType = Convert.ToUInt32(daclInfo["AceType"]);

                //Update the the AceType object with modified access modes
                if (dgRow.Cells[1].Value.ToString().Equals("True"))
                {
                    aceType = 0;
                    _securityDescriptor.GetAceType(aceType, ref iAceType);
                }
                else if (dgRow.Cells[1].Value.ToString().Equals("False"))
                    iAceType -= 0;

                if (dgRow.Cells[2].Value.ToString().Equals("True"))
                {
                    aceType = 1;
                    _securityDescriptor.GetAceType(aceType, ref iAceType);
                }
                else if (dgRow.Cells[2].Value.ToString().Equals("False"))
                    iAceType -= 1;

                daclInfo["AceType"] = iAceType.ToString();

                if (_editedObjects.ContainsKey(sobjectname))
                    _editedObjects[sobjectname] = daclInfo;
                else
                    _editedObjects.Add(sobjectname, daclInfo);
            }
        }

        #endregion
    }
}
