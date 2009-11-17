using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.FileShareManager
{
    public class FileSharesPage : SharesPage
    {
        public FileSharesPage()
            : base(FileShareManagerIPlugIn.PluginNodeType.SHARES)
        {
        }

        private void InitializeComponent()
        {
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            //
            // FileSharesPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "FileSharesPage";
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }
    }
    public class FilesBrowserPluginPage : SharesPage
    {
        public FilesBrowserPluginPage()
            : base(FileShareManagerIPlugIn.PluginNodeType.UNDEFINED)
        {
        }
    }

    public interface IDirectoryPropertiesPage
    {
        void SetData(CredentialEntry ce, string sharename, object Object);
    }
}
