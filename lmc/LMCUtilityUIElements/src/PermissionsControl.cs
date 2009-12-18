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
    public partial class PermissionsControl : UserControl
    {
        #region Class Data

        private SecurityDescriptor _securityDescriptor = null;
        public string _ObjectPath = string.Empty;

        private Dictionary<string, object> _addedObjects = new Dictionary<string, object>();
        private Dictionary<string, object> _removedObjects = new Dictionary<string, object>();
        private Dictionary<string, object> _editedObjects = new Dictionary<string, object>();

        #endregion

        #region Constructors

        public PermissionsControl()
        {
            InitializeComponent();
        }

        public PermissionsControl(SecurityDescriptor securityDescriptor,
                                  string _objectPath)
            : this()
        {
            this._securityDescriptor = securityDescriptor;
            this._ObjectPath = _objectPath;
        }

        #endregion

        #region Event handlers

        private void PermissionsControl_Load(object sender, EventArgs e)
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

        private void DgPermissions_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex == 0 && e.ColumnIndex != 0)
            {
                DataGridViewRow Row = DgPermissions.Rows[e.RowIndex];
                if (Row.Cells[e.ColumnIndex].Value.ToString().Equals("True"))
                {
                    DataGridViewRowCollection dgRows = DgPermissions.Rows;
                    foreach (DataGridViewRow dgRow in dgRows)
                    {
                        dgRow.Cells[e.ColumnIndex].Value = true;
                        dgRow.Cells[e.ColumnIndex].ReadOnly = true;
                    }
                }
                else
                {
                    foreach (DataGridViewRow dgRow in DgPermissions.Rows)
                    {
                        dgRow.Cells[e.ColumnIndex].ReadOnly = false;
                    }
                }
            }
        }

        private void btnAdd_Click(object sender, EventArgs e)
        {
            string distinguishedName = string.Empty;
            string domainName = _securityDescriptor.GetDCInfo(null);
            distinguishedName = System.DirectoryServices.SDSUtils.DomainNameToDN(domainName);

            // show picker
            string sLdapPath = string.Format("LDAP://{0}/{1}", domainName, distinguishedName);
            string sProtocol;
            string sServer;
            string sCNs;
            string sDCs;

            System.DirectoryServices.SDSUtils.CrackPath(sLdapPath, out sProtocol, out sServer, out sCNs, out sDCs);
            System.DirectoryServices.Misc.DsPicker dsPickerDlg = new System.DirectoryServices.Misc.DsPicker();
            dsPickerDlg.SetData(System.DirectoryServices.Misc.DsPicker.DialogType.SELECT_USERS_OR_GROUPS,
                                sProtocol,
                                sServer,
                                sDCs,
                                true);
            if (dsPickerDlg.waitForm != null && dsPickerDlg.waitForm.bIsInterrupted)
            {
                return;
            }

            if (dsPickerDlg.ShowDialog(this) == DialogResult.OK)
            {
                if (dsPickerDlg.ADobjectsArray != null && dsPickerDlg.ADobjectsArray.Length != 0)
                {
                    foreach (System.DirectoryServices.Misc.ADObject ado in dsPickerDlg.ADobjectsArray)
                    {
                        byte[] sObjectSid = ado.de.Properties["objectSid"].Value as byte[];
                        string sAMAccountName = ado.de.Properties["sAMAccountName"].Value as string;

                        string sSID = _securityDescriptor.ConvetByteSidToStringSid(sObjectSid);

                        //Need to set the permission check list in the permission set
                        LwAccessControlEntry Ace = new LwAccessControlEntry();
                        Ace.SID = sSID;
                        Ace.Username = sAMAccountName;
                        Ace.AceType = 0;
                        Ace.AccessMask = LwAccessMask.ACCESS_MASK.Special_Permissions.ToString();

                        bool bIsEntryFound = false;
                        List<LwAccessControlEntry> acelist = null;

                        foreach (ListViewItem item in lvGroupOrUserNames.Items)
                        {
                            if (item.Text.Contains(sAMAccountName))
                            {
                                acelist = item.Tag as List<LwAccessControlEntry>;
                                foreach (LwAccessControlEntry aceEntry in acelist)
                                {
                                    if (aceEntry.AceType == 0) {
                                        aceEntry.AccessMask = Ace.AccessMask;
                                        item.Tag = acelist;
                                        item.Selected = true;
                                    }
                                }
                            }
                        }
                        if (!bIsEntryFound)
                        {
                            ListViewItem lvItem = new ListViewItem(sAMAccountName);
                            acelist = new List<LwAccessControlEntry>();
                            acelist.Add(Ace);
                            lvItem.Tag = acelist; //Need to initialize the DaclInfo for the object
                            lvGroupOrUserNames.Items.Add(lvItem);

                            lvGroupOrUserNames.Items[lvGroupOrUserNames.Items.Count - 1].Selected = true;
                        }
                        _addedObjects.Add(sAMAccountName, acelist);
                    }
                }
            }
        }

        private void btnRemove_Click(object sender, EventArgs e)
        {
            if (lvGroupOrUserNames.SelectedItems.Count == 0)
                return;

            ListViewItem lvItem = lvGroupOrUserNames.SelectedItems[0];
            List<LwAccessControlEntry> daclInfo = lvItem.Tag as List<LwAccessControlEntry>;

            if (daclInfo != null)
            {
                string name = lvItem.Text.Substring(0, lvItem.Text.IndexOf('('));

                _removedObjects.Add(name, daclInfo);

                if (_addedObjects.ContainsKey(name))
                    _addedObjects.Remove(name);

                if (_editedObjects.ContainsKey(name))
                    _editedObjects.Remove(name);

                lvGroupOrUserNames.SelectedItems[0].Remove();
            }
        }

        private void DgPermissions_CellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            DataGridViewRow dgRow = DgPermissions.Rows[e.RowIndex];
            if (dgRow != null)
            {
                ListViewItem lvItem = lvGroupOrUserNames.SelectedItems[0];
                List<LwAccessControlEntry> daclInfo = lvItem.Tag as List<LwAccessControlEntry>;
                string sobjectname = lvItem.Text.Substring(0, lvItem.Text.IndexOf('('));

                foreach (LwAccessControlEntry ace in daclInfo)
                {
                    long iAceMask = Convert.ToInt64(ace.AccessMask);
                    if (iAceMask < 0)
                        iAceMask = 0;

                    //Validation for the AceType = Allow
                    //Update the the AceType object with modified access modes
                    //AceFlags = 16 is the inherited permission
                    if (ace.AceType == 0 && ace.AceFlags != 16) {
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

                if (_addedObjects.ContainsKey(sobjectname))
                    _addedObjects[sobjectname] = daclInfo;
                else if (_editedObjects.ContainsKey(sobjectname))
                    _editedObjects[sobjectname] = daclInfo;
                else
                    _editedObjects.Add(sobjectname, daclInfo);
            }
        }

        #endregion

        #region Helper functions

        public bool OnApply()
        {
            if ((_securityDescriptor.EditAce(_editedObjects, _addedObjects, _removedObjects) != 0))
                return false;

            return true;
        }

        private void InitailizeData()
        {
            this.lblObjectName.Text = string.Format(lblObjectName.Text, _ObjectPath);

            List<string> possiblePermissions = new List<string>();

            if (_securityDescriptor != null)
            {
                if (_securityDescriptor.Descretionary_Access_Control_List != null)
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
                        lvGroupOrUserNames.Items[0].Selected = true;
                        lvGroupOrUserNames.SelectedIndexChanged += new EventHandler(lvGroupOrUserNames_SelectedIndexChanged);
                    }
                }
            }
        }

        #endregion

        #region Access Specifiers

        public SecurityDescriptor securityDescriptor
        {
            set
            {
                _securityDescriptor = value;
            }
            get
            {
                return _securityDescriptor;
            }
        }

        #endregion
    }
}
