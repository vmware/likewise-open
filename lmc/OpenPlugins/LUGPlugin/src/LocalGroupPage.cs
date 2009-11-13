using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG.src
{
    public class LocalGroupPage : LUGPage
    {
        #region Constructor
        public LocalGroupPage()
            : base(LUGAPI.LUGType.Group)
        {

        }
        #endregion

        private void InitializeComponent()
        {
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            //
            // LocalGroupPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "LocalGroupPage";
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }
    }
}
