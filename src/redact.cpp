#include "redact.h"
#include <regex>

static std::wstring ReplaceAllRegex(const std::wstring& in, const std::wregex& re, const std::wstring& repl) {
  return std::regex_replace(in, re, repl);
}

std::wstring RedactSecrets(const std::wstring& input) {
  std::wstring out = input;

  // OpenAI-like keys: sk-....
  out = ReplaceAllRegex(out, std::wregex(LR"(sk-[A-Za-z0-9]{10,})"), L"[REDACTED_API_KEY]");

  // Bearer tokens
  out = ReplaceAllRegex(out, std::wregex(LR"(Bearer\s+[A-Za-z0-9\-\._~\+\/]+=*)"), L"Bearer [REDACTED_TOKEN]");

  // AWS Access Key ID (common)
  out = ReplaceAllRegex(out, std::wregex(LR"(AKIA[0-9A-Z]{16})"), L"[REDACTED_AWS_KEY]");

  // Private key blocks (very rough)
  out = ReplaceAllRegex(out, std::wregex(LR"(-----BEGIN [A-Z ]+PRIVATE KEY-----[\s\S]*?-----END [A-Z ]+PRIVATE KEY-----)"),
                        L"[REDACTED_PRIVATE_KEY_BLOCK]");

  // Generic "api_key=..." patterns
  out = ReplaceAllRegex(out, std::wregex(LR"((api[_-]?key\s*[:=]\s*)([^\s"']+))", std::regex_constants::icase),
                        L"$1[REDACTED]");

  // Generic "token=..." patterns
  out = ReplaceAllRegex(out, std::wregex(LR"((token\s*[:=]\s*)([^\s"']+))", std::regex_constants::icase),
                        L"$1[REDACTED]");

  return out;
}
