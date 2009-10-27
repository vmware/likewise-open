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
using System.Collections;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.FileShareManager
{
public partial class NewShareWizardDlg : Likewise.LMC.ServerControl.WizardDialog
{
    #region Class Data

    public ShareInfo shareInfo;
    private FileShareManagerIPlugIn plugin;
    public List<ShareInfo> sharedFolderList;
    
    #endregion
    
    #region Constructors  
    
    public NewShareWizardDlg()
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
    public NewShareWizardDlg(IPlugInContainer container, StandardPage parentPage, IPlugIn plugin)
        : this()
    {
        this.plugin = plugin as FileShareManagerIPlugIn;      
        this.shareInfo = new ShareInfo();
        shareInfo.hostName = this.plugin.HostInfo.hostName;
        sharedFolderList = new List<ShareInfo>();

        this.AddPage(new NewShareWelcomePage(this, plugin, container));
        this.AddPage(new NewShareFolderSetUpPage(this, plugin, container));
        this.AddPage(new NewSharePermissionsPage(this, plugin, container));
        this.AddPage(new NewShareFinishPage(this, plugin, container));
    }

    #endregion
}

    public class ShareInfo
    {
        #region Class Data

        public string hostName = string.Empty;
        public string folderName = string.Empty;
        public string shareName = string.Empty;
        public string shareDesc = string.Empty;

        public string perUserList = string.Empty;
        public bool AdminFullPermissions = false;
        public bool userReadOnly = false;
        public bool UserNoAccess = false;
        public bool CustomPermissions = false;

        public bool commitChnages = false;

        #endregion
    }
}

