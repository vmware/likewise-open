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

#include "domainjoin.h"
#include "ctarray.h"
#include "ctstrutils.h"
#include "ctfileutils.h"
#include "djstr.h"
#include "djdistroinfo.h"

#define NSSWITCH_CONF_PATH "/etc/nsswitch.conf"
#define NSSWITCH_LWIDEFAULTS "/etc/nsswitch.lwi_defaults"

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

typedef struct
{
    char *leadingWhiteSpace;
    CTParseToken name;
    DynamicArray modules;
    char *comment;
} NsswitchEntry;

typedef struct
{
    char *filename;
    /*Holds NsswitchEntry*/
    DynamicArray lines;
    BOOLEAN modified;
} NsswitchConf;

static const NsswitchEntry * GetEntry(const NsswitchConf *conf, size_t index)
{
    if(index >= conf->lines.size)
        return NULL;
    return ((NsswitchEntry *)conf->lines.data) + index;
}

static CTParseToken * GetEntryModule(NsswitchEntry *entry, size_t index)
{
    if(index >= entry->modules.size)
        return NULL;
    return ((CTParseToken *)entry->modules.data) + index;
}

static void FreeNsswitchEntryContents(NsswitchEntry *entry)
{
    size_t i;
    for(i = 0; i < entry->modules.size; i++)
    {
        CTFreeParseTokenContents(GetEntryModule(entry, i));
    }
    CTArrayFree(&entry->modules);
    CT_SAFE_FREE_STRING(entry->leadingWhiteSpace);
    CT_SAFE_FREE_STRING(entry->comment);
    CTFreeParseTokenContents(&entry->name);
}

static void FreeNsswitchConfContents(NsswitchConf *conf)
{
    size_t i;
    for(i = 0; i < conf->lines.size; i++)
    {
        FreeNsswitchEntryContents(((NsswitchEntry *)conf->lines.data) + i);
    }
}

/* Get the printed form of a line from the parsed form by concatenating all of the strings together */
static CENTERROR GetPrintedLine(DynamicArray *dest, NsswitchConf *conf, int line)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    size_t len = 0;
    char *pos;
    int i;
    const NsswitchEntry *lineObj = GetEntry(conf, line);

    len += CTGetTokenLen(&lineObj->name);
    for(i = 0; i < lineObj->modules.size; i++)
    {
        len += CTGetTokenLen(&((CTParseToken *)lineObj->modules.data)[i]);
    }
    if(lineObj->comment != NULL)
        len += strlen(lineObj->comment);

    //For the terminating NULL
    len++;

    if(len > dest->capacity)
        GCE(ceError = CTSetCapacity(dest, 1, len));
    pos = dest->data;
    CTAppendTokenString(&pos, &lineObj->name);
    for(i = 0; i < lineObj->modules.size; i++)
    {
        CTAppendTokenString(&pos, &((CTParseToken *)lineObj->modules.data)[i]);
    }
    if(lineObj->comment != NULL)
    {
        memcpy(pos, lineObj->comment, strlen(lineObj->comment));
        pos += strlen(lineObj->comment);
    }
    *pos = '\0';
    dest->size = len;

cleanup:
    return ceError;
}

static CENTERROR AddFormattedLine(NsswitchConf *conf, const char *filename, const char *linestr, const char **endptr)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    NsswitchEntry lineObj;
    const char *pos = linestr;
    const char *token_start = NULL;
    CTParseToken token;

    memset(&lineObj, 0, sizeof(lineObj));
    memset(&token, 0, sizeof(token));

    /* Find the leading whitespace in the line */
    token_start = pos;
    while(isblank(*pos)) pos++;
    GCE(ceError = CTStrndup(token_start, pos - token_start, &lineObj.leadingWhiteSpace));

    /* Read the name of the entry and attach its trailing : or = */
    GCE(ceError = CTReadToken(&pos, &lineObj.name, "=: \t", ";#\r\n", ""));

    /* Create an array of the modules for this entry */
    while(strchr("\r\n;#", *pos) == NULL)
    {
        GCE(ceError = CTReadToken(&pos, &token, "=: \t", ";#\r\n", ""));
        GCE(ceError = CTArrayAppend(&lineObj.modules, sizeof(CTParseToken), &token, 1));
        memset(&token, 0, sizeof(token));
    }

    /*Read the comment, if there is one*/
    token_start = pos;
    while(strchr("\r\n", *pos) == NULL) pos++;

    if(pos != token_start)
        GCE(ceError = CTStrndup(token_start, pos - token_start, &lineObj.comment));

    GCE(ceError = CTArrayAppend(&conf->lines, sizeof(lineObj), &lineObj, 1));
    memset(&lineObj, 0, sizeof(lineObj));

    if(endptr != NULL)
        *endptr = pos;

