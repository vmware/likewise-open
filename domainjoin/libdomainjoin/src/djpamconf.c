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

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include "domainjoin.h"
#include "ctarray.h"
#include "ctstrutils.h"
#include "ctfileutils.h"
#include "djstr.h"
#include "djdistroinfo.h"
#include "djpamconf.h"

#define GCE(x) GOTO_CLEANUP_ON_CENTERROR((x))

struct PamLine
{
    char *fromFile;
    char *leadingWhiteSpace;
    CTParseToken *service;
    CTParseToken *phase; /*auth, account, etc*/
    CTParseToken *control; /*required, sufficient, [success=1 default=die] etc*/
    CTParseToken *module; /*pam_unix2.so, /usr/lib/security/pam_aix, etc.*/
    int optionCount;
    CTParseToken *options;
    char *comment;
};

struct PamConf
{
    struct PamLine *lines;
    int lineCount;
    DynamicArray private_data;
    int modified;
};

/* return the next line in the pam configuration, or -1 if there are no more lines. */
static int NextLine(const struct PamConf *conf, int line);

/* return the next line in the pam configuration that has the given service
 * name and phase name. So you could search for the next su auth line. NULL
 * may be passed for service or phase to match any value. If there are no more
 * matching lines, -1 is returned.
 *
 * -1 may be passed for the line to indicate that the first line with that
 * service should be found */
static int NextLineForService(const struct PamConf *conf, int line, const char *service, const char *phase);

/* Find a line with a different service. This can be used to iterate through
 * all services in the file. Doing this may repeat service names though.
 */
static int NextService(struct PamConf *conf, int line);

/* Returns an array of all the service names in the file. The array is NULL
 * terminated, and the size can optionally be returned through serviceCount.
 */
static CENTERROR ListServices(struct PamConf *conf, char ***result, int *serviceCount);

static void FreeServicesList(char **list);

/* Copy all lines that contain a given service name. */
static CENTERROR CopyService(struct PamConf *conf, const char *oldName, const char *newName);

#if 0
/* Parse and add a service to the list. */
static CENTERROR AddFormattedService(struct PamConf *conf, const char *filename, const char *contents);
#endif

/* Copy a pam configuration line and add it below the old line. */
static CENTERROR CopyLine(struct PamConf *conf, int oldLine, int *newLine);

static CENTERROR RemoveLine(struct PamConf *conf, int *line);

static CENTERROR AddOption(struct PamConf *conf, int line, const char *option);

static int ContainsOption(struct PamConf *conf, int line, const char *option);

static CENTERROR RemoveOption(struct PamConf *conf, int line, const char *option, int *pFound);

/* On a real system, set the rootPrefix to "". When testing, set it to the
 * test directory that mirrors the target file system.
 */
static CENTERROR ReadPamConfiguration(const char *rootPrefix, struct PamConf *conf);

static CENTERROR WritePamConfiguration(const char *rootPrefix, struct PamConf *conf, PSTR *diff);

static CENTERROR FindModulePath(const char *testPrefix, const char *basename, char **destName);

static BOOLEAN IsRequiredService(PCSTR service, const struct PamConf *conf);

static int NextLine(const struct PamConf *conf, int line)
{
    line++;
    if(line >= conf->lineCount)
        line = -1;
    return line;
}

static int NextLineForService(const struct PamConf *conf, int line, const char *service, const char *phase)
{
    /* Start the search on the next line */
    line++;
    if(line >= conf->lineCount)
        return -1;

    for(; line != -1; line = NextLine(conf, line))
    {
        struct PamLine *lineObj = &conf->lines[line];
        if(service != NULL && (lineObj->service == NULL ||
                    strcmp(lineObj->service->value, service)))
            continue;
        if(lineObj->phase != NULL &&
                    !strcmp(lineObj->phase->value, "@include"))
            return line;
        if(phase != NULL && (lineObj->phase == NULL ||
                    strcmp(lineObj->phase->value, phase)))
            continue;
        return line;
    }
    return -1;
}

static int NextService(struct PamConf *conf, int line)
{
    const char *currentService;
    if(line == -1)
       currentService = NULL;
    else
       currentService = conf->lines[line].service->value;
    /* Start the search on the next line */
    for(line = NextLine(conf, line); line != -1; line = NextLine(conf, line))
    {
        if(conf->lines[line].service == NULL)
            continue;
        if(currentService == NULL || strcmp(conf->lines[line].service->value, currentService))
            return line;
    }
    return -1;
}

static CENTERROR ListServices(struct PamConf *conf, char ***result, int *serviceCount)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line;
    DynamicArray services;
    char *service;
    int i;
    memset(&services, 0, sizeof(services));

    for(line = NextService(conf, -1); line != -1; line = NextService(conf, line))
    {
        service = conf->lines[line].service->value;
        for(i = 0; i < services.size; i++)
        {
            if(!strcmp(((char **)services.data)[i], service))
                break;
        }
        if(i == services.size)
        {
            BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(service, &service));
            BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&services, sizeof(char *), &service, 1));
        }
    }

    if(serviceCount != NULL)
        *serviceCount = services.size;
    service = NULL;
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&services, sizeof(char *), &service, 1));
    *result = (char **)services.data;

error:
    return ceError;
}

static void FreeServicesList(char **list)
{
    CTFreeNullTerminatedStringArray(list);
}

static void UpdatePublicLines(struct PamConf *conf)
{
    conf->lines = conf->private_data.data;
    conf->lineCount = conf->private_data.size;
}

static const char *Basename(const char *path)
{
    const char *lastslash = strrchr(path, '/');
    if(lastslash != NULL)
        return lastslash + 1;
    return path;
}

//This function is only used on AIX right now
static CENTERROR CopyService(struct PamConf *conf, const char *oldName, const char *newName)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line = -1;
    int newLine;
    struct PamLine *oldLineObj, *newLineObj;
    for(;;)
    {
        line = NextLineForService(conf, line, oldName, NULL);
        if(line == -1)
            break;

        BAIL_ON_CENTERIS_ERROR(ceError = CopyLine(conf, line, &newLine));
        oldLineObj = &conf->lines[line];
        newLineObj = &conf->lines[newLine];
        /* Update the service name */
        CT_SAFE_FREE_STRING(newLineObj->service->value);
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(newName, &newLineObj->service->value));
        if(!strcmp(Basename(newLineObj->fromFile), oldName))
        {
            /* The file name is something like /etc/pam.d/<oldName>. It should
             * be updated. */
            char *newPath;

            /* This trims off the service name from the path */
            *strrchr(newLineObj->fromFile, '/') = '\0';
            BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&newPath, "%s/%s", newLineObj->fromFile, newName));
            CTFreeString(newLineObj->fromFile);
            newLineObj->fromFile = newPath;
        }
    }
    UpdatePublicLines(conf);
    conf->modified = 1;

error:
    return ceError;
}

static void FreePamLineContents(struct PamLine *line)
{
    int i;
    CT_SAFE_FREE_STRING(line->fromFile);
    CT_SAFE_FREE_STRING(line->leadingWhiteSpace);
    CTFreeParseToken(&line->service);
    CTFreeParseToken(&line->phase);
    CTFreeParseToken(&line->control);
    CTFreeParseToken(&line->module);

    for(i = 0; i < line->optionCount; i++)
    {
        CTFreeParseTokenContents(&line->options[i]);
    }
    CT_SAFE_FREE_MEMORY(line->options);
    CT_SAFE_FREE_STRING(line->comment);
    line->optionCount = 0;
}

static CENTERROR ParsePamLine(struct PamLine *lineObj, const char *filename, const char *linestr, const char **endptr)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    const char *pos = linestr;
    const char *token_start = NULL;
    DynamicArray tokens;

    memset(&tokens, 0, sizeof(tokens));
    memset(lineObj, 0, sizeof(*lineObj));
    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateString(filename, &lineObj->fromFile));

    /* Find the leading whitespace in the line */
    token_start = pos;
    while(isblank(*pos)) pos++;
    BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &lineObj->leadingWhiteSpace));

    /* Create an array of white space separated tokens */
    /* Due to what pos points to when this gets calls, !isspace(*pos) actually
     * makes sure that pos is not pointing to '\n' or '\r'. pos will not be
     * pointing to a space or tab anyway.
     */
    while(!isspace(*pos) && *pos != '\0' && *pos != '#')
    {
        CTParseToken token;
        token_start = pos;
        while(!isspace(*pos) && *pos != '\0' && *pos != '#')
        {
            if(*pos == '[')
            {
                /* This could be a token like [success=1 default=die] . So
                 * this token continues until it finds the matching ], or end
                 * of line.
                 */
                do
                {
                    pos++;
                } while(*pos != '\0' && *pos != '#' && *pos != ']');
                if(*pos == ']')
                    pos++;
            }
            else
                pos++;
        }
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &token.value));

        token_start = pos;
        while(isblank(*pos)) pos++;
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &token.trailingSeparator));
        BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&tokens, sizeof(CTParseToken), &token, 1));
    }

    if(!strcmp(Basename(filename), "pam.conf"))
    {
        /* this did not come from a pam.d system */
        if(tokens.size > 0)
        {
            BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) &lineObj->service));
            CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->service, 1);
        }
    }
    else
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) &lineObj->service));
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateString(Basename(filename), &lineObj->service->value));
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateString(" ", &lineObj->service->trailingSeparator));
    }

    if(tokens.size > 0)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) &lineObj->phase));
        CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->phase, 1);
    }

    if(tokens.size > 0)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) &lineObj->control));
        CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->control, 1);
    }

    if(tokens.size > 0)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) &lineObj->module));
        CTArrayRemoveHead(&tokens, sizeof(CTParseToken), lineObj->module, 1);
    }

    lineObj->optionCount = tokens.size;
    lineObj->options = tokens.data;
    memset(&tokens, 0, sizeof(DynamicArray));

    token_start = pos;
    while(*pos != '\0' && *pos != '\n' && *pos != '\r') pos++;

    if(pos != token_start)
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(token_start, pos - token_start, &lineObj->comment));
    else
        lineObj->comment = NULL;

    if(endptr != NULL)
        *endptr = pos;
    return ceError;

error:
    FreePamLineContents(lineObj);
    if(tokens.data != NULL)
    {
        int i;
        for(i = 0; i < tokens.size; i++)
        {
            CTFreeParseTokenContents(&((CTParseToken *)tokens.data)[i]);
        }
        CTArrayFree(&tokens);
    }
    return ceError;
}

static CENTERROR AddFormattedLine(struct PamConf *conf, const char *filename, const char *linestr, const char **endptr)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct PamLine lineObj;

    BAIL_ON_CENTERIS_ERROR(ceError = ParsePamLine(&lineObj, filename, linestr, endptr));

    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&conf->private_data, sizeof(struct PamLine), &lineObj, 1));
    memset(&lineObj, 0, sizeof(struct PamLine));
    UpdatePublicLines(conf);
    conf->modified = 1;

error:
    return ceError;
}

//This function is currently unused
#if 0
/* Parse and add a service to the list. */
static CENTERROR AddFormattedService(struct PamConf *conf, const char *filename, const char *contents)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    const char *pos = contents;
    while(*pos != '\0')
    {
        BAIL_ON_CENTERIS_ERROR(ceError = AddFormattedLine(conf, filename, pos, &pos));
        if(*pos == '\r')
            pos++;
        if(*pos == '\n')
            pos++;
    }

error:
    return ceError;
}
#endif

/* Copy a pam configuration line and add it below the old line. */
static CENTERROR CopyLine(struct PamConf *conf, int oldLine, int *newLine)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct PamLine lineObj;
    struct PamLine *oldObj = &conf->lines[oldLine];
    int i;
    memset(&lineObj, 0, sizeof(struct PamLine));

    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->fromFile, &lineObj.fromFile));
    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->leadingWhiteSpace, &lineObj.leadingWhiteSpace));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->service, &lineObj.service));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->phase, &lineObj.phase));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->control, &lineObj.control));
    BAIL_ON_CENTERIS_ERROR(ceError = CTCopyToken(oldObj->module, &lineObj.module));
    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken)*oldObj->optionCount, (void **) &lineObj.options));
    lineObj.optionCount = oldObj->optionCount;

    for(i = 0; i < oldObj->optionCount; i++)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->options[i].value, &lineObj.options[i].value));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->options[i].trailingSeparator, &lineObj.options[i].trailingSeparator));
    }

    if(oldObj->comment != NULL)
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(oldObj->comment, &lineObj.comment));
    else
        lineObj.comment = NULL;

    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayInsert(&conf->private_data, oldLine + 1, sizeof(struct PamLine), &lineObj, 1));
    memset(&lineObj, 0, sizeof(lineObj));
    if(newLine != NULL)
        *newLine = oldLine + 1;
    UpdatePublicLines(conf);
    conf->modified = 1;

