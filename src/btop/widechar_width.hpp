// Minimal drop-in replacement for the "widechar_width" single-header library
// that upstream btop vendors for wcwidth(3)-style column width calculation.
//
// The OSD only uses this for text-wrapping helpers in btop_tools.cpp; a
// conservative estimate (every codepoint is 1 column wide) is sufficient for
// the monospace OSD rendering used by btop-osd.

#pragma once

#include <cwchar>

[[gnu::always_inline]] inline int widechar_wcwidth(wchar_t) { return 1; }
