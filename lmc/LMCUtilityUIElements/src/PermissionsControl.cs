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
            :this()
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
            if (dsPickerDlg.waitForm != null && dsPickerDlg.waitForm.bIsInterrupted) {
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
                        Dictionary<string, string> daclInfo = new Dictionary<string, string>();
                        daclInfo.Add("Sid", sSID);
                        daclInfo.Add("Username", sAMAccountName);
                        daclInfo.Add("AceType", "0");
                        daclInfo.Add("AccessMask", SecurityDescriptorApi.ACCESS_MASK.READ_CONTROL.ToString());

                        ListViewItem lvItem = new ListViewItem();
                        lvItem.Tag = daclInfo; //Need to initialize the DaclInfo for the object
                        lvGroupOrUserNames.Items.Add(lvItem);

                        _addedObjects.Add(sAMAccountName, daclInfo);
                    }

                    lvGroupOrUserNames.Items[lvGroupOrUserNames.Items.Count - 1].Selected = true;
                }
            }
        }

        private void btnRemove_Click(object sender, EventArgs e)
        {
            if (lvGroupOrUserNames.SelectedItems.Count == 0)
                return;

            ListViewItem lvItem = lvGroupOrUserNames.SelectedItems[0];
            Dictionary<string, string> daclInfo = lvItem.Tag as Dictionary<string, string>;

            if (daclInfo != null)
            {
                _removedObjects.Add(lvItem.Text, daclInfo);
                lvGroupOrUserNames.SelectedItems[0].Remove();
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
                else if (dgRow.Cells[1].Value.ToString().Equals("False"))
                    iAceType -= 1;

                daclInfo["AceType"] = iAceType.ToString();

                if (_addedObjects.ContainsKey(lvItem.Text))
                    _addedObjects.Remove(lvItem.Text);
                _editedObjects.Add(lvItem.Text, daclInfo);
            }
        }

        #endregion

        #region Helper functions

        private void InitailizeData()
        {
            this.lblObjectName.Text = string.Format(lblObjectName.Text, _ObjectPath);

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
                        foreach (string permission in possiblePermissions) {
                            DgPermissions.Rows.Add(new object[] { permission, false, false });
                        }

                        lvGroupOrUserNames.Items[0].Selected = true;
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