error:
    FreePamLineContents(&lineObj);
    return ceError;
}

static CENTERROR RemoveLine(struct PamConf *conf, int *line)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayRemove(&conf->private_data, *line, sizeof(struct PamLine),
            1));
    UpdatePublicLines(conf);
    conf->modified = 1;

    if(*line >= conf->lineCount)
        *line = -1;

error:
    return ceError;
}

static int ContainsOption(struct PamConf *conf, int line, const char *option)
{
    struct PamLine *lineObj = &conf->lines[line];
    int i;
    int found = 0;

    for(i = 0; i < lineObj->optionCount; i++)
    {
        if(!strcmp(lineObj->options[i].value, option))
            found++;
    }
    return found;
}

static CENTERROR AddOption(struct PamConf *conf, int line, const char *option)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct PamLine *lineObj = &conf->lines[line];
    CTParseToken newOption;
    CTParseToken *prevToken;
    CTParseToken *prevprevToken;
    /* Do not free the options variable */
    DynamicArray options;
    memset(&newOption, 0, sizeof(CTParseToken));

    if(ContainsOption(conf, line, option))
        return ceError;

    options.data = lineObj->options;
    options.size = lineObj->optionCount;
    options.capacity = lineObj->optionCount;

    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(option, &newOption.value));
    if(options.size >= 1)
    {
        prevprevToken = lineObj->module;
        prevToken = &lineObj->options[options.size - 1];
    }
    else
    {
        prevprevToken = lineObj->control;
        prevToken = lineObj->module;
    }

    /* Copy the white space from the previous token. Hopefully this will
     * make the spacing look consistent.
     */
    newOption.trailingSeparator = prevToken->trailingSeparator;
    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(prevprevToken->trailingSeparator, &prevToken->trailingSeparator));

    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&options, sizeof(CTParseToken), &newOption, 1));
    memset(&newOption, 0, sizeof(CTParseToken));

    lineObj->options = options.data;
    lineObj->optionCount = options.size;
    conf->modified = 1;

error:
    /* Do not free the options variable */
    CTFreeParseTokenContents(&newOption);
    return ceError;
}

static CENTERROR RemoveOption(struct PamConf *conf, int line, const char *option, int *pFound)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct PamLine *lineObj = &conf->lines[line];
    /* Do not free the options variable */
    DynamicArray options;
    int i;
    int found = 0;
    options.data = lineObj->options;
    options.size = lineObj->optionCount;
    options.capacity = lineObj->optionCount;

    for(i = 0; i < options.size;)
    {
        if(!strcmp(((CTParseToken *)options.data)[i].value, option))
        {
            if(i == options.size - 1)
            {
                /* If this is the last option in the list, then move it's trailing white space value back one */
                if(i >= 1)
                {
                    CT_SAFE_FREE_STRING(((CTParseToken *)options.data)[i - 1].trailingSeparator);
                    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(
                        ((CTParseToken *)options.data)[i].trailingSeparator,
                        &((CTParseToken *)options.data)[i - 1].trailingSeparator));
                }
                else if(lineObj->module != NULL)
                {
                    CT_SAFE_FREE_STRING(lineObj->module->trailingSeparator);
                    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(
                        ((CTParseToken *)options.data)[i].trailingSeparator,
                        &lineObj->module->trailingSeparator));
                }
            }
            BAIL_ON_CENTERIS_ERROR(ceError = CTArrayRemove(&options, i, sizeof(CTParseToken), 1));
            found++;
        }
        else
            i++;
    }
    lineObj->options = options.data;
    lineObj->optionCount = options.size;
    if(found != 0)
    {
        conf->modified = 1;
    }
    if(pFound != NULL)
        *pFound = found;

error:
    /* Do not free the options variable */
    return ceError;
}

static CENTERROR ReadPamFile(struct PamConf *conf, const char *rootPrefix, const char *filename)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct stat statbuf;
    FILE *file = NULL;
    PSTR buffer = NULL;
    char *fullPath = NULL;
    BOOLEAN endOfFile = FALSE;

    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(
            &fullPath, "%s%s", rootPrefix, filename));
    DJ_LOG_INFO("Reading pam file %s", fullPath);
    if(stat(fullPath, &statbuf) < 0)
    {
        DJ_LOG_INFO("File %s does not exist", fullPath);
        ceError = CENTERROR_INVALID_FILENAME;
        goto error;
    }
    if(!S_ISREG(statbuf.st_mode))
    {
        DJ_LOG_INFO("File %s is not a regular file", fullPath);
        ceError = CENTERROR_INVALID_FILENAME;
        goto error;
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CTOpenFile(fullPath, "r", &file));
    while(TRUE)
    {
        CT_SAFE_FREE_STRING(buffer);
        BAIL_ON_CENTERIS_ERROR(ceError = CTReadNextLine(file, &buffer, &endOfFile));
        if(endOfFile)
            break;
        BAIL_ON_CENTERIS_ERROR(ceError = AddFormattedLine(conf, filename, buffer, NULL));
    }
    BAIL_ON_CENTERIS_ERROR(ceError = CTCloseFile(file));
    file = NULL;

error:
    if(file != NULL)
        CTCloseFile(file);
    CT_SAFE_FREE_STRING(fullPath);
    CT_SAFE_FREE_STRING(buffer);
    return ceError;
}

/* On a real system, set the rootPrefix to "". When testing, set it to the
 * test directory that mirrors the target file system.
 */
static CENTERROR ReadPamConfiguration(const char *rootPrefix, struct PamConf *conf)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DIR *dp = NULL;
    struct dirent *dirp;
    char *fullPath = NULL;
    BOOLEAN foundPamd = FALSE;

    memset(conf, 0, sizeof(struct PamConf));

    DJ_LOG_INFO("Reading pam configuration");

    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&fullPath, "%s/etc/pam.d", rootPrefix));
    dp = opendir(fullPath);
    CT_SAFE_FREE_STRING(fullPath);
    if(dp != NULL)
    {
        while((dirp = readdir(dp)) != NULL)
        {
            if(dirp->d_name[0] != '.' && /*Ignore hidden files*/
                    !CTStrEndsWith(dirp->d_name, ".bak") &&
                    !CTStrEndsWith(dirp->d_name, ".new") &&
                    !CTStrEndsWith(dirp->d_name, ".orig"))
            {
                BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&fullPath, "/etc/pam.d/%s", dirp->d_name));
                BAIL_ON_CENTERIS_ERROR(ceError = ReadPamFile(conf, rootPrefix, fullPath));
                CT_SAFE_FREE_STRING(fullPath);
                foundPamd = TRUE;
            }
        }
    }

    ceError = ReadPamFile(conf, rootPrefix, "/etc/pam.conf");
    if(ceError == CENTERROR_INVALID_FILENAME && foundPamd)
        ceError = CENTERROR_SUCCESS;
    BAIL_ON_CENTERIS_ERROR(ceError);

    conf->modified = 0;

error:
    CT_SAFE_FREE_STRING(fullPath);
    if(dp != NULL)
        closedir(dp);
    return ceError;
}

struct OpenFile
{
    const char *name;
    FILE *file;
};

static CENTERROR FindFile(const char *rootPrefix, const char *filename, DynamicArray *openFiles, int *lastIndexed, FILE **file)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct OpenFile *files = (struct OpenFile *)openFiles->data;
    int i;
    struct OpenFile newFile;
    char *fullPath = NULL;
    memset(&newFile, 0, sizeof(newFile));

    if(*lastIndexed < openFiles->size &&
            !strcmp(filename, files[*lastIndexed].name))
    {
        *file = files[*lastIndexed].file;
        return ceError;
    }

    for(i = 0; i < openFiles->size; i++)
    {
        if(!strcmp(filename, files[i].name))
        {
            *lastIndexed = i;
            *file = files[*lastIndexed].file;
            return ceError;
        }
    }

    /* The file hasn't been opened yet. */
    newFile.name = filename;
    BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&fullPath, "%s%s.new", rootPrefix, filename));
    ceError = CTOpenFile(fullPath, "w", &newFile.file);
    if(!CENTERROR_IS_OK(ceError))
    {
        DJ_LOG_ERROR("Unable to open '%s' for writing", fullPath);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(openFiles, sizeof(struct OpenFile), &newFile, 1));
    files = (struct OpenFile *)openFiles->data;
    memset(&newFile, 0, sizeof(newFile));

    *lastIndexed = openFiles->size - 1;
    *file = files[*lastIndexed].file;

error:
    if(newFile.file != NULL)
        CTCloseFile(newFile.file);
    CT_SAFE_FREE_STRING(fullPath);
    return ceError;
}

static CENTERROR MoveFiles(const char *rootPrefix, DynamicArray *openFiles, PSTR *diff)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int i;
    char *tempName = NULL;
    char *finalName = NULL;
    PSTR oldDiff = NULL;
    PSTR fileDiff = NULL;
    BOOLEAN same;
    struct OpenFile *files = (struct OpenFile *)openFiles->data;
    for(i = 0; i < openFiles->size; i++)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&tempName, "%s%s.new", rootPrefix, files[i].name));
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&finalName, "%s%s", rootPrefix, files[i].name));

        BAIL_ON_CENTERIS_ERROR(ceError = CTFileContentsSame(tempName, finalName, &same));
        if(same)
        {
            DJ_LOG_INFO("Pam file %s unmodified", finalName);
            BAIL_ON_CENTERIS_ERROR(ceError = CTRemoveFile(tempName));
        }
        else
        {
            DJ_LOG_INFO("Pam file %s modified", finalName);
            if(diff != NULL)
            {
                CT_SAFE_FREE_STRING(oldDiff);
                CT_SAFE_FREE_STRING(fileDiff);
                oldDiff = *diff;
                if(!oldDiff)
                    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("", &oldDiff));
                else
                    *diff = NULL;
                BAIL_ON_CENTERIS_ERROR(ceError = CTGetFileDiff(finalName, tempName, &fileDiff, FALSE));
                BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(diff, "%sChanges to %s:\n%s", oldDiff, files[i].name, fileDiff));
            }
            BAIL_ON_CENTERIS_ERROR(ceError = CTCloneFilePerms(finalName, tempName));
            BAIL_ON_CENTERIS_ERROR(ceError = CTBackupFile(finalName));
            BAIL_ON_CENTERIS_ERROR(ceError = CTMoveFile(tempName, finalName));
        }
        CT_SAFE_FREE_STRING(tempName);
        CT_SAFE_FREE_STRING(finalName);
    }

error:
    CT_SAFE_FREE_STRING(tempName);
    CT_SAFE_FREE_STRING(finalName);
    CT_SAFE_FREE_STRING(oldDiff);
    CT_SAFE_FREE_STRING(fileDiff);
    return ceError;
}

static CENTERROR CloseFiles(const char *rootPrefix, DynamicArray *openFiles)
{
    CENTERROR firstError = CENTERROR_SUCCESS, lastError;
    int i;
    struct OpenFile *files = (struct OpenFile *)openFiles->data;
    for(i = 0; i < openFiles->size; i++)
    {
        /* Close all of the files, but return the error from the first one that
         * failed. */
        if(files[i].file != NULL)
        {
            lastError = CTCloseFile(files[i].file);
            if(CENTERROR_IS_OK(firstError))
                firstError = lastError;
            files[i].file = NULL;
        }
    }

    return firstError;
}

static CENTERROR AppendToken(StringBuffer *save, CTParseToken *token)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    if(token != NULL)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppend(save, token->value));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppend(save, token->trailingSeparator));
    }

error:
    return ceError;
}

//The string buffer must have already been constructed
static CENTERROR AppendPamLine(StringBuffer *save, struct PamLine *lineObj)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int j;

    GCE(ceError = CTStringBufferAppend(save, lineObj->leadingWhiteSpace));
    if(lineObj->service != NULL && strcmp(Basename(lineObj->fromFile), lineObj->service->value))
    {
        /* This entry is from a pam.conf file */
        GCE(ceError = AppendToken(save, lineObj->service));
    }
    GCE(ceError = AppendToken(save, lineObj->phase));
    GCE(ceError = AppendToken(save, lineObj->control));
    GCE(ceError = AppendToken(save, lineObj->module));
    for(j = 0; j < lineObj->optionCount; j++)
    {
        GCE(ceError = AppendToken(save, &lineObj->options[j]));
    }
    if(lineObj->comment != NULL)
        GCE(ceError = CTStringBufferAppend(save, lineObj->comment));

