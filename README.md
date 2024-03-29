% 📋 ClearClipboard
% Periodically clear the clipboard, e.g. to hide sensitive information


# Introduction

**ClearClipboard** is a simple tool that will *periodically clear* your clipboard. This is useful, for example, to hide sensitive information, such as passwords. The clear timeout can be configured freely and defaults to 30 seconds. Also, the clipboard will *not* simply be cleared at a fixed interval. Instead, the clear timer will be *reset* every time that new content is copied to the clipboard. This ensures that only "stale" content will be cleared; recently copied content will *never* be cleared away.

*Note:* The ClearClipboard program runs "hidden" in the background. However, there will be an icon in the [notification area](https://docs.microsoft.com/en-us/windows/desktop/uxguide/winenv-notification), which can be used to control or terminate ClearClipboard. Only one instance of ClearClipboard can be running at a time.


# System requirements

ClearClipboard runs on Windows Vista or newer. The "x64" version requires a 64-Bit version of Windows Vista or newer.

Windows XP is **not** supported, i.a., due to the lack of the `AddClipboardFormatListener` system function!

## Windows 10 Warning

Windows 10 contains some "problematic" features that can put a risk on sensitive information copied to the clipboard:

* The first of those features is called [*Clipboard History*](https://www.tenforums.com/tutorials/109799-turn-off-clipboard-history-windows-10-a.html), which will silently keep a history (copy) of *all* data that has been copied to clipboard at some time. This history will persist even after the clipboard has been cleared!

* The second feature is called [*Automatic Syncing*](https://www.tenforums.com/tutorials/110048-enable-disable-clipboard-sync-across-devices-windows-10-a.html) (Cloud Clipboard), which will automatically upload *all* data that is copied to clipboard to the Microsoft cloud servers – purportedly to synchronize the clipboard between your devices!

We ***highly*** recommend to *disable* both of these features in order to keep your data safe and allow ClearClipboard to function as expected. To the best of our knowledge, the most reliable way to achieve this is to completely *disabled* the "Clipboard History" (`cbdhsvc`) system service. ClearClipboard will now detect whether the "problematic" service is running on your system, and if so, offer to disable that service. Note that a *reboot* will be required in order to make the changes take effect.

### Registry Hacks

Optionally, you can *disable* the "Clipboard History" service with the following **`.reg`** file:

	Windows Registry Editor Version 5.00

	[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\cbdhsvc]
	"Start"=dword:00000004
	[HKEY_CURRENT_USER\Software\Microsoft\Clipboard]
	"EnableClipboardHistory"=dword:00000000


Use this **`.reg`** file, if you ever whish to *re-enable* the "Clipboard History" service:

	Windows Registry Editor Version 5.00

	[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\cbdhsvc]
	"Start"=dword:00000003
	[HKEY_CURRENT_USER\Software\Microsoft\Clipboard]
	"EnableClipboardHistory"=dword:00000001

As always, a reboot will be required in order to make the above registry "hacks" take effect!


# Command-line Options

The ClearClipboard program supports the following *mutually exclusive* command-line options:

* **`--close`**  
  Close the running instance of ClearClipboard, if ClearClipboard is currently running. Does nothing, otherwise.

* **`--restart`**  
  Start a new instance of ClearClipboard. If ClearClipboard is already running, the running instance is closed.

* **`--install`**  
  Add "autorun" entry for ClearClipboard to the registry, so that ClearClipboard runs *automatically* at system startup.

* **`--uninstall`**  
  Remove the "autorun" entry for ClearClipboard from the registry, if it currently exists. Does nothing, otherwise.

In addition, one or more of the following options may be appended to the command-line:

* **`--silent`**  
  Suppress message boxes. Use, e.g., in combination with `--install` or `--uninstall` command.

* **`--debug`**  
  Enable diagnostic output. You can use the [DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview) tool from Sysinternals to show the generated messages.

* **`--trace`**  
  Enable more verbose diagnostic output. Not normally recommended, as the system performance may be degraded.

* **`--slunk`**  
  Enable slunk mode for improved user experience. Check it out!


# Configuration File

The behavior of the ClearClipboard program can be adjusted via a configuration file, in the [INI format](https://en.wikipedia.org/wiki/INI_file).

The configuration file must be located in the same directory as the ClearClipboard executable. Also, it must have the same file name as the ClearClipboard executable, except that the file extension is replaced by **`.ini`**. The default configuration file name therefore is **`ClearClipboard.ini`**. All parameters need to be located in the **`[ClearClipboard]`** section.

The following configuration parameters are supported:

* **`Timeout=<msec>`**  
  Specifies the timeout for automatic clipboard clearing, in milliseconds. Default: `30000`.

* **`TextOnly=<0|1>`**  
  If this parameter is set to `1`, ClearClipboard *only* clears the clipboard automatically, if it contains textual data. Otherwise, automatically clearing happens regardless of the current data format. Manual clearing is *not* effected. Default: `0`.
  > Recognized textual formats include the *standard* text formats (`CF_TEXT`, `CF_OEMTEXT`, `CF_UNICODETEXT` and `CF_DSPTEXT`) as well as the most common "registered" formats for textual data. See [here](https://www.codeproject.com/Reference/1091137/Windows-Clipboard-Formats) for details.

* **`Sound=<0|1|2>`**  
  Controls sound effects. Mode `1` plays a sound, when the clipboard is cleared manually. Mode `2` additionally plays a sound every time that the clipboard is cleared automatically. And mode `0` disables all sounds. Default: `1`.
  > ClearClipboard uses the *"Empty Recycle Bin"* system sound, as set up in the control panel (`control mmsys.cpl`). If that sound file is *not* found or was set to "None", ClearClipboard will fall back to the "Asterisk" default sound.

* **`Hotkey=<key_id>`**  
  Specifies a system-wide hotkey (shortcut) to immediately clear the clipboard. The hotkey is specified as a three-digit *hexadecimal* number in the **`0xMNN`** format: `M` is the one-digit *modifier*, and `NN` is the two-digit *virtual-key code*. Allowed *modifiers* include Alt-key (`0x1`), Ctrl-key (`0x2`), Shift-key (`0x4`) and Win-key (`0x8`). Default: disabled.
  > Multiple modifiers can be combined by adding up the corresponding numbers, in hexadecimal numeral system. The *virtual-key code* can be any one defined in the [Virtual-Key Codes](https://docs.microsoft.com/en-us/windows/desktop/inputdev/virtual-key-codes) table, except for codes smaller than `0x08`. For example, in order to use the combination »Alt+Ctrl+B« as your hotkey, you have to specify the value `0x342`. That is because the virtual-key code for »B« is `0x42`, and `0x1` (i.e Alt-key) plus `0x2` (i.e. Ctrl-key) makes `0x3`.

* **`Halted=<0|1>`**  
  If this parameter is set to `1`, ClearClipboard starts in "halted" mode, i.e. with automatic clearing paused. Default: `0`.

* **`DisableWarningMessages=<0|1>`**  
  If this parameter is set to `1`, ClearClipboard will *not* warn about "problematic" Windows features or other programs that may expose your data to a risk. It is *not* recommended to do this, except for debugging purposes! Default: `0`.

* **`HideNotificationIcon=<0|1>`**  
  If this parameter is set to `1`, ClearClipboard will *not* create an icon in the notification area. The periodic clearing of the clipboard will work as usual, but the only way to exit ClearClipboard will be via the Task Manager. Default: `0`.

## Example Configuration

An example configuration file:

	[ClearClipboard]
	Timeout=90000
	TextOnly=1
	Sound=2
	Hotkey=0x342
	Halted=1


# Updates & Source Code

Please check the official web-site at **<http://muldersoft.com/>** or **<http://muldersoft.sourceforge.net/>** for updates!

The source code of ClearClipboard is available from our public Git repository, mirrored at:
* `git clone https://github.com/lordmulder/ClearClipboard.git` ([Browse](https://github.com/lordmulder/ClearClipboard))
* `git clone https://muldersoft@bitbucket.org/muldersoft/clearclipboard.git` ([Browse](https://bitbucket.org/muldersoft/clearclipboard/))
* `git clone https://repo.or.cz/ClearClipboard.git` ([Browse](https://repo.or.cz/ClearClipboard.git))


# Version History

## Version 1.07 [2019-06-15] {-}

* Slightly improved detection of "textual" data on the clipboard.

* Implemented detection of "Ditto", a popular clipboard manager for Microsoft Windows.

* Some fixes and improvements.

## Version 1.06 [2019-06-05] {-}

* Optional system-wide *hotkey* to immediately clear the clipboard. Use parameter `Hotkey` to configure.

* Allow more fine-grained control of diagnostic output.

* Some fixes and improvements.

## Version 1.05 [2019-06-02] {-}

* Detect whether the Windows 10 "Clipboard History" service is running, and, if so, suggest to disable.

* Some fixes and improvements.

## Version 1.04 [2019-06-01] {-}

* Optionally restrict the automatic clearing to *textual* data. Use parameter `TextOnly` to configure.

* Some fixes and improvements.

## Version 1.03 [2019-05-31] {-}

* Clipboard can be cleared *manually* by double-click on the shell notification icon or from the context menu.

* Optionally, a sound file can be played whenever the clipboard is cleared.

* Automatic clipboard clearing can now be halted (suspended) or resumed at any time via the context menu.

* The additional configuration parameters `Sound` and `Halted` are supported now.

* Show message when "autorun" entry has been created or removed. Use option `--silent` to suppress.

* Detection of "problematic" Windows 10 features (*Clipboard History* and *Cloud Clipboard Sync*)

* Added "x64" (64-Bit) binaries of the ClearClipboard program. Note: Requires 64-Bit Windows edition to run.

* Some fixes and improvements.

## Version 1.02 [2019-05-26] {-}

* Added shell notification icon that can be used to control the ClearClipboard program.

* Some fixes and improvements.

## Version 1.01 [2019-05-25] {-}

* First public release.


# License

**Copyright(&#9400;) 2019 LoRd_MuldeR &lt;mulder2@gmx.de&gt;, released under the MIT License.**  

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software
	and associated documentation files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or
	substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
	BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
	DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

<https://opensource.org/licenses/MIT>

&nbsp;  
e.o.f.
