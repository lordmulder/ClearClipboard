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
#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <Mmsystem.h>

#include "Version.h"

// Defaults
#define DEFAULT_TIMEOUT 30000U
#define DEFAULT_SOUND_LEVEL 1U

// Const
#define MUTEX_NAME L"{E19E5CE1-5EF2-4C10-843D-E79460920A4A}"
#define CLASS_NAME L"{6D6CB8E6-BFEE-40A1-A6B2-2FF34C43F3F8}"
#define TIMER_ID 0x5281CC36
#define ID_NOTIFYICON 0x8EF73CE1
#define ID_HOTKEY 0xBC86
#define WM_NOTIFYICON (WM_APP+101U)
#define MENU1_ID 0x1A5C
#define MENU2_ID 0x6810
#define MENU3_ID 0x46C3
#define MENU4_ID 0x38D6
#define WIN32_WINNT_WINTHRESHOLD 0x0A00

// Common text formats
const WCHAR *const TEXT_FORMATS[12U] =
{
	L"CSV",
	L"HTML (Hyper Text Markup Language)",
	L"HTML Format",
	L"RTF As Text",
	L"Rich Text Format",
	L"Rich Text Format Without Objects",
	L"RichEdit Text and Objects",
	L"text/csv",
	L"text/html",
	L"text/plain",
	L"text/richtext",
	L"text/uri-list"
};

// User settings
static UINT cfg_timeout = DEFAULT_TIMEOUT;
static BOOL cfg_textual_only = FALSE;
static UINT cfg_sound_enabled = DEFAULT_SOUND_LEVEL;
static BOOL cfg_halted = FALSE;
static WORD cfg_hotkey = 0U;
static BOOL cfg_ignore_warning = FALSE;
static BOOL cfg_silent = FALSE;
#ifndef _DEBUG
static UINT cfg_debug = 0U;
#else
static const UINT cfg_debug = 2U;
#endif

// Global variables
static ULONGLONG g_tickCount = 0U;
static UINT g_text_formats[16U] = { CF_TEXT, CF_OEMTEXT, CF_UNICODETEXT, CF_DSPTEXT, 0U };
static UINT g_taskbar_created = 0U;
static const WCHAR *g_sound_file = NULL;
static const WCHAR *g_config_path = NULL;
static HICON g_app_icon[2U] = { NULL, NULL };
static HMENU g_context_menu = NULL;

// Forward declaration
static LRESULT CALLBACK my_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static UINT clear_clipboard(const BOOL force);
static BOOL is_textual_format(void);
static BOOL check_clipboard_history(void);
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
static WCHAR *get_system_directory(void);
static BOOL file_exists(const WCHAR *const path);
static int get_config_value(const WCHAR *const path, const WCHAR *const name, const int default_value, const int min_value, const int max_value);
static DWORD reg_read_value(const HKEY root, const WCHAR *const path, const WCHAR *const name, const DWORD default_value);
static WCHAR *reg_read_string(const HKEY root, const WCHAR *const path, const WCHAR *const name);
static BOOL reg_write_value(const HKEY root, const WCHAR *const path, const WCHAR *const name, const DWORD value);
static BOOL reg_write_string(const HKEY root, const WCHAR *const path, const WCHAR *const name, const WCHAR *const text);
static BOOL reg_delete_value(const HKEY root, const WCHAR *const path, const WCHAR *const name);
static BOOL find_running_service(const WCHAR *const name_prefix);
static WCHAR *quote_string(const WCHAR *const text);
static WCHAR *concat_strings(const WCHAR *const text_1, const WCHAR *const text_2);
static void output_formatted_string(const char *const format, ...);
static BOOL is_windows_version_or_greater(const WORD wMajorVersion, const WORD wMinorVersion, const WORD wServicePackMajor);

// ==========================================================================
// Utility macros
// ==========================================================================

// Wide string wrapper macro
#define _WTEXT_(X) L##X
#define WTEXT(X) _WTEXT_(X)

