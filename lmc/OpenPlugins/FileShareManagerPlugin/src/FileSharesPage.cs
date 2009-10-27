using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC.Plugins.FileShareManager
{
    public class FileSharesPage : SharesPage
    {
        public FileSharesPage()
            : base(FileShareManagerIPlugIn.PluginNodeType.SHARES)
        {
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
