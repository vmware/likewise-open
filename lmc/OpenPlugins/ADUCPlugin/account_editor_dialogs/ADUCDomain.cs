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
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.LDAP;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Plugins.ADUCPlugin.Properties;
using Likewise.LMC.LMConsoleUtils;
using System.Windows.Forms;
using System.DirectoryServices;
using System.Net;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public class ADUCDomain
{
    #region Class Data
    
    private Hostinfo _hn = null;
    private IPlugIn _plugin = null;
    private LACTreeNode _pluginNode = null;
    private DirectoryContext _adContext = null;
    private ADUCDirectoryNode _rootNode = null;
    private string _rootDN;
    private string _shortDomainName = "";
    
    private string _defaultUnixHomedir = "";
    private string _loginShell = "";
    private LDAPSchemaCache _schemaCache = null;
    
    private LWTreeView _lmctreeview = null;
    
    #endregion Class Data
    
    #region Helper Methods
    
    /// <summary>
    /// Method that is used to connect to multiple domains
    /// </summary>
    /// <returns></returns>
    public DirectoryContext ConnectToDomain(bool usingSimpleBind)
    {
        
        string[] rootDNcom = _hn.domainName.Split('.');
        
        string rootDN = "";
        foreach (string str in rootDNcom)
        {
            string temp = string.Concat("dc=", str, ",");
            rootDN = string.Concat(rootDN, temp);
        }
        
        rootDN = rootDN.Substring(0, rootDN.Length - 1);
        
        _rootDN = rootDN;
        
        if (DirectoryEntry.exisitngDirContext != null && DirectoryEntry.exisitngDirContext.Count > 0)
        {
            foreach (DirectoryContext dirContext in DirectoryEntry.exisitngDirContext)
            {
                if (dirContext.DistinguishedName.ToLower().Contains(rootDN.ToLower()))
                {
                    DirectoryEntry.exisitngDirContext.Remove(dirContext);
                    break;
                }
            }
        }
        
        if (DirectoryEntry.existingSchemaCache != null && DirectoryEntry.existingSchemaCache.Count > 0)
        {
            foreach (LDAPSchemaCache ldapSchema in DirectoryEntry.existingSchemaCache)
            {
                if (ldapSchema != null && ldapSchema.rootDN.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase))
                {
                    DirectoryEntry.existingSchemaCache.Remove(ldapSchema);
                    break;
                }
            }
        }

        string errorMessage = null;

        Logger.Log(String.Format("ADUCDomain: About to build directory context: {0}", _hn.ToString()));
        
        _adContext = DirectoryContext.CreateDirectoryContext(_hn.domainControllerName,
        //_hn.domainName,
        rootDN,
        _hn.creds.UserName,
        _hn.creds.Password,
        389,
        usingSimpleBind,
        out errorMessage);        
        
        if (String.IsNullOrEmpty(errorMessage))
        {            
            Logger.Log("ADUCDomain: Built directory context", Logger.ADUCLogLevel);
        }
        else
        {
            Logger.ShowUserError(errorMessage);
        }
        
        if (_adContext != null && !BuildSchemaCache(usingSimpleBind))
        {
            Logger.Log("BuildSchemaCache() failed!");
        }
        
        return _adContext;
    }
    
    
    private bool BuildSchemaCache(bool usingSimpleBind)
    {
        
        if (_adContext == null)
        {
            return false;
        }
        
        _adContext.SchemaCache = LDAPSchemaCache.Build(_adContext);
        _schemaCache = _adContext.SchemaCache;
        DirectoryEntry.exisitngDirContext.Add(_adContext);
        DirectoryEntry.existingSchemaCache.Add(_schemaCache);
        
        string sldapPath = string.Concat("LDAP://", _hn.domainName, "/", rootDN);       
         
        ADUCDirectoryNode rootNode = ADUCDirectoryNode.GetDirectoryRoot(
            _adContext,          
            _rootDN,
            Resources.ADUC,
            typeof(ADUCPage),
            _plugin);
        
        if (rootNode == null)
        {
            Logger.Log("The rootNode is null");
            return false;
        }
        
        _rootNode = rootNode;
        
        _shortDomainName = UserGroupUtils.getnetBiosName(rootNode);
        
        Logger.Log(
        "the obtained NetbiosName is " + _shortDomainName,
        Logger.ldapLogLevel);
        
        return true;
    }
    
    /// <summary>
    /// Gets and initializes the LikewiseIdentityCell for each root level
    /// </summary>
    /// <param name="dirnode"></param>
    private void likewiseIdentityCell_init(ADUCDirectoryNode dirnode)
    {
        string likewiseCellDN = string.Concat("CN=$LikewiseIdentityCell,", dirnode.LdapContext.RootDN);
        
        LdapValue[] descriptionValues =
        UserGroupUtils.SearchAttrByDn(likewiseCellDN, dirnode, "description");
        
        if (descriptionValues == null)
        {
            Logger.Log(String.Format(
            "No description attribute found in {0}",
            likewiseCellDN),
            Logger.LogLevel.Error);
            return;
        }
        
        foreach ( LdapValue value in descriptionValues )
        {
            string descriptionString = value.stringData;
            if (!String.IsNullOrEmpty(descriptionString))
            {
                string[] split = descriptionString.Split('=');
                if (split[0].Equals("unixHomeDirectory", StringComparison.InvariantCultureIgnoreCase))
                {
                    _defaultUnixHomedir = split[1].Trim();
                }
                if (split[0].Equals("loginShell", StringComparison.InvariantCultureIgnoreCase))
                {
                    _loginShell = split[1].Trim();
                }
                if (split[0].Equals("use2307Attrs", StringComparison.InvariantCultureIgnoreCase))
                {
                    if (split[1].Equals("True", StringComparison.InvariantCultureIgnoreCase))
                    {
                        Logger.Log("RFC2307 mode is detected!", Logger.ldapLogLevel);              
                    }
                }
            }
        }
    }
    
    /// <summary>
    /// Returns the ADUC Plugin root node
    /// </summary>
    /// <returns></returns>
    private LACTreeNode GetADUCRootNode()
    {
        Logger.Log("ADUCPlugin.GetADUCRootNode", Logger.manageLogLevel);
        
        if (_pluginNode == null)
        {            
            _pluginNode = Manage.CreateIconNode(Resources.ADUCTitle,
            Resources.ADUC,
            typeof(ADUCPage),
            _plugin);
            _pluginNode.IsPluginNode = true;            
        }
        
        _pluginNode.ImageIndex = (int)Manage.ManageImageType.Generic;
        _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Generic;
        
        return _pluginNode;
    }
    
    #endregion
    
    #region accessor functions
    
    
    public DirectoryContext adContext
    {
        get
        {
            return _adContext;
        }
    }
    
    public LDAPSchemaCache schemaCache
    {
        get
        {
            return _schemaCache;
        }
    }
    
    public ADUCDirectoryNode rootNode
    {
        get
        {
            return _rootNode;
        }
    }
    
    public IPlugIn plugin
    {
        get
        {
            return _plugin;
        }
        set
        {
            _plugin = value;
        }
    }
    
    public LACTreeNode pluginNode
    {
        get
        {
            return _pluginNode;
        }
        set
        {
            _pluginNode = value;
        }
    }
    
    public string rootDN
    {
        get
        {
            return _rootDN;
        }
    }
    
    public string ShortDomainName
    {
        get
        {
            return _shortDomainName;
        }
    }
    
    public string DefaultUnixHomeDir
    {
        get
        {
            return _defaultUnixHomedir;
        }
        set
        {
            _defaultUnixHomedir = value;
        }
    }
    
    public string LoginShell
    {
        get
        {
            return _loginShell;
        }
        set
        {
            _loginShell = value;
        }
    }
    
    public Hostinfo HostInfo
    {
        get
        {
            return _hn;
        }
        set
        {
            _hn = value;
        }
    }
    
    public LWTreeView lmcTreeview
    {
        get
        {
            return _lmctreeview;
        }
        set
        {
            _lmctreeview = value;
        }
    }  
   
    
    #endregion
    
}
}