cleanup:
    return ceError;
}

static CENTERROR WritePamConfiguration(const char *rootPrefix, struct PamConf *conf, PSTR *diff)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DynamicArray openFiles;
    int lastIndexedFile;
    int i;
    StringBuffer buffer;
    memset(&openFiles, 0, sizeof(openFiles));
    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferConstruct(&buffer));

    DJ_LOG_INFO("Writing pam configuration");

    for(i = 0; i < conf->lineCount; i++)
    {
        struct PamLine *lineObj = &conf->lines[i];
        FILE *file = NULL;
        BAIL_ON_CENTERIS_ERROR(ceError = FindFile(rootPrefix, lineObj->fromFile, &openFiles, &lastIndexedFile, &file));
        CTStringBufferClear(&buffer);
        BAIL_ON_CENTERIS_ERROR(ceError = AppendPamLine(&buffer, lineObj));
        BAIL_ON_CENTERIS_ERROR(ceError = CTFilePrintf(file, "%s\n", buffer.data));
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CloseFiles(rootPrefix, &openFiles));
    BAIL_ON_CENTERIS_ERROR(ceError = MoveFiles(rootPrefix, &openFiles, diff));

error:
    CloseFiles(rootPrefix, &openFiles);
    CTArrayFree(&openFiles);
    CTStringBufferDestroy(&buffer);
    return ceError;
}

static void FreePamConfContents(struct PamConf *conf)
{
    int i;
    for(i = 0; i < conf->lineCount; i++)
    {
        FreePamLineContents(&conf->lines[i]);
    }
    CTArrayFree(&conf->private_data);
    UpdatePublicLines(conf);
}

/* Strips known module paths off from the beginning of the module name, and strips off known extensions from the end of the module name.
 *
 * The result is stored in destName. destName is a buffer of size 'bufferSize'.
 *
 * If destName is not large enough, as much of srcName is copied as possible,
 * and false is returned.
 *
 * If bufferSize >= 1, then destName will always be null terminated.
 */
static BOOLEAN NormalizeModuleName( char *destName, const char *srcName, size_t bufferSize)
{
    size_t trimEnd = 0;
    size_t copyLen;
    if(bufferSize < 1)
        return FALSE;
    if(CTStrStartsWith(srcName, "/usr/lib/security/"))
        srcName += strlen("/usr/lib/security/");
    else if(CTStrStartsWith(srcName, "/lib64/security/"))
        srcName += strlen("/lib64/security/");
    else if(CTStrStartsWith(srcName, "/usr/lib64/security/"))
        srcName += strlen("/usr/lib64/security/");
    else if(CTStrStartsWith(srcName, "/lib/security/hpux32/"))
        srcName += strlen("/lib/security/hpux32/");
    else if(CTStrStartsWith(srcName, "/lib/security/hpux64/"))
        srcName += strlen("/lib/security/hpux64/");
    else if(CTStrStartsWith(srcName, "/lib/security/$ISA/"))
        srcName += strlen("/lib/security/$ISA/");
    else if(CTStrStartsWith(srcName, "/lib/security/"))
        srcName += strlen("/lib/security/");
    else if(CTStrStartsWith(srcName, "/usr/lib/vmware/lib/libpam.so.0/security/"))
        srcName += strlen("/usr/lib/vmware/lib/libpam.so.0/security/");
    else if(CTStrStartsWith(srcName, "/usr/lib/pam/"))
        srcName += strlen("/usr/lib/pam/");
    else if(CTStrStartsWith(srcName, "/usr/lib/security/sparcv9/"))
        srcName += strlen("/usr/lib/security/sparcv9/");
    else if(CTStrStartsWith(srcName, "/usr/lib/security/amd64/"))
        srcName += strlen("/usr/lib/security/amd64/");

    /* Remove any "lib" prefix so that libpam_foo == pam_foo.  HP-UX has a bunch
     * of libpam_foo.  If we need to distinguish libpam_foo vs pam_foo in the future,
     * we will need to add back libpam_X rules in various places. */
    if(CTStrStartsWith(srcName, "lib"))
        srcName += strlen("lib");

    if(CTStrEndsWith(srcName, ".sl"))
        trimEnd = strlen(".sl");
    else if(CTStrEndsWith(srcName, ".sl.1"))
        trimEnd = strlen(".sl.1");
    else if(CTStrEndsWith(srcName, ".so"))
        trimEnd = strlen(".so");
    else if(CTStrEndsWith(srcName, ".so.1"))
        trimEnd = strlen(".so.1");
    else if(CTStrEndsWith(srcName, ".1"))
        trimEnd = strlen(".1");

    copyLen = strlen(srcName) - trimEnd;
    if(copyLen > bufferSize - 1)
        copyLen = bufferSize - 1;

    strncpy(destName, srcName, copyLen);
    destName[copyLen] = 0;
    return TRUE;
}

/* returns true if the pam module should be commented out when pam_lwidentity is enabled.
 */
static BOOLEAN PamModuleGrants( const char * phase, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_deny"))
        return FALSE;
    if(!strcmp(buffer, "pam_prohibit"))
        return FALSE;
    /* We use pam_sample on Solaris 8 to act like pam_deny. pam_sample could
     * actually act like something else depending on which options are passed,
     * but it is safe to assume that it always blocks.
     */
    if(!strcmp(buffer, "pam_sample"))
        return FALSE;

    /* Assume that the module returns success for someone */
    return TRUE;
}

static BOOLEAN PamModuleIsLwidentity(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_lwidentity"))
        return TRUE;
    if(!strcmp(buffer, "libpam_lwidentity"))
        return TRUE;

    return FALSE;
}

static BOOLEAN PamModuleIsPwcheck(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_pwcheck"))
        return TRUE;

    return FALSE;
}

static BOOLEAN PamModuleIsOldCenteris(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_centeris"))
        return TRUE;

    return FALSE;
}

static BOOLEAN PamModuleIsLwiPassPolicy(const char *phase, const char *module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));
    if(!strcmp(buffer, "pam_lwipasspolicy"))
        return TRUE;

    return FALSE;
}

/* returns true if the pam module grants users access based on the caller's UID or GID, or maybe the machine the user is trying to connect from.
 *
 * Basically this returns true if the module does not prompt for a password, but can let the user login based on some kind of semi-intellegent check.
 */
static BOOLEAN PamModuleChecksCaller( const char * phase, const char * module)
{
    char buffer[256];

    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_wheel"))
        return TRUE;
    if(!strcmp(buffer, "pam_rootok"))
        return TRUE;
    if(!strcmp(buffer, "pam_allowroot"))
        return TRUE;
    if(!strcmp(buffer, "pam_rhosts_auth"))
        return TRUE;
    if(!strcmp(buffer, "pam_console"))
        return TRUE;
    if(!strcmp(buffer, "pam_timestamp"))
        return TRUE;
    if(!strcmp(buffer, "pam_krb5"))
        return TRUE;
    if(!strcmp(buffer, "pam_securid"))
        return TRUE;
    //Used by IBM Director. Found at Gap
    if(!strcmp(buffer, "pam_ve"))
        return TRUE;
    //Used by Opsware. Found at Gap
    if(!strcmp(buffer, "/opt/OPSWsshd/libexec/opsw_auth"))
        return TRUE;

    return FALSE;
}

/* returns true if the pam module prompts for a password and stores it in the pam state.
 */
static BOOLEAN PamModulePrompts( const char * phase, const char * module)
{
    char buffer[256];

    /* Modules only prompt during the auth and account phase */
    if(strcmp(phase, "auth") && strcmp(phase, "password"))
        return FALSE;

    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_unix"))
        return TRUE;
    /* On Redhat 2.1, 7.3, and 7.2 there is a pam_pwdb.so. It seems to do
       the same thing as pam_unix.so. For some reason, the distro includes
       both modules, and services use one or the other.
     */
    if(!strcmp(buffer, "pam_pwdb"))
        return TRUE;
    if(!strcmp(buffer, "pam_unix2"))
        return TRUE;
    if(!strcmp(buffer, "pam_authtok_get"))
        return TRUE;
    if(!strcmp(buffer, "pam_unix_auth"))
        return TRUE;
    /* Fix bug 5239. Upon further research it looks like pam_dhkeys does
     * check the user's password. However, the module will still return
     * PAM_IGNORE if the user isn't logging in from NIS.
     */
    if(!strcmp(buffer, "pam_dhkeys"))
        return TRUE;
    if(!strcmp(buffer, "pam_aix"))
        return TRUE;
    if(!strcmp(buffer, "pam_passwd_auth"))
        return TRUE;
    if(!strcmp(buffer, "pam_securityserver"))
        return TRUE;
    if(!strcmp(buffer, "pam_serialnumber"))
        return TRUE;
    if(!strcmp(buffer, "pam_winbind"))
        return TRUE;
    if(!strcmp(buffer, "pam_ldap"))
        return TRUE;
    //This was found on SLED10 SP1. It is used for Novell's E-Directory.
    if(!strcmp(buffer, "pam_nam"))
        return TRUE;
    //This was found on Ubuntu Gutsy. It is used for pure-ftpd
    if(!strcmp(buffer, "pam_ftp"))
        return TRUE;

    /* pam_lwidentity will only prompt for domain users during the password phase. All in all, it doesn't store passwords for subsequent modules in the password phase. */
    if(PamModuleIsLwidentity(phase, module) && !strcmp(phase, "auth"))
        return TRUE;
    if(PamModuleIsLwiPassPolicy(phase, module) && !strcmp(phase, "password"))
        return TRUE;
    if(!strcmp(phase, "password") && !strcmp(buffer, "pam_pwcheck"))
        return TRUE;
    if(!strcmp(phase, "password") && !strcmp(buffer, "pam_cracklib"))
        return TRUE;

    /* Assume that the module does not prompt */
    return FALSE;
}

static BOOLEAN PamModuleUnderstandsTryFirstPass( const char * phase, const char * module)
{
    char buffer[256];

    if(!PamModulePrompts(phase, module))
        return FALSE;

    NormalizeModuleName( buffer, module, sizeof(buffer));

    /* pam_aix treats try_first_pass as try the existing password if it is set.
     * If the existing password that was set is wrong, prompt again and put up
     * an obscure error message if the second password is correct.
     */
    if(!strcmp(buffer, "pam_aix"))
        return FALSE;
    /* libpam_unix.so.1 on HP-UX will prompt with "System Password:" if the
       existing password is incorrect. The module name will get normalized to
       pam_unix, which Linux systems also use (but it works on them). So the
       prefix of the unnormalized module name is also checked.
       */
    if(!strcmp(buffer, "pam_unix") && CTStrStartsWith(module, "lib"))
        return FALSE;

    return TRUE;
}

/* returns true if the pam module returns PAM_SUCCESS either all the time, or for users based on a password.
 */
static BOOLEAN PamModuleRemoveOnEnable( const char * phase, const char *control, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(phase, "auth") &&
            !strcmp(control, "required") &&
            !strcmp(buffer, "pam_hpsec"))
    {
        return TRUE;
    }

    return FALSE;
}

/* returns true if the pam module returns a pam error code either all the time, or for users based on a password.
 */
static BOOLEAN PamModuleDenies( const char * phase, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    if(!strcmp(buffer, "pam_authtok_get"))
        return FALSE;
    if(!strcmp(buffer, "pam_permit"))
        return FALSE;

    /* Assume that the module sometimes denies logins */
    return TRUE;
}

/* returns true if the pam module will always return an error code for a domain user (assuming that a local account by the same name doesn't exist)
 */
