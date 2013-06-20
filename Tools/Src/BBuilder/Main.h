#include <ConsoleApp.h>

// Main header

#define VERSION "3.0 alpha"

#define EXIT_APP_FAIL	return ExitApp(false, WaitKey)
#define EXIT_APP_OK		return ExitApp(true, WaitKey)

int ExitApp(bool NoError, bool WaitKey);
