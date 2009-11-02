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
using System.Collections;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;
using Likewise.LMC.LDAP;
using System.DirectoryServices;
using Likewise.LMC.Netlogon;


namespace Likewise.LMC.Plugins.LUG
{
public partial class NewGroupDlg : EditDialog, IUIInitialize
{
    #region Class Data
    
    private string _servername = "";
    private LUGPage localParent = null;    
    private Hashtable users = null;
    private Hostinfo _hn = null;
    private LUGPlugIn _plugin = null;
    public static bool Applied = false;
    
    private ListViewColumnSorter lvwColumnSorter;
    
    #endregion
    
    #region Contructors
    public NewGroupDlg(IPlugInContainer container, StandardPage parentPage,Hostinfo hn, IPlugIn plugin)
    : base(container, parentPage)
    {
        InitializeComponent();
        
        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvMembers.ListViewItemSorter = lvwColumnSorter;
        
        this.ButtonCancel.Text = "Cancel";
        
        this.ButtonOK.Text = "Create";
        
        this.SetAllValueChangedHandlers(this);
        
        localParent = (LUGPage)parentPage;
        
        if (localParent == null)
        {
            throw new ArgumentException("NewGroupDlg constructor: (LUGPage) parentPage == null");
        }

        this._hn = hn;
        this._plugin = (LUGPlugIn)plugin;

        ((EditDialog)this).btnApply.Visible = false;
        users = new Hashtable();
        
        this.tbGroupName.Select();
        
    }
    #endregion
    
    #region Interfaces Implementation
    
    public void SetData(string servername, string username)
    {
        Applied = false;
        _servername = servername;
    }
    
    private bool ProcessMembers(CredentialEntry ce, string domain)
    {
        bool retVal = true;
        
        for (int i = 0; i < lvMembers.Items.Count; i++)
        {
            try
            {
                retVal = !Convert.ToBoolean(
                    LUGAPI.NetAddGroupMember(
                        ce,
                        _hn.hostName,
                        this.GroupName,
                        lvMembers.Items[i].Text
                        )
                    );
            }
            catch (Exception)
            {
                retVal = false;
                container.ShowError(
                String.Format("Failed to add user {0} to group {1}", lvMembers.Items[i].Text, this.GroupName));
            }
            
        }
        return retVal;
    }
    
    #endregion
    
    #region override methods
    
    protected override void ValueChangedHandler(object sender, EventArgs e)
    {
        base.ValueChangedHandler(sender, e);
        bDataWasChanged = true;
    }
    
    #endregion
    
    #region Event Handlers
    protected override bool Apply(EditDialogAction actionCause)
    {
        if (Applied && !bDataWasChanged)
        {
            return true;
        }

        bool bRet = true;
        try
        {
            bRet = localParent.AddLUG(this);
            if (!bRet)
            {
                container.ShowError(
                "Likewise Administrative Console encountered an error when trying to add a new group.",
                MessageBoxButtons.OK);
                bDataWasChanged = true;
                return false;
            }
            Hostinfo hn = localParent.GetContext() as Hostinfo;

            bRet = bRet && hn != null && hn.creds != null &&
                   ProcessMembers(hn.creds, localParent.GetDomain());            
        }
        catch (Exception e)
        {            
            bRet = false;
            Logger.LogException("NewGroupDlg.Apply", e);
        }
        
        if (bRet)
        {
            Applied = true;
            Close();
        }        
       
        return bRet;
    }
    
    
    protected bool AddMember(string[] userArr, Object context)
    {
        if (userArr == null || userArr.Length != 1)
        {
            return false;
        }
        
        string user = userArr[0];
        
        // If this button is enabled we have one and only one group selected...
        try
        {
            if (users.ContainsKey(user))
            {
                container.ShowError(
                "Error: Cannot add member which already exists.");
                return false;
            }            
            else
            {
                users.Add(user, 0);
                ListViewItem lvi = new ListViewItem(user);
                lvMembers.Items.Add(lvi);
                return true;
            }
        }
        catch (Exception exp)
        {
            container.ShowError(exp.Message);
        }
        return false;
    }