static BOOLEAN PamModuleAlwaysDeniesDomainLogins( const char * phase, const char * module)
{
    char buffer[256];
    NormalizeModuleName( buffer, module, sizeof(buffer));

    /* If it never denies login, it won't deny login for domain users */
    if(!PamModuleDenies(phase, module))
        return FALSE;

    /* If it always denies login, it will for domain users too */
    if(!PamModuleGrants(phase, module))
        return TRUE;

    if(PamModuleIsLwidentity(phase, module))
        return FALSE;

    /* The domain user will not be root */
    if(!strcmp(buffer, "pam_rootok"))
        return TRUE;
    if(!strcmp(buffer, "pam_allowroot"))
        return TRUE;

    /* It would be too difficult to debug a POC where securid was blocking SSH
     * for domain users. */
    if(!strcmp(buffer, "pam_securid"))
        return TRUE;

    /* These modules return errors for users they don't know about */
    if(!strcmp(buffer, "pam_unix_account"))
        return TRUE;
    if(!strcmp(buffer, "pam_authtok_store"))
        return TRUE;

    /* We don't really want to use local password checks for domain users*/
    if(!strcmp(buffer, "pam_authtok_check"))
        return TRUE;
    if(!strcmp(buffer, "pam_cracklib"))
        return TRUE;
    if(!strcmp(buffer, "pam_pwcheck"))
        return TRUE;

    /* pam_hpsec on HP-UX blocks only for auth and session.  It is a no-op for
     * account and password.  The issue is that it returns unknown user because
     * it does not use nsswitch and only looks at local accounts. */
    if((!strcmp(phase, "auth") || !strcmp(phase, "session")) && !strcmp(buffer, "pam_hpsec"))
        return TRUE;

    /* We don't really want to use our own local password checks either */
    if(PamModuleIsLwiPassPolicy(phase, module))
        return TRUE;

    /* Assume that if it prompts for a password, it will complain about a
     * domain user
     */
    return PamModulePrompts("auth", module);
}

/* returns true if the pam module at least sometimes returns success for AD users
 */
/*Currently unused
static BOOLEAN PamModuleGrantsDomainLogins( const char * phase, const char * module)
{
    if(PamModuleGrants(phase, module) && !PamModuleAlwaysDeniesDomainLogins(phase, module))
        return TRUE;

    return FALSE;
}
*/

static CENTERROR GetIncludeName(struct PamLine *lineObj, PSTR *includeService)
{
    char buffer[256] = "";
    if(lineObj->module != NULL)
        NormalizeModuleName( buffer, lineObj->module->value, sizeof(buffer));

    if(lineObj->phase != NULL && !strcmp(lineObj->phase->value, "@include"))
    {
        return CTStrdup(lineObj->control->value, includeService);
    }
    if(lineObj->control != NULL && !strcmp(lineObj->control->value, "include"))
    {
        return CTStrdup(lineObj->module->value, includeService);
    }
    if(!strcmp(buffer, "pam_stack"))
    {
        int i;
        for(i = 0; i < lineObj->optionCount; i++)
        {
            if(CTStrStartsWith(lineObj->options[i].value, "service="))
            {
                return CTStrdup(lineObj->options[i].value + strlen("service="), includeService);
            }
        }
    }
    *includeService = NULL;
    return CENTERROR_SUCCESS;
}

struct ConfigurePamModuleState
{
    /* Set to true if the requested module (lwidentity, lwipasspolicy, etc.) has either been disabled or enabled
     */
    BOOLEAN configuredRequestedModule;
    /* Set to true if a line has been passed that has "sufficient" as the control and a module that lets users in based off of a password. This will not be set to true for modules that use caller based authentication like pam_rootok.
     */
    BOOLEAN sawSufficientPasswordCheck;
    /* Set to true if a line has been passed that has a module that prompts for passwords (assuming that try_first_pass, etc. are not on the line), regardless of what the control is.
     */
    BOOLEAN sawPromptingModule;
    /* Set to true if a line has been passed with pam_pwcheck as its module.
     */
    BOOLEAN sawPwcheck;
    /* Set to true if a line has been passed that has "required" or "requisite" as its control and a module that checks an attribute of the caller without prompting for a password. pam_rhosts_auth is an example of such a module.
     */
    BOOLEAN sawCallerRequirementLine;
    /* Set to true if a line has been passed that has a module that atleast sometimes returns PAM_SUCCESS for domain users, regardless of what the control is onthat line.
     */
    BOOLEAN sawDomainUserGrantingLine;

    int includeLevel;
};

static CENTERROR PamOldCenterisDisable(struct PamConf *conf, const char *service, const char * phase, struct ConfigurePamModuleState *state)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;

    DJ_LOG_INFO("Disabling pam_centeris for pam service %s for phase %s", service, phase);

    while(line != -1)
    {
        lineObj = &conf->lines[line];
        if(lineObj->module == NULL)
            module = "";
        else
            module = lineObj->module->value;

        if(lineObj->control == NULL)
            control = "";
        else
            control = lineObj->control->value;

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        BAIL_ON_CENTERIS_ERROR(ceError = GetIncludeName(lineObj, &includeService));
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamOldCenterisDisable(conf, includeService, phase, state);
            if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                ceError = CENTERROR_SUCCESS;
            if(ceError == CENTERROR_DOMAINJOIN_PAM_BAD_CONF)
                ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsOldCenteris(phase, module))
        {
            DJ_LOG_INFO("Removing pam_centeris from service %s", service);
            BAIL_ON_CENTERIS_ERROR(ceError = RemoveLine(conf, &line));
            state->configuredRequestedModule = TRUE;
            continue;
        }
        //Don't worry about try_first_pass. Our old configuration code didn't
        //get that right anyway.

        line = NextLineForService(conf, line, service, phase);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    return ceError;
}

static CENTERROR PamLwidentityDisable(struct PamConf *conf, const char *service, const char * phase, const char *pam_lwidentity, struct ConfigurePamModuleState *state)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line;
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;
    char *fromFileBackup = NULL;
    char *commentBackup = NULL;

    DJ_LOG_INFO("Disabling pam_lwidentity for pam service %s for phase %s", service, phase);

    DJ_LOG_INFO("Looking for lines commented out during the pam enable");
    for(line = 0; line < conf->lineCount; line++)
    {
        lineObj = &conf->lines[line];
        if(CTStrStartsWith(lineObj->comment, "#Commented out by lwidentity: "))
        {
            const char *original = lineObj->comment + strlen("#Commented out by lwidentity: ");
            DJ_LOG_INFO("Found line '%s' commented out by lwidentity", original);

            //Replace the lineObj object in place with its uncommented
            //version.
            CT_SAFE_FREE_STRING(fromFileBackup);
            CT_SAFE_FREE_STRING(commentBackup);
            fromFileBackup = lineObj->fromFile;
            lineObj->fromFile = NULL;
            commentBackup = lineObj->comment;
            lineObj->comment = NULL;
            FreePamLineContents(lineObj);

            BAIL_ON_CENTERIS_ERROR(ceError = ParsePamLine(lineObj, fromFileBackup, original, NULL));

            conf->modified = 1;
            continue;
        }
    }

    line = NextLineForService(conf, -1, service, phase);
    while(line != -1)
    {
        lineObj = &conf->lines[line];
        if(lineObj->module == NULL)
            module = "";
        else
            module = lineObj->module->value;

        if(lineObj->control == NULL)
            control = "";
        else
            control = lineObj->control->value;

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        BAIL_ON_CENTERIS_ERROR(ceError = GetIncludeName(lineObj, &includeService));
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamLwidentityDisable(conf, includeService, phase, pam_lwidentity, state);
            if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                ceError = CENTERROR_SUCCESS;
            if(ceError == CENTERROR_DOMAINJOIN_PAM_BAD_CONF)
                ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsLwidentity(phase, module))
        {
            DJ_LOG_INFO("Removing pam_lwidentity from service %s", service);
            BAIL_ON_CENTERIS_ERROR(ceError = RemoveLine(conf, &line));
            state->configuredRequestedModule = TRUE;
            continue;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            if(state->configuredRequestedModule && !state->sawPromptingModule)
            {
                int found = 0;
                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "try_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed try_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_authtok", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_authtok from module %s", module);

            }
            state->sawPromptingModule = TRUE;
        }

        line = NextLineForService(conf, line, service, phase);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    CT_SAFE_FREE_STRING(fromFileBackup);
    CT_SAFE_FREE_STRING(commentBackup);
    return ceError;
}

static CENTERROR SetPamTokenValue(CTParseToken **token, CTParseToken *prev, const char *value)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    if(prev != NULL && (prev->trailingSeparator == NULL ||
        strlen(prev->trailingSeparator) < 1))
    {
        CT_SAFE_FREE_STRING(prev->trailingSeparator);
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(" ", &prev->trailingSeparator));
    }
    if(*token == NULL)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(CTParseToken), (void **) token));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("", &(*token)->trailingSeparator));
    }
    else
        CT_SAFE_FREE_STRING((*token)->value);

    BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(value, &(*token)->value));

error:
    return ceError;
}

static CENTERROR FindPamDenyLikeModule(const char *testPrefix, char **modulePath, char **moduleOption)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    DistroInfo distro;
    memset(&distro, 0, sizeof(distro));
    *modulePath = NULL;
    *moduleOption = NULL;

    ceError = FindModulePath(testPrefix, "pam_deny", modulePath);
    if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_MODULE)
    {
        ceError = FindModulePath(testPrefix, "pam_prohibit", modulePath);
    }
    if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_MODULE)
    {
        /*Solaris 8 does not have pam_deny. It does have pam_sample that has a
         * mode to act like pam_deny. pam_sample should be available on all
         * Solaris systems because it is provided by the same package that
         * provides pam_unix.
         *
         * Because pam_sample is a vague name, it will only be used on Solaris
         * systems.
         */
        ceError = DJGetDistroInfo(testPrefix, &distro);
        BAIL_ON_CENTERIS_ERROR(ceError);
        if(distro.os == OS_SUNOS)
        {
            ceError = FindModulePath(testPrefix, "pam_sample", modulePath);
            if(CENTERROR_IS_OK(ceError))
            {
                ceError = CTStrdup("always_fail", moduleOption);
                BAIL_ON_CENTERIS_ERROR(ceError);
            }
        }
    }
error:
    if(!CENTERROR_IS_OK(ceError))
    {
        CT_SAFE_FREE_STRING(*modulePath);
        CT_SAFE_FREE_STRING(*moduleOption);
    }
    DJFreeDistroInfo(&distro);
    return ceError;
}

