using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public class FileBrowserPage : FilesDetailPage
    {

        public FileBrowserPage()
            : base(FileBrowserIPlugIn.PluginNodeType.SHARES)
        {
        }

        private void InitializeComponent()
        {
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            //
            // pnlHeader
            //
            this.pnlHeader.Location = new System.Drawing.Point(8, 8);
            this.pnlHeader.Size = new System.Drawing.Size(531, 59);
            //
            // FileSharesPage
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "FileSharesPage";
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }
    }
    public class FilesBrowserPluginPage : FilesDetailPage
    {
        public FilesBrowserPluginPage()
            : base(FileBrowserIPlugIn.PluginNodeType.UNDEFINED)
        {
        }
    }

    public interface IDirectoryPropertiesPage
    {
        void SetData(CredentialEntry ce, string sharename, object Object);
    }
}
