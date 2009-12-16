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
/// <summary>
/// This class encapsulates the form that is used to Add/Remove Columns from the right panel Listview
/// as a whole
/// </summary>
public partial class AddRemoveColumnsForm : Form
{
    #region Class Data

    private static List<string> displayedcolumns = new List<string>();
    public static List<string> pluginDisplayedcolumns = new List<string>();
    public static List<string> logDisplayedcolumns = new List<string>();
    public static EventViewerControl.EventViewerNodeType nodeType;

    #endregion

    #region Accessors

    public static List<string> Diplayedcolumns
    {
        get
        {
            return displayedcolumns;
        }
        set
        {
            displayedcolumns = value;
        }
    }

    #endregion

    #region Constructors

    public AddRemoveColumnsForm()
    {
        InitializeComponent();
    }

    public AddRemoveColumnsForm(List<string> availableCol, List<string> displayedcol, EventViewerControl.EventViewerNodeType type)
        : this()
    {
        if (availableCol != null && availableCol.Count != 0)
        {
            lbAvailableColumns.Items.AddRange(availableCol.ToArray());
        }

        if (displayedcol != null && displayedcol.Count != 0)
        {
            lbDisplayedColumns.Items.AddRange(displayedcol.ToArray());
        }

        if (lbAvailableColumns.Items.Count != 0)
        {
            lbAvailableColumns.SelectedIndex = lbAvailableColumns.Items.Count - 1;
        }

        if (lbDisplayedColumns.Items.Count != 0)
        {
            lbDisplayedColumns.SelectedIndex = lbDisplayedColumns.Items.Count - 1;
        }

        nodeType = type;

        if (lbDisplayedColumns.Items.Count == 1)
        {
            btnRemove.Enabled = false;
        }

        btnAdd.Enabled = btnRestoredefaults.Enabled = lbAvailableColumns.Items.Count != 0;
        btnMovedown.Enabled = (lbDisplayedColumns.Items.Count - 1) != (lbDisplayedColumns.SelectedIndex);
    }

    #endregion

    #region Event Handlers

    private void btnOk_Click(object sender, EventArgs e)
    {
        if (nodeType == EventViewerControl.EventViewerNodeType.PLUGIN)
        {
            pluginDisplayedcolumns.Clear();
            foreach (object item in lbDisplayedColumns.Items)
            {
                pluginDisplayedcolumns.Add(item.ToString());
            }
        }
        else if (nodeType == EventViewerControl.EventViewerNodeType.LOG)
        {
            logDisplayedcolumns.Clear();
            foreach (object item in lbDisplayedColumns.Items)
            {
                logDisplayedcolumns.Add(item.ToString());
            }
        }

        this.DialogResult = DialogResult.OK;
    }

    private void btnCancel_Click(object sender, EventArgs e)
    {
        this.Close();
    }

    private void btnMoveup_Click(object sender, EventArgs e)
    {
        int idx = lbDisplayedColumns.SelectedIndex;
        if (idx >= 0)
        {
            int newIdx = idx - 1;
            if (newIdx >= 0)
            {
                Object lvSelectedItem = lbDisplayedColumns.Items[idx];
                Object[] lvItemArr = new Object[lbDisplayedColumns.Items.Count - 1];
                List<Object> lvItemList = new List<Object>();
                int i = 0;

                foreach (Object lvitem in lbDisplayedColumns.Items)
                {
                    if (!lvitem.Equals(lvSelectedItem))
                    {
                        lvItemList.Add(lvitem);
                    }
                }
                foreach (Object lvitem in lvItemList)
                {
                    lvItemArr[i] = lvitem;
                    i++;
                }

                lbDisplayedColumns.Items.Clear();
                lbDisplayedColumns.Items.AddRange(lvItemArr);

                lbDisplayedColumns.Items.Insert(newIdx, lvSelectedItem);
                lbDisplayedColumns.SelectedIndex = newIdx;

                btnRemove.Enabled = lbDisplayedColumns.Items.Count > 1 && !(lbDisplayedColumns.Items[newIdx].ToString().Equals("Name"));
            }
        }
        btnRestoredefaults.Enabled = true;
        AdjustButtons();
    }

