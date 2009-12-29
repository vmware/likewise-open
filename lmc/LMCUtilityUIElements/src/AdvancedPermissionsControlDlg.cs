using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Likewise.LMC.SecurityDesriptor;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class AdvancedPermissionsControlDlg : Form
    {
        #region Class Data

        private bool dataChanged = false;
        private string _objectPath = null;
        private SecurityDescriptor _securityDescriptor = null;

        private Dictionary<string, object> _addedObjects = new Dictionary<string, object>();
        private Dictionary<string, object> _removedObjects = new Dictionary<string, object>();
        private Dictionary<string, object> _editedObjects = new Dictionary<string, object>();

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

        public bool DataChanged
        {
            set
            {
                dataChanged = value;
                btnApply.Enabled = dataChanged;
            }
            get
            {
                return dataChanged;
            }
        }

        #endregion

        #endregion

        #region Constructors

        public AdvancedPermissionsControlDlg()
        {
            InitializeComponent();
        }

        public AdvancedPermissionsControlDlg(SecurityDescriptor securityDescriptor, string ObjectPath)
            : this()
        {
            _securityDescriptor = securityDescriptor;
            _objectPath = ObjectPath;
        }

        #endregion

        private void AdvancedPermissionsControlDlg_Load(object sender, EventArgs e)
        {
            InitializeData();
            this.Text = string.Format(Properties.Resources.AdvancedSecurityDialogText, _objectPath);
            tabControl_SelectedIndexChanged(sender, e);

            DataChanged = false;
        }

        private void tabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            Dictionary<string, List<LwAccessControlEntry>> SdDacls =
                                                _securityDescriptor.Descretionary_Access_Control_List as
                                                Dictionary<string, List<LwAccessControlEntry>>;

            if (SdDacls != null && SdDacls.Count != 0)
            {
                switch (tabControl.SelectedTab.Text)
                {
                    case "Permissions":
                        FillPermissionsPage(SdDacls);
                        break;

                    case "Auditing":
                        FillAuditPage(SdDacls);
                        break;

                    case "Owner":
                        FillOwnerPage();
                        break;

                    case "Effective Permissions":
                        FillEffectivePermissionsPage();
                        break;

                    default:
                        break;
                }
            }
        }

        private void btnPermissionsAdd_Click(object sender, EventArgs e)
        {
            object ADObjects = new object();
            ShowDsPickerDialog(ADObjects);

            System.DirectoryServices.Misc.ADObject[] ADobjectsArray = ADObjects as System.DirectoryServices.Misc.ADObject[];

            if (ADobjectsArray != null && ADobjectsArray.Length != 0)
            {
                foreach (System.DirectoryServices.Misc.ADObject ado in ADobjectsArray)
                {
                    string sAMAccountName = ado.de.Properties["sAMAccountName"].Value as string;
                    string UPN = ado.de.Properties["userPrincipalName"].Value as string;
                    byte[] sObjectSid = ado.de.Properties["objectSid"].Value as byte[];

                    string sSID = _securityDescriptor.ConvetByteSidToStringSid(sObjectSid);

                    PermissionEntry permissionEntryDlg = new PermissionEntry(_objectPath, lvPermissions.SelectedItems[0].Text);
                    permissionEntryDlg.InitializeData(null, _securityDescriptor);
                    if (permissionEntryDlg.ShowDialog(this) == DialogResult.OK && permissionEntryDlg.IsCommit)
                    {
                        Do_PermissionsEdit(permissionEntryDlg._daclInfo, sSID, UPN);
                        _addedObjects.Add(sAMAccountName, permissionEntryDlg._daclInfo);

                        DataChanged = true;
                    }
                }
            }
        }

        private void btnPermissionsEdit_Click(object sender, EventArgs e)
        {
            if (lvPermissions.SelectedItems.Count == 0)
                return;

            PermissionEntry permissionEntryDlg = new PermissionEntry(_objectPath, lvPermissions.SelectedItems[0].Text);
            permissionEntryDlg.InitializeData(lvPermissions.SelectedItems[0].Tag as List<LwAccessControlEntry>, _securityDescriptor);
            if (permissionEntryDlg.ShowDialog(this) == DialogResult.OK && permissionEntryDlg.IsCommit)
            {
                if (permissionEntryDlg._daclInfo != null)
                {
                    ListViewItem lvItem = lvPermissions.SelectedItems[0];
                    string name = lvItem.SubItems[1].Text.Substring(0, lvItem.SubItems[1].Text.IndexOf('('));

                    Do_PermissionsEdit(permissionEntryDlg._daclInfo, null, null);

                    if (_addedObjects.ContainsKey(name))
                        _addedObjects.Remove(name);

                    else if (_editedObjects.ContainsKey(name))
                        _editedObjects[name] = permissionEntryDlg._daclInfo;
                    else
                        _editedObjects.Add(name, permissionEntryDlg._daclInfo);

                    DataChanged = true;
                }
            }
        }

        private void btnPermissionsRemove_Click(object sender, EventArgs e)
        {
            if (lvPermissions.SelectedItems.Count == 0)
                return;

            ListViewItem lvItem = lvPermissions.SelectedItems[0];
            List<LwAccessControlEntry> daclInfo = lvItem.Tag as List<LwAccessControlEntry>;

            if (daclInfo != null)
            {
                string name = lvItem.SubItems[1].Text.Substring(0, lvItem.SubItems[1].Text.IndexOf('('));

                if (_removedObjects.ContainsKey(name))
                    _removedObjects[name] = daclInfo;
                else
                    _removedObjects.Add(name, daclInfo);

                if (_addedObjects.ContainsKey(name))
                    _addedObjects.Remove(name);

                if (_editedObjects.ContainsKey(name))
                    _editedObjects.Remove(name);

                lvPermissions.SelectedItems[0].Remove();

                DataChanged = true;
            }
        }

        private void lvPermissions_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lvPermissions.SelectedItems.Count == 0)
                return;

            btnPermissionsEdit.Enabled = btnPermissionsRemove.Enabled = lvPermissions.SelectedItems.Count != 0;
        }

        private void btnEffectivePermissions_Click(object sender, EventArgs e)
        {
            object ADObjects = new object();
            ShowDsPickerDialog(ADObjects);

            System.DirectoryServices.Misc.ADObject[] ADobjectsArray = ADObjects as System.DirectoryServices.Misc.ADObject[];

            if (ADobjectsArray != null && ADobjectsArray.Length != 0)
            {
                foreach (System.DirectoryServices.Misc.ADObject ado in ADobjectsArray)
                {
                    string sAMAccountName = ado.de.Properties["sAMAccountName"].Value as string;
                    string UPN = ado.de.Properties["userPrincipalName"].Value as string;

                    lblUserorGroup.Text = UPN;
                }
            }
        }

        private void btnApply_Click(object sender, EventArgs e)
        {
            if (CheckForDeniedPermissions() &&  DataChanged)
            {
                DialogResult Dlg = MessageBox.Show(this, Properties.Resources.PermissionsWarningMsg,
                    Properties.Resources.Console_Caption, MessageBoxButtons.OKCancel);
                if (Dlg == DialogResult.OK)
                {
                    //Edit the DACL entries filled by the Permissions tab page
                    _securityDescriptor.EditAce(_editedObjects, _addedObjects, _removedObjects, ref _securityDescriptor.pSecurityDescriptorOut);
                    if (_securityDescriptor.pSecurityDescriptorOut == IntPtr.Zero)
                    {
                        MessageBox.Show(this, Properties.Resources.EditPermissionErrorDisplay, Properties.Resources.PermissionsWarningMsg,
                                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }

                    //TODO: Edit the SACL entries filled by the Audit tab page

                    dataChanged = false;
                }
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            if (!DataChanged)
                btnApply_Click(sender, e);

            this.DialogResult = DialogResult.OK;
            Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void btnAuditAdd_Click(object sender, EventArgs e)
        {

        }

        private void btnAuditEdit_Click(object sender, EventArgs e)
        {

        }

        private void btnAuditRemove_Click(object sender, EventArgs e)
        {

        }

        private void InitializeData()
        {
            this.Text = string.Format(Properties.Resources.AdvancedSecurityDialogText, _objectPath);
            tabControl.SelectedIndex = 0;
        }

        private void FillPermissionsPage(Dictionary<string, List<LwAccessControlEntry>> SdDacls)
        {
            if (lvPermissions.Items.Count != 0)
                return;

            foreach (string key in SdDacls.Keys)
            {
                List<LwAccessControlEntry> daclInfo = SdDacls[key];

                foreach (LwAccessControlEntry ace in daclInfo)
                {
                    string sAccessString = _securityDescriptor.GetKeyPermissionName(ace.AccessMask);
                    if (!String.IsNullOrEmpty(sAccessString))
                    {
                        string[] strItems = new string[]{
                                                Convert.ToInt32(ace.AceType) == 0 ? "Allow" : "Deny",
                                                ace.Username,
                                                sAccessString,
                                                "",
                                                Properties.Resources.FolderApplyToText
                                                };
                        ListViewItem lvItem = new ListViewItem(strItems);
                        lvItem.Tag = daclInfo;
                        lvPermissions.Items.Add(lvItem);
                    }
                }
            }
        }

        private void FillAuditPage(Dictionary<string, List<LwAccessControlEntry>> SdDacls)
        {
            //TO DO: Need to fill the SACL object in the Security descriptor to read the Audit permissions about the objects
        }

        private void FillOwnerPage()
        {
            if (LWlvOwner.Items.Count != 0)
                return;

            string sUsername = _securityDescriptor.CovertStringSidToLookupName(securityDescriptor.Owner);
            this.lblUsername.Text = sUsername;

            ListViewItem lvItem = new ListViewItem(new string[] { sUsername });
            lvItem.Tag = securityDescriptor.Owner;
            LWlvOwner.Items.Add(lvItem);

            string sGroupname = _securityDescriptor.CovertStringSidToLookupName(securityDescriptor.PrimaryGroupID);
            lvItem = new ListViewItem(new string[] { sGroupname });
            lvItem.Tag = securityDescriptor.PrimaryGroupID;
            LWlvOwner.Items.Add(lvItem);
        }

        private void FillEffectivePermissionsPage()
        {
            if (checkedListviewPermissions.Items.Count != 0)
                return;

            switch (SecurityDescriptor.objectType)
            {
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_FILE_OBJECT:
                    foreach (string permission in AdvancedPermissions.DirectoryPermissionSet)
                    {
                        checkedListviewPermissions.Items.Add(permission, false);
                    }
                    break;

                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_KEY:
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_WOW64_32KEY:
                    foreach (string permission in AdvancedPermissions.RegistryPermissionSet)
                    {
                        checkedListviewPermissions.Items.Add(permission, false);
                    }
                    break;

                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT:
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT_ALL:
                    foreach (string permission in AdvancedPermissions.AdsPermissionSet)
                    {
                        checkedListviewPermissions.Items.Add(permission, false);
                    }
                    break;

                default:
                    break;
            }
        }

        private void ShowDsPickerDialog(object ADObjects)
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
                if (dsPickerDlg.ADobjectsArray != null && dsPickerDlg.ADobjectsArray.Length != 0) {
                    ADObjects = dsPickerDlg.ADobjectsArray;
                }
            }
        }

        private void Do_PermissionsEdit(List<LwAccessControlEntry> daclInfo,
                     string ObjectSid, string Objectname)
        {
            if (daclInfo != null)
            {
                bool bIsEntryFound = false;
                List<LwAccessControlEntry> acelist = null;

                foreach (LwAccessControlEntry Ace in daclInfo)
                {
                    foreach (ListViewItem item in lvPermissions.Items)
                    {
                        if (item.SubItems[1].Text.Contains(Objectname))
                        {
                            acelist = item.Tag as List<LwAccessControlEntry>;
                            foreach (LwAccessControlEntry aceEntry in acelist)
                            {
                                if (aceEntry.AceType == Ace.AceType)
                                {
                                    aceEntry.AccessMask = Ace.AccessMask;
                                    item.Tag = acelist;
                                    item.Selected = true;
                                }
                            }
                            bIsEntryFound = true;
                            break;
                        }
                    }
                    if (!bIsEntryFound)
                    {
                        Ace.SID = ObjectSid;
                        Ace.Username = Objectname;
                        Ace.AceFlags = 0;
                        Ace.AceSize = 20;

                        string[] strItems = new string[]{
                                            Convert.ToInt32(Ace.AceType) == 0 ? "Allow" : "Deny",
                                            Ace.Username,
                                            _securityDescriptor.GetKeyPermissionName(Ace.AccessMask),
                                            "",
                                            Properties.Resources.FolderApplyToText
                                            };

                        ListViewItem lvItem = new ListViewItem(strItems);
                        lvItem.Tag = daclInfo;
                        lvPermissions.Items.Add(lvItem);
                    }
                }
            }
        }

        private bool CheckForDeniedPermissions()
        {
            foreach (ListViewItem lvItem in lvPermissions.Items)
            {
                if (lvItem.SubItems[0].Text.Equals("Deny"))
                    return true;
            }

            return false;
        }
    }
}