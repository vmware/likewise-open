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

        private SecurityDescriptor _securityDescriptor = null;
        private string _objectPath = null;

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
        }

        private void tabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            Dictionary<string, Dictionary<string, string>> SdDacls =
                                                _securityDescriptor.Descretionary_Access_Control_List as
                                                Dictionary<string, Dictionary<string, string>>;

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
                    byte[] sObjectSid = ado.de.Properties["objectSid"].Value as byte[];
                    string sAMAccountName = ado.de.Properties["sAMAccountName"].Value as string;

                    string sSID = _securityDescriptor.ConvetByteSidToStringSid(sObjectSid);

                    //Need to set the permission check list in the permission set
                    Dictionary<string, string> daclInfo = new Dictionary<string, string>();
                    daclInfo.Add("Sid", sSID);
                    daclInfo.Add("Username", sAMAccountName);
                    daclInfo.Add("AceType", "-1");

                    PermissionEntry permissionEntryDlg = new PermissionEntry(_objectPath, lvPermissions.SelectedItems[0].Text);
                    permissionEntryDlg.InitializeData(daclInfo);
                    if (permissionEntryDlg.ShowDialog(this) == DialogResult.OK && permissionEntryDlg.IsCommit)
                    {
                        //TODO:
                    }


                    daclInfo.Add("AccessMask", SecurityDescriptorApi.ACCESS_MASK.Special_Permissions.ToString());

                    string[] strItems = new string[] { "Allow", sAMAccountName };

                    ListViewItem lvItem = new ListViewItem();
                    lvItem.Tag = daclInfo; //Need to initialize the DaclInfo for the object
                    lvPermissions.Items.Add(lvItem);

                    _addedObjects.Add(sAMAccountName, daclInfo);
                }
            }
        }

        private void btnPermissionsEdit_Click(object sender, EventArgs e)
        {
            PermissionEntry permissionEntryDlg = new PermissionEntry(_objectPath, lvPermissions.SelectedItems[0].Text);
            permissionEntryDlg.InitializeData(lvPermissions.SelectedItems[0].Tag as Dictionary<string, string>);
            if (permissionEntryDlg.ShowDialog(this) == DialogResult.OK && permissionEntryDlg.IsCommit)
            {

            }
        }

        private void InitializeData()
        {
            this.Text = string.Format(Properties.Resources.AdvancedSecurityDialogText, _objectPath);

            if (_securityDescriptor != null)
            {
                if (_securityDescriptor.Descretionary_Access_Control_List != null)
                {
                    tabControl.SelectedIndex = 0;
                }
            }
        }

        private void FillPermissionsPage(Dictionary<string, Dictionary<string, string>> SdDacls)
        {
            foreach (string key in SdDacls.Keys)
            {
                Dictionary<string, string> daclInfo = SdDacls[key];

                string[] strItems = new string[]{
                                                Convert.ToInt32(daclInfo["AceType"]) == 0 ? "Allow" : "Deny",
                                                SdDacls[key]["Username"],
                                                _securityDescriptor.GetKeyPermissionName(daclInfo["AceMask"]),
                                                "",
                                                Properties.Resources.FolderApplyToText
                                                };

                ListViewItem lvItem = new ListViewItem(strItems);
                lvItem.Tag = daclInfo;
                lvPermissions.Items.Add(lvItem);
            }
        }

        private void FillAuditPage(Dictionary<string, Dictionary<string, string>> SdDacls)
        {
            //TO DO: Need to fill the SACL object in the Security descriptor to read the Audit information about the objects
        }

        private void FillOwnerPage()
        {
            ListViewItem lvItem = new ListViewItem(new string[] { securityDescriptor.Owner });
            lvItem.Tag = securityDescriptor.Owner;
            LWlvOwner.Items.Add(lvItem);
        }

        private void FillEffectivePermissionsPage()
        {
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
    }
}