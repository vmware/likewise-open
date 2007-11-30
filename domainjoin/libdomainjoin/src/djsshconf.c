/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007.  
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include "domainjoin.h"
#include "ctarray.h"
#include <ctype.h>

#if !HAVE_DECL_ISBLANK
int isblank(int c);
#endif

struct SshLine {
	char *leadingWhiteSpace;
	char *optionName;
	char *separatingWhiteSpace;
	char *optionValue;
	/* Includes comment */
	char *trailingWhiteSpace;
};

struct SshConf {
	char *filename;
	struct SshLine *lines;
	int lineCount;
	DynamicArray private_data;
	int modified;
};

static int FindOption(struct SshConf *conf, int startLine, const char *name);

static CENTERROR SetOption(struct SshConf *conf, const char *name,
			   const char *value);

static CENTERROR WriteSshConfiguration(const char *rootPrefix,
				       struct SshConf *conf);

static void UpdatePublicLines(struct SshConf *conf)
{
	conf->lines = conf->private_data.data;
	conf->lineCount = conf->private_data.size;
}

static CENTERROR GetLineStrings(char ***dest, struct SshLine *line,
				int *destSize)
{
	if (*destSize < 5)
		return CENTERROR_OUT_OF_MEMORY;
	dest[0] = &line->leadingWhiteSpace;
	dest[1] = &line->optionName;
	dest[2] = &line->separatingWhiteSpace;
	dest[3] = &line->optionValue;
	dest[4] = &line->trailingWhiteSpace;
	*destSize = 5;
	return CENTERROR_SUCCESS;
}

static void FreeSshLineContents(struct SshLine *line)
{
	char **strings[5];
	int stringCount = 5;
	int i;

	GetLineStrings(strings, line, &stringCount);
	for (i = 0; i < stringCount; i++) {
		CT_SAFE_FREE_STRING(*strings[i]);
	}
}

static int FindOption(struct SshConf *conf, int startLine, const char *name)
{
	int i;
	if (startLine == -1)
		return -1;
	for (i = startLine; i < conf->lineCount; i++) {
		if (conf->lines[i].optionName != NULL &&
		    !strcmp(conf->lines[i].optionName, name)) {
			return i;
		}
	}
	return -1;
}

/* Get the printed form of a line from the parsed form by concatenating all of the strings together */
static CENTERROR GetPrintedLine(DynamicArray * dest, struct SshConf *conf,
				int line)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	size_t len = 0;
	char **strings[5];
	size_t stringLengths[5];
	size_t pos;
	int stringCount = 5;
	int i;

	GetLineStrings(strings, &conf->lines[line], &stringCount);
	for (i = 0; i < stringCount; i++) {
		stringLengths[i] = strlen(*strings[i]);
		len += stringLengths[i];
	}
	//For the terminating NULL
	len++;

	if (len > dest->capacity)
		BAIL_ON_CENTERIS_ERROR(ceError = CTSetCapacity(dest, 1, len));
	pos = 0;
	for (i = 0; i < stringCount; i++) {
		memcpy((char *)dest->data + pos, *strings[i], stringLengths[i]);
		pos += stringLengths[i];
	}
	((char *)dest->data)[pos] = '\0';
	dest->size = len;

      error:
	return ceError;
}

static CENTERROR RemoveLine(struct SshConf *conf, int *line)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTArrayRemove(&conf->private_data, *line,
					     sizeof(struct SshLine), 1));
	UpdatePublicLines(conf);
	conf->modified = 1;

	if (*line >= conf->lineCount)
		*line = -1;

      error:
	return ceError;
}

static CENTERROR RemoveOption(struct SshConf *conf, const char *name)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int line;

	for (line = 0; line < conf->lineCount; line++) {
		line = FindOption(conf, line, name);
		if (line == -1)
			break;

		BAIL_ON_CENTERIS_ERROR(ceError = RemoveLine(conf, &line));
		if (line > 0)
			line--;
	}

      error:
	UpdatePublicLines(conf);
	return ceError;
}

