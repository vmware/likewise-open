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
using System.Diagnostics;
using System.Threading;

namespace System.DirectoryServices.Misc
{
    public partial class DsPicker : Form
    {
        #region enum varaibles
        //DsPicker dialogue selection types
        public enum DialogType
        {
            SELECT_USERS = 1,
            SELECT_GROUPS = 2,
            SELECT_COMPUTERS = 4,
            SELECT_LOCAL_USERS = 8,
            SELECT_LOCAL_GROUPS = 16,
            SELECT_ONLY_DOMAIN_USERS = 32,
            SELECT_USERS_OR_GROUPS = 64,
            SELECT_ONLY_DOMAIN_USERS_OR_GROUPS = 128,
            SELECT_USERS_COMPUTERS = 256,            
        }

        //To specify AD object type
        public enum ProviderMemberFilter { GroupMembersOnly, UserMembersOnly, AllMembers, AllDomainMembers, AllUserComputerMembersOnly, AllUserGroupMembersOnly, LocalGroupsOnly };

        // public data
        public enum OperatingMode { Unknown, RFC2307, nonschema };

        #endregion

        #region Class data
        //DerectirySearcher page limit
        const int SEARCHER_PAGE_SIZE = 1000;

        private ADObject[] adoo = null;
        private ListViewColumnSorter lvwColumnSorter;

        string sDomainName = string.Empty;

        //private System.Object lockThis_setData = new System.Object(); 
        // Used to display progress to user...
        public WaitForm waitForm = null;
        int Count = 0;
        int percentDone = 0;
        private BackgroundWorker bw = null;      
       
        private DirectoryEntry de = null;
        private Dictionary<string, DirectoryEntry> deList = null;
        private DialogType dt = DialogType.SELECT_USERS_OR_GROUPS;

        public string groupScope = string.Empty;
        private Form parentHandle = null;
        private bool formResult = false;
             
        #endregion

        #region Accessors

        public ADObject[] ADobjectsArray
        {
            set
            {
                adoo = value;
            }
            get
            {
                return adoo;
            }
        }

        public bool FormResult
        {
            set
            {
                formResult = value;
            }
            get
            {
                return formResult;
            }
        }

        #endregion

        #region Constructors

        public DsPicker()
        {

            //MessageBox.Show("Please wait while ADUC-plugin is loading users/groups information.", "Likewise Administrative Console", MessageBoxButtons.OK,
                           //MessageBoxIcon.Information);  
            InitializeComponent();             

            // Create an instance of a ListView column sorter and assign it 
            // to the ListView control.
            lvwColumnSorter = new ListViewColumnSorter();
            this.lvUserToGroup.ListViewItemSorter = lvwColumnSorter;
            lvwColumnSorter.Order = SortOrder.Ascending;

           
        }

        public DsPicker(Form HWND)
            : this()
        {
            this.parentHandle = HWND;
        }

        public DsPicker(string groupScope)
            : this()
        {
            this.groupScope = groupScope;
        }

       // public DsPicker(DialogType dt,string sProtocal,string targetServer,string sDCs,bool allowMultiSelect):this()
        public void SetData(DialogType dt, string sProtocal, string targetServer, string sDCs, bool allowMultiSelect)
        {
            if (timer.Enabled)
                timer.Start();
            else
            {
                timer.Enabled = true;
                timer.Start();
            }
            lvUserToGroup.MultiSelect = allowMultiSelect;           
            // if no server specified, try to synthesize it from the dc's
            if (targetServer == null && sDCs != null)
                targetServer = SDSUtils.DNToDomainName(sDCs);

            sDomainName = SDSUtils.DNToDomainName(sDCs);

            string sPath = SDSUtils.MakePath(sProtocal, targetServer, null, sDCs);
            de = new DirectoryEntry(sPath);

            deList = new Dictionary<string, DirectoryEntry>();
            this.dt = dt;

            try
            {
                waitForm = new WaitForm(backgroundWorker, timer, de);               
                backgroundWorker.RunWorkerAsync(null);

                //System.Threading.Thread.Sleep(1000);
                if (waitForm != null)
                    waitForm.ShowDialog(this);
                //this.lvUserToGroup.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Likewise Administrator Console",
                                    MessageBoxButtons.OK);
            } 

