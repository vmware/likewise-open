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

        #endregion

        #region Constructors

        public PermissionsControl()
        {
            InitializeComponent();
        }

        public PermissionsControl(SecurityDescriptor securityDescriptor)
            :this()
        {
            this._securityDescriptor = securityDescriptor;
        }

        #endregion

        #region Event handlers

        private void PermissionsControl_Load(object sender, EventArgs e)
        {

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
