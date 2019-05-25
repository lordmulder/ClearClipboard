% Clear Clipboard
% Periodically clear clipboard, e.g. to hide sensitive infomration  

# Introduction

**Clear Clipboard** is a simple tool that will *periodically clear* your clipboard. This is useful, for example, to hide sensitive information, such as passwords. The clear timeout can be configured freely and defaults to 30 seconds. Also, the clipboard will *not* simply be cleard at a fixed interval. Instead, the clear timer will be *reset* everytime that new contentent is copied to the clipboard. This ensures that only "stale" content will be cleared; recently copied content will *never* be cleared away.

*Note:* Clear Clipboard runs "hidden" in the background. Only one instance of Clear Clipboard can be running at a time.

# Command-line Options

The following command-line options are available:

* **`--close`**  
  Close the running instance of Clear Clipboard, if it exists, then exit.

* **`--restart`**  
  Start a new instance of Clear Clipboard; the existing instance will be closed, if it exists.

* **`--install`**  
  Add an autorun entry for Clear Clipboard to the registry, so that it will be run automatically at system startup.

* **`--uninstall`**  
  Remove the autorun entry for Clear Clipboard from the registry, if it exists.

* **`--debug`**  
  Enable additional debug outputs. Use the [DebugView](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview) tool to show the generated debug outputs.

# Configuration File

The configuration file **`ClearClipboard.ini`**, which must be located in the same directory as the executable file, can be used to adjust the behavior of Clear Clipboard. Currently, this file can be used to specify the clear timeout, in milliseconds:

	[ClearClipboard]
	Timeout=90000

# License

**Copyright(&#9400;) 2019 LoRd_MuldeR &lt;mulder2@gmx.de&gt;, released under the MIT License.**  
**Check <http://muldersoft.com/> or <http://muldersoft.sourceforge.net/> for updates!**

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
