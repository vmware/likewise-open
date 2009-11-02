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
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Diagnostics;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Netlogon;
using Likewise.LMC.Eventlog;


namespace Likewise.LMC.Plugins.EventlogPlugin
{
    /// <summary>
    /// This class implements the main Event Viewer page
    /// </summary>
    public partial class EventViewerControl : StandardPage
    {
        #region enum Variables

        public enum EventViewerNodeType
        {
            PLUGIN,
            LOG
        }

        #endregion

        #region Constants

        // Displaying all the rows in a log might be prohibitive. This constant
        // controls the maximum number of rows that are retrieved and displayed
        protected const int cMaxRowsToFetch = 500;

        #endregion

        #region Class data

        // The current filter that controls what gets displayed (null if not filtering)
        protected EventFilter ef = null;
        protected EventCustomize ec = null;

        private ListViewColumnSorter lvwColumnSorter;

        public static string[] _PluginColumns = new string[] { "Name", "Type", "Size", "Description" };
        public static string[] _LogColumns = new string[] { "Type", "Date", "Time", "Source", "Category", "Event", "User", "Computer" };
        public static List<string> _displayedcolumn = new List<string>();
        public static List<string> _availableColumns = new List<string>();

        private int numColumns = 0;
        private string[] columnLabels = null;
        private ColumnHeader[] columnHeaders = null;

        public static EventViewerNodeType memberType;
        private LACTreeNode lactreeNode = null;
        private EventlogPlugin plugin;

        private string sqlQuery = "";
        private UInt32 nOffset = 0;
        private UInt32 nPageNumber = 0;
        private UInt32 nPageSize = 24;

        #endregion

        #region Properties

        public ListView EventViewerListView
        {
            get
            {
                return this.lvEvents;
            }
        }

        #endregion

        #region Constructor

        public EventViewerControl()
        {
            if (Configurations.useListScrolling)
            {
                nPageSize = 100;
            }
            else
            {
                nPageSize = 24;
            }

            InitializeComponent();
        }

        public EventViewerControl(EventViewerNodeType type)
            : this()
        {
            Logger.Log("EventViewerControl constructor: running Initialize()", Logger.eventLogLogLevel);

            memberType = type;

            // Create an instance of a ListView column sorter and assign it
            // to the ListView control.
            lvwColumnSorter = new ListViewColumnSorter();
            this.lvEvents.ListViewItemSorter = lvwColumnSorter;

            if (lvEvents.Columns.Count != 0)
            {
                lvEvents.Columns.Clear();
            }

            if (type == EventViewerNodeType.PLUGIN)
            {
                columnLabels = _PluginColumns;
            }
            else if (type == EventViewerNodeType.LOG)
            {
                columnLabels = _LogColumns;
            }
            else
            {
                throw new ArgumentException("EventViewerControl must be initialized as either EventViewerNodeType.LOG, EventViewerNodeType.PLUGIN was not set");
            }

            numColumns = columnLabels.Length;
            columnHeaders = new ColumnHeader[numColumns];
            for (int i = 0; i < numColumns; i++)
            {
                columnHeaders[i] = new ColumnHeader();
                columnHeaders[i].Name = columnLabels[i];
                columnHeaders[i].Text = columnLabels[i];
                columnHeaders[i].Width = 125;
            }

            this.lvEvents.Columns.AddRange(columnHeaders);

            //Filter listview columns
            if (memberType == EventViewerNodeType.LOG)
            {
                AddRemoveColumnsForm.Diplayedcolumns = AddRemoveColumnsForm.logDisplayedcolumns;
                this.panel1.Show();
            }
            else if (memberType == EventViewerNodeType.PLUGIN)
            {
                AddRemoveColumnsForm.Diplayedcolumns = AddRemoveColumnsForm.pluginDisplayedcolumns;

                this.panel1.Hide();
                this.lvEvents.ResumeLayout();
            }
            if (AddRemoveColumnsForm.Diplayedcolumns != null &&
                AddRemoveColumnsForm.Diplayedcolumns.Count != 0)
            {
                ShowHideColumns();
            }
        }

        #endregion

        #region IPlugInPage Members

