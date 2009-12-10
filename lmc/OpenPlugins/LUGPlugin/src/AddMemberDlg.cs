using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG
{
    public partial class AddMemberDlg : Form
    {
        private string _member = "";
        private string _hostName = "";
        private bool _listUsers = true;

        public AddMemberDlg()
        {
            InitializeComponent();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            Close();
        }

        private void tbMember_TextChanged(object sender, EventArgs e)
        {
            _member = tbMember.Text;

            btnOK.Enabled = false;
            if (_member.Length != 0)
            {
                btnOK.Enabled = true;
            }
        }

        private void lvMembers_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lvMembers.SelectedItems.Count != 1)
            {
                return;
            }

            ListViewItem item = lvMembers.SelectedItems[0];

            tbMember.Text = item.SubItems[0].Text;
        }

        private void PopulateListView(LUGAPI.LUGEnumStatus enumStatus)
        {
            if (enumStatus.entries != null && enumStatus.entries.Count > 0)
            {
                ListViewItem [] lvArr = new ListViewItem[Convert.ToInt32(enumStatus.entries.Count)];

                for (int i = 0; i < enumStatus.entries.Count; i++)
                {
                    if (enumStatus.type == LUGAPI.LUGType.User)
                    {
                        lvArr[i] = new ListViewItem(enumStatus.entries[i][2]);
                    }
                    else
                    {
                        lvArr[i] = new ListViewItem(enumStatus.entries[i][1]);
                    }
                }

                this.lvMembers.Items.AddRange(lvArr);

                if (enumStatus.moreEntries)
                {
                    try
                    {
                        if (enumStatus.type == LUGAPI.LUGType.User)
                        {
                            LUGAPI.NetEnumUsers(
                                _hostName,
                                enumStatus.resumeHandle,
                                out enumStatus);
                            PopulateListView(enumStatus);
                        }
                        else if (enumStatus.type == LUGAPI.LUGType.Group)
                        {
                            LUGAPI.NetEnumGroups(
                                _hostName,
                                enumStatus.resumeHandle,
                                out enumStatus);
                            PopulateListView(enumStatus);
                        }
                    }
                    catch (Exception)
                    {
                    }
                }
            }
        }

        public void PopulatePage(string hostName, bool bListUsers)
        {
            LUGAPI.LUGEnumStatus enumStatus;
            _hostName = hostName;

            if (bListUsers)
            {
                LUGAPI.NetEnumUsers(_hostName, 0, out enumStatus);
            }
            else
            {
                LUGAPI.NetEnumGroups(_hostName, 0, out enumStatus);
            }

            lvMembers.Clear();

            lvMembers.Columns.Add("", 20, HorizontalAlignment.Left);

            PopulateListView(enumStatus);

            lvMembers.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
        }

        public string Member
        {
            get
            {
                return _member;
            }
            set
            {
                _member = value;
            }
        }

        public string Label
        {
            get
            {
                return label1.Text;
            }
            set
            {
                label1.Text = value;
            }
        }
    }
}