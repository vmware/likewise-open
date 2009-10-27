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
using System.Windows.Forms;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using System.Reflection;
using System.Threading;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class MultiItemPropertiesDlg : MPContainer
    {
        #region Class Data
           
        private ADUCPlugin _plugin;
        private IPlugInContainer _container;
        private ObjectPropertyInfo objInfo;

        public static bool Applied = false;
        private bool IsCanceled = false;

        #endregion

        #region Constructors
        public MultiItemPropertiesDlg(IPlugInContainer container, StandardPage parentPage, ADUCPlugin plugin,
                                      List<object> ObjectCounts)
            : base(container, parentPage)
        {
            InitializeComponent();
            this.ObjectCounts = ObjectCounts;
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

            if (ADUCPage.ADObjectType == "user")
            {                
                page = new UserMultiselectGeneralEditPage(this);
                this.AddPage(page,
                new MPMenuItem(page.PageID, "General", "General"),
                MPMenu.POSITION_BEGINING
                );

                page = new MultiItemsAddressEditPage(this, true);
                this.AddPage(page,
                new MPMenuItem(page.PageID, "Address", "Address"),
                MPMenu.POSITION_BEGINING
                );               
            }
            else if (ADUCPage.ADObjectType == "group")
            {
                page = new MultiItemsGeneralEditPage(this);
                this.AddPage(page,
                new MPMenuItem(page.PageID, "General", "General"),
                MPMenu.POSITION_BEGINING
                );               
            }
            else if (ADUCPage.ADObjectType == "organizationalUnit")
            {
                page = new MultiItemsGeneralEditPage(this);
                this.AddPage(page,
                new MPMenuItem(page.PageID, "General", "General"),
                MPMenu.POSITION_BEGINING
                );                                         
            }
            else
            {
                page = new MultiItemsGeneralEditPage(this);
                this.AddPage(page,
                new MPMenuItem(page.PageID, "General", "General"),
                MPMenu.POSITION_BEGINING
                );
            }
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
        public void SetData(CredentialEntry ce, string servername, string sOU, ADUCDirectoryNode dirnode, LDAPSchemaCache ldapSchemaCache)
        {
            Applied = false;          
            _plugin = dirnode.Plugin as ADUCPlugin;

            objInfo = new ObjectPropertyInfo
                                     (ce,
                                     servername,
                                     sOU,
                                     dirnode,
                                     null);

            threadMain = new Thread(new ThreadStart(AddPagesToThread));
            threadMain.Start();            
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
            //bool b = base.Apply(actionCause);
            foreach (MPPage page in this.GetPages())
            {
                if (page != null)
                {
                    IDirectoryPropertiesPage ipp = page as IDirectoryPropertiesPage;

                    if (page.PageID.Trim().Equals("UserMultiSelectProperities"))
                    {
                        UserMultiselectGeneralEditPage _editPage = (UserMultiselectGeneralEditPage)page;
                        if (!_editPage.OnApply())
                        {
                            return false;
                        }
                    }
                    if (page.PageID.Trim().Equals("UserMultiSelectAddressProperities"))
                    {
                        MultiItemsAddressEditPage _editPage = (MultiItemsAddressEditPage)page;
                        if (!_editPage.OnApply())
                        {
                            return false;
                        }
                    }                    
                    if (page.PageID.Trim().Equals("MultiItemsGeneralEditPage"))
                    {
                        MultiItemsGeneralEditPage _editPage = (MultiItemsGeneralEditPage)page;
                        if (!_editPage.OnApply())
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

            objInfo.dirnode.LdapContext.Ldap_CancelSynchronous();
            base.OnCancel(sender, e);

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

        #endregion

    }
}

