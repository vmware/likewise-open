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
using System.Text;
using System.Collections.Specialized;

namespace System.DirectoryServices
{
    public class DirectorySearcher
    {
        private string sFilter;
        private DirectoryEntry deSearchRoot;
        private SearchScope searchScope;
        private int pageSize;
        private int sizeLimit;                  // TODO: Make use of this!
        private string[] propertiesToLoad;

        public DirectorySearcher()
        {
            pageSize = 1000;
            propertiesToLoad = null;
            searchScope = SearchScope.OneLevel;
            sFilter = "";
        }

        public DirectorySearcher(string sFilter) : this()
        {
            this.sFilter = sFilter;
        }

        public DirectorySearcher(DirectoryEntry deSearchRoot)
            : this()
        {
            this.deSearchRoot = deSearchRoot;
        }

        public DirectorySearcher(DirectoryEntry deSearchRoot, string sFilter)
            : this(deSearchRoot)
        {
            this.sFilter = sFilter;
        }

        public DirectorySearcher(DirectoryEntry deSearchRoot, string sFilter, SearchScope searchScope)
            : this(deSearchRoot, sFilter)
        {
            this.searchScope = searchScope;
        }

        public DirectorySearcher(DirectoryEntry deSearchRoot, string sFilter, string[] propertiesToLoad, SearchScope searchScope)
            : this(deSearchRoot, sFilter, searchScope)
        {
            this.propertiesToLoad = propertiesToLoad;
        }

        public string Filter
        {
            get
            {
                return sFilter;
            }
            set
            {
                sFilter = value;
            }
        }

        public DirectoryEntry SearchRoot
        {
            get
            {
                return deSearchRoot;
            }
            set
            {
                deSearchRoot = value;
            }
        }

        public SearchScope SearchScope
        {
            get
            {
                return searchScope;
            }
            set
            {
                searchScope = value;
            }
        }

        public int PageSize
        {
            get
            {
                return pageSize;
            }
            set
            {
                pageSize = value;
            }
        }

        public int SizeLimit
        {
            get
            {
                return sizeLimit;
            }
            set
            {
                sizeLimit = value;
            }
        }

        public SearchResult FindOne()
        {
            string sPath = deSearchRoot.FindFirstChild(sFilter, searchScope, propertiesToLoad);

            if (sPath == null) return null;

            return new SearchResult(sPath);
        }

        public SearchResultCollection FindAll()
        {
            List<string> sPaths = deSearchRoot.FindAllChild(sFilter, searchScope, propertiesToLoad);

            SearchResultCollection collectionResult = new SearchResultCollection();

            if (sPaths != null && sPaths.Count > 0)
            {
                foreach (string path in sPaths)
                {
                    collectionResult.Add(new SearchResult(path));

                    if (sizeLimit != 0 && collectionResult.Count == sizeLimit)
                    {
                        break;
                    }
                }
            }

            return collectionResult;
        }

        public StringCollection PropertiesToLoad
        {
            get
            {
                StringCollection properties = new StringCollection();

                if (propertiesToLoad != null && propertiesToLoad.Length > 0)
                    properties.AddRange(propertiesToLoad);

                return properties;
            }

            set
            {
                if (value != null && value.Count > 0)
                {
                    propertiesToLoad = new string[value.Count];

                    value.CopyTo(propertiesToLoad, 0);
                }
            }
        }


    }
}