static void PamLwidentityEnable(const char *testPrefix, const DistroInfo *distro, struct PamConf *conf, const char *service, const char * phase, const char *pam_lwidentity, struct ConfigurePamModuleState *state, LWException **exc)
{
    int prevLine = -1;
    int line = NextLineForService(conf, -1, service, phase);
    int lwidentityLine;
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;
    BOOLEAN sawNonincludeLine = FALSE;
    char *pam_deny_path = NULL;
    char *pam_deny_option = NULL;
    StringBuffer comment;
    BOOLEAN doingPasswdForAIX = FALSE;
    LWException *nestedException = NULL;
    PSTR newMessage = NULL;

    DJ_LOG_INFO("Enabling pam_lwidentity for pam service %s for phase %s", service, phase);
    memset(&comment, 0, sizeof(comment));

    if(line == -1)
    {
        /* This means that this service is not defined for this phase. An example is the session phase of passwd.
         * This function will return an error, and the parent function should decide whether or not to ignore the error.
         */
        LW_CLEANUP_CTERR(exc, CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE);
    }

    /* Pam enable algorithm:

     Find the first pam line that would block our module from accepting
     logins, and insert our module in front of it. Along the way, note
     if any password prompting modules were passed.  (e.g., required
     pam_allowroot, sufficient pam_permit)

     If the blocking line blocks all logins (including local), do not
     add lwidentity to this service.  (For example, if the service only
     had pam_deny.  Note that a sufficent line above would still allow
     users in.)

     If there are no lines which block domain logins, add our module to
     the end of the stack.  If this is the auth phase, throw an error
     instead.

     For the auth and password phases, fix up the try_first_pass options
     afterwards.

    */
    while(line != -1)
    {
        lineObj = &conf->lines[line];
        if(lineObj->module == NULL)
            module = "";
        else
            module = lineObj->module->value;

        if(lineObj->control == NULL)
            control = "";
        else
            control = lineObj->control->value;

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        LW_CLEANUP_CTERR(exc, GetIncludeName(lineObj, &includeService));
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            state->includeLevel++;
            PamLwidentityEnable(testPrefix, distro, conf, includeService, phase, pam_lwidentity, state, &nestedException);
            state->includeLevel--;
            if(LW_IS_OK(nestedException))
            {
                LW_HANDLE(&nestedException);
                if(state->configuredRequestedModule)
                {
                    prevLine = line;
                    line = NextLineForService(conf, line, service, phase);
                    break;
                }
            }
            else if(nestedException->code == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
            {
                LW_HANDLE(&nestedException);
            }
            else
            {
                CTAllocateStringPrintf(&newMessage, "Encountered while processing %s service:\n%s", service, nestedException->longMsg);
                CT_SAFE_FREE_STRING(nestedException->longMsg);
                nestedException->longMsg = newMessage;
                newMessage = NULL;
            }
            LW_CLEANUP(exc, nestedException);
        }

        if(PamModuleRemoveOnEnable(phase, control, module))
        {
            char *fromFileBackup;
            char *leadingWhiteSpaceBackup;

            DJ_LOG_INFO("Commenting out '%s' module", module);
            CTStringBufferClear(&comment);
            LW_CLEANUP_CTERR(exc, CTStringBufferAppend(&comment, "#Commented out by lwidentity: "));
            LW_CLEANUP_CTERR(exc, AppendPamLine(&comment, lineObj));

            fromFileBackup = lineObj->fromFile;
            lineObj->fromFile = NULL;
            leadingWhiteSpaceBackup = lineObj->leadingWhiteSpace;
            lineObj->leadingWhiteSpace = NULL;
            FreePamLineContents(lineObj);
            lineObj->fromFile = fromFileBackup;
            lineObj->leadingWhiteSpace = leadingWhiteSpaceBackup;
            lineObj->comment = CTStringBufferFreeze(&comment);

            prevLine = line;
            line = NextLineForService(conf, line, service, phase);
            continue;
        }

        if( !strcmp(module, "pam_passwd_auth.so.1") &&
                (!strcmp(control, "required") ||
                !strcmp(control, "requisite")))
        {
            /*Solaris's password authentication module must be first on the stack, but it blocks our module. So we'll just rework their config a little bit.
             *
             * This changes:
             * auth required pam_passwd_auth.so.1
             * to:
             * auth sufficient pam_passwd_auth.so.1
             * auth required pam_deny.so.1
             */
            int newLine = -1;
            LW_CLEANUP_CTERR(exc, CopyLine(conf, line, &newLine));
            lineObj = &conf->lines[newLine];
            if(pam_deny_path == NULL)
            {
                LW_CLEANUP_CTERR(exc, FindPamDenyLikeModule(testPrefix, &pam_deny_path, &pam_deny_option));
            }
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_deny_path));
            lineObj->optionCount = 0;
            if(pam_deny_option != NULL)
                LW_CLEANUP_CTERR(exc, AddOption(conf, newLine, pam_deny_option));

            lineObj = &conf->lines[line];
            CT_SAFE_FREE_STRING(lineObj->control->value);
            LW_CLEANUP_CTERR(exc, CTStrdup("sufficient", &lineObj->control->value));
        }

        if(PamModuleAlwaysDeniesDomainLogins(phase, module) && (
                    !strcmp(control, "required") ||
                    !strcmp(control, "requisite")))
            break;

        if(PamModuleIsLwidentity(phase, module))
        {
            DJ_LOG_INFO("Found pam_lwidentity");
            /* Fix up the options incase this module was installed by a previous version of the product */
            if(state->sawPromptingModule)
            {
                if(!strcmp(phase, "auth") && !ContainsOption(conf, line, "try_first"))
                {
                    DJ_LOG_INFO("Fixing up pam_lwidentity line which must have come from a previous install");
                    /* Use the previously prompted current password */
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, "try_first_pass"));
                }
                else if(!strcmp(phase, "password") && (!ContainsOption(conf, line, "try_first_pass") || !ContainsOption(conf, line, "use_authtok")))
                {
                    DJ_LOG_INFO("Fixing up pam_lwidentity line which must have come from a previous install");
                    /* Try the old and new password, if it has been prompted */
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, "try_first_pass"));
                    /* Definitely use the new password */
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, "use_authtok"));
                }
            }
            state->configuredRequestedModule = TRUE;
            prevLine = line;
            line = NextLineForService(conf, line, service, phase);
            break;
        }

        if(!strcmp(control, "sufficient") &&
                PamModuleGrants(phase, module) &&
                !PamModulePrompts(phase, module) &&
                !PamModuleChecksCaller(phase, module))
        {
            /* This module seems to let anyone through */
            break;
        }

        if(includeService == NULL)
        {
            DJ_LOG_VERBOSE("It is not an include line");
            sawNonincludeLine = TRUE;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            state->sawPromptingModule = TRUE;
        }

        if( !strcmp(control, "sufficient") &&
                PamModulePrompts(phase, module) &&
                PamModuleAlwaysDeniesDomainLogins(phase, module))
        {
            state->sawSufficientPasswordCheck = TRUE;
        }

        if( (!strcmp(control, "required") ||
                    !strcmp(control, "requisite")) &&
                PamModuleChecksCaller(phase, module))
        {
            state->sawCallerRequirementLine = TRUE;
        }

        if(!PamModuleAlwaysDeniesDomainLogins(phase, module))
        {
            state->sawDomainUserGrantingLine = TRUE;
        }

        prevLine = line;
        line = NextLineForService(conf, line, service, phase);
    }

    if(line == -1)
    {
        DJ_LOG_INFO("Bottomed out of pam stack");
    }

    if(!state->configuredRequestedModule)
    {
        if(!strcmp(phase, "auth") &&
                line == -1 &&
                !state->sawDomainUserGrantingLine &&
                sawNonincludeLine)
        {
            /* This tries to fix the user's pam configuration. The user has
             * something like:
             * auth sufficient pam_rootok.so
             *
             * Without a required line after it. This is a risky way to set
             * a pam configuration, but it works. We'll add this to the end of
             * the stack so the rest of our logic works:
             * auth required pam_deny.so
             *
             * Here are the services this is known to affect:
             * - groupadd, groupmod, groupdel, newusers, useradd,
             *   usermod, and userdel - on Ubuntu
             * - chage on Ubuntu
             * - quagga on Centos
             */
            LW_CLEANUP_CTERR(exc, CopyLine(conf, prevLine, &line));
            lineObj = &conf->lines[line];
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->phase, lineObj->service, "auth"));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "required"));

            if(pam_deny_path == NULL)
            {
                LW_CLEANUP_CTERR(exc, FindPamDenyLikeModule(testPrefix, &pam_deny_path, &pam_deny_option));
            }
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_deny_path));
            lineObj->optionCount = 0;
            if(pam_deny_option != NULL)
                LW_CLEANUP_CTERR(exc, AddOption(conf, line, pam_deny_option));

            module = lineObj->module->value;
            control = lineObj->control->value;
        }
        if(!strcmp(phase, "auth") &&
                (line == -1 ||
                (!strcmp(control, "sufficient") &&
                PamModuleGrants(phase, module) &&
                !PamModulePrompts(phase, module) &&
                !PamModuleChecksCaller(phase, module))))
        {
            if(state->sawCallerRequirementLine)
            {
                /* This service is protected by something other than a password
                 */
                goto cleanup;
            }

            /* Nothing checks the password for local users?
             */

            /* There are a few systems that are known to be broken. They
             * are listed here:
             *
             * - gdm-autologin doesn't prompt for a password
             * - passwd - doesn't use the auth phase on Ubuntu
             * - rsh - I can't figure out how rsh on Solaris works. It
             *   must just use rlogin
             * - shadow - Suse has something unprotected called
             *    /etc/pam.d/shadow. I can't figure out what uses it.
             * - runuser/runlevel-l - Requires root-ness to do its job.  So it only
             *   checks for that.  (Found on CentOS 5.)
             *
             * kde-np is the passwordless login. It is the kde equivalent of
             * gdm-autologin.
             * xdm-np is the passwordless login. It is the xdm equivalent of
             * gdm-autologin.
             * - useradd on Suse 10.2 has sufficient pam_rootok followed by
             *   required pam_permit.so. The program is not setuid root though
             *
             * - gnome-screensaver-smartcard on Suse 10.3 includes
             *   common-auth-smartcard. If that file does not exist on the
             *   system, then gnome-screensaver-smartcard does not have any
             *   auth modules.
             */
            if(!strcmp(service, "gdm-autologin"))
                goto cleanup;
            if(!strcmp(service, "passwd"))
                goto cleanup;
            if(!strcmp(service, "rsh"))
                goto cleanup;
            if(!strcmp(service, "shadow"))
                goto cleanup;
            if(!strcmp(service, "runuser"))
                goto cleanup;
            if(!strcmp(service, "runuser-l"))
                goto cleanup;
            /* I'm not sure if this is a typo or not */
            if(!strcmp(service, "kde-np"))
                goto cleanup;
            /* Centos 5 has kdm-np as part of an optional package */
            if(!strcmp(service, "kdm-np"))
                goto cleanup;
            if(!strcmp(service, "xdm-np"))
                goto cleanup;
            if(!strcmp(service, "useradd"))
                goto cleanup;
            if(!strcmp(service, "gnome-screensaver-smartcard"))
                goto cleanup;

            DJ_LOG_ERROR("Nothing seems to be protecting logins for service %s", service);
            if(!strcmp(control, "sufficient"))
            {
                LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_PAM_BAD_CONF, "Unknown pam module", "The likewise PAM module cannot be configured for the %s service. This services uses the '%s' module, which is not in this program's list of known modules. Please email Likewise technical support and include a copy of /etc/pam.conf or /etc/pam.d.", service, module);
            }
            //It is somewhat normal to not require a password in an included
            //pam file. It is up to the top-most parent to require the password.
            else if(state->includeLevel == 0)
            {
                LW_RAISE_EX(exc, CENTERROR_DOMAINJOIN_PAM_BAD_CONF, "Unknown pam configuration", "The likewise PAM module cannot be configured for the %s service. Either this service is unprotected (does not require a valid password for access), or it is using a pam module that this program is unfamiliar with. Please email Likewise technical support and include a copy of /etc/pam.conf or /etc/pam.d.", service);
            }
            goto cleanup;
        }

        if(line != -1)
        {
            char buffer[256] = "";
            if(!state->sawSufficientPasswordCheck &&
                    (!PamModuleGrants(phase, module) || PamModuleChecksCaller(phase, module)) &&
                    (!strcmp(control, "required") ||
                    !strcmp(control, "requisite")))
            {
                /* I guess the user wants to block everyone from logging in.
                 * In this case, we won't let domain users in either, unless
                 * it is the password phase on OS X. OS X has an OS bug where
                 * it blocks password changes during login for all services
                 * except passwd and interactive logins.
                 */
                if( !(distro->os == OS_DARWIN && !strcmp(phase, "password")) )
                {
                    goto cleanup;
                }
            }
            /*Insert our module before the line*/
            DJ_LOG_INFO("Inserting pam_lwidentity before %s", module);
            NormalizeModuleName( buffer, module, sizeof(buffer));
            if(!strcmp(phase, "password") && !strcmp(buffer, "pam_aix"))
                doingPasswdForAIX = TRUE;
            LW_CLEANUP_CTERR(exc, CopyLine(conf, line, NULL));
            lwidentityLine = line;
        }
        else
        {
            if(!sawNonincludeLine)
            {
                LW_CLEANUP_CTERR(exc, CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE);
            }
            /*There was nothing to block domain users. We'll add our module at the bottom of the stack.
             * prevLine != -1, because a noninclude line was seen.
             *
             * Since this is not an auth phase, our module is still added because we should do things like support password changes, make home directories, etc..
             */
            DJ_LOG_INFO("Inserting pam_lwidentity at the end of the pam stack");
            LW_CLEANUP_CTERR(exc, CopyLine(conf, prevLine, &lwidentityLine));
        }

        /* Fill in the correct values for lwidentityLine */
        lineObj = &conf->lines[lwidentityLine];
        /* This is incase the phase is @include */
        LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->phase, lineObj->service, phase));
        LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "sufficient"));
        LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->module, lineObj->control, pam_lwidentity));
        lineObj->optionCount = 0;

        if(!strcmp(phase, "account"))
        {
            /* Group membership checks, aka "allow local logins" is
             * normally enforced during the auth phase. However, logins
             * using ssh keys bypass entering a password, and the whole
             * auth phase. So pam_lwidentity can't block users with bad
             * group membership in that case.
            *
            * The account phase will still be called. Group membership
            * will be enforced in the account phase as well as the auth
            * phase. The following lines will allow local users to
            * continue down the stack (first line succeeds and second
            * fails).
            *
            * In the case of domain users, the first line will block
            * domain users who don't meet group membership
            * requirements. The second line will allow the rest of the
            * domain users into the system.
            *
            * account required pam_lwidentity unknown_ok
            * account sufficient pam_lwidentity
            */
            LW_CLEANUP_CTERR(exc, CopyLine(conf, lwidentityLine, NULL));
            lineObj = &conf->lines[lwidentityLine];
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "required"));
            LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "unknown_ok"));
        }
        else if(state->sawPromptingModule)
        {
            DJ_LOG_INFO("Telling pam_lwidentity to use the previously prompted password");
            if(!strcmp(phase, "auth"))
            {
                /* Use the previously prompted current password */
                LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "try_first_pass"));
            }
            else if(!strcmp(phase, "password"))
            {
                /* Try the old and new password, if it has been prompted */
                LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "try_first_pass"));
                /* Definitely use the new password */
                LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "use_authtok"));
            }
        }
        if(doingPasswdForAIX)
        {
            /* On AIX, first PAM based modules will have a chance to change
             * the password, then LAM based modules will have a chance. The
             * problem is that if the pam_lwidentity module is sufficient,
             * then we will first prompt during the PAM check. If the PAM check
             * fails (because they didn't enter the right password or
             * something), then the password will be reprompted for LAM. We
             * don't have much control over the LAM prompts, so they look bad.
             * We would like to avoid having the LAM prompts displayed.
             *
             * To do this correctly, we need a way of:
             * 1. exiting the PAM stack successfully if the password is
             * correctly changed for a domain
             * 2. exiting the PAM stack with an error if something went wrong
             * for a domain user.
             * 3. continuing down the PAM stack if the user is not a domain
             * user.
             *
             * This configuration will accomplish all of those tasks:
             * passwd  password  requisite     pam_lwidentity.so unknown_ok remember_chpass
             * passwd  password  sufficient    pam_lwidentity.so
             * passwd  password  required      /usr/lib/security/pam_aix
             */
            LW_CLEANUP_CTERR(exc, CopyLine(conf, lwidentityLine, NULL));
            LW_CLEANUP_CTERR(exc, SetPamTokenValue(&lineObj->control, lineObj->phase, "requisite"));
            LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "unknown_ok"));
            LW_CLEANUP_CTERR(exc, AddOption(conf, lwidentityLine, "remember_chpass"));
            lineObj = &conf->lines[lwidentityLine];
        }

        /* I don't think this code is necessary
        BOOLEAN understandsNewAuthtokReqd;
#ifdef __LWI_SOLARIS__
        understandsNewAuthtokReqd = FALSE;
#else
        understandsNewAuthtokReqd = TRUE;
#endif

        if(!strcmp(phase, "auth") && !understandsNewAuthtokReqd)
        {
            * This is for machines which do not correctly handle the PAM_NEW_AUTHTOK_REQD return code when the module is marked as sufficient. For these systems, we have to have our module run as required underneath the sufficient line.
             *
            BAIL_ON_CENTERIS_ERROR(ceError = CopyLine(conf, lwidentityLine, &lwidentityLine));
            lineObj = &conf->lines[lwidentityLine];
            CT_SAFE_FREE_STRING(lineObj->control->value);
            BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("required", &lineObj->control->value));
            BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, lwidentityLine, "unknown_ok"));
            * This will only add the option if it hasn't been added so far *
            BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, lwidentityLine, "use_first_pass"));
            state->sawPromptingModule = TRUE;
        }*/
        line = NextLineForService(conf, lwidentityLine, service, phase);
        state->configuredRequestedModule = TRUE;
    }

    /* If pam_lwidentity is the first prompting module on the stack, the next module needs to have something like try_first_pass added.
     */
    while(line != -1 && !state->sawPromptingModule)
    {
        lineObj = &conf->lines[line];
        if(lineObj->module == NULL)
            module = "";
        else
            module = lineObj->module->value;

        if(lineObj->control == NULL)
            control = "";
        else
            control = lineObj->control->value;

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);
        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_INFO("Making sure module %s uses the password stored by pam_lwidentity", module);
            if(!strcmp(phase, "auth"))
            {
                if(!ContainsOption(conf, line, "use_first_pass"))
                {
                    const char *optionName = "try_first_pass";
                    if(!PamModuleUnderstandsTryFirstPass(phase, module))
                        optionName = "use_first_pass";
                    LW_CLEANUP_CTERR(exc, AddOption(conf, line, optionName));
                }
            }
            /* Right now pam_lwidentity doesn't store the password for non-domain users, so we shouldn't do this for subsequent modules.
            else if(!strcmp(phase, "password"))
            {
                if(!ContainsOption(conf, line, "use_first_pass"))
                {
                    * Try the old and new password, if it has been prompted *
                    BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "try_first_pass"));
                }
                * Definitely use the new password *
                BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_authtok"));
            }
            */
            state->sawPromptingModule = TRUE;
        }
        line = NextLineForService(conf, line, service, phase);
    }
