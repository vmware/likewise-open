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
using System.Drawing;
using System.Windows.Forms;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.ServerControl
{
    public class LACTreeNode:  TreeNode
    {
        #region class data
        protected Bitmap _image;
        protected Type _t;
        protected IPlugIn _plugin;
        protected IPlugInPage _pluginPage = null;
       // protected Hostinfo _hn = null;
        protected IContext _ctx = null;
        protected CServerControl _sc = null;
        protected bool _IsModified = false;
        public bool _IsPlugIn = false;
        public bool _IsSelected = false;
        #endregion

        #region accessors
        public Bitmap image
        {
            get
            {
                return _image;
            }
        }
        public Type t
        {
            get
            {
                return _t;
            }
        }
        public CServerControl sc
        {
            get
            {
                return _sc;
            }
            set
            {
                _sc = value;
            }
        }
        public bool IsModified
        {
            get
            {
                return _IsModified;
            }
            set
            {
                _IsModified = value;
            }
        }
        public bool IsPluginNode
        {
            get
            {
                return _IsPlugIn;
            }
            set
            {
                _IsPlugIn = value;
            }
        }

        public new bool IsSelected
        {
            get
            {
                return _IsSelected;
            }
            set
            {
                _IsSelected = value;
            }
        }

        #endregion

        #region Constructors
        public LACTreeNode()
        {
        }

        public LACTreeNode(string Name, Bitmap Image, Type t, IPlugIn plugin)
        {
            Text = Name;
            this.Name = Name;
            _image = Image;
            _t = t;
            _plugin = plugin;
        }

        public LACTreeNode(string Name, Icon icon, Type t, IPlugIn plugin)
        : this(Name,
               (icon != null ? icon.ToBitmap() : null),
               t,
               plugin)
        {
        }
        #endregion

        #region helper functions
        public LACTreeNode DeepCopy()
        {
            LACTreeNode clone = new LACTreeNode(this.Name, this._image, this._t,this._plugin);

            clone.PluginPage = this.PluginPage;
            clone.IsPluginNode = this.IsPluginNode;

            return clone;

        }

        /// <summary>
        /// Recursively calls SetContext on the plugin page associated with this LACTreeNode,
        /// and does the same to all descendents of this LACTreeNode
        /// </summary>
        /// <param name="hn">Hostinfo to assign to this LACTreeNode and its descendents.  May be null.</param>
        public void SetContext(IContext ctx)
        {
            _ctx = ctx;

            if (_pluginPage != null)
            {
                _pluginPage.SetContext(ctx);
            }

            if (Nodes != null)
            {
                foreach (TreeNode node in Nodes)
                {
                    LACTreeNode lac = (LACTreeNode)node;
                    if (lac != null)
                    {
                        lac.SetContext(ctx);
                    }
                }
            }

        }

        public IContext GetContext()
        {
            return _ctx;
        }

        public void toggleLACNode()
        {
            if (IsExpanded)
            {
                Collapse();
            }
            else
            {
                Expand();
            }
        }
        #endregion

        #region accessor functions
        public Bitmap NodeImage
        {
            get
            {
                return _image;
            }
            set
            {
                _image = value;
            }
        }

        public Type NodeType
        {
            get
            {
                return _t;
            }
            set
            {
                _t = value;
            }
        }

        public IPlugIn Plugin
        {
            get
            {
                return _plugin;
            }
            set
            {
                _plugin = value;
            }

        }

        public IPlugInPage PluginPage
        {
            set
            {
                _pluginPage = value;
            }
            get
            {
                return _pluginPage;
            }
        }
        #endregion
    }

    public class LACTreeNodeList : List<LACTreeNode>
    {

    }
}