cleanup:
    FreeNsswitchEntryContents(&lineObj);
    CTFreeParseTokenContents(&token);

    return ceError;
}

static CENTERROR ReadNsswitchFile(NsswitchConf *conf, const char *rootPrefix, const char *filename)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    FILE *file = NULL;
    PSTR buffer = NULL;
    char *fullPath = NULL;
    BOOLEAN endOfFile = FALSE;
    BOOLEAN exists;

    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = CTAllocateStringPrintf(
            &fullPath, "%s%s", rootPrefix, filename));
    DJ_LOG_INFO("Reading nsswitch file %s", fullPath);
    GCE(ceError = CTCheckFileOrLinkExists(fullPath, &exists));
    if(!exists)
    {
        DJ_LOG_INFO("File %s does not exist", fullPath);
        ceError = CENTERROR_INVALID_FILENAME;
        goto cleanup;
    }

    GCE(ceError = CTStrdup(filename,
        &conf->filename));
    GCE(ceError = CTOpenFile(fullPath, "r", &file));
    CT_SAFE_FREE_STRING(fullPath);
    while(TRUE)
    {
        CT_SAFE_FREE_STRING(buffer);
        GCE(ceError = CTReadNextLine(file, &buffer, &endOfFile));
        if(endOfFile)
            break;
        GCE(ceError = AddFormattedLine(conf, filename, buffer, NULL));
    }

    conf->modified = FALSE;

cleanup:
    CT_SAFE_FREE_STRING(buffer);
    if(file != NULL)
        CTCloseFile(file);
    CT_SAFE_FREE_STRING(fullPath);
    if(!CENTERROR_IS_OK(ceError))
        FreeNsswitchConfContents(conf);
    return ceError;
}

static CENTERROR WriteNsswitchConfiguration(const char *rootPrefix, NsswitchConf *conf)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DynamicArray printedLine;
    int i;
    char *tempName = NULL;
    char *finalName = NULL;
    FILE *file = NULL;
    memset(&printedLine, 0, sizeof(printedLine));

    DJ_LOG_INFO("Writing nsswitch configuration for %s", conf->filename);

    GCE(ceError = CTAllocateStringPrintf(&tempName, "%s%s.new", rootPrefix, conf->filename));
    ceError = CTOpenFile(tempName, "w", &file);
    if(!CENTERROR_IS_OK(ceError))
    {
        DJ_LOG_ERROR("Unable to open '%s' for writing", tempName);
        GCE(ceError);
    }

    for(i = 0; i < conf->lines.size; i++)
    {
        GCE(ceError = GetPrintedLine(&printedLine, conf, i));
        GCE(ceError = CTFilePrintf(file, "%s\n", printedLine.data));
    }

    GCE(ceError = CTCloseFile(file));
    file = NULL;

    /* Make sure to set perms */

    GCE(ceError = CTChangePermissions(tempName, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));

    GCE(ceError = CTAllocateStringPrintf(&finalName, "%s%s", rootPrefix, conf->filename));
    GCE(ceError = CTBackupFile(finalName));
    GCE(ceError = CTMoveFile(tempName, finalName));
    DJ_LOG_INFO("File moved into place");

