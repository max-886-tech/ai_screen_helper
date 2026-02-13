#include <windows.h>
#include <string>
#include "overlay.h"

static constexpr int HOTKEY_ID = 1;

// Placeholder: replace this with (capture + text extraction + AI call)
static std::wstring BuildDemoAnswer() {
  return
    L"✅ Captured hotkey event.\r\n\r\n"
    L"Next steps to implement Phase 1:\r\n"
    L"1) Capture active window image (PrintWindow / DXGI)\r\n"
    L"2) Extract text (UIA first, OCR fallback)\r\n"
    L"3) Send minimal prompt to your AI endpoint\r\n"
    L"4) Display response here\r\n\r\n"
    L"Tip: Add a 'Preview what will be sent' toggle for privacy.";
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
  OverlayWindow overlay;
  if (!overlay.Create(hInst)) {
    MessageBoxW(nullptr, L"Failed to create overlay window.", L"Error", MB_ICONERROR);
    return 1;
  }

  // Register Ctrl+Shift+X
  if (!RegisterHotKey(nullptr, HOTKEY_ID, MOD_CONTROL | MOD_SHIFT, 'X')) {
    MessageBoxW(nullptr, L"Failed to register hotkey Ctrl+Shift+X.", L"Error", MB_ICONERROR);
    return 1;
  }

  // Start hidden; show on hotkey
  overlay.Hide();

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID) {
      // Toggle overlay + update text
      overlay.SetText(BuildDemoAnswer());
      overlay.Show();
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  UnregisterHotKey(nullptr, HOTKEY_ID);
  return 0;
}
