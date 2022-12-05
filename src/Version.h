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

// Version
#define VERSION_MAJOR		1
#define VERSION_MINOR_HI	0
#define VERSION_MINOR_LO	7
#define VERSION_PATCH		2

// Version string helper
#if VERSION_PATCH > 0
#define ___VERSION_STR___(X) #X
#define __VERSION_STR__(W,X,Y,Z) ___VERSION_STR___(W.X##Y-Z)
#define _VERSION_STR_(W,X,Y,Z) __VERSION_STR__(W,X,Y,Z)
#define VERSION_STR _VERSION_STR_(VERSION_MAJOR, VERSION_MINOR_HI, VERSION_MINOR_LO, VERSION_PATCH)
#else
#define ___VERSION_STR___(X) #X
#define __VERSION_STR__(X,Y,Z) ___VERSION_STR___(X.Y##Z)
#define _VERSION_STR_(X,Y,Z) __VERSION_STR__(X,Y,Z)
#define VERSION_STR _VERSION_STR_(VERSION_MAJOR, VERSION_MINOR_HI, VERSION_MINOR_LO)
#endif
