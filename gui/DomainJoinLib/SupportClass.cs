//
// Copyright (C) Centeris Corporation 2004-2007
// Copyright (C) Likewise Software 2007.  
// All rights reserved.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

using System;
using System.Collections;

#if NET_20
static
#endif
class SupportClass
{
    public static bool IsNullOrEmpty(string value)
    {
#if NET_20
        return String.IsNullOrEmpty(value);
#else
        return null == value || value.Length == 0;
#endif
    }

    public static bool IsEqualIgnoreCase(string strA, string strB)
    {
        return ( 0 == String.Compare(strA, strB, true) );
    }

    public static bool ContainsStringIgnoreCase(ArrayList entries, string entry)
    {
        for (int i = 0; i < entries.Count; i++)
        {
            if (IsEqualIgnoreCase((string)entries[i], entry))
            {
                return true;
            }
        }
        return false;
    }

}