        /// <summary>
        /// Called when the page is about to be displayed
        /// </summary>
        public override void SetPlugInInfo(IPlugInContainer ccontainer, IPlugIn ppi, LACTreeNode ttreeNode, LWTreeView llmctreeview, CServerControl sc)
        {
            treeNode = ttreeNode;            
            // let the base do its thing
            base.SetPlugInInfo(ccontainer, ppi, ttreeNode, llmctreeview, sc);
         
            plugin = ppi as EventlogPlugin;           
            ctx = (IContext)plugin.HostInfo;
           
            lactreeNode = ttreeNode;

            Hostinfo hn = ctx as Hostinfo;
            if (hn != null && !String.IsNullOrEmpty(hn.hostName) && hn.IsConnectionSuccess)
            {
                this.lblCaption.Text = String.Format("EventViewer for {0}", hn.hostName);
            }
            else
            {               
                this.lblCaption.Text = Properties.Resources.sTitleEventsPage;
                return;
            }

            if (memberType == EventViewerNodeType.LOG &&
                !String.IsNullOrEmpty(hn.hostName))
            {
                try
                {                    
                    FillComboWithLogCount();
                }
                catch (Exception e)
                {
                    Logger.LogException("EventViewerControl.SetPlugInInfo", e);
                    container.ShowError("Unable to open the event log; eventlog server may disabled");                    
                }
            }
            else if (memberType == EventViewerNodeType.PLUGIN &&
                !String.IsNullOrEmpty(hn.hostName) && hn.IsConnectionSuccess)
            {               
                InitializePluginNodeControl();
            }
        }

        public ContextMenu GetTreeContextMenu()
        {
            int cmIndex = 0;
            ContextMenu contextmenu = null;
            MenuItem m_item = null;

            contextmenu = new ContextMenu();

            m_item = new MenuItem("&Clear all Events", new EventHandler(cm_OnMenuClick));
            contextmenu.MenuItems.Add(cmIndex++, m_item);

            m_item = new MenuItem("&Filter...", new EventHandler(cm_OnMenuClick));
            contextmenu.MenuItems.Add(cmIndex++, m_item);

            m_item = new MenuItem("Prop&erties", new EventHandler(cm_OnMenuClick));
            contextmenu.MenuItems.Add(cmIndex++, m_item);

            m_item = new MenuItem("&Save Log file as...", new EventHandler(cm_OnMenuClick));
            contextmenu.MenuItems.Add(cmIndex++, m_item);

            m_item = new MenuItem("&View");
            MenuItem subm_item = new MenuItem("&Add/Remove Columns...", new EventHandler(cm_OnMenuClick));
            m_item.MenuItems.Add(subm_item);

            subm_item = new MenuItem("-");
            m_item.MenuItems.Add(subm_item);

            subm_item = new MenuItem("C&ustomize View...", new EventHandler(cm_OnMenuClick));
            m_item.MenuItems.Add(subm_item);

            contextmenu.MenuItems.Add(cmIndex++, m_item);

            m_item = new MenuItem("-");
            contextmenu.MenuItems.Add(contextmenu.MenuItems.Count, m_item);

            m_item = new MenuItem("Re&fresh", new EventHandler(cm_OnMenuClick));
            contextmenu.MenuItems.Add(contextmenu.MenuItems.Count, m_item);

            m_item = new MenuItem("&Help", new EventHandler(cm_OnMenuClick));
            contextmenu.MenuItems.Add(contextmenu.MenuItems.Count, m_item);

            return contextmenu;
        }

        public override void Refresh()
        {
            base.Refresh();
        }

        #endregion

        #region Event Handlers

        private void InitializePluginNodeControl()
        {
            lvEvents.Items.Clear();
            Hostinfo hn = ctx as Hostinfo;

            lblCaption.Text = string.Format("EventViewer for {0}", hn.hostName);

            if (plugin.logs == null)
                return;
            
            string[][] sArray = new string[plugin.logs.Length][];
            int idx = 0;

            foreach (string log in plugin.logs)
            {
                sArray[idx] = new string[]{
                                            log,
                                            "Log",
                                            "",
                                            string.Format("{0} Error Records",log)
                                          };
                idx++;
            }

            ListViewItem[] lvItems = new ListViewItem[sArray.Length];
            int indx = 0;

            foreach (string[] sLog in sArray)
            {
                string[] values = new string[lvEvents.Columns.Count];
                for (int index = 0; index < lvEvents.Columns.Count; index++)
                {
                    string header = lvEvents.Columns[index].Text.Trim();

                    if (String.Equals(header, "Name", StringComparison.InvariantCultureIgnoreCase))
                    {
                        values[index] = sLog[0];
                    }
                    if (String.Equals(header, "Description", StringComparison.InvariantCultureIgnoreCase))
                    {
                        values[index] = sLog[3];
                    }
                    if (String.Equals(header, "Type", StringComparison.InvariantCultureIgnoreCase))
                    {
                        values[index] = sLog[1];
                    }
                    if (String.Equals(header, "Size", StringComparison.InvariantCultureIgnoreCase))
                    {
                        values[index] = sLog[2];
                    }
                }
                lvItems[indx] = new ListViewItem(values);
                lvItems[indx].Tag = (int)Manage.ManageImageType.EventLog;
                lvItems[indx++].ImageIndex = (int)GetNodeType("EventLog");
            }

            lvEvents.Items.AddRange(lvItems);

            lvwColumnSorter.Order = SortOrder.Ascending;
            this.lvEvents.Sort();
        }