cleanup:
    if(file != NULL)
        CTCloseFile(file);
    CTArrayFree(&printedLine);
    CT_SAFE_FREE_STRING(tempName);
    CT_SAFE_FREE_STRING(finalName);
    return ceError;
}

static int FindEntry(const NsswitchConf *conf, int startLine, const char *name)
{
    int i;
    if(startLine == -1)
        return -1;
    for(i = startLine; i < conf->lines.size; i++)
    {
        if(GetEntry(conf, i)->name.value != NULL &&
                !strcmp(GetEntry(conf, i)->name.value, name))
        {
            return i;
        }
    }
    return -1;
}

static CENTERROR AddEntry(NsswitchConf *conf, const DistroInfo *distro,
        int *addedIndex, const char *name)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line = -1;
    NsswitchEntry lineObj;
    const NsswitchEntry *copy;

    memset(&lineObj, 0, sizeof(lineObj));
    GCE(ceError = CTStrdup(name, &lineObj.name.value));

    for(line = 0; line < conf->lines.size; line++)
    {
        copy = GetEntry(conf, line);
        if(copy->name.value != NULL)
        {
            GCE(ceError = CTStrdup(copy->name.trailingSeparator, &lineObj.name.trailingSeparator));
            break;
        }
    }

    if(lineObj.name.trailingSeparator == NULL)
    {
        //Couldn't find an existing line to copy the separator from. We'll
        //have to guess based on the OS
        if(distro->os == OS_AIX)
            GCE(ceError = CTStrdup(" = ", &lineObj.name.trailingSeparator));
        else
            GCE(ceError = CTStrdup(": ", &lineObj.name.trailingSeparator));
    }

    GCE(ceError = CTArrayAppend(&conf->lines,
                sizeof(NsswitchEntry), &lineObj, 1));
    memset(&lineObj, 0, sizeof(lineObj));
    conf->modified = 1;
    if(addedIndex != NULL)
        *addedIndex = conf->lines.size - 1;

cleanup:
    FreeNsswitchEntryContents(&lineObj);
    return ceError;
}

const char * GetModuleSeparator(NsswitchConf *conf, const DistroInfo *distro)
{
    int line;
    const NsswitchEntry *copy;

    for(line = 0; line < conf->lines.size; line++)
    {
        copy = GetEntry(conf, line);
        if(copy->modules.size > 1)
        {
            /*This line has at least two modules in it. There will be a
             * separator after the first module.
             */
            return ((CTParseToken *)copy->modules.data)[0].trailingSeparator;
        }
    }
    /* We have to guess based on the OS */
    if(distro->os == OS_AIX)
    {
        return ", ";
    }
    return " ";
}

static int FindModuleOnLine(const NsswitchConf *conf, int line, const char *name)
{
    const NsswitchEntry *lineObj;
    int i;

    if(line < 0)
        return -1;
    lineObj = GetEntry(conf, line);
    for(i = 0; i < lineObj->modules.size; i++)
    {
        if(!strcmp(((CTParseToken *)lineObj->modules.data)[i].value, name))
            return i;
    }
    return -1;
}

