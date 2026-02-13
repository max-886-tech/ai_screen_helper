#include "uia_text.h"
#include <UIAutomation.h>
#include <vector>

static void EnsureCOM() {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  (void)hr;
}

static std::wstring BstrToWString(BSTR b) {
  if (!b) return L"";
  std::wstring s(b, SysStringLen(b));
  SysFreeString(b);
  return s;
}

std::wstring ExtractTextFromWindowUIA(HWND hwnd, int maxChars) {
  if (!hwnd) return L"";
  EnsureCOM();

  IUIAutomation* uia = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&uia));
  if (FAILED(hr) || !uia) return L"";

  IUIAutomationElement* root = nullptr;
  hr = uia->ElementFromHandle(hwnd, &root);
  if (FAILED(hr) || !root) { uia->Release(); return L""; }

  // Find all descendants that have a Name (visible text) or Value pattern.
  IUIAutomationCondition* condTrue = nullptr;
  uia->CreateTrueCondition(&condTrue);

  IUIAutomationElementArray* arr = nullptr;
  hr = root->FindAll(TreeScope_Subtree, condTrue, &arr);

  std::wstring out;
  if (SUCCEEDED(hr) && arr) {
    int len = 0;
    arr->get_Length(&len);

    for (int i = 0; i < len; i++) {
      IUIAutomationElement* el = nullptr;
      if (FAILED(arr->GetElement(i, &el)) || !el) continue;

      // 1) Try Name property
      BSTR bname = nullptr;
      el->get_CurrentName(&bname);
      std::wstring name = BstrToWString(bname);

      // 2) Try ValuePattern
      std::wstring val;
      IUIAutomationValuePattern* vp = nullptr;
      if (SUCCEEDED(el->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&vp))) && vp) {
        BSTR bval = nullptr;
        if (SUCCEEDED(vp->get_CurrentValue(&bval))) {
          val = BstrToWString(bval);
        }
        vp->Release();
      }

      // Heuristic: append non-trivial strings, avoid duplicates & noise
      auto append = [&](const std::wstring& s) {
        if (s.size() < 3) return;
        if (out.size() + s.size() + 2 > (size_t)maxChars) return;
        // Avoid too much repetition
        if (out.find(s) != std::wstring::npos) return;
        out.append(s).append(L"\r\n");
      };

      append(name);
      append(val);

      el->Release();

      if ((int)out.size() >= maxChars) break;
    }

    arr->Release();
  }

  if (condTrue) condTrue->Release();
  root->Release();
  uia->Release();

  // Trim very small output
  if (out.size() < 40) return L"";
  return out;
}
