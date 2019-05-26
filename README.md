% Clear Clipboard
% Periodically clear clipboard, e.g. to hide sensitive infomration  

# Introduction

**Clear Clipboard** is a simple tool that will *periodically clear* your clipboard. This is useful, for example, to hide sensitive information, such as passwords. The clear timeout can be configured freely and defaults to 30 seconds. Also, the clipboard will *not* simply be cleard at a fixed interval. Instead, the clear timer will be *reset* everytime that new contentent is copied to the clipboard. This ensures that only "stale" content will be cleared; recently copied content will *never* be cleared away.

*Note:* Clear Clipboard runs "hidden" in the background. Only one instance of Clear Clipboard can be running at a time.


# Command-line Options

The following *mutually exclusive* commands are supported by the Clear Clipboard program:

* **`--close`**  
  Close the running instance of Clear Clipboard, if Clear Clipboard is currently running. Does nothing, otherwise.

* **`--restart`**  
  Start a new instance of Clear Clipboard. If Clear Clipboard is already running, the running instance is closed.

* **`--install`**  
  Add "autorun" entry for Clear Clipboard to the registry, so that Clear Clipboard runs *automatically* at system startup.

* **`--uninstall`**  
  Remove the "autorun" entry for Clear Clipboard from the registry, if it currently exists. Does nothing, otherwise.

Additionally, the follwing options can be added:

* **`--debug`**  
  Enable additional debug outputs. Use the [DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview) tool to show the generated debug outputs.

* **`--slunk`**  
  Enable slunk mode for improved user experience.


# Configuration File

The configuration file **`ClearClipboard.ini`**, which must be located in the same directory as the executable file, can be used to adjust the behavior of Clear Clipboard. Currently, this file can be used to specify the clear timeout, in milliseconds.

***Example:***

	[ClearClipboard]
	Timeout=90000

# Version History

## Version 1.02 [2019-05-26]

* Added shell notification icon that can be used to control the Clear Clipboard program.

* Some fixes and improvements.

## Version 1.01 [2019-05-25]

* First public releease.


# License

**Copyright(&#9400;) 2019 LoRd_MuldeR &lt;mulder2@gmx.de&gt;, released under the MIT License.**  
**Check <http://muldersoft.com/> or <https://github.com/lordmulder/ClearClipboard> for updates!**

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
