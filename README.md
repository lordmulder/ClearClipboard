% ClearClipboard
% Periodically ClearClipboard, e.g. to hide sensitive information  

# Introduction

**ClearClipboard** is a simple tool that will *periodically clear* your clipboard. This is useful, for example, to hide sensitive information, such as passwords. The clear timeout can be configured freely and defaults to 30 seconds. Also, the clipboard will *not* simply be cleared at a fixed interval. Instead, the clear timer will be *reset* every time that new content is copied to the clipboard. This ensures that only "stale" content will be cleared; recently copied content will *never* be cleared away.

*Note:* The ClearClipboard program runs "hidden" in the background. However, there will be an icon in the [notification area](https://docs.microsoft.com/en-us/windows/desktop/uxguide/winenv-notification), which can be used to control or terminate ClearClipboard. Only one instance of ClearClipboard can be running at a time.


# System requirements

ClearClipboard runs on Windows Vista or newer. The "x64" version requires a 64-Bit version of Windows Vista or newer.

Windows XP is **not** supported due to the lack of the `AddClipboardFormatListener` system function!

## Windows 10 Warning

Windows 10 contains some "problematic" features that can put a risk on sensitive information copied to the clipboard:

* The first of those features is called "Clipboard History", which will silently keep a history (copy) of *all* data that has been copied to clipboard at some time. This history will persist even after the clipboard has been cleared!

* The second feature is called "Automatic Syncing" (Cloud Clipboard), which will automatically upload *all* data that is copied to clipboard to the Microsoft cloud servers – purportedly to synchronize the clipboard between your devices!

We ***highly*** recommend to *disable* both of these features in order to allow ClearClipboard to function as expected. You can easily do this in the "Windows Settings" on the "System/Clipboard" page. See [here](https://www.tenforums.com/tutorials/109799-turn-off-clipboard-history-windows-10-a.html) and [here](https://www.tenforums.com/tutorials/110048-enable-disable-clipboard-sync-across-devices-windows-10-a.html) for more information!


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
  Enable debug outputs (verbose mode). You can use the [DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview) tool to show the generated debug messages.

* **`--slunk`**  
  Enable slunk mode for improved user experience. Check it out!


# Configuration File

The behavior of the ClearClipboard program can be adjusted via a configuration file, in the [INI format](https://en.wikipedia.org/wiki/INI_file).

The configuration file must be located in the same directory as the ClearClipboard executable. Also, it must have the same file name as the ClearClipboard executable, except that the file extension is replaced by **`.ini`**. The default configuration file name therefore is **`ClearClipboard.ini`**. All parameters need to be located in the **`[ClearClipboard]`** section.

The following configuration parameters are supported:

* **`Timeout=<msec>`**  
  Specifies the timeout for automatic clipboard clearing, in milliseconds. Default: `30000`.

* **`Sound=<0|1|2>`**  
  Controls sound effects. Mode `1` plays a sound, when the clipboard is cleared manually. Mode `2` additionally plays a sound every time that the clipboard is cleared automatically. And mode `0` disables all sounds. Default: `1`.

* **`Halted=<0|1>`**  
  If this parameter is set to `1`, ClearClipboard starts in "halted" mode, i.e. with automatic clearing paused. Default: `0`.

* **`DisableWarningMessages=<0|1>`**
  If this parameter is set to `1`, ClearClipboard will *not* warn about "problematic" Windows 10 features. Default: `0`.

## Example Configuration

An example configuration file:

	[ClearClipboard]
	Timeout=30000
	Sound=1
	Halted=0

## Sound File

ClearClipboard uses the "Empty Recycle Bin" system sound, as configured in the control panel (`control mmsys.cpl`). If that sound file is *not* found (or *not* configured), ClearClipboard will fall back to the "Asterisk" system sound.


# Updates & Source Code

Please check the official web-site at **<http://muldersoft.com/>** or **<http://muldersoft.sourceforge.net/>** for updates!

The source code of ClearClipboard is available from our public Git repository, mirrored at:
* `git clone https://github.com/lordmulder/ClearClipboard.git` ([Browse](https://github.com/lordmulder/ClearClipboard))
* `git clone https://muldersoft@bitbucket.org/muldersoft/clearclipboard.git` ([Browse](https://bitbucket.org/muldersoft/clearclipboard/))
* `git clone https://repo.or.cz/ClearClipboard.git` ([Browse](https://repo.or.cz/ClearClipboard.git))

# Version History

## Version 1.03 [2019-05-31]

* Clipboard can be cleared *manually* by double-click on the shell notification icon or from the context menu.

* Optionally, a sound file can be played whenever the clipboard is cleared.

* Automatic clipboard clearing can now be halted (suspended) or resumed at any time via the context menu.

* The additional configuration parameters `Sound` and `Halted` are supported now.

* Show message when "autorun" entry has been created or removed. Use option `--silent` to suppress.

* Detection of "problematic" Windows 10 features (*Clipboard History* and *Cloud Clipboard Sync*)

* Added "x64" (64-Bit) binaries of the ClearClipboard program. Note: Requires 64-Bit Windows edition to run.

* Some fixes and improvements.

## Version 1.02 [2019-05-26]

* Added shell notification icon that can be used to control the ClearClipboard program.

* Some fixes and improvements.

## Version 1.01 [2019-05-25]

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