// Debug output
#define _OUTPUT_DBGSTR(X,Y) do \
{ \
	if (cfg_debug >= (X)) \
		OutputDebugStringA("ClearClipboard -- " Y "\n"); \
} \
while(0)
#define DEBUG(X) _OUTPUT_DBGSTR(1U, X)
#define TRACE(X) _OUTPUT_DBGSTR(2U, X)

// Formatted output
#define _OUTPUT_DBGSTR2(X,Y,...) do \
{ \
	if (cfg_debug >= (X)) \
		output_formatted_string("ClearClipboard -- " Y "\n", __VA_ARGS__); \
} \
while(0)
#define DEBUG2(X,...) _OUTPUT_DBGSTR2(1U, X, __VA_ARGS__)
#define TRACE2(X,...) _OUTPUT_DBGSTR2(2U, X, __VA_ARGS__)

// Play sound
#define PLAY_SOUND(X) do \
{ \
	if(cfg_sound_enabled >= (X)) \
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
#define MESSAGE_BOX(X,Y) MessageBoxW(NULL, (X), L"ClearClipboard v" WTEXT(VERSION_STR), (Y) | MB_SETFOREGROUND | MB_TOPMOST)
#define SHOW_MESSAGE(X,Y) do \
{ \
	if(!cfg_silent) \
		MESSAGE_BOX(X,Y); \
} \
while(0)

// Boolify
#define BOOLIFY(X) ((X) ? "true" : "false")

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
	HANDLE mutex = NULL, sf_lock = INVALID_HANDLE_VALUE;
	HWND hwnd = NULL;
	BOOL have_listener = FALSE;
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
	DEBUG("ClearClipboard v" VERSION_STR " [" __DATE__ "]");

	// Check argument
	if(mode > 4U)
	{
		MESSAGE_BOX(L"Invalid command-line argument(s). Exiting!", MB_ICONERROR);
		return -1;
	}

	// Close running instances, if it was requested
	if((mode == 1U) || (mode == 2U))
	{
		DEBUG("closing all running instances...");
		while(hwnd = FindWindowExW(NULL, hwnd, CLASS_NAME, NULL))
		{
			DEBUG2("sending WM_CLOSE message to: hwnd=%p", hwnd);
			SendMessageW(hwnd, WM_CLOSE, 0U, 0U);
		}
		if(mode == 1U)
		{
			DEBUG("goodbye.");
			return 0;
		}
	}

	// Add or remove autorun entry, if it was requested
	if((mode == 3U) || (mode == 4U))
	{
		const BOOL success = update_autorun_entry(mode > 3U);
		if(success)
		{
			SHOW_MESSAGE((mode > 3U) ? L"Autorun entry has been removed." : L"Autorun entry has been created.", MB_ICONINFORMATION);
		}
		else
		{
			SHOW_MESSAGE((mode > 3U) ? L"Failed to remove autorun entry!" : L"Failed to create autorun entry!", MB_ICONWARNING);
		}
		DEBUG("goodbye.");
		return success ? 0 : 1;
	}

	// Lock single instance mutex
	if(mutex = CreateMutexW(NULL, FALSE, MUTEX_NAME))
	{
		const DWORD ret = WaitForSingleObject(mutex, (mode > 1U) ? 5000U : 250U);
		if((ret != WAIT_OBJECT_0) && (ret != WAIT_ABANDONED))
		{
			DEBUG("already running, exiting!");
			ERROR_EXIT(1);
		}
	}
	else
	{
		DEBUG("failed to create mutex!");
	}

	// Read configuration
	if(g_config_path = get_configuration_path())
	{
		if(file_exists(g_config_path))
		{
			DEBUG("reading configuration file...");
			cfg_timeout = (UINT) get_config_value(g_config_path, L"Timeout", DEFAULT_TIMEOUT, 1000, 3600000/*1h*/);
			cfg_textual_only = !!get_config_value(g_config_path, L"TextOnly", FALSE, FALSE, TRUE);
			cfg_sound_enabled = (UINT) get_config_value(g_config_path, L"Sound", DEFAULT_SOUND_LEVEL, 0, 2);
			cfg_halted = !!get_config_value(g_config_path, L"Halted", FALSE, FALSE, TRUE);
			cfg_hotkey = (WORD) get_config_value(g_config_path, L"Hotkey", 0U, 0U, 0x8FF);
			cfg_ignore_warning = !!get_config_value(g_config_path, L"DisableWarningMessages", FALSE, FALSE, TRUE);
		}
		else
		{
			DEBUG("configuration file not found!");
		}
	}

	// Dump config variables
	DEBUG2("config: timeout=%u", cfg_timeout);
	DEBUG2("config: textual_only=%s", BOOLIFY(cfg_textual_only));
	DEBUG2("config: halted=%s", BOOLIFY(cfg_halted));
	DEBUG2("config: sound_enabled=%u", cfg_sound_enabled);
	DEBUG2("config: hotkey=0x%03X", (UINT)cfg_hotkey);
	DEBUG2("config: ignore_warning=%s", BOOLIFY(cfg_ignore_warning));

	// Show the disclaimer message
	if(!show_disclaimer())
	{
		ERROR_EXIT(2);
	}

	// Check clipboard system configuration
	if(!(cfg_ignore_warning || check_clipboard_history()))
	{
		ERROR_EXIT(4);
	}

	// Print status
	DEBUG("starting up...");

	// Register "TaskbarCreated" window message
	if(g_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated"))
	{
		ChangeWindowMessageFilter(g_taskbar_created, MSGFLT_ADD);
	}

	// Register common clipboard formats
	if(cfg_textual_only)
	{
		size_t i;
		for(i = 0U; i < _countof(TEXT_FORMATS); ++i)
		{
			g_text_formats[4U + i] = RegisterClipboardFormatW(TEXT_FORMATS[i]);
			TRACE2("text_format[%02u] = 0x%04X", i, g_text_formats[i]);
		}
	}

	// Load icon resources
	g_app_icon[0U] = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
	g_app_icon[1U] = LoadIconW(hInstance, MAKEINTRESOURCEW(102));
	if(!(g_app_icon[0U] && g_app_icon[1U]))
	{
		DEBUG("failed to load icon resource!");
	}

	// Detect sound file path
	if(g_sound_file = reg_read_string(HKEY_CURRENT_USER, L"AppEvents\\Schemes\\Apps\\Explorer\\EmptyRecycleBin\\.Current", L""))
	{
		if(g_sound_file[0] && file_exists(g_sound_file))
		{
			if((sf_lock = CreateFileW(g_sound_file, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0U, NULL)) != INVALID_HANDLE_VALUE)
			{
				SetHandleInformation(sf_lock, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);
			}
			else
			{
				DEBUG("failed to open sound file for reading!");
				FREE(g_sound_file);
			}
		}
		else
		{
			DEBUG("sound file does not exist!");
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
		DEBUG("failed to create context menu!");
		ERROR_EXIT(5);
	}

	// Register window class
	wcl.lpfnWndProc   = my_wnd_proc;
	wcl.hInstance     = hInstance;
	wcl.lpszClassName = CLASS_NAME;
	if(!RegisterClassW(&wcl))
	{
		DEBUG("failed to register window class!");
		ERROR_EXIT(6);
	}

	// Create the message-only window
	if(!(hwnd = CreateWindowExW(0L, CLASS_NAME, L"ClearClipboard window", WS_OVERLAPPEDWINDOW/*|WS_VISIBLE*/, 0, 0, 0, 0, NULL, 0, hInstance, NULL)))
	{
		DEBUG("failed to create the window!");
		ERROR_EXIT(7);
	}

	// Create notification icon
	if(!create_shell_notify_icon(hwnd, cfg_halted))
	{
		DEBUG("failed to create the shell notification icon!");
	}

	// Add clipboard listener
	if(!(have_listener = AddClipboardFormatListener(hwnd)))
	{
		DEBUG("failed to install clipboard listener!");
		ERROR_EXIT(8);
	}

	// Register hotkey
	if((cfg_hotkey >= 0x100) && (cfg_hotkey <= 0xFFF) && (LOBYTE(cfg_hotkey) >= 0x08))
	{
		if(!RegisterHotKey(hwnd, ID_HOTKEY, HIBYTE(cfg_hotkey) | MOD_NOREPEAT, LOBYTE(cfg_hotkey)))
		{
			DEBUG("failed to register hotkey! already registred?");
		}
	}

	// Set up window timer
	g_tickCount = GetTickCount64();
	if(!(timer_id = SetTimer(hwnd, TIMER_ID, max(cfg_timeout / 30U, USER_TIMER_MINIMUM), NULL)))
	{
		DEBUG("failed to install the window timer!");
		ERROR_EXIT(9);
	}

	DEBUG("clipboard monitoring started.");

	// Message loop
	while(status = GetMessageW(&msg, NULL, 0, 0) != 0)
	{
		if(status == -1)
		{
			DEBUG("failed to fetch next message!");
			ERROR_EXIT(10);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DEBUG("shutting down now...");

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

	// Close sound file
	if(sf_lock != INVALID_HANDLE_VALUE)
	{
		SetHandleInformation(sf_lock, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0U);
		CloseHandle(sf_lock);
	}

	// Close mutex
	if(mutex)
	{
		CloseHandle(mutex);
	}
	
	DEBUG("goodbye.");
	return result;
}

// ==========================================================================
// Window Procedure
// ==========================================================================

static LRESULT CALLBACK my_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CLIPBOARDUPDATE:
		TRACE("WM_CLIPBOARDUPDATE");
		{
			const ULONGLONG tickCount = GetTickCount64();
			if((tickCount > g_tickCount) && ((tickCount - g_tickCount) > 10U))
			{
				DEBUG("clipboard content has changed.");
				g_tickCount = tickCount;
			}
		}
		break;
	case WM_TIMER:
		TRACE("WM_TIMER");
		{
			const ULONGLONG tickCount = GetTickCount64();
			if((tickCount > g_tickCount) && ((tickCount - g_tickCount) > cfg_timeout))
			{
				DEBUG("timer triggered!");
				if(!cfg_halted)
				{
					const UINT result = clear_clipboard(!cfg_textual_only);
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
					DEBUG("automatic clearing is halted.");
					g_tickCount = tickCount;
				}
			}
		}
		break;
	case WM_NOTIFYICON:
		TRACE("WM_NOTIFYICON");
		switch(LOWORD(lParam))
		{
		case WM_CONTEXTMENU:
			TRACE("--> WM_CONTEXTMENU");
			if(g_context_menu)
			{
				SetForegroundWindow(hWnd);
				TrackPopupMenu(g_context_menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, LOWORD(wParam), HIWORD(wParam), 0, hWnd, NULL);
			}
			break;
		case WM_LBUTTONDBLCLK:
			TRACE("--> WM_LBUTTONDBLCLK");
			DEBUG("manual clearing has been triggered.");
			if(clear_clipboard(TRUE))
			{
				g_tickCount = GetTickCount64();
				PLAY_SOUND(1U);
			}
			break;
		}
		break;
	case WM_COMMAND:
		TRACE("WM_COMMAND");
		if(HIWORD(wParam) == 0)
		{
			switch(LOWORD(wParam))
			{
			case MENU1_ID:
				DEBUG("menu item #1 triggered");
				about_screen(FALSE);
				break;
			case MENU2_ID:
				DEBUG("menu item #2 triggered");
				if(clear_clipboard(TRUE))
				{
					g_tickCount = GetTickCount64();
					PLAY_SOUND(1U);
				}
				break;
			case MENU3_ID:
				DEBUG("menu item #3 triggered");
				cfg_halted = !cfg_halted;
				CheckMenuItem(g_context_menu, MENU3_ID, cfg_halted ? MF_CHECKED : MF_UNCHECKED);
				if(!update_shell_notify_icon(hWnd, cfg_halted))
				{
					DEBUG("failed to modify the shell notification icon!");
				}
				g_tickCount = GetTickCount64();
				break;
			case MENU4_ID:
				DEBUG("menu item #4 triggered");
				PostMessageW(hWnd, WM_CLOSE, 0, 0);
				break;
			}
		}
		break;
	case WM_HOTKEY:
		TRACE("WM_HOTKEY");
		if(wParam == ID_HOTKEY)
		{
			DEBUG("hotkey has been triggered.");
			if(clear_clipboard(TRUE))
			{
				g_tickCount = GetTickCount64();
				PLAY_SOUND(1U);
			}
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		if(message == g_taskbar_created)
		{
			DEBUG("taskbar has been (re)created.");
			if(!create_shell_notify_icon(hWnd, cfg_halted))
			{
				DEBUG("failed to create the shell notification icon!");
			}
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

	DEBUG("clearing clipboard...");

	for(retry = 0; retry < 32; ++retry)
	{
		if(retry > 0)
		{
			TRACE("retry!");
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
			DEBUG("format is not text --> skipped!");
		}
		else
		{
			DEBUG("cleared.");
		}
	}
	else
	{
		DEBUG("failed to clear clipboard!");
	}

	return success;
}

static BOOL is_textual_format(void)
{
	const INT result = GetPriorityClipboardFormat(g_text_formats, _countof(g_text_formats));
	TRACE2("clipboard_format=%d", result);
	return (result > 0);
}

// ==========================================================================
// Check clipboard history service
// ==========================================================================

static BOOL check_clipboard_history(void)
{
	if(!is_windows_version_or_greater(HIBYTE(WIN32_WINNT_WINTHRESHOLD), LOBYTE(WIN32_WINNT_WINTHRESHOLD), 0))
	{
		TRACE("not running on Windows 10 or later.");
		return TRUE;
	}

	if(find_running_service(L"cbdhsvc"))
	{
		DEBUG("windows clipboard history service is running!");
		if(MESSAGE_BOX(L"The \"Clipboard History\" service of Windows 10 is currently running on your machine. As long as that service is running, Windows 10 will silently keep a history (copy) of *all* data that has been copied to the clipboard at some time.\n\nIn order to keep your sensitive data private, it is *highly* recommended to disable the \"Clipboard History\" service! Do you want to disable the \"Clipboard History\" service now?", MB_ICONWARNING | MB_YESNO) == IDYES)
		{
			WCHAR *sys_path = get_system_directory();
			if(sys_path)
			{
				WCHAR *reg_path = concat_strings(sys_path, L"\\reg.exe");
				if(reg_path)
				{
					BOOL success = FALSE;
					while(!success)
					{
						DEBUG("trying to disable clipboard history service...");
						if(success = (((INT_PTR)ShellExecuteW(NULL, L"runas", reg_path, L"ADD HKLM\\SYSTEM\\CurrentControlSet\\Services\\cbdhsvc /f /v Start /t REG_DWORD /d 4", NULL, SW_HIDE)) > 32))
						{
							DEBUG("clipboard history service disable.");
							MESSAGE_BOX(L"The \"Clipboard History\" service has been disabled. Please reboot your computer for these changes to take effect!", MB_ICONINFORMATION);
						}
						else
						{
							DEBUG("failed to disable clipboard history service!");
							if(MESSAGE_BOX(L"Failed to disable the service. Retry?", MB_ICONERROR | MB_YESNO) == IDNO)
							{
								break; /*aborted*/
							}
						}
					}
					FREE(reg_path);
				}
				FREE(sys_path);
			}
			DEBUG("reboot is required --> exiting!");
			MESSAGE_BOX(L"The ClearClipboard program is going to exit for now!", MB_ICONWARNING);
		}
		else
		{
			DEBUG("clipboard history service is still running --> exiting!");
			MESSAGE_BOX(L"The ClearClipboard program is going to exit now!\n\nNote: If you want to use ClearClipboard despite the fact that the \"Clipboard History\" service is still running on your machine, pelease refer to the documentation (README file).", MB_ICONWARNING);
		}
		return FALSE;
	}

	return TRUE;
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
					cfg_debug = max(cfg_debug, 1U);
				}
				else if(!lstrcmpiW(value, L"--trace"))
				{
					cfg_debug = max(cfg_debug, 2U);
				}
#endif //_DEBUG
				else if(!lstrcmpiW(value, L"--silent"))
				{
					cfg_silent = TRUE;
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
				DEBUG("adding autorun entry to registry...");
				if(reg_write_string(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME, command_str))
				{
					DEBUG("succeeded.");
					success = TRUE;
				}
				else
				{
					DEBUG("failed to add autorun entry to registry!");
				}
				FREE(command_str);
			}
			else
			{
				DEBUG("failed to allocate string buffer!");
			}
			FREE(executable_path);
		}
		else
		{
			DEBUG("failed to determine executable path!");
		}
	}
	else
	{
		DEBUG("removing autorun entry from registry...");
		if(reg_delete_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME))
		{
			DEBUG("succeeded.");
			success = TRUE;
		}
		else
		{
			DEBUG("failed to remove autorun entry from registry!");
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
		return FALSE;
	}

	return TRUE;
}

// ==========================================================================
// About dialog
// ==========================================================================

#define ABOUT_TEXT_SHRT \
	L"ClearClipboard v" WTEXT(VERSION_STR) L" [" WTEXT(__DATE__) L"]\n" \
	L"Copyright(\x24B8) 2019 LoRd_MuldeR <mulder2@gmx.de>\n\n" \
	L"This software is released under the MIT License.\n" \
	L"(https://opensource.org/licenses/MIT)\n\n" \
	L"For news and updates please check the website at:\n" \
	L"\x2022 http://muldersoft.com/\n" \
	L"\x2022 http://muldersoft.sourceforge.net/\n\n" \
	L"Source code available from:\n" \
	L"https://github.com/lordmulder/ClearClipboard"

#define ABOUT_TEXT_FULL \
	ABOUT_TEXT_SHRT \
	L"\n\n\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\x2015\n\n" \
	L"DISCLAIMER: " \
	L"The software is provided \"as is\", without warranty of any kind, express or implied, including" \
	L"but not limited to the warranties of merchantability, fitness for a particular purpose and" \
	L"noninfringement. In no event shall the authors or copyright holders be liable for any claim," \
	L"damages or other liability, whether in an action of contract, tort or otherwise, arising from," \
	L"out of or in connection with the software or the use or other dealings in the software.\n\n" \
	L"If you agree to the license terms, click OK in order to proceed. Otherwise you must click Cancel and exit the program."

static BOOL about_screen(const BOOL first_run)
{
	MSGBOXPARAMSW params;
	SecureZeroMemory(&params, sizeof(MSGBOXPARAMSW));
	
	params.cbSize = sizeof(MSGBOXPARAMSW);
	params.hInstance = GetModuleHandle(NULL);
	params.lpszCaption = first_run ? L"License Terms" : L"About...";
	params.lpszText = first_run ? ABOUT_TEXT_FULL : ABOUT_TEXT_SHRT;
	params.lpszIcon = MAKEINTRESOURCEW(101);
	params.dwStyle = MB_SETFOREGROUND | MB_TOPMOST | MB_USERICON;
	if(first_run)
	{
		params.dwStyle |= MB_OKCANCEL | MB_DEFBUTTON2;
	}

	return (MessageBoxIndirectW(&params) == IDOK);
}

static BOOL show_disclaimer(void)
{
	static const DWORD REG_VALUE_DATA = (((DWORD)VERSION_MAJOR) << 16) | (((DWORD)VERSION_MINOR_HI) << 8) | ((DWORD)VERSION_MINOR_LO);
	static const WCHAR *const REG_VALUE_NAME = L"DisclaimerAccepted";
	static const WCHAR *const REG_VALUE_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{7816D5D9-5D9D-4B3A-B5A8-DD7A7F4C44A3}";

	if(g_config_path)
	{
		if(get_config_value(g_config_path, REG_VALUE_NAME, FALSE, FALSE, TRUE) > FALSE)
		{
			return TRUE;
		}
	}

	if(reg_read_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME, 0U) != REG_VALUE_DATA)
	{
		DEBUG("disclaimer not accepted yet...");
		if(about_screen(TRUE))
		{
			DEBUG("disclaimer accepted!");
			reg_write_value(HKEY_CURRENT_USER, REG_VALUE_PATH, REG_VALUE_NAME, REG_VALUE_DATA);
		}
		else
		{
			DEBUG("disclaimer rejected!");
			return FALSE;
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
	WCHAR *path = get_executable_path();

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
		FREE(path);
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
			FREE(buffer);
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

	FREE(buffer);
	return NULL;
}

static WCHAR *get_system_directory(void)
{
	const UINT size = GetSystemDirectoryW(NULL, 0U);
	if(size > 0U)
	{
		WCHAR *buffer = (WCHAR*) LocalAlloc(LPTR, size * sizeof(WCHAR));
		if(buffer)
		{
			const UINT result = GetSystemDirectoryW(buffer, size);
			if((result > 0U) && (result < size))
			{
				return buffer;
			}
			FREE(buffer);
		}
	}

	TRACE("failed to determine system directory!");
	return NULL;
}

static BOOL file_exists(const WCHAR *const path)
{
	const DWORD attribs = GetFileAttributesW(path);
	return (attribs != INVALID_FILE_ATTRIBUTES) && (!(attribs & FILE_ATTRIBUTE_DIRECTORY));
}

// ==========================================================================
// Configuration routines
// ==========================================================================

static int get_config_value(const WCHAR *const path, const WCHAR *const name, const int default_value, const int min_value, const int max_value)
{
	static const WCHAR *const SECTION_NAME = L"ClearClipboard";
	WCHAR buffer[32U];

	if((min_value > max_value) || (default_value < min_value) || (default_value > max_value))
	{
		RaiseException(STATUS_INVALID_PARAMETER, 0U, 0U, NULL);
		return 0U;
	}

	if(GetPrivateProfileStringW(SECTION_NAME, name, NULL, buffer, 32U, path))
	{
		int value;
		if(StrToIntExW(buffer, STIF_SUPPORT_HEX, &value))
		{
			return max(min_value, min(max_value, value));
		}
	}

	return default_value;
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
		TRACE("failed to open registry key for reading!");
		return default_value;
	}

	if(RegQueryValueExW(hkey, name, NULL, &type, (BYTE*)&result, &size) == ERROR_SUCCESS)
	{
		if((type != REG_DWORD) && (type != REG_DWORD_BIG_ENDIAN))
		{
			TRACE("registry value is not a DWORD!");
			result = default_value;
		}
		else if(type == REG_DWORD_BIG_ENDIAN)
		{
			_byteswap_ulong(result); /*cirrect endianess*/
		}
	}
	else
	{
		TRACE("failed to read registry value!");
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
		TRACE("failed to open registry key for reading!");
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
			
					TRACE("registry value is not a string!");
					FREE(buffer);
				}
				break; /*done*/
			}
			else
			{
				FREE(buffer);
				if(error != ERROR_MORE_DATA)
				{
					TRACE("failed to read registry string!");
					break; /*failed*/
				}
			}
		}
		else
		{
			TRACE("failed to allocate string buffer!");
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
		TRACE("failed to open registry key for writing!");
		return FALSE;
	}

	if(RegSetValueExW(hkey, name, 0U, REG_DWORD, (BYTE*)&value, sizeof(DWORD)) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		TRACE("failed to write registry value!");
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
		TRACE("failed to open registry key for writing!");
		return FALSE;
	}

	if(RegSetValueExW(hkey, name, 0U, REG_SZ, (BYTE*)text, (lstrlenW(text) + 1U) * sizeof(WCHAR)) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		TRACE("failed to write registry value!");
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
			TRACE("failed to open registry key for writing!");
			return FALSE;
		}
		TRACE("registry key does not exist.");
		return TRUE;
	}

	if((error = RegDeleteValueW(hkey, name)) != ERROR_SUCCESS)
	{
		if(error != ERROR_FILE_NOT_FOUND)
		{
			RegCloseKey(hkey);
			TRACE("failed to delete registry value!");
			return FALSE;
		}
		TRACE("registry value does not exist.");
	}

	RegCloseKey(hkey);
	return TRUE;
}

