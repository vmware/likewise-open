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

#ifndef __CT_LOGGER_H__
#define __CT_LOGGER_H__

#include <ctstatus.h>
#include <stdarg.h>

#define CT_LOG_WRAP_BOOL(x)   ((x) ? 'Y' : 'N')
#define CT_LOG_WRAP_STRING(x) ((x) ? (x) : "(null)")



/*
 * The logging interface presented here is process-global because it
 * can use syslog, which is process-global.  In the future, it may be
 * desirable to add the ability to have a handle-based log too.
 */

/*
 * Log levels
 */
typedef uint32_t CT_LOG_LEVEL;

#define CT_LOG_LEVEL_ALWAYS   0
#define CT_LOG_LEVEL_CRITICAL 1
#define CT_LOG_LEVEL_ERROR    2
#define CT_LOG_LEVEL_WARN     3
#define CT_LOG_LEVEL_NOTICE   4
#define CT_LOG_LEVEL_INFO     5
#define CT_LOG_LEVEL_VERBOSE  6
#define CT_LOG_LEVEL_DEBUG    7
#define CT_LOG_LEVEL_TRACE    8
#define CT_LOG_LEVEL_MAX      8

extern CT_LOG_LEVEL _gCtLoggerLogLevel;

/****************************************************************************/
/* Global Log Functions */
/****************************************************************************/

CT_STATUS
CtLoggerSyslogOpen(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Identifier,
    IN int Option,
    IN int Facility
    );

CT_STATUS
CtLoggerFileOpen(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Path
    );

CT_STATUS
CtLoggerSetLogLevel(
    IN CT_LOG_LEVEL LogLevel
    );

void
CtLoggerLogMessageV(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN va_list Args
    );

void
CtLoggerLogMessage(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN ...
    );

void
CtLoggerClose(
    );

/****************************************************************************/
/* Handle-Based Log Functions */
/****************************************************************************/

struct _CT_LOG_HANDLE_DATA;
typedef struct _CT_LOG_HANDLE_DATA *CT_LOG_HANDLE, **PCT_LOG_HANDLE;

CT_STATUS
CtLoggerOpenHandle(
    OUT PCT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Path
    );

CT_STATUS
CtLoggerSetLogLevelHandle(
    IN CT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel
    );

void
CtLoggerLogMessageHandleV(
    IN CT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN va_list Args
    );

void
CtLoggerLogMessageHandle(
    IN CT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN ...
    );

void
CtLoggerCloseHandle(
    IN OUT CT_LOG_HANDLE Handle
    );

/****************************************************************************/
/* Global Log Macros */
/****************************************************************************/

#define _CT_LOG_INTERNAL(Level, Format, ...) \
    do { \
        if (_gCtLoggerLogLevel >= Level ) \
        { \
            if (Level > CT_LOG_LEVEL_DEBUG) \
            { \
                CtLoggerLogMessage(Level, "%s() " Format, __FUNCTION__, ## __VA_ARGS__); \
            } \
            else \
            { \
                CtLoggerLogMessage(Level, Format, ## __VA_ARGS__); \
            } \
        } \
    } while (0)

#define CT_LOG_ALWAYS(Format, ...) \
    CtLoggerLogMessage(CT_LOG_LEVEL_ALWAYS, Format, ## __VA_ARGS__)

#define CT_LOG_CRITICAL(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_CRITICAL, Format, ## __VA_ARGS__)

#define CT_LOG_ERROR(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_ERROR, Format, ## __VA_ARGS__)

#define CT_LOG_WARN(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_WARN, Format, ## __VA_ARGS__)

#define CT_LOG_NOTICE(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_NOTICE, Format, ## __VA_ARGS__)

#define CT_LOG_INFO(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_INFO, Format, ## __VA_ARGS__)

#define CT_LOG_VERBOSE(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_VERBOSE, Format, ## __VA_ARGS__)

#define CT_LOG_DEBUG(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_DEBUG, Format, ## __VA_ARGS__)

#define CT_LOG_TRACE(Format, ...) \
    _CT_LOG_INTERNAL(CT_LOG_LEVEL_TRACE, Format, ## __VA_ARGS__)

/****************************************************************************/
/* Handle-Based Log Macros Functions */
/****************************************************************************/

/* TODO - may want to expose log level in handle for faster check */

#define _CT_LOG_H_INTERNAL(Handle, Level, Format, ...) \
    do { \
        CtLoggerLogMessageHandle(Handle, Level, Format, ## __VA_ARGS__); \
    } while (0)

#define CT_LOG_H_ALWAYS(Handle, Format, ...) \
    CtLoggerLogMessageHandle(Handle, CT_LOG_LEVEL_ALWAYS, Format, ## __VA_ARGS__)

#define CT_LOG_H_CRITICAL(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_CRITICAL, Format, ## __VA_ARGS__)

#define CT_LOG_H_ERROR(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_ERROR, Format, ## __VA_ARGS__)

#define CT_LOG_H_WARN(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_WARN, Format, ## __VA_ARGS__)

#define CT_LOG_H_NOTICE(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_NOTICE, Format, ## __VA_ARGS__)

#define CT_LOG_H_INFO(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_INFO, Format, ## __VA_ARGS__)

#define CT_LOG_H_VERBOSE(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_VERBOSE, Format, ## __VA_ARGS__)

#define CT_LOG_H_DEBUG(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_DEBUG, Format, ## __VA_ARGS__)

#define CT_LOG_H_TRACE(Handle, Format, ...) \
    _CT_LOG_H_INTERNAL(Handle, CT_LOG_LEVEL_TRACE, Format, ## __VA_ARGS__)

#if defined(DEBUG) || defined(_DEBUG)
#define CT_ASSERT(x) do { if(!(x)) { CT_LOG_ERROR("ASSERT on line %d of file %s failed.\n", __LINE__, __FILE__); abort(); } while(0)
#else
#define CT_ASSERT(x) do { } while(0)
#endif

#endif /* __CT_LOGGER_H__ */
