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
using System.Collections;
using System.Drawing;
using System.Diagnostics;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.Plugins.ADUCPlugin.Properties;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.ADUCPlugin
{

    public interface IDirectoryPropertiesPage
    {
        void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode node);
    }

/// <summary>
/// Summary description for ADUCDirectoryNode.
/// </summary>
public class ADUCDirectoryNode: DirectoryNode
{
    #region data    

    public bool _IsDisabled = false;
    public bool _IsDomainController = false;
    public bool _hasChidren = true;
    
    public List<LdapEntry> ldapEntries = new List<LdapEntry>();
    
    #endregion
    
    #region constructors
    
    /// <summary>
    /// initializes the Directory node with the selected AD Object DistinguishedName,Objectclass and directory context objects
    /// initializes selected pulgin info
    /// </summary>
    /// <param name="distinguishedName"></param>
    /// <param name="dirContext"></param>
    /// <param name="objectClass"></param>
    /// <param name="image"></param>
    /// <param name="t"></param>
    /// <param name="plugin"></param>
    public ADUCDirectoryNode(string distinguishedName,
    DirectoryContext dirContext,   
    string objectClass,
    Icon image,
    Type t,
    IPlugIn plugin,
    bool IsDisabled)
    : base(distinguishedName, image, t, plugin)
    {
        
        this.distinguishedName = distinguishedName;
        this.dirContext = dirContext;      
        sc = this.sc;
        
        _IsDisabled = IsDisabled;
        
        if (!distinguishedName.StartsWith("DC=", StringComparison.InvariantCultureIgnoreCase))
        {
            string[] parts = distinguishedName.Split(',');
            if (parts.Length > 0)
            {
                Text = parts[0];
            }
        }
        
        _objectClass = objectClass;
        
        if (_IsDisabled && _objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase))
        {
            objectClass = "disabledUser";
        }
        else if (_IsDisabled && _objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
        {
            objectClass = "disabledComputer";
        }
        
        ImageIndex = (int)GetNodeType(objectClass);
        SelectedImageIndex = (int)GetNodeType(objectClass);
    }
    
    /// <summary>
    /// initializes the Directory node with the selected AD Object DistinguishedName,Objectclass and directory context objects
    /// </summary>
    /// <param name="dirnode"></param>
    public ADUCDirectoryNode(ADUCDirectoryNode dirnode)
    {
        this.distinguishedName = dirnode.distinguishedName;
        this.dirContext = dirnode.dirContext;
        
        _objectClass = dirnode.ObjectClass; 
        this._IsDisabled = dirnode.IsDisabled;
        sc = this.sc;
        
        if (!dirnode.distinguishedName.StartsWith("DC=", StringComparison.InvariantCultureIgnoreCase))
        {
            string[] parts = dirnode.distinguishedName.Split(',');
            if (parts.Length > 0)
            {
                Text = parts[0];
            }
        }
        
        string objectClass = _objectClass;
        
        if (_IsDisabled && _objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase))
        {
            objectClass = "disabledUser";
        }
        else if (_IsDisabled && _objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
        {
            objectClass = "disabledComputer";
        }
        
        ImageIndex = (int) GetNodeType(objectClass);
        SelectedImageIndex = (int) GetNodeType(objectClass);
        
    }
    
    
    /// <summary>
    /// initializes the Directory node DistinguishedName with DistinguishedName Dn for the selected AD Object
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="objectClass"></param>
    /// <param name="newDn"></param>
    public ADUCDirectoryNode(ADUCDirectoryNode dirnode, string objectClass, string newDn)
    {
        this.distinguishedName = newDn;
        this.dirContext = dirnode.dirContext;
        
        _objectClass = objectClass;      
        this._IsDisabled = dirnode.IsDisabled;
        
        if (!dirnode.distinguishedName.StartsWith("DC=", StringComparison.InvariantCultureIgnoreCase))
        {
            string[] parts = dirnode.distinguishedName.Split(',');
            if (parts.Length > 0)
            {
                Text = parts[0];
            }
        }
        
        if (_IsDisabled && _objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase))
        {
            objectClass = "disabledUser";
        }
        else if (_IsDisabled && _objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
        {
            objectClass = "disabledComputer";
        }
        
        ImageIndex = (int)GetNodeType(objectClass);
        SelectedImageIndex = (int)GetNodeType(objectClass);
        
    }
    