cleanup:
    CTStringBufferDestroy(&comment);
    CT_SAFE_FREE_STRING(includeService);
    CT_SAFE_FREE_STRING(pam_deny_path);
    CT_SAFE_FREE_STRING(pam_deny_option);
    CT_SAFE_FREE_STRING(newMessage);
    LW_HANDLE(&nestedException);
}

static CENTERROR PamLwiPassPolicyDisable(struct PamConf *conf, const char *service, const char * phase, const char *pam_lwipasspolicy, struct ConfigurePamModuleState *state)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;

    if(strcmp(phase, "password"))
    {
        /*Only install the password policy for the password phase*/
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    DJ_LOG_INFO("Disabling password policy for pam service %s for phase %s", service, phase);

    while(line != -1)
    {
        lineObj = &conf->lines[line];
        if(lineObj->module == NULL)
            module = "";
        else
            module = lineObj->module->value;

        if(lineObj->control == NULL)
            control = "";
        else
            control = lineObj->control->value;

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        BAIL_ON_CENTERIS_ERROR(ceError = GetIncludeName(lineObj, &includeService));
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamLwiPassPolicyDisable(conf, includeService, phase, pam_lwipasspolicy, state);
            if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                ceError = CENTERROR_SUCCESS;
            if(ceError == CENTERROR_DOMAINJOIN_PAM_BAD_CONF)
                ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsLwiPassPolicy(phase, module))
        {
            DJ_LOG_INFO("Removing pam_lwipasspolicy from service %s", service);
            BAIL_ON_CENTERIS_ERROR(ceError = RemoveLine(conf, &line));
            state->configuredRequestedModule = TRUE;
            continue;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            if(state->configuredRequestedModule && !state->sawPromptingModule)
            {
                int found = 0;
                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "try_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed try_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_first_pass", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_first_pass from module %s", module);

                BAIL_ON_CENTERIS_ERROR(ceError = RemoveOption(conf, line, "use_authtok", &found));
                if(found > 0)
                    DJ_LOG_INFO("Removed use_authtok from module %s", module);

            }
            state->sawPromptingModule = TRUE;
        }

        line = NextLineForService(conf, line, service, phase);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    return ceError;
}

static CENTERROR PamLwiPassPolicyEnable(struct PamConf *conf, const char *service, const char * phase, const char *pam_lwipasspolicy, struct ConfigurePamModuleState *state)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;
    BOOLEAN sawNonincludeLine = FALSE;

    if(strcmp(phase, "password"))
    {
        /*Only install the password policy for the password phase*/
        ceError = CENTERROR_SUCCESS;
        goto error;
    }

    DJ_LOG_INFO("Enabling password policy for pam service %s for phase %s", service, phase);

    if(line == -1)
    {
        /* This means that this service is not defined for this phase.
         * This function will return an error, and the parent function should decide whether or not to ignore the error.
         */
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE);
    }

    /* Insert the password policy module before the first module that prompts and isn't pam_lwidentity. Add use_authtok to the prompting module if necessary. */
    while(line != -1)
    {
        lineObj = &conf->lines[line];
        if(lineObj->module == NULL)
            module = "";
        else
            module = lineObj->module->value;

        if(lineObj->control == NULL)
            control = "";
        else
            control = lineObj->control->value;

        DJ_LOG_VERBOSE("Looking at entry %s %s %s %s", service, phase, control, module);

        CT_SAFE_FREE_STRING(includeService);
        BAIL_ON_CENTERIS_ERROR(ceError = GetIncludeName(lineObj, &includeService));
        if(includeService != NULL)
        {
            DJ_LOG_INFO("Including %s for service %s", includeService, service);
            ceError = PamLwiPassPolicyEnable(conf, includeService, phase, pam_lwipasspolicy, state);
            if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                ceError = CENTERROR_SUCCESS;
            if(ceError == CENTERROR_DOMAINJOIN_PAM_BAD_CONF)
                ceError = CENTERROR_SUCCESS;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }

        if(PamModuleIsLwiPassPolicy(phase, module))
        {
            DJ_LOG_INFO("Found pam_lwipasspolicy");
            state->configuredRequestedModule = TRUE;
        }

        if(includeService == NULL)
        {
            DJ_LOG_VERBOSE("It is not an include line");
            sawNonincludeLine = TRUE;
        }

        if(PamModulePrompts(phase, module))
        {
            DJ_LOG_VERBOSE("The module prompts");
            /* On Suse, we rely on pam_pwcheck to prompt for the old password,
             * so our module must come after it.
             */
            if(!state->configuredRequestedModule && !PamModuleIsLwidentity(phase, module) && !PamModuleIsPwcheck(phase, module))
            {
                DJ_LOG_INFO("Inserting %s before %s", pam_lwipasspolicy, module);
                BAIL_ON_CENTERIS_ERROR(ceError = CopyLine(conf, line, NULL));
                /* Fill in the correct values for the password policy */
                lineObj = &conf->lines[line];

                BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(pam_lwipasspolicy, &lineObj->module->value));
                BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("required", &lineObj->control->value));
                lineObj->optionCount = 0;

                if(state->sawPromptingModule)
                {
                    if(state->sawPwcheck)
                    {
                        /* Always use the old and new password */
                        BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_first_pass"));
                    }
                    else
                    {
                        /* Try the old and new password, if it has been prompted */
                        BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "try_first_pass"));
                    }
                    /* Definitely use the new password */
                    BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_authtok"));
                }
                state->configuredRequestedModule = TRUE;
            }
            else
            {
                if(!state->sawPromptingModule && state->configuredRequestedModule && !PamModuleIsLwiPassPolicy(phase, module))
                {
                    if(!ContainsOption(conf, line, "use_first_pass"))
                    {
                        /* Try the old and new password, if it has been prompted */
                        BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "try_first_pass"));
                    }
                    /* Definitely use the new password */
                    BAIL_ON_CENTERIS_ERROR(ceError = AddOption(conf, line, "use_authtok"));
                }
                state->sawPromptingModule = TRUE;
                if(PamModuleIsPwcheck(phase, module))
                    state->sawPwcheck = TRUE;
            }
        }

        line = NextLineForService(conf, line, service, phase);
    }

    if(!state->configuredRequestedModule && !sawNonincludeLine)
    {
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE);
    }

error:
    CT_SAFE_FREE_STRING(includeService);
    return ceError;
}

static CENTERROR FindModulePath(const char *testPrefix, const char *basename, char **destName)
{
    /*If you update this function, update NormalizeModuleName too */
    CENTERROR ceError = CENTERROR_SUCCESS;
    int i, j, k;
    int foundPath = -1;
    char *fullPath = NULL;
    BOOLEAN exists;
    const char *searchDirs[] = {
        "/lib/security",
        "/lib64/security",
        "/usr/lib/security",
        "/usr/lib64/security",
        "/lib/security/hpux32",
        "/lib/security/hpux64",
        "/usr/lib/pam",
        "/usr/lib/security/sparcv9",
        "/usr/lib/security/amd64",
        NULL
    };

    const char *searchNamePrefixes[] = {
        "",
        "lib",
        NULL
    };

    const char *searchNameSuffixes[] = {
        ".sl",
        ".sl.1",
        ".so",
        ".so.1",
        ".1",
        NULL
    };

    char *foundName = NULL;

    DJ_LOG_INFO("Searching for the system specific path of %s", basename);

    for(i = 0; searchDirs[i] != NULL; i++)
    {
        for(j = 0; searchNamePrefixes[j] != NULL; j++)
        {
            for(k = 0; searchNameSuffixes[k] != NULL; k++)
            {
                CT_SAFE_FREE_STRING(fullPath);
                BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&fullPath, "%s%s/%s%s%s", testPrefix, searchDirs[i], searchNamePrefixes[j], basename, searchNameSuffixes[k]));
                DJ_LOG_VERBOSE("Checking if %s exists", fullPath);
                BAIL_ON_CENTERIS_ERROR(ceError = CTCheckFileOrLinkExists(fullPath, &exists));
                if(exists)
                {
                    DJ_LOG_INFO("Found pam module %s", fullPath);
                    /* If the file is found in exactly one path, use the full path. If the file is found in more than one path, use the relative name. */
                    if(foundPath == i)
                    {
                        /* We already found something in this dir */
                    }
                    else if(foundPath == -1)
                    {
                        CT_SAFE_FREE_STRING(fullPath);
                        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&fullPath, "%s/%s%s%s", searchDirs[i], searchNamePrefixes[j], basename, searchNameSuffixes[k]));
                        foundName = fullPath;
                        fullPath = NULL;
                        foundPath = i;
                    }
                    else if(foundPath != -2)
                    {
                        /* This is the second directory that the file has been
                         * found in. Strip off the directory name
                         */
                        CT_SAFE_FREE_STRING(foundName);
                        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&fullPath, "%s%s%s", searchNamePrefixes[j], basename, searchNameSuffixes[k]));
                        foundName = fullPath;
                        fullPath = NULL;
                        foundPath = -2;
                    }
                    break;
                }
            }
        }
    }

    if(foundPath == -1)
    {
        DJ_LOG_WARNING("Unable to find %s", basename);
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_DOMAINJOIN_PAM_MISSING_MODULE);
    }
    else
    {
        DJ_LOG_INFO("Using module path '%s'", foundName);
    }

