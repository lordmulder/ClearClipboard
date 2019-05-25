/* ---------------------------------------------------------------------------------------------- */
/* Clear Clipboard                                                                                */
/* Copyright(c) 2019 LoRd_MuldeR <mulder2@gmx.de>                                                 */
/*                                                                                                */
/* Permission is hereby granted, free of charge, to any person obtaining a copy of this software  */
/* and associated documentation files (the "Software"), to deal in the Software without           */
/* restriction, including without limitation the rights to use, copy, modify, merge, publish,     */
/* distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the  */
/* Software is furnished to do so, subject to the following conditions:                           */
/*                                                                                                */
/* The above copyright notice and this permission notice shall be included in all copies or       */
/* substantial portions of the Software.                                                          */
/*                                                                                                */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING  */
/* BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND     */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   */
/* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, */
/* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.        */
/* ---------------------------------------------------------------------------------------------- */

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <shellapi.h>

#include "Version.h"

// Options
#define ENABLE_DEBUG_OUTPOUT 1
#define DEFAULT_TIMEOUT 30000U

// Const
#define MUTEX_NAME L"{E19E5CE1-5EF2-4C10-843D-E79460920A4A}"
#define CLASS_NAME L"{6D6CB8E6-BFEE-40A1-A6B2-2FF34C43F3F8}"
#define TIMER_UUID 0x5281CC36

// Debug output
#if defined(ENABLE_DEBUG_OUTPOUT) && ENABLE_DEBUG_OUTPOUT
#define PRINT(TEXT) do \
{ \
	if (g_debug) \
		OutputDebugStringA("ClearClipboard -- " TEXT "\n"); \
} \
while(0)
#else
#define PRINT(TEXT) __noop((X))
#endif

// Helper macro
#define ERROR_EXIT(X) do \
{ \
	result = (X); goto clean_up; \
} \
while(0)

// Global variables
static ULONGLONG g_tickCount = 0ULL;
static UINT g_timeout = DEFAULT_TIMEOUT;
#ifndef _DEBUG
static BOOL g_debug = FALSE;
#else
static const BOOL g_debug = TRUE;
#endif

// Forward declaration
static LRESULT CALLBACK my_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static BOOL clear_clipboard(void);
static UINT parse_arguments(const WCHAR *const command_line);
static BOOL update_autorun_entry(const BOOL remove);
static WCHAR *get_configuration_path(void);
static WCHAR *get_executable_path(void);

// Entry point function
#ifndef _DEBUG
extern IMAGE_DOS_HEADER __ImageBase;
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);
int startup(void)
{
	return wWinMain((HINSTANCE)&__ImageBase, NULL, GetCommandLineW(), SW_SHOWDEFAULT);
}
#endif //_DEBUG

