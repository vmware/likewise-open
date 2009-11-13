using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public class FileBrowserPage : FilesDetailPage
    {

        public FileBrowserPage()
            : base()
        {
        }

        private void InitializeComponent()
        {
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            //
            // pnlActions
            //
            this.pnlActions.Size = new System.Drawing.Size(131, 338);
            //
            // pnlHeader
            //
            this.pnlHeader.Size = new System.Drawing.Size(687, 59);
            //
            // FileBrowserPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "FileBrowserPage";
            this.Size = new System.Drawing.Size(687, 397);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }
    }

    public class FilesBrowserPluginPage : FilesDetailPage
    {
        public FilesBrowserPluginPage()
            : base()
        {
        }
    }

    public interface IDirectoryPropertiesPage
    {
        void SetData(CredentialEntry ce, string sharename, object Object);
    }
}
