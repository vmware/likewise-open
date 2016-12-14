#include "dcethread-debug.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef _WIN32
#include <Windows.h>
void
dcethread__output_debug_string_log_callback (const char* file, unsigned int line, int level, const char* str, void* data)
{
    const char* level_name = NULL;
    static char *fmtbuf = NULL;

    if (!fmtbuf)
    {
        fmtbuf = (char *) malloc(1024);
    }

    switch (level)
    {
    case DCETHREAD_DEBUG_ERROR:
        level_name = "ERROR";
        break;
    case DCETHREAD_DEBUG_WARNING:
        level_name = "WARNING";
        break;
    case DCETHREAD_DEBUG_INFO:
        level_name = "INFO";
        break;
    case DCETHREAD_DEBUG_VERBOSE:
        level_name = "VERBOSE";
        break;
    case DCETHREAD_DEBUG_TRACE:
        level_name = "TRACE";
        break;
    default:
        level_name = "UNKNOWN";
        break;
    }

    pthread_mutex_lock(&log_lock);
    _snprintf(fmtbuf, 1024, "dcethread-%s %s:%i: %s\n", level_name, file, line, str);
    OutputDebugStringA(fmtbuf);
    if (level == DCETHREAD_DEBUG_ERROR)
        abort();
    pthread_mutex_unlock(&log_lock);
}
#else

void
dcethread__default_log_callback (const char* file, unsigned int line, int level, const char* str, void* data)
{
    const char* level_name = NULL;

    switch (level)
    {
    case DCETHREAD_DEBUG_ERROR:
        level_name = "ERROR";
        break;
    case DCETHREAD_DEBUG_WARNING:
        level_name = "WARNING";
        break;
    case DCETHREAD_DEBUG_INFO:
        level_name = "INFO";
        break;
    case DCETHREAD_DEBUG_VERBOSE:
        level_name = "VERBOSE";
        break;
    case DCETHREAD_DEBUG_TRACE:
        level_name = "TRACE";
        break;
    default:
        level_name = "UNKNOWN";
        break;
    }

    pthread_mutex_lock(&log_lock);
    fprintf(stderr, "dcethread-%s %s:%i: %s\n", level_name, file, line, str);
    if (level == DCETHREAD_DEBUG_ERROR)
        abort();
    pthread_mutex_unlock(&log_lock);
}
#endif /* #ifdef _WIN32 */

static void (*log_callback) (const char* file, unsigned int line, int level, const char* str, void* data) = NULL;
static void *log_callback_data = NULL;

void
dcethread__debug_set_callback(void (*cb) (const char*, unsigned int, int, const char*, void* data), void* data)
{
    log_callback = cb;
    log_callback_data = data;
}

static char *
my_vasprintf(const char* format, va_list args)
{
    char *smallBuffer;
    unsigned int bufsize;
    int requiredLength;
    unsigned int newRequiredLength;
    char* outputString = NULL;
    va_list args2;

#ifndef _WIN32
    va_copy(args2, args);
#else
    /* TBD: Adam-This may not work properly with VS2008 */
    args2 = args;
#endif

    bufsize = 4;
    /* Use a small buffer in case libc does not like NULL */
    do
    {
        smallBuffer = malloc(bufsize);
	
	if (!smallBuffer)
	{
	    return NULL;
	}

        requiredLength = vsnprintf(smallBuffer, bufsize, format, args);
        if (requiredLength < 0)
        {
            bufsize *= 2;
        }
	free(smallBuffer);
    } while (requiredLength < 0);

    if (requiredLength >= (0xFFFFFFFF - 1))
    {
        return NULL;
    }

    outputString = malloc(requiredLength + 2);

    if (!outputString)
    {
	return NULL;
    }

    newRequiredLength = vsnprintf(outputString, requiredLength + 1, format, args2);
    if (newRequiredLength < 0)
    {
	free(outputString);
	return NULL;
    }

    va_end(args2);

    return outputString;
}

void
dcethread__debug_printf(const char* file, unsigned int line, int level, const char* fmt, ...)
{
    va_list ap;
    char* str;

    if (!log_callback)
	return;

    va_start(ap, fmt);

    str = my_vasprintf(fmt, ap);

    if (str)
    {
	log_callback(file, line, level, str, log_callback_data);
	free(str);
    }

    va_end(ap);
}
