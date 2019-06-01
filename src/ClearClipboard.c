/* ---------------------------------------------------------------------------------------------- */
/* ClearClipboard                                                                                */
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
#include <MMSystem.h>

#include "Version.h"

// Options
#define ENABLE_DEBUG_OUTPOUT 1
#define DEFAULT_TIMEOUT 30000U
#define DEFAULT_SOUND_LEVEL 1U

// Const
#define MUTEX_NAME L"{E19E5CE1-5EF2-4C10-843D-E79460920A4A}"
#define CLASS_NAME L"{6D6CB8E6-BFEE-40A1-A6B2-2FF34C43F3F8}"
#define TIMER_ID 0x5281CC36
#define ID_NOTIFYICON 0x8EF73CE1
#define WM_NOTIFYICON (WM_APP+101U)
#define MENU1_ID 0x1A5C
#define MENU2_ID 0x6810
#define MENU3_ID 0x46C3
#define MENU4_ID 0x38D6
#define WIN32_WINNT_WINTHRESHOLD 0x0A00

// Global variables
static UINT g_taskbar_created = 0U;
static HICON g_app_icon[2U] = { NULL, NULL };
static HMENU g_context_menu = NULL;
static UINT g_timeout = DEFAULT_TIMEOUT;
static BOOL g_textual_only = FALSE;
static UINT g_sound_enabled = DEFAULT_SOUND_LEVEL;
static BOOL g_halted = FALSE;
static BOOL g_silent = FALSE;
static const WCHAR *g_sound_file = NULL;
static const WCHAR *g_config_path = NULL;
static ULONGLONG g_tickCount = 0U;
#ifndef _DEBUG
static BOOL g_debug = FALSE;
#else
static const BOOL g_debug = TRUE;
#endif

// Forward declaration
static LRESULT CALLBACK my_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static UINT clear_clipboard(const BOOL force);
static BOOL is_textual_format(void);
static BOOL check_clipboard_settings(void);
static UINT parse_arguments(const WCHAR *const command_line);
static BOOL update_autorun_entry(const BOOL remove);
static BOOL create_shell_notify_icon(const HWND hwnd, const BOOL halted);
static BOOL update_shell_notify_icon(const HWND hwnd, const BOOL halted);
static BOOL delete_shell_notify_icon(const HWND hwnd);
static BOOL about_screen(const BOOL first_run);
static BOOL show_disclaimer(void);
static BOOL play_sound_effect(void);
static WCHAR *get_configuration_path(void);
static WCHAR *get_executable_path(void);
static UINT get_config_value(const WCHAR *const path, const WCHAR *const name, const UINT default_value, const UINT min_value, const UINT max_value);
static DWORD reg_read_value(const HKEY root, const WCHAR *const path, const WCHAR *const name, const DWORD default_value);
static WCHAR *reg_read_string(const HKEY root, const WCHAR *const path, const WCHAR *const name);
static BOOL reg_write_value(const HKEY root, const WCHAR *const path, const WCHAR *const name, const DWORD value);
static BOOL reg_write_string(const HKEY root, const WCHAR *const path, const WCHAR *const name, const WCHAR *const text);
static BOOL reg_delete_value(const HKEY root, const WCHAR *const path, const WCHAR *const name);
static WCHAR *quote_string(const WCHAR *const text);
static BOOL is_windows_version_or_greater(const WORD wMajorVersion, const WORD wMinorVersion, const WORD wServicePackMajor);

// ==========================================================================
// Utility macros
// ==========================================================================

// Wide string wrapper macro
#define _WTEXT_(X) L##X
#define WTEXT(X) _WTEXT_(X)

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

// Play sound
#define PLAY_SOUND(X) do \
{ \
	if(g_sound_enabled >= (X)) \
		play_sound_effect(); \
} \
while(0)

// Exit program
#define ERROR_EXIT(X) do \
{ \
	result = (X); \
	goto clean_up; \
} \
while(0)

// Free buffer
#define FREE(X) do \
{ \
	LocalFree((HLOCAL)(X)); \
	(X) = NULL; \
} \
while(0)

