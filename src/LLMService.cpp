#include "LLMService.h"
#include "SimpleJson.h"
#include "TextCodec.h"

#include <mutex>
#include <sstream>

#pragma comment(lib, "wininet.lib")

namespace {
constexpr DWORD kRequestTimeoutMilliseconds = 30000;

std::wstring lastErrorMessage(const wchar_t* operation)
{
    const DWORD error = GetLastError();
    return std::wstring(operation) + L" failed (Win32 error " + std::to_wstring(error) + L").";
}
}

LLMService* LLMService::instance = nullptr;

LLMService::LLMService() : apiUrl(L"https://api.openai.com/v1/chat/completions")
{
}

LLMService::~LLMService()
{
}

LLMService& LLMService::getInstance()
{
    static std::mutex instanceMutex;
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance = new LLMService();
    }
    return *instance;
}

void LLMService::initialize()
{
    loadSettings();
}

void LLMService::cleanup()
{
    saveSettings();
    delete instance;
    instance = nullptr;
}

void LLMService::setApiKey(const std::wstring& key)
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    apiKey = key;
}

std::wstring LLMService::getApiKey() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return apiKey;
}

bool LLMService::isConfigured() const
{
    return !getApiKey().empty();
}

LLMResult LLMService::callLLM(const std::wstring& prompt, const std::wstring& model)
{
    return callLLMWithApiKey(prompt, getApiKey(), model);
}

LLMResult LLMService::callLLMWithApiKey(
    const std::wstring& prompt, const std::wstring& configuredApiKey, const std::wstring& model)
{
    if (configuredApiKey.empty()) {
        return LLMResult::Failure(L"OpenAI API key is not configured. Set it in the plugin settings.");
    }

    const std::string modelUtf8 = wideToUtf8(model);
    const std::string promptUtf8 = wideToUtf8(prompt);
    const std::string apiKeyUtf8 = wideToUtf8(configuredApiKey);
    const std::string urlUtf8 = wideToUtf8(apiUrl);
    if ((model.empty() || !modelUtf8.empty())
        && (prompt.empty() || !promptUtf8.empty())
        && !apiKeyUtf8.empty()
        && !urlUtf8.empty()) {
        JsonBuilder builder;
        builder.startObject();
        builder.addString("model", modelUtf8);
        builder.addNumber("max_tokens", 1000);
        builder.addNumber("temperature", 0.7);
        builder.startArray("messages");
        builder.addObjectToArray();
        builder.addString("role", "user");
        builder.addString("content", promptUtf8);
        builder.endObjectInArray();
        builder.endArray();
        builder.endObject();

        const std::string headers = "Content-Type: application/json\r\nAuthorization: Bearer "
            + apiKeyUtf8 + "\r\n";
        const HttpResponse response = makeHttpRequest(urlUtf8, headers, builder.toString());
        if (!response.succeeded) {
            return LLMResult::Failure(response.error);
        }

        if (response.statusCode < 200 || response.statusCode >= 300) {
            const LLMResult apiError = extractContentFromResponse(response.body);
            if (!apiError.success && !apiError.error.empty()) {
                return apiError;
            }
            return LLMResult::Failure(L"OpenAI returned HTTP " + std::to_wstring(response.statusCode) + L".");
        }

        return extractContentFromResponse(response.body);
    }

    return LLMResult::Failure(L"Unable to encode the request as UTF-8.");
}