static CENTERROR InsertModule(NsswitchConf *conf, const DistroInfo *distro,
        int line, int insertIndex, const char *name)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    NsswitchEntry *lineObj = (NsswitchEntry *)GetEntry(conf, line);
    CTParseToken *beforeModule = NULL, *afterModule = NULL;
    CTParseToken addModule;

    memset(&addModule, 0, sizeof(addModule));
    if(insertIndex == -1)
        insertIndex = lineObj->modules.size;

    GCE(ceError = CTStrdup(name, &addModule.value));

    if(insertIndex - 1 >= 0)
        beforeModule = (CTParseToken *)lineObj->modules.data + insertIndex - 1;
    if(insertIndex < lineObj->modules.size)
        afterModule = (CTParseToken *)lineObj->modules.data + insertIndex;

    if(beforeModule != NULL)
    {
        /* Copy the separator from the previous module */
        GCE(ceError = CTDupOrNullStr(beforeModule->trailingSeparator,
                &addModule.trailingSeparator));
        if(afterModule == NULL)
        {
            /*This is the last module.  Put in the correct separator after the
             * previous module */
            CT_SAFE_FREE_STRING(beforeModule->trailingSeparator);
            GCE(ceError = CTStrdup(GetModuleSeparator(conf, distro),
                    &beforeModule->trailingSeparator));
        }
    }
    else
    {
        if(afterModule == NULL)
        {
            //This is the last module
            if(lineObj->comment == NULL)
            {
                //Leave the trailingSeparator as NULL
            }
            else
            {
                GCE(ceError = CTStrdup(" ", &addModule.trailingSeparator));
            }
        }
        else
        {
            //This is the first module. Add the appropriate separator to
            //distinguish it from the next module.
            GCE(ceError = CTStrdup(GetModuleSeparator(conf, distro),
                    &addModule.trailingSeparator));
        }
    }

    GCE(ceError = CTArrayInsert(&lineObj->modules, insertIndex,
                sizeof(addModule), &addModule, 1));
    memset(&addModule, 0, sizeof(addModule));
    conf->modified = 1;

cleanup:
    CTFreeParseTokenContents(&addModule);
    return ceError;
}

static CENTERROR RemoveModule(NsswitchConf *conf, 
        int line, int moduleIndex)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    NsswitchEntry *lineObj = (NsswitchEntry *)GetEntry(conf, line);
    CTParseToken *beforeModule = NULL, *afterModule = NULL;
    CTParseToken *removeModule;

    removeModule = (CTParseToken *)lineObj->modules.data + moduleIndex;

    if(moduleIndex - 1 >= 0)
        beforeModule = (CTParseToken *)lineObj->modules.data + moduleIndex - 1;
    if(moduleIndex + 1 < lineObj->modules.size)
        afterModule = (CTParseToken *)lineObj->modules.data + moduleIndex + 1;

    if(afterModule == NULL && beforeModule != NULL)
    {
        /* Since the last module is being removed, move the trailingSeparator
         * to the previous module */
        CT_SAFE_FREE_STRING(beforeModule->trailingSeparator);
        beforeModule->trailingSeparator = removeModule->trailingSeparator;
        removeModule->trailingSeparator = NULL;
    }
    CTFreeParseTokenContents(removeModule);

    GCE(ceError = CTArrayRemove(&lineObj->modules, moduleIndex, sizeof(CTParseToken), 1));
    conf->modified = 1;

cleanup:
    return ceError;
}

CENTERROR
UnConfigureNameServiceSwitch()
{
    return DJConfigureNameServiceSwitch(NULL, FALSE);
}