// Message box
#define MESSAGE_BOX(X,Y) do \
{ \
	if(!g_silent) \
		MessageBoxW(NULL, (X), L"ClearClipboard v" WTEXT(VERSION_STR), (Y) | MB_TOPMOST); \
} \
while(0)


// ==========================================================================
// Entry point function
// ==========================================================================

#ifndef _DEBUG

extern IMAGE_DOS_HEADER __ImageBase;
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

#pragma warning(push)
#pragma warning(disable: 4702)

int startup(void)
{
	const int exit_code =  wWinMain((HINSTANCE)&__ImageBase, NULL, GetCommandLineW(), SW_SHOWDEFAULT);
	ExitProcess((UINT)exit_code);
	return exit_code;
}

#pragma warning(pop)

#endif //_DEBUG

// ==========================================================================
// MAIN
// ==========================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	int result = 0, status = -1;
	UINT mode = 0U;
	HANDLE mutex = NULL;
	HWND hwnd = NULL;
	BOOL have_listener = FALSE, ignore_warning = FALSE;
	UINT_PTR timer_id = (UINT_PTR)NULL;
	WNDCLASSW wcl;
	MSG msg;

	// Unused params
	(void)hPrevInstance;
	(void)nCmdShow;

	// Initialize variables
	SecureZeroMemory(&wcl, sizeof(WNDCLASSW));
	SecureZeroMemory(&msg, sizeof(MSG));

	// Parse CLI arguments
	mode = parse_arguments(lpCmdLine);
	PRINT("ClearClipboard v" VERSION_STR " [" __DATE__ "]");

	// Check argument
	if(mode > 4U)
	{
		MESSAGE_BOX(L"Invalid command-line argument(s). Exiting!", MB_ICONERROR);
		return -1;
	}

	// Close running instances, if it was requested
	if((mode == 1U) || (mode == 2U))
	{
		PRINT("closing all running instances...");
		while(hwnd = FindWindowExW(NULL, hwnd, CLASS_NAME, NULL))
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
		if(success)
		{
			MESSAGE_BOX((mode > 3U) ? L"Autorun entry has been removed." : L"Autorun entry has been created.", MB_ICONINFORMATION);
		}
		else
		{
			MESSAGE_BOX((mode > 3U) ? L"Failed to remove autorun entry!" : L"Failed to create autorun entry!", MB_ICONWARNING);
		}
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

	// Print status
	PRINT("starting up...");

	// Register "TaskbarCreated" window message
	if(g_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated"))
	{
		ChangeWindowMessageFilter(g_taskbar_created, MSGFLT_ADD);
	}

	// Read config from INI file
	if(g_config_path = get_configuration_path())
	{
		g_timeout = get_config_value(g_config_path, L"Timeout", DEFAULT_TIMEOUT, 1000U, USER_TIMER_MAXIMUM);
		g_textual_only = !!get_config_value(g_config_path, L"TextOnly", FALSE, FALSE, TRUE);
		g_sound_enabled = get_config_value(g_config_path, L"Sound", DEFAULT_SOUND_LEVEL, 0U, 2U);
		g_halted = !!get_config_value(g_config_path, L"Halted", FALSE, FALSE, TRUE);
		ignore_warning = !!get_config_value(g_config_path, L"DisableWarningMessages", FALSE, FALSE, TRUE);
	}

	// Show the disclaimer message
	if(!show_disclaimer())
	{
		ERROR_EXIT(2);
	}

	// Check clipboard system configuration
	if(!(ignore_warning || check_clipboard_settings()))
	{
		ERROR_EXIT(4);
	}

	// Load icon resources
	g_app_icon[0U] = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
	g_app_icon[1U] = LoadIconW(hInstance, MAKEINTRESOURCEW(102));
	if(!(g_app_icon[0U] && g_app_icon[1U]))
	{
		PRINT("failed to load icon resource!");
	}

	// Detect sound file path
	if(g_sound_file = reg_read_string(HKEY_CURRENT_USER, L"AppEvents\\Schemes\\Apps\\Explorer\\EmptyRecycleBin\\.Current", L""))
	{
		if((!g_sound_file[0]) || (GetFileAttributesW(g_sound_file) == INVALID_FILE_ATTRIBUTES))
		{
			PRINT("sound file does not exist!");
			FREE(g_sound_file);
		}
	}

	// Create context menu
	if(g_context_menu = CreatePopupMenu())
	{
		AppendMenuW(g_context_menu, MF_STRING, MENU1_ID, L"ClearClipboard v" WTEXT(VERSION_STR));
		AppendMenuW(g_context_menu, MF_SEPARATOR, 0, NULL);
		AppendMenuW(g_context_menu, MF_STRING, MENU2_ID, L"Clear now!");
		AppendMenuW(g_context_menu, MF_STRING, MENU3_ID, L"Halt automatic clearing");
		AppendMenuW(g_context_menu, MF_SEPARATOR, 0, NULL);
		AppendMenuW(g_context_menu, MF_STRING, MENU4_ID, L"Quit");
		SetMenuDefaultItem(g_context_menu, MENU1_ID, FALSE);
	}
	else
	{
		PRINT("failed to create context menu!");
		ERROR_EXIT(5);
	}

	// Register window class
	wcl.lpfnWndProc   = my_wnd_proc;
	wcl.hInstance     = hInstance;
	wcl.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
	wcl.lpszClassName = CLASS_NAME;
	if(!RegisterClassW(&wcl))
	{
		PRINT("failed to register window class!");
		ERROR_EXIT(6);
	}

	// Create the message-only window
	if(!(hwnd = CreateWindowExW(0L, CLASS_NAME, L"ClearClipboard window", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 0, 0, 0, 0, NULL, 0, hInstance, NULL)))
	{
		PRINT("failed to create the window!");
		ERROR_EXIT(7);
	}

	// Create notification icon
	create_shell_notify_icon(hwnd, g_halted);

	// Add clipboard listener
	if(!(have_listener = AddClipboardFormatListener(hwnd)))
	{
		PRINT("failed to install clipboard listener!");
		ERROR_EXIT(8);
	}

	// Set up window timer
	g_tickCount = GetTickCount64();
	if(!(timer_id = SetTimer(hwnd, TIMER_ID, min(max(g_timeout / 50U, USER_TIMER_MINIMUM), USER_TIMER_MAXIMUM), NULL)))
	{
		PRINT("failed to install the window timer!");
		ERROR_EXIT(9);
	}

	PRINT("clipboard monitoring started.");

	// Message loop
	while(status = GetMessageW(&msg, NULL, 0, 0) != 0)
	{
		if(status == -1)
		{
			PRINT("failed to fetch next message!");
			ERROR_EXIT(10);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	PRINT("shutting down now...");

clean_up:
	
	// Kill timer
	if(hwnd && timer_id)
	{
		KillTimer(hwnd, timer_id);
	}

	// Delete notification icon
	if(hwnd)
	{
		delete_shell_notify_icon(hwnd);
	}

	// Remove clipboard listener
	if(hwnd && have_listener)
	{
		RemoveClipboardFormatListener(hwnd);
	}

	// Free icon resource
	if(g_context_menu)
	{
		DestroyMenu(g_context_menu);
	}

	// Free icon resources
	if(g_app_icon[0U])
	{
		DestroyIcon(g_app_icon[0U]);
	}
	if(g_app_icon[1U])
	{
		DestroyIcon(g_app_icon[1U]);
	}

	// Free sound file path
	if(g_sound_file)
	{
		FREE(g_sound_file);
	}

	// Free config file path
	if(g_config_path)
	{
		FREE(g_config_path);
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
				if(!g_halted)
				{
					const UINT result = clear_clipboard(!g_textual_only);
					if(result)
					{
						g_tickCount = tickCount;
						if(result == 1U)
						{
							PLAY_SOUND(2U);
						}
					}
				}
				else
				{
					PRINT("skipped. (clearing is halted)");
					g_tickCount = tickCount;
				}
			}
			else
			{
				PRINT("not ready yet."); /*wait for next WM_TIMER*/
			}
		}
		break;
	case WM_NOTIFYICON:
		switch(LOWORD(lParam))
		{
		case WM_CONTEXTMENU:
			PRINT("WM_NOTIFYICON --> WM_CONTEXTMENU");
			if(g_context_menu)
			{
				SetForegroundWindow(hWnd);
				TrackPopupMenu(g_context_menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, LOWORD(wParam), HIWORD(wParam), 0, hWnd, NULL);
			}
			break;
		case WM_LBUTTONDBLCLK:
			PRINT("WM_LBUTTONDBLCLK");
			if(clear_clipboard(TRUE))
			{
				g_tickCount = GetTickCount64();
				PLAY_SOUND(1U);
			}
			break;
		}
		break;
	case WM_COMMAND:
		PRINT("WM_COMMAND");
		if(HIWORD(wParam) == 0)
		{
			switch(LOWORD(wParam))
			{
			case MENU1_ID:
				PRINT("menu item #1 triggered");
				about_screen(FALSE);
				break;
			case MENU2_ID:
				PRINT("menu item #2 triggered");
				if(clear_clipboard(TRUE))
				{
					g_tickCount = GetTickCount64();
					PLAY_SOUND(1U);
				}
				break;
			case MENU3_ID:
				PRINT("menu item #3 triggered");
				g_halted = !g_halted;
				CheckMenuItem(g_context_menu, MENU3_ID, g_halted ? MF_CHECKED : MF_UNCHECKED);
				update_shell_notify_icon(hWnd, g_halted);
				if(!g_halted)
				{
					g_tickCount = GetTickCount64();
				}
				break;
			case MENU4_ID:
				PRINT("menu item #4 triggered");
				PostMessageW(hWnd, WM_CLOSE, 0, 0);
				break;
			}
		}
		break;
	default:
		if(message == g_taskbar_created)
		{
			PRINT("TaskbarCreated");
			create_shell_notify_icon(hWnd, g_halted);
		}
		else
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

	return 0;
}

// ==========================================================================
// Clear Clipboard
// ==========================================================================

static UINT clear_clipboard(const BOOL force)
{
	int retry;
	UINT success = 0U;

	PRINT("clearing clipboard...");

	for(retry = 0; retry < 32; ++retry)
	{
		if(retry > 0)
		{
			PRINT("retry!");
			Sleep(1); /*yield*/
		}
		if(OpenClipboard(NULL))
		{
			if(force || is_textual_format())
			{
				if(EmptyClipboard())
				{
					success = 1U; /*cleared*/
				}
			}
			else
			{
				success = 2U; /*skipped*/
			}
			CloseClipboard();
		}
		if(success)
		{
			break; /*completed*/
		}
	}

	if(success)
	{
		if(success > 1U)
		{
			PRINT("skipped. (not textual data)");
		}
		else
		{
			PRINT("cleared.");
		}
	}
	else
	{
		PRINT("failed to clean clipboard!");
	}

	return success;
}

static BOOL is_textual_format(void)
{
	UINT format = 0U;

	do
	{
		switch(format = EnumClipboardFormats(format))
		{
		case CF_TEXT:
		case CF_OEMTEXT:
		case CF_UNICODETEXT:
		case CF_DSPTEXT:
			return TRUE;
		}
	}
	while(format != 0U);

	return FALSE;
}

// ==========================================================================
// Check clipboard configuration
// ==========================================================================

#define SHOW_WARNING(X) \
	MessageBoxW(NULL, (X), L"ClearClipboard v" WTEXT(VERSION_STR), MB_ICONWARNING | MB_ABORTRETRYIGNORE | MB_TOPMOST)

static BOOL check_clipboard_settings(void)
{
	static const WCHAR *const REG_VALUE_PATH = L"Software\\Microsoft\\Clipboard";
	static const WCHAR *const REG_VALUE_NAME[2] = { L"EnableClipboardHistory", L"CloudClipboardAutomaticUpload" };

	if(!is_windows_version_or_greater(HIBYTE(WIN32_WINNT_WINTHRESHOLD), LOBYTE(WIN32_WINNT_WINTHRESHOLD), 0))
	{
		PRINT("not running on Windows 10 or later.");
		return TRUE;
	}

	while(reg_read_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME[0], 0U) > 0U)
	{
		PRINT("warning: windows clipboard history is enabled!");
		switch(SHOW_WARNING(L"It apperas that the \"Clipboard History\" feature of Windows 10 is enabled on your machine. As long as that feature is enabled, Windows 10 will silently keep a history (copy) of *all* data that has been copied to the clipboard at some time.\n\nIn order to keep your data private, it is *highly* recommended to disable the \"Clipboard History\" in the system settings!"))
		{
		case IDABORT:
			return FALSE;
		case IDIGNORE:
			goto exit_loop_1;
		}
	}

exit_loop_1:
	while(reg_read_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME[1], 0U) > 0U)
	{
		PRINT("warning: windows clipboard cloud-sync is enabled!");
		switch(SHOW_WARNING(L"It apperas that the \"Automatic Syncing\" (Cloud Clipboard) feature of Windows 10 is enabled on your machine. As long as that feature is enabled, Windows 10 will upload *all* data that is copied to the clipboard to the Microsoft cloud.\n\nIn order to keep your data private, it is *highly* recommended to disable the \"Automatic Syncing\" in the system settings!"))
		{
		case IDABORT:
			return FALSE;
		case IDIGNORE:
			goto exit_loop_2;
		}
	}

exit_loop_2:
	return TRUE;
}