    private void btnAdd_Click(object sender, EventArgs e)
    {
        try
        {
            //StringRequestDialog strDlg = new StringRequestDialog(
            //AddMember,
            //"Include users in new group",
            //"",
            //new string[]
            //{
            //    "User: "
            //}
            //,
            //new string[]
            //{
            //    "e.g. myUserAcct"
            //}
            //,
            //new string[]
            //{
            //    ""
            //}
            //,
            //null);
            //strDlg.ShowDialog();
            string Domain = string.Empty;
            if (this._hn.domainName == null)
            {
                uint error = CNetlogon.GetCurrentDomain(out _hn.domainName);
                if (error != 0 && String.IsNullOrEmpty(_hn.domainName))
                {
                    this._hn.domainName = Environment.UserDomainName;
                }
            }
            Domain = this._hn.domainName;           
            
            string[] rootDNcom = Domain.Split('.');

            string rootDN = ""; string errorMessage = "";
            foreach (string str in rootDNcom)
            {
                string temp = string.Concat("dc=", str, ",");
                rootDN = string.Concat(rootDN, temp);
            }
            rootDN = rootDN.Substring(0, rootDN.Length - 1);

            //show DsPicker
            if (DirectoryEntry.exisitngDirContext != null && DirectoryEntry.exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext dirContext in DirectoryEntry.exisitngDirContext)
                {
                    if (dirContext != null && dirContext.DistinguishedName.ToLower().Contains(rootDN.ToLower()))
                    {
                        DirectoryEntry.exisitngDirContext.Remove(dirContext);
                        break;
                    }
                }
            }
            
            DirectoryContext _adContext = DirectoryContext.CreateDirectoryContext(Domain,
                                             rootDN,
                                             _hn.creds.UserName,
                                             _hn.creds.Password,
                                             389,
                                             _plugin._usingManualCreds,
                                             out errorMessage);

            DirectoryEntry.exisitngDirContext.Add(_adContext);

            if (String.IsNullOrEmpty(errorMessage))
            {
                Logger.Log("LUGPlugin: Built directory context");
            }
            else
            {
                Logger.ShowUserError(errorMessage);
                return;
            }

            if (_adContext != null)
            {
                Logger.Log("_adContext is not null");
            }
            else
                return;

            string sLdapPath = string.Format("LDAP://{0}/{1}", Domain, rootDN);
            string sProtocol;
            string sServer;
            string sCNs;
            string sDCs;

            System.DirectoryServices.SDSUtils.CrackPath(sLdapPath, out sProtocol, out sServer, out sCNs, out sDCs);
            System.DirectoryServices.Misc.DsPicker f = new System.DirectoryServices.Misc.DsPicker();
            f.SetData(System.DirectoryServices.Misc.DsPicker.DialogType.SELECT_USERS_OR_GROUPS, sProtocol, sServer, sDCs, true);

            if (f.waitForm != null && f.waitForm.bIsInterrupted)
            {
                return;
            }
           
            if (f.ShowDialog(this) == DialogResult.OK)
            {
                if (f.ADobjectsArray != null && f.ADobjectsArray.Length != 0)
                {
                    foreach (System.DirectoryServices.Misc.ADObject ado in f.ADobjectsArray)
                    {
                        string sDN = ado.de.Properties["distinguishedName"].Value as string;
                        string[] TokensDN = sDN.Split(',');
                        string[] members = new string[] { string.Concat(System.Environment.UserDomainName, @"\", TokensDN[0].Substring(3)) };
                        AddMember(members, null);
                    }
                }
            }
        }
        catch (Exception exp)
        {
            container.ShowError(exp.Message);
        }
    }

    private void btnRemove_Click(object sender, EventArgs e)
    {
        // If this button is enabled we have one and only one group selected...
        try
        {
            ListViewItem lvMember = lvMembers.SelectedItems[0];
            string user = lvMember.Text;
            
            if (users.ContainsKey(user))
            {
                users.Remove(user);
            }
            else
            {
                Logger.LogMsgBox(
                "EditSimpleListPage.btnRemove_Click: tried to remove member which does not exist");
                return;
            }
            lvMembers.SelectedItems.Clear();
            lvMembers.Items.Remove(lvMember);
            btnRemove.Enabled = false;    
        }
        catch (Exception exp)
        {
            container.ShowError(exp.Message);
        }
    }
    
    private void lvMembers_SelectedIndexChanged(object sender, EventArgs e)
    {
        ListView lv = sender as ListView;
        if (lv.SelectedItems.Count == 1)
        {
            btnRemove.Enabled = true;
        }
        else
        {
            btnRemove.Enabled = false;
        }
    }
    
    private void lvMembers_ColumnClick(object sender, ColumnClickEventArgs e)
    {
        // Determine if clicked column is already the column that is being sorted.
        if (e.Column == lvwColumnSorter.SortColumn)
        {
            // Reverse the current sort direction for this column.
            if (lvwColumnSorter.Order == SortOrder.Ascending)
            {
                lvwColumnSorter.Order = SortOrder.Descending;
            }
            else
            {
                lvwColumnSorter.Order = SortOrder.Ascending;
            }
        }
        else
        {
            // Set the column number that is to be sorted; default to ascending.
            lvwColumnSorter.SortColumn = e.Column;
            lvwColumnSorter.Order = SortOrder.Ascending;
        }
        
        // Perform the sort with these new sort options.
        this.lvMembers.Sort();
    }
    
    #endregion
    
    #region accessor functions
    
    public string GroupName
    {
        get
        {
            return tbGroupName.Text;
        }
    }
    
    public string Description
    {
        get
        {
            return tbDescription.Text;
        }
    }
    
    #endregion

    private void tbGroupName_TextChanged(object sender, EventArgs e)
    {
        if (!String.IsNullOrEmpty(tbGroupName.Text))
        {
            ValueChangedHandler(sender, e);
        }
        else
        {
            bDataWasChanged = !String.IsNullOrEmpty(tbGroupName.Text);
        }
    }

    private void btnOK_Click(object sender, EventArgs e)
    {
        if (!Applied)
        {
            Apply(EditDialogAction.ACTION_OK);
        }       
    }
    
}
}

