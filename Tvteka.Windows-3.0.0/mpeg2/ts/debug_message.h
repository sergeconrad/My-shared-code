/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef DEBUG_MESSAGE_H
#define DEBUG_MESSAGE_H

#ifdef _DEBUG

#include <Windows.h>
#include <iomanip>
#include <sstream>

// special case for windows phone 8 dll
// use OutputDebugStringW instead of OutputDebugString

#define DEBUG_MESSAGE(s) \
		{ \
		std::wostringstream _os_; _os_ << s;  _os_ << "\n"; \
		OutputDebugStringW(_os_.str().c_str()); \
		}
#else

#define DEBUG_MESSAGE(s) {} \

#endif
#endif