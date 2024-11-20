#include "debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

static FILE *logfile = NULL;

char *levelString(DebugLevel level)
{
    switch (level)
    {
    case INFO:
        return "   INFO";
    case ERROR:
        return "  ERROR";
    case VERBOSE:
        return "VERBOSE";
    case WARNING:
        return "WARNING";
    }
    return "UNKNOWN";
}

void debugLog(DebugLevel level, const char *fmt, ...)
{
    if (logfile == NULL)
    {
        logfile = fopen("debug.log", "w");
        setvbuf(logfile, NULL, _IONBF, 0);
    }

    va_list args;
    va_start(args, fmt);
    char logtime[9];
    time_t rawtime;
    struct tm *curtime;
    time(&rawtime);
    curtime = localtime(&rawtime);
    strftime(logtime, sizeof(logtime), "%H:%M:%S", curtime);
    fprintf(logfile, "%s %s | ", logtime, levelString(level));
    vfprintf(logfile, fmt, args);
    fprintf(logfile, "\n");
    va_end(args);
}

void debugLogDone()
{
    if (logfile != NULL)
        fprintf(logfile, "done\n");
}

void closeDebugLog()
{
    if (logfile != NULL)
        fclose(logfile);
}

void debug(DebugLevel level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char logtime[9];
    time_t rawtime;
    struct tm *curtime;
    time(&rawtime);
    curtime = localtime(&rawtime);
    strftime(logtime, sizeof(logtime), "%H:%M:%S", curtime);
    printf("%s %s | ", logtime, levelString(level));
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void debugDone() {
    printf("done\n");
}