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
using System.Security.Principal;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.LDAP;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;
using System.Windows.Forms;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public class UserGroupUtils
{
    
    #region class data
    private const int base10 = 10;
    static char[] cHexa = new char[] { 'A', 'B', 'C', 'D', 'E', 'F' };
    static int[] iHexaNumeric = new int[] { 10, 11, 12, 13, 14, 15 };
    static int[] iHexaIndices = new int[] { 0, 1, 2, 3, 4, 5 };
    private const int asciiDiff = 48;
    #endregion
    
    #region Public Methods


    /// <summary>
    /// Splis the AD Object names from the distinguishedName
    /// </summary>
    /// <param name="groupDn"></param>
    /// <param name="aPartName"></param>
    /// <returns></returns>
    public static string[] splitDn(string groupDn, string aPartName)
    {
        if (string.IsNullOrEmpty(groupDn) || string.IsNullOrEmpty(aPartName))
            return null;
        try
        {

            string[] aParts = groupDn.Split(new char[]
                                                {
                                                  ','
                                                });
            string aPartAcitveDirectoryFolder = string.Empty, aPart = string.Empty;

            if (aParts[0].StartsWith("CN=", StringComparison.InvariantCultureIgnoreCase))
            {
                aPartName = aParts[0].Substring(3);
            }
            if (aParts[1].StartsWith("CN=", StringComparison.InvariantCultureIgnoreCase) ||
            aParts[1].StartsWith("OU=", StringComparison.InvariantCultureIgnoreCase))
            {
                aPart = aParts[1].Substring(3);
            }
            else
            {
                aPart = aParts[1];
            }
            for (int i = 2; i < aParts.Length; i++)
            {
                if (aParts[i].StartsWith("dc=", StringComparison.InvariantCultureIgnoreCase))
                {
                    aPartAcitveDirectoryFolder += "." + aParts[i].Substring(3);
                }
            }

            if (aPartAcitveDirectoryFolder.StartsWith("."))
            {
                aPartAcitveDirectoryFolder = aPartAcitveDirectoryFolder.Substring(1).Trim();
            }

            if (!string.IsNullOrEmpty(aPartAcitveDirectoryFolder))
            {
                aPartAcitveDirectoryFolder = aPartAcitveDirectoryFolder + "/" + aPart;
            }

            string[] slvItem = { aPartName, aPartAcitveDirectoryFolder };

            return slvItem;
        }
        catch (Exception ex)
        {
            Logger.LogException("UserGroupUtils.splitDn(string groupDn, string aPartName)", ex);
            return null;
        }
    }

    /// <summary>
    /// Splis the AD Object names from the distinguishedName
    /// </summary>
    /// <param name="groupDn"></param>
    /// <param name="aPartName"></param>
    /// <returns></returns>
    public static string[] splitDn(string groupDn)
    {
        if (string.IsNullOrEmpty(groupDn))
            return null;

        //As of now we are not getting canonicalName attribute in the list because of paging issue
        //LdapValue[] attr = ldapNextEntry.GetAttributeValues("canonicalName", dirnode.LdapContext);
        //if (attr != null && attr.Length > 0)
        //    this.lblCreatein.Text = "Create in: " + attr[0].stringData;

        //As of now we are taking "DistinguishedName" and spliting and displaying it.
        try
        {
            string[] aParts = groupDn.Split(new char[]
                                            {
                                                ','
                                            });
            string aPartAcitveDirectoryFolder = string.Empty, aPartName = string.Empty;

            if (aParts[0].StartsWith("CN=", StringComparison.InvariantCultureIgnoreCase))
            {
                aPartName = aParts[0].Substring(3);
            }

            for (int i = aParts.Length - 1; i >= 1; i--)
            {
                if (aParts[i].ToString().Trim().StartsWith("DC", StringComparison.InvariantCultureIgnoreCase))
                {
                    aPartAcitveDirectoryFolder = aParts[i].ToString().Trim().Substring(3) + "." + aPartAcitveDirectoryFolder;
                }
                else if (aParts[i].ToString().Trim().StartsWith("OU", StringComparison.InvariantCultureIgnoreCase) || aParts[i].ToString().Trim().StartsWith("CN", StringComparison.InvariantCultureIgnoreCase))
                {
                    aPartAcitveDirectoryFolder += "/" + aParts[i].ToString().Trim().Substring(3);
                }
            }
            if (aPartAcitveDirectoryFolder.EndsWith("."))
            {
                aPartAcitveDirectoryFolder += "/";
            }
            aPartAcitveDirectoryFolder = aPartAcitveDirectoryFolder.Replace("./", "/");

            string[] slvItem = { aPartName, aPartAcitveDirectoryFolder };

            return slvItem;
        }
        catch (Exception ex)
        {
            Logger.LogException("UserGroupUtils.splitDn(string groupDn)", ex);
            return null;
        }
    }
    
    /// <summary>
    /// Returns the list of LdapEntries for the selected AD Object
    /// </summary>
    /// <param name="useRootDn"></param>
    /// <param name="dirnode"></param>
    /// <param name="search_attrs"></param>
    /// <param name="searchFilter"></param>
    /// <param name="scope"></param>
    /// <returns></returns>
    public static List<LdapEntry> getLdapEntries(bool useRootDn, DirectoryNode dirnode, string[] search_attrs, string searchFilter, LdapAPI.LDAPSCOPE scope)
    {
        int ret = -1;
        DirectoryContext dirContext = dirnode.LdapContext;
        List<LdapEntry> ldapEntries = new List<LdapEntry>();
        
        if (!useRootDn)
        {
            ret = dirnode.LdapContext.ListChildEntriesSynchronous(
            dirnode.DistinguishedName,
            scope,
            searchFilter,
            null,
            false,
            out ldapEntries);
        }
        else
        {
            ret = dirnode.LdapContext.ListChildEntriesSynchronous
            (dirnode.LdapContext.RootDN,
            scope,
            searchFilter,
            null,
            false,
            out ldapEntries);
        }
        
        
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            return null;
        }
        
        return ldapEntries;
        
    }
    
    
    /// <summary>
    /// Converts a byte array sid into a string SID
    /// </summary>
    /// <param name="sidBytes"></param>
    /// <returns></returns>
    ///
    public static string SIDtoString(byte[] sidBinary)
    {
        //SecurityIdentifier sid = new SecurityIdentifier(sidBinary, 0);
        SecurityID sid = new SecurityID(sidBinary, 0);
        return sid.ToString();
    }
    
    //find the DN of given the groupsid
    public static string SearchBySid(string groupSid, DirectoryNode dirnode)
    {
        string searchFilter = groupSid;
        searchFilter = string.Concat("(objectSid=", groupSid, ")");
        
        List<LdapEntry> ldapEntries = getLdapEntries(true, dirnode, null, searchFilter, LdapAPI.LDAPSCOPE.SUB_TREE);
        
        if (ldapEntries != null && ldapEntries.Count > 0)
        {
            LdapEntry ldapNextEntry = ldapEntries[0];
            
            if (ldapNextEntry != null)
            {
                string[] attrsList = ldapNextEntry.GetAttributeNames();
                
                if (attrsList != null)
                {
                    foreach (string attr in attrsList)
                    {
                        if (attr.Equals("distinguishedName", StringComparison.InvariantCultureIgnoreCase))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                            
                            if (attrValues != null && attrValues.Length > 0)
                            {
                                return attrValues[0].stringData;
                            }
                        }
                    }
                }
            }
        }
        return null;
    }
    
    /// <summary>
    /// given a DN, retrieve the specified attribute's value (LdapValue[])
    /// </summary>
    /// <param name="groupDn">DN of the object in the directory node to search</param>
    /// <param name="dirnode">Directory node in which to search</param>
    /// <param name="attrName">Name of the attribute within the target object to retrieve</param>
    /// <returns></returns>
    public static LdapValue[] SearchAttrByDn(string groupDn, DirectoryNode dirnode, string attrName)
    {
        string searchFilter = "(objectClass=*)";
        string[] searchAttrs = new string[] { attrName };
        bool attrsOnly = false;
        List<LdapEntry> objectEntries = null;
        LdapValue[] objectAttrValues = null;
        int numObjectEntries = 0;
        int ret = 0;
        
        ret = dirnode.LdapContext.ListChildEntriesSynchronous(
        groupDn,
        LdapAPI.LDAPSCOPE.BASE,
        searchFilter,
        searchAttrs,
        attrsOnly,
        out objectEntries);
        
        if (objectEntries != null)
        {
            numObjectEntries = objectEntries.Count;
        }
        
        if (objectEntries == null || numObjectEntries != 1)
        {
            return null;
        }
        
        objectAttrValues = objectEntries[0].GetAttributeValues(
        attrName,
        dirnode.LdapContext);
        
        return objectAttrValues;
    }
    
    /// <summary>
    /// Gets the primary groupname from the "member" attribute list for the seleted "user" or "group"
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    public static string GetPrimaryGroup(DirectoryNode dirnode)
    {
        DirectoryContext dirContext = dirnode.LdapContext;
        Byte[] sidUser = null;
        Byte[] sidGroup = null;
        string sidUserStr = null;
        string primaryGroupID = "";
        
        string[] search_attrs = { null };
        
        List<LdapEntry> ldapEntries = getLdapEntries(false, dirnode, search_attrs, "(objectClass=*)", LdapAPI.LDAPSCOPE.BASE);
        
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            Logger.Log("ldapEntries.Count == 0");
            return null;
        }
        
        if (ldapEntries != null && ldapEntries.Count > 0)
        {
            LdapEntry ldapNextEntry = ldapEntries[0];
            
            if (ldapNextEntry != null)
            {
                
                string[] attrsList = ldapNextEntry.GetAttributeNames();
                
                if (attrsList != null)
                {
                    foreach (string attr in attrsList)
                    {
                        if (attr.Equals("objectsid", StringComparison.InvariantCultureIgnoreCase))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                            
                            if (attrValues != null && attrValues.Length > 0)
                            {
                                sidUser = attrValues[0].byteData;
                                sidUserStr = SIDtoString(sidUser);
                                //  Logger.Log("sid string is " + sidUserStr);
                            }
                        }
                        if (attr.Equals("primaryGroupId", StringComparison.InvariantCultureIgnoreCase))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                            
                            if (attrValues != null && attrValues.Length > 0)
                            {
                                primaryGroupID = attrValues[0].stringData;
                                sidGroup = LdapMessage.StrToByteArray(primaryGroupID.ToString());
                            }
                            
                        }
                    }
                    if (sidUserStr != null)
                    {
                        int endDomainPart = sidUserStr.LastIndexOf('-');
                        string sidGroupStr = sidUserStr.Substring(0, endDomainPart + 1);
                        sidGroupStr = sidGroupStr + primaryGroupID;
                        //use group sid to search for the group DN
                            /*primaryGroupSid = new Byte[sidUser.Length+sidGroup.Length];
                            int i = 0, j = 0;
                            for (i = 0; i < sidUser.Length; i++) primaryGroupSid[i] = sidUser[i];
                            for (j = 0; j < sidGroup.Length; j++) primaryGroupSid[i + j] = sidGroup[j];*/
                        
                        {
                            return SearchBySid(sidGroupStr, dirnode);
                        }
                    }
                }
            }
        }
        return null;
    }
    
    
    /// <summary>
    /// given a dirnode, get the groups that this user belongs to
    /// union of (1) primaryGroupId (2) member of property values
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    public static string[] GetGroupsforUser(DirectoryNode dirnode)
    {
        string[] search_attrs = { "memberOf", null };
        bool foundMemOf = false;
        string[] groupDns = null; //plus the primary group the user is in
        List<LdapEntry> ldapEntries;
        
        ldapEntries = getLdapEntries(false, dirnode, search_attrs, "(objectClass=*)", LdapAPI.LDAPSCOPE.BASE);
        
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            Logger.Log("ldapEntries.Count == 0");
            return null;
        }
        
        if (ldapEntries != null && ldapEntries.Count > 0)
        {
            LdapEntry ldapNextEntry = ldapEntries[0];
            if (ldapNextEntry != null)
            {
                
                string[] attrsList = ldapNextEntry.GetAttributeNames();
                
                if (attrsList != null)
                {
                    foreach (string attr in attrsList)
                    {
                        if (attr.Equals("memberOf", StringComparison.InvariantCultureIgnoreCase))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                            
                            string primaryGroupDn = GetPrimaryGroup(dirnode);
                            if (primaryGroupDn != null)
                            {
                                groupDns = new string[attrValues.Length + 1];
                                groupDns[attrValues.Length] = primaryGroupDn;
                            }
                            else
                            {
                                groupDns = new string[attrValues.Length];
                            }
                            
                            int i = 0;
                            foreach (LdapValue value in attrValues)
                            {
                                groupDns[i] = value.stringData;
                                i++;
                            }
                            foundMemOf = true;
                        }
                    }
                    if (!foundMemOf)
                    {
                        string primaryGroupDn = GetPrimaryGroup(dirnode);
                        if (primaryGroupDn != null)
                        {
                            groupDns = new string[1];
                            groupDns[0] = primaryGroupDn;
                        }
                    }
                }
            }
        }
        return groupDns;
    }
    
    /// <summary>
    /// Gets the primary group token
    /// </summary>
    /// <param name="sidGroupStr"></param>
    /// <returns></returns>
    public static string getRid(string sidGroupStr)
    {
        int endDomainRID = sidGroupStr.LastIndexOf('-');
        string primaryGroupToken = sidGroupStr.Substring(endDomainRID + 1);
        
        return primaryGroupToken;
    }
    
    
    public static Dictionary<string,string> GetGroupMembers(DirectoryNode dirnode)
    {
        //(1)member list (2) search all objects whose primaryGrouId = group's rid
        //first get this group dirnode's objectSid in order to get its RID to form filter
        string[] search_attrs = { "objectsid", "member", null };
        string RID = "";
        string sidGroupStr = null;
        Dictionary<string, string> member = new Dictionary<string,string>();
        List<LdapEntry> ldapEntries;
        
        ldapEntries = getLdapEntries(false, dirnode, search_attrs, "(objectClass=*)", LdapAPI.LDAPSCOPE.BASE);
        if (ldapEntries == null || ldapEntries.Count == 0)
        {
            Logger.Log("ldapEntries.Count == 0");
            return null;
        }
        
        if (ldapEntries != null && ldapEntries.Count > 0)
        {
            LdapEntry ldapNextEntry = ldapEntries[0];
            if (ldapNextEntry != null)
            {
                string[] attrsList = ldapNextEntry.GetAttributeNames();
                
                
                if (attrsList != null)
                {
                    foreach (string attr in attrsList)
                    {
                        if (attr.Equals("objectSid", StringComparison.InvariantCultureIgnoreCase))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                            sidGroupStr = SIDtoString(attrValues[0].byteData);
                            RID = getRid(sidGroupStr);
                        }
                        if (attr.Equals("member", StringComparison.InvariantCultureIgnoreCase))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                            foreach (LdapValue attrValue in attrValues)
                            {
                                string ObjClass = "";
                                if (attrValue.stringData.Trim().ToLower().Contains("CN=ForeignSecurityPrincipals".ToLower()))
                                {
                                    System.DirectoryServices.DirectoryEntry de = new System.DirectoryServices.DirectoryEntry(attrValue.stringData, dirnode.LdapContext.UserName, dirnode.LdapContext.Password);
                                    object[] Classes = de.Properties["objectClass"].Value as object[];
                                    foreach (string cls in Classes)
                                    {
                                        if (cls.Equals("foreignSecurityPrincipal", StringComparison.InvariantCultureIgnoreCase))
                                        {
                                            ObjClass = "foreignSecurityPrincipal";
                                        
}
                                    
}

                                        
                                    }
                                    member.Add(attrValue.stringData, ObjClass);
                                }
                            }
                        }
                    }
                }
            }
            //find in the scope of domain that those objects with primiaryGroupId = RID
            string searchfilter = "(primaryGroupId=";
            searchfilter = string.Concat(searchfilter, RID, ")");
            
            searchfilter = "(&" + searchfilter + "(|(objectClass=group)(objectClass=user)))";
            
            List<LdapEntry> ldapEntries1 = getLdapEntries(true, dirnode, search_attrs, searchfilter, LdapAPI.LDAPSCOPE.SUB_TREE);
            
            if (ldapEntries1 != null && ldapEntries1.Count > 0)
            {
                foreach (LdapEntry ldapNextEntry in ldapEntries1)
                {
                    if (ldapNextEntry != null)
                    {
                        string[] attrsList = ldapNextEntry.GetAttributeNames();
                        if (attrsList != null)
                        {
                            foreach (string attr in attrsList)
                            {
                                if (attr.Equals("distinguishedName", StringComparison.InvariantCultureIgnoreCase))
                                {
                                    LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                                    string ObjClass = "";
                                    if (attrValues[0].stringData.Trim().ToLower().Contains("CN=ForeignSecurityPrincipals".ToLower()))
                                    {
                                        System.DirectoryServices.DirectoryEntry de = new System.DirectoryServices.DirectoryEntry(attrValues[0].stringData, dirnode.LdapContext.UserName, dirnode.LdapContext.Password);
                                        object[] Classes = de.Properties["objectClass"].Value as object[];
                                        foreach (string cls in Classes)
                                        {
                                            if (cls.Equals("foreignSecurityPrincipal", StringComparison.InvariantCultureIgnoreCase))
                                            {
                                                ObjClass = "foreignSecurityPrincipal";
                                        
}
                                    
}

                                            
                                        }
                                        member.Add(attrValues[0].stringData, ObjClass);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                
                return member;
            }
            
            public static string GetGroupFromForeignSecurity(string Sid,DirectoryContext dirContext)
            {
                int ret = -1;
                string dn = string.Concat("CN=WellKnown Security Principals,CN=Configuration,", dirContext.RootDN);
                List<LdapEntry> ldapEntries = null;
                
                ret = dirContext.ListChildEntriesSynchronous(
                dn,
                LdapAPI.LDAPSCOPE.SUB_TREE,
                "(objectClass=*)",
                null,
                false,
                out ldapEntries);
                if (ldapEntries == null || ldapEntries.Count == 0)
                {
                    Logger.Log("ldapEntries.Count == 0");
                    return null;
                }
                //LdapEntry ldapNextEntry = ldapEntries[0];
                
                foreach (LdapEntry ldapNextEntry in ldapEntries)
                {
                    string[] attrsList = ldapNextEntry.GetAttributeNames();
                    
                    if (attrsList != null)
                    {
                        foreach (string attr in attrsList)
                        {
                            if (attr.Equals("objectSid", StringComparison.InvariantCultureIgnoreCase))
                            {
                                byte[] attrSid = null;
                                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirContext);
                                if (attrValues != null)
                                {
                                    attrSid = attrValues[0].byteData;
                                }
                                SecurityID sidObj=new SecurityID(attrSid,0);
                                string sSid = sidObj.ToString();
                                if (sSid.Equals(Sid))
                                {
                                    LdapValue[] attrsid = ldapNextEntry.GetAttributeValues("cn", dirContext);
                                    if (attrsid != null && attrsid.Length != 0)
                                    {
                                        return attrsid[0].stringData;
                                    }
                                    
                                }
                            }
                        }
                    }
                }
                return null;
            }
            
            public static string isGroupHasGidNumberSet(DirectoryNode dirnode)
            {
                int ret = -1;
                DirectoryContext dirContext = dirnode.LdapContext;
                
                List<LdapEntry> ldapEntries = null;
                ret = dirContext.ListChildEntriesSynchronous(
                dirnode.DistinguishedName,
                LdapAPI.LDAPSCOPE.BASE,
                "(objectClass=*)",
                null,
                false,
                out ldapEntries);
                
                if (ldapEntries == null || ldapEntries.Count == 0)
                {
                    Logger.Log("ldapEntries.Count == 0");
                    return null;
                }
                LdapEntry ldapNextEntry = ldapEntries[0];
                
                if (ldapNextEntry != null)
                {
                    
                    string[] attrsList = ldapNextEntry.GetAttributeNames();
                    
                    
                    if (attrsList != null)
                    {
                        foreach (string attr in attrsList)
                        {
                            if (attr.Equals("gidNumber", StringComparison.InvariantCultureIgnoreCase))
                            {
                                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirnode.LdapContext);
                                if (attrValues != null)
                                {
                                    return attrValues[0].stringData;
                                }
                            }
                        }
                    }
                }
                return null;
            }
            
            public static void GetObjectSidGuidBytes(DirectoryNode dirnode, out byte[] objectGUID, out byte[] objectSid)
            {
                if (dirnode == null)
                {
                    objectSid = null;
                    objectGUID = null;
                    return;
                }
                
                int ret = -1;
                string[] attrs = { "objectSid", "objectGUID", null };
                
                List<LdapEntry> ldapEntries = null;
                
                ret = dirnode.LdapContext.ListChildEntriesSynchronous
                (dirnode.DistinguishedName,
                LdapAPI.LDAPSCOPE.BASE,
                "(objectClass=*)",
                attrs,
                false,
                out ldapEntries);
                
                if (ldapEntries == null || ldapEntries.Count == 0)
                {
                    objectSid = null;
                    objectGUID = null;
                    return;
                }
                
                LdapEntry ldapNextEntry = ldapEntries[0];
                
                LdapValue[] attrValuesGUID = ldapNextEntry.GetAttributeValues("objectGUID", dirnode.LdapContext);
                
                if (attrValuesGUID != null && attrValuesGUID.Length > 0)
                {
                    objectGUID = attrValuesGUID[0].byteData;
                }
                else
                {
                    objectGUID = null;
                }
                
                LdapValue[] attrValuesSid = ldapNextEntry.GetAttributeValues("objectSid", dirnode.LdapContext);
                
                if (attrValuesSid != null && attrValuesSid.Length > 0)
                {
                    objectSid = attrValuesSid[0].byteData;
                }
                else
                {
                    objectSid = null;
                }
            }
            
            
            
            //Mapping a sid to a uin
            public static uint GetMappedID(byte[] absid, MPPage page)
            {
                // verify it's version 1
                if (absid[0] != 1)
                {
                    if (page != null)
                    {
                        MessageBox.Show(
                        page,
                        "Unexpected sid type!",
                        CommonResources.GetString("Caption_Console"),
                        MessageBoxButtons.OK);
                    }
                    
                    return 0;
                }
                
                // get the subauthority count
                int cSubs = (int)absid[1];
                
                // get the subauthority values
                uint[] auiSubs = new uint[cSubs];
                for (int i = 0; i < cSubs; i++)
                {
                    auiSubs[i] = BitConverter.ToUInt32(absid, i * 4 + 8);
                }
                
                // xor the last three (non rid) subauths
                uint uHash = 0;
                if (cSubs > 3)
                {
                    uHash = auiSubs[cSubs - 4] ^ auiSubs[cSubs - 3] ^ auiSubs[cSubs - 2];
                }
                
                // squish into 12 bits
                uHash = (((uHash & 0xFFF00000) >> 20) +
                ((uHash & 0x000FFF00) >> 8) +
                ((uHash & 0x000000FF))) & 0x0000FFF;
                
                // now, combine with 19 bits of the RID
                uint uNewHash = (uHash << 19) + (auiSubs[cSubs - 1] & 0x0007FFFF);
                
                // can't have this be the auto assign ID!
            /* if (uNewHash == DSInfo.AutoAssignID)
             {
                 throw new AuthException("Mapped SID==0xFFFFFFFF. This should never happen");
             }*/
                return uNewHash;
            }
            
            static public List<UserOrGroup> getUnixUsersAndGroups(DirectoryNode dirnode)
            {
                int ret = -1;
                List<UserOrGroup> usergroups = new List<UserOrGroup>();
                List<DirectoryNode> top_dirnodes = new List<DirectoryNode>();
                string[] attrs =
                {
                    "dummy", "objectClass", "distinguishedName", "uid",
                    "loginShell", "description", "gidNumber", "uidNumber", "name", "gecos", "keywords","unixHomeDirectory", "objectSid", null };
                
                List<LdapEntry> ldapEntries_top = null;
                
                ret = dirnode.LdapContext.ListChildEntriesSynchronous(
                dirnode.LdapContext.RootDN,
                LdapAPI.LDAPSCOPE.ONE_LEVEL,
                "(objectClass=*)",
                attrs,
                false,
                out ldapEntries_top);
                
                if (ldapEntries_top == null || ldapEntries_top.Count == 0)
                {
                    Logger.Log("ldapEntries_top.Count == 0");
                    return null;
                }
                
                List<UserOrGroup> topUsersAndGroups = getUnixUsersAndGroups_inner(dirnode, ldapEntries_top);
                
                if (topUsersAndGroups != null && topUsersAndGroups.Count > 0)
                {
                    foreach (UserOrGroup usergroup in topUsersAndGroups)
                    {
                        usergroups.Add(usergroup);
            
}
        
}

                    
                    
                    
                    if (ldapEntries_top != null && ldapEntries_top.Count > 0)
                    {
                        foreach (LdapEntry ldapNextEntry in ldapEntries_top)
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues("distinguishedName", dirnode.LdapContext);
                            if (attrValues != null && attrValues.Length > 0)
                            {
                                DirectoryNode innerdirnode = new DirectoryNode(dirnode);
                                innerdirnode.DistinguishedName = attrValues[0].stringData;
                                top_dirnodes.Add(innerdirnode);
                            }
                        }
                    }
                    
                    foreach (DirectoryNode inner_dirnode in top_dirnodes)
                    {
                        List<LdapEntry> ldapEntries = null;
                        ret =dirnode.LdapContext.ListChildEntriesSynchronous
                        (dirnode.DistinguishedName,
                        LdapAPI.LDAPSCOPE.SUB_TREE,
                        "(objectClass=*)",
                        attrs,
                        false,
                        out ldapEntries);
                        
                        if (ldapEntries == null || ldapEntries.Count == 0)
                        {
                            Logger.Log("ldapEntries.Count == 0");
                            return null;
                        }
                        
                        List<UserOrGroup> innerusergroups = getUnixUsersAndGroups_inner(inner_dirnode,ldapEntries);
                        
                        if (innerusergroups != null && innerusergroups.Count > 0)
                        {
                            foreach (UserOrGroup usergroup in innerusergroups)
                            {
                                usergroups.Add(usergroup);
                
}
            
}

                            
                            
                        }
                        
                        return usergroups;
                        
                    }
                    
                    
                    
                    static public List<UserOrGroup> getUnixUsersAndGroups_inner(DirectoryNode dirnode, List<LdapEntry> ldapEntries)
                    {
                        string object_class;
                        List<UserOrGroup> usergroups = new List<UserOrGroup>();
                        if (ldapEntries == null || ldapEntries.Count == 0)
                        {
                            Logger.Log("ldapEntries.Count == 0");
                            return null;
                        }
                        
                        if (ldapEntries != null && ldapEntries.Count > 0)
                        {
                            foreach (LdapEntry ldapNextEntry in ldapEntries)
                            {
                                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues("objectClass", dirnode.LdapContext);
                                
                                if (attrValues != null && attrValues.Length > 0)
                                {
                                    object_class = attrValues[attrValues.Length - 1].stringData;
                                    if (object_class.Equals("user", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        //get the username, uid, gid
                                        UserOrGroup newUser = new UserOrGroup();
                                        newUser.obj_class = object_class;
                                        LdapValue[] attrUserDn = ldapNextEntry.GetAttributeValues("distinguishedName", dirnode.LdapContext);
                                        if (attrUserDn != null && attrUserDn.Length > 0)
                                        {
                                            newUser.name = attrUserDn[0].stringData;
                                        }
                                        
                                        LdapValue[] attrUserUid = ldapNextEntry.GetAttributeValues("uidNumber", dirnode.LdapContext);
                                        if (attrUserUid != null && attrUserUid.Length > 0)
                                        {
                                            newUser.uid = attrUserUid[0].stringData;
                                        }
                                        
                                        LdapValue[] attrUserGid = ldapNextEntry.GetAttributeValues("gidNumber", dirnode.LdapContext);
                                        if (attrUserGid != null && attrUserGid.Length > 0)
                                        {
                                            newUser.gid = attrUserGid[0].stringData;
                                        }
                                        LdapValue[] attrUnixHomeDir = ldapNextEntry.GetAttributeValues("unixHomeDirectory", dirnode.LdapContext);
                                        if (attrUnixHomeDir != null && attrUnixHomeDir.Length > 0)
                                        {
                                            newUser.unixHomedir = attrUnixHomeDir[0].stringData;
                                        }
                                        LdapValue[] attrloginshell = ldapNextEntry.GetAttributeValues("loginShell", dirnode.LdapContext);
                                        if (attrloginshell != null && attrloginshell.Length > 0)
                                        {
                                            newUser.loginShell = attrloginshell[0].stringData;
                                        }
                                        LdapValue[] attrsid = ldapNextEntry.GetAttributeValues("objectSid", dirnode.LdapContext);
                                        if (attrsid != null && attrsid.Length > 0)
                                        {
                                            SecurityID sid = new SecurityID(attrsid[0].byteData, 0);
                                            newUser.objectSid = sid.ToString();
                                        }
                                        
                                        if (!newUser.uid.Equals("",StringComparison.InvariantCultureIgnoreCase))
                                        {
                                            usergroups.Add(newUser);
                                        }
                                    }
                                    if (object_class.Equals("group", StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        //getLdapEntries the group name, gid
                                        UserOrGroup newGroup = new UserOrGroup();
                                        newGroup.obj_class = object_class;
                                        LdapValue[] attrGroupDn = ldapNextEntry.GetAttributeValues("distinguishedName", dirnode.LdapContext);
                                        if (attrGroupDn != null && attrGroupDn.Length > 0)
                                        {
                                            newGroup.name = attrGroupDn[0].stringData;
                                        }
                                        
                                        LdapValue[] attrGroupGid = ldapNextEntry.GetAttributeValues("gidNumber", dirnode.LdapContext);
                                        if (attrGroupGid != null && attrGroupGid.Length > 0)
                                        {
                                            newGroup.gid = attrGroupGid[0].stringData;
                                        }
                                        LdapValue[] attrsid = ldapNextEntry.GetAttributeValues("objectSid", dirnode.LdapContext);
                                        if (attrsid != null && attrsid.Length > 0)
                                        {
                                            SecurityID sid = new SecurityID(attrsid[0].byteData, 0);
                                            newGroup.objectSid = sid.ToString();
                                        }
                                        
                                        if (!newGroup.gid.Equals("", StringComparison.InvariantCultureIgnoreCase))
                                        {
                                            newGroup.uid = null;
                                            usergroups.Add(newGroup);
                                        }
                                    }
                                }
                            }
                        }
                        
                        return usergroups;
                    }
                    
                    
                    //- RootDSE -> configurationNamingContext
                    //- cn=partition, "configurationNamingContext" -> netBIOSName
                    //Using the value of "netBIOSName" as shortDomain name.
                    public static string getnetBiosName(DirectoryNode dirnode)
                    {
                        int ret = -1;
                        string configurationName = null;
                        string netbiosName = null;
                        string baseDn = "";
                        string[] search_attrs = { null };
                        
                        //searching with baseDn="" allows ldap to access the domain “RootDSE”.
                        //Without passing that, it cannot access the configurationNamingContext
                        List<LdapEntry> ldapEntries = null;
                        ret = dirnode.LdapContext.ListChildEntriesSynchronous
                        (baseDn,
                        LdapAPI.LDAPSCOPE.BASE,
                        "(objectClass=*)",
                        search_attrs,
                        false,
                        out ldapEntries);
                        
                        if (ldapEntries != null && ldapEntries.Count > 0)
                        {
                            LdapEntry ldapNextEntry = ldapEntries[0];

                            LdapValue[] values = ldapNextEntry.GetAttributeValues("configurationNamingContext", dirnode.LdapContext);

                            if (values != null && values.Length > 0)
                            {
                                configurationName = values[0].stringData;
                            }
                            Logger.Log("configurationNamingContext is " + configurationName);
                        }
                        //by default, if we couldn't find configurateName we use CN=configuration + rootDN as one
                        if (configurationName == null)
                        {
                            configurationName = "CN=configuration,";
                            configurationName = string.Concat(configurationName, dirnode.LdapContext.RootDN);
                        }
                        
                        string partitionDn = "CN=Partitions,";
                        
                        partitionDn = string.Concat(partitionDn, configurationName);
                        
                        string domainName = dirnode.LdapContext.RootDN; //DC=qadom,dc=centeris,dc=com
                        
                        domainName = domainName.Replace("DC=", ""); ;
                        domainName = domainName.Replace("dc=", "");
                        domainName = domainName.Replace(",", ".");
                        
                        string sFilter = "(&(objectcategory=Crossref)(dnsRoot=" + domainName.ToLower() + ")(netBIOSName=*))";
                        
                        List<LdapEntry> ldapEntries1 = null;
                        ret = dirnode.LdapContext.ListChildEntriesSynchronous
                        (partitionDn,
                        LdapAPI.LDAPSCOPE.SUB_TREE,
                        sFilter,
                        search_attrs,
                        false,
                        out ldapEntries1);
                        
                        if (ldapEntries1 != null && ldapEntries1.Count > 0)
                        {
                            LdapEntry ldapNextEntry = ldapEntries1[0];
                            
                            LdapValue[] values = ldapNextEntry.GetAttributeValues("netBIOSName", dirnode.LdapContext);
                            netbiosName = values[0].stringData;
                        }
                        
                        //by default, if we couldn't find netbiosName we use the first portion of rootDn as one
                        if (netbiosName == null)
                        {
                            string[] rootDns = dirnode.LdapContext.RootDN.Split(',');
                            netbiosName = rootDns[0].Substring(3).ToUpper();
                        }
                        
                        Logger.Log("netbiosName is " + netbiosName);
                        
                        return netbiosName;
                        
                    }
                    
                    //- RootDSE -> configurationNamingContext
                    //- cn=partition, get "msDS-Behavior-Version" value
                    public static int getForestFunctionalLevel(DirectoryNode dirnode)
                    {
                        int ret = -1;
                        string configurationName = null;
                        int domainoperating_level = -1;
                        string baseDn = "";
                        string[] search_attrs = { null };
                        
                        //searching with baseDn="" allows ldap to access the domain “RootDSE”.
                        //Without passing that, it cannot access the configurationNamingContext
                        List<LdapEntry> ldapEntries = null;
                        ret = dirnode.LdapContext.ListChildEntriesSynchronous
                        (baseDn,
                        LdapAPI.LDAPSCOPE.BASE,
                        "(objectClass=*)",
                        search_attrs,
                        false,
                        out ldapEntries);
                        
                        if (ldapEntries != null && ldapEntries.Count > 0)
                        {
                            LdapEntry ldapNextEntry = ldapEntries[0];
                            
                            LdapValue[] values = ldapNextEntry.GetAttributeValues("configurationNamingContext", dirnode.LdapContext);
                            
                            if (values != null && values.Length > 0)
                            {
                                configurationName = values[0].stringData;
                            }
                            Logger.Log("configurationNamingContext is " + configurationName);
                        }
                        //by default, if we couldn't find configurateName we use CN=configuration + rootDN as one
                        if (configurationName == null)
                        {
                            configurationName = "CN=configuration,";
                            configurationName = string.Concat(configurationName, dirnode.LdapContext.RootDN);
                        }
                        
                        string partitionDn = "CN=Partitions,";
                        
                        partitionDn = string.Concat(partitionDn, configurationName);
                        
                        string domainName = dirnode.LdapContext.RootDN; //DC=qadom,dc=centeris,dc=com
                        
                        domainName = domainName.Replace("DC=", ""); ;
                        domainName = domainName.Replace("dc=", "");
                        domainName = domainName.Replace(",", ".");
                        
                        string[] search_attrs_partition = { "msDS-Behavior-Version", null };
                        
                        List<LdapEntry> ldapEntries1 = null;
                        ret = dirnode.LdapContext.ListChildEntriesSynchronous
                        (partitionDn,
                        LdapAPI.LDAPSCOPE.BASE,
                        "(objectClass=*)",
                        search_attrs_partition,
                        false,
                        out ldapEntries1);
                        
                        if (ldapEntries1 != null && ldapEntries1.Count > 0)
                        {
                            LdapEntry ldapNextEntry = ldapEntries1[0];
                            
                            LdapValue[] values = ldapNextEntry.GetAttributeValues("msDS-Behavior-Version", dirnode.LdapContext);
                            domainoperating_level = values[0].intData;
                        }
                        
                        Logger.Log("msDS-Behavior-Version is " + domainoperating_level);
                        
                        return domainoperating_level;
                        
                    }
                    
                    public static LdapEntry GetSchemaClass(string cnFilter, DirectoryNode dirnode)
                    {
                        int ret = -1;
                        string[] search_attrs = { "dummy", "distinguishedName", "cn", "isMemberOfPartialAttributeSet", "searchFlags", null };
                        string baseDn = string.Concat("CN=schema,CN=configuration,", dirnode.LdapContext.RootDN);
                        
                        //searching with baseDn="CN=schema, rootDN"
                        List<LdapEntry> ldapEntries = null;
                        ret = dirnode.LdapContext.ListChildEntriesSynchronous
                        (baseDn,
                        LdapAPI.LDAPSCOPE.SUB_TREE,
                        cnFilter,
                        search_attrs,
                        false,
                        out ldapEntries);
                        
                        if (ldapEntries != null && ldapEntries.Count > 0)
                        {
                            LdapEntry ldapNextEntry = ldapEntries[0];
                            
                            LdapValue[] values = ldapNextEntry.GetAttributeValues("distinguishedName", dirnode.LdapContext);
                            
                            if (values != null && values.Length > 0)
                            {
                                string[] splits = values[0].stringData.Split(',');
                                if (splits[0].Equals(cnFilter, StringComparison.InvariantCultureIgnoreCase))
                                {
                                    Logger.Log("In GetSchemaClass function, " + cnFilter + " is found!");
                                    return ldapNextEntry;
                                }
                            }
                        }
                        
                        return null;
                    }
                    
                    
                    public static LdapEntry GetSchemaAttribute(string attrFilter, DirectoryNode dirnode)
                    {
                        return GetSchemaClass(attrFilter, dirnode);
                    }
                    
                    
                    
                    //obtain objectSid of the "cn=Domain users" objectClass of "group" in the current domain
                    public static byte[] GetDomainUsersSid(DirectoryNode dirnode)
                    {
                        int ret = -1;
                        string sFilter = "(&(objectClass=group)(cn=Domain Users))";
                        string[] search_attrs = {"dummy", "distinguishedName", "objectSid", null};
                        
                        List<LdapEntry> ldapEntries = null;
                        
                        ret = dirnode.LdapContext.ListChildEntriesSynchronous
                        (dirnode.LdapContext.RootDN,
                        LdapAPI.LDAPSCOPE.SUB_TREE,
                        sFilter,
                        search_attrs,
                        false,
                        out ldapEntries);
                        
                        if (ldapEntries != null && ldapEntries.Count > 0)
                        {
                            LdapEntry ldapNextEntry = ldapEntries[0];
                            
                            LdapValue[] values = ldapNextEntry.GetAttributeValues("objectSid", dirnode.LdapContext);
                            return values[0].byteData;
                        }
                        
                        return null;
                    }
                    
                    public static string DecimalToBase(int iDec, int numbase)
                    {
                        string strBin = "";
                        int[] result = new int[32];
                        int MaxBit = 32;
                        for (; iDec > 0; iDec /= numbase)
                        {
                            int rem = iDec % numbase;
                            result[--MaxBit] = rem;
                        }
                        for (int i = 0; i < result.Length; i++)
                        {
                            if ((int)result.GetValue(i) >= base10)
                            {
                                strBin += cHexa[(int)result.GetValue(i) % base10];
                            }
                            else
                            {
                                strBin += result.GetValue(i);
                            }
                        }
                        strBin = strBin.TrimStart(new char[]
                        {
                            '0'
                        }
                        );
                        return strBin;
                    }
                    
                    #endregion
                    
                }
                
                public class UserOrGroup
                {
                    #region Class Data
                    public string name = "";
                    public string uid = "";
                    public string gid = "";
                    public string objectSid = "";
                    public string obj_class = "";
                    
                    public string unixHomedir = "";
                    public string loginShell = "";
                    #endregion
                    
                }
            }
            
