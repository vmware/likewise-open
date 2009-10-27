/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

using System;
using System.Drawing;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LMConsoleUtils;
using System.Windows.Forms;
using System.Xml;

namespace Likewise.LMC.Plugins.Root
{
public class RootPlugin: IPlugIn
{
    #region Class Data
    private IPlugInContainer _container;
    //private Hostinfo         _hn = null;
    private IContext _ctx = null;
    private LACTreeNode _rootNode = null;
    #endregion
    
    #region IPlugIn Members    
    
    string IPlugIn.GetName()
    {
        return "Console";
    }

    string IPlugIn.GetDescription()
    {
        return "Internal plugin - should never be visible to the user!";
    }

    public string GetPluginDllName()
    {
#if !QUARTZ
        return "Likewise.LMC.Plugins.RootPlugin.dll";
#else 
        return "Likewise.LMC.Plugins.RootPlugin.unix.dll";
#endif
    }

    public IContextType GetContextType()
    {
        return IContextType.Rootinfo;
    }

    public void SerializePluginInfo(LACTreeNode pluginNode, ref int Id, out XmlElement viewElement, XmlElement ViewsNode, TreeNode SelectedNode)
    {
        // To be implemented
        viewElement = null;
        return;
    }

    public void DeserializePluginInfo(XmlNode node, ref LACTreeNode pluginnode, string nodepath)
    {
        // To be implemented
        return;
    }
    
    void IPlugIn.Initialize(IPlugInContainer container)
    {
        _container = container;
    }

    void IPlugIn.SetContext(IContext ctx)
    {
        _ctx = ctx;
    }
    
    public IContext GetContext()
    {
        return _ctx;
    }
    
    public LACTreeNode GetPlugInNode()
    {
        return GetRootNode();
    }
    
    public void EnumChildren(LACTreeNode parentNode)
    {
        return;
    }
    
    public void SetCursor(System.Windows.Forms.Cursor cursor)
    {
        if (_container != null)
        {
            _container.SetCursor(cursor);
        }
    }
    
    public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
    {
        return null;
    }
    
    public void SetSingleSignOn(bool useSingleSignOn)
    {
        // do nothing
    }

    public void AddExtPlugin(IPlugIn extPlugin)
    {
        // do nothing
    }
    
    #endregion
    
    #region Private helper functions
    
    private LACTreeNode GetRootNode()
    {
        if (_rootNode == null)
        {
            
            //this bitmap never gets used for display, but is required by the LACTreeNode constructor.
            Bitmap bmp = new Bitmap(32, 32);
            
            _rootNode = new LACTreeNode("Console", bmp, typeof(ConsolePage), this);
            
            _rootNode.IsPluginNode = true;
        }
        
        return _rootNode;
    }
    
    #endregion
    
}
}
