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
using Likewise.LMC.Utilities;
using Likewise.LMC.Eventlog;

namespace Likewise.LMC.Plugins.EventlogPlugin
{
public partial class CustomizeViewForm : Form
{
    #region Class data
    
    private bool bDefaults;
    
    #endregion
    
    
    public CustomizeViewForm()
    {
        InitializeComponent();
    }
    
    public CustomizeViewForm(EventCustomize ec)
    : this()
    {
        // either set the defaults or initialize as per the existing settings
        if (ec == null)
        {
            SetDefaults();
        }
        else
        {
            Initialize(ec);
        }
    }
    
    
    #region Helper functions
    
    /// <summary>
    /// Sets the UI widgets to the default (no filtering) values
    /// </summary>
    private void SetDefaults()
    {
        lmcCustomView.ConsoleTree = true;
        lmcCustomView.StandardMenus = true;
        lmcCustomView.StandardToolbar = true;
        lmcCustomView.StatusBar = true;
        lmcCustomView.TaskpadNavigationpads = true;
        
        cbMenus.Checked = cbToolbars.Checked = true;
        
        bDefaults = true;
    }
    
    /// <summary>
    /// Sets the UI widgets as per a given filter object
    /// </summary>
    /// <param name="ef">The given filter object</param>
    private void Initialize(EventCustomize ec)
    {
        bDefaults = false;
        
        lmcCustomView.ConsoleTree = ec.ConsoleTree;
        lmcCustomView.StandardMenus = ec.StandardMenus;
        lmcCustomView.StandardToolbar = ec.StandardToolbar;
        lmcCustomView.StatusBar = ec.StatusBar;
        lmcCustomView.TaskpadNavigationpads = ec.TaskpadNavigationpads;
        cbToolbars.Checked = ec.Toolbars;
        cbMenus.Checked = ec.Menus;
    }
    
    #endregion
    
    #region Property Accessors
    
    /// <summary>
    /// Returns a Customizeview object that represents the user settings
    /// </summary>
    public EventCustomize Customizeview
    {
        get
        {
            // if nothing's changed, return a null
            if (bDefaults)
            {
                return null;
            }
            
            EventCustomize ec = new EventCustomize();
            
            ec.ConsoleTree = lmcCustomView.ConsoleTree;
            ec.StandardMenus = lmcCustomView.StandardMenus;
            ec.StandardToolbar = lmcCustomView.StandardToolbar;
            ec.StatusBar = lmcCustomView.StatusBar;
            ec.TaskpadNavigationpads = lmcCustomView.TaskpadNavigationpads;
            ec.Toolbars = cbToolbars.Checked;
            ec.Menus = cbMenus.Checked;
            
            return ec;
        }
    }
    
    #endregion
    
    private void btnOk_Click(object sender, EventArgs e)
    {
        DialogResult = DialogResult.OK;
        Close();
    }
    
    private void SettingsChanged(object sender, EventArgs e)
    {
        bDefaults = false;
    }
}

public class EventCustomize
{
    #region Class data
    
    // booleans for the various types
    private bool bConsoletree = true;
    private bool bStandardmenus = true;
    private bool bStandardtoolbar = true;
    private bool bstatusbar = true;
    private bool btaskpadnavigationpads = true;
    private bool bmenus = true;
    private bool btoolbars = true;
    
    #endregion
    
    #region Constructor
    
    
    #endregion
    
    #region Property Accessors
    
    // These are all straightforward get/set pairs
    
    public bool ConsoleTree
    {
        get
        {
            return bConsoletree;
        }
        set
        {
            bConsoletree = value;
        }
    }
    
    public bool StandardMenus
    {
        get
        {
            return bStandardmenus;
        }
        set
        {
            bStandardmenus = value;
        }
    }
    
    public bool StandardToolbar
    {
        get
        {
            return bStandardtoolbar;
        }
        set
        {
            bStandardtoolbar = value;
        }
    }
    
    public bool StatusBar
    {
        get
        {
            return bstatusbar;
        }
        set
        {
            bstatusbar = value;
        }
    }
    
    public bool TaskpadNavigationpads
    {
        get
        {
            return btaskpadnavigationpads;
        }
        set
        {
            btaskpadnavigationpads = value;
        }
    }
    
    public bool Menus
    {
        get
        {
            return bmenus;
        }
        set
        {
            bmenus = value;
        }
    }
    
    public bool Toolbars
    {
        get
        {
            return btoolbars;
        }
        set
        {
            btoolbars = value;
        }
    }
    #endregion
}
}
