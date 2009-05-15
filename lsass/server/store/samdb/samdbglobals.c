/*
 * samdbglobals.c
 *
 *  Created on: Mar 9, 2009
 *      Author: krishnag
 */

#include "includes.h"

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassDomainAttrMaps[] =
    { SAMDB_DOMAIN_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassBuiltInDomainAttrMaps[] =
    { SAMDB_CONTAINER_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassContainerAttrMaps[] =
    { SAMDB_CONTAINER_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassGroupAttrMaps[] =
    { SAMDB_GROUP_ATTRIBUTE_MAP };

static
SAMDB_ATTRIBUTE_MAP_INFO gObjectClassUserAttrMaps[] =
    { SAMDB_USER_ATTRIBUTE_MAP };

static
SAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO gObjectClassMaps[] =
{
    {
        SAMDB_OBJECT_CLASS_DOMAIN,
        &gObjectClassDomainAttrMaps[0],
        sizeof(gObjectClassDomainAttrMaps)/sizeof(gObjectClassDomainAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN,
        &gObjectClassBuiltInDomainAttrMaps[0],
        sizeof(gObjectClassBuiltInDomainAttrMaps)/sizeof(gObjectClassBuiltInDomainAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_CONTAINER,
        &gObjectClassContainerAttrMaps[0],
        sizeof(gObjectClassContainerAttrMaps)/sizeof(gObjectClassContainerAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_LOCAL_GROUP,
        &gObjectClassGroupAttrMaps[0],
        sizeof(gObjectClassGroupAttrMaps)/sizeof(gObjectClassGroupAttrMaps[0])
    },
    {
        SAMDB_OBJECT_CLASS_USER,
        &gObjectClassUserAttrMaps[0],
        sizeof(gObjectClassUserAttrMaps)/sizeof(gObjectClassUserAttrMaps[0])
    }
};

static
SAM_DB_ATTRIBUTE_MAP gAttrMaps[] =
{
    SAMDB_OBJECT_ATTRIBUTE_MAP
};

SAM_GLOBALS gSamGlobals =
    {
        PTHREAD_MUTEX_INITIALIZER,
        &gObjectClassMaps[0],
        sizeof(gObjectClassMaps)/sizeof(gObjectClassMaps[0]),
        &gAttrMaps[0],
        sizeof(gAttrMaps)/sizeof(gAttrMaps[0])
    };
