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

        public Dictionary<string, string> daclInfo = null;
        public bool IsCommit = false;
        public Dictionary<int, string> AceTypes = new Dictionary<int, string>();

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

        public void InitializeData(Dictionary<string, string> _daclInfo)
        {
           this.daclInfo = _daclInfo;
           FillRowPermissions();
        }

        private void FillRowPermissions()
        {
            switch (SecurityDescriptor.objectType)
            {
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_FILE_OBJECT:
                    foreach (string permission in AdvancedPermissions.DirectoryPermissionSet)
                    {
                        dgPermissions.Rows.Add(new object[]{
                                                permission,
                                                true, //Need to set the value depends on the
                                                false});
                    }
                    break;

                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_KEY:
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_REGISTRY_WOW64_32KEY:
                    foreach (string permission in AdvancedPermissions.RegistryPermissionSet)
                    {
                        dgPermissions.Rows.Add(new object[]{
                                                permission,
                                                true,
                                                false});
                    }
                    break;

                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT:
                case SecurityDescriptorApi.SE_OBJECT_TYPE.SE_DS_OBJECT_ALL:
                    foreach (string permission in AdvancedPermissions.AdsPermissionSet)
                    {
                        dgPermissions.Rows.Add(new object[]{
                                                permission,
                                                true,
                                                false});
                    }
                    break;

                default:
                    break;
            }
        }

        #endregion

        private void btnOK_Click(object sender, EventArgs e)
        {
            //TODO : Need to check with the existing AceMask and then add a
            //new entry to the Advanced permissions dialog if it varies
            int iOldAceMask = Convert.ToInt32(daclInfo["AceMask"]);
            int iNewAceMask = -1;

            foreach (DataGridViewRow dgRow in dgPermissions.Rows)
            {
                //Need to calculate the each rows AceMask value
                if (dgRow.Cells[1].Value.ToString().Equals("True"))
                { }

                //if (dgRow.Cells[2].Value.ToString().Equals("True"))
                //{
                //    if(AceTypes.ContainsKey(1))
                //        AceTypes[1] =
                //}
            }

            if (iOldAceMask != iNewAceMask)
            {
                daclInfo["AceMask"] = iNewAceMask.ToString();
                IsCommit = true;
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