#undef SHOW_WARNING

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
			const WCHAR *value = argv[i];
			while((*value) && (*value <= 0x20))
			{
				++value;
			}
			if(*value)
			{
				if(!lstrcmpiW(value, L"--close"))
				{
					mode = 1U;
				}
				else if(!lstrcmpiW(value, L"--restart"))
				{
					mode = 2U;
				}
				else if(!lstrcmpiW(value, L"--install"))
				{
					mode = 3U;
				}
				else if(!lstrcmpiW(value, L"--uninstall"))
				{
					mode = 4U;
				}
#ifndef _DEBUG
				else if(!lstrcmpiW(value, L"--debug"))
				{
					g_debug = TRUE;
				}
#endif //_DEBUG
				else if(!lstrcmpiW(value, L"--silent"))
				{
					g_silent = TRUE;
				}
				else if(!lstrcmpiW(value, L"--slunk"))
				{
					ShellExecuteW(NULL, NULL, L"https://youtu.be/n4bply6Ibqw", NULL, NULL, SW_SHOW);
				}
				else
				{
					mode = MAXUINT;
					break; /*bad argument*/
				}
			}
		}
		FREE(argv);
	}

	return mode;
}

// ==========================================================================
// Autorun support
// ==========================================================================

static BOOL update_autorun_entry(const BOOL remove)
{
	static const WCHAR *const REG_VALUE_NAME = L"com.muldersoft.clear_clipboard";
	static const WCHAR *const REG_VALUE_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

	BOOL success = FALSE;

	if(!remove)
	{
		const WCHAR *executable_path = get_executable_path();
		if(executable_path)
		{
			const WCHAR *command_str = quote_string(executable_path);
			if(command_str)
			{
				PRINT("adding autorun entry to registry...");
				if(reg_write_string(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME, command_str))
				{
					PRINT("succeeded.");
					success = TRUE;
				}
				else
				{
					PRINT("failed to add autorun entry to registry!");
				}
				FREE(command_str);
			}
			else
			{
				PRINT("failed to allocate string buffer!");
			}
			FREE(executable_path);
		}
		else
		{
			PRINT("failed to determine executable path!");
		}
	}
	else
	{
		PRINT("removing autorun entry from registry...");
		if(reg_delete_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME))
		{
			PRINT("succeeded.");
			success = TRUE;
		}
		else
		{
			PRINT("failed to remove autorun entry from registry!");
		}
	}

	return success;
}

