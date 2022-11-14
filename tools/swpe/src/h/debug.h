#pragma once

/* controls result checking of DirectX functions */
/*#define DXDEBUG*/

#ifdef DEBUG
void __cdecl WriteToLogFunc(char *str, ...);
# define WriteToLog(a) WriteToLogFunc a
# define CheckResult(a) if (a) WriteToLog(("Call to " #a " succeeded")); else\
                       WriteToLog(("Call to " #a " failed"))
#else
# define WriteToLog(a) (void)0
# define CheckResult(a) a
#endif /* DEBUG */

#ifdef DXDEBUG
# define DXCheckResult(a) if (!a) WriteToLog(("Call to " #a " succeeded")); \
                         else WriteToLog(("Call to " #a " failed"))
# define DXWriteToLog(a) WriteToLogFunc a
#else
# define DXCheckResult(a) a
# define DXWriteToLog(a) (void)0
#endif /* DXDEBUG */

void OpenLogFile();
void CloseLogFile();
