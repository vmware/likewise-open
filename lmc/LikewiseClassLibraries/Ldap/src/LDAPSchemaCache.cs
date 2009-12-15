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
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using Likewise.LMC.Utilities;
using Likewise.LMC.LDAP.Interop;
using System.Runtime.InteropServices;


namespace Likewise.LMC.LDAP
{
    //maps AttributeName? to LdapValue
    public class AttributeMap:Dictionary<string, LdapValue[]>
    {
        public DirectoryContext ldapContext = null;
        public Dictionary<string, LdapEntry> ObjectClasses = new Dictionary<string, LdapEntry>();
        public object Tag = null;
    }

    public class LDAPSchemaCache
    {
        #region Class data

        private static Mutex instanceMtx = new Mutex();

        //maps the common name ('cn') of an attribute to its type (SchemaType)
        private readonly Dictionary<string, SchemaType> _attrTypeLookup;

        //maps the display name ('dn') of an attribute to its type (SchemaType)
        private readonly Dictionary<string, SchemaType> _dnTypeLookup;

        //maps the common name 'cn' of an ObjectClass to its definition (SchemaType)
        private readonly Dictionary<string, SchemaType> _classTypeLookup;

        public string rootDN = null;

        private const string CN_TAG                  = "cn";
        private const string OBJECT_CATEGORY_TAG     = "objectCategory";
        private const string ATTR_SYNTAX_TAG         = "attributeSyntax";
        private const string IS_SINGLE_VALUED_TAG    = "isSingleValued";
        private const string ATTR_DISPLAYNAME_TAG    = "lDAPDisplayName";
        private const string SUBCLASS_OF_TAG         = "subclassOf";
        private const string SYSTEM_MUST_CONTAIN_TAG = "systemMustContain";
        private const string SYSTEM_MAY_CONTAIN_TAG  = "systemMayContain";

        #endregion

        #region Constructor

        protected LDAPSchemaCache()
        {
            _attrTypeLookup = new Dictionary<string, SchemaType>();
            _classTypeLookup = new Dictionary<string, SchemaType>();
            _dnTypeLookup = new Dictionary<string, SchemaType>();
        }

        #endregion

        #region Public methods

        //To get 'ObjectClass' attribute for the selected key
        public string[] GetObjectClasses()
        {
            string[] result = null;

            int nKeys = _classTypeLookup.Keys.Count;
            if (nKeys > 0)
            {
                result = new string[_classTypeLookup.Keys.Count];

                _classTypeLookup.Keys.CopyTo(result, 0);
            }

            return result;
        }

        public LdapAttributeType[] GetAttributesForClass(string objectClassName)
        {
            List<LdapAttributeType> result = new List<LdapAttributeType>();
            string className = objectClassName;

            while (!string.IsNullOrEmpty(className))
            {
                LdapClassType classType = this.GetSchemaTypeByDisplayName(className) as LdapClassType;
                if (classType == null)
                {
                    className = null;
                    break;
                }

                if (classType.MandatoryAttributes != null)
                {
                    foreach (string attr in classType.MandatoryAttributes)
                    {
                        SchemaType attrSchemaType = this.GetSchemaTypeByDisplayName(attr);
                        if (attrSchemaType != null)
                        {
                            attrSchemaType.AttributeType = "Mandatory";
                            LdapAttributeType attrLdapAttributeType = attrSchemaType as LdapAttributeType;
                            result.Add(attrLdapAttributeType);
                        }
                    }
                }

                if (classType.OptionalAttributes != null)
                {
                    foreach (string attr in classType.OptionalAttributes)
                    {
                        SchemaType attrSchemaType = this.GetSchemaTypeByDisplayName(attr);
                        if (attrSchemaType != null)
                        {
                            attrSchemaType.AttributeType = "Optional";
                            LdapAttributeType attrLdapAttributeType = attrSchemaType as LdapAttributeType;
                            result.Add(attrLdapAttributeType);
                        }
                    }
                }

                className = classType.SuperClassName;
            }

            if (result.Count > 0)
            {
                return result.ToArray();
            }
            else
            {
                return null;
            }
        }

        protected bool AddAttributeSchemaType(AttributeMap attribute_map)
        {
            LdapValue[] ldapvalues = getFirstValue(attribute_map, CN_TAG);
            string cName = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                cName = ldapvalues[0].stringData;
            }

            ldapvalues = getFirstValue(attribute_map, ATTR_SYNTAX_TAG);
            string attributeSyntax = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                attributeSyntax = ldapvalues[0].stringData;
            }