CENTERROR
ReadNsswitchConf(NsswitchConf *conf, const char *testPrefix,
        BOOLEAN allowFileCreate)
{
    PSTR copyDestPath = NULL;
    PSTR defaultFilePath = NULL;
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN bFileExists = FALSE;
    memset(conf, 0, sizeof(*conf));
    
    //Keep trying to read different filenames until one of them is found
    if(!bFileExists)
    {
        bFileExists = TRUE;
        ceError = ReadNsswitchFile(conf, testPrefix, "/etc/nsswitch.conf");
        if(ceError == CENTERROR_INVALID_FILENAME)
        {
            bFileExists = FALSE;
            ceError = CENTERROR_SUCCESS;
        }
        GCE(ceError);
    }

    if(!bFileExists)
    {
        bFileExists = TRUE;
        ceError = ReadNsswitchFile(conf, testPrefix, "/etc/netsvc.conf");
        if(ceError == CENTERROR_INVALID_FILENAME)
        {
            bFileExists = FALSE;
            ceError = CENTERROR_SUCCESS;
        }
        GCE(ceError);
    }

    /* HP-UX 11.xx does not appear to have an nsswitch file in
       place by default. If we don't find on already installed,
       use our own */

    if(!bFileExists)
    {
        GCE(ceError = CTAllocateStringPrintf(
          &defaultFilePath, "%s%s", testPrefix, NSSWITCH_LWIDEFAULTS));
        GCE(ceError = CTCheckFileExists(defaultFilePath, &bFileExists));
      
        if (bFileExists) {
            ceError = ReadNsswitchFile(conf, testPrefix, NSSWITCH_LWIDEFAULTS);
            GCE(ceError);
            CT_SAFE_FREE_STRING(conf->filename);
            GCE(ceError = CTStrdup(NSSWITCH_CONF_PATH, &conf->filename));
            conf->modified = TRUE;

            if(allowFileCreate)
            {
                /* Copy over the original file. This way the user can more
                 * clearly see what we changed by comparing nsswitch.conf with
                 * nsswitch.conf.lwidentity.orig. Also, the permissions will be
                 * correct this way when the file is written out.
                 */
                DJ_LOG_INFO("Copying default nsswitch file");
                GCE(ceError = CTAllocateStringPrintf(
                    &copyDestPath, "%s%s", testPrefix, NSSWITCH_CONF_PATH));
                ceError = CTCopyFileWithOriginalPerms(defaultFilePath, copyDestPath);
                GCE(ceError);
            }
        }
    }

    if(!bFileExists)
    {
        GCE(ceError = CENTERROR_INVALID_FILENAME);
    }

cleanup:
    CT_SAFE_FREE_STRING(copyDestPath);
    CT_SAFE_FREE_STRING(defaultFilePath);

    return ceError;
}

CENTERROR
UpdateNsswitchConf(NsswitchConf *conf, BOOLEAN enable)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DistroInfo distro;
    int line;
    int lwiIndex;

    GCE(ceError = DJGetDistroInfo(NULL, &distro));

    line = FindEntry(conf, 0, "passwd");
    if(enable && line == -1)
    {
        DJ_LOG_INFO("Adding passwd line");
        GCE(ceError = AddEntry(conf, &distro, &line, "passwd"));
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }
    lwiIndex = FindModuleOnLine(conf, line, "lwidentity");
    if(enable && lwiIndex == -1)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, "lwidentity"));
    }
    if(!enable && lwiIndex != -1)
    {
        GCE(ceError = RemoveModule(conf, line, lwiIndex));
    }
    // If lwidentity was the only entry
    // and we removed that now, don't write
    // an empty entry into the file
    if(!enable && line != -1 && GetEntry(conf, line)->modules.size == 0)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }

    line = FindEntry(conf, 0, "group");
    if(line == -1)
    {
        line = FindEntry(conf, 0, "groups");
    }
    if(enable && line == -1)
    {
        /* The nsswitch file doesn't have an existing groups line. We have to
         * guess based on platform whether it uses 'group' or 'groups'.
         */
        const char *groupName = "group";
        if(distro.os == OS_AIX || distro.os == OS_DARWIN)
        {
            groupName = "groups";
        }
        DJ_LOG_INFO("Adding %s line", groupName);
        GCE(ceError = AddEntry(conf, &distro, &line, groupName));
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }
    lwiIndex = FindModuleOnLine(conf, line, "lwidentity");
    if(enable && lwiIndex == -1)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, "lwidentity"));
    }
    if(!enable && lwiIndex != -1)
    {
        GCE(ceError = RemoveModule(conf, line, lwiIndex));
    }
    // If lwidentity was the only entry
    // and we removed that now, don't write
    // an empty entry into the file
    if(!enable && line != -1 && GetEntry(conf, line)->modules.size == 0)
    {
        GCE(ceError = InsertModule(conf, &distro, line, -1, "files"));
    }

cleanup:
    DJFreeDistroInfo(&distro);

    return ceError;
}

CENTERROR
DJConfigureNameServiceSwitch(const char *testPrefix, BOOLEAN enable)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    NsswitchConf conf;

    if(testPrefix == NULL)
        testPrefix = "";

    ceError = ReadNsswitchConf(&conf, testPrefix, TRUE);
    if(ceError == CENTERROR_INVALID_FILENAME)
    {
        ceError = CENTERROR_SUCCESS;
        DJ_LOG_WARNING("Warning: Could not find nsswitch file");
        goto cleanup;
    }
    GCE(ceError);

    ceError = UpdateNsswitchConf(&conf, enable);

    if(conf.modified)
        WriteNsswitchConfiguration(testPrefix, &conf);
    else
        DJ_LOG_INFO("nsswitch not modified");

