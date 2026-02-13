// src/main.cpp
#include <windows.h>
#include <string>
#include <ShlObj.h>   // SHGetFolderPathW

#include "overlay.h"
#include "capture.h"
#include "uia_text.h"
#include "ocr_winrt.h"

static constexpr int HOTKEY_ID = 1;

static std::wstring GetPicturesPath() {
  wchar_t path[MAX_PATH]{};
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, SHGFP_TYPE_CURRENT, path))) {
    return path;
  }
  return L".";
}

static bool RegisterBestHotkey() {
  // Ensure message queue exists
  MSG qmsg{};
  PeekMessageW(&qmsg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

  struct HK { UINT mod; UINT vk; };
  HK candidates[] = {
    { MOD_CONTROL | MOD_SHIFT, 'X' },
    { MOD_CONTROL | MOD_ALT,   'X' },
    { MOD_CONTROL | MOD_SHIFT, 'Z' },
  };

  for (auto& hk : candidates) {
    if (RegisterHotKey(nullptr, HOTKEY_ID, hk.mod | MOD_NOREPEAT, hk.vk)) {
      return true;
    }
  }
  return false;
}

static std::wstring TruncateForOverlay(const std::wstring& s, size_t maxChars = 6000) {
  if (s.size() <= maxChars) return s;
  return s.substr(0, maxChars) + L"\r\n\r\n...[truncated]...";
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
  OverlayWindow overlay;
  if (!overlay.Create(hInst)) {
    MessageBoxW(nullptr, L"Failed to create overlay window.", L"Error", MB_ICONERROR);
    return 1;
  }

  if (!RegisterBestHotkey()) {
    DWORD err = GetLastError();
    wchar_t buf[256];
    wsprintfW(buf, L"Failed to register hotkey.\nGetLastError=%lu", err);
    MessageBoxW(nullptr, buf, L"Error", MB_ICONERROR);
    return 1;
  }

  overlay.Hide();

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID) {

      // 1) Capture active window
      HWND hwnd{};
      HBITMAP hbmp = CaptureForegroundWindowBitmap(hwnd);
      if (!hbmp) {
        overlay.SetText(L"❌ Failed to capture foreground window.");
        overlay.Show();
        continue;
      }

      // 2) Save to PNG
      std::wstring file = GetPicturesPath() + L"\\ai_capture.png";
      bool ok = SaveHBitmapToPngFile(hbmp, file);
      DeleteObject(hbmp);

      if (!ok) {
        overlay.SetText(L"❌ Capture OK, but failed to save PNG.");
        overlay.Show();
        continue;
      }

      // 3) Try UIA extraction first
      std::wstring uia = ExtractTextFromWindowUIA(hwnd);

      // If UIA is tiny, it's usually useless for VS Code / browsers
      bool uiaUseful = (uia.size() >= 200);

      std::wstring content;
      if (uiaUseful) {
        content = L"📄 Extracted text (UIA):\r\n\r\n" + TruncateForOverlay(uia);
      } else {
        // 4) OCR fallback on the saved image
        std::wstring ocr = OcrImageFileWinRT(file);
        if (ocr.empty()) {
          content =
            L"⚠️ UIA text not useful for this window.\r\n"
            L"❌ OCR failed (WinRT).\r\n"
            L"Next: verify Windows OCR language packs / try different capture method.";
        } else {
          content = L"🔎 OCR text (WinRT):\r\n\r\n" + TruncateForOverlay(ocr);
        }
      }

      overlay.SetText(
        L"✅ Captured active window and saved:\r\n" + file +
        L"\r\n\r\n" + content
      );
      overlay.Show();
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  UnregisterHotKey(nullptr, HOTKEY_ID);
  return 0;
}