/* Copy a ssh configuration line and add it below the old line. */
static CENTERROR SetOption(struct SshConf *conf, const char *name,
			   const char *value)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int line = -1;
	DynamicArray printedLine;
	struct SshLine lineObj;
	int found = 0;

	memset(&lineObj, 0, sizeof(struct SshLine));
	memset(&printedLine, 0, sizeof(printedLine));

	for (line = 0; line < conf->lineCount; line++) {
		line = FindOption(conf, line, name);
		if (line == -1)
			break;
		found++;
		if (!strcmp(conf->lines[line].optionValue, value))
			continue;

		//Insert a commented out version of the line
		BAIL_ON_CENTERIS_ERROR(ceError =
				       GetPrintedLine(&printedLine, conf,
						      line));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup("",
						&lineObj.leadingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup("", &lineObj.optionName));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup("",
						&lineObj.separatingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup("", &lineObj.optionValue));
		CTAllocateStringPrintf(&lineObj.trailingWhiteSpace,
				       "#Overwritten by lwidentity: %s",
				       printedLine.data);
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTArrayInsert(&conf->private_data, line,
						     sizeof(struct SshLine),
						     &lineObj, 1));
		memset(&lineObj, 0, sizeof(lineObj));
		conf->modified = 1;
		line++;

		//Change the option value of the line
		CT_SAFE_FREE_STRING(conf->lines[line].optionValue);
		BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(value,
							  &conf->lines[line].
							  optionValue));
	}

	/*If the option wasn't already in the file, search for comments that
	   mention the option, and insert the line after the comment */
	for (line = 0; !found && line < conf->lineCount; line++) {
		if (strstr(conf->lines[line].trailingWhiteSpace, name) == NULL)
			continue;

		BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("",
							  &lineObj.
							  leadingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup(name, &lineObj.optionName));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup(" ",
						&lineObj.separatingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup(value, &lineObj.optionValue));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup("",
						&lineObj.trailingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTArrayInsert(&conf->private_data,
						     line + 1,
						     sizeof(struct SshLine),
						     &lineObj, 1));
		memset(&lineObj, 0, sizeof(lineObj));
		conf->modified = 1;
		found++;
	}

	/*If the option wasn't even in a comment, just add the option at the
	   end of the file
	 */
	if (!found) {
		BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup("",
							  &lineObj.
							  leadingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup(name, &lineObj.optionName));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup(" ",
						&lineObj.separatingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup(value, &lineObj.optionValue));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTStrdup("",
						&lineObj.trailingWhiteSpace));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTArrayAppend(&conf->private_data,
						     sizeof(struct SshLine),
						     &lineObj, 1));
		memset(&lineObj, 0, sizeof(lineObj));
		conf->modified = 1;
	}

      error:
	UpdatePublicLines(conf);
	FreeSshLineContents(&lineObj);
	CTArrayFree(&printedLine);
	return ceError;
}

static CENTERROR AddFormattedLine(struct SshConf *conf, const char *filename,
				  const char *linestr, const char **endptr)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct SshLine lineObj;
	const char *pos = linestr;
	const char *token_start = NULL;

	memset(&lineObj, 0, sizeof(lineObj));

	/* Find the leading whitespace in the line */
	token_start = pos;
	while (isblank(*pos))
		pos++;
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTStrndup(token_start, pos - token_start,
					 &lineObj.leadingWhiteSpace));

	/* Find the option name in the line */
	token_start = pos;
	while (!isspace(*pos) && *pos != '\0' && *pos != '#') {
		pos++;
	}
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTStrndup(token_start, pos - token_start,
					 &lineObj.optionName));

	/* Find the blank space separating the option name and option value */
	token_start = pos;
	while (isblank(*pos))
		pos++;
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTStrndup(token_start, pos - token_start,
					 &lineObj.separatingWhiteSpace));

	/* Find the option value */
	token_start = pos;
	//This token can contain spaces and tabs
	while (*pos != '\0' && *pos != '#' && *pos != '\n' && *pos != '\r')
		pos++;
	//But not at the end, so trim off the trailing white space
	while (pos > token_start && isblank(*pos))
		pos--;
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTStrndup(token_start, pos - token_start,
					 &lineObj.optionValue));

	/* Find the line's trailing whitespace and comment */
	token_start = pos;
	while (*pos != '\0' && *pos != '\n' && *pos != '\r')
		pos++;
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTStrndup(token_start, pos - token_start,
					 &lineObj.trailingWhiteSpace));

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTArrayAppend(&conf->private_data,
					     sizeof(lineObj), &lineObj, 1));
	memset(&lineObj, 0, sizeof(lineObj));
	UpdatePublicLines(conf);

	if (endptr != NULL)
		*endptr = pos;
	conf->modified = 1;

      error:
	FreeSshLineContents(&lineObj);
	return ceError;
}

