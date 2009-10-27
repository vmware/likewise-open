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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using SchemaType=Likewise.LMC.LDAP.SchemaType;
using System.Collections;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ObjectAddWelcomePage : WizardPage
{
    #region Class Data
    
    private ADObjectAddDlg _objectAddDlg;
    public static string PAGENAME = "ObjectAddWelcomePage";
    private ADUCDirectoryNode dirnode = null;
    private IPlugInContainer _container = null;
    private StandardPage _parentPage = null;
    
    #endregion
    
    #region Constructors
    public ObjectAddWelcomePage()
    {
        InitializeComponent();
    }
    
    
    /// <summary>
    /// Gets all class attributes from the Schema template and add them to the treeview
    /// </summary>
    /// <param name="objectAddDlg"></param>
    public ObjectAddWelcomePage(ADObjectAddDlg objectAddDlg, ADUCDirectoryNode dirnode, IPlugInContainer container, StandardPage parentPage)
        : this()
    {
        this._objectAddDlg = objectAddDlg;
        this.dirnode = dirnode;
        this._container = container;
        this._parentPage = parentPage;

        if (_objectAddDlg.objectClasses != null)
        {
            if (treeView1.Nodes != null)
            {
                treeView1.Nodes.Clear();
            }

            foreach (string node in _objectAddDlg.objectClasses)
            {
                if (node != null)
                {
                    SchemaType schemaType = _objectAddDlg.schemaCache.GetSchemaTypeByObjectClass(node);
                    LdapClassType ldapClassType = schemaType as LdapClassType;
                    String[] mandatoryAttributes = ldapClassType.MandatoryAttributes;
                    TreeNode schemeNode = new TreeNode();
                    schemeNode.Text = schemaType.AttributeDisplayName;
                    schemeNode.Tag = mandatoryAttributes;
                    treeView1.Nodes.Add(schemeNode);
                    treeView1.Sort();
                }
            }
        }

        treeView1.HideSelection = true;
    }
    #endregion
    
    #region Override Methods
    public override string OnWizardNext()
    {       
        if (_objectAddDlg.objectInfo.addedPages.Count != 0 &&
            _objectAddDlg.objectInfo.addedPages.Count > ObjectInfo.PageIndex)
        {
            return _objectAddDlg.objectInfo.addedPages[ObjectInfo.PageIndex++];
        }

        return base.OnWizardNext();
    }
    
    public override string OnWizardStart()
    {
        try
        {
            if (_objectAddDlg.objectInfo.addedPages.Count != 0 &&
                _objectAddDlg.objectInfo.addedPages.Count > ObjectInfo.PageIndex)
            {
                return _objectAddDlg.objectInfo.addedPages[ObjectInfo.PageIndex++];
            }
        }
        catch
        {
            ObjectInfo.PageIndex = 0;
            return _objectAddDlg.objectInfo.addedPages[ObjectInfo.PageIndex++];
        }

        return base.OnWizardNext();
    }
    
    public override string OnWizardBack()
    {
        return base.OnWizardBack();
    }
    
    public override bool OnSetActive()
    {
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.disableButton(WizardDialog.WizardButton.Back);
        Wizard.hideButton(WizardDialog.WizardButton.Next);
        Wizard.showButton(WizardDialog.WizardButton.Start);
        if (ObjectInfo.selectedNode != null && ObjectInfo.selectedNode.IsSelected)
        {
            treeView1.SelectedNode = ObjectInfo.selectedNode;
            Wizard.enableButton(WizardDialog.WizardButton.Start);
        }
        else
        {
            Wizard.disableButton(WizardDialog.WizardButton.Start);
        }
        Wizard.disableButton(WizardDialog.WizardButton.Finish);
        Wizard.hideButton(WizardDialog.WizardButton.Finish);
        
        return true;
    }
    #endregion
    
    #region Events
    
    /// <summary>
    /// initializes the seleted node to get the all attribute schema list for the selected objectclass
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void treeView1_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
    {
        if (sender != null)
        {
            ObjectInfo.selectedNode = e.Node as TreeNode;
        }
    }
    
    /// <summary>
    /// Adds the wizard pages to the wizard dialog based on selected objectclass
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
    {
        if (sender != null)
        {
            ObjectInfo.selectedNode = e.Node as TreeNode;
            _objectAddDlg.choosenClass = e.Node.Text;
            string[] mandatoryAttributes = e.Node.Tag as string[];
            AddWizardPages(e.Node.Text, mandatoryAttributes);
            Wizard.enableButton(WizardDialog.WizardButton.Start);
        }
    }
    #endregion
    
    #region Methods
    
    /// <summary>
    /// initializes the wiazrd pages based on "systemMustContain" attribute value list for the selected objectclass
    /// Adds the wizard pages to the wizard dialog
    /// </summary>
    /// <param name="nodeText"></param>
    /// <param name="mandatoryAttributes"></param>
    private void AddWizardPages(string nodeText, String[] mandatoryAttributes)
    {
        treeView1.HideSelection = false;
        _objectAddDlg.choosenClass = nodeText;
        _objectAddDlg.objectInfo.htMandatoryAttrList = new Hashtable();        

        List<string> attrlist = new List<string>();
        _objectAddDlg.ClassAttributeList = new List<LdapAttributeType>();

        attrlist.Add("instanceType");
        attrlist.Add("objectCategory");
        attrlist.Add("objectClass");
        if (_objectAddDlg.choosenClass.Trim().Equals("user", StringComparison.InvariantCultureIgnoreCase) ||
            _objectAddDlg.choosenClass.Trim().Equals("group", StringComparison.InvariantCultureIgnoreCase) ||
            _objectAddDlg.choosenClass.Trim().Equals("computer", StringComparison.InvariantCultureIgnoreCase))
        {
            attrlist.Add("objectSid");
            attrlist.Add("sAMAccountName");
            if (!attrlist.Contains("cn"))
            {
                attrlist.Add("cn");
            }
            if (mandatoryAttributes != null)
            {
                foreach (string attr in mandatoryAttributes)
                {
                    if (!attrlist.Contains(attr))
                    {
                        attrlist.Add(attr);
                    }
                }
            }
        }
        
        LdapClassType classtype = _objectAddDlg.schemaCache.GetSchemaTypeByObjectClass(_objectAddDlg.choosenClass) as LdapClassType;

        AttributeMap attr_map = classtype.Tag as AttributeMap;
        LdapEntry ldapentry = attr_map.Tag as LdapEntry;

        string DN = ldapentry.GetDN();
        string[] attrs = { "name", "allowedAttributes", null };       

        List<LdapEntry> innerLdapEntries = null;
        int ret = dirnode.LdapContext.ListChildEntriesSynchronous
        (dirnode.DistinguishedName,
        LdapAPI.LDAPSCOPE.BASE,
        "(objectClass=*)",
        attrs,
        false,
        out innerLdapEntries);

        ldapentry = innerLdapEntries[0];

        LdapValue[] ldapValues = ldapentry.GetAttributeValues("allowedAttributes", dirnode.LdapContext);
        if (ldapValues != null && ldapValues.Length > 0)
        {
            string[] optionalAttrs = new string[ldapValues.Length];           
            foreach (LdapValue Oclass in ldapValues)
            {
                string attrValue = Oclass.stringData;
                SchemaType schematype = _objectAddDlg.schemaCache.GetSchemaTypeByDisplayName(attrValue) as SchemaType;
                if (schematype != null)
                {
                    schematype.AttributeType = "Optional";
                    _objectAddDlg.ClassAttributeList.Add(schematype as LdapAttributeType);
                }
            }

            foreach (string strValue in attrlist)
            {
                SchemaType schematype = _objectAddDlg.schemaCache.GetSchemaTypeByDisplayName(strValue) as SchemaType;
                if (schematype != null)
                {
                    schematype.AttributeType = "Mandatory";
                    _objectAddDlg.ClassAttributeList.Add(schematype as LdapAttributeType);
                }
            }           
        }
       
        if (_objectAddDlg.ClassAttributeList != null && _objectAddDlg.ClassAttributeList.Count != 0)
        {
            foreach (LdapAttributeType Attribute in _objectAddDlg.ClassAttributeList)
            {
                AttributeInfo attributeInfo = new AttributeInfo();
                attributeInfo.sAttributename = Attribute.AttributeDisplayName;
                attributeInfo.sAttributeValue = "<not set>";
                attributeInfo.sAttributeType = Attribute.AttributeType;
                attributeInfo.schemaInfo =
                    _objectAddDlg.schemaCache.GetSchemaTypeByCommonName(Attribute.CName);
                if (!_objectAddDlg.objectInfo._AttributesList.ContainsKey(Attribute.AttributeDisplayName))
                {
                    _objectAddDlg.objectInfo._AttributesList.Add(Attribute.AttributeDisplayName, attributeInfo);
                }
            }
        }      
        ObjectAddSinglePage ObjectAddSinglePage = null;
        _objectAddDlg.objectInfo.addedPages = new List<string>();
        ObjectInfo.PageIndex = 0;
        //for all objects we should prompt to ask for their cn
        if (nodeText.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
        {
            ObjectAddSinglePage = new ObjectAddSinglePage(_objectAddDlg, "ou");
            _objectAddDlg.AddPage(ObjectAddSinglePage);
            _objectAddDlg.objectInfo.addedPages.Add("ou");
        }
        else
        {
            ObjectAddSinglePage = new ObjectAddSinglePage(_objectAddDlg, "cn");
            _objectAddDlg.AddPage(ObjectAddSinglePage);
            _objectAddDlg.objectInfo.addedPages.Add("cn");
        }
       
        if (mandatoryAttributes != null && mandatoryAttributes.Length != 0)
        {
            for (int i = 0; i < mandatoryAttributes.Length; i++)
            {
                if (!((mandatoryAttributes[i].Trim().ToLower().Equals("cn")) || (mandatoryAttributes[i].Trim().ToLower().Equals("ou"))))
                {
                    ObjectAddSinglePage = new ObjectAddSinglePage(_objectAddDlg, mandatoryAttributes[i].Trim());
                    _objectAddDlg.AddPage(ObjectAddSinglePage);
                    _objectAddDlg.objectInfo.addedPages.Add(mandatoryAttributes[i].Trim());
                }
            }
        }
        
        if (_objectAddDlg.choosenClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase)
        ||
        _objectAddDlg.choosenClass.Equals("user", StringComparison.InvariantCultureIgnoreCase)
        ||
        _objectAddDlg.choosenClass.Equals("group", StringComparison.InvariantCultureIgnoreCase))
        {
            ObjectAddSinglePage = new ObjectAddSinglePage(_objectAddDlg, "sAMAccountName");
            _objectAddDlg.AddPage(ObjectAddSinglePage);
            _objectAddDlg.objectInfo.addedPages.Add("sAMAccountName");
        }
    
        //for all objects they all come to the end of final page
        _objectAddDlg.AddPage(new ObjectAddFinalPage(_objectAddDlg, _container, _parentPage));       

        Wizard.enableButton(WizardDialog.WizardButton.Start);
    }
    #endregion
    
}
}