// ==========================================================================
// Service routines
// ==========================================================================

static BOOL find_running_service(const WCHAR *const name_prefix)
{
	const int prefix_len = lstrlenW(name_prefix);
	DWORD buffer_size = 0U, bytes_needed = 0U, services_returned = 0U, resume_handle = 0U;
	BOOL result = FALSE, success = FALSE;

	const SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if(!scm)
	{
		TRACE("failed to open service manager!");
		return FALSE;
	}

	success = EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE, NULL, 0U, &buffer_size, &services_returned, &resume_handle, NULL);
	if((success || (GetLastError() == ERROR_MORE_DATA)) && (buffer_size > 0U))
	{
		ENUM_SERVICE_STATUS_PROCESS *buffer = (ENUM_SERVICE_STATUS_PROCESS*) LocalAlloc(LPTR, buffer_size);
		if(buffer)
		{
			for(;;)
			{
				success = EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE, (BYTE*)buffer, buffer_size, &bytes_needed, &services_returned, &resume_handle, NULL);
				if(success || (GetLastError() == ERROR_MORE_DATA))
				{
					DWORD i;
					for(i = 0U; i < services_returned; ++i)
					{
						if(!StrCmpNIW(buffer[i].lpServiceName, name_prefix, prefix_len))
						{
							if((buffer[i].lpServiceName[prefix_len] == L'\0') || (buffer[i].lpServiceName[prefix_len] == L'_'))
							{
								result = TRUE;
								break;
							}
						}
					}
					if(!(success || result))
					{
						continue;
					}
				}
				else
				{
					TRACE("failed to enumerate active services!");
				}
				break; /*completed*/
			}
			FREE(buffer);
		}
		else
		{
			TRACE("failed to allocate service status buffer!");
		}
	}
	else
	{
		TRACE("failed to determine service status size!");
	}

	CloseServiceHandle(scm);
	return result;
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
	
	TRACE("failed to allocate string buffer!");
	return NULL;
}

static WCHAR *concat_strings(const WCHAR *const text_1, const WCHAR *const text_2)
{
	WCHAR *const buffer = (WCHAR*) LocalAlloc(LPTR, (lstrlenW(text_1) + lstrlenW(text_2) + 1U) * sizeof(WCHAR));
	if(buffer)
	{
		lstrcpyW(buffer, text_1);
		lstrcatW(buffer, text_2);
		return buffer;
	}
	
	TRACE("failed to allocate string buffer!");
	return NULL;
}

static void output_formatted_string(const char *const format, ...)
{
	char buffer[128U];
	va_list args;
	va_start(args, format);
	if(wvnsprintfA(buffer, 128U, format, args) > 0)
	{
		buffer[127U] = '\0';
		OutputDebugStringA(buffer);
	}
	va_end(args);
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
