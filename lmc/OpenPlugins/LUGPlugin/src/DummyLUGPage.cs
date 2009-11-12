using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG.src
{
    public class DummyLUGPage : LUGPage
    {
        #region Constructor
        public DummyLUGPage()
            : base(LUGAPI.LUGType.Dummy)
        {
        }
        #endregion

        private void InitializeComponent()
        {
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            //
            // DummyLUGPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "DummyLUGPage";
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }
    }
}
