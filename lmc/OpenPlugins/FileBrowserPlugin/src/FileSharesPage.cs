using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public class FileSharesPage : SharesPage
    {
        public FileSharesPage()
            : base(FileBrowserIPlugIn.PluginNodeType.SHARES)
        {
        }
    }
    public class FilesBrowserPluginPage : SharesPage
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
