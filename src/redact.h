#pragma once
#include <string>

// Masks common secrets in text before displaying/sending to APIs.
std::wstring RedactSecrets(const std::wstring& input);