cleanup:
    FreeNsswitchConfContents(&conf);

    return ceError;
}

const char *GetNameOfHostsByFile(const NsswitchConf *conf, const DistroInfo *distro)
{
    int line = FindEntry(conf, 0, "hosts");

    if (FindModuleOnLine(conf, line, "local") != -1)
        return "local";

    if (FindModuleOnLine(conf, line, "files") != -1)
        return "files";

    if(distro->os == OS_AIX)
        return "local";
    else
        return "files";
}

const char *GetNameOfHostsByDns(const NsswitchConf *conf, const DistroInfo *distro)
{
    int line = FindEntry(conf, 0, "hosts");

    if (FindModuleOnLine(conf, line, "dns") != -1)
        return "dns";

    if (FindModuleOnLine(conf, line, "bind") != -1)
        return "bind";

    if(distro->os == OS_AIX)
        return "bind";
    else
        return "dns";
}

//Does the platform-specific equivalent of this in nsswitch.conf:
// hosts: files dns
CENTERROR
DJConfigureHostsEntry(const char *testPrefix)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    NsswitchConf conf;
    DistroInfo distro;
    int line;
    const char *hostsByFile;
    const char *hostsByDns;
    int moduleIndex;

    if(testPrefix == NULL)
        testPrefix = "";

    memset(&distro, 0, sizeof(distro));
    memset(&conf, 0, sizeof(conf));

    GCE(ceError = DJGetDistroInfo(testPrefix, &distro));

    ceError = ReadNsswitchConf(&conf, testPrefix, TRUE);
    GCE(ceError);

    hostsByFile = GetNameOfHostsByFile(&conf, &distro);
    hostsByDns = GetNameOfHostsByDns(&conf, &distro);

    line = FindEntry(&conf, 0, "hosts");
    if(line == -1)
    {
        DJ_LOG_INFO("Adding hosts line");
        GCE(ceError = AddEntry(&conf, &distro, &line, "hosts"));
        GCE(ceError = InsertModule(&conf, &distro, line, 0, hostsByDns));
        GCE(ceError = InsertModule(&conf, &distro, line, 0, hostsByFile));
    }
    moduleIndex = FindModuleOnLine(&conf, line, hostsByFile);
    if(moduleIndex > 0)
    {
        /* The local module exists on the line, but it is not the first
         * entry. */
        GCE(ceError = RemoveModule(&conf, line, moduleIndex));
    }
    if(moduleIndex != 0)
    {
        GCE(ceError = InsertModule(&conf, &distro, line, 0, hostsByFile));
    }

    if(conf.modified)
        WriteNsswitchConfiguration(testPrefix, &conf);
    else
        DJ_LOG_INFO("nsswitch not modified");

cleanup:
    FreeNsswitchConfContents(&conf);
    DJFreeDistroInfo(&distro);

    return ceError;
}

CENTERROR
ConfigureNameServiceSwitch()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DistroInfo distro;
    GCE(ceError = DJGetDistroInfo(NULL, &distro));
    GCE(ceError = DJConfigureNameServiceSwitch(NULL, TRUE));

    /* By default, AIX will check a bind server before it will check its
     * hosts file after checking DNS. This means that our FQDN setting code
     * won't necessarily take effect. So instead we rewrite the line to put
     * local in front.
     *
     * TODO: this code should be moved to where the FQDN is set so that it
     * is not run when the --nohosts option is given.
     */
    if(distro.os == OS_AIX)
    {
        GCE(ceError = DJConfigureHostsEntry(NULL));
    }

cleanup:
    DJFreeDistroInfo(&distro);
    return ceError;
}

