cmake_minimum_required(VERSION 3.20)
project(AIScreenHelper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ai_screen_helper WIN32
  src/main.cpp
  src/overlay.cpp
  src/capture.cpp
  src/uia_text.cpp
  src/ocr_winrt.cpp
  src/redact.cpp
)

target_include_directories(ai_screen_helper PRIVATE src)

# Win32 + Common Controls
target_link_libraries(ai_screen_helper PRIVATE
  user32
  gdi32
  comctl32
  windowscodecs
  ole32
  uiautomationcore
  windowsapp
)

# Needed for C++/WinRT
target_compile_options(ai_screen_helper PRIVATE /await)