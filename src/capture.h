#pragma once
#include <windows.h>
#include <string>

// Captures foreground window client area into an HBITMAP.
// Caller must DeleteObject() the returned HBITMAP.
HBITMAP CaptureForegroundWindowBitmap(HWND& outHwnd);

// Saves an HBITMAP to a PNG file using WIC.
bool SaveHBitmapToPngFile(HBITMAP hbmp, const std::wstring& filePath);
