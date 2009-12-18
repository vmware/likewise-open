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
    public partial class PermissionEntry : Form
    {
        #region Class Data

        public bool IsCommit = false;
        public Dictionary<int, string> AceTypes = new Dictionary<int, string>();
        public List<LwAccessControlEntry> _daclInfo = null;
        public SecurityDescriptor _securityDescriptor = null;

        #endregion

        #region Constructors

        public PermissionEntry()
        {
            InitializeComponent();
        }

        public PermissionEntry(string sObjectName,
                               string sOwner)
        {
            this.Text = string.Format(Properties.Resources.PermissionEntryText, sObjectName);
            this.txtObjectName.Text = sOwner;

            comboApplyTo.SelectedIndex = 0;
        }

        #endregion

        #region Helper functions

        /// <summary>
        /// Initialize the DACL info to null in add mode and to object in edit mode
        /// </summary>
        /// <param name="daclInfo"></param>
        /// <param name="securityDescriptor"></param>
        public void InitializeData(List<LwAccessControlEntry> daclInfo, SecurityDescriptor securityDescriptor)
        {
           this._daclInfo = daclInfo;
           this._securityDescriptor = securityDescriptor;
           FillRowPermissions();
        }

        private void FillRowPermissions()
        {
            string[] possiblePermissions = null;

            //Read all possibel permissions based on type of object
            switch (SecurityDescriptor.objectType)
            {
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_FILE_OBJECT:
                    possiblePermissions = AdvancedPermissions.DirectoryPermissionSet;
                    break;

                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_KEY:
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_WOW64_32KEY:
                    possiblePermissions = AdvancedPermissions.RegistryPermissionSet;
                    break;

                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT:
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT_ALL:
                    possiblePermissions = AdvancedPermissions.AdsPermissionSet;
                    break;

                default:
                    break;
            }
            //Read all allowed/Denied permissions from the dacl list
            List<string> AllowedPermissions = new List<string>();
            List<string> DeniedPermissions = new List<string>();
            if (_daclInfo != null && _daclInfo.Count != 0)
            {
                foreach (LwAccessControlEntry ace in _daclInfo)
                {
                    if (ace.AceType == 0) {
                        AllowedPermissions = _securityDescriptor.GetUserOrGroupPermissions(ace.AccessMask);
                    }
                    else if (ace.AceType == 1) {
                        DeniedPermissions = _securityDescriptor.GetUserOrGroupPermissions(ace.AccessMask);
                    }
                }
            }

            //Check for the acetype for each permission in a set
            foreach (string permission in possiblePermissions)
            {
                bool IsAllowed = false;
                bool IsDenied = false;

                if (AllowedPermissions.Contains(permission)) {
                    IsAllowed = true;
                }
                if (DeniedPermissions.Contains(permission)) {
                    IsDenied = true;
                }

                //Need to set the the values depends on the data from the security descriptor
                dgPermissions.Rows.Add(new object[]{
                                                permission,
                                                IsAllowed,
                                                IsDenied});
            }
        }

        #endregion

        private void btnOK_Click(object sender, EventArgs e)
        {
            //Need to check with the existing AceMask and then add a
            //new entry to the Advanced permissions dialog if it varies

             //Check for the edit mode or add mode. Since in the add mode user should send the null for daclInfo object
            if (_daclInfo == null)
            {
                _daclInfo = new List<LwAccessControlEntry>();

                LwAccessControlEntry ace = new LwAccessControlEntry();
                ace.AccessMask = "-1";
                ace.AceType = 0;
                _daclInfo.Add(ace);

                ace = new LwAccessControlEntry();
                ace.AccessMask = "-1";
                ace.AceType = 1;
                _daclInfo.Add(ace);
            }
            //Need to calculate the access mask for the Allow and deny permission sets.
            foreach (LwAccessControlEntry ace in _daclInfo)
            {
                long iAceMask = Convert.ToInt64(ace.AccessMask);
                //Validation for the AceType = Allow
                //Update the the AceType object with modified access modes
                if (ace.AceType == 0)
                {
                    foreach (DataGridViewRow dgRow in dgPermissions.Rows)
                    {
                        if (dgRow.Cells[1].Value.ToString().Equals("True"))
                            _securityDescriptor.GetIntAccessMaskFromStringAceMask(dgRow.Cells[0].Value.ToString(), ref iAceMask);
                    }
                }

                //Validation for the AceType = Deny
                if (ace.AceType == 1)
                {
                    foreach (DataGridViewRow dgRow in dgPermissions.Rows)
                    {
                        if (dgRow.Cells[2].Value.ToString().Equals("True"))
                            _securityDescriptor.GetIntAccessMaskFromStringAceMask(dgRow.Cells[0].Value.ToString(), ref iAceMask);
                    }
                }
                //Check for the edit values
                if (Convert.ToInt32(ace.AccessMask) != Convert.ToInt32(iAceMask))
                {
                    ace.AccessMask = iAceMask.ToString();
                    IsCommit = true;
                }
            }

            this.DialogResult = DialogResult.OK;
            Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            Close();
        }
    }
}