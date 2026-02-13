// src/main.cpp
#include <windows.h>
#include <string>
#include <ShlObj.h>   // SHGetFolderPathW

#include "overlay.h"
#include "capture.h"
#include "uia_text.h"
#include "ocr_winrt.h"
#include "redact.h"

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

static std::wstring TruncateForOverlay(const std::wstring& s, size_t maxChars = 6500) {
  if (s.size() <= maxChars) return s;
  return s.substr(0, maxChars) + L"\r\n\r\n...[truncated]...";
}

// UIA in apps like VS Code often returns chrome/menu items, not editor/terminal text.
// Detect that and prefer OCR.
static bool LooksLikeChromeOnly(const std::wstring& t) {
  if (t.size() < 200) return true;

  const wchar_t* badTokens[] = {
    L"Minimize", L"Restore", L"Close",
    L"File", L"Edit", L"View", L"Help",
    L"Toggle", L"Explorer", L"Extensions",
    L"vscode-file://", L"Title actions", L"Side Bar",
    L"Run and Debug", L"Source Control", L"Search (Ctrl+Shift+F)"
  };

  int hits = 0;
  for (auto tok : badTokens) {
    if (t.find(tok) != std::wstring::npos) hits++;
  }
  return hits >= 5;
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

      // 3) UIA first
      std::wstring uia = ExtractTextFromWindowUIA(hwnd);

      // 4) Decide: if UIA looks like chrome (menus/buttons) -> OCR
      bool useOcr = uia.empty() || LooksLikeChromeOnly(uia);

      std::wstring rawText;
      std::wstring sourceLabel;

      if (!useOcr) {
        rawText = uia;
        sourceLabel = L"📄 Extracted text (UIA)";
      } else {
        std::wstring ocr = OcrImageFileWinRT(file);
        if (ocr.empty()) {
          overlay.SetText(
            L"✅ Saved screenshot:\r\n" + file +
            L"\r\n\r\n⚠️ UIA not useful and OCR failed.\r\n"
            L"Tip: install Windows language packs or try different capture/crop."
          );
          overlay.Show();
          continue;
        }
        rawText = ocr;
        sourceLabel = L"🔎 OCR text (WinRT)";
      }

      // 5) Redact secrets before showing/sending to AI
      std::wstring safeText = RedactSecrets(rawText);

      overlay.SetText(
        L"✅ Captured active window and saved:\r\n" + file +
        L"\r\n\r\n" + sourceLabel + L":\r\n\r\n" + TruncateForOverlay(safeText) +
        L"\r\n\r\nNext: send this text to AI and show answer here."
      );
      overlay.Show();
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  UnregisterHotKey(nullptr, HOTKEY_ID);
  return 0;
}
