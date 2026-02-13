#include "ocr_winrt.h"

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>

using namespace winrt;

std::wstring OcrImageFileWinRT(const std::wstring& filePath) {
  try {
    winrt::init_apartment(apartment_type::single_threaded);

    // StorageFile from path
    auto file = Windows::Storage::StorageFile::GetFileFromPathAsync(filePath).get();
    auto stream = file.OpenReadAsync().get();

    // Decode bitmap
    auto decoder = Windows::Graphics::Imaging::BitmapDecoder::CreateAsync(stream).get();
    auto softwareBitmap = decoder.GetSoftwareBitmapAsync().get();

    // Create OCR engine (user profile language)
    auto engine = Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
    if (!engine) return L"";

    // OCR
    auto result = engine.RecognizeAsync(softwareBitmap).get();
    auto text = result.Text();

    return std::wstring(text.c_str());
  } catch (...) {
    return L"";
  }
}
