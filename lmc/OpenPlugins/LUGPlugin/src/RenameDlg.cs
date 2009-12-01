using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.NETAPI;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.LUG
{
    public partial class RenameDlg : Form
    {
        private string _newName = "";
        private string _oldName = "";
        private string _hostName = "";
        private bool _isUser = true;

        public RenameDlg()
        {
            InitializeComponent();
        }

        private void tbNewName_TextChanged(object sender, EventArgs e)
        {
            _newName = tbNewName.Text;

            btnOK.Enabled = false;
            if (_newName.Length != 0)
            {
                btnOK.Enabled = true;
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            uint result = (uint)LUGAPI.WinError.ERROR_SUCCESS;

            if(_isUser)
            {
                result = LUGAPI.NetRenameUser(_hostName, _oldName, _newName);
            }
            else
            {
                result = LUGAPI.NetRenameGroup(_hostName, _oldName, _newName);
            }

            if (result == (uint)LUGAPI.WinError.ERROR_SUCCESS)
            {
                this.DialogResult = DialogResult.OK;
                Close();
                return;
            }

            Logger.ShowUserError(ErrorCodes.WIN32String((int)result));
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            Close();
        }

        public string HostName
        {
            get
            {
                return _hostName;
            }
            set
            {
                _hostName = value;
            }
        }

        public string OldName
        {
            get
            {
                return _oldName;
            }
            set
            {
                _oldName = value;
                tbNewName.Text = _oldName;
                tbNewName.SelectAll();
            }
        }

        public bool IsUser
        {
            get
            {
                return _isUser;
            }
            set
            {
                _isUser = value;
            }
        }
    }
}
