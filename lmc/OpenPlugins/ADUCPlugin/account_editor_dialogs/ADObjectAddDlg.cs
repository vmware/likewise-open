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
using System.Collections;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ADObjectAddDlg : Likewise.LMC.ServerControl.WizardDialog
{
    #region Class Data
    public ObjectInfo objectInfo;
    public string[] objectClasses; //this saves the cName info easy to look up
    public LDAPSchemaCache schemaCache;
    public string choosenClass;
    public List<LdapAttributeType> ClassAttributeList = null;
    #endregion
    
    #region Constructors
    public struct ADObjectAttibutes
    {
        
    }
    
    public ADObjectAddDlg()
    {
        InitializeComponent();
    }
    
    /// <summary>
    /// Overriden constructor gets all class schema attributes from AD Schema template
    /// </summary>
    /// <param name="container"></param>
    /// <param name="parentPage"></param>
    /// <param name="text"></param>
    /// <param name="schemaCache"></param>
    public ADObjectAddDlg(IPlugInContainer container, StandardPage parentPage, string text, LDAPSchemaCache schemaCache, ADUCDirectoryNode dirnode)
        : this()
    {
        this.IPlugInContainer = container;
        this.Text = text;

        string[] objectClasses = null;
        string[] attrs = { "name", "allowedAttributes", "allowedChildClasses", null };

        if (schemaCache != null && dirnode != null)
        {
            List<LdapEntry> ldapEntries = null;
            int ret = dirnode.LdapContext.ListChildEntriesSynchronous
            (dirnode.DistinguishedName,
            LdapAPI.LDAPSCOPE.BASE,
            "(objectClass=*)",
            attrs,
            false,
            out ldapEntries);

            if (ldapEntries == null)
            {
                return;
            }

            LdapEntry ldapNextEntry = ldapEntries[0];

            LdapValue[] ldapValues = ldapNextEntry.GetAttributeValues("allowedChildClasses", dirnode.LdapContext);
            if (ldapValues != null && ldapValues.Length > 0)
            {
                objectClasses = new string[ldapValues.Length];
                int index = 0;
                foreach (LdapValue Oclass in ldapValues)
                {
                    objectClasses[index] = Oclass.stringData;
                    index++;
                }
            }
        }

        this.objectClasses = objectClasses;
        this.schemaCache = schemaCache;
        this.choosenClass = null;

        this.objectInfo = new ObjectInfo();

        this.AddPage(new ObjectAddWelcomePage(this, dirnode, container, parentPage));
    }
    #endregion
}

public class ObjectInfo
{
    #region Class Data
    public static TreeNode selectedNode = null;
    public string cn;
    public string sAMAccountName;
    public string groupType;
    public List<string> addedPages = new List<string>();
    public static int PageIndex = 0;
    
    public AttributeInfo _attributeInfo = null;
    
    public Hashtable htMandatoryAttrList = null;    
    public bool commit = false;    
    public Dictionary<string, AttributeInfo> _AttributesList = new Dictionary<string, AttributeInfo>();

    #endregion
}

public class AttributeInfo
{
    #region Class Data
    public string sAttributename = "";
    public string sAttributeValue = "<not set>";
    public string sAttributeType = "";
    public LMC.LDAP.SchemaType schemaInfo;
    #endregion
}

}

