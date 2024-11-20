typedef enum
{
    INFO,
    ERROR,
    VERBOSE,
    WARNING
} DebugLevel;

void debugLog(DebugLevel level, const char *fmt, ...);
void debugLogDone();
void closeDebugLog();
void debug(DebugLevel level, const char *fmt, ...);
void debugDone();