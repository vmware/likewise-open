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
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.Eventlog;

namespace Likewise.LMC.Plugins.EventlogPlugin
{
public partial class EventPropertiesDlg : MPContainer
{
    #region class data
    
    private EventlogPlugin _plugin;
    private IPlugInContainer _container;
    private bool Applied = false;
    
    // back reference to the log being viewed and the
    // datagrid that is viewing it (so that we can
    // navigate up and down on it)
    private ListView _eventsListView;
    
    #endregion
    
    #region Constructors
    
    public EventPropertiesDlg(IPlugInContainer container, StandardPage parentPage, EventlogPlugin plugin,ListView lvEvents)
    : base(container, parentPage)
    {
        InitializeComponent();
        btnApply.Visible = false;
        btnCancel.Visible = false;
        btnOK.Location = btnApply.Location;
        this.Text = "Event Properties";
        _plugin = plugin;
        _container = container;
        _eventsListView = lvEvents;
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
        
        page = new EventPropertiesPage(_eventsListView);
        this.AddPage(page,
        new MPMenuItem(page.PageID, "Event", "Event"),
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
    public void SetData()
    {
        if (this.GetPages() != null)
        {
            foreach (MPPage page in this.GetPages())
            {
                if (page != null)
                {
                    IDirectoryPropertiesPage ipp = page as IDirectoryPropertiesPage;
                    if (ipp != null)
                    {
                        ipp.SetData();
                    }
                }
            }
        }
    }
    #endregion
    
    
    #region Event Handlers
    
    private void btnOK_Click(object sender, EventArgs e)
    {
        if (!Applied)
        {
            Apply(EditDialogAction.ACTION_OK);
        }
        this.Close();
    }
    
    #endregion
}
}
