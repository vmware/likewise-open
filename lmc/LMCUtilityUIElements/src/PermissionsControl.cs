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
        private string _ObjectPath = string.Empty;

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
                        Dictionary<string, string> daclInfo = lvItem.Tag as Dictionary<string, string>;
                        if (daclInfo != null)
                        {
                            foreach (DataGridViewRow dgRow in DgPermissions.Rows)
                            {
                                dgRow.Cells[1].Value = _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, dgRow.Cells[0].Value as string);
                                dgRow.Cells[2].Value = _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, dgRow.Cells[0].Value as string);
                            }
                        }
                    }
                }
            }
        }

        #endregion

        #region Helper functions

        private void InitailizeData()
        {
            this.lblObjectName.Text = string.Format(lblObjectName.Text, _ObjectPath);

            List<ListViewItem> lvItems = new List<ListViewItem>();

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
                            lvItems.Add(lvItem);

                            DataGridViewRow dgRow = new DataGridViewRow();
                            dgRow.CreateCells(DgPermissions);
                            dgRow.Cells[0].Value = _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, "AceMask");
                            dgRow.Cells[1].Value = _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, "AceType");
                            dgRow.Cells[2].Value = _securityDescriptor.GetUserOrGroupSecurityInfo(daclInfo, "AceType");
                            DgPermissions.Rows.Add(dgRow);
                        }
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
