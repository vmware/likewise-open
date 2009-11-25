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
using System.Drawing;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.LUG
{
    public partial class EditSimpleListPage : MPPage, IPropertiesPage
    {
        #region class data
        private string _servername;
        private string _subjectname;

        private Hashtable members = null;
        private Hashtable membersAdded = null;
        private Hashtable membersDeleted = null;

        public delegate string GetSubjectDescriptionDelegate();
        public delegate string[] GetListMembersDelegate();
        public delegate bool DoAddMemberDelegate(string member);
        public delegate bool DoDelMemberDelegate(string member);

        private readonly GetSubjectDescriptionDelegate getSubjectDescription;
        private readonly GetListMembersDelegate getListMembers;
        private readonly DoAddMemberDelegate addMember;
        private readonly DoDelMemberDelegate delMember;

        private EditDialog editDialog;

        private string listOwnerType = "list";
        private string listMemberType = "member";
        #endregion

        #region Constructors

        public EditSimpleListPage(
                string pageID, 
                string subjectName,
                Icon displayIcon,
                string ownerType, 
                string subjectListRelationship, 
                string memberType, 
                GetSubjectDescriptionDelegate getSubjectDescription,
                GetListMembersDelegate getListMembers,
                DoAddMemberDelegate addMember, 
                DoDelMemberDelegate delMember, 
                EditDialog editDialog)
        {
            this.pageID = pageID;
            InitializeComponent();
            lbSubject.Text = String.Format(lbSubject.Text, subjectName);
            lbSubjectListRelationship.Text = String.Format(lbSubjectListRelationship.Text, subjectListRelationship);
            pbSubjectImage.Image = displayIcon.ToBitmap();
            this.getSubjectDescription = getSubjectDescription;
            this.getListMembers = getListMembers;
            this.addMember = addMember;
            this.delMember = delMember;

            this.listOwnerType = ownerType;
            this.listMemberType = memberType;

            this.editDialog = editDialog;

            this.tbDescription.Select();
        }
        #endregion

        #region Initialization Methods
        public void SetData(CredentialEntry ce, string servername, string subjectname)
        {
            _servername = servername;
            _subjectname = subjectname;

            this.tbDescription.Text = getSubjectDescription();

            string[] membersArr = getListMembers();

            members = new Hashtable();
            membersAdded = new Hashtable();
            membersDeleted = new Hashtable();

            if(membersArr != null)
            {
                foreach (string member in membersArr)
                {
                    if (member != null)
                    {
                        if (members.ContainsKey(member))
                        {
                            Logger.Log(String.Format(
                                "EditSimpleListPage.SetData: cannot add duplicate member!: {0}",
                                member), 
                                Logger.LogLevel.Error);
                        }
                        else
                        {
                            members.Add(member, 0);
                        }
                    }    
                }
            }

            PopulateListView();

        }
        #endregion

        #region helper methods
        public void ApplyMembers()
        {
            foreach (string deleted in membersDeleted.Keys)
            {
                if (members.ContainsKey(deleted))
                {
                    members.Remove(deleted);
                    delMember(deleted);
                }
                else
                {
                    Logger.LogMsgBox("EditSimpleListPage.applyMembers: membersDeleted contains an element not in members");
                }
            }
            foreach (string added in membersAdded.Keys)
            {
                if (!members.ContainsKey(added))
                {
                    members.Add(added, 0);
                    addMember(added);
                }
                else
                {
                    Logger.LogMsgBox("EditSimpleListPage.applyMembers: membersAdded contains an element already in members");
                }
            }
            PopulateListView();

        }

        protected bool AddMember(string[] membersArg, Object context)
        {
            if (membersArg == null || membersArg[0] == null)
            {
                return false;
            }
            string member = membersArg[0];

            // If this button is enabled we have one and only one group selected...
            try
            {
                if(membersDeleted.ContainsKey(member))
                {
                    membersDeleted.Remove(member);
                    ListViewItem lvi = new ListViewItem(member);
                    lvMembers.Items.Add(lvi);
                    return true;
                }
                else if (members.ContainsKey(member))
                {
                    container.ShowError("Error: Cannot add member which already exists.");
                    return false;
                }
                else if (membersAdded.ContainsKey(member))
                {
                    container.ShowError("Error: Cannot add member which has already been added.");
                    return false;
                }
                else
                {
                    membersAdded.Add(member, 0);
                    ListViewItem lvi = new ListViewItem(member);
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

        private void PopulateListView()
        {
            lvMembers.Items.Clear();
            if (members.Count > 0)
            {
                ListViewItem[] lvItemArr = new ListViewItem[members.Count];
                int i = 0;

                foreach (string m in members.Keys)
                {
                    lvItemArr[i] = new ListViewItem(m);
                    lvItemArr[i].Tag = "";

                    i++;
                }

                lvMembers.Items.AddRange(lvItemArr);
            }
            
        }

        #endregion


        #region accessor functions

        public string SubjectName
        {
            get
            {
                return this._subjectname;
            }
        }

        public string Description
        {
            get
            {
                return this.tbDescription.Text;
            }
            set
            {
                this.tbDescription.Text = value;
            }
        }

        #endregion

        #region Event Handlers
        private void btnAdd_Click(object sender, EventArgs e)
        {
            try
            {
                AddMemberDlg addMemberDlg = new AddMemberDlg();
                addMemberDlg.Label = listMemberType + ":";

                if (addMemberDlg.ShowDialog() == DialogResult.OK)
                {
                    string[] arr = new string[1];
                    arr[0] = addMemberDlg.Member;
                    AddMember(arr, null);
                }

                editDialog.bDataWasChanged = (membersAdded.Count + membersDeleted.Count + Description.Length > 0);
                editDialog.btnApply.Enabled = editDialog.bDataWasChanged;
                
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
                string member = lvMember.Text;

                if (members.ContainsKey(member))
                {
                    membersDeleted.Add(member, 0);
                }
                else if (membersAdded.ContainsKey(member))
                {
                    membersAdded.Remove(member);
                }
                else
                {
                    Logger.Log(
                        "EditSimpleListPage.btnRemove_Click: tried to remove member which does not exist",
                        Logger.manageLogLevel);
                    return;
                }
                lvMembers.SelectedItems.Clear();
                lvMembers.Items.Remove(lvMember);
                btnRemove.Enabled = false;

                editDialog.bDataWasChanged = (membersAdded.Count + membersDeleted.Count + Description.Length > 0);
                editDialog.btnApply.Enabled = editDialog.bDataWasChanged;

            }
            catch (Exception exp)
            {
                container.ShowError(exp.Message);
            }
        }

        private void lvMembers_SelectedIndexChanged(object sender, EventArgs e)
        {
            ListView lv = sender as ListView;
            if(lv != null)
            {
                if (lv.SelectedItems.Count == 1)
                {
                    btnRemove.Enabled = true;
                }
                else
                {
                    btnRemove.Enabled = false;
                } 
            }
        }

        #endregion
    }
}