// ==========================================================================
// Shell notification icon
// ==========================================================================

#define INITIALIZE_TIP(X,Y) do \
{ \
	lstrcpyW((X), L"ClearClipboard v" WTEXT(VERSION_STR)); \
	if((Y)) \
		lstrcatW(shell_icon_data.szTip, L" - halted"); \
} \
while(0)

static BOOL create_shell_notify_icon(const HWND hwnd, const BOOL halted)
{
	NOTIFYICONDATAW shell_icon_data;
	SecureZeroMemory(&shell_icon_data, sizeof(NOTIFYICONDATAW));

	shell_icon_data.cbSize = sizeof(NOTIFYICONDATAW);
	shell_icon_data.hWnd = hwnd;
	shell_icon_data.uID = ID_NOTIFYICON;
	shell_icon_data.uCallbackMessage = WM_NOTIFYICON;
	INITIALIZE_TIP(shell_icon_data.szTip, halted);
	shell_icon_data.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE;
	if(g_app_icon[halted ? 1U : 0U])
	{
		shell_icon_data.hIcon = g_app_icon[halted ? 1U : 0U];
		shell_icon_data.uFlags |= NIF_ICON;
	}

	if(Shell_NotifyIconW(NIM_ADD, &shell_icon_data))
	{
		shell_icon_data.uVersion = NOTIFYICON_VERSION_4;
		Shell_NotifyIconW(NIM_SETVERSION, &shell_icon_data);
	}
	else
	{
		PRINT("failed to create the shell notification icon!");
		return FALSE;
	}

	return TRUE;
}

