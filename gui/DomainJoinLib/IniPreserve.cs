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
using System.IO;
using System.Text;

namespace Centeris.DomainJoinLib
{
	
    public class AssertionException : ApplicationException
    {
        public AssertionException(string message) : base(message)
        {
        }
        
        public AssertionException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }

	/// This class is designed to read and write ini files (like smb.conf), while
	/// attempting to preserve as much as possible. Modifying or removing a section will 
	/// lose all comments within that section. Modified sections will show up in the
	/// original section location.
	/// 
	/// ini files look something like this:
	/// ; comments must begin the line according to smb.conf
	/// # comment
	/// # global section
	/// [global]
	/// # key/value pairs under each section
	/// description = true
	/// bob = "foo"
	/// 
	/// # another section
	/// [section3]
	/// betty = "boop"
	
	class IniPreserve
	{
		private string fileName;
		private ArrayList fileLines;
		private ArrayList sections; // Mapping of section headers and the lines they're on
		private string tab = "    "; // Use this as our tab for inserting keys
		
		// read the lines of the file into a local array, and initialize 'sections'
		public IniPreserve(string iniFileName)
		{
			fileName = iniFileName;
			try
			{
			    //
			    // Note that the default is UTF-8.
			    //

			    FileStream stream = new FileStream(fileName, FileMode.Open, FileAccess.Read);
				StreamReader br = new StreamReader(stream);
				
				string line;
				fileLines = new ArrayList();
				while ((line = br.ReadLine()) != null)
				{
					fileLines.Add(line);
				}
				br.Close();
				findSections();
			}
			catch (IOException)
			{
				throw new IOException("Unsupported encoding exception in IniPreserve");
			}
		}
		
		// find all the sections in the array, and save for later
		private void findSections()
		{
			IniSection currentSection = null;
			
			sections = new ArrayList();
            for (int i = 0; i < fileLines.Count; i++)
            {
                string line = (string) fileLines[i];

                // clean up the line
                line.Trim();
                if (line.Length == 0 || line.StartsWith(";") || line.StartsWith("#"))
                {
                    continue;
                }

                if (line.StartsWith("["))
                {
                    // section
                    int endOfSectionName = line.IndexOf("]");
                    int startOfSectionName = 1;
                    string sectionName = line.Substring(startOfSectionName, (endOfSectionName) - (startOfSectionName));
                    currentSection = new IniSection(sectionName);
                    currentSection.StartLine = i;
                    currentSection.EndLine = i;
                    sections.Add(currentSection);
                }
                else
                {
                    // must be a key value pair
                    if (null == currentSection)
                    {
                        continue;
                    } // line before section? 
                    int equals;
                    if ((equals = line.IndexOf("=")) != -1)
                    {
                        int beginningOfLine = 0;
                        string key = line.Substring(beginningOfLine, (equals) - (beginningOfLine));
                        string value_Renamed = line.Substring(equals + 1);
                        key = key.Trim();
                        value_Renamed = value_Renamed.Trim();
                        currentSection.putKey(key, value_Renamed);
                        currentSection.EndLine = i;
                    }
                    else
                    {
                        // bad line? skip it.
                        continue;
                    }
                }
            }
		}
		
		// add a new empty section at the end of the array
		public IniSection addSection(string sectionName)
		{
			IniSection ini;
			
			try
			{
				ini = getSection(sectionName);
				throw new IniSectionAlreadyExists();
			}
			catch (IniSectionNotFound)
			{
				// This is good, we don't want it to exist!
			}
			fileLines.Add("");
			fileLines.Add("[" + sectionName + "]");
			findSections();
			try
			{
				ini = getSection(sectionName);
			}
			catch (IniSectionNotFound)
			{
                throw new AssertionException("IniSectionNotFound exception right after adding it. Ieeeeeeeee!");
			}
			return ini;
		}
		
		// remove a section from the array
		public void  removeSection(string sectionName)
		{
			try
			{
				IniSection section = getSection(sectionName);
				for (int i = section.StartLine; i <= section.EndLine; i++)
				{
					fileLines.RemoveAt(section.StartLine); //remove the first line over and over
				}
				// remove any trailing empty lines
				int startLine = section.StartLine;
				while (startLine < fileLines.Count && ((string) fileLines[startLine]).Trim().Length == 0)
					fileLines.RemoveAt(startLine);
				
				// We will be at the end of the file if this was the last section in the file.
				// So, trim all blank lines above the section as well.
				if (startLine == fileLines.Count)
					for (int i = startLine - 1; i >= 0 && ((string) fileLines[i]).Trim().Length == 0; i--)
						fileLines.RemoveAt(i);
			}
			catch (IniSectionNotFound)
			{
				// section is already gone - quietly ignore?
			}
			findSections();
		}
		
		// return a hash full of the key/value pairs in a given section
		public IniSection getSection(string sectionName)
		{
		    foreach (IniSection section in sections)
		    {
                if (section.Name.Equals(sectionName))
                {
                    return section;
                }
		    }
			throw new IniSectionNotFound();
		}
		
		// replaces the section in the array	
		public void replaceSection(IniSection section)
		{
			removeSection(section.Name);
			//UPGRADE_TODO: Method 'java.util.HashMap.keySet' was converted to 'SupportClass.HashSetSupport' which has a different behavior. "ms-help://MS.VSCC.v80/dv_commoner/local/redirect.htm?index='!DefaultContextWindowIndex'&keyword='jlca1073_javautilHashMapkeySet'"
			int insertLine = section.StartLine;
			if (insertLine >= fileLines.Count)
			{
				fileLines.Add("");
				insertLine = fileLines.Count;
			}
			fileLines.Insert(insertLine++, "[" + section.Name + "]");
            foreach (string keyName in section.Keys.Keys)
			{
				string keyValue = (string) section.Keys[keyName];
				string line = tab + keyName + " = " + keyValue;
				fileLines.Insert(insertLine++, line);
			}
			// this eventually leads to a lot of leftover whitespace
			//		fileLines.add(insertLine++, "");
			findSections();
		}
		
		// make sure the section name is legal
		// we don't allow the character %
		// or any of our reserved section names
		public virtual bool legalSectionName(string section)
		{
			string[] illegalWords = new string[]{"global", "ipc$", "admin$", "print$", "homes", "printers", "centeris$", "c$"};
			
			int notFound = - 1;
			if (section.IndexOf("%") != notFound)
			{
				return false;
			}
			
			int stringsMatch = 0;
			for (int i = 0; i < illegalWords.Length; i++)
			{
				if (stringsMatch == string.Compare(section, illegalWords[i], true))
				{
					return false;
				}
			}
			return true;
		}
		
        private static void AtomicReplace(string source, string destination)
        {
            ExecUtils.RunCommand("/bin/mv " + source + " " + destination);
        }
	    
		// save the array back to disk
		public virtual void  commit()
		{
			try
			{
                //string tempFileName = fileName + ".temp";
                StreamWriter ps = new StreamWriter(fileName, false, Encoding.ASCII);
			    foreach (string line in fileLines)
				{
					ps.WriteLine(line);
				}
				ps.Close();
			    //AtomicReplace(tempFileName, fileName);
			}
			catch (FileNotFoundException e)
			{
                throw new AssertionException("FileNotFoundException writing to config file?", e);
			}
			catch (IOException e)
			{
                throw new AssertionException("UnsupportedEncodingException", e);
			}
		}
	}
}