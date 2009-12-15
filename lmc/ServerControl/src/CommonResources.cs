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
using System.Windows.Forms;

namespace Likewise.LMC.ServerControl
{
    public class CommonResources
    {
        #region helper function
        public static string GetString(string resourceID)
        {
            return Properties.Resources.ResourceManager.GetString(resourceID);
        }

        public static System.Drawing.Icon GetIcon(string resourceID)
        {
            object obj = Properties.Resources.ResourceManager.GetObject(resourceID);
            return ((System.Drawing.Icon)(obj));
        }

        public static void AutoResizePage(ListView lv, int containerWidth)
        {

            if (lv == null || lv.Columns == null || lv.Columns.Count < 2)
            {
                return;
            }

            int numColumns = lv.Columns.Count;

            int minColumnWidth = (containerWidth / numColumns) / 2;

            int widthExpansion = containerWidth;

            int columnMargin = 10;


            lv.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);


            for (int i = 1; i < numColumns; i++)
            {
                widthExpansion -= lv.Columns[i].Width;
            }

            if (widthExpansion <= 0)
            {
                return;
            }

            for (int i = 1; i < numColumns; i++)
            {
                int requestedIncrease = 0;

                if (lv.Columns[i].Width + columnMargin < minColumnWidth)
                {
                    requestedIncrease = minColumnWidth - lv.Columns[i].Width;
                }
                else
                {
                    requestedIncrease = columnMargin;
                }

                if (requestedIncrease > 0)
                {
                    if (requestedIncrease > widthExpansion)
                    {
                        requestedIncrease = widthExpansion;
                    }

                    lv.Columns[i].Width += requestedIncrease;
                    widthExpansion -= requestedIncrease;
                }
            }
        }


        #endregion
    }
}