static BOOL update_shell_notify_icon(const HWND hwnd, const BOOL halted)
{
	NOTIFYICONDATAW shell_icon_data;
	SecureZeroMemory(&shell_icon_data, sizeof(NOTIFYICONDATAW));

	shell_icon_data.cbSize = sizeof(NOTIFYICONDATAW);
	shell_icon_data.hWnd = hwnd;
	shell_icon_data.uID = ID_NOTIFYICON;
	INITIALIZE_TIP(shell_icon_data.szTip, halted);
	shell_icon_data.uFlags = NIF_TIP | NIF_SHOWTIP;
	if(g_app_icon[halted ? 1U : 0U])
	{
		shell_icon_data.hIcon = g_app_icon[halted ? 1U : 0U];
		shell_icon_data.uFlags |= NIF_ICON;
	}

	if(!Shell_NotifyIconW(NIM_MODIFY, &shell_icon_data))
	{
		PRINT("failed to modify the shell notification icon!");
		return FALSE;
	}

	return TRUE;
}

static BOOL delete_shell_notify_icon(const HWND hwnd)
{
	NOTIFYICONDATAW shell_icon_data;
	SecureZeroMemory(&shell_icon_data, sizeof(NOTIFYICONDATAW));

	shell_icon_data.cbSize = sizeof(NOTIFYICONDATAW);
	shell_icon_data.hWnd = hwnd;
	shell_icon_data.uID = ID_NOTIFYICON;
	
	if(!Shell_NotifyIconW(NIM_DELETE, &shell_icon_data))
	{
		PRINT("failed to remove the shell notification icon!");
		return FALSE;
	}

	return TRUE;
}

