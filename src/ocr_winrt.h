#pragma once
#include <string>

// OCR text from an image file path (PNG/JPG/etc).
// Returns empty string on failure.
std::wstring OcrImageFileWinRT(const std::wstring& filePath);