    private void btnMovedown_Click(object sender, EventArgs e)
    {
        int idx = lbDisplayedColumns.SelectedIndex;
        if (idx >= 0)
        {
            int nItems = lbDisplayedColumns.Items.Count;

            int newIdx = idx + 1;
            if (newIdx <= nItems - 1)
            {
                Object lvSelectedItem = lbDisplayedColumns.Items[idx];

                Object[] lvItemArr = new Object[lbDisplayedColumns.Items.Count -1];
                List<Object> lvItemList = new List<Object>();
                int i = 0;

                foreach (Object lvitem in lbDisplayedColumns.Items)
                {
                    if (!lvitem.Equals(lvSelectedItem))
                    {
                        lvItemList.Add(lvitem);
                    }
                }
                foreach (Object lvitem in lvItemList)
                {
                    lvItemArr[i] = lvitem;
                    i++;
                }

                lbDisplayedColumns.Items.Clear();
                lbDisplayedColumns.Items.AddRange(lvItemArr);

                lbDisplayedColumns.Items.Insert(newIdx, lvSelectedItem);
                lbDisplayedColumns.SelectedIndex = newIdx;
            }
        }
        btnRestoredefaults.Enabled = true;

        AdjustButtons();
    }

    private void btnRestoredefaults_Click(object sender, EventArgs e)
    {
        if (nodeType.Equals(EventViewerControl.EventViewerNodeType.LOG))
        {
            lbDisplayedColumns.Items.Clear();
            lbDisplayedColumns.Items.AddRange(EventViewerControl._LogColumns);
            lbAvailableColumns.Items.Clear();
            lbDisplayedColumns.SelectedIndex = lbDisplayedColumns.Items.Count - 1;
        }
        else
        {
            lbDisplayedColumns.Items.Clear();
            lbDisplayedColumns.Items.AddRange(EventViewerControl._PluginColumns);
            lbAvailableColumns.Items.Clear();
            lbDisplayedColumns.SelectedIndex = lbDisplayedColumns.Items.Count - 1;
        }
        btnMovedown.Enabled = (lbDisplayedColumns.Items.Count - 1) != (lbDisplayedColumns.SelectedIndex);
        btnAdd.Enabled = btnRestoredefaults.Enabled = false;
    }

    private void btnAdd_Click(object sender, EventArgs e)
    {
        handle_add(sender, e);
    }

    private void btnRemove_Click(object sender, EventArgs e)
    {
        if (!btnRemove.Enabled)
            return;

        handle_remove(sender, e);
    }

    private void lbAvailableColumns_DoubleClick(object sender, EventArgs e)
    {
        handle_add(sender, e);
    }

    private void lbDisplayedColumns_DoubleClick(object sender, EventArgs e)
    {
        handle_remove(sender, e);
    }

    #endregion

    #region Helper functions

    private void AdjustButtons()
    {
        btnMoveup.Enabled = lbDisplayedColumns.Items.Count > 1;
        btnMovedown.Enabled = lbDisplayedColumns.Items.Count > 1;
        int idx = lbDisplayedColumns.SelectedIndex;
        // There must be only one since we have single-select on
        if (idx >= 0)
        {
            int nItems = lbDisplayedColumns.Items.Count;
            if (nItems > 1 && idx > 0)
            {
                btnMoveup.Enabled = true;
            }
            else
            {
                btnMoveup.Enabled = false;
            }

            if (nItems > 1 && idx != nItems - 1)
            {
                btnMovedown.Enabled = true;
            }
            else
            {
                btnMovedown.Enabled = false;
            }
        }
        else
        {
            btnMoveup.Enabled = false;
            btnMovedown.Enabled = false;
        }
    }