#undef INITIALIZE_TIP

// ==========================================================================
// About dialog
// ==========================================================================

static BOOL about_screen(const BOOL first_run)
{
	const int resut = MessageBoxW(
		NULL,
		L"ClearClipboard v" WTEXT(VERSION_STR) L" [" WTEXT(__DATE__) L"]\n"
		L"Copyright(\x24B8) 2019 LoRd_MuldeR <mulder2@gmx.de>\n\n"
		L"This software is released under the MIT License.\n"
		L"(https://opensource.org/licenses/MIT)\n\n"
		L"For news and updates please check the website at:\n"
		L"\x2022 http://muldersoft.com/\n"
		L"\x2022 https://github.com/lordmulder/ClearClipboard\n\n"
		L"DISCLAIMER: "
		L"The software is provided \"as is\", without warranty of any kind, express or implied, including"
		L"but not limited to the warranties of merchantability, fitness for a particular purpose and"
		L"noninfringement. In no event shall the authors or copyright holders be liable for any claim,"
		L"damages or other liability, whether in an action of contract, tort or otherwise, arising from,"
		L"out of or in connection with the software or the use or other dealings in the software.",
		L"About...",
		MB_ICONINFORMATION | MB_TOPMOST | (first_run ? MB_OKCANCEL|MB_DEFBUTTON2 : MB_OK)
	);

	return (resut == IDOK);
}

