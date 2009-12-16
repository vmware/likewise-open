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
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ObjectAddMoreAttributesDlg  : MPContainer
{
    #region Class Data
    
    private ADObjectAddDlg _objectAddDlg;
    
    #endregion
    
    #region Constructors
    public ObjectAddMoreAttributesDlg(ADObjectAddDlg objectAddDlg, IPlugInContainer container, StandardPage parentPage)
        : this(container, parentPage)
    {
        this._objectAddDlg = objectAddDlg;
        InitializeComponent();
        InitializePages();
    }
    
    public ObjectAddMoreAttributesDlg(IPlugInContainer container, StandardPage parentPage)
    : base(container, parentPage)
    {
    }
    
    #endregion
    
    #region Initialization Methods
    
    /// <summary>
    /// Method to initailize the tab pages for the property sheet
    /// </summary>
    private void InitializePages()
    {
        MPPage page = null;
        
        page = new ObjectAddAttributesTab(_objectAddDlg);
        this.AddPage(page,
        new MPMenuItem(page.PageID, "Attributes", "Attributes"),
        MPMenu.POSITION_END);
    }
    
    
    /// <summary>
    /// Method to load data to the tab pages while loading
    /// Gets all the tab pages that are of type MPage and gets call the SetData()
    /// Queries the ldap message to the selected node
    /// </summary>
    /// <param name="ce"></param>
    /// <param name="servername"></param>
    /// <param name="computer"></param>
    /// <param name="dirnode"></param>
    /// <param name="ldapSchemaCache"></param>
    public void SetData(CredentialEntry ce, string servername, string computer, ADUCDirectoryNode dirnode, LDAPSchemaCache ldapSchemaCache)
    {
        string PageTitle = string.Format("cn={0}", _objectAddDlg.objectInfo.htMandatoryAttrList["cn"]);
        
        if (PageTitle != null)
        {
            this.Text = String.Format(this.Text, PageTitle);
        }
        
        if (this.GetPages() != null)
        {
            foreach (MPPage page in this.GetPages())
            {
                if (page != null)
                {
                    IDirectoryPropertiesPage ipp = page as IDirectoryPropertiesPage;
                    if (ipp != null)
                    {
                        ipp.SetData(ce, servername, computer, dirnode);
                    }
                }
            }
        }
    }
    #endregion
    
    #region Overriden Methods
    
    /// <summary>
    /// Method to call the Apply functionality for each of tab pages of type MPage.
    /// </summary>
    /// <param name="actionCause"></param>
    /// <returns></returns>
    protected override bool Apply(EditDialogAction actionCause)
    {
        foreach (MPPage page in this.GetPages())
        {
            if (page != null)
            {
                IDirectoryPropertiesPage ipp = page as IDirectoryPropertiesPage;
                if (page.PageID.Trim().Equals("ObjectAddAttributesTab"))
                {
                    ObjectAddAttributesTab _AddAttributesPage = (ObjectAddAttributesTab)page;
                    if (_AddAttributesPage.OnApply())
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    #endregion
    
    #region Events
    
    private void btnCancel_Click(object sender, EventArgs e)
    {
        base.OnCancel(sender, e);
    }
    
    private void btnOK_Click(object sender, EventArgs e)
    {
        Apply(EditDialogAction.ACTION_OK);
        this.Close();
    }
    
    #endregion
}
}