static void FreeSshConfContents(struct SshConf *conf)
{
	int i;
	for (i = 0; i < conf->lineCount; i++) {
		FreeSshLineContents(&conf->lines[i]);
	}
	CTArrayFree(&conf->private_data);
	UpdatePublicLines(conf);
	CT_SAFE_FREE_STRING(conf->filename);
}

static CENTERROR ReadSshFile(struct SshConf *conf, const char *rootPrefix,
			     const char *filename)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	FILE *file = NULL;
	char buffer[1024];
	char *fullPath = NULL;
	BOOLEAN endOfFile = FALSE;
	BOOLEAN exists;

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTAllocateStringPrintf(&fullPath, "%s%s",
						      rootPrefix, filename));
	DJ_LOG_INFO("Reading ssh file %s", fullPath);
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTCheckFileOrLinkExists(fullPath, &exists));
	if (!exists) {
		DJ_LOG_INFO("File %s does not exist", fullPath);
		ceError = CENTERROR_INVALID_FILENAME;
		goto error;
	}

	BAIL_ON_CENTERIS_ERROR(ceError = CTStrdup(filename, &conf->filename));
	BAIL_ON_CENTERIS_ERROR(ceError = CTOpenFile(fullPath, "r", &file));
	CT_SAFE_FREE_STRING(fullPath);
	while (TRUE) {
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTReadNextLine(file, buffer,
						      sizeof(buffer),
						      &endOfFile));
		if (endOfFile)
			break;
		BAIL_ON_CENTERIS_ERROR(ceError =
				       AddFormattedLine(conf, filename, buffer,
							NULL));
	}
	BAIL_ON_CENTERIS_ERROR(ceError = CTCloseFile(file));
	file = NULL;

	return ceError;

      error:
	if (file != NULL)
		CTCloseFile(file);
	CT_SAFE_FREE_STRING(fullPath);
	FreeSshConfContents(conf);
	return ceError;
}

static CENTERROR WriteSshConfiguration(const char *rootPrefix,
				       struct SshConf *conf)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	DynamicArray printedLine;
	int i;
	char *tempName = NULL;
	char *finalName = NULL;
	FILE *file = NULL;
	memset(&printedLine, 0, sizeof(printedLine));

	DJ_LOG_INFO("Writing ssh configuration for %s", conf->filename);

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTAllocateStringPrintf(&tempName, "%s%s.new",
						      rootPrefix,
						      conf->filename));
	ceError = CTOpenFile(tempName, "w", &file);
	if (!CENTERROR_IS_OK(ceError)) {
		DJ_LOG_ERROR("Unable to open '%s' for writing", tempName);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	for (i = 0; i < conf->lineCount; i++) {
		BAIL_ON_CENTERIS_ERROR(ceError =
				       GetPrintedLine(&printedLine, conf, i));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTFilePrintf(file, "%s\n",
						    printedLine.data));
	}

	BAIL_ON_CENTERIS_ERROR(ceError = CTCloseFile(file));
	file = NULL;
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTAllocateStringPrintf(&finalName, "%s%s",
						      rootPrefix,
						      conf->filename));
	BAIL_ON_CENTERIS_ERROR(ceError = CTBackupFile(finalName));
	BAIL_ON_CENTERIS_ERROR(ceError = CTMoveFile(tempName, finalName));
	DJ_LOG_INFO("File moved into place");

      error:
	if (file != NULL)
		CTCloseFile(file);
	CTArrayFree(&printedLine);
	CT_SAFE_FREE_STRING(tempName);
	CT_SAFE_FREE_STRING(finalName);
	return ceError;
}

