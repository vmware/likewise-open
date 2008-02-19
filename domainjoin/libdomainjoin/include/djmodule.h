/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __DJMODULE_H__
#define __DJMODULE_H__

#include "ctarray.h"
#include "lwexc.h"

typedef enum _QueryResult
{
    CannotConfigure,
    NotConfigured,
    SufficientlyConfigured,
    FullyConfigured,
    NotApplicable,
} QueryResult;

struct _JoinModule;
typedef struct _JoinModule JoinModule;

//Used inside of JoinProcessOptions
typedef struct
{
    BOOLEAN runModule;
    QueryResult lastResult;
    const JoinModule *module;
    //Can be used by domainjoin frontend.
    void *userData;
    //Can be used by domainjoin module.
    void *moduleData;
} ModuleState;

struct _JoinProcessOptions;
typedef struct _JoinProcessOptions JoinProcessOptions;
typedef void (*WarningFunction)(JoinProcessOptions *options, const char *title, const char *message);

//Options filled in by the UI in order to communicate with the join modules.
struct _JoinProcessOptions
{
    PSTR domainName;
    PSTR shortDomainName;
    PSTR computerName;
    PSTR ouName;
    PSTR username;
    PSTR password;
    void *userData;
    //TRUE if joining to AD, FALSE if leaving
    BOOLEAN joiningDomain;
    BOOLEAN showTraces;
    WarningFunction warningCallback;
    /* Contains modules that are enabled and disabled by the user, but does
     * not contain NA modules. This list is populated from the moduleTable.
     * The data type inside of the array is ModuleState.
    */
    DynamicArray moduleStates;
};

//A struct that defines a join module.
struct _JoinModule
{
    BOOLEAN runByDefault;
    PCSTR shortName;
    PCSTR longName;

    //Every module has the following entry points
    QueryResult (*QueryState)(const JoinProcessOptions *, LWException **);
    void (*MakeChanges)(JoinProcessOptions *, LWException **);
    PSTR (*GetChangeDescription)(const JoinProcessOptions *, LWException **);
    void (*FreeModuleData)(const JoinProcessOptions *, ModuleState *state);
};

void DJZeroJoinProcessOptions(JoinProcessOptions *options);
void DJFreeJoinProcessOptions(JoinProcessOptions *options);
void DJRefreshModuleStates(JoinProcessOptions *options, LWException **err);
void DJInitModuleStates(JoinProcessOptions *options, LWException **err);
void DJRunJoinProcess(JoinProcessOptions *options, LWException **err);

ModuleState *DJGetModuleState(JoinProcessOptions *options, size_t index);
ModuleState *DJGetModuleStateByName(JoinProcessOptions *options, const char *shortName);

void DJEnableModule(JoinProcessOptions *options, PCSTR shortName, BOOLEAN enable, LWException **exc);

void DJCheckRequiredEnabled(const JoinProcessOptions *options, LWException **exc);

#endif // __DJMODULE_H__