    #endregion    
    
    #region public methods
    
    /// <summary>
    /// List the all children for the selected distinguished name
    /// Adds the all children to the node
    /// </summary>
    public void ListChildren()
    {
        Logger.Log("ADUCDirectoryNode.ListChildren() called", Logger.ldapLogLevel);
        int ret = -1;

        if (haveRetrievedChildren && !this.IsModified)
        {
            return;
        }

        string[] attrList = new string[]
        {
            "dummy", 
            "objectClass", 
            "distinguishedName", 
            "userAccountControl", 
            null
        };

        ret = dirContext.ListChildEntriesSynchronous
        (distinguishedName,
        LdapAPI.LDAPSCOPE.ONE_LEVEL,
        "(objectClass=*)",
        attrList,
        false,
        out ldapEntries);

        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            //clears the domian level node, if the ldap connection timed out or disconnected
            ADUCPlugin plugin = this.Plugin as ADUCPlugin;
            haveRetrievedChildren = true;
            this.IsModified = false;
            if (ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
            ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
            ret == -1)
            {
                if (ret == -1)
                {
                    ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                    Logger.LogMsgBox(ErrorCodes.LDAPString(ret));
                }
                plugin._pluginNode.Nodes.Clear();
                this.sc.ShowControl(plugin._pluginNode);
            }
            else
            {
                Nodes.Clear();
            }

            return;
        }
        else if (IsModified)
        {
            Nodes.Clear();
        }

        DateTime timer = Logger.StartTimer();

        //The following is optimized for speed, taking into account that in Mono,
        //Nodes.Add() and Nodes.AddRange() both take a long time to complete.
        //Nodes.AddRange() does not offer much time savings over Nodes.Add()
        //Therefore, make two hashtables holding the new and old contents of the DN.
        //Determine which have been added, and which have been deleted, to minimize the number of calls
        //to Nodes.Add() and Nodes.Remove();
        Hashtable oldEntries = new Hashtable();
        Hashtable newEntries = new Hashtable();

        List<ADUCDirectoryNode> nodesToAdd = new List<ADUCDirectoryNode>();
        int nodesAdded = 0;

        foreach (TreeNode node in Nodes)
        {
            ADUCDirectoryNode dNode = (ADUCDirectoryNode)node;
            if (dNode != null && !String.IsNullOrEmpty(dNode.distinguishedName) &&
            !oldEntries.ContainsKey(dNode.distinguishedName))
            {
                oldEntries.Add(dNode.distinguishedName, dNode);
            }
        }

        foreach (LdapEntry ldapNextEntry in ldapEntries)
        {
            string currentDN = ldapNextEntry.GetDN();

            if (!String.IsNullOrEmpty(currentDN))
            {

                LdapValue[] values = ldapNextEntry.GetAttributeValues("objectClass", dirContext);
                string objectClass = "";
                if (values != null && values.Length > 0)
                {
                    objectClass = values[values.Length - 1].stringData;
                }

                bool IsDisabled = false;
                bool IsDc = false;
                if (objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase)
                || objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                {
                    values = ldapNextEntry.GetAttributeValues("userAccountControl", dirContext);
                    int userCtrlVal = 0;
                    if (values != null && values.Length > 0)
                    {
                        userCtrlVal = Convert.ToInt32(values[0].stringData);
                    }
                    if (userCtrlVal.Equals(8224) || userCtrlVal.Equals(8202) || userCtrlVal.Equals(532480))
                    {
                        IsDc = true;
                    }
                    string userCtrlBinStr = UserGroupUtils.DecimalToBase(userCtrlVal, 2);

                    //Determine whether this user is enabled or disabled
                    //examine the second to last position from the right (0=Active, 1=Inactive)
                    if (userCtrlBinStr.Length >= 2)
                    {
                        if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '1')
                        {
                            IsDisabled = true;
                        }
                        else if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '0')
                        {
                            IsDisabled = false;
                        }
                    }
                }

                ADUCDirectoryNode newNode = new ADUCDirectoryNode(currentDN, dirContext, objectClass,
                Resources.Group_16, NodeType, Plugin, IsDisabled);
                newNode.sc = this.sc;
                newNode._IsDomainController = IsDc;
                newNode.Text = newNode.Text.Substring(3);

                Logger.Log(String.Format("new Entry: {0}", currentDN), Logger.ldapLogLevel);