LLMService::HttpResponse LLMService::makeHttpRequest(
    const std::string& url, const std::string& headers, const std::string& postData)
{
    HINTERNET internet = InternetOpenA("LLM Assistant", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!internet) {
        return { false, 0, "", lastErrorMessage(L"InternetOpen") };
    }

    const auto closeInternet = [&internet]() {
        if (internet) {
            InternetCloseHandle(internet);
            internet = nullptr;
        }
    };

    DWORD timeout = kRequestTimeoutMilliseconds;
    InternetSetOptionA(internet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(internet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(internet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    URL_COMPONENTSA components = { 0 };
    components.dwStructSize = sizeof(components);
    char hostName[256] = { 0 };
    char path[1024] = { 0 };
    components.lpszHostName = hostName;
    components.dwHostNameLength = static_cast<DWORD>(sizeof(hostName) - 1);
    components.lpszUrlPath = path;
    components.dwUrlPathLength = static_cast<DWORD>(sizeof(path) - 1);
    if (!InternetCrackUrlA(url.c_str(), 0, 0, &components)) {
        const std::wstring error = lastErrorMessage(L"InternetCrackUrl");
        closeInternet();
        return { false, 0, "", error };
    }

    HINTERNET connection = InternetConnectA(internet, hostName, components.nPort, nullptr, nullptr,
        INTERNET_SERVICE_HTTP, 0, 0);
    if (!connection) {
        const std::wstring error = lastErrorMessage(L"InternetConnect");
        closeInternet();
        return { false, 0, "", error };
    }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (components.nScheme == INTERNET_SCHEME_HTTPS) {
        flags |= INTERNET_FLAG_SECURE;
    }

    HINTERNET request = HttpOpenRequestA(connection, "POST", path, nullptr, nullptr, nullptr, flags, 0);
    if (!request) {
        const std::wstring error = lastErrorMessage(L"HttpOpenRequest");
        InternetCloseHandle(connection);
        closeInternet();
        return { false, 0, "", error };
    }

    const BOOL requestSent = HttpSendRequestA(request, headers.c_str(), static_cast<DWORD>(headers.size()),
        const_cast<char*>(postData.data()), static_cast<DWORD>(postData.size()));
    if (!requestSent) {
        const std::wstring error = lastErrorMessage(L"HttpSendRequest");
        InternetCloseHandle(request);
        InternetCloseHandle(connection);
        closeInternet();
        return { false, 0, "", error };
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!HttpQueryInfoA(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, nullptr)) {
        statusCode = 0;
    }

    std::string responseBody;
    char buffer[4096];
    DWORD bytesRead = 0;
    bool readSucceeded = true;
    while (true) {
        if (!InternetReadFile(request, buffer, sizeof(buffer), &bytesRead)) {
            readSucceeded = false;
            break;
        }
        if (bytesRead == 0) {
            break;
        }
        responseBody.append(buffer, bytesRead);
    }

    const std::wstring readError = readSucceeded ? L"" : lastErrorMessage(L"InternetReadFile");
    InternetCloseHandle(request);
    InternetCloseHandle(connection);
    closeInternet();
    if (!readSucceeded) {
        return { false, 0, "", readError };
    }

    return { true, statusCode, responseBody, L"" };
}

LLMResult LLMService::extractContentFromResponse(const std::string& jsonResponse)
{
    try {
        SimpleJson parser(jsonResponse);
        if (parser.hasError()) {
            const std::wstring error = utf8ToWide(parser.getErrorMessage());
            return LLMResult::Failure(error.empty() ? L"OpenAI returned an API error." : error);
        }

        const std::wstring content = utf8ToWide(parser.extractContent());
        if (!content.empty()) {
            return LLMResult::Success(content);
        }
        return LLMResult::Failure(L"OpenAI returned no usable content.");
    } catch (...) {
        return LLMResult::Failure(L"Unable to parse the OpenAI response.");
    }
}

void LLMService::loadSettings()
{
    HKEY key = nullptr;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\NotepadPlusPlus\\LLMAssistant", 0, KEY_READ, &key)
        != ERROR_SUCCESS) {
        return;
    }

    wchar_t buffer[512] = { 0 };
    DWORD bufferSize = sizeof(buffer) - sizeof(wchar_t);
    DWORD valueType = REG_SZ;
    if (RegQueryValueEx(key, L"ApiKey", nullptr, &valueType, reinterpret_cast<LPBYTE>(buffer), &bufferSize)
        == ERROR_SUCCESS && valueType == REG_SZ && bufferSize > 0) {
        buffer[bufferSize / sizeof(wchar_t)] = L'\0';
        setApiKey(buffer);
    }
    RegCloseKey(key);
}

void LLMService::saveSettings()
{
    const std::wstring currentApiKey = getApiKey();
    HKEY key = nullptr;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\NotepadPlusPlus\\LLMAssistant", 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return;
    }

    const DWORD dataSize = static_cast<DWORD>((currentApiKey.length() + 1) * sizeof(wchar_t));
    RegSetValueEx(key, L"ApiKey", 0, REG_SZ, reinterpret_cast<const BYTE*>(currentApiKey.c_str()), dataSize);
    RegCloseKey(key);
}
