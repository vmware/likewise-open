using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.Plugins.LUG
{
    public partial class AddMemberDlg : Form
    {
        private string _member = "";

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
    }
}