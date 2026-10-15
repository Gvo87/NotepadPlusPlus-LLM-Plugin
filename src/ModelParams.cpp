#include "ModelParams.h"

#include <algorithm>
#include <cwctype>

namespace {
std::wstring toLower(const std::wstring& text)
{
    std::wstring lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
    return lower;
}

bool startsWith(const std::wstring& text, const wchar_t* prefix)
{
    return text.rfind(prefix, 0) == 0;
}
}

bool isReasoningModel(const std::wstring& model)
{
    const std::wstring name = toLower(model);

    // Only the legacy GPT-3.5/GPT-4 families (gpt-4o, gpt-4.1, ...) and the
    // chat-tuned "*-chat-latest" variants keep the classic params. Any other
    // GPT model is treated as reasoning so new releases (gpt-6-luna, ...)
    // work without a plugin update.
    if (startsWith(name, L"gpt-3") || startsWith(name, L"gpt-4") || name.find(L"-chat") != std::wstring::npos) {
        return false;
    }
    return startsWith(name, L"gpt-") || startsWith(name, L"o1") || startsWith(name, L"o3")
        || startsWith(name, L"o4");
}

bool isRetiredModel(const std::wstring& model)
{
    const std::wstring name = toLower(model);

    // See https://developers.openai.com/api/docs/deprecations.
    return startsWith(name, L"gpt-3") || startsWith(name, L"gpt-4")
        || name == L"gpt-5" || startsWith(name, L"gpt-5-")
        || name == L"gpt-5.1" || startsWith(name, L"gpt-5.1-")
        || name.find(L"-chat-latest") != std::wstring::npos;
}