                newEntries.Add(currentDN, newNode);

                if (oldEntries.ContainsKey(currentDN))
                {
                    ADUCDirectoryNode oldNode = (ADUCDirectoryNode)oldEntries[currentDN];

                    if ((oldNode != null && oldNode.ObjectClass != objectClass)
                    ||
                    (oldNode != null && oldNode.IsDisabled != IsDisabled))
                    {
                        oldEntries.Remove(currentDN);
                        oldEntries.Add(currentDN, newNode);
                        Nodes.Remove(oldNode);
                        nodesToAdd.Add(newNode);
                        nodesAdded++;
                    }

                }
                else
                {
                    Logger.Log(String.Format("scheduling addition of new Entry: {0}", currentDN), Logger.ldapLogLevel);
                    nodesToAdd.Add(newNode);
                    nodesAdded++;
                }

            }
        }

        foreach (Object o in oldEntries.Keys)
        {
            string oldNodeKey = (string)o;

            Logger.Log(String.Format("old Entry: {0}", oldNodeKey));

            if (!String.IsNullOrEmpty(oldNodeKey) && !newEntries.ContainsKey(oldNodeKey))
            {
                ADUCDirectoryNode oldNode = (ADUCDirectoryNode)oldEntries[oldNodeKey];

                if (oldNode != null)
                {
                    Logger.Log(String.Format("removing old Entry: {0}", oldNodeKey), Logger.ldapLogLevel);

                    Nodes.Remove(oldNode);
                }
            }
        }

        ADUCDirectoryNode[] nodesToAddRecast = new ADUCDirectoryNode[nodesAdded];
        try
        {
            nodesToAdd.Sort(delegate(ADUCDirectoryNode d1, ADUCDirectoryNode d2)
            {
                return d1.Text.CompareTo(d2.Text);
            }
            );
            for (int i = 0; i < nodesAdded; i++)
            {
                nodesToAddRecast[i] = nodesToAdd[i];
            }
        }
        catch (Exception)
        {
        }

        Nodes.AddRange(nodesToAddRecast);

        Logger.TimerMsg(ref timer, String.Format("DirectoryNode.ListChildren(): Entry Processing({0})", distinguishedName));
        this.IsModified = false;
        haveRetrievedChildren = true;
    }
    
    /// <summary>
    /// List the all children for the selected distinguished name
    /// Adds the all children to the node
    /// </summary>
    public ADUCDirectoryNode[] ListChildren(ADUCDirectoryNode dnode)
    {
        Logger.Log("DirectoryNode.ListChildren() called", Logger.ldapLogLevel);
        int ret = -1;
        
        if (haveRetrievedChildren && !this.IsModified)
        {
            return null;
        }

        string[] attrList = new string[]
        {
            "dummy", 
            "objectClass", 
            "distinguishedName", 
            "userAccountControl", 
            null
        };

        ret = dirContext.ListChildEntriesSynchronous
        (distinguishedName,
        LdapAPI.LDAPSCOPE.ONE_LEVEL,
        "(objectClass=*)",
        attrList,
        false,
        out ldapEntries);
        
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            //haveRetrievedChildren = true;
            //return;
            
            //clears the domian level node, if the ldap connection timed out or disconnected
            ADUCPlugin plugin = this.Plugin as ADUCPlugin;
            haveRetrievedChildren = true;
            this.IsModified = false;
            if (ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
            ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
            ret == -1)
            {
                if (ret == -1)
                {
                    ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                    Logger.LogMsgBox(ErrorCodes.LDAPString(ret));
                }
                plugin._pluginNode.Nodes.Clear();
                this.sc.ShowControl(plugin._pluginNode);
            }
            else
            {
                Nodes.Clear();
            }
            
            return null;
        }
        
        DateTime timer = Logger.StartTimer();
        
        //The following is optimized for speed, taking into account that in Mono,
        //Nodes.Add() and Nodes.AddRange() both take a long time to complete.
        //Nodes.AddRange() does not offer much time savings over Nodes.Add()
        //Therefore, make two hashtables holding the new and old contents of the DN.
        //Determine which have been added, and which have been deleted, to minimize the number of calls
        //to Nodes.Add() and Nodes.Remove();
        Hashtable oldEntries = new Hashtable();
        Hashtable newEntries = new Hashtable();

        List<ADUCDirectoryNode> nodesToAdd = new List<ADUCDirectoryNode>();
        int nodesAdded = 0;
        
        foreach (TreeNode node in Nodes)
        {
            ADUCDirectoryNode dNode = (ADUCDirectoryNode)node;
            if (dNode != null && !String.IsNullOrEmpty(dNode.distinguishedName) &&
            !oldEntries.ContainsKey(dNode.distinguishedName))
            {
                oldEntries.Add(dNode.distinguishedName, dNode);
            }
            
        }
        
        foreach (LdapEntry ldapNextEntry in ldapEntries)
        {
            string currentDN = ldapNextEntry.GetDN();
            
            if (!String.IsNullOrEmpty(currentDN))
            {
                
                LdapValue[] values = ldapNextEntry.GetAttributeValues("objectClass", dirContext);
                string objectClass = "";
                if (values != null && values.Length > 0)
                {
                    objectClass = values[values.Length - 1].stringData;
                }

                List<LdapEntry> childLdapEntries = null;
                ret = dirContext.ListChildEntriesSynchronous
                                (currentDN,
                                LdapAPI.LDAPSCOPE.ONE_LEVEL,
                                "(objectClass=*)",
                                new string[] { null },
                                false,
                                out childLdapEntries);
                if (childLdapEntries == null || childLdapEntries.Count == 0)
                    _hasChidren = false;
                
                bool IsDisabled = false;
                bool IsDc = false;
                if (objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase)
                || objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                {
                    values = ldapNextEntry.GetAttributeValues("userAccountControl", dirContext);
                    int userCtrlVal = 0;
                    if (values != null && values.Length > 0)
                    {
                        userCtrlVal = Convert.ToInt32(values[0].stringData);
                    }
                    if (userCtrlVal.Equals(8224)||userCtrlVal.Equals(8202)||userCtrlVal.Equals(532480))
                    {
                        IsDc = true;
                    }
                    string userCtrlBinStr = UserGroupUtils.DecimalToBase(userCtrlVal, 2);
                    
                    //Determine whether this user is enabled or disabled
                    //examine the second to last position from the right (0=Active, 1=Inactive)
                    if (userCtrlBinStr.Length >= 2)
                    {
                        if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '1')
                        {
                            IsDisabled = true;
                        }
                        else if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '0')
                        {
                            IsDisabled = false;
                        }
                    }
                }
                
                ADUCDirectoryNode newNode = new ADUCDirectoryNode(currentDN, dirContext, objectClass,
                Resources.Group_16, NodeType, Plugin, IsDisabled);
                newNode.sc = this.sc;
                newNode._IsDomainController = IsDc;
                newNode.Text = newNode.Text.Substring(3);
                newNode._hasChidren = _hasChidren;
                
                Logger.Log(String.Format("new Entry: {0}", currentDN), Logger.ldapLogLevel);
                
                newEntries.Add(currentDN, newNode);
                
                if (oldEntries.ContainsKey(currentDN))
                {
                    ADUCDirectoryNode oldNode = (ADUCDirectoryNode)oldEntries[currentDN];
                    
                    if ((oldNode != null && oldNode.ObjectClass != objectClass)
                    ||
                    (oldNode != null && oldNode.IsDisabled != IsDisabled))
                    {
                        oldEntries.Remove(currentDN);
                        oldEntries.Add(currentDN, newNode);
                        Nodes.Remove(oldNode);
                        nodesToAdd.Add(newNode);
                        nodesAdded++;
                    }
                    
                }
                else
                {
                    Logger.Log(String.Format("scheduling addition of new Entry: {0}", currentDN), Logger.ldapLogLevel);
                    nodesToAdd.Add(newNode);
                    nodesAdded++;
                }
                
            }
        }
        
        foreach (Object o in oldEntries.Keys)
        {
            string oldNodeKey = (string)o;
            
            Logger.Log(String.Format("old Entry: {0}", oldNodeKey));
            
            if (!String.IsNullOrEmpty(oldNodeKey) && !newEntries.ContainsKey(oldNodeKey))
            {
                ADUCDirectoryNode oldNode = (ADUCDirectoryNode)oldEntries[oldNodeKey];
                
                if (oldNode != null)
                {
                    Logger.Log(String.Format("removing old Entry: {0}", oldNodeKey), Logger.ldapLogLevel);
                    
                    Nodes.Remove(oldNode);
                }
            }
        }
        
        ADUCDirectoryNode[] nodesToAddRecast = new ADUCDirectoryNode[nodesAdded];

        try
        {
            nodesToAdd.Sort(delegate(ADUCDirectoryNode d1, ADUCDirectoryNode d2)
            {
                return d1.Text.CompareTo(d2.Text);
            }
            );
            for (int i = 0; i < nodesAdded; i++)
            {
                nodesToAddRecast[i] = nodesToAdd[i];
            }
        }
        catch (Exception)
        {
        }
        Logger.TimerMsg(ref timer, String.Format("DirectoryNode.ListChildren(): Entry Processing({0})", distinguishedName));
        this.IsModified = false;
        haveRetrievedChildren = true;        
        return nodesToAddRecast;
    }
    
    /// <summary>
    /// Getting added all children to the selected node that are of type ObjectClass="group"
    /// </summary>
    public void ListGroupOUChildren()
    {
        
        int ret = -1;
        
        if (haveRetrievedChildren && !this.IsModified)
        {
            return;
        }
        
        string[] attrs = { "objectClass", "distinguishedName", null };
        
        DateTime start = DateTime.Now;
        
        ret = dirContext.ListChildEntriesSynchronous
        (distinguishedName,
        LdapAPI.LDAPSCOPE.ONE_LEVEL,
        "(objectClass=*)",
        attrs,
        false,
        out ldapEntries);
        
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            //haveRetrievedChildren = true;
            //return;
            
            //clears the domian level node, if the ldap connection timed out or disconnected
            ADUCPlugin plugin = this.Plugin as ADUCPlugin;
            haveRetrievedChildren = true;
            this.IsModified = false;
            if (ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
            ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
            ret == -1)
            {
                if (ret == -1)
                {
                    ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                    Logger.LogMsgBox(ErrorCodes.LDAPString(ret));
                }
                plugin._pluginNode.Nodes.Clear();
                this.sc.ShowControl(plugin._pluginNode);
            }
            else
            {
                Nodes.Clear();
            }
            
            return;
        }
        
        foreach (LdapEntry ldapNextEntry in ldapEntries)
        {
            string s = ldapNextEntry.GetDN();
            string objectClass = "";
            
            LdapValue[] values = ldapNextEntry.GetAttributeValues("objectClass", dirContext);
            if (values != null && values.Length > 0)
            {
                //use the most specific object Class, which will be listed last.
                objectClass = values[values.Length - 1].stringData;
                
                Logger.Log("Start--", Logger.ldapLogLevel);
                for (int i = 0; i < values.Length; i++)
                {
                    Logger.Log("objectclass is " + values[i], Logger.ldapLogLevel);
                }
                Logger.Log("End--", Logger.ldapLogLevel);
            }
            
            if (objectClass.Equals("group", StringComparison.InvariantCultureIgnoreCase) || objectClass.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
            {
                
                ADUCDirectoryNode dtn = new ADUCDirectoryNode(s, dirContext, objectClass,
                Resources.Group_16, NodeType, Plugin, false);
                dtn.sc = this.sc;
                Nodes.Add(dtn);
            }
        }
        haveRetrievedChildren = true;
    }   
   
    
    /// <summary>
    /// Getting added all children to the selected node that are of type ObjectClass="group"
    /// , ObjectClass="user", ObjectClass="computer", ObjectClass="organizationalUnit"
    /// </summary>
    public void ListGroupAndUserOUChildren()
    {
        int ret = -1;
        
        if (haveRetrievedChildren && !this.IsModified)
        {
            return;
        }
        
        string[] attrs = { "objectClass", "distinguishedName", null };
        
        DateTime start = DateTime.Now;
        
        ret = dirContext.ListChildEntriesSynchronous
        (distinguishedName,
        LdapAPI.LDAPSCOPE.ONE_LEVEL,
        "(objectClass=*)",
        attrs,
        false,
        out ldapEntries);
        
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            //haveRetrievedChildren = true;
            //return;
            
            //clears the domian level node, if the ldap connection timed out or disconnected
            ADUCPlugin plugin = this.Plugin as ADUCPlugin;
            haveRetrievedChildren = true;
            this.IsModified = false;
            if (ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
            ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
            ret == -1)
            {
                if (ret == -1)
                {
                    ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                    Logger.LogMsgBox(ErrorCodes.LDAPString(ret));
                }
                plugin._pluginNode.Nodes.Clear();
                this.sc.ShowControl(plugin._pluginNode);
            }
            else
            {
                Nodes.Clear();
            }
            
            return;
        }
        
        foreach (LdapEntry ldapNextEntry in ldapEntries)
        {
            string s = ldapNextEntry.GetDN();
            string objectClass = "";
            
            LdapValue[] values = ldapNextEntry.GetAttributeValues("objectClass", dirContext);
            if (values != null && values.Length > 0)
            {
                //use the most specific object Class, which will be listed last.
                objectClass = values[values.Length - 1].stringData;
                
                Logger.Log("Start--", Logger.ldapLogLevel);
                for (int i = 0; i < values.Length; i++)
                {
                    Logger.Log("objectclass is " + values[i], Logger.ldapLogLevel);
                }
                Logger.Log("End--", Logger.ldapLogLevel);
            }
            
            if (objectClass.Equals("group", StringComparison.InvariantCultureIgnoreCase))
            {
                
                ADUCDirectoryNode dtn = new ADUCDirectoryNode(s, dirContext, objectClass,
                Resources.Group_16, NodeType, Plugin, false);
                dtn.sc = this.sc;
                Nodes.Add(dtn);
            }
            
            bool IsDisabled = false;
            bool IsDc = false;
            if (objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase)
            || objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
            {
                values = ldapNextEntry.GetAttributeValues("userAccountControl", dirContext);
                int userCtrlVal = 0;
                if (values != null && values.Length > 0)
                {
                    userCtrlVal = Convert.ToInt32(values[0].stringData);
                }
                if (userCtrlVal.Equals(8224) || userCtrlVal.Equals(8202) || userCtrlVal.Equals(532480))
                {
                    IsDc = true;
                }
                string userCtrlBinStr = UserGroupUtils.DecimalToBase(userCtrlVal, 2);
                
                //Determine whether this user is enabled or disabled
                //examine the second to last position from the right (0=Active, 1=Inactive)
                if (userCtrlBinStr.Length >= 2)
                {
                    if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '1')
                    {
                        IsDisabled = true;
                    }
                    else if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '0')
                    {
                        IsDisabled = false;
                    }
                }
                
                ADUCDirectoryNode dtn = new ADUCDirectoryNode(s, dirContext, objectClass,
                Resources.Group_16, NodeType, Plugin, IsDisabled);
                dtn.sc = this.sc;
                dtn._IsDomainController = IsDc;
                Nodes.Add(dtn);
            }
            
            
            if (objectClass.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
            {
                
                ADUCDirectoryNode dtn = new ADUCDirectoryNode(s, dirContext, objectClass,
                Resources.Group_16, NodeType, Plugin,false);
                dtn.sc = this.sc;
                Nodes.Add(dtn);
            }
        }
        haveRetrievedChildren = true;
    }
    
    
    /// <summary>
    /// Getting added the selected node that are of type ObjectClass="container"
    /// and ObjectClass="organizationalUnit"
    /// </summary>
    public void ListContainerChildren()
    {
        int ret = -1;
        if (haveRetrievedChildren && !this.IsModified)
        {
            return;
        }
        
        string[] attrs = { "objectClass", "distinguishedName", null };
        
        DateTime start = DateTime.Now;
        
        ret = dirContext.ListChildEntriesSynchronous
        (distinguishedName,
        LdapAPI.LDAPSCOPE.ONE_LEVEL,
        "(objectClass=*)",
        attrs,
        false,
        out ldapEntries);
        
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            //haveRetrievedChildren = true;
            //return;
            
            //clears the domian level node, if the ldap connection timed out or disconnected
            ADUCPlugin plugin = this.Plugin as ADUCPlugin;
            haveRetrievedChildren = true;
            this.IsModified = true;
            if (ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
            ret == (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
            ret == -1)
            {
                if (ret == -1)
                {
                    ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                    Logger.LogMsgBox(ErrorCodes.LDAPString(ret));
                }
                plugin._pluginNode.Nodes.Clear();
                this.sc.ShowControl(plugin._pluginNode);
            }
            else
            {
                Nodes.Clear();
            }
            
            return;
        }
        
        foreach (LdapEntry ldapNextEntry in ldapEntries)
        {
            string s = ldapNextEntry.GetDN();
            string objectClass = "";
            
            LdapValue[] values = ldapNextEntry.GetAttributeValues("objectClass", dirContext);
            if (values != null && values.Length > 0)
            {
                //use the most specific object Class, which will be listed last.
                objectClass = values[values.Length - 1].stringData;
                
                Logger.Log("Start--", Logger.ldapLogLevel);
                for (int i = 0; i < values.Length; i++)
                {
                    Logger.Log("objectclass is " + values[i], Logger.ldapLogLevel);
                }
                Logger.Log("End--", Logger.ldapLogLevel);
            }
            
            if (objectClass.Equals("container", StringComparison.InvariantCultureIgnoreCase) ||
                objectClass.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase) ||
                objectClass.Equals("serviceConnectionPoint", StringComparison.InvariantCultureIgnoreCase))
            {                
                ADUCDirectoryNode dtn = new ADUCDirectoryNode(s, dirContext, objectClass,
                Resources.Group_16, NodeType, Plugin, false);
                dtn.sc = this.sc;
                Nodes.Add(dtn);
            }
        }
        haveRetrievedChildren = true;
    }
    
    /// <summary>
    /// Refreshes the listview with all children for the selected node
    /// </summary>
    public void Refresh()
    {
        Logger.Log("DirectoryNode::Refresh", Logger.ldapLogLevel);
        haveRetrievedChildren = false;
        this.IsModified = true;
        ListChildren();
        this.Collapse();
        //this.ExpandAll();
        this.Expand();
    }
    
    public static ADUCDirectoryNode GetDirectoryRoot(DirectoryContext dirCtx, string DistinguishedName, Icon image, Type t, IPlugIn plugin)
    {
        if (dirCtx == null)
        {
            Logger.LogMsgBox("DirectoryNode.GetDirectoryRoot(): dirCtx == null");
            return null;
        }
        
        ADUCDirectoryNode dn = new ADUCDirectoryNode(DistinguishedName,
        dirCtx, "container", image, t, plugin, false);
        
        return dn;
    }
    
    /// <summary>
    /// Returns the Image type for the selected object
    /// </summary>
    /// <param name="objectClass"></param>
    /// <returns></returns>
    public static Manage.ManageImageType GetNodeType(string objectClass)
    {
        if (!string.IsNullOrEmpty(objectClass))
        {
            if (objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.User;
            }
            else if (objectClass.Equals("disabledUser", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.DisabledUser;
            }
            else if (objectClass.Equals("disabledComputer", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.DisabledComputer;
            }
            else if (objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.Computer;
            }
            else if (objectClass.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.OrganizationalUnit;
            }
            else if (objectClass.Equals("container", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.Container;
            }
            else if (objectClass.Equals("group", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.Group;
            }
            else if (objectClass.Equals("serviceConnectionPoint", StringComparison.InvariantCultureIgnoreCase))
            {
                return Manage.ManageImageType.Container;
            }
        }
        
        Logger.Log(
        String.Format(
        "DirectoryNode.GetNodeType could not resolve objectClass={0}: using Generic",
        objectClass), Logger.ldapLogLevel);
        
        return Manage.ManageImageType.Generic;
    }
    
    
    /// <summary>
    /// Method to get the NodeType for the selected object
    /// </summary>
    /// <param name="dirNode"></param>
    /// <returns></returns>
    public static Manage.ManageImageType GetNodeType(ADUCDirectoryNode dirNode)
    {
        if (dirNode != null)
        {
            string objClass = dirNode._objectClass;
            
            if (dirNode._IsDisabled && dirNode._objectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase))
            {
                objClass = "disabledUser";
            }
            else if (dirNode._IsDisabled && dirNode._objectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
            {
                objClass = "disabledComputer";
            }
            
            return GetNodeType(objClass);
        }
        return Manage.ManageImageType.Generic;
    }
    
    #endregion

    #region accessors

    public bool IsDisabled
    {
        get
        {
            return _IsDisabled;
        }
        set
        {
            _IsDisabled = value;
        }
    }

    public bool IsDomainController
    {
        get
        {
            return _IsDomainController;
        }
        set
        {
            _IsDomainController = value;
        }
    }

    public Hostinfo hn
    {
        get
        {
            return _ctx as Hostinfo;
        }
        set
        {
            _ctx = value;
        }
    }

    #endregion
  }
}