// ==========================================================================
// MAIN
// ==========================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	int result = 0;
	UINT mode = 0U;
	HANDLE mutex = NULL;
	HWND hwnd = NULL;
	BOOL have_listener = FALSE, have_timer = FALSE;
	const WCHAR *config_path = NULL;
	WNDCLASSW wc;
	MSG msg;

	// Initialize variables
	SecureZeroMemory(&wc, sizeof(WNDCLASSW));
	SecureZeroMemory(&msg, sizeof(MSG));

	// Parse CLI arguments
	mode = parse_arguments(lpCmdLine);
	PRINT("ClearClipboard v" VERSION_STR);

	// Close running instances, if it was requested
	if((mode == 1U) || (mode == 2U))
	{
		PRINT("closing all running instances...");
		while(hwnd = FindWindowExW(HWND_MESSAGE, hwnd, CLASS_NAME, NULL))
		{
			PRINT("sending WM_CLOSE");
			SendMessageW(hwnd, WM_CLOSE, 0U, 0U);
		}
		if(mode == 1U)
		{
			PRINT("goodbye.");
			return 0;
		}
	}

	// Add or remove autorun entry, if it was requested
	if((mode == 3U) || (mode == 4U))
	{
		const BOOL success = update_autorun_entry(mode > 3U);
		PRINT("goodbye.");
		return success ? 0 : 1;
	}

	// Lock single instance mutex
	if(mutex = CreateMutexW(NULL, FALSE, MUTEX_NAME))
	{
		const DWORD ret = WaitForSingleObject(mutex, (mode > 1U) ? 5000U : 250U);
		if((ret != WAIT_OBJECT_0) && (ret != WAIT_ABANDONED))
		{
			PRINT("already running, exiting!");
			ERROR_EXIT(1);
		}
	}

	PRINT("starting up...");

	// Read configuration from INI
	if(config_path = get_configuration_path())
	{
		const UINT value = GetPrivateProfileInt(L"ClearClipboard", L"Timeout", DEFAULT_TIMEOUT, config_path);
		g_timeout = min(max(1000, value), USER_TIMER_MAXIMUM);
		LocalFree((HLOCAL)config_path);
	}

	// Register window class
	wc.lpfnWndProc   = my_wnd_proc;
	wc.hInstance     = hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = CLASS_NAME;
	if(!RegisterClassW(&wc))
	{
		PRINT("failed to register window class!");
		ERROR_EXIT(2);
	}

	// Create the message-only window
	if(!(hwnd = CreateWindowExW(0L, CLASS_NAME, L"ClearClipboard Window", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 0, 0, 0, 0, HWND_MESSAGE, 0, hInstance, NULL)))
	{
		PRINT("failed to create the window!");
		ERROR_EXIT(3);
	}

	// Add clipboard listener
	if(!(have_listener = AddClipboardFormatListener(hwnd)))
	{
		PRINT("failed to install clipboard listener!");
		ERROR_EXIT(4);
	}

	// Set up window timer
	g_tickCount = GetTickCount64();
	if(!(have_timer = SetTimer(hwnd, TIMER_UUID, min(max(g_timeout / 50U, USER_TIMER_MINIMUM), USER_TIMER_MAXIMUM), NULL)))
	{
		PRINT("failed to install the window timer!");
		ERROR_EXIT(5);
	}

	PRINT("clipboard monitoring started.");

	// Message loop
	while(GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	PRINT("shutting down now...");

clean_up:
	
	// Kill timer
	if(hwnd && have_timer)
	{
		KillTimer(hwnd, TIMER_UUID);
	}

	// Remove clipboard listener
	if(hwnd && have_listener)
	{
		RemoveClipboardFormatListener(hwnd);
	}

	// Close mutex
	if(mutex)
	{
		CloseHandle(mutex);
	}
	
	PRINT("goodbye.");
	return result;
}

// ==========================================================================
// Window Procedure
// ==========================================================================

