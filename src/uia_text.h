#pragma once
#include <windows.h>
#include <string>

// Attempts to extract visible text from a window using UI Automation.
// Returns empty string if not available.
std::wstring ExtractTextFromWindowUIA(HWND hwnd, int maxChars = 12000);