#define APPARMOR_NSSWITCH "/etc/apparmor.d/abstractions/nameservice"

static CENTERROR
HasApparmor(BOOLEAN *hasApparmor)
{
    return CTCheckFileOrLinkExists(APPARMOR_NSSWITCH,
                hasApparmor);
}

static CENTERROR
IsApparmorConfigured(BOOLEAN *configured)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN hasApparmor;

    *configured = FALSE;

    GCE(ceError = HasApparmor(&hasApparmor));
    if(hasApparmor)
    {
        GCE(ceError = CTCheckFileHoldsPattern(APPARMOR_NSSWITCH,
                    "centeris", configured));
    }
    else
        *configured = TRUE;

cleanup:
    return ceError;
}

static void ConfigureApparmor(BOOLEAN enable, LWException **exc)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BOOLEAN hasApparmor;
    BOOLEAN configured;
    BOOLEAN usingMr;
    FILE *file = NULL;
    PCSTR addString;
    PSTR restartPath = NULL;
    PSTR restartCommand = NULL;

    LW_CLEANUP_CTERR(exc, IsApparmorConfigured(&configured));
    if(configured == enable)
        goto cleanup;

    LW_CLEANUP_CTERR(exc, CTCheckFileOrLinkExists(APPARMOR_NSSWITCH,
                &hasApparmor));
    if(!hasApparmor)
        goto cleanup;

    LW_CLEANUP_CTERR(exc, CTCheckFileHoldsPattern(APPARMOR_NSSWITCH,
                "mr,", &usingMr));

    if(usingMr)
        addString = 
"/usr/centeris/lib/*.so*     mr,\n"
"/usr/centeris/lib64/*.so*   mr,\n"
"/tmp/.lwidentity/pipe       rw,\n";
    else
        addString =
"/usr/centeris/lib/*.so*     r,\n"
"/usr/centeris/lib64/*.so*   r,\n"
"/tmp/.lwidentity/pipe       rw,\n";


    if(enable)
    {
        LW_CLEANUP_CTERR(exc, CTCopyFileWithOriginalPerms(APPARMOR_NSSWITCH, APPARMOR_NSSWITCH ".new"));
        LW_CLEANUP_CTERR(exc, CTOpenFile(APPARMOR_NSSWITCH ".new", "a", &file));
        LW_CLEANUP_CTERR(exc, CTFilePrintf(file, "# centeris\n%s# end centeris\n",
                    addString));

        CTSafeCloseFile(&file);

        LW_CLEANUP_CTERR(exc, CTSafeReplaceFile(APPARMOR_NSSWITCH, APPARMOR_NSSWITCH ".new"));
    }
    else
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(APPARMOR_NSSWITCH, APPARMOR_NSSWITCH, FALSE, "/^[ \t]*#[ \t]*centeris[ \t]*$/,/^[ \t]*#[ \t]*end centeris[ \t]*$/d"));


    ceError = CTFindFileInPath("rcapparmor", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", &restartPath);
    if(ceError == CENTERROR_FILE_NOT_FOUND)
    {
        ceError = CTFindFileInPath("apparmor", "/etc/init.d/apparmor", &restartPath);
    }
    
    if(ceError == CENTERROR_FILE_NOT_FOUND)
    {
        ceError = CENTERROR_SUCCESS;
    }
    else if(CENTERROR_IS_OK(ceError))
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&restartCommand,
                    "%s restart", restartPath));
        LW_TRY(exc, CTCaptureOutputToExc(restartCommand, &LW_EXC));
    }
    LW_CLEANUP_CTERR(exc, ceError);

cleanup:
    if(file != NULL)
    {
        CTCloseFile(file);
        CTRemoveFile(APPARMOR_NSSWITCH ".new");
    }
    CT_SAFE_FREE_STRING(restartPath);
    CT_SAFE_FREE_STRING(restartCommand);
}

