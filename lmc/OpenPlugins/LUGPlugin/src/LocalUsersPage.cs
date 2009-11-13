using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG.src
{
    public class LocalUserPage : LUGPage
    {
        #region Constructor
        public LocalUserPage()
            : base(LUGAPI.LUGType.User)
        {

        }
        #endregion

        private void InitializeComponent()
        {
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            //
            // LocalUserPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "LocalUserPage";
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }
    }
}