static BOOL show_disclaimer(void)
{
	static const DWORD REG_VALUE_DATA = (((DWORD)VERSION_MAJOR) << 16) | (((DWORD)VERSION_MINOR_HI) << 8) | ((DWORD)VERSION_MINOR_LO);
	static const WCHAR *const REG_VALUE_NAME = L"DisclaimerAccepted";
	static const WCHAR *const REG_VALUE_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{7816D5D9-5D9D-4B3A-B5A8-DD7A7F4C44A3}";

	if(g_config_path)
	{
		if(get_config_value(g_config_path, REG_VALUE_NAME, FALSE, FALSE, TRUE) > 0)
		{
			return TRUE;
		}
	}

	if(reg_read_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME, 0U) != REG_VALUE_DATA)
	{
		if(about_screen(TRUE))
		{
			reg_write_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME, REG_VALUE_DATA);
		}
		else
		{
			return FALSE; /*rejected*/
		}
	}

	return TRUE;
}

// ==========================================================================
// Play sound effect
// ==========================================================================

static BOOL play_sound_effect(void)
{
	BOOL success = FALSE;

	if(g_sound_file)
	{
		success = PlaySoundW(g_sound_file, NULL, SND_ASYNC);
	}

	if(!success)
	{
		success = PlaySoundW(L"SystemAsterisk", NULL, SND_ALIAS | SND_ASYNC);
	}

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

// ==========================================================================
// Configuration routines
// ==========================================================================

static UINT get_config_value(const WCHAR *const path, const WCHAR *const name, const UINT default_value, const UINT min_value, const UINT max_value)
{
	static const WCHAR *const SECTION_NAME = L"ClearClipboard";
	const UINT value = GetPrivateProfileIntW(SECTION_NAME, name , default_value, path);
	return max(min_value, min(max_value, value));
}

// ==========================================================================
// Registry routines
// ==========================================================================

unsigned long _byteswap_ulong(unsigned long value);

static DWORD reg_read_value(const HKEY root, const WCHAR *const path, const WCHAR *const name, const DWORD default_value)
{
	HKEY hkey = NULL;
	DWORD result = 0U, type = REG_NONE, size = sizeof(DWORD);

	if(RegOpenKeyExW(root, path, 0U, KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		PRINT("failed to open registry key for reading!");
		return default_value;
	}

	if(RegQueryValueExW(hkey, name, NULL, &type, (BYTE*)&result, &size) == ERROR_SUCCESS)
	{
		if((type != REG_DWORD) && (type != REG_DWORD_BIG_ENDIAN))
		{
			PRINT("registry value is not a DWORD!");
			result = default_value;
		}
		else if(type == REG_DWORD_BIG_ENDIAN)
		{
			_byteswap_ulong(result); /*cirrect endianess*/
		}
	}
	else
	{
		PRINT("failed to read registry value!");
		result = default_value;
	}

	RegCloseKey(hkey);
	return result;
}

static WCHAR *reg_read_string(const HKEY root, const WCHAR *const path, const WCHAR *const name)
{
	HKEY hkey = NULL;
	WCHAR *buffer = NULL;
	DWORD buff_size;

	if(RegOpenKeyExW(root, path, 0U, KEY_READ, &hkey) != ERROR_SUCCESS)
	{
		PRINT("failed to open registry key for reading!");
		return NULL;
	}

	for(buff_size = 2048U; buff_size <= 1048576U; buff_size <<= 1U)
	{
		if(buffer = (WCHAR*) LocalAlloc(LPTR, buff_size))
		{
			DWORD type = REG_NONE, size = buff_size;
			const LSTATUS error = RegQueryValueExW(hkey, name, NULL, &type, (BYTE*)buffer, &size);
			if(error == ERROR_SUCCESS)
			{
				if((type != REG_SZ) && (type != REG_EXPAND_SZ))
				{
			
					PRINT("registry value is not a string!");
					FREE(buffer);
				}
				break; /*done*/
			}
			else
			{
				FREE(buffer);
				if(error != ERROR_MORE_DATA)
				{
					PRINT("failed to read registry string!");
					break; /*failed*/
				}
			}
		}
		else
		{
			PRINT("failed to allocate string buffer!");
			break; /*failed*/
		}
	}

	RegCloseKey(hkey);
	return buffer;
}

static BOOL reg_write_value(const HKEY root, const WCHAR *const path, const WCHAR *const name, const DWORD value)
{
	HKEY hkey = NULL;

	if(RegCreateKeyExW(root, path, 0U, NULL, 0U, KEY_WRITE, NULL, &hkey, NULL) != ERROR_SUCCESS)
	{
		PRINT("failed to open registry key for writing!");
		return FALSE;
	}

	if(RegSetValueExW(hkey, name, 0U, REG_DWORD, (BYTE*)&value, sizeof(DWORD)) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		PRINT("failed to write registry value!");
		return FALSE;
	}

	RegCloseKey(hkey);
	return TRUE;
}

static BOOL reg_write_string(const HKEY root, const WCHAR *const path, const WCHAR *const name, const WCHAR *const text)
{
	HKEY hkey = NULL;

	if(RegCreateKeyExW(root, path, 0U, NULL, 0U, KEY_WRITE, NULL, &hkey, NULL) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		PRINT("failed to open registry key for writing!");
		return FALSE;
	}

	if(RegSetValueExW(hkey, name, 0U, REG_SZ, (BYTE*)text, (lstrlenW(text) + 1U) * sizeof(WCHAR)) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		PRINT("failed to write registry value!");
		return FALSE;
	}

	RegCloseKey(hkey);
	return TRUE;
}

static BOOL reg_delete_value(const HKEY root, const WCHAR *const path, const WCHAR *const name)
{
	HKEY hkey = NULL;
	LSTATUS error;

	if((error = RegOpenKeyExW(root, path, 0U, KEY_WRITE, &hkey)) != ERROR_SUCCESS)
	{
		if(error != ERROR_FILE_NOT_FOUND)
		{
			PRINT("failed to open registry key for writing!");
			return FALSE;
		}
		return TRUE;
	}

	if((error = RegDeleteValueW(hkey, name)) != ERROR_SUCCESS)
	{
		if(error != ERROR_FILE_NOT_FOUND)
		{
			RegCloseKey(hkey);
			PRINT("failed to delete registry value!");
			return FALSE;
		}
	}

	RegCloseKey(hkey);
	return TRUE;
}

// ==========================================================================
// String helper routines
// ==========================================================================

static WCHAR *quote_string(const WCHAR *const text)
{
	WCHAR *const buffer = (WCHAR*) LocalAlloc(LPTR, (lstrlenW(text) + 3U) * sizeof(WCHAR));
	if(buffer)
	{
		lstrcpyW(buffer, L"\"");
		lstrcatW(buffer, text);
		lstrcatW(buffer, L"\"");
		return buffer;
	}
	
	PRINT("failed to allocate string buffer!");
	return NULL;
}

// ==========================================================================
// Windows version helper
// ==========================================================================

static BOOL is_windows_version_or_greater(const WORD wMajorVersion, const WORD wMinorVersion, const WORD wServicePackMajor)
{
	OSVERSIONINFOEXW osvi;
	const DWORDLONG dwlConditionMask = VerSetConditionMask(VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	SecureZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	osvi.dwMajorVersion = wMajorVersion;
	osvi.dwMinorVersion = wMinorVersion;
	osvi.wServicePackMajor = wServicePackMajor;
	return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}