            ldapvalues = getFirstValue(attribute_map, IS_SINGLE_VALUED_TAG);
            string isSingleValued = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                isSingleValued = ldapvalues[0].stringData;
            }

            ldapvalues = getFirstValue(attribute_map, ATTR_DISPLAYNAME_TAG);
            string attributeDisplayName = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                attributeDisplayName = ldapvalues[0].stringData;
            }


            if ((attributeSyntax == null) || (isSingleValued == null))
            {
                Logger.Log("Add one entry in schemacache failed", Logger.ldapLogLevel);
                return false;
            }

            LdapAttributeType schemaType = new LdapAttributeType(cName, attributeSyntax, attributeDisplayName);
            schemaType.Tag = attribute_map;

            if (String.Compare(isSingleValued, "TRUE") == 0)
            {
                schemaType.IsSingleValued = true;
            }
            else
            {
                schemaType.IsSingleValued = false;
            }

            if (!_attrTypeLookup.ContainsKey(cName.ToLower()))
            {
                _attrTypeLookup.Add(cName.ToLower(), schemaType);
            }
            if (!_dnTypeLookup.ContainsKey(attributeDisplayName.ToLower()))
            {
                _dnTypeLookup.Add(attributeDisplayName.ToLower(), schemaType);
            }

            return true;
        }

        protected bool AddClassSchemaType(AttributeMap attribute_map)
        {
            LdapValue[] ldapvalues = getFirstValue(attribute_map, CN_TAG);
            string cName = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                cName = ldapvalues[0].stringData;
            }

            ldapvalues = getFirstValue(attribute_map, ATTR_SYNTAX_TAG);
            string attributeSyntax = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                attributeSyntax = ldapvalues[0].stringData;
            }

            ldapvalues = getFirstValue(attribute_map, IS_SINGLE_VALUED_TAG);
            string isSingleValued = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                isSingleValued = ldapvalues[0].stringData;
            }

            ldapvalues = getFirstValue(attribute_map, ATTR_DISPLAYNAME_TAG);
            string attributeDisplayName = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                attributeDisplayName = ldapvalues[0].stringData;
            }

            ldapvalues = getFirstValue(attribute_map, SUBCLASS_OF_TAG);
            string superClassName = null;
            if (ldapvalues != null && ldapvalues.Length > 0)
            {
                superClassName = ldapvalues[0].stringData;
            }

            string[] systemMustContain = GetAttrsWithTag(attribute_map, SYSTEM_MUST_CONTAIN_TAG);
            string[] systemMayContain = GetAttrsWithTag(attribute_map, SYSTEM_MAY_CONTAIN_TAG);

            //5th Parameter is Mandatory and 6th Parameter is Optional atrributes.
            LdapClassType schemaType = new LdapClassType(
                                                   cName,
                                                   attributeSyntax,
                                                   attributeDisplayName,
                                                   superClassName,
                                                   systemMustContain,
                                                   systemMayContain
                                                   );
            schemaType.Tag = attribute_map;

            if (String.Compare(isSingleValued, "TRUE") == 0)
            {
                schemaType.IsSingleValued = true;
            }
            else
            {
                schemaType.IsSingleValued = false;
            }

            if (!_classTypeLookup.ContainsKey(cName.ToLower()))
            {
                _classTypeLookup.Add(cName.ToLower(), schemaType);
            }

            if (!_dnTypeLookup.ContainsKey(attributeDisplayName.ToLower()))
            {
                _dnTypeLookup.Add(attributeDisplayName.ToLower(), schemaType);
            }

            return true;
        }

        protected bool AddSchemaType(AttributeMap attribute_map)
        {
            LdapValue[] ldapValues = getFirstValue(attribute_map, OBJECT_CATEGORY_TAG);

            if (ldapValues == null ||
                ldapValues.Length == 0)
            {
                return false;
            }

            string objectCategory = ldapValues[0].stringData;

            if(String.IsNullOrEmpty(objectCategory))
            {
                return false;
            }

            if (objectCategory.StartsWith("CN=Attribute-Schema", StringComparison.InvariantCultureIgnoreCase))
            {
                return AddAttributeSchemaType(attribute_map);
            }
            else if (objectCategory.StartsWith("CN=Class-Schema", StringComparison.InvariantCultureIgnoreCase))
            {
                return AddClassSchemaType(attribute_map);
            }
            else
            {
                Logger.Log(String.Format(
                    "Failed to cache schema type - [{0}]",
                    objectCategory), Logger.LogLevel.Error);

                return false;
            }
        }

        protected static LdapValue[] getFirstValue(AttributeMap attribute_map, string attr_name)
        {
            if (attr_name != null && attribute_map.ContainsKey(attr_name))
            {
                LdapValue[] values = attribute_map[attr_name];

                if (values != null &&
                    values.Length > 0)
                {
                    return values;
                }
            }

            return null;
        }

        public void reportSchemaCache(Logger.LogLevel logLevel)
        {
            if (logLevel > Logger.currentLogLevel)
            {
                return;
            }

            foreach (KeyValuePair<string, SchemaType> entry in _attrTypeLookup)
            {
                string key = entry.Key as string;
                LdapAttributeType schemaType = entry.Value as LdapAttributeType;
                if (schemaType != null)
                {
                    Logger.Log("Attribute Key = " + key, logLevel);
                    Logger.Log("    SchemaType::CName      = " + schemaType.CName, logLevel);
                    Logger.Log("              ::AttrSyntax = " + schemaType.AttributeSyntax, logLevel);
                    Logger.Log("              ::DataTypa   = " + schemaType.DataType, logLevel);
                    Logger.Log("              ::AttributeDisplayName   = " + schemaType.AttributeDisplayName, logLevel);
                }
            }
            foreach (KeyValuePair<string, SchemaType> entry in _classTypeLookup)
            {
                string key = entry.Key as string;
                LdapClassType schemaType = entry.Value as LdapClassType;
                if (schemaType != null)
                {
                    Logger.Log("Class Key = " + key, logLevel);
                    Logger.Log("    SchemaType::CName      = " + schemaType.CName, logLevel);
                    Logger.Log("              ::AttrSyntax = " + schemaType.AttributeSyntax, logLevel);
                    Logger.Log("              ::DataTypa   = " + schemaType.DataType, logLevel);
                    Logger.Log("              ::AttributeDisplayName   = " + schemaType.AttributeDisplayName, logLevel);
                }
            }

            foreach (KeyValuePair<string, SchemaType> entry in _dnTypeLookup)
            {
                string key = entry.Key as string;
                SchemaType schemaType = entry.Value as SchemaType;
                if (schemaType != null)
                {
                    Logger.Log("Display Name Key = " + key, logLevel);
                    Logger.Log("    SchemaType::CName      = " + schemaType.CName, logLevel);
                    Logger.Log("              ::AttrSyntax = " + schemaType.AttributeSyntax, logLevel);
                    Logger.Log("              ::DataTypa   = " + schemaType.DataType, logLevel);
                    Logger.Log("              ::AttributeDisplayName   = " + schemaType.AttributeDisplayName, logLevel);
                }
            }

        }

        public static LDAPSchemaCache Build(DirectoryContext ldapContext)
        {
            DateTime BuildTimer = DateTime.Now;

            LDAPSchemaCache result = null;

            List<LdapEntry> ldapEntries = new List<LdapEntry>();
            Dictionary<string, LdapEntry> ldapEntriesDic = new Dictionary<string, LdapEntry>();

            string[] search_attrs = { null };

            DateTime timer = Logger.StartTimer();

            Logger.Log(
                "Started building schema cache",
                Logger.ldapLogLevel);

            int ret = ldapContext.ListChildEntriesSynchronous(
                                  string.Concat("cn=Schema,", ldapContext.ConfigurationNamingContext),
                                  LdapAPI.LDAPSCOPE.SUB_TREE,
                                  "(objectClass=*)",
                                  search_attrs,
                                  false,
                                  out ldapEntries);

            if (ldapEntries != null && ldapEntries.Count > 0)
            {
                Logger.TimerMsg(ref timer,
                    String.Format(
                        "schema cache: {0} entries read from LDAP",
                        ldapEntries.Count));
                result = new LDAPSchemaCache();
                result.rootDN = ldapContext.RootDN;
                int entryIndex = 0;

                foreach (LdapEntry entry in ldapEntries)
                {
                    LdapValue[] attrName = entry.GetAttributeValues("lDAPDisplayName", ldapContext);
                    if (attrName != null)
                    {
                        ldapEntriesDic.Add(attrName[0].stringData, entry);
                    }
                }

                foreach (LdapEntry ldapNextEntry in ldapEntries)
                {
                    AttributeMap attrmap = new AttributeMap();

                    string dn = ldapNextEntry.GetDN();

                    Logger.Log(String.Format(
                        "LDAPSchemaCache.Build: ldapEntry[{0}]: DN = " + dn,
                        entryIndex++),
                        Logger.ldapLogLevel);

                    //Getting 'objectClasss' value instead 'subClassOf' attribute
                    //becuase 'subClassOf' attribute is not giving the all mandatory attribute for the selected object.
                    LdapValue[] objectClasses = ldapNextEntry.GetAttributeValues("objectClass", ldapContext);
                    foreach (LdapValue Oclass in objectClasses)
                    {
                        if (ldapEntriesDic.ContainsKey(Oclass.stringData))
                        {
                            attrmap.ObjectClasses.Add(Oclass.stringData, ldapEntriesDic[Oclass.stringData]);
                        }
                    }
                    attrmap.ldapContext = ldapContext;
                    attrmap.Tag = ldapNextEntry;

                    string[] attrs = ldapNextEntry.GetAttributeNames();

                    foreach (string attr in attrs)
                    {
                        LdapValue[] values = ldapNextEntry.GetAttributeValues(attr, ldapContext);

                        attrmap.Add(attr, values);
                    }

                    result.AddSchemaType(attrmap);
                }

                Logger.TimerMsg(ref timer,
                    String.Format(
                        "LDAPSchemaCache.Build({0})-- finished building the schemaCache",
                        ldapContext.RootDN));

                result.reportSchemaCache(Logger.ldapLogLevel);
            }


            return result;
        }

        //maps the common name ('cn') of an attribute to its type (SchemaType)
        public SchemaType GetSchemaTypeByCommonName(string CN)
        {
            SchemaType foundType = null;

            string key = CN.ToLower();

            try
            {
                foundType = this._attrTypeLookup[key];
            }
            catch (KeyNotFoundException e)
            {
                Logger.LogException(String.Format(
                    "LdapSchemaCache.GetSchemaTypeByCN(CName={0})", key), e);
            }

            return foundType;
        }

        //maps the display name ('dn') of an attribute to its type (SchemaType)
        public SchemaType GetSchemaTypeByDisplayName(string DN)
        {
            SchemaType foundType = null;

            string key = DN.ToLower();

            try
            {
                //foundType = _dnTypeLookup[key];
                foreach (string Class in this._dnTypeLookup.Keys)
                {
                    SchemaType type = this._dnTypeLookup[Class];
                    if (type.AttributeDisplayName.Trim().ToLower().Equals(key))
                    {
                        foundType = this._dnTypeLookup[Class];
                        break;
                    }
                }
            }
            catch (KeyNotFoundException e)
            {
                Logger.LogException(String.Format(
                    "LdapSchemaCache.GetSchemaTypeByDN(DN={0})", key), e);
            }

            return foundType;
        }

        //maps an object class to its type (SchemaType)
        public SchemaType GetSchemaTypeByObjectClass(string objectClass)
        {
            SchemaType foundType = null;

            string key = objectClass.ToLower();

            try
            {
                foreach (string Class in this._classTypeLookup.Keys)
                {
                    SchemaType type = this._classTypeLookup[Class];
                    if (type.AttributeDisplayName.Trim().ToLower().Equals(key))
                    {
                        foundType = this._classTypeLookup[Class];
                        break;
                    }
                }
            }
            catch (KeyNotFoundException e)
            {
                Logger.LogException(String.Format(
                    "LdapSchemaCache.GetSchemaTypeByCName(objectClass={0})", key), e);
            }

            return foundType;
        }

        #endregion

        #region helper methods

        /// Getting attributes with give tag from all parent classes
        /// use dictionary to improve set operation performance
        private string[] GetAttrsWithTag(AttributeMap attrMap, string sTag)
        {
            Dictionary<string, int> resultAttrSet = new Dictionary<string, int>();
            //foreach (LdapEntry ldapEntry in attrMap.ObjectClasses.Values)
            //{
            //LdapValue[] ldapValues = ldapEntry.GetAttributeValues(
            //    sTag,
            //    attrMap.ldapContext);
            LdapValue[] ldapValues = getFirstValue(attrMap, sTag);

            if (ldapValues == null ||
                ldapValues.Length == 0)
            {
                return null;
            }

            foreach (LdapValue ldapValue in ldapValues)
            {
                string newKey = ldapValue.stringData;
                if (!String.IsNullOrEmpty(newKey) &&
                    !resultAttrSet.ContainsKey(newKey))
                {
                    resultAttrSet.Add(newKey, 0);
                }
            }
            //}
            string[] resultAttrArray = new string[resultAttrSet.Count];
            resultAttrSet.Keys.CopyTo(resultAttrArray, 0);
            return resultAttrArray;
        }
        #endregion

    }
}