static LRESULT CALLBACK my_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_CLIPBOARDUPDATE:
		PRINT("WM_CLIPBOARDUPDATE");
		g_tickCount = GetTickCount64();
		break;
	case WM_TIMER:
		PRINT("WM_TIMER");
		{
			const ULONGLONG tickCount = GetTickCount64();
			if((tickCount > g_tickCount) && ((tickCount - g_tickCount) > g_timeout))
			{
				if(clear_clipboard())
				{
					g_tickCount = tickCount;
				}
			}
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// ==========================================================================
// Clear Clipboard
// ==========================================================================

static BOOL clear_clipboard(void)
{
	int retry;
	BOOL success = FALSE;

	PRINT("clearing clipboard...");

	for(retry = 0; retry < 25; ++retry)
	{
		if(retry > 0)
		{
			PRINT("retry!");
			Sleep(1); /*yield*/
		}
		if(OpenClipboard(NULL))
		{
			success = EmptyClipboard();
			CloseClipboard();
		}
		if(success)
		{
			break; /*successful*/
		}
	}

	if(success)
	{
		PRINT("cleared.");
	}
	else
	{
		PRINT("failed to clean clipboard!");
	}

	return success;
}

// ==========================================================================
// Process CLI arguments
// ==========================================================================

static UINT parse_arguments(const WCHAR *const command_line)
{
	const WCHAR **argv;
	int i, argc;
	UINT mode = 0U;

	argv = CommandLineToArgvW(command_line, &argc);
	if(argv)
	{
		for(i = 1; i < argc; ++i)
		{
			if(!lstrcmpiW(argv[i], L"--close"))
			{
				mode = 1U;
			}
			else if(!lstrcmpiW(argv[i], L"--restart"))
			{
				mode = 2U;
			}
			else if(!lstrcmpiW(argv[i], L"--install"))
			{
				mode = 3U;
			}
			else if(!lstrcmpiW(argv[i], L"--uninstall"))
			{
				mode = 4U;
			}
#ifndef _DEBUG
			else if(!lstrcmpiW(argv[i], L"--debug"))
			{
				g_debug = TRUE;
			}
#endif //_DEBUG
		}
		LocalFree((HLOCAL)argv);
	}

	return mode;
}

// ==========================================================================
// Autorun support
// ==========================================================================

static BOOL update_autorun_entry(const BOOL remove)
{
	static const WCHAR *const REG_VALUE_NAME = L"com.muldersoft.clear_clipboard";
	HKEY hkey = NULL;
	BOOL success = FALSE;
	HRESULT ret;
	
	if(RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0U, NULL, 0U, KEY_WRITE, NULL, &hkey, NULL) != ERROR_SUCCESS)
	{
		PRINT("failed to open registry key!");
		return FALSE;
	}

	if(!remove)
	{
		const WCHAR *const executable_path = get_executable_path();
		if(executable_path)
		{
			PRINT("adding autorun entry to registry...");
			if(RegSetKeyValueW(hkey, NULL, REG_VALUE_NAME, REG_SZ, executable_path, (lstrlenW(executable_path) + 1U) * sizeof(WCHAR)) == ERROR_SUCCESS)
			{
				PRINT("succeeded.");
				success = TRUE;
			}
			else
			{
				PRINT("failed to add autorun entry to registry!");
			}
			LocalFree((HLOCAL)executable_path);
		}
		else
		{
			PRINT("failed to determine executable path!");
		}
	}
	else
	{
		PRINT("removing autorun entry from registry...");
		if((ret = RegDeleteKeyValueW(hkey, NULL, REG_VALUE_NAME)) == ERROR_SUCCESS)
		{
			PRINT("succeeded.");
			success = TRUE;
		}
		else
		{
			if(ret != ERROR_FILE_NOT_FOUND)
			{
				PRINT("failed to remove autorun entry from registry!");
			}
			else
			{
				PRINT("autorun entry does not exist.");
				success = TRUE;
			}
		}
	}

	RegCloseKey(hkey);
	return success;
}

// ==========================================================================
// File path routines
// ==========================================================================

static WCHAR *get_configuration_path(void)
{
	static const WCHAR *const DEFAULT_PATH = L"ClearClipboard.ini";
	WCHAR *buffer = NULL;
	WCHAR *const path = get_executable_path();

	if(path)
	{
		const DWORD path_len = lstrlenW(path);
		if(path_len > 1U)
		{
			DWORD pos, last_sep = 0U;
			for(pos = 0U; pos < path_len; ++pos)
			{
				if((path[pos] == L'/') || (path[pos] == L'\\') || (path[pos] == L'.'))
				{
					last_sep = pos;
				}
			}
			if(last_sep > 1U)
			{
				const DWORD copy_len = (path[last_sep] == '.') ? last_sep : path_len;
				buffer = (WCHAR*) LocalAlloc(LPTR, (5U + copy_len) * sizeof(WCHAR));
				if(buffer)
				{
					lstrcpynW(buffer, path, last_sep + 1U);
					lstrcatW(buffer, L".ini");
				}
			}
		}
		LocalFree((HLOCAL)path);
	}

	if(!buffer)
	{
		buffer = (WCHAR*) LocalAlloc(LPTR, (1U + lstrlenW(DEFAULT_PATH)) * sizeof(WCHAR));
		if(buffer)
		{
			lstrcpyW(buffer, DEFAULT_PATH);
		}
	}

	return buffer;
}

static WCHAR *get_executable_path(void)
{
	DWORD size = 256U;

	WCHAR *buffer = (WCHAR*) LocalAlloc(LPTR, size * sizeof(WCHAR));
	if(!buffer)
	{
		return NULL; /*malloc failed*/
	}

	for(;;)
	{
		const DWORD result = GetModuleFileNameW(NULL, buffer, size);
		if((result > 0) && (result < size))
		{
			return buffer;
		}
		if((size < MAXWORD) && (result >= size))
		{
			LocalFree((HLOCAL)buffer);
			size *= 2U;
			if(!(buffer = (WCHAR*) LocalAlloc(LPTR, size * sizeof(WCHAR))))
			{
				return NULL; /*malloc failed*/
			}
		}
		else
		{
			break; /*something else went wrong*/
		}
	}

	LocalFree((HLOCAL)buffer);
	return NULL;
}
