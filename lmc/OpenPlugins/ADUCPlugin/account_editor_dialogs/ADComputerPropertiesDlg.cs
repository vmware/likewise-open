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
using Likewise.LMC.LDAP.Interop;
using System.Threading;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ADComputerPropertiesDlg : MPContainer
{
    #region class data

    private ADUCPlugin _plugin;
    private ObjectPropertyInfo objInfo;
    private IPlugInContainer _container;   
    
    public static bool Applied = false;
    private bool IsCanceled = false;

    #endregion
    
    #region Constructors
    public ADComputerPropertiesDlg(IPlugInContainer container, StandardPage parentPage, ADUCPlugin plugin)
    : base(container, parentPage)
    {
        InitializeComponent();
        this.Text = "{0} Properties";
        _plugin = plugin;
        _container = container;
        InitializePages();
    }
    #endregion
    
    #region Initialization Methods
    
    /// <summary>
    /// Method to initailize the tab pages for the property sheet
    /// </summary>
    private void InitializePages()
    {
        MPPage page = null;
        
        page = new ComputerGeneralEditPage();
        this.AddPage(page,
        new MPMenuItem(page.PageID, "General", "General"),
        MPMenu.POSITION_END);

        page = new ComputerOSEditPage();
        this.AddPage(page,
        new MPMenuItem(page.PageID, "Operating System", "Operating System"),
        MPMenu.POSITION_END
        );
        
        page = new ComputerMemOfPage(this._container, this._plugin);
        this.AddPage(page,
        new MPMenuItem(page.PageID, "Member Of", "Member Of"),
        MPMenu.POSITION_END);
        
        page = new ADEditPage(this);
        this.AddPage(page,
        new MPMenuItem(page.PageID, "Advanced", "Advanced"),
        MPMenu.POSITION_END
        );       
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
        Applied = false;       
        _plugin = dirnode.Plugin as ADUCPlugin;
        this.Text = String.Format(this.Text, computer);

        objInfo = new ObjectPropertyInfo
                                       (ce,
                                       servername,
                                       computer,
                                       dirnode,
                                       null
                                       );

        threadMain = new Thread(new ThreadStart(AddPagesToThread));
        threadMain.Start();

        _plugin.Propertywindowhandles.Add(objInfo.dirnode.DistinguishedName, this);
    }


    private void AddPagesToThread()
    {
        if (this.GetPages() != null)
        {
            ThreadsInner = new List<Thread>();
            foreach (MPPage page in this.GetPages())
            {
                if (IsCanceled)
                    break;

                if (page != null && page is IDirectoryPropertiesPage)
                {
                    ObjectPropertyInfo objInformation = new ObjectPropertyInfo
                                                        (objInfo.ce,
                                                         objInfo.servername,
                                                         objInfo.objectName,
                                                         objInfo.dirnode,
                                                         page);

                    Thread threadInner = new Thread(new ParameterizedThreadStart(doPage_SetData));
                    threadInner.Start(objInformation);
                    threadInner.Join();
                    ThreadsInner.Add(threadInner);
                }
            }
        }
    }

    private void doPage_SetData(object args)
    {
        if (!(args is ObjectPropertyInfo))
        {
            return;
        }

        ObjectPropertyInfo info = args as ObjectPropertyInfo;
        IDirectoryPropertiesPage page = info.page as IDirectoryPropertiesPage;
        if (page != null)
        {
            page.SetData(info.ce, info.servername, info.objectName, info.dirnode);
        }
    }        

    #endregion
    
    #region Event Handlers
    
    /// <summary>
    /// Method to call the Apply functionality for each of tab pages of type MPage.
    /// </summary>
    /// <param name="actionCause"></param>
    /// <returns></returns>
    protected override bool Apply(EditDialogAction actionCause)
    {
        if (Applied && !bDataWasChanged)
        {
            return true;
        }
        
        foreach (MPPage page in this.GetPages())
        {
            if (page != null)
            {
                IDirectoryPropertiesPage ipp = page as IDirectoryPropertiesPage;
                if (page.PageID.Trim().Equals("EditProperitiesAdvanced"))
                {
                    ADEditPage _editPage = (ADEditPage)page;
                    if (!_editPage.OnApply())
                    {
                        return false;
                    }
                }
                if (page.PageID.Trim().Equals("ComputerMemofEditProperities"))
                {
                    ComputerMemOfPage _computerMemPage = (ComputerMemOfPage)page;
                    if (!_computerMemPage.OnApply())
                    {
                        return false;
                    }
                }
                if (page.PageID.Trim().Equals("ComputerGeneralEditProperities"))
                {
                    ComputerGeneralEditPage _computereditPage = (ComputerGeneralEditPage)page;
                    if (!_computereditPage.OnApply())
                    {
                        return false;
                    }
                }
            }
        }
        Applied = true;
        return true;
    }

    private void btnCancel_Click(object sender, EventArgs e)
    {
        IsCanceled = true;

        base.OnCancel(sender, e);
        objInfo.dirnode.LdapContext.Ldap_CancelSynchronous();

        this.Close();
    }
    
    private void btnOK_Click(object sender, EventArgs e)
    {
        //if (!Applied)
        //{
        //    Apply(EditDialogAction.ACTION_OK);
        //}
        if (Applied || !bDataWasChanged)
        {
            StopThreads();
            this.Close();
        }
    }
    
    protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
    {
        if (_plugin.Propertywindowhandles.ContainsKey(objInfo.dirnode.DistinguishedName))
        {
            _plugin.Propertywindowhandles.Remove(objInfo.dirnode.DistinguishedName);
        }
        base.OnClosing(e);
    }
    
    #endregion
    
}
}

