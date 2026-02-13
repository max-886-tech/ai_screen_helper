// src/main.cpp
#include <windows.h>
#include <string>
#include <ShlObj.h>   // SHGetFolderPathW
#include "overlay.h"
#include "capture.h"

static constexpr int HOTKEY_ID = 1;

static std::wstring GetPicturesPath() {
  wchar_t path[MAX_PATH]{};
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, SHGFP_TYPE_CURRENT, path))) {
    return path;
  }
  // fallback to current directory
  return L".";
}

static bool RegisterBestHotkey() {
  // Ensure message queue exists (helps avoid rare failures)
  MSG qmsg{};
  PeekMessageW(&qmsg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

  struct HK { UINT mod; UINT vk; const wchar_t* name; };
  HK candidates[] = {
    { MOD_CONTROL | MOD_SHIFT, 'X', L"Ctrl+Shift+X" }, // preferred
    { MOD_CONTROL | MOD_ALT,   'X', L"Ctrl+Alt+X"   },
    { MOD_CONTROL | MOD_SHIFT, 'Z', L"Ctrl+Shift+Z" },
  };

  for (auto& hk : candidates) {
    if (RegisterHotKey(nullptr, HOTKEY_ID, hk.mod | MOD_NOREPEAT, hk.vk)) {
      return true;
    }
  }
  return false;
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

  // Start hidden; show on hotkey
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

      overlay.SetText(L"✅ Captured active window and saved:\r\n" + file +
                      L"\r\n\r\nNext: Extract text (UIA/OCR) then call AI.");
      overlay.Show();
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  UnregisterHotKey(nullptr, HOTKEY_ID);
  return 0;
}