error:
    CT_SAFE_FREE_STRING(fullPath);
    if(destName != NULL)
        *destName = foundName;
    else
        CT_SAFE_FREE_STRING(foundName);
    return ceError;
}

static CENTERROR FindPamLwiPassPolicy(const char *testPrefix, char **destName)
{
    return FindModulePath(testPrefix, "pam_lwipasspolicy", destName);
}

static CENTERROR FindPamLwidentity(const char *testPrefix, char **destName)
{
    CENTERROR ceError = FindModulePath(testPrefix, "pam_lwidentity", destName);
    if(!CENTERROR_IS_OK(ceError))
    {
        DJ_LOG_ERROR("Unable to find pam_lwidentity");
    }
    return ceError;
}

void DJUpdatePamConf(const char *testPrefix,
        struct PamConf *conf,
        WarningFunction warning,
        BOOLEAN enable,
        LWException **exc)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct ConfigurePamModuleState state;
    char *pam_lwidentity = NULL;
    char *pam_lwipasspolicy = NULL;
    int i,j;
    const char *phases[] = {"auth", "account", "password", "session", NULL};
    int serviceCount;
    char **services = NULL;
    DistroInfo distro;
    LWException *nestedException = NULL;

    memset(&distro, 0, sizeof(distro));
    if(testPrefix == NULL)
        testPrefix = "";
    LW_CLEANUP_CTERR(exc, DJGetDistroInfo(testPrefix, &distro));
    LW_CLEANUP_CTERR(exc, ListServices(conf, &services, &serviceCount));

    if(enable)
    {
        LW_CLEANUP_CTERR(exc, FindPamLwidentity(testPrefix, &pam_lwidentity));
        ceError = FindPamLwiPassPolicy(testPrefix, &pam_lwipasspolicy);
        /* Ignore the password policy on systems that don't have it */
        if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_MODULE)
            ceError = CENTERROR_SUCCESS;
        LW_CLEANUP_CTERR(exc, ceError);
    }

    for(i = 0; i < serviceCount; i++)
    {
        for(j = 0; phases[j] != NULL; j++)
        {
            memset(&state, 0, sizeof(state));
            if(enable)
            {
                LW_HANDLE(&nestedException);
                PamLwidentityEnable(testPrefix, &distro, conf, services[i], phases[j], pam_lwidentity, &state, &nestedException);
                if(LW_IS_OK(nestedException) &&
                        !state.configuredRequestedModule &&
                        !strcmp(phases[j], "auth"))
                {
                    /* The only way this could happen is if the module lets the users through based on something other than passwords (like whether the user is on a system console). If pam_lwidentity was not installed for the auth phase, it shouldn't be installed for the other phases either.
                     */
                    break;
                }
                if(!LW_IS_OK(nestedException))
                {
                    if(nestedException->code ==
                            CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                    {
                        LW_HANDLE(&nestedException);
                    }
                    else if(warning != NULL &&
                            !IsRequiredService(services[i], conf))
                    {
                        //Show all errors for this service as warnings and continue
                        warning(nestedException->shortMsg, nestedException->longMsg);
                        DJLogException(LOG_LEVEL_WARNING, nestedException);
                        LW_HANDLE(&nestedException);
                    }
                }
                LW_CLEANUP(exc, nestedException);
            }
            else
            {
                ceError = PamLwidentityDisable(conf, services[i], phases[j], pam_lwidentity, &state);
                if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                    ceError = CENTERROR_SUCCESS;
                LW_CLEANUP_CTERR(exc, ceError);
            }

            memset(&state, 0, sizeof(state));
            ceError = PamOldCenterisDisable(conf, services[i], phases[j], &state);
            if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                ceError = CENTERROR_SUCCESS;
            LW_CLEANUP_CTERR(exc, ceError);
        }
        memset(&state, 0, sizeof(state));
        if(enable && pam_lwipasspolicy != NULL)
        {
            ceError = PamLwiPassPolicyEnable(conf, services[i], "password", pam_lwipasspolicy, &state);
        }
        else
        {
            ceError = PamLwiPassPolicyDisable(conf, services[i], "password", pam_lwipasspolicy, &state);
        }
        if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
            ceError = CENTERROR_SUCCESS;
        LW_CLEANUP_CTERR(exc, ceError);
    }

cleanup:
    LW_HANDLE(&nestedException);
    DJFreeDistroInfo(&distro);
    if(services == NULL)
        FreeServicesList(services);
    CT_SAFE_FREE_STRING(pam_lwidentity);
    CT_SAFE_FREE_STRING(pam_lwipasspolicy);
}

static CENTERROR
AddMissingAIXServices(struct PamConf *conf)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    /*AIX does not use PAM by default. It's default pam.conf is actually broken for a few services.
     *
     * During the install we switch it over to pam mode, so we have to fix up its default services
     */
    if(NextLineForService(conf, -1, "sshd", "auth") == -1)
    {
        /* Fixes bug 3799 */
        DJ_LOG_INFO("Adding pam entries for sshd");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "telnet", "sshd"));
    }

    if(NextLineForService(conf, -1, "sudo", "auth") == -1)
    {
        DJ_LOG_INFO("Adding pam entries for sudo");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "telnet", "sudo"));
    }

    if(NextLineForService(conf, -1, "dtsession", "auth") == -1)
    {
        /* Fixes bug 4200 */
        DJ_LOG_INFO("Adding pam entries for dtsession");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "sshd", "dtsession"));
    }

    if(NextLineForService(conf, -1, "dtlogin", "auth") == -1)
    {
        /* Fixes bug 4906 */
        DJ_LOG_INFO("Adding pam entries for dtlogin");
        BAIL_ON_CENTERIS_ERROR(ceError = CopyService(conf, "telnet", "dtlogin"));
    }

error:
    return ceError;
}

CENTERROR
DJAddMissingAIXServices(PCSTR rootPrefix)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct PamConf conf;
    memset(&conf, 0, sizeof(conf));

    if(rootPrefix == NULL)
        rootPrefix = "";

    GCE(ceError = ReadPamConfiguration(rootPrefix, &conf));
    GCE(ceError = AddMissingAIXServices(&conf));
    if(conf.modified)
    {
        GCE(ceError = WritePamConfiguration(rootPrefix, &conf, NULL));
    }

cleanup:
    FreePamConfContents(&conf);
    return ceError;
}

void DJNewConfigurePamForADLogin(
    const char * testPrefix,
    WarningFunction warning,
    BOOLEAN enable,
    LWException **exc
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    struct PamConf conf;
    char *pam_lwidentityconf = NULL;
    memset(&conf, 0, sizeof(conf));

    if(testPrefix == NULL)
        testPrefix = "";
    ceError = ReadPamConfiguration(testPrefix, &conf);
#ifdef __LWI_AIX__
    if(ceError == CENTERROR_INVALID_FILENAME)
    {
        /* This is an AIX 5.2 machine that doesn't have a pam.conf */
        ceError = CENTERROR_SUCCESS;
        goto cleanup;
    }
#endif
    LW_CLEANUP_CTERR(exc, ceError);

#ifdef __LWI_AIX__
    if(enable)
    {
        AddMissingAIXServices(&conf);
    }
#endif

    if(enable)
    {
        DJ_LOG_INFO("Making sure that try_first_pass is not on in pam_lwidentity.conf");
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
            &pam_lwidentityconf, "%s%s", testPrefix,
            "/etc/security/pam_lwidentity.conf"));
        LW_CLEANUP_CTERR(exc, CTRunSedOnFile(pam_lwidentityconf, pam_lwidentityconf,
            FALSE, "s/^\\([ \t]*try_first_pass[ \t]*=.*\\)$/# \\1/"));
    }

    LW_TRY(exc, DJUpdatePamConf(testPrefix, &conf, warning, enable, &LW_EXC));

    if(conf.modified)
        LW_CLEANUP_CTERR(exc, WritePamConfiguration(testPrefix, &conf, NULL));
    else
        DJ_LOG_INFO("Pam configuration not modified");

cleanup:
    FreePamConfContents(&conf);
    CT_SAFE_FREE_STRING(pam_lwidentityconf);
}

