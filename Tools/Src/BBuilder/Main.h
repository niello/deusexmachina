
// Main header

#define VERSION "3.0 alpha"

#define SEP_LINE "--------------------------------------------------------------------------------"

#define n_msg(Verbosity, String, ...) { if (Verbose >= Verbosity) n_printf(String, __VA_ARGS__); }
#define VR_ALWAYS	0
#define VR_ERROR	1
#define VR_WARNING	2
#define VR_INFO		3
#define VR_DETAILS	4
#define VR_DEBUG	5

#define EXIT_APP_FAIL	return ExitApp(false, WaitKey)
#define EXIT_APP_OK		return ExitApp(true, WaitKey)

int		Verbose = VR_ERROR;

int		ExitApp(bool NoError, bool WaitKey);
void	Release();