        public void LoadData(EventViewerNodeType memberType)
        {
            EventlogPlugin eventLogPlugin = (EventlogPlugin)pi;
            Hostinfo hn = ctx as Hostinfo;

            if (hn != null && hn.hostName != null && hn.IsConnectionSuccess)
            {
                lblCaption.Text = string.Format("EventViewer for {0}", hn.hostName);
            }
            else
            {
                lblCaption.Text = string.Format("EventViewer");
            }
            bool bLayoutSuspended = false;
            try
            {
                if (lvEvents.Items.Count != 0)
                {
                    lvEvents.Items.Clear();
                }

                if (memberType == EventViewerNodeType.LOG)
                {
                    Logger.Log("Getting eventlog records", Logger.eventLogLogLevel);

                    Logger.Log(String.Format("SQL Filter: {0}", sqlQuery), Logger.eventLogLogLevel);

                    EventLogRecord[] logs = eventLogPlugin.ReadEventLog(nOffset, nPageSize, sqlQuery);
                    lvEvents.SuspendLayout();
                    bLayoutSuspended = true;

                    if (logs != null)
                    {
                        ListViewItem[] lvItems = new ListViewItem[logs.Length];
                        int index = 0;
                        foreach (EventLogRecord log in logs)
                        {
                            if (log != null)
                            {
                                EventAPI.EventLogRecord record = log.Record;                                
                                DateTime eventTime = EventUtils.Time_T2DateTime(record.dwEventDateTime);
                                eventTime = eventTime.ToLocalTime();
                                string[] values = new string[lvEvents.Columns.Count];

                                for (int idx = 0; idx < lvEvents.Columns.Count; idx++)
                                {
                                    string header = lvEvents.Columns[idx].Text.Trim();
                                    if (String.Equals(header, "Type", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = record.pszEventType;
                                    }
                                    else if (String.Equals(header, "Date", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = eventTime.ToString("MM/dd/yyyy");
                                    }
                                    else if (String.Equals(header, "Time", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = eventTime.ToLongTimeString();
                                    }
                                    else if (String.Equals(header, "Source", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = record.pszEventSource;
                                    }
                                    else if (String.Equals(header, "Category", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = record.pszEventCategory;
                                    }
                                    else if (String.Equals(header, "Event", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = record.dwEventSourceId.ToString();
                                    }
                                    else if (String.Equals(header, "User", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = record.pszUser;
                                    }
                                    else if (String.Equals(header, "Computer", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        values[idx] = record.pszComputer;
                                    }
                                }
                                lvItems[index] = new ListViewItem(values);
                                lvItems[index].Tag = log;
                                lvItems[index++].ImageIndex = (int)GetNodeType(record.pszEventType);
                            }
                        }
                        lvEvents.Items.AddRange(lvItems);
                    }
                }
                else if (memberType == EventViewerNodeType.PLUGIN)
                {
                    InitializePluginNodeControl();
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("EventViewerControl.LoadData", ex);
            }
            finally
            {
                if (bLayoutSuspended)
                {
                    lvEvents.ResumeLayout();
                }
            }
        }

        private EventlogHandle OpenEventLogHandle(EventlogHandle eventlogHandle)
        {
            Logger.Log("Opening eventlog", Logger.eventLogLogLevel);
            EventlogPlugin plugin = (EventlogPlugin)pi;
            EventlogHandle result = null;
            if (eventlogHandle == null)
            {
                if (plugin.HostInfo != null &&
                    !String.IsNullOrEmpty(plugin.HostInfo.hostName))
                {
                    result = EventlogAdapter.OpenEventlog((plugin.HostInfo.hostName));
                }
            }

            if (result == null)
            {
                throw new Exception("Failed to get the eventlog handle");
            }

            Logger.Log(String.Format(
            "EventViewerControl.OpenEventLogHandle(found handle={0:X}",
            result.Handle.ToInt32()));

            return result;
        }


        private void FillComboWithLogCount()
        {
            EventlogPlugin eventLogPlugin = (EventlogPlugin)pi;

            sqlQuery = FilterSQLQuery(lactreeNode);

            Logger.Log(String.Format("SQL Filter: {0}", sqlQuery), Logger.eventLogLogLevel);           

            if (!eventLogPlugin.EventLogIsOpen())
            {
                return;
            }

            UInt32 logCount = eventLogPlugin.CountEventLog(sqlQuery);

            if (logCount == 0)
            {
                cbLog.Items.Clear();
                LoadData(memberType);
                return;
            }

            Logger.Log(String.Format("EventViewerControl.FillComboWithLogCount(): {0} records match the SQL Filter",
            logCount), Logger.eventLogLogLevel);

            UInt32 nNoOfPages = (logCount / nPageSize);
            UInt32 nReminder = (logCount % nPageSize);

            if (nReminder != 0)
            {
                nNoOfPages = nNoOfPages + 1;
            }

            if (cbLog.Items.Count != 0)
            {
                cbLog.Items.Clear();
            }

            for (int npage = 0; npage < nNoOfPages; npage++)
            {
                cbLog.Items.Add(npage);
            }

            if (cbLog.Items.Count != 0)
            {
                cbLog.SelectedIndex = 0;
            }
        }


        private enum ImageType
        {
            Warning = 0,   //Warning.ico
            Error = 1,      //Error.ico
            Information = 2,       //Information.ico
            EventLog = 3        //EventViewer_48.ico
        }

        /// <summary>
        /// Returns the Image type for the selected object
        /// </summary>
        /// <param name="objectClass"></param>
        /// <returns></returns>
        private ImageType GetNodeType(string type)
        {
            if (!string.IsNullOrEmpty(type))
            {
                if (String.Equals(type, "Warning", StringComparison.InvariantCultureIgnoreCase))
                {
                    return ImageType.Warning;
                }
                else if (String.Equals(type, "Error", StringComparison.InvariantCultureIgnoreCase))
                {
                    return ImageType.Error;
                }
                else if (String.Equals(type, "Information", StringComparison.InvariantCultureIgnoreCase))
                {
                    return ImageType.Information;
                }
                else if (String.Equals(type, "Success Audit", StringComparison.InvariantCultureIgnoreCase))
                {
                    return ImageType.Information;
                }
                else if (String.Equals(type, "Failure Audit", StringComparison.InvariantCultureIgnoreCase))
                {
                    return ImageType.Error;
                }
            }

            Logger.Log(
            String.Format(
            "EventViewerControl.GetNodeType could not resolve type={0}:",
            type),
            Logger.eventLogLogLevel);

            return ImageType.EventLog;
        }

        /// <summary>
        /// Event raises when we click on any contextmenu item
        /// And then performs the specified action
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void cm_OnMenuClick(object sender, EventArgs e)
        {
            // assure that the sender is a MenuItem
            MenuItem mi = sender as MenuItem;
            if (mi == null)
            {
                return;
            }

            switch (mi.Text.Trim())
            {
                case "&Save Log file as...":
                    {
                        if (exportLogsaveFileDialog.ShowDialog(this) == DialogResult.OK)
                        {
                            switch (exportLogsaveFileDialog.FilterIndex)
                            {
                                case 1:
                                    ExportLogFile(1, exportLogsaveFileDialog.FileName.ToString());
                                    break;
                                case 2:
                                    ExportLogFile(2, exportLogsaveFileDialog.FileName.ToString());
                                    break;
                                case 3:
                                    container.ShowError("Eventlog export to XML not implemented");
                                    break;
                            }
                        }
                        break;
                    }

                case "&Add/Remove Columns...":
                    {
                        _displayedcolumn.Clear();
                        _availableColumns.Clear();
                        foreach (ColumnHeader ch in lvEvents.Columns)
                        {
                            if (ch.Width != 0)
                            {
                                _displayedcolumn.Add(ch.Text);
                            }
                        }

                        string[] columnlist = null;
                        if (memberType == EventViewerNodeType.PLUGIN)
                        {
                            columnlist = _PluginColumns;
                        }
                        else
                        {
                            columnlist = _LogColumns;
                        }

                        foreach (string col in columnlist)
                        {
                            bool isMatchFound = false;
                            foreach (string column in _displayedcolumn)
                            {
                                if (column.Trim().Equals(col.Trim()))
                                {
                                    isMatchFound = true;
                                    break;
                                }
                            }
                            if (!isMatchFound)
                            {
                                _availableColumns.Add(col);
                            }
                        }

                        AddRemoveColumnsForm arcf = new AddRemoveColumnsForm(_availableColumns, _displayedcolumn, memberType);
                        if (arcf.ShowDialog(this) == DialogResult.OK)
                        {
                            if (memberType == EventViewerNodeType.LOG)
                            {
                                AddRemoveColumnsForm.Diplayedcolumns =
                                AddRemoveColumnsForm.logDisplayedcolumns;
                            }
                            else if (memberType == EventViewerNodeType.PLUGIN)
                            {
                                AddRemoveColumnsForm.Diplayedcolumns =
                                AddRemoveColumnsForm.pluginDisplayedcolumns;
                            }
                            if (AddRemoveColumnsForm.Diplayedcolumns != null &&
                            AddRemoveColumnsForm.Diplayedcolumns.Count != 0)
                            {
                                ShowHideColumns();
                            }
                        }
                        break;
                    }

                case "C&ustomize View...":
                    {
                        CustomizeViewForm cvf = new CustomizeViewForm(ec);
                        if (cvf.ShowDialog(this) == DialogResult.OK)
                        {
                            // if the user pressed ok, ask the form for the new EventCustomize instance
                            // (this could be null if the user has reverted to default values)
                            ec = cvf.Customizeview;
                        }
                        break;
                    }

                case "Re&fresh":
                    {
                        FillComboWithLogCount();
                        break;
                    }

                case "&Clear all Events":
                    {                                   
                        break;
                    }

                case "&Filter...":
                    {
                        this.OnFilter_Clicked(sender, null);
                        break;
                    }

                case "Prop&erties":
                    {
                        this.OnEventProperties_Clicked();
                        break;
                    }

                case "&Help":
                    {
                        ShowHelp();
                        break;
                    }

            }
        }

        private void ClearAllLogEvents()
        {
            EventlogPlugin plugin = pi as EventlogPlugin;

            string sqlFilter = string.Format("(EventTableCategoryId = '{0}')", treeNode.Text);

            UInt32 dwError = EventlogAdapter.DeleteFromEventLog(plugin.eventLogHandle, sqlFilter);

            if (dwError != 0)
            {
                Logger.Log(string.Format("Unable to delete the events from the current log {0}", treeNode.Text));
            }

            treeNode.sc.ShowControl(treeNode);
        }

        private void ShowHideColumns()
        {
            if (lvEvents.Columns.Count != 0)
            {
                this.lvEvents.Columns.Clear();
            }
            int index = 0;
            foreach (string s in AddRemoveColumnsForm.Diplayedcolumns)
            {
                if (lvEvents.Columns.ContainsKey(s))
                {
                    ColumnHeader col = this.lvEvents.Columns[s];
                    this.lvEvents.Columns.Remove(col);
                    this.lvEvents.Columns.Insert(index, col);
                }
                else
                {
                    ColumnHeader col = new ColumnHeader();
                    col.Text = s;
                    col.Name = s;
                    col.Width = 125;
                    this.lvEvents.Columns.Insert(index, col);
                }
                index++;
            }
            for (; index < lvEvents.Columns.Count; index++)
            {
                this.lvEvents.Columns[index].Width = 0;
            }
            LoadData(memberType);
        }

        /// <summary>
        /// For exporting log file
        /// </summary>
        /// <param name="fileType">1=textfile, 2=csv file, 3=xml file</param>
        /// <param name="filePath">the target file path</param>
        private void ExportLogFile(int fileType, string filePath)
        {

            FileStream fileStream = null;
            try
            {
                FileStream fStream = new FileStream(
                    filePath,
                    FileMode.Create,
                    FileAccess.ReadWrite,
                    FileShare.ReadWrite);
                StreamWriter writer = new StreamWriter(fStream);
                string logData = "";
                EventLogRecord eventLog = null;
                for (int i = 0; i < lvEvents.Items.Count; i++)
                {
                    //for reading each rows
                    #region Column Values
                    logData = "";
                    if (lvEvents.Items[i].SubItems[0].Text != null)
                    {
                        logData = lvEvents.Items[i].SubItems[0].Text.ToString();
                    }
                    for (int j = 1; j < lvEvents.Columns.Count; j++)
                    {
                        //for reading each columns
                        if (lvEvents.Items[i].SubItems[j].Text != null && lvEvents.Columns[j].Width != 0)
                        {
                            //txt
                            if (fileType == 1)
                            {
                                logData += "\t" + lvEvents.Items[i].SubItems[j].Text.ToString();
                            }
                            //CSV
                            else if (fileType == 2)
                            {
                                logData += "," + lvEvents.Items[i].SubItems[j].Text.ToString();
                            }
                        }
                    }
                    #endregion Column Values

                    #region Description
                    eventLog = lvEvents.Items[0].Tag as EventLogRecord;
                    if (eventLog != null)
                    {
                        //Txt
                        if (fileType == 1)
                        {
                            logData += "\t" + eventLog.Record.pszDescription.ToString();
                        }
                        //CSV
                        else if (fileType == 2)
                        {
                            logData += "," + eventLog.Record.pszDescription.ToString();
                        }
                    }
                    #endregion Description

                    writer.WriteLine(logData);
                }
                writer.Flush();
                writer.Close();
                fStream.Close();
                fStream.Dispose();

            }
            catch (Exception e)
            {
                Logger.LogException("EventViewerControl.ExportLogFile", e);
            }
            finally
            {
                if (fileStream != null)
                {
                    fileStream.Close();
                }
            }
        }

        /// <summary>
        /// Called when the user changes which log is to be displayed
        /// </summary>
        private void cbLog_SelectedIndexChanged(object sender, EventArgs e)
        {
            //swap our event log, clear the filter, refresh our data
            nPageNumber = (UInt32)Int32.Parse(cbLog.SelectedItem.ToString());
            nOffset = nPageNumber * nPageSize;
            LoadData(memberType);
        }

        /// <summary>
        /// Called when the user selects the "Event properties..." action in the action box
        /// </summary>
        private void OnEventProperties_Clicked()
        {
            LogPropertiesDlg lpDlg = new LogPropertiesDlg(
                 base.container,
                 this,
                 (EventlogPlugin)base.pi,
                 lvEvents,
                 null,
                 ef,
                 "LogProperities");
            lpDlg.SetData();
            lpDlg.ShowDialog(this);
            if (lpDlg.DialogResult == DialogResult.OK)
            {
                EventFilterControl filterform = (EventFilterControl)lpDlg.GetPage("FilterProperities");
                ef = filterform.Filter;
                lblFiltered.Visible = ef != null;
                lactreeNode.Tag = ef;
                FillComboWithLogCount();
            }
        }

        /// <summary>
        /// Called when the user selects the "Filter..." action in the action box
        /// </summary>
        private void OnFilter_Clicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            EventlogPlugin evtPlugin = (EventlogPlugin)base.pi;

            LogPropertiesDlg lpDlg = new LogPropertiesDlg(
                base.container,
                this,
                evtPlugin,
                lvEvents,
                null,
                ef,
                "FilterProperities");
            lpDlg.SetData();
            lpDlg.ShowDialog(this);
            if (lpDlg.DialogResult == DialogResult.OK)
            {
                EventFilterControl filterform = (EventFilterControl)lpDlg.GetPage("FilterProperities");
                ef = filterform.Filter;                
                lblFiltered.Visible = true;
                lactreeNode.Tag = ef;
                FillComboWithLogCount();
            }           
        }

        /// <summary>
        /// Called when the user selects the "Clear all Events..." action in the action box
        /// </summary>
        private void OnClear_Clicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            string sMsg = string.Format("Do you want to save \"{0}\" before clearing it", treeNode.Text);
            DialogResult dlg = MessageBox.Show(this,
                               sMsg,
                               CommonResources.GetString("Caption_Console"),
                               MessageBoxButtons.YesNoCancel,
                               MessageBoxIcon.Information);

            if (dlg == DialogResult.Yes)
            {
                if (exportLogsaveFileDialog.ShowDialog(this) == DialogResult.OK)
                {
                    switch (exportLogsaveFileDialog.FilterIndex)
                    {
                        case 1:
                            ExportLogFile(1, exportLogsaveFileDialog.FileName.ToString());
                            break;
                        case 2:
                            ExportLogFile(2, exportLogsaveFileDialog.FileName.ToString());
                            break;
                    }
                }
                ClearAllLogEvents();
            }
            else if (dlg == DialogResult.No)
            {
                ClearAllLogEvents();
            }             
        }

        /// <summary>
        /// Called when the selection state of the listview is changed. If nothing is
        /// selected, we disable the "Properties" action in the actionbox
        /// </summary>
        private void lvEvents_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lvEvents != null &&
                lvEvents.SelectedItems != null &&
                lvEvents.SelectedItems.Count > 0)
            {
                ListViewItem lvItem = lvEvents.SelectedItems[0];
                if (lvItem != null &&
                    lvItem.Tag != null)
                {
                    EventLog log = lvItem.Tag as EventLog;
                    if (log != null)
                    {
                    }
                }
            }
        }

        private void lvEvents_KeyDown(object sender, KeyEventArgs e)
        {
            if (e != null &&
                e.KeyCode == Keys.Enter)
            {
                OnEventProperties_Clicked();
                e.Handled = true;
            }
        }

        /// <summary>
        /// Called when the user clicks (actually, when s/he releases the mouse button) on
        /// on the DataGridView. This is where we show a context menu.
        /// </summary>
        private void lvEvents_MouseUp(object sender, MouseEventArgs e)
        {
            ListView lvSender = sender as ListView;
            if (lvSender != null &&
                e != null &&
                e.Button == MouseButtons.Right &&
                lvEvents != null &&
                lvEvents.SelectedItems != null &&
                lvEvents.SelectedItems.Count == 1)
            {
                ListViewHitTestInfo hti = lvEvents.HitTest(e.X, e.Y);
                if (hti != null &&
                    hti.Item != null)
                {
                    ContextMenu eventpropertyContextMenu = null;
                    ListViewItem lvItem = hti.Item;

                    if (!hti.Item.Selected)
                    {
                        hti.Item.Selected = true;
                    }

                    if (lvItem.Tag != null)
                    {
                        //EventLog log = lvItem.Tag as EventLog;
                        //if (log != null)                       
                        EventLogRecord eventLogRecord = lvItem.Tag as EventLogRecord;
                        if (eventLogRecord != null) {
                            eventpropertyContextMenu = GetPropertyContextMenu();
                        }
                        else {
                            string slog = lvItem.Tag as string;
                            if (!String.IsNullOrEmpty(slog) &&
                                slog.Equals("EventLog"))
                            {
                                eventpropertyContextMenu = GetTreeContextMenu();
                            }
                        }
                    }

                    if (eventpropertyContextMenu != null) {
                        eventpropertyContextMenu.Show(lvSender, new Point(e.X, e.Y));
                    }
                }
            }
        }
        public ContextMenu GetPropertyContextMenu()
        {
            ContextMenu contextmenu = new ContextMenu();

            MenuItem m_item = new MenuItem("Prop&erties", new EventHandler(cm_OnPropertyClick));
            contextmenu.MenuItems.Add(m_item);

            m_item = new MenuItem("&Help", new EventHandler(cm_OnMenuClick));
            contextmenu.MenuItems.Add(m_item);

            return contextmenu;

        }

        private void cm_OnPropertyClick(object sender, EventArgs e)
        {
            // assure that the sender is a MenuItem
            MenuItem mi = sender as MenuItem;

            if (mi != null &&
                !String.IsNullOrEmpty(mi.Text) &&
                String.Equals(mi.Text.Trim(), "Prop&erties", StringComparison.InvariantCultureIgnoreCase))
            {
                ShowEventProperties();
            }
        }
        /// <summary>
        /// Called when the user double clicks on an item in the datagridview. Displays the
        /// event properties for the selected item.
        /// </summary>
        private void lvEvents_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            ListViewHitTestInfo hti = lvEvents.HitTest(e.X, e.Y);
            if (hti != null &&
                hti.Item != null)
            {
                if (!hti.Item.Selected)
                {
                    hti.Item.Selected = true;
                }

                if (memberType == EventViewerNodeType.LOG)
                {
                    ShowEventProperties();
                   
                }
                else if (memberType == EventViewerNodeType.PLUGIN)
                {
                    if (hti.Item.Tag != null)
                    {
                        LACTreeNode node = hti.Item.Tag as LACTreeNode;
                        if (node != null)
                        {
                            lmctreeview.SelectedNode = node;
                            lmctreeview.Select();
                            node.sc.ShowControl(node);
                        }
                    }
                }
            }
        }

        private void lvEvents_ItemMouseHover(object sender, ListViewItemMouseHoverEventArgs e)
        {
            if (e.Item != null)
            {
                EventLogRecord el = e.Item.Tag as EventLogRecord;
                if (el != null)
                {
                    string s = el.Record.pszDescription;
                    if (!string.IsNullOrEmpty(s))
                    {
                        e.Item.ToolTipText = Wrap(s, 70);
                    }
                }
            }
        }

        private void lvEvents_ColumnClick(object sender, ColumnClickEventArgs e)
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
            this.lvEvents.Sort();
        }

        #endregion

        #region Helper functions

        // <summary>
        // This method queries the indicated server and sets up various data in the Hostinfo structure.
        // It also establishes a set of working credentials for the machine
        // </summary>
        // <returns>FALSE if unable to manage the machine</returns>
        private void GetMachineInfo()
        {
            Hostinfo hn = ctx as Hostinfo;
            Logger.Log(String.Format(
            "GetMachineInfo called for EventViewerControl.  hn: {0}",
            !Hostinfo.HasCreds(hn) ? "empty" : hn.hostName),
            Logger.manageLogLevel);

            Hostinfo defaultHostInfo = null;

            //if Hostinfo is empty, attempt to retrieve details using kerberos
            if (!Hostinfo.HasCreds(hn))
            {
                defaultHostInfo = new Hostinfo();
            }
            else
            {
                defaultHostInfo = hn.Clone();
            }

            uint requestedFields = (uint)Hostinfo.FieldBitmaskBits.FQ_HOSTNAME;

            if (!this.container.GetTargetMachineInfo(this.pi, defaultHostInfo, requestedFields))
            {
                Logger.Log(
                "Could find information about target machine",
                Logger.netAPILogLevel);
                //hn = null;
            }

            lblCaption.Text = string.Format("EventViewer for {0}", hn.hostName);
        }

        /// <summary>
        /// Shows Event Viewer Help
        /// </summary>
        private void ShowHelp()
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = CommonResources.GetString("LAC_Help");
            psi.Verb = "open";
            psi.WindowStyle = ProcessWindowStyle.Normal;
            Process.Start(psi);
            return;
        }
        /// <summary>
        /// Retrieves event data and updates the DataTable we use to cache them.
        /// </summary>
        public override void RefreshData()
        {
            Hostinfo hn = ctx as Hostinfo;
            try
            {
                LoadData(memberType);
            }
            catch (Exception ex)
            {
                container.ShowError(
                "Unable to retrieve events from " +
                hn.hostName +
                ": " + ex.Message);
                Logger.LogException("EventViewerControl.RefreshData", ex);
            }
        }

        /// <summary>
        /// Forming SQL Query based on Filter parameter
        /// </summary>
        private string FilterSQLQuery(LACTreeNode lacTreeNode)
        {
            if (lacTreeNode == null)
            {
                return "EventTableCategoryId = 'Application'";
            }

            ef = (EventFilter)lactreeNode.Tag;
            string sqlfilter = string.Empty;

            string categoryId = string.Empty;
            switch (lactreeNode.Text)
            {
                case "Application":
                    categoryId = string.Format("EventTableCategoryId = 'Application'"); //(UInt32)EventAPI.TableCategoryType.Application);
                    break;
                case "WebBrowser":
                    categoryId = string.Format("EventTableCategoryId = 'WebBrowser'");// (UInt32)EventAPI.TableCategoryType.WebBrowser);
                    break;
                case "Security":
                    categoryId = string.Format("EventTableCategoryId = 'Security'");// (UInt32)EventAPI.TableCategoryType.Security);
                    break;
                case "System":
                    categoryId = string.Format("EventTableCategoryId = 'System'");// (UInt32)EventAPI.TableCategoryType.Security);
                    break;
                default:
                    categoryId = string.Format("EventTableCategoryId = 'Application'");// (UInt32)EventAPI.TableCategoryType.System);
                    break;
            }

            if (!String.IsNullOrEmpty(categoryId))
            {
                sqlfilter = categoryId;
            }

            if (ef != null)
            {
                string eventType = "(";
                string opStr = "";
                if (ef.ShowInformation)
                {
                    eventType += " (EventType LIKE 'Information')";
                    opStr = " OR ";
                }
                if (ef.ShowWarning)
                {
                    eventType += opStr + "(EventType LIKE 'Warning')";
                    opStr = " OR ";
                }
                if (ef.ShowError)
                {
                    eventType += opStr + "(EventType LIKE 'Error')";
                    opStr = " OR ";
                }
                if (ef.ShowSuccessAudit)
                {
                    eventType += opStr + "(EventType LIKE 'Success Audit')";
                    opStr = " OR ";
                }
                if (ef.ShowFailureAudit)
                {
                    eventType += opStr + "(EventType LIKE 'Failure Audit')";
                }
                eventType += ")";


                sqlfilter = sqlfilter + " AND " + eventType;

                if (!String.IsNullOrEmpty(ef.EventSource))
                {
                    string eventsource = string.Format("EventSource LIKE '{0}'", ef.EventSource);
                    if (eventsource != "")
                    {
                        sqlfilter = sqlfilter + " AND " + eventsource;
                    }
                }
                if (!String.IsNullOrEmpty(ef.Category))
                {
                    string category = string.Format("EventCategory LIKE '{0}'", ef.Category);
                    if (category != "")
                    {
                        sqlfilter = sqlfilter + " AND " + category;
                    }
                }
                if (ef.EventId != -1)
                {
                    string eventid = string.Format("EventSourceId LIKE {0}", (UInt32)ef.EventId);
                    if (eventid != "")
                    {
                        sqlfilter = sqlfilter + " AND " + eventid;
                    }
                }
                if (!String.IsNullOrEmpty(ef.User))
                {
                    string user = string.Format("User LIKE '{0}'", ef.User);
                    if (user != "")
                    {
                        sqlfilter = sqlfilter + " AND " + user;
                    }
                }
                if (!String.IsNullOrEmpty(ef.Computer))
                {
                    string computer = string.Format("Computer LIKE '{0}'", ef.Computer);
                    if (computer != "")
                    {
                        sqlfilter = sqlfilter + " AND " + computer;
                    }
                }
                if (ef.StartDate != DateTime.MinValue &&
                    ef.EndDate != DateTime.MaxValue)
                {
                    string datetime = string.Format(
                        "EventDateTime between {0} and {1}",
                        EventAPI.ConvertToUnixTimestamp(ef.StartDate),
                        EventAPI.ConvertToUnixTimestamp(ef.EndDate));
                    if (datetime != "")
                    {
                        sqlfilter = sqlfilter + " AND " + datetime;
                    }
                }
                if (ef.CustomFilterString != null &&
                    ef.CustomFilterString != "")
                {
                    sqlfilter = sqlfilter + " AND " + ef.CustomFilterString;
                }
            }

            return sqlfilter;
        }


        /// <summary>
        /// Shows the event properties form
        /// </summary>
        private void ShowEventProperties()
        {
            if (lvEvents != null &&
                lvEvents.SelectedItems != null &&
                lvEvents.SelectedItems.Count > 0)
            {
                EventlogPlugin evtPlugin = (EventlogPlugin)base.pi;

                EventPropertiesDlg epf = new EventPropertiesDlg(
                    base.container,
                    this,
                    evtPlugin,
                    lvEvents);

                epf.SetData();
                epf.ShowDialog(this);
            }
        }

        // inserts line breaks every cchWidth or so characters
        private static string Wrap(string s, int cchWidth)
        {
            if (s.Length < cchWidth)
            {
                return s;
            }

            string sOut = s.Substring(0, cchWidth);
            int ich = cchWidth;
            int cchLastLine = cchWidth;
            while (ich < s.Length)
            {
                if (cchLastLine >= cchWidth && char.IsWhiteSpace(s[ich]))
                {
                    sOut += s[ich] + "\n";
                    cchLastLine = 0;
                }
                else
                {
                    sOut += s[ich];
                    cchLastLine++;
                }
                ich++;
            }
            return sOut;
        }

        #endregion

    }
}

