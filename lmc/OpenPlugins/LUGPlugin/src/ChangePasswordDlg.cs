using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG
{
    public partial class ChangePasswordDlg : Form
    {
        private string _sUserName = "";
        private string _sHostName = "";
        private string _sNewPassword = "";
        private string _sConfirmPassword = "";

        public ChangePasswordDlg()
        {
            InitializeComponent();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            string errorMessage = null;
            if (Hostinfo.ValidatePassword(_sNewPassword, _sConfirmPassword, out errorMessage))
            {
                uint result = (uint)LUGAPI.WinError.ERROR_SUCCESS;

                try
                {
                    result = LUGAPI.NetChangePassword(
                        _sHostName, _sUserName, _sNewPassword);
                }
                catch (Exception)
                {
                    result = (uint)LUGAPI.WinError.ERROR_EXCEPTION_IN_SERVICE;
                }

                if (result == (uint)LUGAPI.WinError.ERROR_SUCCESS)
                {
                    this.DialogResult = DialogResult.OK;
                    Close();
                    return;
                }

                errorMessage = "Win32 Error: ";
                errorMessage += Convert.ToString(Enum.Parse(typeof(LUGAPI.WinError), result.ToString()));
            }

            Logger.ShowUserError(errorMessage);
        }

        private void newPassword_TextChanged(object sender, EventArgs e)
        {
            _sNewPassword = newPassword.Text;
        }

        private void confirmPassword_TextChanged(object sender, EventArgs e)
        {
            _sConfirmPassword = confirmPassword.Text;
        }

        public string UserName
        {
            get
            {
                return _sUserName;
            }
            set
            {
                _sUserName = value;
            }
        }

        public string HostName
        {
            get
            {
                return _sHostName;
            }
            set
            {
                _sHostName = value;
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }
    }
}
