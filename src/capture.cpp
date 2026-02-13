#include "capture.h"
#include <wincodec.h>
#include <vector>

static bool EnsureCOM() {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
}

HBITMAP CaptureForegroundWindowBitmap(HWND& outHwnd) {
  outHwnd = GetForegroundWindow();
  if (!outHwnd) return nullptr;

  RECT rc{};
  GetClientRect(outHwnd, &rc);
  int w = rc.right - rc.left;
  int h = rc.bottom - rc.top;
  if (w <= 0 || h <= 0) return nullptr;

  HDC hdcWindow = GetDC(outHwnd);
  HDC hdcMem = CreateCompatibleDC(hdcWindow);
  HBITMAP hbmp = CreateCompatibleBitmap(hdcWindow, w, h);
  HGDIOBJ old = SelectObject(hdcMem, hbmp);

  // Try to capture with PrintWindow (works for many apps)
  BOOL ok = PrintWindow(outHwnd, hdcMem, PW_RENDERFULLCONTENT);
  if (!ok) {
    // Fallback to BitBlt
    BitBlt(hdcMem, 0, 0, w, h, hdcWindow, 0, 0, SRCCOPY);
  }

  SelectObject(hdcMem, old);
  DeleteDC(hdcMem);
  ReleaseDC(outHwnd, hdcWindow);

  return hbmp;
}

bool SaveHBitmapToPngFile(HBITMAP hbmp, const std::wstring& filePath) {
  if (!hbmp) return false;
  EnsureCOM();

  BITMAP bm{};
  if (!GetObject(hbmp, sizeof(bm), &bm)) return false;

  // Prepare DIB buffer (32-bit BGRA)
  BITMAPINFO bi{};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = bm.bmWidth;
  bi.bmiHeader.biHeight = -bm.bmHeight; // top-down
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;

  std::vector<BYTE> pixels((size_t)bm.bmWidth * (size_t)bm.bmHeight * 4);

  HDC hdc = GetDC(nullptr);
  int got = GetDIBits(hdc, hbmp, 0, (UINT)bm.bmHeight, pixels.data(), &bi, DIB_RGB_COLORS);
  ReleaseDC(nullptr, hdc);
  if (got == 0) return false;

  IWICImagingFactory* factory = nullptr;
  HRESULT hr = CoCreateInstance(
    CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
    IID_PPV_ARGS(&factory)
  );
  if (FAILED(hr) || !factory) return false;

  IWICStream* stream = nullptr;
  hr = factory->CreateStream(&stream);
  if (FAILED(hr) || !stream) { factory->Release(); return false; }

  hr = stream->InitializeFromFilename(filePath.c_str(), GENERIC_WRITE);
  if (FAILED(hr)) { stream->Release(); factory->Release(); return false; }

  IWICBitmapEncoder* encoder = nullptr;
  hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
  if (FAILED(hr) || !encoder) { stream->Release(); factory->Release(); return false; }

  hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
  if (FAILED(hr)) { encoder->Release(); stream->Release(); factory->Release(); return false; }

  IWICBitmapFrameEncode* frame = nullptr;
  IPropertyBag2* props = nullptr;
  hr = encoder->CreateNewFrame(&frame, &props);
  if (FAILED(hr) || !frame) { encoder->Release(); stream->Release(); factory->Release(); return false; }

  hr = frame->Initialize(props);
  if (props) props->Release();
  if (FAILED(hr)) { frame->Release(); encoder->Release(); stream->Release(); factory->Release(); return false; }

  hr = frame->SetSize(bm.bmWidth, bm.bmHeight);
  if (FAILED(hr)) { frame->Release(); encoder->Release(); stream->Release(); factory->Release(); return false; }

  // We provide 32bpp BGRA
  WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
  hr = frame->SetPixelFormat(&format);
  if (FAILED(hr)) { frame->Release(); encoder->Release(); stream->Release(); factory->Release(); return false; }

  const UINT stride = bm.bmWidth * 4;
  const UINT imageSize = stride * bm.bmHeight;

  hr = frame->WritePixels(bm.bmHeight, stride, imageSize, pixels.data());
  if (FAILED(hr)) { frame->Release(); encoder->Release(); stream->Release(); factory->Release(); return false; }

  hr = frame->Commit();
  if (SUCCEEDED(hr)) hr = encoder->Commit();

  frame->Release();
  encoder->Release();
  stream->Release();
  factory->Release();

  return SUCCEEDED(hr);
}
