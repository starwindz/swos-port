#include "log.h"

void initLog() {}
void flushLog() {}
void finishLog() {}
void log(LogCategory category, const char *format, ...) {}
void logv(LogCategory category, const char *format, va_list args) {}
std::string logPath() { return {}; }