static QueryResult QueryNsswitch(const JoinProcessOptions *options, LWException **exc)
{
    QueryResult result = FullyConfigured;
    BOOLEAN configured;
    BOOLEAN exists;
    BOOLEAN hasApparmor;
    NsswitchConf conf;
    CENTERROR ceError = CENTERROR_SUCCESS;

    memset(&conf, 0, sizeof(conf));

    ceError = ReadNsswitchConf(&conf, NULL, FALSE);
    if(ceError == CENTERROR_INVALID_FILENAME)
    {
        ceError = CENTERROR_SUCCESS;
        DJ_LOG_WARNING("Warning: Could not find nsswitch file");
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    ceError = UpdateNsswitchConf(&conf, options->joiningDomain);
    LW_CLEANUP_CTERR(exc, ceError);
    if(conf.modified)
    {
        result = NotConfigured;
        goto cleanup;
    }

    LW_CLEANUP_CTERR(exc, DJHasMethodsCfg(&exists));
    if(exists)
    {
        LW_CLEANUP_CTERR(exc, DJIsMethodsCfgConfigured(&configured));
        if((configured != FALSE) != (options->joiningDomain != FALSE))
        {
            result = NotConfigured;
            goto cleanup;
        }
    }

    LW_CLEANUP_CTERR(exc, IsApparmorConfigured(&configured));
    LW_CLEANUP_CTERR(exc, HasApparmor(&hasApparmor));
    if(options->joiningDomain)
    {
        if(!configured)
        {
            result = NotConfigured;
            goto cleanup;
        }
    }
    else
    {
        if(hasApparmor && configured)
        {
            result = SufficientlyConfigured;
            goto cleanup;
        }
    }

cleanup:
    FreeNsswitchConfContents(&conf);
    return result;
}

static void DoNsswitch(JoinProcessOptions *options, LWException **exc)
{
    LWException *restartException = NULL;

    LW_TRY(exc, ConfigureApparmor(options->joiningDomain, &LW_EXC));

    if(options->joiningDomain)
        LW_CLEANUP_CTERR(exc, DJFixMethodsConfigFile());
    else
        LW_CLEANUP_CTERR(exc, DJUnconfigMethodsConfigFile());

    LW_CLEANUP_CTERR(exc, DJConfigureNameServiceSwitch(NULL, options->joiningDomain));

    CTCaptureOutputToExc( SCRIPTDIR "/ConfigureLogin nsswitch_restart",
            &restartException);
    if(restartException != NULL && restartException->code == CENTERROR_COMMAND_FAILED)
    {
        options->warningCallback(options, "Unable to restart services after nsswitch modification", restartException->longMsg);
        LW_HANDLE(&restartException);
    }
    LW_CLEANUP(exc, restartException);

cleanup:
    ;
}

static PSTR GetNsswitchDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR ret = NULL;
    PCSTR configureSteps;

    if(options->joiningDomain)
        configureSteps = 
"The following steps are required:\n"
"\t* Edit nsswitch apparmor profile to allow libraries in the /usr/centeris/lib  and /usr/centeris/lib64 directories\n"
"\t* List lwidentity module in /usr/lib/security/methods.cfg (AIX only)\n"
"\t* Add lwidentity to passwd and group/groups line /etc/nsswitch.conf or /etc/netsvc.conf\n";
    else
        configureSteps = 
"The following steps are required:\n"
"\t* Remove lwidentity module from /usr/lib/security/methods.cfg (AIX only)\n"
"\t* Remove lwidentity from passwd and group/groups line /etc/nsswitch.conf or /etc/netsvc.conf\n"
"The following step is optional:\n"
"\t* Remove apparmor exception for centeris nsswitch libraries\n";

    LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(&ret,
"%sIf any changes are performed, then the following services must be restarted:\n"
"\t* GDM\n"
"\t* XDM\n"
"\t* Cron\n"
"\t* Dbus\n"
"\t* Nscd", configureSteps));

cleanup:
    return ret;
}

const JoinModule DJNsswitchModule = { TRUE, "nsswitch", "enable/disable Likewise nsswitch module", QueryNsswitch, DoNsswitch, GetNsswitchDescription };
