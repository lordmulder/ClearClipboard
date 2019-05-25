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

#define ENABLE_DEBUG_OUTPOUT 1
#define DEFAULT_TIMEOUT 30000U

#define MUTEX_NAME L"{E19E5CE1-5EF2-4C10-843D-E79460920A4A}"
#define CLASS_NAME L"{6D6CB8E6-BFEE-40A1-A6B2-2FF34C43F3F8}"
#define TIMER_UUID 0x5281CC36

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

#define ERROR_EXIT(X) do \
{ \
	result = (X); goto clean_up; \
} \
while(0)

static ULONGLONG g_tickCount = 0ULL;
static UINT g_timeout = DEFAULT_TIMEOUT;
static CHAR g_text_buffer[2048U];

#ifndef _DEBUG
static BOOL g_debug = FALSE;
#else
static const BOOL g_debug = TRUE;
#endif

static LRESULT CALLBACK my_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static BOOL clear_clipboard(void);
static BOOL check_argument(const WCHAR *const command_line, const WCHAR *const arg_name);
static WCHAR *get_configuration_path(void);
static WCHAR *get_executable_path(void);

#ifndef _DEBUG
extern IMAGE_DOS_HEADER __ImageBase;
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);
int startup(void)
{
	return wWinMain((HINSTANCE)&__ImageBase, NULL, GetCommandLineW(), SW_SHOWDEFAULT);
}
#endif //_DEBUG

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	int result = 0;
	HANDLE mutex = NULL;
	HWND hwnd = NULL;
	BOOL have_listener = FALSE, have_timer = FALSE;
	const WCHAR *config_path = NULL;
	WNDCLASSW wc;
	MSG msg;

	SecureZeroMemory(&wc, sizeof(WNDCLASSW));
	SecureZeroMemory(&msg, sizeof(MSG));

#ifndef _DEBUG
	g_debug = check_argument(lpCmdLine, L"--debug");
#endif //_DEBUG

	if(mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME))
	{
		const DWORD error = GetLastError();
		if(error == ERROR_ALREADY_EXISTS)
		{
			PRINT("already running, exiting!");
			ERROR_EXIT(1);
		}
	}

	PRINT("starting up...");

	if(config_path = get_configuration_path())
	{
		const UINT timeout = GetPrivateProfileInt(L"ClearClipboard", L"Timeout", DEFAULT_TIMEOUT, config_path);
		g_timeout = min(max(1000, timeout), USER_TIMER_MAXIMUM);
	}

	wc.lpfnWndProc   = my_wnd_proc;
	wc.hInstance     = hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = CLASS_NAME;

	if(!RegisterClassW(&wc))
	{
		PRINT("failed to register window class!");
		ERROR_EXIT(2);
	}

	if(!(hwnd = CreateWindowExW(0L, CLASS_NAME, L"ClearClipboard Window", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 0, 0, 640, 480, 0, 0, hInstance, NULL)))
	{
		PRINT("failed to create the window!");
		ERROR_EXIT(3);
	}

	if(!(have_listener = AddClipboardFormatListener(hwnd)))
	{
		PRINT("failed to install clipboard listener!");
		ERROR_EXIT(4);
	}

	g_tickCount = GetTickCount64();

	if(!(have_timer = SetTimer(hwnd, TIMER_UUID, min(max(g_timeout / 25U, USER_TIMER_MINIMUM), USER_TIMER_MAXIMUM), NULL)))
	{
		PRINT("failed to install the window timer!");
		ERROR_EXIT(5);
	}

	PRINT("monitoring started.");

	while(GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	PRINT("shutting down now...");

clean_up:
	
	if(hwnd && have_timer)
	{
		KillTimer(hwnd, TIMER_UUID);
	}

	if(hwnd && have_listener)
	{
		RemoveClipboardFormatListener(hwnd);
	}

	if(mutex)
	{
		CloseHandle(mutex);
	}
	
	PRINT("goodbye.");
	return result;
}

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

static BOOL check_argument(const WCHAR *const command_line, const WCHAR *const arg_name)
{
	const WCHAR **argv;
	int argc;
	BOOL found = FALSE;

	argv = CommandLineToArgvW(command_line, &argc);
	if(argv)
	{
		while(argc > 1)
		{
			if(!lstrcmpiW(argv[--argc], arg_name))
			{
				found = TRUE;
			}
		}
		LocalFree((HLOCAL)argv);
	}

	return found;
}


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