            this.CancelBtn.Enabled = true;
            lvUserToGroup.Enabled = true;
            lvUserToGroup.Sort();
        }

        public DialogResult invokeDialog(Control parentHandle)
        {
            if (this.ShowDialog(parentHandle) == DialogResult.OK)
                return DialogResult.OK;
            else
            {
                return DialogResult.None;
            }
        }

        #endregion

        #region Events

        private void Okbtn_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            
            this.Close();
            if (lvUserToGroup.SelectedItems.Count == 0)
                return;

            adoo = new ADObject[lvUserToGroup.SelectedItems.Count];
            int i=0;
            foreach (ListViewItem lvitem in lvUserToGroup.SelectedItems)
            {
                DirectoryEntry deEntry = lvitem.Tag as DirectoryEntry;
                if (deEntry != null)
                {
                    adoo[i].de = deEntry;
                    adoo[i].ADsPath = deEntry.Path;
                    adoo[i].ClassName = deEntry.SchemaClassName;
                    adoo[i].Name = deEntry.Properties["cn"].Value as string;
                    adoo[i++].UPN = deEntry.Properties["userPrincipalName"].Value as string;
                }
            }
        }

        private void CancelBtn_Click(object sender, EventArgs e)
        {  
          

            this.Close();
        }

        private void lvUserToGroup_SelectedIndexChanged(object sender, EventArgs e)
        {
            Okbtn.Enabled = lvUserToGroup.SelectedItems.Count != 0;
        }

        private void lvUserToGroup_ColumnClick(object sender, ColumnClickEventArgs e)
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
            this.lvUserToGroup.Sort();
        }

        #region Worker Threads

        private void backgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            if (waitForm.bIsInterrupted && this.backgroundWorker.IsBusy)
            {
                e.Cancel = true;
                this.backgroundWorker.CancelAsync();
                timer.Stop();
            }
            else
            {
                this.bw = (BackgroundWorker)sender;

                try
                {
                    Thread thread = new Thread(new ParameterizedThreadStart(GetUserGroupList));
                    thread.Start(bw as object);
                    thread.Join();
                }
                catch { }
            }
        }

        private void backgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            if (waitForm == null)
                return;

            if (waitForm.bIsInterrupted && this.backgroundWorker.CancellationPending)
            {
                timer.Stop();               
                this.backgroundWorker.CancelAsync();                              
                waitForm.Close();
                waitForm = null;
                return;
            }
            if (e.UserState != null)
            {
                Exception ex = e.UserState as Exception;
                if (ex != null)
                {
                    timer.Stop();
                    this.backgroundWorker.CancelAsync();                  
                    return;
                }

                string statusString = e.UserState as string;
                if (statusString != null)
                {
                    waitForm.labelStatus.Text = statusString;
                }
            }
            waitForm.pb.Value = e.ProgressPercentage;
            waitForm.pb.Update();
        }

        private void backgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {           
            timer.Stop();
            timer.Enabled = false;
            if (waitForm != null)
            {
                waitForm.Close();
                waitForm = null;
            }           
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            try
            {
                Count += 10;
                if (Count > 119)
                    Count = 10;
                percentDone = Count;

                if (bw.IsBusy)
                {
                    bw.ReportProgress(percentDone, (Object)"Please wait while plugin is loading users/groups information");
                }
            }
            catch
            { }
        }

        #endregion

        #endregion

        #region Helper Methods

        private void SetListviewColumns()
        {
            string[] columnLabels = null;
            if (lvUserToGroup.Columns.Count != 0)
                lvUserToGroup.Columns.Clear();

            if (dt == DialogType.SELECT_USERS_OR_GROUPS ||
                dt== DialogType.SELECT_ONLY_DOMAIN_USERS_OR_GROUPS ||
                dt== DialogType.SELECT_USERS ||
                dt==DialogType.SELECT_USERS_COMPUTERS)
            {
                columnLabels = new string[] { "Name(RDN)", "E-Mail Address", "Description", "In Folder" };
            }
            else if (dt == DialogType.SELECT_GROUPS)
            {
                columnLabels = new string[] { "Name(RDN)", "Description", "In Folder" };
            }
            else
            {
                columnLabels = new string[] { "Name(RDN)", "E-Mail Address", "Description", "In Folder", "Logon Name" };
            }
                       
            int numColumns = columnLabels.Length;
            ColumnHeader[] columnHeaders = new ColumnHeader[numColumns];
            for (int i = 0; i < numColumns; i++)
            {
                columnHeaders[i] = new ColumnHeader();
                columnHeaders[i].Text = columnLabels[i];
                columnHeaders[i].Width = 100;
            }

            this.lvUserToGroup.Columns.AddRange(columnHeaders);

            if (this.lvUserToGroup.Columns.Count == 1)
                lvUserToGroup.Columns[numColumns - 1].Width = this.Width;            
            lvUserToGroup.Show();
        }

        private void GetUserGroupList(object args)
        {
            if (!(args is BackgroundWorker))
            {
                return;
            }
            bw = args as BackgroundWorker;
            
            bw.ReportProgress(percentDone, (Object)"Please wait while plugin is loading users/groups information");
            SetListviewColumns();
            switch (dt)
            {
                case DialogType.SELECT_USERS:
                    Text = "Select Users";
                    this.textBoxDomain.Text = sDomainName;
                    this.textBoxObjectType.Text = "Users";
                    deList = GetAllMembers(de, ProviderMemberFilter.UserMembersOnly, int.MaxValue, bw);
                    break;
                case DialogType.SELECT_GROUPS:
                    Text = "Select Groups";
                    this.textBoxDomain.Text = sDomainName;
                    this.textBoxObjectType.Text = "Groups";
                    deList = GetAllMembers(de, ProviderMemberFilter.GroupMembersOnly, int.MaxValue, bw);
                    break;
                case DialogType.SELECT_LOCAL_GROUPS:
                    Text = "Select Groups";
                    this.textBoxDomain.Text = sDomainName;
                    this.textBoxObjectType.Text = "Groups";
                    deList = GetAllMembers(de, ProviderMemberFilter.LocalGroupsOnly, int.MaxValue, bw);
                    break;
                case DialogType.SELECT_USERS_OR_GROUPS:
                    //Text = "Select Users or Groups";
                    this.textBoxDomain.Text = sDomainName;
                    this.textBoxObjectType.Text = "Users or Groups";
                    deList = GetAllMembers(de, ProviderMemberFilter.AllUserGroupMembersOnly, int.MaxValue, bw);
                    break;
                case DialogType.SELECT_ONLY_DOMAIN_USERS_OR_GROUPS:
                    Text = "Select Domain Users or Groups";
                    this.textBoxDomain.Text = sDomainName;
                    this.textBoxObjectType.Text = "Domain Users or Groups";
                    //this might be changed to domain users/groups
                    deList = GetAllMembers(de, ProviderMemberFilter.AllMembers, int.MaxValue, bw);
                    break;
                case DialogType.SELECT_USERS_COMPUTERS:
                    Text = "Select Users and Computers";
                    this.textBoxDomain.Text = sDomainName;
                    this.textBoxObjectType.Text = "Users and Computers";
                    deList = GetAllMembers(de, ProviderMemberFilter.AllUserComputerMembersOnly, int.MaxValue, bw);
                    break;
                default: break;
            }
            if (deList != null && deList.Count != 0)
            {
                ListViewItem[] lvitems = new ListViewItem[deList.Count];
                int index = 0;
                foreach (string dn in deList.Keys)
                {
                    string[] itemsTodisplay = null;
                    string aPartName = string.Empty;
                    string[] sItems = splitDn(dn, aPartName);
                    DirectoryEntry entry = deList[dn];
                    string email = string.Empty;
                    if (entry.Properties["mail"].Value != null)
                        email = entry.Properties["mail"].Value as string;
                    string desc = string.Empty;
                    if (entry.Properties["description"].Value != null)
                        desc = entry.Properties["description"].Value as string;
                    string logonname = "";
                    if (entry.Properties["sAMAccountName"].Value != null)
                        logonname = entry.Properties["sAMAccountName"].Value as string;
                    if (dt == DialogType.SELECT_GROUPS)
                    {
                        itemsTodisplay = new string[] { sItems[0], desc, sItems[1] };
                    }
                    else if (dt == DialogType.SELECT_USERS_OR_GROUPS ||
                             dt == DialogType.SELECT_ONLY_DOMAIN_USERS_OR_GROUPS ||
                             dt == DialogType.SELECT_USERS ||
                             dt == DialogType.SELECT_USERS_COMPUTERS)
                             
                    {
                        itemsTodisplay = new string[] { sItems[0], email, desc, sItems[1] };
                    }
                    else
                    {
                        itemsTodisplay = new string[] { sItems[0], email, desc, sItems[1], logonname };
                    }
                    ListViewItem lvItem = new ListViewItem(itemsTodisplay);
                    lvItem.Tag = deList[dn];
                    lvitems[index++] = lvItem;
                }
                lvUserToGroup.Items.AddRange(lvitems);
            }          
        }

        public DirectoryEntry GetRootDSE(string serverOrDomain)
        {
            DirectoryEntry de;
            de = new DirectoryEntry(SDSUtils.MakePath(null, serverOrDomain, "RootDSE", null));

            return de;
        }

        public Dictionary<string, DirectoryEntry> GetAllMembers(DirectoryEntry deTop, ProviderMemberFilter filter, int maxResultSetSize,BackgroundWorker bw)
        {
            // map SIDs to AD user object
            Dictionary<string, DirectoryEntry> adUserGroupMap = new Dictionary<string, DirectoryEntry>();
            int count = 0;
            string filterString = "";
            string objecttype = "";

            // yep, let's get the default cell data first

            if (filter == ProviderMemberFilter.AllUserGroupMembersOnly)
            {
                if (groupScope == null || groupScope == string.Empty)
                {
                    filterString = "(|(objectClass=user)(objectClass=group))"; //"(|(&(objectClass=user)(uidNumber=*))(&(objectClass=group)(gidNumber=*)))";
                }
                else
                {
                    filterString = "(|(&(objectClass=group)(grouptype=-2147483640))(&(objectClass=group)(grouptype=8))" +
                                   "(&(objectClass=group)(grouptype=-2147483646))(&(objectClass=group)(grouptype=2))" +
                                   "(&(objectClass=group)(grouptype=-2147483644))(&(objectClass=group)(grouptype=4))(objectClass=user))";
                }
                objecttype = "usergroup";
            }
            else if (filter == ProviderMemberFilter.GroupMembersOnly)
            {
                if (groupScope == null || groupScope == string.Empty)
                    filterString = "(objectClass=group)";// "(&(objectClass=group)(gidNumber=*))";
                else if (groupScope == "-2147483644" || groupScope == "4")
                    filterString = "(|(&(objectClass=group)(grouptype=-2147483644))(&(objectClass=group)(grouptype=4)))";
                else if (groupScope == "-2147483640" || groupScope == "8")
                    filterString = "(|(&(objectClass=group)(grouptype=-2147483640))(&(objectClass=group)(grouptype=8))" +
                        "(&(objectClass=group)(grouptype=-2147483644))(&(objectClass=group)(grouptype=4)))";
                else if (groupScope == "-2147483646" || groupScope == "2")
                    filterString = "(objectClass=group)";

                objecttype = "group";
            }
            else if (filter == ProviderMemberFilter.LocalGroupsOnly)
            {
                filterString = "(|(&(objectClass=group)(grouptype=-2147483640))(&(objectClass=group)(grouptype=8))" +
                               "(&(objectClass=group)(grouptype=-2147483646))(&(objectClass=group)(grouptype=2))" +
                               "(&(objectClass=group)(grouptype=-2147483644))(&(objectClass=group)(grouptype=4)))";

                objecttype = "group";
            }
            else if (filter == ProviderMemberFilter.UserMembersOnly)
            {
                filterString = "(objectClass=user)";//"(&(objectClass=user)(uidNumber=*))";
                objecttype = "user";
            }
            else if (filter == ProviderMemberFilter.AllMembers)
            {
                if (groupScope == null || groupScope == string.Empty)
                {
                    filterString = "(|(objectClass=user)(objectClass=group)(objectClass=computer))";//"(|(&(objectClass=user)(uidNumber=*))(&(objectClass=group)(gidNumber=*)))";
                }
                else
                {
                    if (groupScope == "-2147483644" || groupScope == "4")
                    {
                        filterString = "(|(&(objectClass=group)(grouptype=-2147483644))(&(objectClass=group)(grouptype=4))(objectClass=user))";
                    }
                    else if (groupScope == "-2147483640" || groupScope == "8")
                    {
                        filterString = "(|(&(objectClass=group)(grouptype=-2147483640))(&(objectClass=group)(grouptype=8))" +
                            "(&(objectClass=group)(grouptype=-2147483644))(&(objectClass=group)(grouptype=4))(objectClass=user))";
                    }
                    else if (groupScope == "-2147483646" || groupScope == "2")
                    {
                        filterString = "(|(objectClass=user)(objectClass=group))";
                    }
                    else if (groupScope == "-2147483643")
                    {
                        filterString = "(|(&(objectClass=group)(grouptype=-2147483640))(&(objectClass=group)(grouptype=8))" +
                            "(&(objectClass=group)(grouptype=-2147483646))(&(objectClass=group)(grouptype=2))(objectClass=user))";
                    }
                }
                objecttype = "domainusergroup";
            }
            else if (filter == ProviderMemberFilter.AllUserComputerMembersOnly)
            {
                filterString = "(|(objectClass=user)(objectClass=computer))";
            }

            DirectorySearcher dsT = new DirectorySearcher(deTop, filterString);
            dsT.SearchScope = SearchScope.Subtree;
            dsT.PageSize = SEARCHER_PAGE_SIZE;

            SearchResultCollection src = null;

            try
            {
                src = dsT.FindAll();              

                foreach (SearchResult sr in src)
                {
                    if (count == maxResultSetSize)
                        return adUserGroupMap;

                    DirectoryEntry o = new DirectoryEntry(sr.Path);
                    if (filter == ProviderMemberFilter.AllMembers)
                    {
                        if (o != null && (o.SchemaClassName.Equals("user") || (o.SchemaClassName.Equals("group")) || (o.SchemaClassName.Equals("computer"))))
                        {
                            adUserGroupMap.Add(o.Properties["distinguishedName"].Value as string, o);
                            count++;
                        }
                    }
                    else if (filter == ProviderMemberFilter.AllUserComputerMembersOnly)
                    {
                        if (o != null && (o.SchemaClassName.Equals("user") || (o.SchemaClassName.Equals("computer"))))
                        {
                            adUserGroupMap.Add(o.Properties["distinguishedName"].Value as string, o);
                            count++;
                        }
                    }
                    else if (filter == ProviderMemberFilter.AllUserGroupMembersOnly)
                    {
                        if (o != null && (o.SchemaClassName.Equals("user") || (o.SchemaClassName.Equals("group"))))
                        {
                            adUserGroupMap.Add(o.Properties["distinguishedName"].Value as string, o);
                            count++;
                        }
                    }
                    else
                    {
                        if (o != null && o.SchemaClassName.Equals(objecttype))
                        {
                            adUserGroupMap.Add(o.Properties["distinguishedName"].Value as string, o);
                            count++;
                        }
                    }                  
                }
            }
            catch (Exception aex)
            {
                bw.ReportProgress(percentDone, (Object)aex);
            }
            finally
            {
                src.Dispose();
            }
            return adUserGroupMap;
        }

        public void GetGroupsLevel(DirectoryEntry de)
        {

        }

        /*private Dictionary<string, DirectoryEntry> GetAllUserGroupList(DirectoryEntry entry,int maxResultSetSize)
        {
            if (entry == null)
                return null;

            Dictionary<string, DirectoryEntry> objectlist=new Dictionary<string,DirectoryEntry>();

            foreach (DirectoryEntry o in entry.Children)
            {
                if (count == maxResultSetSize)
                    return objectlist;

                // only tweak if we need to                   
                if (o != null && !o.Properties["objectClass"].Contains("user"))
                {
                    objectlist.Add(o.Properties["distinguishedName"].Value as string, o);
                    count++;
                }
                else if (o.Children.Count > 0)
                    GetAllUserGroupList(Chnode, maxResultSetSize);
            }           
        }*/

        /// <summary>
        /// Splits the AD Object names from the distinguishedName
        /// </summary>
        /// <param name="groupDn"></param>
        /// <param name="aPartName"></param>
        /// <returns></returns>
        public static string[] splitDn(string groupDn, string aPartName)
        {
            string[] aParts = groupDn.Split(new char[] { ',' });
            string aPartAcitveDirectoryFolder = string.Empty, aPart = string.Empty;

            if (aParts[0].StartsWith("CN=", StringComparison.InvariantCultureIgnoreCase))
                aPartName = aParts[0].Substring(3);
            if (aParts[1].StartsWith("CN=", StringComparison.InvariantCultureIgnoreCase) ||
                aParts[1].StartsWith("OU=", StringComparison.InvariantCultureIgnoreCase))
                aPart = aParts[1].Substring(3);
            else
                aPart = aParts[1];
            for (int i = 2; i < aParts.Length; i++)
            {
                if (aParts[i].StartsWith("dc=", StringComparison.InvariantCultureIgnoreCase))
                    aPartAcitveDirectoryFolder += "." + aParts[i].Substring(3);
            }

            if (aPartAcitveDirectoryFolder.StartsWith("."))
                aPartAcitveDirectoryFolder = aPartAcitveDirectoryFolder.Substring(1).Trim();

            if (!string.IsNullOrEmpty(aPartAcitveDirectoryFolder))
                aPartAcitveDirectoryFolder = aPartAcitveDirectoryFolder + "/" + aPart;

            string[] slvItem = { aPartName, aPartAcitveDirectoryFolder };

            return slvItem;
        }

        #endregion   
    }

    //ADObject used to make SDS ADObject to send as result from the dailogue
    public struct ADObject
    {
        public string ADsPath;
        public string ClassName;
        public string Name;
        public string UPN;
        public DirectoryEntry de;
    }

    /// <summary>
    /// This class is an implementation of the 'IComparer' interface.
    /// </summary>
    public class ListViewColumnSorter : IComparer
    {
        #region Class data
        /// <summary>
        /// Specifies the column to be sorted
        /// </summary>
        private int ColumnToSort;
        /// <summary>
        /// Specifies the order in which to sort (i.e. 'Ascending').
        /// </summary>
        private SortOrder OrderOfSort;
        /// <summary>
        /// Case insensitive comparer object
        /// </summary>
        private CaseInsensitiveComparer ObjectCompare;
        #endregion

        #region Constructor
        /// <summary>
        /// Class constructor.  Initializes various elements
        /// </summary>
        public ListViewColumnSorter()
        {
            // Initialize the column to '0'
            ColumnToSort = 0;

            // Initialize the sort order to 'none'
            OrderOfSort = SortOrder.None;

            // Initialize the CaseInsensitiveComparer object
            ObjectCompare = new CaseInsensitiveComparer();
        }
        #endregion

        #region helper function
        /// <summary>
        /// This method is inherited from the IComparer interface.  It compares the two objects passed using a case insensitive comparison.
        /// </summary>
        /// <param name="x">First object to be compared</param>
        /// <param name="y">Second object to be compared</param>
        /// <returns>The result of the comparison. "0" if equal, negative if 'x' is less than 'y' and positive if 'x' is greater than 'y'</returns>
        public int Compare(object x, object y)
        {
            int compareResult;
            ListViewItem listviewX, listviewY;

            // Cast the objects to be compared to ListViewItem objects
            listviewX = (ListViewItem)x;
            listviewY = (ListViewItem)y;

            // Compare the two items
            compareResult = ObjectCompare.Compare(listviewX.SubItems[ColumnToSort].Text, listviewY.SubItems[ColumnToSort].Text);

            // Calculate correct return value based on object comparison
            if (OrderOfSort == SortOrder.Ascending)
            {
                // Ascending sort is selected, return normal result of compare operation
                return compareResult;
            }
            else if (OrderOfSort == SortOrder.Descending)
            {
                // Descending sort is selected, return negative result of compare operation
                return (-compareResult);
            }
            else
            {
                // Return '0' to indicate they are equal
                return 0;
            }
        }
        #endregion

        #region accessor functions
        /// <summary>
        /// Gets or sets the number of the column to which to apply the sorting operation (Defaults to '0').
        /// </summary>
        public int SortColumn
        {
            set
            {
                ColumnToSort = value;
            }
            get
            {
                return ColumnToSort;
            }
        }

        /// <summary>
        /// Gets or sets the order of sorting to apply (for example, 'Ascending' or 'Descending').
        /// </summary>
        public SortOrder Order
        {
            set
            {
                OrderOfSort = value;
            }
            get
            {
                return OrderOfSort;
            }
        }
        #endregion
    }    
}