    private void lbDisplayedColumns_SelectedIndexChanged(object sender, EventArgs e)
    {
        int idx = lbDisplayedColumns.SelectedIndex;

        if (idx >= 0)
        {
            if (nodeType.Equals(EventViewerControl.EventViewerNodeType.LOG))
            {
                if (lbDisplayedColumns.Items[idx].ToString().Trim().Equals("Type"))
                {
                    btnRemove.Enabled = false;
                    btnAdd.Focus();
                }
                else
                    btnRemove.Enabled = true;
            }
            if (nodeType.Equals(EventViewerControl.EventViewerNodeType.PLUGIN))
            {
                if (lbDisplayedColumns.Items[idx].ToString().Trim().Equals("Name"))
                {
                    btnRemove.Enabled = false;
                    btnAdd.Focus();
                }
                else
                    btnRemove.Enabled = true;
            }
        }
        AdjustButtons();
    }


    private void handle_add(object sender, EventArgs e)
    {
        int idx = lbAvailableColumns.SelectedIndex;
        if (idx >= 0)
        {
            Object lvSelectedItem = lbAvailableColumns.Items[idx];
            lbDisplayedColumns.Items.Add(lvSelectedItem);

            Object[] lvItemArr = new Object[lbAvailableColumns.Items.Count - 1];
            List<Object> lvItemList = new List<Object>();
            int i = 0;

            foreach (Object lvitem in lbAvailableColumns.Items)
            {
                if (!lvitem.Equals(lvSelectedItem))
                {
                    lvItemList.Add(lvitem);
                }
            }
            foreach (Object lvitem in lvItemList)
            {
                lvItemArr[i] = lvitem;
                i++;
            }

            lbAvailableColumns.Items.Clear();
            lbAvailableColumns.Items.AddRange(lvItemArr);
            lbAvailableColumns.SelectedIndex = idx - 1;
            lbDisplayedColumns.SelectedIndex = lbDisplayedColumns.Items.Count - 1;
            btnRemove.Enabled = true;
        }
        else if (idx < 0 && lbAvailableColumns.Items.Count != 0)
        {
            if (lbAvailableColumns.Items.Count <= 1)
            {
                lbAvailableColumns.SelectedIndex = 0;
            }
            //else
            //{
            //    lbAvailableColumns.SelectedIndex = idx + 1;
            //}
        }
        btnAdd.Enabled = btnRestoredefaults.Enabled = lbAvailableColumns.Items.Count != 0;
        btnMovedown.Enabled = (lbDisplayedColumns.Items.Count - 1) != (lbDisplayedColumns.SelectedIndex);
    }

    private void handle_remove(object sender, EventArgs e)
    {
        int idx = lbDisplayedColumns.SelectedIndex;
        if (idx >= 0)
        {
            Object lvSelectedItem = lbDisplayedColumns.Items[idx];
            lbAvailableColumns.Items.Add(lvSelectedItem);

            Object[] lvItemArr = new Object[lbDisplayedColumns.Items.Count - 1];
            List<Object> lvItemList = new List<Object>();
            int i = 0;

            foreach (Object lvitem in lbDisplayedColumns.Items)
            {
                if (!lvitem.Equals(lvSelectedItem))
                {
                    lvItemList.Add(lvitem);
                }
            }
            foreach (Object lvitem in lvItemList)
            {
                lvItemArr[i] = lvitem;
                i++;
            }

            lbDisplayedColumns.Items.Clear();
            lbDisplayedColumns.Items.AddRange(lvItemArr);

            if (idx != 0)
                lbDisplayedColumns.SelectedIndex = idx - 1;
            else
                lbDisplayedColumns.SelectedIndex = 0;

            lbAvailableColumns.SelectedIndex = lbAvailableColumns.Items.Count - 1;
        }
        else if (idx < 0 && lbAvailableColumns.Items.Count != 0)
        {
            if (lbDisplayedColumns.Items.Count <= 1)
            {
                lbDisplayedColumns.SelectedIndex = 0;
            }
            //else
            //{
            //    lbDisplayedColumns.SelectedIndex = idx + 1;
            //}
        }
        btnAdd.Enabled = btnRestoredefaults.Enabled = lbAvailableColumns.Items.Count != 0;
        btnMovedown.Enabled = (lbDisplayedColumns.Items.Count - 1) != (lbDisplayedColumns.SelectedIndex);
    }

    #endregion

}
}
