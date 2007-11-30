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

namespace Centeris.DomainJoinLib
{
	
	/// Keep all the useful information about an ini section
	/// Useful external functions are putKey and getKeys
	
	class IniSection
	{
		virtual public int StartLine
		{
			get
			{
				return startLine;
			}
			
			set
			{
				startLine = value;
			}
			
		}
		virtual public int EndLine
		{
			get
			{
				return endLine;
			}
			
			set
			{
				endLine = value;
			}
			
		}
		virtual public String Name
		{
			get
			{
				return sectionName;
			}
			
		}
		//UPGRADE_TODO: Class 'java.util.HashMap' was converted to 'System.Collections.Hashtable' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javautilHashMap'"
		virtual public Hashtable Keys
		{
			// allows you to get a hashmap of the key/value pairs for the section
			
			get
			{
				return keys;
			}
			
		}
		private int startLine;
		private int endLine;
		//UPGRADE_TODO: Class 'java.util.HashMap' was converted to 'System.Collections.Hashtable' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javautilHashMap'"
		private Hashtable keys;
		private String sectionName;
		
		/// <summary> Represents a section of an .ini file.
		/// 
		/// </summary>
		/// <param name="sectionName	The">name of the .ini section.
		/// </param>
		public IniSection(String sectionName)
		{
			this.sectionName = sectionName;
			//UPGRADE_TODO: Class 'java.util.HashMap' was converted to 'System.Collections.Hashtable' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javautilHashMap'"
			keys = new Hashtable();
		}
		
		// allows you to add a key/value pair to the section
		public virtual void  putKey(String keyName, String keyValue)
		{
			keys[keyName] = keyValue;
		}
		
		public virtual void  removeKey(String keyName)
		{
			keys.Remove(keyName);
		}
		
		public virtual String lookupKey(String keyName)
		{
			//UPGRADE_TODO: Method 'java.util.HashMap.get' was converted to 'System.Collections.Hashtable.Item' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javautilHashMapget_javalangObject'"
			return (String) keys[keyName];
		}
		
		public virtual void  removeAllKeys()
		{
			keys.Clear();
		}
	}
}