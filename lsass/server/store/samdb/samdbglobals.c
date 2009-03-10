/*
 * samdbglobals.c
 *
 *  Created on: Mar 9, 2009
 *      Author: krishnag
 */

#include "includes.h"

PSTR gSamDbProviderName = "Likewise SAM Local Database";

DIRECTORY_PROVIDER_FUNCTION_TABLE gSamDbProviderAPITable =
{
        .pfnDirectoryOpen   = &SamDbOpen,
        .pfnDirectoryBind   = &SamDbBind,
        .pfnDirectoryAdd    = &SamDbAddObject,
        .pfnDirectoryModify = &SamDbModifyObject,
        .pfnDirectoryDelete = &SamDbDeleteObject,
        .pfnDirectorySearch = &SamDbSearchObject,
        .pfnDirectoryClose  = &SamDbClose
};