CENTERROR DJConfigureSshForADLogin(const char *testPrefix, BOOLEAN enable)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct SshConf conf;
	int i;
	/* Mac OS X stores the configuration in /etc */
	const char *configPaths[] =
	    { "/etc/ssh", "/opt/ssh/etc", "/usr/local/etc",
		"/etc", NULL
	};
	/* Solaris Sparc 10 stores sshd in /usr/lib/ssh */
	const char *binaryPaths[] = { "/usr/sbin", "/opt/ssh/sbin",
		"/usr/local/sbin", "/usr/bin", "/opt/ssh/bin", "/usr/local/bin",
		"/usr/lib/ssh", NULL
	};
	char *configPath = NULL;
	char *binaryPath = NULL;
	char *command = NULL;
	char *commandOutput = NULL;
	BOOLEAN exists;
	const char *sshdOptions[] = { "GSSAPIAuthentication",
		"GSSAPICleanupCredentials", "ChallengeResponseAuthentication",
		"UsePAM", NULL
	};
	const char *sshOptions[] = { "GSSAPIAuthentication",
		"GSSAPIDelegateCredentials", NULL
	};

	if (testPrefix == NULL)
		testPrefix = "";
	memset(&conf, 0, sizeof(conf));

	for (i = 0; configPaths[i] != NULL && conf.filename == NULL; i++) {
		CT_SAFE_FREE_STRING(configPath);
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTAllocateStringPrintf(&configPath,
							      "%s/%s",
							      configPaths[i],
							      "sshd_config"));
		ceError = ReadSshFile(&conf, testPrefix, configPath);
		if (ceError == CENTERROR_INVALID_FILENAME)
			ceError = CENTERROR_SUCCESS;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	exists = FALSE;
	for (i = 0; binaryPaths[i] != NULL && !exists; i++) {
		CT_SAFE_FREE_STRING(binaryPath);
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTAllocateStringPrintf(&binaryPath,
							      "%s%s/%s",
							      testPrefix,
							      binaryPaths[i],
							      "sshd"));
		DJ_LOG_INFO("Checking if %s exists", binaryPath);
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTCheckFileOrLinkExists(binaryPath,
							       &exists));
	}
	if (!exists) {
		if (conf.filename != NULL) {
			DJ_LOG_ERROR
			    ("Found the sshd_config, but not the sshd binary");
			BAIL_ON_CENTERIS_ERROR(ceError =
					       CENTERROR_INVALID_FILENAME);
		} else
			DJ_LOG_INFO
			    ("Sshd not found on system, skipping configuration");
	} else {
		DJ_LOG_INFO("Found binary %s", binaryPath);
		if (conf.filename == NULL) {
			DJ_LOG_ERROR
			    ("Found the sshd binary, but not the sshd config");
			BAIL_ON_CENTERIS_ERROR(ceError =
					       CENTERROR_INVALID_FILENAME);
		}
	}

	if (exists) {
		for (i = 0; sshdOptions[i] != NULL; i++) {
			if (!enable) {
				if (!strcmp
				    (sshdOptions[i], "GSSAPIAuthentication")) {
					BAIL_ON_CENTERIS_ERROR(ceError =
							       RemoveOption
							       (&conf,
								sshdOptions
								[i]));
				}
				continue;
			}
			DJ_LOG_INFO("Testing option %s", sshdOptions[i]);
			/* Most versions of sshd support the -t option which runs it in test
			   mode. Test mode is used to verify that a config file is correct, or
			   in our case that the passed options are valid.

			   The only version of sshd known to not support -t is the version that
			   comes with Solaris 9. However, this version does not support the -o
			   option, and it will error out if the -o option is used. The Solaris
			   9 version of sshd does not support any of the options we'd like to
			   enable, so it will correctly fail all of the option tests.

			   Sshd will either complain about the first invalid option that is
			   passed with -o, or it will complain about all invalid options. -o
			   BadOption=yes is passed to verify that sshd understands -o, and to
			   make doubly sure that it will not start listening on a port.
			 */
			CT_SAFE_FREE_STRING(command);
			BAIL_ON_CENTERIS_ERROR(ceError =
					       CTAllocateStringPrintf(&command,
								      "%s -t -o %s=yes -o BadOption=yes 2>&1",
								      binaryPath,
								      sshdOptions
								      [i]));

			CT_SAFE_FREE_STRING(commandOutput);
			ceError = CTCaptureOutput(command, &commandOutput);
			/* Some versions of sshd will return an error code because an invalid
			   option was passed, but not all will. */
			if (ceError == CENTERROR_COMMAND_FAILED)
				ceError = CENTERROR_SUCCESS;
			BAIL_ON_CENTERIS_ERROR(ceError);

			if (strstr(commandOutput, sshdOptions[i]) != NULL) {
				DJ_LOG_INFO("Option %s not supported",
					    sshdOptions[i]);
				continue;
			}

			if (strstr(commandOutput, "BadOption") == NULL) {
				DJ_LOG_INFO("Sshd does not support -o");
				break;
			}

			DJ_LOG_INFO("Option %s supported", sshdOptions[i]);
			BAIL_ON_CENTERIS_ERROR(ceError =
					       SetOption(&conf, sshdOptions[i],
							 "yes"));
		}

		if (conf.modified)
			WriteSshConfiguration(testPrefix, &conf);
		else
			DJ_LOG_INFO("sshd_config not modified");
		FreeSshConfContents(&conf);
	}

	/* Configuring ssh */
	for (i = 0; configPaths[i] != NULL && conf.filename == NULL; i++) {
		CT_SAFE_FREE_STRING(configPath);
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTAllocateStringPrintf(&configPath,
							      "%s/%s",
							      configPaths[i],
							      "ssh_config"));
		ceError = ReadSshFile(&conf, testPrefix, configPath);
		if (ceError == CENTERROR_INVALID_FILENAME)
			ceError = CENTERROR_SUCCESS;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	exists = FALSE;
	for (i = 0; binaryPaths[i] != NULL && !exists; i++) {
		CT_SAFE_FREE_STRING(binaryPath);
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTAllocateStringPrintf(&binaryPath,
							      "%s%s/%s",
							      testPrefix,
							      binaryPaths[i],
							      "ssh"));
		DJ_LOG_INFO("Checking if %s exists", binaryPath);
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTCheckFileOrLinkExists(binaryPath,
							       &exists));
	}
	if (!exists) {
		if (conf.filename != NULL) {
			DJ_LOG_ERROR
			    ("Found the ssh_config, but not the ssh binary");
			BAIL_ON_CENTERIS_ERROR(ceError =
					       CENTERROR_INVALID_FILENAME);
		} else
			DJ_LOG_INFO
			    ("Ssh not found on system, skipping configuration");
	} else {
		DJ_LOG_INFO("Found binary %s", binaryPath);
		if (conf.filename == NULL) {
			DJ_LOG_ERROR
			    ("Found the ssh binary, but not the ssh config");
			BAIL_ON_CENTERIS_ERROR(ceError =
					       CENTERROR_INVALID_FILENAME);
		}
	}

	if (exists) {
		for (i = 0; sshOptions[i] != NULL; i++) {
			if (!enable) {
				if (!strcmp
				    (sshOptions[i], "GSSAPIAuthentication")) {
					BAIL_ON_CENTERIS_ERROR(ceError =
							       RemoveOption
							       (&conf,
								sshOptions[i]));
				}
				continue;
			}
			DJ_LOG_INFO("Testing option %s", sshOptions[i]);
			/* Passing -v to ssh will cause it to print out its version number and
			   not attempt to connect to a machine. All versions of ssh seem to
			   parse the options passed with -o even when -v is passed.

			   Ssh will either complain about the first invalid option that is
			   passed with -o, or it will complain about all invalid options. -o
			   BadOption=yes is passed to verify that ssh understands -o.
			 */
			CT_SAFE_FREE_STRING(command);
			BAIL_ON_CENTERIS_ERROR(ceError =
					       CTAllocateStringPrintf(&command,
								      "%s -v -o %s=yes -o BadOption=yes 2>&1",
								      binaryPath,
								      sshOptions
								      [i]));

			CT_SAFE_FREE_STRING(commandOutput);
			ceError = CTCaptureOutput(command, &commandOutput);
			/* All known versions of ssh will return an error code, either because
			   -v was passed, or because of the bad option. */
			if (ceError == CENTERROR_COMMAND_FAILED)
				ceError = CENTERROR_SUCCESS;
			BAIL_ON_CENTERIS_ERROR(ceError);

			if (strstr(commandOutput, sshOptions[i]) != NULL) {
				DJ_LOG_INFO("Option %s not supported",
					    sshOptions[i]);
				continue;
			}

			if (strstr(commandOutput, "BadOption") == NULL) {
				DJ_LOG_INFO("Ssh does not support -o");
				break;
			}

			DJ_LOG_INFO("Option %s supported", sshOptions[i]);
			BAIL_ON_CENTERIS_ERROR(ceError =
					       SetOption(&conf, sshOptions[i],
							 "yes"));
		}

		if (conf.modified)
			WriteSshConfiguration(testPrefix, &conf);
		else
			DJ_LOG_INFO("ssh_config not modified");
		FreeSshConfContents(&conf);
	}

      error:
	FreeSshConfContents(&conf);
	CT_SAFE_FREE_STRING(configPath);
	CT_SAFE_FREE_STRING(binaryPath);
	CT_SAFE_FREE_STRING(command);
	CT_SAFE_FREE_STRING(commandOutput);
	return ceError;
}
