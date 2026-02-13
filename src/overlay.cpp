#include "overlay.h"
#include <commctrl.h>

static const wchar_t* kOverlayClass = L"AIScreenHelperOverlay";

static void CopyToClipboard(HWND hwndOwner, const std::wstring& text) {
  if (!OpenClipboard(hwndOwner)) return;
  EmptyClipboard();

  const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!hMem) { CloseClipboard(); return; }

  void* ptr = GlobalLock(hMem);
  memcpy(ptr, text.c_str(), bytes);
  GlobalUnlock(hMem);

  SetClipboardData(CF_UNICODETEXT, hMem);
  CloseClipboard();
  // Do NOT free hMem after SetClipboardData; clipboard owns it.
}

bool OverlayWindow::Create(HINSTANCE hInst) {
  INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
  InitCommonControlsEx(&icc);

  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = OverlayWindow::WndProcThunk;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = kOverlayClass;

  if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    return false;
  }

  // Tool window + topmost overlay
  DWORD ex = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
  DWORD style = WS_POPUP | WS_THICKFRAME;

  m_hwnd = CreateWindowExW(
    ex, kOverlayClass, L"AI Helper",
    style,
    CW_USEDEFAULT, CW_USEDEFAULT, 720, 420,
    nullptr, nullptr, hInst, this
  );

  if (!m_hwnd) return false;

  // Multiline read-only edit control to display answer
  m_edit = CreateWindowExW(
    WS_EX_CLIENTEDGE, L"EDIT", L"",
    WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
    0, 0, 0, 0,
    m_hwnd, (HMENU)101, hInst, nullptr
  );

  m_btnCopy = CreateWindowExW(
    0, L"BUTTON", L"Copy",
    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    0, 0, 0, 0,
    m_hwnd, (HMENU)102, hInst, nullptr
  );

  m_btnClose = CreateWindowExW(
    0, L"BUTTON", L"Close",
    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
    0, 0, 0, 0,
    m_hwnd, (HMENU)103, hInst, nullptr
  );

  SetText(L"Press Ctrl+Shift+X to analyze.\r\n\r\nThis is the overlay. Replace the hotkey handler with capture + AI call.");
  Layout();
  return true;
}

void OverlayWindow::Show() {
  ShowWindow(m_hwnd, SW_SHOW);
  SetForegroundWindow(m_hwnd);
}

void OverlayWindow::Hide() {
  ShowWindow(m_hwnd, SW_HIDE);
}

void OverlayWindow::SetText(const std::wstring& text) {
  m_text = text;
  if (m_edit) SetWindowTextW(m_edit, m_text.c_str());
}

void OverlayWindow::Layout() {
  if (!m_hwnd) return;

  RECT rc{};
  GetClientRect(m_hwnd, &rc);

  const int pad = 12;
  const int btnH = 34;
  const int btnW = 100;
  const int gap = 10;

  int width = rc.right - rc.left;
  int height = rc.bottom - rc.top;

  int btnY = height - pad - btnH;
  int closeX = width - pad - btnW;
  int copyX = closeX - gap - btnW;

  MoveWindow(m_btnClose, closeX, btnY, btnW, btnH, TRUE);
  MoveWindow(m_btnCopy,  copyX,  btnY, btnW, btnH, TRUE);

  int editH = btnY - pad - pad;
  MoveWindow(m_edit, pad, pad, width - pad * 2, editH, TRUE);
}

LRESULT CALLBACK OverlayWindow::WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  OverlayWindow* self = nullptr;

  if (msg == WM_NCCREATE) {
    auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
    self = reinterpret_cast<OverlayWindow*>(cs->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    self->m_hwnd = hwnd;
  } else {
    self = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  }

  if (self) return self->WndProc(hwnd, msg, wParam, lParam);
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_SIZE:
      Layout();
      return 0;

    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == 102) { // Copy
        CopyToClipboard(hwnd, m_text);
        return 0;
      }
      if (id == 103) { // Close
        Hide();
        return 0;
      }
      break;
    }

    case WM_CLOSE:
      Hide();
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}
