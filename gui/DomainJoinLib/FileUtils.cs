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
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace Centeris.DomainJoinLib
{
    internal class FileUtils
    {
        static public string ReadFile(string fileName)
        {
            //
            // If we used .NET 2.0, we could use File.ReadAllText().
            //
            StreamReader reader = new StreamReader(fileName, Encoding.ASCII);
            string contents = reader.ReadToEnd();
            reader.Close();
            return contents;
        }

        static public void CopyFile(string sourceName, string targetName)
        {
            string[] cmd = new string[] { "cp", "-p", sourceName, targetName };
            ExecUtils.RunCommand(cmd);
        }

        static public void RenameFile(string sourceName, string targetName)
        {
            string[] cmd = new string[] { "mv", sourceName, targetName };
            ExecUtils.RunCommand(cmd);
        }

        static public void BackupFile(string fileName, string backupFileName)
        {
            if (!File.Exists(new FileInfo(backupFileName).FullName))
            {
                CopyFile(fileName, backupFileName);
            }
        }

        static public void WriteFile(string fileName, string contents)
        {
            StreamWriter writer = new StreamWriter(fileName, false, Encoding.ASCII);
            writer.Write(contents);
            writer.Close();
        }

        static public void ReplaceFile(string fileName, string contents)
        {
            if (File.Exists(new FileInfo(fileName).FullName))
            {
                //
                // We are changing the file, so do a backup if there is none
                // already.
                //

                BackupFile(fileName, fileName + ".bak");

                //
                // We do a rename dance for atomic replacement.
                // Note that we copy the file first so that we have
                // the same perm and mode bits.
                //

                string tempFileName = fileName + ".new.tmp";
                CopyFile(fileName, tempFileName);
                WriteFile(tempFileName, contents);
                RenameFile(tempFileName, fileName);
            }
            else
            {
                //
                // ISSUE-This case should never happen given how we are
                // called, but if it does, we try our best.  However,
                // we are not verifying that we are setting the correct
                // mode bits, etc.
                //

                logger.debug("Unexpected case: new file");
                WriteFile(fileName, contents);
            }
        }
    }

    class TextFileUtils
    {
#if DISABLED
        /// <summary>
        /// FakseSed sort of simulates GNU sed.
        /// </summary>
        /// <param name="contents">File contents to which the operation applies.</param>
        /// <param name="matchPattern">A regular expression pattern</param>
        /// <param name="replacePattern">Use $1, $2 etc for match groups.</param>
        /// <param name="ignoreCase"></param>
        /// <param name="isGlobalReplace">Determines whether we replace first or all occurrences.</param>
        static public string FakeSed(string contents, string matchPattern, string replacePattern, bool ignoreCase, bool isGlobalReplace)
        {
            RegexOptions options = RegexOptions.Multiline;
            if (ignoreCase)
            {
                options |= RegexOptions.IgnoreCase;
            }
            Match match = Regex.Match(contents, matchPattern, options);
            while (match.Success)
            {
                match.Result(replacePattern);
                if (!isGlobalReplace)
                {
                    break;
                }
                match = match.NextMatch();
            }
            return contents;
        }
#endif

        /// <summary>
        /// FakseSed sort of simulates GNU sed.
        /// </summary>
        /// <param name="fileName">File to which the operation applies.</param>
        /// <param name="matchPattern">A regular expression pattern</param>
        /// <param name="replaceEvaluator">MatchEvaluator used to determine the replacement string.</param>
        /// <param name="ignoreCase"></param>
        /// <param name="backupFileName">Currently not used</param>
        static public void FakeSed(string fileName, string matchPattern, MatchEvaluator replaceEvaluator, bool ignoreCase, string backupFileName)
        {
            string contents = FileUtils.ReadFile(fileName);
            RegexOptions options = RegexOptions.Multiline;
            if (ignoreCase)
            {
                options |= RegexOptions.IgnoreCase;
            }
            contents = Regex.Replace(contents, matchPattern, replaceEvaluator, options);
            FileUtils.BackupFile(fileName, backupFileName == null ? fileName + ".bak" : backupFileName);
            FileUtils.ReplaceFile(fileName, contents);
        }

        /// <summary>
        /// FakseSed sort of simulates GNU sed.
        /// </summary>
        /// <param name="fileName">File to which the operation applies.</param>
        /// <param name="matchPattern">A regular expression pattern</param>
        /// <param name="replacePattern">Use $1, $2 etc for match groups.</param>
        /// <param name="ignoreCase"></param>
        /// <param name="backupFileName">Currently not used</param>
        static public void FakeSed(string fileName, string matchPattern, string replacePattern, bool ignoreCase, string backupFileName)
        {
            string contents = FileUtils.ReadFile(fileName);
            RegexOptions options = RegexOptions.Multiline;
            if (ignoreCase)
            {
                options |= RegexOptions.IgnoreCase;
            }
            contents = Regex.Replace(contents, matchPattern, replacePattern, options);
            FileUtils.BackupFile(fileName, backupFileName == null ? fileName + ".bak" : backupFileName);
            FileUtils.ReplaceFile(fileName, contents);
        }
        
        static public bool FakeGrep(string fileName, string matchPattern, bool ignoreCase)
        {
            string contents = FileUtils.ReadFile(fileName);
            RegexOptions options = RegexOptions.Multiline;
            if (ignoreCase)
            {
                options |= RegexOptions.IgnoreCase;
            }
            return Regex.IsMatch(contents, matchPattern, options);
        }
        
        static public void Append(string fileName, string appendString, string backupFileName)
        {
            string contents = FileUtils.ReadFile(fileName);
            FileUtils.BackupFile(fileName, backupFileName);
            FileUtils.ReplaceFile(fileName, contents + appendString);
        }

        static public void Append(string fileName, string appendString)
        {
            Append(fileName, appendString, fileName + ".bak");
        }
    }

}
