using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.ServiceProcess;
using System.Management;

using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.Services;

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public partial class DependenciesPage : MPPage, IDirectoryPropertiesPage
    {
        #region

        private string serviceName;

        #endregion

        #region Constructors

        public DependenciesPage()
        {
            InitializeComponent();
        }

        public DependenciesPage(IPlugInContainer container,
                               string serviceName)
        {
            InitializeComponent();

            this.pageID = "DependenciesPage";
            SetPageTitle("DependenciesPage");

            this.serviceName = serviceName;
        }

        #endregion

        #region overriden functions

        public void SetData()
        {
            this.ParentContainer.btnApply.Enabled = false;
            this.ParentContainer.DataChanged = false;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows) {
                GetWinServiceDependencies();
            }
            else {
                GetUnixServiceDependencies();
            }
        }

        #endregion

        #region Helper functions

        private void GetWinServiceDependencies()
        {
            ServiceController sc = new ServiceController(serviceName, System.Environment.MachineName);
            if (sc != null)
            {
                lblDisplayName.Text = sc.DisplayName;

                TreeNode[] dependentServices = ServiceManagerWindowsWrapper.WMIGetDependentServices(serviceName);
                if (dependentServices != null && dependentServices.Length > 0)
                {
                    treeViewDependents.Nodes.AddRange(dependentServices);
                    treeViewDependents.Enabled = true;
                }
                else
                    treeViewDependents.Nodes.Add(new TreeNode("<No Dependencies>"));

                TreeNode[] DependencyServices = ServiceManagerWindowsWrapper.WMIGetDependencyServices(serviceName);
                if (DependencyServices != null && DependencyServices.Length > 0)
                {
                    treeViewDependencies.Nodes.AddRange(DependencyServices);
                    treeViewDependencies.Enabled = true;
                }
                else
                    treeViewDependencies.Nodes.Add(new TreeNode("<No Dependencies>"));
            }
        }

        private void GetUnixServiceDependencies()
        {
            IntPtr pServiceHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(serviceName);

            if (pServiceHandle == IntPtr.Zero)
                return;

            //Counting of Dependencies
            ServiceManagerApi.LW_SERVICE_INFO serviceInfo = ServiceManagerInteropWrapper.ApiLwSmQueryServiceInfo(pServiceHandle);
            if (serviceInfo.ppwszDependencies != null && serviceInfo.ppwszDependencies.Length != 0)
            {
                TreeNode[] servicelist = new TreeNode[serviceInfo.ppwszDependencies.Length];
                for (int idx = 0; idx < serviceInfo.ppwszDependencies.Length; idx++)
                {
                    TreeNode tn = new TreeNode(serviceInfo.ppwszDependencies[idx]);
                    tn.Tag = serviceInfo.pwszName;
                    servicelist[idx] = tn;
                }

                if (servicelist != null && servicelist.Length != 0)
                    treeViewDependencies.Nodes.AddRange(servicelist);
                else
                    treeViewDependencies.Nodes.Add(new TreeNode("<No Dependencies>"));
            }
            ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pServiceHandle);

            //Counting of Dependents
            List<TreeNode> dependentservices = new List<TreeNode>();
            string[] sServiceEnum = ServiceManagerInteropWrapper.ApiLwSmEnumerateServices();
            if (sServiceEnum != null)
            {
                foreach (string name in sServiceEnum)
                {
                    IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(name);
                    if (pHandle != IntPtr.Zero)
                    {
                        serviceInfo = ServiceManagerInteropWrapper.ApiLwSmQueryServiceInfo(pHandle);

                        if (serviceInfo.ppwszDependencies != null && serviceInfo.ppwszDependencies.Length != 0)
                        {
                            foreach (string service in serviceInfo.ppwszDependencies)
                            {
                                if (serviceName.Equals(service, StringComparison.InvariantCultureIgnoreCase))
                                {
                                    TreeNode tn = new TreeNode(name);
                                    tn.Tag = name;
                                    dependentservices.Add(tn);
									break;
                                }
                            }
                        }
                        ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);
                    }
                }

                if (dependentservices.Count != 0)
                {
                    TreeNode[] services = new TreeNode[dependentservices.Count];
                    dependentservices.CopyTo(services);
                    treeViewDependents.Nodes.AddRange(services);
                }
                else
                    treeViewDependents.Nodes.Add(new TreeNode("<No Dependencies>"));
            }
        }

        #endregion

        #region Helper functions

        public bool OnApply()
        {
            //return true always since the depends page does event works
            return true;
        }

        #endregion

        #region Event Handlers

        private void treeViewDependents_AfterExpand(object sender, TreeViewEventArgs e)
        {
            TreeNode node = e.Node;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                if (node != null && node.Nodes.Count == 0)
                {
                    TreeNode[] dependentServices = ServiceManagerWindowsWrapper.WMIGetDependentServices(node.Tag as string);
                    if (dependentServices != null && dependentServices.Length > 0)
                    {
                        node.Nodes.AddRange(dependentServices);
                    }
                }
            }
            else
            {
                string[] sServicedepencies = ServiceManagerInteropWrapper.ApiLwSmEnumerateServices();
                if (sServicedepencies != null)
                {
                    foreach (string name in sServicedepencies)
                    {
                        IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(name);
                        if (pHandle != IntPtr.Zero)
                        {
                            ServiceManagerApi.LW_SERVICE_INFO serviceInfo = ServiceManagerInteropWrapper.ApiLwSmQueryServiceInfo(pHandle);
                            if (serviceInfo.ppwszDependencies != null && serviceInfo.ppwszDependencies.Length != 0)
                            {
                                foreach (string service in serviceInfo.ppwszDependencies)
                                {
                                    if ((node.Tag as string).Equals(service, StringComparison.InvariantCultureIgnoreCase))
                                        node.Nodes.Add(service);
                                }
                            }
                            ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);
                        }
                    }
                }
            }
        }

        private void treeViewDependencies_AfterSelect(object sender, TreeViewEventArgs e)
        {
            TreeNode node = e.Node;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                if (node != null && node.Nodes.Count == 0)
                {
                    TreeNode[] dependencyServices = ServiceManagerWindowsWrapper.WMIGetDependencyServices(node.Tag as string);
                    if (dependencyServices != null && dependencyServices.Length > 0)
                    {
                        node.Nodes.AddRange(dependencyServices);
                    }
                }
            }
            else
            {
                IntPtr pServiceHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(node.Tag as string);
                ServiceManagerApi.LW_SERVICE_INFO serviceInfo = ServiceManagerInteropWrapper.ApiLwSmQueryServiceInfo(pServiceHandle);
                if (serviceInfo.ppwszDependencies != null && serviceInfo.ppwszDependencies.Length != 0)
                {
                    TreeNode[] servicelist = new TreeNode[serviceInfo.ppwszDependencies.Length];
                    for (int idx = 0; idx < serviceInfo.ppwszDependencies.Length; idx++)
                    {
                        servicelist[idx] = new TreeNode(serviceInfo.pwszName);
                    }

                    if (servicelist != null && servicelist.Length != 0)
                        node.Nodes.AddRange(servicelist);
                }
                ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pServiceHandle);
            }
        }

        #endregion
    }
}