CENTERROR
ConfigurePamForADLogin(
    PSTR pszShortDomainName
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    PSTR ppszArgs[5];
    DWORD nArgs = 0;
    long status = 0;
    PPROCINFO pProcInfo = NULL;
    BOOLEAN enable = !IsNullOrEmptyString(pszShortDomainName);
    BOOLEAN restartDtLogin;
    int fds[3] = {-1, -1, -1};
    int i;

    char szBuff[PATH_MAX+1];

    sprintf(szBuff, "%s/ConfigureLogin", SCRIPTDIR);

    nArgs = 0;
    ppszArgs[nArgs++] = szBuff;

    if(!enable)
    {
        DJ_LOG_INFO("Disabling lwidentity from login files...");
        ppszArgs[nArgs++] = "disable";
    }
    else
    {
        DJ_LOG_INFO("Enabling lwidentity from login files...");
        ppszArgs[nArgs++] = "enable";
    }

    ppszArgs[nArgs++] = NULL;
    fds[0] = open("/dev/zero", O_RDONLY);
    fds[1] = STDOUT_FILENO;
    fds[2] = STDERR_FILENO;
    if (fds[0] < 0)
    {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    if(gdjLogInfo.dwLogLevel < LOG_LEVEL_VERBOSE)
    {
        fds[1] = open("/dev/zero", O_WRONLY);
        if (fds[1] < 0)
        {
            ceError = CTMapSystemError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
    }
    ceError = DJSpawnProcessWithFds(ppszArgs[0], ppszArgs, fds[0], fds[1], fds[2], &pProcInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetProcessStatus(pProcInfo, &status);
    BAIL_ON_CENTERIS_ERROR(ceError);

    if (status != 0) {
        DJ_LOG_ERROR("ConfigureLogin failed [Status code: %d]", status);
        ceError = CENTERROR_DOMAINJOIN_PAM_EDIT_FAIL;
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    /*First try to restart dtlogin if no one is logged in (which should be true if dtgreet is running */
    ceError = CTCheckFileOrLinkExists("/sbin/init.d/dtlogin.rc", &restartDtLogin);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if(restartDtLogin)
    {
        pid_t dtrcPid;
        DJ_LOG_INFO("Found dtlogin start/stop script");

        /* So we found the file to start and stop dtlogin, but is dtlogin
         * running now?
         */
        ceError = CTGetPidOfCmdLine("dtrc", "/sbin/sh /usr/dt/bin/dtrc", 0, &dtrcPid, NULL);
        if(ceError == CENTERROR_NO_SUCH_PROCESS)
        {
            DJ_LOG_INFO("Dtrc is not running");
            restartDtLogin = FALSE;
            ceError = CENTERROR_SUCCESS;
        }
        else if(ceError == CENTERROR_SUCCESS)
        {
            DJ_LOG_INFO("Dtrc is running with pid %ld", dtrcPid);
        }
        BAIL_ON_CENTERIS_ERROR(ceError);
    }
    if(restartDtLogin)
    {
        /* This is a HPUX machine. Dtlogin will be restarted if no one
           is logged in.  If dtgreet is running, nobody is logged in. */
        ceError = CTGetPidOf("dtgreet", 0, NULL, NULL);
        if(ceError == CENTERROR_NO_SUCH_PROCESS)
        {
            size_t dtloginCount = 0;
            DJ_LOG_VERBOSE("Dtgreet is not running");
            // The machine is not at the greet prompt
            ceError = CTGetPidOf("dtlogin", 0, NULL, &dtloginCount);
            BAIL_ON_CENTERIS_ERROR(ceError);
            if(dtloginCount < 2)
            {
                DJ_LOG_VERBOSE("Dtlogin is running %d instances. Dtlogin must be in the middle of a restart", dtloginCount);
                /* Xwindows is in the middle of restarting because dtrc (parent
                 * script for dtlogin) is running, but dtlogin and dtgreet are
                 * not running. Since dtlogin is restarting, no one is logged
                 * in.
                 */
                ceError = CENTERROR_SUCCESS;
            }
            else
            {
                // Dtgreet is not running, so someone is logged in
                restartDtLogin = FALSE;
                if(enable)
                {
                    /* I would use DJ_LOG_ERROR to output this message, but that
                     * doesn't get displayed by default.
                     */
                    fprintf(stderr,
                        "Warning:\n"
                        "The dtlogin process needs to be restarted for domain users to interactively login graphically, but it cannot be restarted at this time because a user is currently logged in. After the user exits, please run these commands as root, outside of an Xwindows session:\n"
                        "/sbin/init.d/dtlogin.rc stop\n"
                        "/sbin/init.d/dtlogin.rc start\n");
                }
                /* If we're disabling domain logins, it isn't critical that
                 * dtlogin is restarted. Without lwiauthd running, domain
                 * users won't be able to log in anyway. */
            }
        }
    }
    if(restartDtLogin)
    {
        DJ_LOG_INFO("Stopping dtlogin");
        ceError = CTRunCommand("/sbin/init.d/dtlogin.rc stop");
        if(ceError == CENTERROR_COMMAND_FAILED)
        {
            DJ_LOG_ERROR("Unable to stop dtlogin");
            ceError = CENTERROR_SUCCESS;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);
        DJ_LOG_INFO("Starting dtlogin");
        ceError = CTRunCommand("/sbin/init.d/dtlogin.rc start");
        if(ceError == CENTERROR_COMMAND_FAILED)
        {
            DJ_LOG_ERROR("Unable to start dtlogin");
            ceError = CENTERROR_SUCCESS;
        }
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

error:
    if(pProcInfo)
        FreeProcInfo(pProcInfo);
    for(i = 0; i < 3; i++)
    {
        //If fds[i] == i, then it is something like stdout
        if(fds[i] != -1 && fds[i] != i)
            close(fds[i]);
    }

    return ceError;
}

static CENTERROR IsLwidentityEnabled(struct PamConf *conf, const char *service, const char * phase, BOOLEAN *configured)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    int line = NextLineForService(conf, -1, service, phase);
    /* Do not free this variable */
    struct PamLine *lineObj;
    const char *module;
    const char *control;
    char *includeService = NULL;
    BOOLEAN sawNonincludeLine = FALSE;

    *configured = FALSE;

    if(line == -1)
    {
        /* This means that this service is not defined for this phase. An example is the session phase of passwd.
         * This function will return an error, and the parent function should decide whether or not to ignore the error.
         */
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE);
    }

    while(line != -1)
    {
        lineObj = &conf->lines[line];
        if(lineObj->module == NULL)
            module = "";
        else
            module = lineObj->module->value;

        if(lineObj->control == NULL)
            control = "";
        else
            control = lineObj->control->value;

        CT_SAFE_FREE_STRING(includeService);
        BAIL_ON_CENTERIS_ERROR(ceError = GetIncludeName(lineObj, &includeService));
        if(includeService != NULL)
        {
            ceError = IsLwidentityEnabled(conf, includeService, phase, configured);
            if(CENTERROR_IS_OK(ceError) && *configured)
                goto error;
            if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
                ceError = CENTERROR_SUCCESS;
            else if(ceError == CENTERROR_DOMAINJOIN_PAM_BAD_CONF)
                ceError = CENTERROR_SUCCESS;
            else
                sawNonincludeLine = TRUE;
            BAIL_ON_CENTERIS_ERROR(ceError);
        }
        else
            sawNonincludeLine = TRUE;

        if(PamModuleAlwaysDeniesDomainLogins(phase, module) && (
                    !strcmp(control, "required") ||
                    !strcmp(control, "requisite")))
            break;

        if(PamModuleIsLwidentity(phase, module))
        {
            DJ_LOG_INFO("Found pam_lwidentity");
            *configured = TRUE;
            break;
        }

        if(!strcmp(control, "sufficient") &&
                PamModuleGrants(phase, module) &&
                !PamModulePrompts(phase, module) &&
                !PamModuleChecksCaller(phase, module))
        {
            /* This module seems to let anyone through */
            break;
        }

        line = NextLineForService(conf, line, service, phase);
    }

    if(!sawNonincludeLine)
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE);

error:
    CT_SAFE_FREE_STRING(includeService);
    return ceError;
}

CENTERROR
DJCopyPamToRootDir(
        const char *srcPrefix,
        const char *destPrefix
        )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR srcPath = NULL;
    PSTR destPath = NULL;
    BOOLEAN exists;

    if(srcPrefix == NULL)
        srcPrefix = "";
    if(destPrefix == NULL)
        destPrefix = "";

    CT_SAFE_FREE_STRING(srcPath);
    GCE(ceError = CTAllocateStringPrintf(&srcPath, "%s/etc", srcPrefix));
    GCE(ceError = CTCheckDirectoryExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        GCE(ceError = CTAllocateStringPrintf(&destPath, "%s/etc", destPrefix));
        GCE(ceError = CTCreateDirectory(destPath, 0700));
    }

    CT_SAFE_FREE_STRING(srcPath);
    GCE(ceError = CTAllocateStringPrintf(&srcPath, "%s/etc/pam.d", srcPrefix));
    GCE(ceError = CTCheckDirectoryExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        GCE(ceError = CTAllocateStringPrintf(&destPath, "%s/etc/pam.d", destPrefix));
        GCE(ceError = CTCopyDirectory(srcPath, destPath));
    }

    CT_SAFE_FREE_STRING(srcPath);
    GCE(ceError = CTAllocateStringPrintf(&srcPath, "%s/etc/pam.conf", srcPrefix));
    GCE(ceError = CTCheckFileOrLinkExists(srcPath, &exists));
    if(exists)
    {
        CT_SAFE_FREE_STRING(destPath);
        GCE(ceError = CTAllocateStringPrintf(&destPath, "%s/etc/pam.conf", destPrefix));
        GCE(ceError = CTCopyFileWithOriginalPerms(srcPath, destPath));
    }

cleanup:
    CT_SAFE_FREE_STRING(srcPath);
    CT_SAFE_FREE_STRING(destPath);
    return ceError;
}

BOOLEAN IsRequiredService(PCSTR service, const struct PamConf *conf)
{
    int i;
    PCSTR requiredServices[] = {"ssh", "login", "su"};
    if(NextLineForService(conf, 0, "ssh", "auth") == -1)
    {
        requiredServices[0] = "sshd";
    }
    for(i = 0; i < sizeof(requiredServices)/sizeof(requiredServices[0]); i++)
    {
        if(NextLineForService(conf, 0, requiredServices[i], "auth") == -1)
        {
            requiredServices[i] = "other";
        }
    }
    for(i = 0; i < sizeof(requiredServices)/sizeof(requiredServices[0]); i++)
    {
        if(!strcmp(service, requiredServices[i]))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static QueryResult QueryPam(const JoinProcessOptions *options, LWException **exc)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LWException *nestedException = NULL;
    QueryResult result = NotConfigured;
    PSTR tempDir = NULL;
    struct PamConf conf;
    BOOLEAN configured;
    PCSTR services[] = {"ssh", "sshd", "login", "su", "other"};
    int i;

    memset(&conf, 0, sizeof(conf));
    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_CLEANUP_CTERR(exc, DJCopyPamToRootDir(NULL, tempDir));
    ceError = ReadPamConfiguration(tempDir, &conf);
    if(ceError == CENTERROR_INVALID_FILENAME)
    {
        result = NotApplicable;
        goto cleanup;
    }
    LW_CLEANUP_CTERR(exc, ceError);

    if(!options->joiningDomain)
    {
        LW_TRY(exc, DJUpdatePamConf(NULL, &conf, NULL, options->joiningDomain,
                &LW_EXC));
        if(conf.modified)
            goto cleanup;
        result = FullyConfigured;
        goto cleanup;
    }

    for(i = 0; i < sizeof(services)/sizeof(services[0]); i++)
    {
        PCSTR service = services[i];
        if(!IsRequiredService(service, &conf))
            continue;
        ceError = IsLwidentityEnabled(&conf, service, "auth", &configured);
        if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
        {
            configured = TRUE;
            ceError = CENTERROR_SUCCESS;
        }
        LW_CLEANUP_CTERR(exc, ceError);
        if(!configured)
            goto cleanup;

        ceError = IsLwidentityEnabled(&conf,
                    service, "account", &configured);
        if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
        {
            configured = TRUE;
            ceError = CENTERROR_SUCCESS;
        }
        LW_CLEANUP_CTERR(exc, ceError);
        if(!configured)
            goto cleanup;

        ceError = IsLwidentityEnabled(&conf,
                    service, "password", &configured);
        if(ceError == CENTERROR_DOMAINJOIN_PAM_MISSING_SERVICE)
        {
            configured = TRUE;
            ceError = CENTERROR_SUCCESS;
        }
        LW_CLEANUP_CTERR(exc, ceError);
        if(!configured)
            goto cleanup;
    }

    result = SufficientlyConfigured;
    DJUpdatePamConf(NULL, &conf, NULL, TRUE,
            &nestedException);
    if(!LW_IS_OK(nestedException) &&
            nestedException->code == CENTERROR_DOMAINJOIN_PAM_BAD_CONF)
    {
        //Eat this error because the configuration is sufficient
        conf.modified = TRUE;
        LW_HANDLE(&nestedException);
    }
    LW_CLEANUP(exc, nestedException);
    if(conf.modified)
        goto cleanup;

    result = FullyConfigured;

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    FreePamConfContents(&conf);
    LWHandle(&nestedException);
    return result;
}

static void DoPam(JoinProcessOptions *options, LWException **exc)
{
    DJNewConfigurePamForADLogin(NULL, options->warningCallback,
                options->joiningDomain, exc);
}

static PSTR GetPamDescription(const JoinProcessOptions *options, LWException **exc)
{
    PSTR tempDir = NULL;
    PSTR ret = NULL;
    PSTR diff = NULL;
    struct PamConf conf;

    memset(&conf, 0, sizeof(conf));

    LW_CLEANUP_CTERR(exc, CTCreateTempDirectory(&tempDir));
    LW_CLEANUP_CTERR(exc, DJCopyPamToRootDir(NULL, tempDir));

    LW_CLEANUP_CTERR(exc, ReadPamConfiguration(tempDir, &conf));

    LW_TRY(exc, DJUpdatePamConf(NULL, &conf, options->warningCallback,
        options->joiningDomain, &LW_EXC));

    if(conf.modified)
        WritePamConfiguration(tempDir, &conf, &diff);

    if(diff == NULL || strlen(diff) < 1)
    {
        LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf(
            &ret, "Fully configured"));
    }
    else
    {
        if(options->joiningDomain)
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"The ssh, su, and login services must list pam_lwidentity in their auth, account, and password phases. If this step is performed automatically, the local password policy module will be installed, and pam_lwidentity will be enabled for all services and all phases. Here is a list of the changes that would be made to the files if this configuration is performed automatically:\n%s", diff));
        }
        else
        {
            LW_CLEANUP_CTERR(exc, CTAllocateStringPrintf( &ret,
"All references to Likewise pam modules must be removed from pam.conf/pam.d. Otherwise, logins will break if these file are later removed from the system. Here is a list of changes that will be performed:\n%s", diff));
        }
    }

cleanup:
    if(tempDir != NULL)
    {
        CTRemoveDirectory(tempDir);
        CT_SAFE_FREE_STRING(tempDir);
    }
    CT_SAFE_FREE_STRING(diff);
    return ret;
}

const JoinModule DJPamModule = { TRUE, "pam", "configure pam.d/pam.conf", QueryPam, DoPam, GetPamDescription };
