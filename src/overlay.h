#pragma once
#include <windows.h>
#include <string>

class OverlayWindow {
public:
  OverlayWindow() = default;

  bool Create(HINSTANCE hInst);
  void Show();
  void Hide();
  void SetText(const std::wstring& text);

  HWND Hwnd() const { return m_hwnd; }

private:
  static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  void Layout();

  HWND m_hwnd = nullptr;
  HWND m_edit = nullptr;
  HWND m_btnCopy = nullptr;
  HWND m_btnClose = nullptr;

  std::wstring m_text;
};
